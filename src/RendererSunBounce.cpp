#include "Renderer.h"
#include "VulkanSync.h"

bool Renderer::prepareSunBounce(Frame &frame, bool enabled) {
    if (enabled && !sunReflectance.view) {
        sunReflectance = createImage(SUN_MAP_SIZE, SUN_MAP_SIZE, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        sunDepth = createImage(SUN_MAP_SIZE, SUN_MAP_SIZE, DEPTH_FORMAT,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
        if (!sunReflectance.view || !sunDepth.view) {
            destroyImage(sunReflectance);
            destroyImage(sunDepth);
            settings.bouncedSunlight = false;
        }
    }
    // Each frame slot changes bindings only after its own fence. A disabled session uses the
    // existing valid depth texture, with no extra render targets or passes.
    const VkImageView colour = sunReflectance.view ? sunReflectance.view : shadowFar.view;
    if (frame.sunMapView != colour) {
        const VkDescriptorImageInfo images[] = {{pointSampler, colour, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
            {pointSampler, sunDepth.view ? sunDepth.view : shadowFar.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
        VkWriteDescriptorSet writes[2]{};
        for (uint32_t i = 0; i < 2; ++i) {
            writes[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            writes[i].dstSet = frame.set;
            writes[i].dstBinding = 4 + i;
            writes[i].descriptorCount = 1;
            writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[i].pImageInfo = &images[i];
        }
        vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
        frame.sunMapView = colour;
    }
    return enabled && sunDepth.view;
}

void Renderer::beginSunBounce(VkCommandBuffer cmd, VkDescriptorSet set) {
    const auto oldLayout = sunMapInitialized ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
    const auto fromStage = sunMapInitialized ? VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_2_NONE;
    const auto fromAccess = sunMapInitialized ? VK_ACCESS_2_SHADER_SAMPLED_READ_BIT : VK_ACCESS_2_NONE;
    vksync::apply(cmd, {
        vksync::barrier(sunReflectance.image, VK_IMAGE_ASPECT_COLOR_BIT, oldLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            fromStage, fromAccess, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT),
        vksync::barrier(sunDepth.image, VK_IMAGE_ASPECT_DEPTH_BIT, oldLayout, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            fromStage, fromAccess, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)});
    VkRenderingAttachmentInfo colour{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    colour.imageView = sunReflectance.view;
    colour.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colour.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colour.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    auto depth = colour;
    depth.imageView = sunDepth.view;
    depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depth.clearValue.depthStencil = {1, 0};
    VkRenderingInfo pass{VK_STRUCTURE_TYPE_RENDERING_INFO};
    pass.renderArea.extent = {SUN_MAP_SIZE, SUN_MAP_SIZE};
    pass.layerCount = pass.colorAttachmentCount = 1;
    pass.pColorAttachments = &colour;
    pass.pDepthAttachment = &depth;
    vkCmdBeginRendering(cmd, &pass);
    VkViewport viewport{0, 0, float(SUN_MAP_SIZE), float(SUN_MAP_SIZE), 0, 1};
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &pass.renderArea);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sunMapPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &set, 0, nullptr);
}

void Renderer::endSunBounce(VkCommandBuffer cmd) {
    vkCmdEndRendering(cmd);
    vksync::apply(cmd, {
        vksync::barrier(sunReflectance.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT),
        vksync::barrier(sunDepth.image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT)});
    sunMapInitialized = true;
}
