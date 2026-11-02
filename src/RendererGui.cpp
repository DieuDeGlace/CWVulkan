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
        // Character requests include an admitted setup-only entry for empty poses.
        // Queued Ring/QueuedModel descriptors were prepared by the earlier overlay pass, before the map.
        const bool characterSetup = request.kind == guiitem::Request::Kind::Character;
        const bool queued = request.kind == guiitem::Request::Kind::Ring || request.kind == guiitem::Request::Kind::QueuedModel;
        if (characterSetup || (request.mesh && !queued))
            steamFog.setDistance(10000);
        guiitem::appendDraws(request, {(int)extent.width, (int)extent.height}, retained, guiDraws);
        guiGroups.emplace_back(std::min(request.beforeVertex, overlayVertices.size()), guiDraws.size());
    }
    const bool models = !guiDraws.empty() && prepareGui(guiDraws.size());
    const auto maximum = plasma::layerDepth(overlayLayers, {overlayVertices.size(), guiRequests.size()});
    if (!maximum || !prepareClipLayers(*maximum) || !prepareCachedLayers())
        return; // defined engine failure policy: leave the resolved scene intact
    for (const auto &command : overlayLayers) {
        const auto at = command.before;
        if ((at.items && guiGroups[at.items - 1].first > at.vertices) ||
            (at.items < guiGroups.size() && guiGroups[at.items].first < at.vertices))
            return; // vertex and callback cuts must describe the same append order
    }
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
            uniforms.projection = flipY * draw.projection;
            uniforms.viewProj = flipY * draw.projection * draw.view;
            uniforms.front = draw.state.front;
            uniforms.back = draw.state.back;
            uniforms.ambient = draw.state.ambient;
            uniforms.steamSun = glm::vec4(draw.state.direction, draw.state.ambient.a);
            uniforms.steamMaterial = draw.material;
            if (draw.state.whiteSky)
                for (int colour = 0; colour < 4; ++colour)
                    uniforms.steamSky[colour] = glm::vec4(1);
            uniforms.fog.w = 10000;
            uniforms.steamSky[5].w = 1.f / 10000;
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
    VkViewport viewport{0, 0, (float)extent.width, (float)extent.height, 0, 1};
    VkRect2D scissor{{0, 0}, extent};
    size_t target = 0, maskSource = 0;
    CachedLayer *cacheTarget = nullptr;
    uint32_t maskMode = 0;
    const auto beginTarget = [&](bool clear) {
        const bool layer = target != 0 || cacheTarget;
        const VkExtent2D dimensions = !target && cacheTarget ? cacheTarget->extent : extent;
        rendering.renderArea.extent = dimensions;
        viewport.width = (float)dimensions.width;
        viewport.height = (float)dimensions.height;
        scissor.extent = dimensions;
        colour.imageView = target        ? clipLayers[target - 1].colour.view
                           : cacheTarget ? cacheTarget->target.colour.view
                                         : swapViews[imageIndex];
        colour.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        colour.clearValue.color = {{0, 0, 0, 0}};
        rendering.pDepthAttachment = !layer && models ? &depth : nullptr;
        if (!clear) {
            const VkImage image = target        ? clipLayers[target - 1].colour.image
                                  : cacheTarget ? cacheTarget->target.colour.image
                                                : swapImages[imageIndex];
            barriers(cmd, {imageBarrier(image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                        VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT)});
            if (!layer && models)
                barriers(cmd,
                         {imageBarrier(guiDepth.image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                       VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                       VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                                       VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                       VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                                       VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)});
        }
        vkCmdBeginRendering(cmd, &rendering);
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    };
    beginTarget(false);
    const bool text = uploadOverlay();
    size_t vertex = 0, drawIndex = 0, request = 0;
    const auto trianglesTo = [&](size_t end) {
        end = std::max(vertex, end);
        if (text)
            drawOverlayRange(cmd, uint32_t(vertex), uint32_t(end - vertex), models && !target && !cacheTarget, target != 0 || cacheTarget,
                             maskMode ? clipLayers[maskSource].set : VK_NULL_HANDLE, maskMode);
        vertex = end;
    };
    const auto drawUntil = [&](plasma::DrawPoint point) {
        for (; request < point.items; ++request) {
            const auto [before, end] = guiGroups[request];
            trianglesTo(before);
            for (; models && drawIndex < end; ++drawIndex) {
                const auto &draw = guiDraws[drawIndex];
                if (!draw.mesh || draw.mesh >= meshes.size() || !meshes[draw.mesh].live || !meshes[draw.mesh].indexCount)
                    continue;
                const auto &mesh = meshes[draw.mesh];
                const VkPipeline pipeline = (target || cacheTarget) ? (draw.mirrored ? layerMirroredModelPipeline : layerModelPipeline)
                                                                    : (draw.mirrored ? guiMirroredModelPipeline : guiModelPipeline);
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                const auto set = guiFrames[frameIndex].sets[drawIndex];
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &set, 0, nullptr);
                const VkDeviceSize zero = 0;
                vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.buffer.buffer, &zero);
                vkCmdBindIndexBuffer(cmd, mesh.buffer.buffer, mesh.indexOffset, VK_INDEX_TYPE_UINT32);
                rendergpu::PushConstants push{draw.model, glm::vec4(float(STEAM_WORLD | GUI_WORLD), 0, draw.depthOffset, draw.opacity),
                                              glm::vec4(1, draw.state.white, draw.state.shininess, draw.state.glow), glm::uvec4(0)};
                vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof push, &push);
                vkCmdDrawIndexed(cmd, mesh.indexCount, 1, 0, 0, 0);
            }
        }
        trianglesTo(point.vertices);
    };
    for (const auto &command : overlayLayers) {
        drawUntil(command.before);
        if (command.kind == plasma::LayerCommand::Kind::Mask) {
            maskMode = command.mode;
            continue;
        }
        if (command.kind == plasma::LayerCommand::Kind::CacheCopy) {
            maskMode = 0;
            drawCachedLayer(cmd, command, target != 0, models && !target);
            continue;
        }
        vkCmdEndRendering(cmd);
        maskMode = 0;
        if (command.kind == plasma::LayerCommand::Kind::CacheBegin) {
            cacheTarget = &cachedLayers.at(CacheKey(command.surface));
            guiTargetRectangle = glm::vec4(command.rectangle);
            auto &layer = cacheTarget->target;
            barriers(cmd, {imageBarrier(layer.colour.image, VK_IMAGE_ASPECT_COLOR_BIT,
                                        layer.sampled ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
                                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                        layer.sampled ? VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_2_NONE,
                                        layer.sampled ? VK_ACCESS_2_SHADER_SAMPLED_READ_BIT : 0,
                                        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT)});
            beginTarget(true);
        } else if (command.kind == plasma::LayerCommand::Kind::CacheEnd) {
            auto &layer = cacheTarget->target;
            barriers(cmd, {imageBarrier(layer.colour.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                        VK_ACCESS_2_SHADER_SAMPLED_READ_BIT)});
            layer.sampled = true;
            command.surface->painted = true;
            cacheTarget = nullptr;
            guiTargetRectangle = {};
            beginTarget(false);
        } else if (command.kind == plasma::LayerCommand::Kind::Push) {
            auto &layer = clipLayers[target++];
            barriers(cmd, {imageBarrier(layer.colour.image, VK_IMAGE_ASPECT_COLOR_BIT,
                                        layer.sampled ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
                                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                        layer.sampled ? VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_2_NONE,
                                        layer.sampled ? VK_ACCESS_2_SHADER_SAMPLED_READ_BIT : 0,
                                        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT)});
            beginTarget(true);
        } else {
            maskSource = --target;
            auto &layer = clipLayers[maskSource];
            barriers(cmd, {imageBarrier(layer.colour.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                        VK_ACCESS_2_SHADER_SAMPLED_READ_BIT)});
            layer.sampled = true;
            beginTarget(false);
        }
    }
    drawUntil({overlayVertices.size(), guiRequests.size()});
    vkCmdEndRendering(cmd);
}

void Renderer::destroyGui() {
    destroyClipLayers();
    destroyCachedLayers();
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
