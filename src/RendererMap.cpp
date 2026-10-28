// The world map's pass (docs/specs/worldmap-rendering.md §3): the map's models drawn after the
// world over a cleared depth, through shaders/map.vert and map.frag, which carry the map's own
// light rig and colour steps whatever the world's look. Opaque items come first in the list
// and blended ones (regions fading in) after, as WorldMap::collect orders them.
#include "Renderer.h"

#include "map_frag.h"
#include "map_vert.h"

#include <cstddef>

namespace {

// Must match the push block of shaders/map.vert and map.frag.
struct MapPush {
    glm::mat4 modelView;
    glm::vec4 lens;
    glm::vec4 turn;     // yaw, pitch (radians), opacity, depth offset
    glm::vec4 material; // colour, white
    glm::vec4 fog;      // the world's fog colour
};
static_assert(sizeof(MapPush) <= 128, "push constants beyond Vulkan's guaranteed 128 bytes");

} // namespace

bool Renderer::createMapPipeline(std::string &error) {
    auto module = [&](const uint32_t *code, size_t bytes) {
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        info.codeSize = bytes;
        info.pCode = code;
        VkShaderModule m = VK_NULL_HANDLE;
        vkCreateShaderModule(device, &info, nullptr, &m);
        return m;
    };
    VkShaderModule vs = module(spv_map_vert, sizeof(spv_map_vert)), fs = module(spv_map_frag, sizeof(spv_map_frag));

    VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MapPush)};
    VkPipelineLayoutCreateInfo pli{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pli.pushConstantRangeCount = 1;
    pli.pPushConstantRanges = &push;
    vkCreatePipelineLayout(device, &pli, nullptr, &mapLayout);

    VkVertexInputBindingDescription binding{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attributes[] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, x)},
        {1, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(Vertex, r)},
        {2, 0, VK_FORMAT_R8G8B8A8_UINT, offsetof(Vertex, normal)},
    };
    VkPipelineVertexInputStateCreateInfo vertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 3;
    vertexInput.pVertexAttributeDescriptions = attributes;
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
    // The map's blocks are closed and the lens mirrors x, so nothing is culled.
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.lineWidth = 1;
    VkPipelineDepthStencilStateCreateInfo depthState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthState.depthTestEnable = VK_TRUE;
    depthState.depthWriteEnable = VK_TRUE;
    depthState.depthCompareOp = VK_COMPARE_OP_LESS;
    // Opacity blends: 1 for every whole model, less while a region fades in.
    VkPipelineColorBlendAttachmentState attachment{};
    attachment.colorWriteMask = 0xf;
    attachment.blendEnable = VK_TRUE;
    attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    attachment.colorBlendOp = VK_BLEND_OP_ADD;
    attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount = 1;
    blend.pAttachments = &attachment;
    VkPipelineRenderingCreateInfo rendering{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachmentFormats = &swapFormat;
    rendering.depthAttachmentFormat = DEPTH_FORMAT;
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vs, "main"};
    stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fs, "main"};
    VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, &rendering};
    info.stageCount = 2;
    info.pStages = stages;
    info.pVertexInputState = &vertexInput;
    info.pInputAssemblyState = &assembly;
    info.pViewportState = &viewport;
    info.pRasterizationState = &raster;
    info.pMultisampleState = &multisample;
    info.pDepthStencilState = &depthState;
    info.pColorBlendState = &blend;
    info.pDynamicState = &dynamic;
    info.layout = mapLayout;
    const bool made = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &mapPipeline) == VK_SUCCESS;
    vkDestroyShaderModule(device, vs, nullptr);
    vkDestroyShaderModule(device, fs, nullptr);
    if (!made)
        error = "Map pipeline creation failed";
    return made;
}

void Renderer::destroyMapPipeline() {
    if (mapPipeline)
        vkDestroyPipeline(device, mapPipeline, nullptr);
    if (mapLayout)
        vkDestroyPipelineLayout(device, mapLayout, nullptr);
    mapPipeline = VK_NULL_HANDLE;
    mapLayout = VK_NULL_HANDLE;
}

unsigned Renderer::drawMap(VkCommandBuffer cmd, const std::vector<DrawItem> &items, glm::vec3 fogColour, glm::vec2 jitter) {
    unsigned draws = 0;
    bool started = false;
    for (const DrawItem &item : items) {
        if (!(item.flags & WORLD_MAP) || !meshes[item.mesh].indexCount)
            continue;
        if (!started) {
            // The map has a depth of its own: nothing of the world covers it.
            VkClearAttachment clear{};
            clear.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            clear.clearValue.depthStencil = {1, 0};
            VkClearRect rect{{{0, 0}, extent}, 0, 1};
            vkCmdClearAttachments(cmd, 1, &clear, 1, &rect);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mapPipeline);
            started = true;
        }
        const DrawItem::MapDraw &m = item.map;
        // The TAA jitter goes into the lens's shift (the shader flips y for Vulkan), so the
        // resolve samples the map back unshifted, as it does the world.
        const glm::vec4 lens = m.lens + glm::vec4(0, 0, jitter.x, -jitter.y);
        const MapPush push{item.model, lens, glm::vec4(m.yaw, m.pitch, m.opacity, m.depthOffset),
                           glm::vec4(m.colour, m.white), glm::vec4(fogColour, 0)};
        vkCmdPushConstants(cmd, mapLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);
        const GpuMesh &mesh = meshes[item.mesh];
        VkDeviceSize zero = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.buffer.buffer, &zero);
        vkCmdBindIndexBuffer(cmd, mesh.buffer.buffer, mesh.indexOffset, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, mesh.indexCount, 1, 0, 0, 0);
        draws++;
    }
    return draws;
}
