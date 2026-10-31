#include "Renderer.h"
#include "RendererUniforms.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cstring>

namespace {
VkImageMemoryBarrier2 imageBarrier(VkImage image, VkImageAspectFlags aspect, VkImageLayout from, VkImageLayout to,
                                   VkPipelineStageFlags2 before, VkAccessFlags2 read, VkPipelineStageFlags2 after, VkAccessFlags2 write) {
    VkImageMemoryBarrier2 b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    b.srcStageMask = before;
    b.srcAccessMask = read;
    b.dstStageMask = after;
    b.dstAccessMask = write;
    b.oldLayout = from;
    b.newLayout = to;
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = image;
    b.subresourceRange = {aspect, 0, 1, 0, 1};
    return b;
}
void barriers(VkCommandBuffer cmd, std::initializer_list<VkImageMemoryBarrier2> images) {
    VkDependencyInfo dependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency.imageMemoryBarrierCount = (uint32_t)images.size();
    dependency.pImageMemoryBarriers = images.begin();
    vkCmdPipelineBarrier2(cmd, &dependency);
}
} // namespace

bool Renderer::prepareGui(size_t count) {
    if (!guiDepth.image || !guiDepth.view) {
        destroyImage(guiDepth);
        guiDepth = createImage(extent.width, extent.height, DEPTH_FORMAT,
                               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
        guiDepthFresh = true;
        if (!guiDepth.image || !guiDepth.view)
            return false;
    }
    if (!guiZeroPoints.buffer) {
        guiZeroPoints = createBuffer(16 * sizeof(rendergpu::NativeGpuPoint), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (!guiZeroPoints.mapped) {
            destroyBuffer(guiZeroPoints);
            return false;
        }
        std::memset(guiZeroPoints.mapped, 0, 16 * sizeof(rendergpu::NativeGpuPoint));
    }
    if (!guiUniformStride) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(gpu, &properties);
        const auto alignment = std::max<VkDeviceSize>(1, properties.limits.minUniformBufferOffsetAlignment);
        guiUniformStride = (sizeof(rendergpu::FrameUniforms) + alignment - 1) / alignment * alignment;
    }
    GuiFrame &frame = guiFrames[frameIndex]; // the frame slot's previous fence has completed
    bool made = false;
    if (frame.sets.size() < count) {
        destroyBuffer(frame.uniforms);
        if (frame.pool)
            vkDestroyDescriptorPool(device, frame.pool, nullptr);
        frame.pool = VK_NULL_HANDLE;
        frame.sets.clear();
        const uint32_t capacity = (uint32_t)std::max<size_t>(64, count * 2);
        frame.uniforms = createBuffer(guiUniformStride * capacity, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (!frame.uniforms.mapped)
            return false;
        VkDescriptorPoolSize sizes[] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, capacity},
                                        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 * capacity},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, capacity}};
        VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        poolInfo.maxSets = capacity;
        poolInfo.poolSizeCount = 3;
        poolInfo.pPoolSizes = sizes;
        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &frame.pool) != VK_SUCCESS)
            return false;
        std::vector<VkDescriptorSetLayout> layouts(capacity, setLayout);
        VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocate.descriptorPool = frame.pool;
        allocate.descriptorSetCount = capacity;
        allocate.pSetLayouts = layouts.data();
        frame.sets.resize(capacity);
        if (vkAllocateDescriptorSets(device, &allocate, frame.sets.data()) != VK_SUCCESS) {
            frame.sets.clear();
            return false;
        }
        made = true;
    }
    if (made || frame.farView != shadowFar.view || frame.nearView != shadowNear.view) {
        const VkDescriptorImageInfo farInfo{shadowSampler, shadowFar.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        const VkDescriptorImageInfo nearInfo{shadowSampler, shadowNear.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        const VkDescriptorBufferInfo points{guiZeroPoints.buffer, 0, 16 * sizeof(rendergpu::NativeGpuPoint)};
        std::vector<VkDescriptorBufferInfo> uniforms(frame.sets.size());
        std::vector<VkWriteDescriptorSet> writes;
        writes.reserve(frame.sets.size() * 4);
        for (size_t i = 0; i < frame.sets.size(); i++) {
            auto append = [&](uint32_t binding, VkDescriptorType type, const VkDescriptorBufferInfo *buffer,
                              const VkDescriptorImageInfo *image) {
                VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                write.dstSet = frame.sets[i];
                write.dstBinding = binding;
                write.descriptorCount = 1;
                write.descriptorType = type;
                write.pBufferInfo = buffer;
                write.pImageInfo = image;
                writes.push_back(write);
            };
            if (made) {
                uniforms[i] = {frame.uniforms.buffer, guiUniformStride * i, sizeof(rendergpu::FrameUniforms)};
                append(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uniforms[i], nullptr);
                append(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &points, nullptr);
            }
            append(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &farInfo);
            append(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &nearInfo);
        }
        vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
        frame.farView = shadowFar.view;
        frame.nearView = shadowNear.view;
    }
    return true;
}

void Renderer::recordGui(VkCommandBuffer cmd, uint32_t imageIndex) {
    guiDraws.clear();
    guiGroups.clear();
    auto retained = guiitem::overlayState();
    for (const auto &request : guiRequests) {
        guiitem::appendDraws(request, {(int)extent.width, (int)extent.height}, retained, guiDraws);
        guiGroups.emplace_back(std::min(request.beforeVertex, overlayVertices.size()), guiDraws.size());
    }
    const bool models = !guiDraws.empty() && prepareGui(guiDraws.size());
    // Fullscreen resolve and the following load are separate rendering instances.
    barriers(cmd, {imageBarrier(swapImages[imageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT)});
    if (models) {
        // Retain the native scene/overlay depth without putting GUI writes into TAA's
        // history depth. No per-item clear, and no clear between black/main/cube draws.
        barriers(
            cmd,
            {imageBarrier(depthImages[parity].image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                          VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT),
             imageBarrier(
                 guiDepth.image, VK_IMAGE_ASPECT_DEPTH_BIT,
                 guiDepthFresh ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 guiDepthFresh ? VK_PIPELINE_STAGE_2_NONE
                               : VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                 guiDepthFresh ? 0 : VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                 VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT)});
        VkImageCopy copy{};
        copy.srcSubresource = copy.dstSubresource = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 0, 1};
        copy.extent = {extent.width, extent.height, 1};
        vkCmdCopyImage(cmd, depthImages[parity].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, guiDepth.image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
        barriers(cmd, {imageBarrier(depthImages[parity].image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
                                    VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT),
                       imageBarrier(guiDepth.image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                    VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                                    VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)});
        guiDepthFresh = false;
        rendergpu::FrameUniforms base;
        std::memcpy(&base, frames[frameIndex].uniforms.mapped, sizeof base);
        const glm::mat4 flipY = glm::scale(glm::mat4(1), glm::vec3(1, -1, 1));
        for (size_t i = 0; i < guiDraws.size(); i++) {
            const auto &draw = guiDraws[i];
            auto uniforms = base;
            uniforms.view = draw.view;
            uniforms.viewProj = flipY * draw.projection * draw.view;
            uniforms.front = draw.state.front;
            uniforms.back = draw.state.back;
            uniforms.ambient = draw.state.ambient;
            uniforms.steamSun = glm::vec4(draw.state.direction, draw.state.ambient.a);
            uniforms.steamMaterial = draw.material;
            uniforms.fog.w = 10000;
            uniforms.lightDir.w = 0; // optional world shadows do not shade the native GUI
            uniforms.wind.w = 0;
            uniforms.alphaView.w = 0; // project underwater grading is for the world, not UI
            std::memcpy((uint8_t *)guiFrames[frameIndex].uniforms.mapped + guiUniformStride * i, &uniforms, sizeof uniforms);
        }
    }
    VkRenderingAttachmentInfo colour{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    colour.imageView = swapViews[imageIndex];
    colour.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colour.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colour.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkRenderingAttachmentInfo depth{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    depth.imageView = guiDepth.view;
    depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkRenderingInfo rendering{VK_STRUCTURE_TYPE_RENDERING_INFO};
    rendering.renderArea = {{0, 0}, extent};
    rendering.layerCount = 1;
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachments = &colour;
    rendering.pDepthAttachment = models ? &depth : nullptr;
    vkCmdBeginRendering(cmd, &rendering);
    VkViewport viewport{0, 0, (float)extent.width, (float)extent.height, 0, 1};
    VkRect2D scissor{{0, 0}, extent};
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    const bool text = uploadOverlay();
    size_t vertex = 0, drawIndex = 0;
    for (const auto &[before, end] : guiGroups) {
        const size_t boundary = std::max(vertex, before);
        if (text)
            drawOverlayRange(cmd, (uint32_t)vertex, (uint32_t)(boundary - vertex), models);
        if (models)
            for (; drawIndex < end; drawIndex++) {
                const auto &draw = guiDraws[drawIndex];
                if (!draw.mesh || draw.mesh >= meshes.size() || !meshes[draw.mesh].live || !meshes[draw.mesh].indexCount)
                    continue;
                const auto &mesh = meshes[draw.mesh];
                // Native cull-front/clockwise is equivalent to keep-counterclockwise here.
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, guiModelPipeline);
                const auto set = guiFrames[frameIndex].sets[drawIndex];
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &set, 0, nullptr);
                const VkDeviceSize zero = 0;
                vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.buffer.buffer, &zero);
                vkCmdBindIndexBuffer(cmd, mesh.buffer.buffer, mesh.indexOffset, VK_INDEX_TYPE_UINT32);
                rendergpu::PushConstants push{draw.model, glm::vec4((float)(STEAM_WORLD | GUI_WORLD), 0, draw.depthOffset, 1),
                                              glm::vec4(1, draw.state.white, draw.state.shininess, draw.state.glow), glm::uvec4(0)};
                vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof push, &push);
                vkCmdDrawIndexed(cmd, mesh.indexCount, 1, 0, 0, 0);
            }
        vertex = boundary;
    }
    if (text)
        drawOverlayRange(cmd, (uint32_t)vertex, (uint32_t)(overlayVertices.size() - vertex), models);
    vkCmdEndRendering(cmd);
}

void Renderer::destroyGui() {
    destroyImage(guiDepth);
    guiDepthFresh = true;
    destroyBuffer(guiZeroPoints);
    for (auto &frame : guiFrames) {
        destroyBuffer(frame.uniforms);
        if (frame.pool)
            vkDestroyDescriptorPool(device, frame.pool, nullptr);
        frame.pool = VK_NULL_HANDLE;
        frame.sets.clear();
    }
}
