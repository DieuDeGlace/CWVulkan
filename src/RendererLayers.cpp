#include "Renderer.h"

bool Renderer::prepareClipLayers(size_t count) {
    if (!count)
        return true;
    if (!overlayImage.view)
        return false;
    while (clipLayers.size() < count) {
        ClipLayer layer;
        layer.colour = createImage(extent.width, extent.height, VK_FORMAT_R8G8B8A8_UNORM,
                                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        const auto discard = [&] {
            if (layer.pool)
                vkDestroyDescriptorPool(device, layer.pool, nullptr);
            destroyImage(layer.colour);
        };
        if (!layer.colour.image || !layer.colour.view) {
            discard();
            return false;
        }
        VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2};
        VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        poolInfo.maxSets = 1;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &size;
        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &layer.pool) != VK_SUCCESS) {
            discard();
            return false;
        }
        VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocate.descriptorPool = layer.pool;
        allocate.descriptorSetCount = 1;
        allocate.pSetLayouts = &overlaySetLayout;
        if (vkAllocateDescriptorSets(device, &allocate, &layer.set) != VK_SUCCESS) {
            discard();
            return false;
        }
        clipLayers.push_back(layer);
    }
    // Existing descriptors are never rewritten while frames may use them. Atlas
    // replacement waits for the device before changing its view; otherwise only
    // newly allocated pool entries need writes. Images are shared on one ordered
    // graphics queue with explicit read/write dependencies between uses.
    const bool atlasChanged = clipAtlas != overlayImage.view;
    for (size_t i = 0; i < clipLayers.size(); ++i) {
        if (!atlasChanged && clipLayers[i].descriptorReady)
            continue;
        VkDescriptorImageInfo images[] = {{overlaySampler, overlayImage.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                                          {overlayMaskSampler, clipLayers[i].colour.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
        VkWriteDescriptorSet writes[2]{};
        for (uint32_t binding = 0; binding < 2; ++binding) {
            writes[binding] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            writes[binding].dstSet = clipLayers[i].set;
            writes[binding].dstBinding = binding;
            writes[binding].descriptorCount = 1;
            writes[binding].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[binding].pImageInfo = &images[binding];
        }
        vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
        clipLayers[i].descriptorReady = true;
    }
    clipAtlas = overlayImage.view;
    return true;
}

void Renderer::destroyClipLayers() {
    for (auto &layer : clipLayers) {
        if (layer.pool)
            vkDestroyDescriptorPool(device, layer.pool, nullptr);
        destroyImage(layer.colour);
    }
    clipLayers.clear();
    clipAtlas = VK_NULL_HANDLE;
}
