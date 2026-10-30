#include "Renderer.h"
#include "VulkanSync.h"
#include <algorithm>
#include <array>
#include <cstring>
namespace {
// std140 layout, shared with omega_water.frag. No game data is stored here.
struct WaterUniforms {
    glm::vec4 optics, body, absorption, screen, fog, sky;
};
} // namespace

bool Renderer::createWaterResources(uint32_t scenePushBytes, std::string &error) {
    const VkDescriptorSetLayoutBinding bindings[] = {{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
                                                     {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
                                                     {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT}};
    VkDescriptorSetLayoutCreateInfo setInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    setInfo.bindingCount = 3;
    setInfo.pBindings = bindings;
    if (vkCreateDescriptorSetLayout(device, &setInfo, nullptr, &waterSetLayout) != VK_SUCCESS) {
        error = "Cannot create the water image bindings";
        return false;
    }
    const VkDescriptorSetLayout sets[] = {setLayout, waterSetLayout};
    const VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, scenePushBytes};
    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutInfo.setLayoutCount = 2;
    layoutInfo.pSetLayouts = sets;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &push;
    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &waterLayout) != VK_SUCCESS) {
        error = "Cannot create the water pipeline layout";
        return false;
    }
    const VkDescriptorPoolSize sizes[] = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 * FRAMES},
                                          {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, FRAMES}};
    VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.maxSets = FRAMES;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = sizes;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &waterPool) != VK_SUCCESS) {
        error = "Cannot create the water descriptor pool";
        return false;
    }
    for (auto &frame : waterFrames) {
        frame.uniforms = createBuffer(sizeof(WaterUniforms), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocation.descriptorPool = waterPool;
        allocation.descriptorSetCount = 1;
        allocation.pSetLayouts = &waterSetLayout;
        if (!frame.uniforms.mapped || vkAllocateDescriptorSets(device, &allocation, &frame.set) != VK_SUCCESS) {
            error = "Cannot allocate the water frame data";
            return false;
        }
    }
    return true;
}

bool Renderer::createWaterTargets() {
    const bool shared = settings.water.style == waterlook::Style::Omega;
    const uint32_t width = shared ? extent.width : 1, height = shared ? extent.height : 1;
    const auto usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    waterColour = createImage(width, height, swapFormat, usage, VK_IMAGE_ASPECT_COLOR_BIT);
    waterDepth = createImage(width, height, DEPTH_FORMAT, usage, VK_IMAGE_ASPECT_DEPTH_BIT);
    waterSnapshotReady = false;
    if (!waterColour.image || !waterDepth.image)
        return false;
    const VkDescriptorImageInfo images[] = {{linearSampler, waterColour.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                                            {pointSampler, waterDepth.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
    for (const auto &frame : waterFrames) {
        const VkDescriptorBufferInfo data{frame.uniforms.buffer, 0, sizeof(WaterUniforms)};
        VkWriteDescriptorSet writes[3]{};
        for (uint32_t index = 0; index < 3; ++index) {
            auto &write = writes[index];
            write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet = frame.set;
            write.dstBinding = index;
            write.descriptorCount = 1;
            write.descriptorType = index < 2 ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            if (index < 2)
                write.pImageInfo = &images[index];
            else
                write.pBufferInfo = &data;
        }
        vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);
    }
    return true;
}

void Renderer::destroyWaterTargets() {
    destroyImage(waterColour);
    destroyImage(waterDepth);
    waterSnapshotReady = false;
}

void Renderer::destroyWaterResources() {
    destroyWaterTargets();
    for (auto &frame : waterFrames)
        destroyBuffer(frame.uniforms);
    vkDestroyDescriptorPool(device, waterPool, nullptr);
    vkDestroyPipelineLayout(device, waterLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, waterSetLayout, nullptr);
    waterPool = VK_NULL_HANDLE;
    waterLayout = VK_NULL_HANDLE;
    waterSetLayout = VK_NULL_HANDLE;
}

void Renderer::updateWaterUniforms(float nearPlane, float farPlane, float sunlight, bool submerged, glm::vec3 fog, float fogRange,
                                   double time) {
    const float brightness = .2f + .8f * std::clamp(sunlight, 0.f, 1.f);
    // Seen from below, a long path through the surface tends toward the same
    // cobalt haze as the submerged scene, rather than the shallow surface blue.
    const auto scattering = submerged ? glm::vec4(fog, 1.f) : glm::vec4(.015f, .27f, .72f, brightness);
    const WaterUniforms data{{nearPlane, farPlane, (float)settings.water.reflectionSteps, settings.water.reflections ? .85f : 0.f},
                             scattering,
                             {.38f, .23f, .06f, 0},
                             {1.f / extent.width, 1.f / extent.height, submerged ? 1.f : 0.f, (float)time},
                             {fog, std::max(fogRange, .001f)},
                             {glm::vec3(.12f, .35f, .62f) * brightness, 1}};
    std::memcpy(waterFrames[frameIndex].uniforms.mapped, &data, sizeof data);
}

void Renderer::snapshotWaterBackground(VkCommandBuffer cmd) {
    using vksync::apply;
    using vksync::barrier;
    const VkPipelineStageFlags2 depthStages = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    const auto oldSnapshotLayout = waterSnapshotReady ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
    const auto oldSnapshotStage = waterSnapshotReady ? VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_2_NONE;
    const auto oldSnapshotAccess = waterSnapshotReady ? VK_ACCESS_2_SHADER_SAMPLED_READ_BIT : VK_ACCESS_2_NONE;
    const VkImageMemoryBarrier2 before[] = {
        barrier(sceneColor.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                VK_ACCESS_2_TRANSFER_READ_BIT),
        barrier(depthImages[parity].image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, depthStages, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT),
        barrier(waterColour.image, VK_IMAGE_ASPECT_COLOR_BIT, oldSnapshotLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, oldSnapshotStage,
                oldSnapshotAccess, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT),
        barrier(waterDepth.image, VK_IMAGE_ASPECT_DEPTH_BIT, oldSnapshotLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, oldSnapshotStage,
                oldSnapshotAccess, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT)};
    apply(cmd, before, 4);
    auto copy = [&](VkImage source, VkImage destination, VkImageAspectFlags aspect) {
        VkImageCopy region{};
        region.srcSubresource = region.dstSubresource = {aspect, 0, 0, 1};
        region.extent = {extent.width, extent.height, 1};
        vkCmdCopyImage(cmd, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    };
    copy(sceneColor.image, waterColour.image, VK_IMAGE_ASPECT_COLOR_BIT);
    copy(depthImages[parity].image, waterDepth.image, VK_IMAGE_ASPECT_DEPTH_BIT);
    const VkImageMemoryBarrier2 after[] = {
        barrier(sceneColor.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT),
        barrier(depthImages[parity].image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, depthStages,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT),
        barrier(waterColour.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT),
        barrier(waterDepth.image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                VK_ACCESS_2_SHADER_SAMPLED_READ_BIT)};
    apply(cmd, after, 4);
    waterSnapshotReady = true;
}
