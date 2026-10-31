// Anti-aliasing after the scene: TAA (the user's Steam TemporalAA mod, ported)
// then SMAA Ultra (as the user's CWSMTAA), then the interface on top.
#include "Renderer.h"
#include "VulkanSync.h"
#include <algorithm>
#include <cstring>

#include "post_vert.h"
#include "copy_frag.h"
#include "taa_frag.h"
#include "smaa_edge_vert.h"
#include "smaa_edge_frag.h"
#include "smaa_weight_vert.h"
#include "smaa_weight_frag.h"
#include "smaa_blend_vert.h"
#include "smaa_blend_frag.h"
#include "../third_party/smaa/AreaTex.h"
#include "../third_party/smaa/SearchTex.h"

namespace {
using vksync::barrier;
using vksync::apply;
// Colour image written by a pass, then read by the next.
VkImageMemoryBarrier2 toWrite(VkImage image) {
    return barrier(image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                   VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                   VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
}
VkImageMemoryBarrier2 toRead(VkImage image) {
    return barrier(image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                   VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                   VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
}
} // namespace

Renderer::Image Renderer::uploadImage(uint32_t w, uint32_t h, VkFormat format, const void *data, size_t bytes) {
    Image img = createImage(w, h, format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                            VK_IMAGE_ASPECT_COLOR_BIT);
    if (!img.image)
        return img;
    Buffer staging = createBuffer(bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    std::memcpy(staging.mapped, data, bytes);
    VkCommandBufferAllocateInfo cai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cai.commandPool = pool;
    cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cai.commandBufferCount = 1;
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(device, &cai, &cmd);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    auto in = barrier(img.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_NONE, 0, VK_PIPELINE_STAGE_2_COPY_BIT,
                      VK_ACCESS_2_TRANSFER_WRITE_BIT);
    apply(cmd, &in, 1);
    VkBufferImageCopy region{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {w, h, 1};
    vkCmdCopyBufferToImage(cmd, staging.buffer, img.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    auto out = barrier(img.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT,
                       VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                       VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
    apply(cmd, &out, 1);
    vkEndCommandBuffer(cmd);
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(device, pool, 1, &cmd);
    destroyBuffer(staging);
    return img;
}

bool Renderer::createPostResources(std::string &error) {
    VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    si.magFilter = si.minFilter = VK_FILTER_LINEAR;
    si.addressModeU = si.addressModeV = si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vkCreateSampler(device, &si, nullptr, &linearSampler);
    si.magFilter = si.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(device, &si, nullptr, &pointSampler);
    // SMAA's precomputed area (RG8) and search (R8) textures.
    areaTex = uploadImage(AREATEX_WIDTH, AREATEX_HEIGHT, VK_FORMAT_R8G8_UNORM, areaTexBytes, AREATEX_SIZE);
    searchTex = uploadImage(SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, VK_FORMAT_R8_UNORM, searchTexBytes, SEARCHTEX_SIZE);
    if (!areaTex.image || !searchTex.image) {
        error = "Cannot create the SMAA textures";
        return false;
    }
    // One layout for every post pass: up to four images, 128 bytes of constants.
    VkDescriptorSetLayoutBinding bindings[4];
    for (uint32_t i = 0; i < 4; i++)
        bindings[i] = {i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT};
    VkDescriptorSetLayoutCreateInfo sli{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    sli.bindingCount = 4;
    sli.pBindings = bindings;
    vkCreateDescriptorSetLayout(device, &sli, nullptr, &postSetLayout);
    VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 128};
    VkPipelineLayoutCreateInfo pli{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pli.setLayoutCount = 1;
    pli.pSetLayouts = &postSetLayout;
    pli.pushConstantRangeCount = 1;
    pli.pPushConstantRanges = &push;
    vkCreatePipelineLayout(device, &pli, nullptr, &postLayout);
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 9 * 4};
    VkDescriptorPoolCreateInfo dpi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    dpi.maxSets = 9;
    dpi.poolSizeCount = 1;
    dpi.pPoolSizes = &size;
    vkCreateDescriptorPool(device, &dpi, nullptr, &postPool);
    VkDescriptorSet *sets[9] = {&taaSets[0], &taaSets[1], &copySets[0], &copySets[1], &edgeSets[0],
                                &edgeSets[1], &weightSet,  &blendSets[0], &blendSets[1]};
    for (auto *s : sets) {
        VkDescriptorSetAllocateInfo dai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        dai.descriptorPool = postPool;
        dai.descriptorSetCount = 1;
        dai.pSetLayouts = &postSetLayout;
        vkAllocateDescriptorSets(device, &dai, s);
    }

    auto module = [&](const uint32_t *code, size_t bytes) {
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        info.codeSize = bytes;
        info.pCode = code;
        VkShaderModule m = VK_NULL_HANDLE;
        vkCreateShaderModule(device, &info, nullptr, &m);
        return m;
    };
    auto pipeline = [&](const uint32_t *vs, size_t vsBytes, const uint32_t *fs, size_t fsBytes, VkFormat format,
                        VkPipeline &out) {
        VkShaderModule v = module(vs, vsBytes), f = module(fs, fsBytes);
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, v, "main"};
        stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, f, "main"};
        VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport.viewportCount = viewport.scissorCount = 1;
        VkDynamicState dynamics[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount = 2;
        dynamic.pDynamicStates = dynamics;
        VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = VK_CULL_MODE_NONE;
        raster.lineWidth = 1;
        VkPipelineDepthStencilStateCreateInfo depthState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        VkPipelineColorBlendAttachmentState attachment{};
        attachment.colorWriteMask = 0xf;
        VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        blend.attachmentCount = 1;
        blend.pAttachments = &attachment;
        VkPipelineRenderingCreateInfo rendering{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        rendering.colorAttachmentCount = 1;
        rendering.pColorAttachmentFormats = &format;
        VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, &rendering};
        info.stageCount = 2;
        info.pStages = stages;
        info.pVertexInputState = &input;
        info.pInputAssemblyState = &assembly;
        info.pViewportState = &viewport;
        info.pRasterizationState = &raster;
        info.pMultisampleState = &multisample;
        info.pDepthStencilState = &depthState;
        info.pColorBlendState = &blend;
        info.pDynamicState = &dynamic;
        info.layout = postLayout;
        VkResult r = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &out);
        vkDestroyShaderModule(device, v, nullptr);
        vkDestroyShaderModule(device, f, nullptr);
        return r == VK_SUCCESS;
    };
    bool ok = pipeline(spv_post_vert, sizeof spv_post_vert, spv_taa_frag, sizeof spv_taa_frag, swapFormat, taaPipeline) &&
              pipeline(spv_post_vert, sizeof spv_post_vert, spv_copy_frag, sizeof spv_copy_frag, swapFormat, copyPipeline) &&
              pipeline(spv_smaa_edge_vert, sizeof spv_smaa_edge_vert, spv_smaa_edge_frag, sizeof spv_smaa_edge_frag,
                       VK_FORMAT_R8G8_UNORM, smaaEdgePipeline) &&
              pipeline(spv_smaa_weight_vert, sizeof spv_smaa_weight_vert, spv_smaa_weight_frag,
                       sizeof spv_smaa_weight_frag, VK_FORMAT_R8G8B8A8_UNORM, smaaWeightPipeline) &&
              pipeline(spv_smaa_blend_vert, sizeof spv_smaa_blend_vert, spv_smaa_blend_frag, sizeof spv_smaa_blend_frag,
                       swapFormat, smaaBlendPipeline);
    if (!ok)
        error = "Anti-aliasing pipeline creation failed";
    return ok;
}

bool Renderer::createPostTargets() {
    uint32_t w = extent.width, h = extent.height;
    const VkImageUsageFlags color = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    sceneColor = createImage(w, h, swapFormat, color | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    for (int i = 0; i < 2; i++) {
        depthImages[i] = createImage(w, h, DEPTH_FORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                     VK_IMAGE_ASPECT_DEPTH_BIT);
        history[i] = createImage(w, h, swapFormat, color, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    smaaEdges = createImage(w, h, VK_FORMAT_R8G8_UNORM, color, VK_IMAGE_ASPECT_COLOR_BIT);
    smaaWeights = createImage(w, h, VK_FORMAT_R8G8B8A8_UNORM, color, VK_IMAGE_ASPECT_COLOR_BIT);
    if (!sceneColor.image || !depthImages[0].image || !depthImages[1].image || !history[0].image ||
        !history[1].image || !smaaEdges.image || !smaaWeights.image)
        return false;
    auto write = [&](VkDescriptorSet set, std::initializer_list<std::pair<VkImageView, VkSampler>> images) {
        std::vector<VkDescriptorImageInfo> info;
        for (auto &[view, sampler] : images)
            info.push_back({sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        // Unused bindings point at the first image, so every binding is valid.
        while (info.size() < 4)
            info.push_back(info[0]);
        VkWriteDescriptorSet writes[4]{};
        for (uint32_t i = 0; i < 4; i++) {
            writes[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            writes[i].dstSet = set;
            writes[i].dstBinding = i;
            writes[i].descriptorCount = 1;
            writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[i].pImageInfo = &info[i];
        }
        vkUpdateDescriptorSets(device, 4, writes, 0, nullptr);
    };
    for (int p = 0; p < 2; p++) {
        int q = 1 - p; // last frame's images
        write(taaSets[p], {{sceneColor.view, linearSampler}, {history[q].view, linearSampler},
                           {depthImages[p].view, pointSampler}, {depthImages[q].view, pointSampler}});
        write(copySets[p], {{history[p].view, linearSampler}});
        write(edgeSets[p], {{history[p].view, linearSampler}});
        write(blendSets[p], {{history[p].view, linearSampler}, {smaaWeights.view, linearSampler}});
    }
    write(weightSet, {{smaaEdges.view, linearSampler}, {areaTex.view, linearSampler}, {searchTex.view, pointSampler}});
    postFresh = true;
    historyValid = false;
    return createWaterTargets();
}

void Renderer::destroyPostTargets() {
    destroyWaterTargets();
    destroyImage(guiDepth);
    guiDepthFresh = true;
    for (Image *i : {&sceneColor, &depthImages[0], &depthImages[1], &history[0], &history[1], &smaaEdges, &smaaWeights})
        destroyImage(*i);
}

void Renderer::destroyPostResources() {
    for (auto p : {taaPipeline, copyPipeline, smaaEdgePipeline, smaaWeightPipeline, smaaBlendPipeline})
        if (p)
            vkDestroyPipeline(device, p, nullptr);
    vkDestroyPipelineLayout(device, postLayout, nullptr);
    vkDestroyDescriptorPool(device, postPool, nullptr);
    vkDestroyDescriptorSetLayout(device, postSetLayout, nullptr);
    vkDestroySampler(device, linearSampler, nullptr);
    vkDestroySampler(device, pointSampler, nullptr);
    destroyImage(areaTex);
    destroyImage(searchTex);
}

bool Renderer::uploadOverlay() {
    if (!overlayImage.image || overlayVertices.empty())
        return false;
    VkDeviceSize bytes = overlayVertices.size() * sizeof(OverlayVertex);
    Buffer &vb = overlayBuffers[frameIndex];
    if (overlayCapacity[frameIndex] < bytes) {
        destroyBuffer(vb); // This frame slot's previous use finished (its fence was waited on).
        overlayCapacity[frameIndex] = std::max<VkDeviceSize>(bytes * 2, 64 * 1024);
        vb = createBuffer(overlayCapacity[frameIndex], VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
    if (!vb.mapped)
        return false;
    auto *out = (OverlayVertex *)vb.mapped;
    for (size_t i = 0; i < overlayVertices.size(); i++) {
        out[i] = overlayVertices[i];
        out[i].u /= overlayWidth; // Atlas texels to normalised coordinates.
        out[i].v /= overlayHeight;
    }
    return true;
}
void Renderer::drawOverlayRange(VkCommandBuffer cmd, uint32_t first, uint32_t count, bool depth) {
    if (!count || !overlayImage.image || !overlayBuffers[frameIndex].buffer)
        return;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, depth ? guiOverlayPipeline : overlayPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, overlayLayout, 0, 1, &overlaySet, 0, nullptr);
    float screen[2] = {(float)extent.width, (float)extent.height};
    vkCmdPushConstants(cmd, overlayLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(screen), screen);
    VkDeviceSize zero = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &overlayBuffers[frameIndex].buffer, &zero);
    vkCmdDraw(cmd, count, 1, first, 0);
}
void Renderer::drawOverlay(VkCommandBuffer cmd) {
    if (uploadOverlay())
        drawOverlayRange(cmd, 0, (uint32_t)overlayVertices.size(), false);
}

void Renderer::recordPost(VkCommandBuffer cmd, uint32_t imageIndex, glm::vec2 jitter, const glm::mat4 &clipToPrevious,
                          bool reproject) {
    const int p = parity, q = 1 - parity;
    VkViewport vp{0, 0, (float)extent.width, (float)extent.height, 0, 1};
    VkRect2D sc{{0, 0}, extent};
    auto begin = [&](VkImageView view, bool clear) {
        VkRenderingAttachmentInfo color{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        color.imageView = view;
        color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        VkRenderingInfo ri{VK_STRUCTURE_TYPE_RENDERING_INFO};
        ri.renderArea = sc;
        ri.layerCount = 1;
        ri.colorAttachmentCount = 1;
        ri.pColorAttachments = &color;
        vkCmdBeginRendering(cmd, &ri);
        vkCmdSetViewport(cmd, 0, 1, &vp);
        vkCmdSetScissor(cmd, 0, 1, &sc);
    };
    auto fullscreen = [&](VkPipeline pipeline, VkDescriptorSet set, const void *constants, uint32_t size) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, postLayout, 0, 1, &set, 0, nullptr);
        if (size)
            vkCmdPushConstants(cmd, postLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, size,
                               constants);
        vkCmdDraw(cmd, 3, 1, 0, 0);
    };

    // The scene and its depth become readable; on fresh targets last frame's
    // depth and history are made valid (and history is ignored this frame).
    std::vector<VkImageMemoryBarrier2> ready = {
        toRead(sceneColor.image),
        barrier(depthImages[p].image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                VK_ACCESS_2_SHADER_SAMPLED_READ_BIT)};
    if (postFresh) {
        ready.push_back(barrier(depthImages[q].image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_NONE, 0,
                                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT));
        ready.push_back(barrier(history[q].image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_NONE, 0,
                                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT));
    }
    ready.push_back(toWrite(history[p].image));
    apply(cmd, ready.data(), (uint32_t)ready.size());

    // TAA resolve into this frame's history (weight 0 = the current image only).
    struct {
        float a[4], b[4];
        float clip[16];
        float previousJitter[4];
    } c{};
    bool useHistory = settings.taa && historyValid && !postFresh;
    c.a[0] = settings.taa ? settings.historyWeight : 0;
    c.a[1] = useHistory ? 1.f : 0.f;
    c.a[2] = jitter.x;
    c.a[3] = jitter.y;
    c.b[0] = settings.taa ? settings.resolveBlur : 0;
    c.b[1] = settings.edgeHistoryWeight;
    c.b[2] = reproject ? 1.f : 0.f;
    c.b[3] = .0005f; // depth tolerance, as the Steam mod
    std::memcpy(c.clip, &clipToPrevious[0][0], sizeof c.clip);
    c.previousJitter[0] = previousJitter.x;
    c.previousJitter[1] = previousJitter.y;
    begin(history[p].view, false);
    fullscreen(taaPipeline, taaSets[p], &c, sizeof c);
    vkCmdEndRendering(cmd);
    auto resolved = toRead(history[p].image);
    apply(cmd, &resolved, 1);

    // SMAA Ultra on the resolved image (after it is stored as history), or a copy.
    VkImage swap = swapImages[imageIndex];
    float metrics[4] = {1.f / extent.width, 1.f / extent.height, (float)extent.width, (float)extent.height};
    if (settings.smaa) {
        auto edgesWrite = toWrite(smaaEdges.image);
        apply(cmd, &edgesWrite, 1);
        begin(smaaEdges.view, true);
        fullscreen(smaaEdgePipeline, edgeSets[p], metrics, sizeof metrics);
        vkCmdEndRendering(cmd);
        VkImageMemoryBarrier2 next[2] = {toRead(smaaEdges.image), toWrite(smaaWeights.image)};
        apply(cmd, next, 2);
        begin(smaaWeights.view, true);
        fullscreen(smaaWeightPipeline, weightSet, metrics, sizeof metrics);
        vkCmdEndRendering(cmd);
        VkImageMemoryBarrier2 last[2] = {toRead(smaaWeights.image), toWrite(swap)};
        apply(cmd, last, 2);
        begin(swapViews[imageIndex], false);
        fullscreen(smaaBlendPipeline, blendSets[p], metrics, sizeof metrics);
    } else {
        auto swapWrite = toWrite(swap);
        apply(cmd, &swapWrite, 1);
        begin(swapViews[imageIndex], false);
        fullscreen(copyPipeline, copySets[p], nullptr, 0);
    }
    // The interface, sharp, over the anti-aliased image. Item callbacks retain scene
    // depth and interleave WORLD models with the Plasma triangle stream.
    if (guiRequests.empty()) {
        drawOverlay(cmd);
        vkCmdEndRendering(cmd);
    } else {
        vkCmdEndRendering(cmd);
        recordGui(cmd, imageIndex);
    }
    postFresh = false;
}
