#pragma once
// Small asset-free shader probe. Run with no live Renderer: volk's device table
// is process-wide. Results use coherent host memory after a compute/host barrier
// and a fence; see Khronos synchronization_examples.html, CPU readback example.
#include <volk.h>
#include <array>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace shadercheck {
inline void require(VkResult result, const char *operation) {
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string(operation) + ": " + std::to_string(result));
}
struct ComputeProbe {
    struct Buffer { VkBuffer handle{}; VkDeviceMemory memory{}; void *mapped = nullptr; };
    VkInstance instance{};
    VkPhysicalDevice physical{};
    VkDevice device{};
    VkQueue queue{};
    uint32_t family = 0;
    VkShaderModule shader{};
    VkDescriptorSetLayout descriptors{};
    VkDescriptorPool descriptorPool{};
    VkPipelineLayout layout{};
    VkPipeline pipeline{};
    VkCommandPool commands{};
    VkFence fence{};
    std::array<Buffer, 2> buffers{};
    ~ComputeProbe() {
        if (device) {
            vkDeviceWaitIdle(device);
            vkDestroyFence(device, fence, nullptr);
            vkDestroyCommandPool(device, commands, nullptr);
            vkDestroyPipeline(device, pipeline, nullptr);
            vkDestroyPipelineLayout(device, layout, nullptr);
            vkDestroyShaderModule(device, shader, nullptr);
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
            vkDestroyDescriptorSetLayout(device, descriptors, nullptr);
            for (auto &buffer : buffers) {
                if (buffer.mapped) vkUnmapMemory(device, buffer.memory);
                vkDestroyBuffer(device, buffer.handle, nullptr);
                vkFreeMemory(device, buffer.memory, nullptr);
            }
            vkDestroyDevice(device, nullptr);
        }
        if (instance) vkDestroyInstance(instance, nullptr);
    }
    void start() {
        require(volkInitialize(), "volkInitialize");
        VkApplicationInfo application{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        application.pApplicationName = "CubeVulkan shader probe";
        application.apiVersion = VK_API_VERSION_1_3;
        VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        create.pApplicationInfo = &application;
        require(vkCreateInstance(&create, nullptr, &instance), "vkCreateInstance");
        volkLoadInstance(instance);
        uint32_t count = 0;
        require(vkEnumeratePhysicalDevices(instance, &count, nullptr), "enumerate devices");
        std::vector<VkPhysicalDevice> candidates(count);
        require(vkEnumeratePhysicalDevices(instance, &count, candidates.data()), "enumerate devices");
        for (auto candidate : candidates) {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(candidate, &properties);
            if (properties.apiVersion < VK_API_VERSION_1_3) continue;
            VkPhysicalDeviceVulkan13Features supported{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
            VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
            features.pNext = &supported;
            vkGetPhysicalDeviceFeatures2(candidate, &features);
            if (!supported.maintenance4) continue;
            uint32_t families = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &families, nullptr);
            std::vector<VkQueueFamilyProperties> queues(families);
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &families, queues.data());
            for (uint32_t i = 0; i < families; ++i)
                if (queues[i].queueCount && (queues[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                    physical = candidate;
                    family = i;
                    break;
                }
            if (physical) break;
        }
        if (!physical) throw std::runtime_error("Vulkan 1.3 compute queue unavailable");
        const float priority = 1;
        VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueInfo.queueFamilyIndex = family;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;
        VkPhysicalDeviceVulkan13Features features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        features.maintenance4 = VK_TRUE;
        VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        deviceInfo.pNext = &features;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        require(vkCreateDevice(physical, &deviceInfo, nullptr, &device), "vkCreateDevice");
        volkLoadDevice(device);
        vkGetDeviceQueue(device, family, 0, &queue);
    }
    void buffer(Buffer &target, VkDeviceSize size) {
        VkBufferCreateInfo create{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        create.size = size;
        create.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        require(vkCreateBuffer(device, &create, nullptr, &target.handle), "vkCreateBuffer");
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(device, target.handle, &requirements);
        VkPhysicalDeviceMemoryProperties properties;
        vkGetPhysicalDeviceMemoryProperties(physical, &properties);
        uint32_t type = properties.memoryTypeCount;
        const auto flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        for (uint32_t i = 0; i < properties.memoryTypeCount; ++i)
            if ((requirements.memoryTypeBits & (1u << i)) && (properties.memoryTypes[i].propertyFlags & flags) == flags) {
                type = i;
                break;
            }
        if (type == properties.memoryTypeCount) throw std::runtime_error("Coherent shader-probe memory unavailable");
        VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocate.allocationSize = requirements.size;
        allocate.memoryTypeIndex = type;
        require(vkAllocateMemory(device, &allocate, nullptr, &target.memory), "vkAllocateMemory");
        require(vkBindBufferMemory(device, target.handle, target.memory, 0), "vkBindBufferMemory");
        require(vkMapMemory(device, target.memory, 0, VK_WHOLE_SIZE, 0, &target.mapped), "vkMapMemory");
    }
};
inline std::vector<std::array<float, 4>> compute(const uint32_t *code, size_t codeBytes,
                                               const void *input, size_t inputBytes, uint32_t count) {
    if (!input || !inputBytes || !count) throw std::runtime_error("Empty shader probe");
    ComputeProbe gpu;
    gpu.start();
    gpu.buffer(gpu.buffers[0], inputBytes);
    gpu.buffer(gpu.buffers[1], size_t(count) * 4 * sizeof(float));
    std::memcpy(gpu.buffers[0].mapped, input, inputBytes);
    VkDescriptorSetLayoutBinding bindings[2]{};
    for (uint32_t i = 0; i < 2; ++i)
        bindings[i] = {i, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
    VkDescriptorSetLayoutCreateInfo descriptors{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    descriptors.bindingCount = 2;
    descriptors.pBindings = bindings;
    require(vkCreateDescriptorSetLayout(gpu.device, &descriptors, nullptr, &gpu.descriptors), "descriptor layout");
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout.setLayoutCount = 1;
    layout.pSetLayouts = &gpu.descriptors;
    require(vkCreatePipelineLayout(gpu.device, &layout, nullptr, &gpu.layout), "pipeline layout");
    VkShaderModuleCreateInfo module{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    module.codeSize = codeBytes;
    module.pCode = code;
    require(vkCreateShaderModule(gpu.device, &module, nullptr, &gpu.shader), "shader module");
    VkComputePipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    pipeline.layout = gpu.layout;
    pipeline.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    pipeline.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline.stage.module = gpu.shader;
    pipeline.stage.pName = "main";
    require(vkCreateComputePipelines(gpu.device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &gpu.pipeline), "compute pipeline");
    VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool.maxSets = 1;
    pool.poolSizeCount = 1;
    pool.pPoolSizes = &poolSize;
    require(vkCreateDescriptorPool(gpu.device, &pool, nullptr, &gpu.descriptorPool), "descriptor pool");
    VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocate.descriptorPool = gpu.descriptorPool;
    allocate.descriptorSetCount = 1;
    allocate.pSetLayouts = &gpu.descriptors;
    VkDescriptorSet set;
    require(vkAllocateDescriptorSets(gpu.device, &allocate, &set), "descriptor set");
    VkDescriptorBufferInfo info[2]{{gpu.buffers[0].handle, 0, inputBytes},
                                 {gpu.buffers[1].handle, 0, size_t(count) * 4 * sizeof(float)}};
    VkWriteDescriptorSet writes[2]{};
    for (uint32_t i = 0; i < 2; ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = set;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].pBufferInfo = &info[i];
    }
    vkUpdateDescriptorSets(gpu.device, 2, writes, 0, nullptr);
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = gpu.family;
    require(vkCreateCommandPool(gpu.device, &poolInfo, nullptr, &gpu.commands), "command pool");
    VkCommandBufferAllocateInfo commandInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commandInfo.commandPool = gpu.commands;
    commandInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandInfo.commandBufferCount = 1;
    VkCommandBuffer command;
    require(vkAllocateCommandBuffers(gpu.device, &commandInfo, &command), "command buffer");
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    require(vkBeginCommandBuffer(command, &begin), "begin commands");
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, gpu.pipeline);
    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_COMPUTE, gpu.layout, 0, 1, &set, 0, nullptr);
    vkCmdDispatch(command, count, 1, 1);
    VkMemoryBarrier readable{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    readable.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    readable.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0,
                         1, &readable, 0, nullptr, 0, nullptr);
    require(vkEndCommandBuffer(command), "end commands");
    VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    require(vkCreateFence(gpu.device, &fence, nullptr, &gpu.fence), "create fence");
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command;
    require(vkQueueSubmit(gpu.queue, 1, &submit, gpu.fence), "compute submit");
    require(vkWaitForFences(gpu.device, 1, &gpu.fence, VK_TRUE, 10000000000ULL), "compute fence");
    std::vector<std::array<float, 4>> result(count);
    std::memcpy(result.data(), gpu.buffers[1].mapped, result.size() * sizeof(result.front()));
    return result;
}
} // namespace shadercheck
