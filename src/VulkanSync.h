#pragma once
// Shared image transitions for off-screen scene, post-process and water passes.
#include <volk.h>
namespace vksync {
inline VkImageMemoryBarrier2 barrier(VkImage image, VkImageAspectFlags aspect, VkImageLayout from, VkImageLayout to,
                                     VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess, VkPipelineStageFlags2 dstStage,
                                     VkAccessFlags2 dstAccess) {
    VkImageMemoryBarrier2 result{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    result.srcStageMask = srcStage;
    result.srcAccessMask = srcAccess;
    result.dstStageMask = dstStage;
    result.dstAccessMask = dstAccess;
    result.oldLayout = from;
    result.newLayout = to;
    result.srcQueueFamilyIndex = result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    result.image = image;
    result.subresourceRange = {aspect, 0, 1, 0, 1};
    return result;
}
inline void apply(VkCommandBuffer cmd, const VkImageMemoryBarrier2 *barriers, uint32_t count) {
    VkDependencyInfo dependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency.imageMemoryBarrierCount = count;
    dependency.pImageMemoryBarriers = barriers;
    vkCmdPipelineBarrier2(cmd, &dependency);
}
} // namespace vksync
