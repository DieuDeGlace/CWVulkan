// Retained Widget color targets, separate from the depth-indexed fullscreen clip pool.
#include "Renderer.h"
#include <cstring>

Renderer::OverlayPush Renderer::overlayPush(uint32_t mode) const {
    static_assert(sizeof(OverlayPush) == 80);
    OverlayPush push;
    push.size = {(float)extent.width, (float)extent.height};
    push.mode = (int32_t)mode;
    push.bias = mode ? .13500000536441803f : 0;
    push.target = guiTargetRectangle.z > 0 ? guiTargetRectangle : glm::vec4(0, 0, extent.width, extent.height);
    return push;
}

bool Renderer::prepareCachedLayers() {
    bool waited = false;
    const auto wait = [&] {
        if (!waited) {
            vkDeviceWaitIdle(device);
            waited = true;
        }
    };
    const auto release = [&](CachedLayer &entry) {
        if (entry.target.pool)
            vkDestroyDescriptorPool(device, entry.target.pool, nullptr);
        destroyImage(entry.target.colour);
        entry = {};
    };
    for (auto it = cachedLayers.begin(); it != cachedLayers.end();) {
        if (!it->first.expired()) {
            ++it;
            continue;
        }
        wait(); // Descriptors/images may still be sampled by an earlier in-flight frame.
        release(it->second);
        it = cachedLayers.erase(it);
    }
    for (const auto &command : overlayLayers) {
        if (command.kind != plasma::LayerCommand::Kind::CacheBegin)
            continue;
        auto &entry = cachedLayers[CacheKey(command.surface)];
        const VkExtent2D size{uint32_t(command.rectangle.z), uint32_t(command.rectangle.w)};
        if (entry.target.set && entry.target.colour.image && entry.extent.width == size.width && entry.extent.height == size.height)
            continue;
        if (entry.target.colour.image) {
            wait();
            release(entry);
        }
        command.surface->painted = false;
        entry.extent = size;
        entry.target.colour = createImage(size.width, size.height, VK_FORMAT_R8G8B8A8_UNORM,
                                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        if (!entry.target.colour.view)
            return false;
        VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool.maxSets = 1;
        pool.poolSizeCount = 1;
        pool.pPoolSizes = &poolSize;
        if (vkCreateDescriptorPool(device, &pool, nullptr, &entry.target.pool) != VK_SUCCESS)
            return false;
        VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocate.descriptorPool = entry.target.pool;
        allocate.descriptorSetCount = 1;
        allocate.pSetLayouts = &overlaySetLayout;
        if (vkAllocateDescriptorSets(device, &allocate, &entry.target.set) != VK_SUCCESS)
            return false;
        const VkDescriptorImageInfo image{overlayMaskSampler, entry.target.colour.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet writes[2]{};
        for (uint32_t binding = 0; binding < 2; ++binding) {
            writes[binding] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            writes[binding].dstSet = entry.target.set;
            writes[binding].dstBinding = binding;
            writes[binding].descriptorCount = 1;
            writes[binding].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[binding].pImageInfo = &image;
        }
        vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
    }
    for (const auto &command : overlayLayers) {
        if (command.kind != plasma::LayerCommand::Kind::CacheCopy)
            continue;
        const auto found = cachedLayers.find(CacheKey(command.surface));
        if (found == cachedLayers.end() || !found->second.target.set) {
            command.surface->painted = false;
            return false;
        }
        if (!cachedQuad.buffer) {
            cachedQuad = createBuffer(6 * sizeof(OverlayVertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            if (!cachedQuad.mapped)
                return false;
            std::memset(cachedQuad.mapped, 0, 6 * sizeof(OverlayVertex));
        }
    }
    return true;
}

void Renderer::drawCachedLayer(VkCommandBuffer cmd, const plasma::LayerCommand &command, bool layer, bool depth) {
    const auto &entry = cachedLayers.at(CacheKey(command.surface));
    if (!entry.target.sampled)
        return;
    auto push = overlayPush();
    push.rectangle = glm::vec4(command.rectangle);
    push.uv = {0, 0, push.rectangle.z / entry.extent.width, push.rectangle.w / entry.extent.height};
    push.drawKind.x = 1;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layer ? layerOverlayPipeline : depth ? guiOverlayPipeline : overlayPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, overlayLayout, 0, 1, &entry.target.set, 0, nullptr);
    vkCmdPushConstants(cmd, overlayLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &cachedQuad.buffer, &offset);
    vkCmdDraw(cmd, 6, 1, 0, 0);
}

void Renderer::destroyCachedLayers() {
    for (auto &[key, entry] : cachedLayers) {
        if (const auto owner = key.lock())
            owner->painted = false;
        if (entry.target.pool)
            vkDestroyDescriptorPool(device, entry.target.pool, nullptr);
        destroyImage(entry.target.colour);
    }
    cachedLayers.clear();
    destroyBuffer(cachedQuad);
}
