#include "Renderer.h"
#include "Config.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <cstdlib>

#include "scene_vert.h"
#include "scene_frag.h"
#include "shadow_vert.h"
#include "water_frag.h"
#include "sky_vert.h"
#include "sky_frag.h"
#include "overlay_vert.h"
#include "overlay_frag.h"
#include "alpha_scene_vert.h"
#include "alpha_scene_frag.h"
#include "alpha_water_frag.h"
#include "alpha_sky_frag.h"
#include "alpha_sprite_vert.h"
#include "alpha_sprite_frag.h"
#include "alpha_cloud_frag.h"
#include "steam_cloud_frag.h"
#include "steam_star_frag.h"
#include "stb_image_write.h"

namespace {
// The Alpha look's point-light table (frame-wide; each draw picks up to 10 of them).
constexpr int MAX_POINT_LIGHTS = 64;
// Lights the frame-wide list keeps (models and creatures use its first 10), and the most a
// draw is lit by (docs/specs/alpha-rendering.md 10.2).
constexpr int FRAME_LIGHTS = 16, LIGHTS_PER_DRAW = 10;

// std140-compatible: only mat4 and vec4. Must match shaders/common.glsl.
struct FrameUniforms {
    glm::mat4 viewProj, invViewProj, lightFar, lightNear;
    glm::vec4 eye, lightDir, front, back, ambient, fog, sky, shadowFar, shadowNear, wind, focus,
        sunDisk, moonDisk, steamSky[6];
    glm::vec4 alphaSky1, alphaSky2, alphaFog, alphaFront, alphaBack, alphaAmbient, alphaMaterial, alphaSun,
        alphaView, sunColour, alphaMoonFront, alphaMoonBack, alphaNight;
    glm::mat4 skyProj, view;
    glm::vec4 pointLights[MAX_POINT_LIGHTS], pointColours[MAX_POINT_LIGHTS];
};
struct PushConstants {
    glm::mat4 model;
    glm::vec4 params;
    glm::vec4 look;    // Alpha look: light where the object stands, hit flash, shimmer
    glm::uvec4 lights; // Alpha look: point-light indices, 8 bits each in x..z; w count
};
static_assert(sizeof(PushConstants) <= 128, "push constants beyond Vulkan's guaranteed 128 bytes");

// Half the light-space depth range in blocks: tall mountains far from the
// player can still cast into the map.
constexpr float LIGHT_DEPTH = 420;
// The shadow direction's step: two minutes of the day, half a degree of the sun's path.
constexpr double SHADOW_DIRECTION_STEP_MS = 2 * 60 * 1000;
// How far a swaying plant's top can move (vegetation-wind.md §4.1: 0.2 block along y).
constexpr float PLANT_SWAY_REACH = .2f;

// docs/specs/sky-and-clouds.md: Alpha's clouds melt into the sky by 1.5 times the fog distance;
// the far plane reaches that, and Steam's clouds (up to 200 blocks past the view distance, 250
// blocks round), with a margin.
constexpr float ALPHA_CLOUD_REACH = 1.5f, FAR_MARGIN = 64;
// The Steam star dome's radius is counted in units of 64 blocks (§B.6).
constexpr float STAR_DOME_UNIT = 64;

VkImageMemoryBarrier2 imageBarrier(VkImage image, VkImageAspectFlags aspect, VkImageLayout from,
                                   VkImageLayout to, VkPipelineStageFlags2 srcStage,
                                   VkAccessFlags2 srcAccess, VkPipelineStageFlags2 dstStage,
                                   VkAccessFlags2 dstAccess) {
    VkImageMemoryBarrier2 b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    b.srcStageMask = srcStage;
    b.srcAccessMask = srcAccess;
    b.dstStageMask = dstStage;
    b.dstAccessMask = dstAccess;
    b.oldLayout = from;
    b.newLayout = to;
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = image;
    b.subresourceRange = {aspect, 0, 1, 0, 1};
    return b;
}
void barriers(VkCommandBuffer cmd, const VkImageMemoryBarrier2 *images, uint32_t count,
              const VkMemoryBarrier2 *memory = nullptr) {
    VkDependencyInfo d{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    d.imageMemoryBarrierCount = count;
    d.pImageMemoryBarriers = images;
    d.memoryBarrierCount = memory ? 1 : 0;
    d.pMemoryBarriers = memory;
    vkCmdPipelineBarrier2(cmd, &d);
}

// Light-space matrix centred on `center`, snapped to whole shadow texels so
// shadow edges do not crawl as the player moves.
glm::mat4 lightMatrix(glm::vec3 lightDir, glm::vec3 center, float radius, uint32_t resolution) {
    glm::vec3 up = std::abs(lightDir.z) > .99f ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, 1);
    glm::mat4 view = glm::lookAtRH(glm::vec3(0), -lightDir, up);
    glm::vec3 p = glm::vec3(view * glm::vec4(center, 1));
    float texel = 2 * radius / resolution;
    p.x = std::round(p.x / texel) * texel;
    p.y = std::round(p.y / texel) * texel;
    glm::mat4 proj = glm::orthoRH_ZO(p.x - radius, p.x + radius, p.y - radius, p.y + radius,
                                     -p.z - LIGHT_DEPTH, -p.z + LIGHT_DEPTH);
    return proj * view;
}

struct Frustum {
    glm::vec4 planes[6];
    explicit Frustum(const glm::mat4 &m) {
        auto row = [&](int i) { return glm::vec4(m[0][i], m[1][i], m[2][i], m[3][i]); };
        planes[0] = row(3) + row(0);
        planes[1] = row(3) - row(0);
        planes[2] = row(3) + row(1);
        planes[3] = row(3) - row(1);
        planes[4] = row(2);
        planes[5] = row(3) - row(2);
        for (auto &p : planes)
            p /= glm::length(glm::vec3(p));
    }
    bool sphere(glm::vec3 c, float r) const {
        for (auto &p : planes)
            if (glm::dot(glm::vec3(p), c) + p.w < -r)
                return false;
        return true;
    }
};
} // namespace

void RenderSettings::read(const Config &c) {
    std::string look = c.text("Render", "Style", "alpha");
    std::transform(look.begin(), look.end(), look.begin(), [](unsigned char ch) { return (char)std::tolower(ch); });
    style = look == "steam" ? RenderStyle::Steam : RenderStyle::Alpha;
    moon.enabled = c.flag("Render", "Moon", moon.enabled);
    moon.brightness = (float)c.number("Render", "MoonBrightness", moon.brightness * 100, 0, 200) / 100;
    moon.shadowStrength = (float)c.number("Render", "MoonShadowStrength", moon.shadowStrength * 100, 0, 100) / 100;
    moon.nightAmbient = (float)c.number("Render", "NightAmbient", moon.nightAmbient * 100, 0, 200) / 100;
    vsync = c.flag("Graphics", "VSync", vsync);
    fov = (float)c.number("Graphics", "FieldOfView", fov, 30, 110);
    fogDistance = (float)c.number("Graphics", "ViewDistance", fogDistance, 128, 2000);
    sceneryDistance = (float)c.number("Graphics", "SceneryDistance", sceneryDistance, 16, 512);
    // Anti-aliasing: the Steam TemporalAA mod's [TAA] keys, overridden by [AntiAliasing].
    taa = c.flag("TAA", "Enabled", taa);
    historyWeight = (float)c.number("TAA", "HistoryPercent", historyWeight * 100, 0, 85) / 100;
    edgeHistoryWeight = (float)c.number("TAA", "EdgeHistoryPercent", edgeHistoryWeight * 100, 0, 95) / 100;
    resolveBlur = (float)c.number("TAA", "BlurPercent", resolveBlur * 100, 0, 50) / 100;
    jitterScale = (float)c.number("TAA", "JitterPercent", jitterScale * 100, 0, 100) / 100;
    taa = c.flag("AntiAliasing", "TAA", taa);
    smaa = c.flag("AntiAliasing", "SMAA", smaa);
    historyWeight = (float)c.number("AntiAliasing", "HistoryPercent", historyWeight * 100, 0, 85) / 100;
    edgeHistoryWeight = (float)c.number("AntiAliasing", "EdgeHistoryPercent", edgeHistoryWeight * 100, 0, 95) / 100;
    resolveBlur = (float)c.number("AntiAliasing", "BlurPercent", resolveBlur * 100, 0, 50) / 100;
    jitterScale = (float)c.number("AntiAliasing", "JitterPercent", jitterScale * 100, 0, 100) / 100;
    shadows = c.flag("Shadows", "Enabled", shadows);
    sunShadowStrength = (float)c.number("Shadows", "StrengthPercent", sunShadowStrength * 100, 0, 100) / 100;
    resolution = (uint32_t)c.number("Shadows", "Resolution", resolution, 512, 8192);
    radius = (float)c.number("Shadows", "Radius", radius, 16, 400);
    cascades = c.flag("Shadows", "Cascades", cascades);
    nearResolution = (uint32_t)c.number("Shadows", "NearResolution", nearResolution, 512, 8192);
    nearRadius = (float)c.number("Shadows", "NearRadius", nearRadius, 8, radius);
    sunWarmth = (float)c.number("Shadows", "SunWarmthPercent", sunWarmth * 100, 0, 100) / 100;
    fadeStart = (float)c.number("Shadows", "FadeStartPercent", fadeStart * 100, 0, 100) / 100;
    cameraFade = c.flag("Shadows", "CameraFade", cameraFade);
    // (CubeVulkan's own key: the WorldMotion mod's Mods/WorldMotion.ini, which CubeVulkan
    // imports, turns its leaf sway on, but vanilla moves no block; vegetation-wind.md §7.)
    wind = c.flag("Motion", "LeafSway", wind);
    leafAmplitude = (float)c.number("Motion", "LeafAmplitudePercent", leafAmplitude * 100, 0, 100) / 100;
    leafRange = (float)c.number("Motion", "LeafRange", leafRange, 8, 512);
    windSpeed = (float)c.number("Motion", "WindSpeedPercent", windSpeed * 100, 0, 400) / 100;
}

uint32_t Renderer::memoryType(uint32_t bits, VkMemoryPropertyFlags flags) const {
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
        if ((bits & (1u << i)) && (memoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    return UINT32_MAX;
}

Renderer::Buffer Renderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                        VkMemoryPropertyFlags flags) {
    Buffer b;
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = size;
    info.usage = usage;
    if (vkCreateBuffer(device, &info, nullptr, &b.buffer) != VK_SUCCESS)
        return {};
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(device, b.buffer, &req);
    VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = memoryType(req.memoryTypeBits, flags);
    if (alloc.memoryTypeIndex == UINT32_MAX ||
        vkAllocateMemory(device, &alloc, nullptr, &b.memory) != VK_SUCCESS) {
        vkDestroyBuffer(device, b.buffer, nullptr);
        return {};
    }
    vkBindBufferMemory(device, b.buffer, b.memory, 0);
    if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        vkMapMemory(device, b.memory, 0, VK_WHOLE_SIZE, 0, &b.mapped);
    return b;
}

void Renderer::destroyBuffer(Buffer &b) {
    if (b.buffer)
        vkDestroyBuffer(device, b.buffer, nullptr);
    if (b.memory)
        vkFreeMemory(device, b.memory, nullptr);
    b = {};
}

Renderer::Image Renderer::createImage(uint32_t w, uint32_t h, VkFormat format,
                                      VkImageUsageFlags usage, VkImageAspectFlags aspect) {
    Image img;
    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.extent = {w, h, 1};
    info.mipLevels = info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usage;
    if (vkCreateImage(device, &info, nullptr, &img.image) != VK_SUCCESS)
        return {};
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(device, img.image, &req);
    VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = memoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (alloc.memoryTypeIndex == UINT32_MAX ||
        vkAllocateMemory(device, &alloc, nullptr, &img.memory) != VK_SUCCESS) {
        vkDestroyImage(device, img.image, nullptr);
        return {};
    }
    vkBindImageMemory(device, img.image, img.memory, 0);
    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view.image = img.image;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = format;
    view.subresourceRange = {aspect, 0, 1, 0, 1};
    vkCreateImageView(device, &view, nullptr, &img.view);
    return img;
}

void Renderer::destroyImage(Image &i) {
    if (i.view)
        vkDestroyImageView(device, i.view, nullptr);
    if (i.image)
        vkDestroyImage(device, i.image, nullptr);
    if (i.memory)
        vkFreeMemory(device, i.memory, nullptr);
    i = {};
}

bool Renderer::createDevice(std::string &error) {
    if (volkInitialize() != VK_SUCCESS) {
        error = "Vulkan is not available (vulkan-1.dll missing). Update your GPU driver.";
        return false;
    }
    if (volkGetInstanceVersion() < VK_API_VERSION_1_3) {
        error = "Vulkan 1.3 is required. Update your GPU driver.";
        return false;
    }
    uint32_t extensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + extensionCount);
    std::vector<const char *> layers;
    // CUBEVULKAN_VALIDATION=1 enables the Khronos validation layer if installed.
    if (const char *v = std::getenv("CUBEVULKAN_VALIDATION"); v && *v == '1') {
        uint32_t n = 0;
        vkEnumerateInstanceLayerProperties(&n, nullptr);
        std::vector<VkLayerProperties> available(n);
        vkEnumerateInstanceLayerProperties(&n, available.data());
        for (auto &l : available)
            if (!std::strcmp(l.layerName, "VK_LAYER_KHRONOS_validation"))
                layers.push_back("VK_LAYER_KHRONOS_validation");
    }
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "CubeVulkan";
    app.pEngineName = "CubeVulkan";
    app.apiVersion = VK_API_VERSION_1_3;
    VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &app;
    ici.enabledExtensionCount = (uint32_t)extensions.size();
    ici.ppEnabledExtensionNames = extensions.data();
    ici.enabledLayerCount = (uint32_t)layers.size();
    ici.ppEnabledLayerNames = layers.data();
    if (vkCreateInstance(&ici, nullptr, &instance) != VK_SUCCESS) {
        error = "vkCreateInstance failed";
        return false;
    }
    volkLoadInstance(instance);
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        error = "Cannot create a Vulkan surface for the window";
        return false;
    }
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    std::vector<VkPhysicalDevice> gpus(count);
    vkEnumeratePhysicalDevices(instance, &count, gpus.data());
    int bestScore = -1;
    for (auto candidate : gpus) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(candidate, &props);
        if (props.apiVersion < VK_API_VERSION_1_3)
            continue;
        VkPhysicalDeviceVulkan13Features f13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        VkPhysicalDeviceFeatures2 f2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &f13};
        vkGetPhysicalDeviceFeatures2(candidate, &f2);
        if (!f13.dynamicRendering || !f13.synchronization2)
            continue;
        uint32_t qn = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &qn, nullptr);
        std::vector<VkQueueFamilyProperties> families(qn);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &qn, families.data());
        for (uint32_t i = 0; i < qn; i++) {
            VkBool32 present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface, &present);
            if (!(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) || !present)
                continue;
            int score = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 2 : 1;
            if (score > bestScore) {
                bestScore = score;
                gpu = candidate;
                queueFamily = i;
                gpuName = props.deviceName;
                depthClamp = f2.features.depthClamp;
            }
            break;
        }
    }
    if (!gpu) {
        error = "No GPU with Vulkan 1.3 dynamic rendering and presentation was found.";
        return false;
    }
    vkGetPhysicalDeviceMemoryProperties(gpu, &memoryProperties);
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(gpu, &props);
        timestampPeriod = props.limits.timestampPeriod;
    }
    float priority = 1;
    VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = queueFamily;
    qci.queueCount = 1;
    qci.pQueuePriorities = &priority;
    VkPhysicalDeviceVulkan13Features f13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    f13.dynamicRendering = VK_TRUE;
    f13.synchronization2 = VK_TRUE;
    VkPhysicalDeviceFeatures2 f2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &f13};
    f2.features.depthClamp = depthClamp;
    const char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, &f2};
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = 1;
    dci.ppEnabledExtensionNames = deviceExtensions;
    if (vkCreateDevice(gpu, &dci, nullptr, &device) != VK_SUCCESS) {
        error = "vkCreateDevice failed";
        return false;
    }
    volkLoadDevice(device);
    vkGetDeviceQueue(device, queueFamily, 0, &queue);
    return true;
}

bool Renderer::createSwapchain() {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &caps);
    int w = 0, h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    if (w == 0 || h == 0)
        return false;
    extent = caps.currentExtent.width != UINT32_MAX
                 ? caps.currentExtent
                 : VkExtent2D{std::clamp((uint32_t)w, caps.minImageExtent.width, caps.maxImageExtent.width),
                              std::clamp((uint32_t)h, caps.minImageExtent.height, caps.maxImageExtent.height)};
    uint32_t n = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &n, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(n);
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &n, formats.data());
    // UNORM like the original game: colours are written as-is, not gamma-encoded.
    VkSurfaceFormatKHR format = formats[0];
    for (auto &f : formats)
        if ((f.format == VK_FORMAT_B8G8R8A8_UNORM || f.format == VK_FORMAT_R8G8B8A8_UNORM) &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            format = f;
            break;
        }
    swapFormat = format.format;
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &n, nullptr);
    std::vector<VkPresentModeKHR> modes(n);
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &n, modes.data());
    VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;
    if (!settings.vsync)
        for (auto m : {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR})
            if (std::find(modes.begin(), modes.end(), m) != modes.end()) {
                mode = m;
                break;
            }
    VkSwapchainCreateInfoKHR sci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    sci.surface = surface;
    sci.minImageCount = std::max(caps.minImageCount, 3u);
    if (caps.maxImageCount)
        sci.minImageCount = std::min(sci.minImageCount, caps.maxImageCount);
    sci.imageFormat = format.format;
    sci.imageColorSpace = format.colorSpace;
    sci.imageExtent = extent;
    sci.imageArrayLayers = 1;
    canCapture = caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | (canCapture ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0);
    sci.preTransform = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = mode;
    sci.clipped = VK_TRUE;
    if (vkCreateSwapchainKHR(device, &sci, nullptr, &swapchain) != VK_SUCCESS)
        return false;
    vkGetSwapchainImagesKHR(device, swapchain, &n, nullptr);
    swapImages.resize(n);
    vkGetSwapchainImagesKHR(device, swapchain, &n, swapImages.data());
    for (auto img : swapImages) {
        VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        v.image = img;
        v.viewType = VK_IMAGE_VIEW_TYPE_2D;
        v.format = swapFormat;
        v.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VkImageView view;
        vkCreateImageView(device, &v, nullptr, &view);
        swapViews.push_back(view);
        VkSemaphoreCreateInfo s{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkSemaphore done;
        vkCreateSemaphore(device, &s, nullptr, &done);
        renderDone.push_back(done);
    }
    // Scene, depth and anti-aliasing targets (once the post resources exist; init
    // creates them after the pipelines).
    return !postPool || createPostTargets();
}

void Renderer::destroySwapchain() {
    destroyPostTargets();
    for (auto v : swapViews)
        vkDestroyImageView(device, v, nullptr);
    for (auto s : renderDone)
        vkDestroySemaphore(device, s, nullptr);
    swapViews.clear();
    renderDone.clear();
    swapImages.clear();
    if (swapchain)
        vkDestroySwapchainKHR(device, swapchain, nullptr);
    swapchain = VK_NULL_HANDLE;
}

bool Renderer::createPipelines(std::string &error) {
    auto module = [&](const uint32_t *code, size_t bytes) {
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        info.codeSize = bytes;
        info.pCode = code;
        VkShaderModule m = VK_NULL_HANDLE;
        vkCreateShaderModule(device, &info, nullptr, &m);
        return m;
    };
    VkShaderModule sceneV = module(spv_scene_vert, sizeof(spv_scene_vert)),
                   sceneF = module(spv_scene_frag, sizeof(spv_scene_frag)),
                   shadowV = module(spv_shadow_vert, sizeof(spv_shadow_vert)),
                   waterF = module(spv_water_frag, sizeof(spv_water_frag)),
                   skyV = module(spv_sky_vert, sizeof(spv_sky_vert)),
                   skyF = module(spv_sky_frag, sizeof(spv_sky_frag)),
                   alphaV = module(spv_alpha_scene_vert, sizeof(spv_alpha_scene_vert)),
                   alphaF = module(spv_alpha_scene_frag, sizeof(spv_alpha_scene_frag)),
                   alphaWaterF = module(spv_alpha_water_frag, sizeof(spv_alpha_water_frag)),
                   alphaSkyF = module(spv_alpha_sky_frag, sizeof(spv_alpha_sky_frag)),
                   spriteV = module(spv_alpha_sprite_vert, sizeof(spv_alpha_sprite_vert)),
                   spriteF = module(spv_alpha_sprite_frag, sizeof(spv_alpha_sprite_frag)),
                   alphaCloudF = module(spv_alpha_cloud_frag, sizeof(spv_alpha_cloud_frag)),
                   steamCloudF = module(spv_steam_cloud_frag, sizeof(spv_steam_cloud_frag)),
                   steamStarF = module(spv_steam_star_frag, sizeof(spv_steam_star_frag));

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
    VkPipelineVertexInputStateCreateInfo noInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    // The stars: one vec4 per instance (position on the sky sphere, size).
    VkVertexInputBindingDescription starBinding{0, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_INSTANCE};
    VkVertexInputAttributeDescription starAttribute{0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0};
    VkPipelineVertexInputStateCreateInfo starInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    starInput.vertexBindingDescriptionCount = 1;
    starInput.pVertexBindingDescriptions = &starBinding;
    starInput.vertexAttributeDescriptionCount = 1;
    starInput.pVertexAttributeDescriptions = &starAttribute;

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

    struct Desc {
        VkShaderModule vs, fs;
        bool vertices, depthWrite, blend, cull, shadow;
        VkCompareOp depthOp;
        VkPipeline *out;
        VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        const VkPipelineVertexInputStateCreateInfo *input = nullptr; // other than the voxel vertex
        VkColorComponentFlags colourMask = 0xf;
    };
    Desc descs[] = {
        {sceneV, sceneF, true, true, false, true, false, VK_COMPARE_OP_LESS, &scenePipeline},
        {sceneV, sceneF, true, true, false, true, false, VK_COMPARE_OP_LESS, &mirroredScenePipeline,
         VK_FRONT_FACE_CLOCKWISE},
        {sceneV, sceneF, true, true, false, false, false, VK_COMPARE_OP_LESS, &menuPipeline},
        {sceneV, sceneF, true, false, true, false, false, VK_COMPARE_OP_ALWAYS, &menuBackgroundPipeline},
        {sceneV, waterF, true, true, true, false, false, VK_COMPARE_OP_LESS_OR_EQUAL, &waterPipeline},
        {sceneV, waterF, true, true, false, false, false, VK_COMPARE_OP_LESS_OR_EQUAL, &waterDepthPipeline,
         VK_FRONT_FACE_COUNTER_CLOCKWISE, nullptr, 0},
        {shadowV, VK_NULL_HANDLE, true, true, false, false, true, VK_COMPARE_OP_LESS, &shadowPipeline},
        {skyV, skyF, false, false, false, false, false, VK_COMPARE_OP_LESS_OR_EQUAL, &skyPipeline},
        {alphaV, alphaF, true, true, false, true, false, VK_COMPARE_OP_LESS, &alphaScenePipeline},
        {alphaV, alphaF, true, true, false, true, false, VK_COMPARE_OP_LESS, &alphaMirroredPipeline,
         VK_FRONT_FACE_CLOCKWISE},
        // Alpha water: blended, no culling, depth writes on (docs/specs/alpha-rendering.md 12).
        {alphaV, alphaWaterF, true, true, true, false, false, VK_COMPARE_OP_LESS_OR_EQUAL, &alphaWaterPipeline},
        {skyV, alphaSkyF, false, false, false, false, false, VK_COMPARE_OP_LESS_OR_EQUAL, &alphaSkyPipeline},
        {spriteV, spriteF, true, false, true, false, false, VK_COMPARE_OP_LESS_OR_EQUAL, &alphaSpritePipeline,
         VK_FRONT_FACE_COUNTER_CLOCKWISE, &starInput},
        // The sky's models (docs/specs/sky-and-clouds.md): Alpha's opaque clouds; Steam's clouds,
        // blended so they can fade near the camera, writing depth so the faded ones can be laid
        // down first at no opacity; Steam's star cubes, blended over the sky behind the world.
        {alphaV, alphaCloudF, true, true, false, true, false, VK_COMPARE_OP_LESS, &alphaCloudPipeline},
        {sceneV, steamCloudF, true, true, true, true, false, VK_COMPARE_OP_LESS_OR_EQUAL, &steamCloudPipeline},
        {sceneV, steamStarF, true, false, true, true, false, VK_COMPARE_OP_LESS, &steamStarPipeline},
    };
    for (auto &d : descs) {
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                     VK_SHADER_STAGE_VERTEX_BIT, d.vs, "main"};
        stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                     VK_SHADER_STAGE_FRAGMENT_BIT, d.fs, "main"};
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = d.cull ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
        // Faces are counter-clockwise from outside; the projection's Y flip keeps that.
        raster.frontFace = d.frontFace;
        raster.lineWidth = 1;
        // Casters behind the light box's near plane still cast (pancaking).
        raster.depthClampEnable = d.shadow && depthClamp;
        VkPipelineDepthStencilStateCreateInfo depthState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depthState.depthTestEnable = VK_TRUE;
        depthState.depthWriteEnable = d.depthWrite;
        depthState.depthCompareOp = d.depthOp;
        VkPipelineColorBlendAttachmentState attachment{};
        attachment.colorWriteMask = d.colourMask;
        if (d.blend) {
            attachment.blendEnable = VK_TRUE;
            attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            attachment.colorBlendOp = VK_BLEND_OP_ADD;
            attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            attachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }
        VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        blend.attachmentCount = d.shadow ? 0 : 1;
        blend.pAttachments = &attachment;
        VkPipelineRenderingCreateInfo rendering{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        rendering.colorAttachmentCount = d.shadow ? 0 : 1;
        rendering.pColorAttachmentFormats = &swapFormat;
        rendering.depthAttachmentFormat = DEPTH_FORMAT;
        VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, &rendering};
        info.stageCount = d.fs ? 2 : 1;
        info.pStages = stages;
        info.pVertexInputState = d.input ? d.input : d.vertices ? &vertexInput : &noInput;
        info.pInputAssemblyState = &assembly;
        info.pViewportState = &viewport;
        info.pRasterizationState = &raster;
        info.pMultisampleState = &multisample;
        info.pDepthStencilState = &depthState;
        info.pColorBlendState = &blend;
        info.pDynamicState = &dynamic;
        info.layout = layout;
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, d.out) != VK_SUCCESS) {
            error = "Pipeline creation failed";
            return false;
        }
    }
    for (auto m : {sceneV, sceneF, shadowV, waterF, skyV, skyF, alphaV, alphaF, alphaWaterF, alphaSkyF, spriteV, spriteF,
                   alphaCloudF, steamCloudF, steamStarF})
        vkDestroyShaderModule(device, m, nullptr);
    return true;
}

bool Renderer::createOverlayPipeline(std::string &error) {
    VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    si.magFilter = si.minFilter = VK_FILTER_NEAREST; // Pixel art stays crisp.
    si.addressModeU = si.addressModeV = si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vkCreateSampler(device, &si, nullptr, &overlaySampler);
    VkDescriptorSetLayoutBinding binding{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT};
    VkDescriptorSetLayoutCreateInfo sli{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    sli.bindingCount = 1;
    sli.pBindings = &binding;
    vkCreateDescriptorSetLayout(device, &sli, nullptr, &overlaySetLayout);
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1};
    VkDescriptorPoolCreateInfo dpi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    dpi.maxSets = 1;
    dpi.poolSizeCount = 1;
    dpi.pPoolSizes = &size;
    vkCreateDescriptorPool(device, &dpi, nullptr, &overlayPool);
    VkDescriptorSetAllocateInfo dai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    dai.descriptorPool = overlayPool;
    dai.descriptorSetCount = 1;
    dai.pSetLayouts = &overlaySetLayout;
    vkAllocateDescriptorSets(device, &dai, &overlaySet);
    VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 2};
    VkPipelineLayoutCreateInfo pli{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pli.setLayoutCount = 1;
    pli.pSetLayouts = &overlaySetLayout;
    pli.pushConstantRangeCount = 1;
    pli.pPushConstantRanges = &push;
    vkCreatePipelineLayout(device, &pli, nullptr, &overlayLayout);

    auto module = [&](const uint32_t *code, size_t bytes) {
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        info.codeSize = bytes;
        info.pCode = code;
        VkShaderModule m = VK_NULL_HANDLE;
        vkCreateShaderModule(device, &info, nullptr, &m);
        return m;
    };
    VkShaderModule vs = module(spv_overlay_vert, sizeof(spv_overlay_vert)),
                   fs = module(spv_overlay_frag, sizeof(spv_overlay_frag));
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vs, "main"};
    stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fs, "main"};
    VkVertexInputBindingDescription vb{0, sizeof(OverlayVertex), VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attributes[] = {
        {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(OverlayVertex, x)},
        {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(OverlayVertex, u)},
        {2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(OverlayVertex, r)},
    };
    VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    input.vertexBindingDescriptionCount = 1;
    input.pVertexBindingDescriptions = &vb;
    input.vertexAttributeDescriptionCount = 3;
    input.pVertexAttributeDescriptions = attributes;
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
    // Drawn after anti-aliasing, straight onto the swapchain image (no depth).
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
    info.layout = overlayLayout;
    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &overlayPipeline);
    vkDestroyShaderModule(device, vs, nullptr);
    vkDestroyShaderModule(device, fs, nullptr);
    if (result != VK_SUCCESS) {
        error = "Interface pipeline creation failed";
        return false;
    }
    return true;
}

bool Renderer::setOverlayAtlas(uint32_t width, uint32_t height, const uint8_t *rgba) {
    if (noGpu)
        return true;
    vkDeviceWaitIdle(device);
    destroyImage(overlayImage);
    overlayImage = createImage(width, height, VK_FORMAT_R8G8B8A8_UNORM,
                               VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    if (!overlayImage.image)
        return false;
    VkDeviceSize bytes = (VkDeviceSize)width * height * 4;
    Buffer staging = createBuffer(bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!staging.buffer)
        return false;
    std::memcpy(staging.mapped, rgba, bytes);
    VkCommandBufferAllocateInfo cai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cai.commandPool = pool;
    cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cai.commandBufferCount = 1;
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(device, &cai, &cmd);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    auto toCopy = imageBarrier(overlayImage.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_NONE, 0,
                               VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
    barriers(cmd, &toCopy, 1);
    VkBufferImageCopy region{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {width, height, 1};
    vkCmdCopyBufferToImage(cmd, staging.buffer, overlayImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    auto toRead = imageBarrier(overlayImage.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT,
                               VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                               VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
    barriers(cmd, &toRead, 1);
    vkEndCommandBuffer(cmd);
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(device, pool, 1, &cmd);
    destroyBuffer(staging);
    VkDescriptorImageInfo image{overlaySampler, overlayImage.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = overlaySet;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image;
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    overlayWidth = width;
    overlayHeight = height;
    return true;
}

void Renderer::initHeadless(const RenderSettings &s) {
    settings = s;
    setRenderStyle(settings.style);
    noGpu = true;
    gpuName = "none (dedicated server)";
    extent = {1600, 900};
}

bool Renderer::init(GLFWwindow *w, const RenderSettings &s, std::string &error) {
    window = w;
    settings = s;
    setRenderStyle(settings.style); // meshes built from now on bake their light for this look
    if (!createDevice(error))
        return false;
    if (!createSwapchain()) {
        error = "Swapchain creation failed";
        return false;
    }
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamily;
    vkCreateCommandPool(device, &poolInfo, nullptr, &pool);

    // Shadow maps and their comparison sampler. Outside the map counts as lit.
    uint32_t nearRes = settings.cascades ? settings.nearResolution : 16;
    shadowFar = createImage(settings.resolution, settings.resolution, DEPTH_FORMAT,
                            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                            VK_IMAGE_ASPECT_DEPTH_BIT);
    shadowNear = createImage(nearRes, nearRes, DEPTH_FORMAT,
                             VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                             VK_IMAGE_ASPECT_DEPTH_BIT);
    if (!shadowFar.image || !shadowNear.image) {
        error = "Cannot allocate shadow maps (lower [Shadows] Resolution)";
        return false;
    }
    VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    si.magFilter = si.minFilter = VK_FILTER_LINEAR;
    si.addressModeU = si.addressModeV = si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    si.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    si.compareEnable = VK_TRUE;
    si.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    si.maxLod = 0;
    vkCreateSampler(device, &si, nullptr, &shadowSampler);

    VkDescriptorSetLayoutBinding bindings[] = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
    };
    VkDescriptorSetLayoutCreateInfo sli{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    sli.bindingCount = 3;
    sli.pBindings = bindings;
    vkCreateDescriptorSetLayout(device, &sli, nullptr, &setLayout);
    VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                             sizeof(PushConstants)};
    VkPipelineLayoutCreateInfo pli{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pli.setLayoutCount = 1;
    pli.pSetLayouts = &setLayout;
    pli.pushConstantRangeCount = 1;
    pli.pPushConstantRanges = &push;
    vkCreatePipelineLayout(device, &pli, nullptr, &layout);
    VkDescriptorPoolSize sizes[] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, FRAMES},
                                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 * FRAMES}};
    VkDescriptorPoolCreateInfo dpi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    dpi.maxSets = FRAMES;
    dpi.poolSizeCount = 2;
    dpi.pPoolSizes = sizes;
    vkCreateDescriptorPool(device, &dpi, nullptr, &descriptorPool);

    for (auto &fr : frames) {
        VkCommandBufferAllocateInfo cai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cai.commandPool = pool;
        cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cai.commandBufferCount = 1;
        vkAllocateCommandBuffers(device, &cai, &fr.cmd);
        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(device, &fi, nullptr, &fr.fence);
        VkSemaphoreCreateInfo semi{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(device, &semi, nullptr, &fr.acquired);
        fr.uniforms = createBuffer(sizeof(FrameUniforms), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        VkDescriptorSetAllocateInfo dai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        dai.descriptorPool = descriptorPool;
        dai.descriptorSetCount = 1;
        dai.pSetLayouts = &setLayout;
        vkAllocateDescriptorSets(device, &dai, &fr.set);
        VkDescriptorBufferInfo bi{fr.uniforms.buffer, 0, sizeof(FrameUniforms)};
        VkDescriptorImageInfo farInfo{shadowSampler, shadowFar.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkDescriptorImageInfo nearInfo{shadowSampler, shadowNear.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet writes[3]{};
        for (int i = 0; i < 3; i++) {
            writes[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            writes[i].dstSet = fr.set;
            writes[i].dstBinding = i;
            writes[i].descriptorCount = 1;
            writes[i].descriptorType = i == 0 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                              : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        }
        writes[0].pBufferInfo = &bi;
        writes[1].pImageInfo = &farInfo;
        writes[2].pImageInfo = &nearInfo;
        vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);
    }
    if (!createPipelines(error) || !createOverlayPipeline(error) || !createMapPipeline(error) || !createPostResources(error))
        return false;
    if (!createPostTargets()) {
        error = "Cannot allocate the scene and anti-aliasing targets";
        return false;
    }
    // The Alpha look's stars, made once (docs/specs/alpha-rendering.md 8.3).
    std::vector<alpha::Star> stars = alpha::stars();
    starBuffer = createBuffer(stars.size() * sizeof(glm::vec4), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (starBuffer.mapped) {
        auto *out = (glm::vec4 *)starBuffer.mapped;
        for (size_t i = 0; i < stars.size(); i++)
            out[i] = glm::vec4(stars[i].position, stars[i].size);
        starCount = (uint32_t)stars.size();
    }
    steamStars = steamsky::starDome(); // the Steam look's dome (sky-and-clouds.md §B.6)
    VkQueryPoolCreateInfo qi{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    qi.queryType = VK_QUERY_TYPE_TIMESTAMP;
    qi.queryCount = 3 * FRAMES; // per frame: start, before anti-aliasing, end
    vkCreateQueryPool(device, &qi, nullptr, &timing);
    return true;
}

uint32_t Renderer::upload(const Mesh &mesh) {
    if (mesh.empty())
        return 0;
    if (noGpu)
        return ++headlessIds;
    VkDeviceSize vbytes = mesh.vertices.size() * sizeof(Vertex);
    VkDeviceSize ibytes = mesh.indices.size() * sizeof(uint32_t);
    VkDeviceSize indexOffset = (vbytes + 15) & ~VkDeviceSize(15);
    VkDeviceSize total = indexOffset + ibytes;
    Buffer staging = createBuffer(total, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!staging.buffer)
        return 0;
    std::memcpy(staging.mapped, mesh.vertices.data(), vbytes);
    std::memcpy((char *)staging.mapped + indexOffset, mesh.indices.data(), ibytes);
    GpuMesh gm;
    gm.buffer = createBuffer(total,
                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                 VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (!gm.buffer.buffer) {
        destroyBuffer(staging);
        return 0;
    }
    gm.indexOffset = indexOffset;
    gm.indexCount = (uint32_t)mesh.indices.size();
    gm.min = mesh.min;
    gm.max = mesh.max;
    gm.live = true;
    uint32_t id;
    if (!freeIds.empty()) {
        id = freeIds.back();
        freeIds.pop_back();
        meshes[id] = gm;
    } else {
        id = (uint32_t)meshes.size();
        meshes.push_back(gm);
    }
    pending.push_back({staging, id, total});
    return id;
}

void Renderer::destroyMesh(uint32_t id) {
    if (id == 0 || id >= meshes.size() || !meshes[id].live)
        return;
    destroyBuffer(meshes[id].buffer);
    meshes[id] = {};
    freeIds.push_back(id);
}

void Renderer::release(uint32_t id) {
    if (noGpu)
        return;
    if (id == 0 || id >= meshes.size() || !meshes[id].live)
        return;
    // The frame currently being built (or the last submitted one) may use it:
    // destroy after the next wait on this frame slot's fence.
    frames[frameIndex].releases.push_back(id);
    // Not drawable from now on.
    meshes[id].indexCount = 0;
}

float Renderer::lightOfBlock(glm::vec3 p) {
    if (!worldProbe.light)
        return 1;
    glm::ivec3 b = glm::ivec3(glm::floor(p));
    uint64_t key = (uint64_t)((uint32_t)b.x & 0x1fffff) << 42 | (uint64_t)((uint32_t)b.y & 0x1fffff) << 21 |
                   ((uint32_t)b.z & 0x1fffff);
    if (auto it = blockLight.find(key); it != blockLight.end())
        return lastBlockLight = it->second;
    // Each new block is a small light flood: a few per frame, the rest keep the last value
    // until a later frame works theirs out.
    if (blockLightBudget <= 0)
        return lastBlockLight;
    blockLightBudget--;
    if (blockLight.size() > 100000)
        blockLight.clear();
    return lastBlockLight = blockLight[key] = worldProbe.light(p);
}

const SkyColumn &Renderer::cameraColumn(glm::dvec3 game) {
    static const SkyColumn none;
    if (!worldProbe.column)
        return none;
    const glm::ivec2 at((int32_t)std::floor(game.x), (int32_t)std::floor(game.y));
    if (!skyColumnValid || at != skyColumnAt) {
        skyColumn = worldProbe.column(at.x, at.y);
        skyColumnAt = at;
        skyColumnValid = true;
    }
    return skyColumn;
}

const std::optional<clouds::Cloud> &Renderer::cloudOfCell(int32_t cx, int32_t cy, bool alphaLook) {
    const auto key = std::make_pair(cx, cy);
    if (auto it = cloudCells.find(key); it != cloudCells.end())
        return it->second;
    if (cloudCells.size() > 4096) // far behind: worked out again if the camera comes back
        cloudCells.clear();
    // A cell asks about up to two columns, several values each: keep the last one asked.
    glm::ivec2 lastAt{0};
    SkyColumn last;
    bool haveLast = false;
    auto column = [&](int32_t x, int32_t y) -> const SkyColumn & {
        if (!haveLast || lastAt != glm::ivec2(x, y)) {
            last = worldProbe.column(x, y);
            lastAt = {x, y};
            haveLast = true;
        }
        return last;
    };
    std::optional<clouds::Cloud> cloud;
    if (alphaLook) {
        clouds::AlphaWorld world;
        world.moisture = [&](int32_t x, int32_t y) { return column(x, y).zone.moisture; };
        world.mountain = [&](int32_t x, int32_t y) { return column(x, y).mountain; };
        world.ground = [&](int32_t x, int32_t y) { return column(x, y).region.ground; };
        cloud = clouds::alphaCloud(cx, cy, world);
    } else {
        clouds::SteamWorld world;
        world.dryness = [&](int32_t x, int32_t y) { return column(x, y).region.dryness; };
        world.ground = [&](int32_t x, int32_t y) { return column(x, y).cloudGround; };
        world.forest = [&](int32_t x, int32_t y) { return column(x, y).region.forest; };
        cloud = clouds::steamCloud(cx, cy, world);
    }
    return cloudCells[key] = cloud;
}

void Renderer::frame(const Camera &camera, const Lighting &lighting, glm::vec3 focus, double time,
                     const std::vector<DrawItem> &opaque, const std::vector<DrawItem> &water) {
    if (noGpu)
        return;
    Frame &fr = frames[frameIndex];
    vkWaitForFences(device, 1, &fr.fence, VK_TRUE, UINT64_MAX);
    // This slot's timestamps from its last frame are complete now.
    if (timing && timingWritten[frameIndex]) {
        uint64_t t[3];
        if (vkGetQueryPoolResults(device, timing, 3 * frameIndex, 3, sizeof t, t, sizeof(uint64_t),
                                  VK_QUERY_RESULT_64_BIT) == VK_SUCCESS) {
            gpuMsSum += (t[2] - t[0]) * timestampPeriod * 1e-6;
            postMsSum += (t[2] - t[1]) * timestampPeriod * 1e-6;
            timingSamples++;
        }
    }
    for (auto &b : fr.staging)
        destroyBuffer(b);
    fr.staging.clear();
    // Releases recorded while this slot was in flight were also unused by the
    // other slot's newer frame only if released before it; wait for both.
    if (!fr.releases.empty()) {
        Frame &other = frames[(frameIndex + 1) % FRAMES];
        vkWaitForFences(device, 1, &other.fence, VK_TRUE, UINT64_MAX);
        for (auto id : fr.releases)
            destroyMesh(id);
        fr.releases.clear();
    }

    uint32_t imageIndex = 0;
    VkResult acquire = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, fr.acquired,
                                             VK_NULL_HANDLE, &imageIndex);
    if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
        vkDeviceWaitIdle(device);
        destroySwapchain();
        createSwapchain();
        return;
    }
    if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR)
        return;
    vkResetFences(device, 1, &fr.fence);

    // --- Uniforms ---
    float aspect = extent.height ? (float)extent.width / extent.height : camera.aspect;
    glm::mat4 view = glm::lookAtRH(camera.eye, camera.target, glm::vec3(0, 0, 1));
    const float farPlane = std::max(settings.fogDistance * ALPHA_CLOUD_REACH,
                                    settings.fogDistance + clouds::STEAM_CULL_BEYOND + clouds::STEAM_CULL_RADIUS) +
                           FAR_MARGIN;
    glm::mat4 proj = glm::perspectiveRH_ZO(glm::radians(settings.fov), aspect, .1f, farPlane);
    proj[1][1] *= -1;
    // TAA: eight-frame Halton sub-pixel jitter (the Steam mod's), content moved by
    // `jitter` pixels; the resolve samples back at the same offset.
    glm::vec2 jitter(0);
    if (settings.taa) {
        auto halton = [](uint32_t i, uint32_t base) {
            float f = 1, r = 0;
            while (i) {
                f /= base;
                r += f * (i % base);
                i /= base;
            }
            return r;
        };
        aaFrame++;
        jitter = glm::vec2(halton(aaFrame % 8 + 1, 2) - .5f, halton(aaFrame % 8 + 1, 3) - .5f) * settings.jitterScale;
        proj[2][0] -= 2 * jitter.x / extent.width;
        proj[2][1] -= 2 * jitter.y / extent.height;
    }
    FrameUniforms u{};
    u.viewProj = proj * view;
    Frustum viewFrustum(u.viewProj);

    // --- The Alpha look (docs/specs/alpha-rendering.md): clock, palette, fog, lights ---
    // The original menu's captured scene keeps the look it was captured with.
    const bool menuSky = lighting.menuSky[5].w > 0;
    const bool alphaLook = settings.style == RenderStyle::Alpha && !menuSky;
    const double dayMs = std::fmod(std::max(0.0, (double)lighting.hour), 24.0) * 3600000.0;
    const float S = alpha::sunlight(dayMs);
    const glm::vec3 D = alpha::sunDirection(dayMs);
    // Milliseconds since the last frame (-1 on the first, or when the clock went back).
    const double elapsedMs = lastFrameTime < 0 || time < lastFrameTime ? -1 : (time - lastFrameTime) * 1000;
    // The camera in the game's block coordinates, for the sky's columns and cells.
    const glm::dvec3 eyeGame = glm::dvec3(camera.eye) + worldProbe.origin;
    // §7.2 (alpha-rendering.md), both looks: the sky gradient follows the camera's tilt (its
    // angle from straight up).
    const glm::vec3 forward = glm::normalize(camera.target - camera.eye);
    const float gradientShift =
        alpha::fogGradientTranslation(glm::degrees(std::acos(std::clamp(forward.z, -1.f, 1.f))));
    u.alphaView = glm::vec4(gradientShift, 2.f / std::max(1u, extent.width), 2.f / std::max(1u, extent.height), 0);
    // §7.1: the fog distance follows the nearest unloaded ground, smoothed over time.
    {
        float radius = settings.fogDistance;
        float nearest = unloadedDistance >= 0 ? std::min(unloadedDistance, radius) : radius;
        float target = alpha::FogDistance::target(nearest);
        if (lastFrameTime < 0 || time < lastFrameTime)
            fogDistance.reset(target);
        else
            fogDistance.update(target, (time - lastFrameTime) * 1000);
        lastFrameTime = time;
    }
    const float fogF = std::max(fogDistance.value(), 1.f);
    blockLightBudget = 8;
    // §10.1: lamps, torches and fires light up as the sun goes, nearest first.
    struct PointLight {
        glm::vec3 at;
        float radius;
        glm::vec3 colour;
        float distance;
    };
    std::vector<PointLight> pointLights;
    bool underwater = false;
    glm::vec3 mapFog(0); // the world map's backdrop: this frame's fog colour
    if (alphaLook) {
        for (const DrawItem &item : opaque) {
            if (item.lightRadius <= 0)
                continue;
            float distance = glm::length(item.lightPosition - camera.eye);
            if (distance > fogF + 22.4f || !viewFrustum.sphere(item.lightPosition, 32))
                continue;
            pointLights.push_back({item.lightPosition, item.lightRadius, item.lightColour * (1 - S), distance});
        }
        std::sort(pointLights.begin(), pointLights.end(),
                  [](const PointLight &a, const PointLight &b) { return a.distance < b.distance; });
        if (pointLights.size() > MAX_POINT_LIGHTS)
            pointLights.resize(MAX_POINT_LIGHTS);
        for (size_t i = 0; i < pointLights.size(); i++) {
            u.pointLights[i] = glm::vec4(pointLights[i].at, pointLights[i].radius);
            u.pointColours[i] = glm::vec4(pointLights[i].colour, 0);
        }

        underwater = worldProbe.water && worldProbe.water(camera.eye);
        alpha::ZoneFactors zone = worldProbe.zone ? worldProbe.zone(camera.eye) : alpha::ZoneFactors{};
        alpha::Palette palette = alpha::palette(alpha::paletteDaylight(dayMs), zone, underwater);
        u.alphaSky1 = glm::vec4(palette.sky1, 1);
        u.alphaSky2 = glm::vec4(palette.sky2, 1);
        u.alphaFog = glm::vec4(palette.fog, 1);
        u.alphaFront = glm::vec4(palette.front, 1);
        u.alphaBack = glm::vec4(palette.back, 1);
        u.alphaAmbient = glm::vec4(palette.ambient, 1);
        u.alphaMaterial = alpha::material(S, underwater);
        u.alphaSun = glm::vec4(D, 1 / fogF);
        mapFog = palette.fog;
        u.sunColour = alpha::sunSpriteColour(S);
        // CubeVulkan's moon (src/AlphaMoon.h): opposite the sun, at night.
        alpha::MoonLight moon = alpha::moonLight(dayMs, settings.moon);
        u.alphaMoonFront = glm::vec4(moon.front, moon.strength);
        u.alphaMoonBack = glm::vec4(moon.back, 0); // w: set with the shadows below
        u.alphaNight = glm::vec4(moon.ambient, alpha::moonPhase(worldDay));
        // §8.4: the sky camera's own 50 degree lens, with this frame's jitter.
        glm::mat4 skyProj = glm::perspectiveRH_ZO(glm::radians(50.f), aspect, .1f, 2000.f);
        skyProj[1][1] *= -1;
        skyProj[2][0] = proj[2][0];
        skyProj[2][1] = proj[2][1];
        u.skyProj = skyProj;
        u.view = view;
    }
    // --- The Steam look's sky (docs/specs/sky-and-clouds.md part B) ---
    float steamStarValue = 1;
    if (!alphaLook && menuSky) { // the title screen's sky, as the game's menu drew it
        std::copy(std::begin(lighting.menuSky), std::end(lighting.menuSky), u.steamSky);
        u.steamSky[5].y = 1;
    } else if (!alphaLook) {
        underwater = worldProbe.water && worldProbe.water(camera.eye);
        const SkyColumn &here = cameraColumn(eyeGame);
        // §B.3, eased as §B.4.
        const steamsky::Palette target = steamsky::palette(alpha::paletteDaylight(dayMs), here.region, underwater);
        if (elapsedMs < 0)
            steamPalette.reset();
        steamPalette.update(target, std::max(elapsedMs, 0.0));
        const steamsky::Palette &palette = steamPalette.value();
        // §B.5: the sun's glow (from straight below while under water).
        const glm::vec3 sun = underwater ? glm::vec3(0, 0, -1) : steamsky::sunDirection(dayMs);
        u.steamSky[0] = glm::vec4(palette.sky1, 1);
        u.steamSky[1] = glm::vec4(palette.sky2, 1);
        u.steamSky[2] = glm::vec4(palette.fog, 1);
        u.steamSky[3] = glm::vec4(palette.sun, 1);
        u.steamSky[4] = steamsky::sunPosition(sun, view, proj, aspect, gradientShift);
        u.steamSky[5] = glm::vec4(gradientShift, 0, underwater ? 1 : 0, 1);
        mapFog = palette.fog;
        steamStarValue = steamsky::starValue(dayMs, here.region.forest);
    }
    // History is released on toggles and large camera jumps (teleports, respawns).
    if (settings.taa != previousTaa || glm::length(camera.eye - previousEye) > 8)
        historyValid = false;
    glm::mat4 clipToPrevious = previousClip * glm::inverse(u.viewProj);
    u.invViewProj = glm::inverse(u.viewProj);
    // Shadows follow the clock's light. In the Alpha look that is the sun by day and
    // CubeVulkan's moon by night, each fading out towards the horizon, and they take away only
    // that light's own term.
    glm::vec3 shadowDir = lighting.lightDir;
    float shadowStrength = lighting.shadowStrength;
    if (alphaLook) {
        // The direction moves in steps of half a degree of the sun's path (two game minutes),
        // not every frame: re-rasterising the maps from a slowly turning light makes every
        // edge crawl (shadows.md §4, shimmer 1). The texel snap in lightMatrix handles moves.
        const double stepMs = SHADOW_DIRECTION_STEP_MS;
        alpha::ShadowLight caster = alpha::shadowLight(std::floor(dayMs / stepMs) * stepMs, settings.sunShadowStrength, settings.moon);
        shadowDir = caster.direction;
        shadowStrength = caster.strength;
        u.alphaMoonBack.w = caster.moon ? 1.f : 0.f;
    }
    bool shadowsOn = settings.shadows && shadowStrength > 0.001f;
    float farRadius = settings.radius, nearRadius = settings.nearRadius;
    u.lightFar = lightMatrix(shadowDir, focus, farRadius, settings.resolution);
    u.lightNear = lightMatrix(shadowDir, focus, nearRadius, settings.nearResolution);
    u.eye = glm::vec4(camera.eye, (float)std::fmod(time, 100000.0));
    u.lightDir = glm::vec4(shadowDir, shadowsOn ? shadowStrength : 0.f);
    u.front = glm::vec4(lighting.front, settings.sunWarmth);
    u.back = glm::vec4(lighting.back, settings.fadeStart);
    u.ambient = glm::vec4(lighting.ambient, settings.cameraFade ? 1.f : 0.f);
    u.fog = glm::vec4(mapFog, settings.fogDistance);
    u.sky = glm::vec4(0, 0, 0, lighting.daylight);
    float invSpan = 1 / (2 * LIGHT_DEPTH);
    // 0.008 blocks of depth bias, as in SteamShadows.
    u.shadowFar = glm::vec4(1.f / settings.resolution, .008f * invSpan, farRadius, invSpan);
    u.shadowNear = glm::vec4(1.f / settings.nearResolution, .008f * invSpan, nearRadius,
                             settings.cascades ? 1.f : 0.f);
    u.wind = glm::vec4(settings.leafAmplitude, settings.windSpeed, settings.leafRange,
                       settings.wind ? 1.f : 0.f);
    u.focus = glm::vec4(focus, alphaLook ? 0.f : 12.f); // MoonandShadow's 12-block lantern (Steam look).
    u.sunDisk = glm::vec4(lighting.sunDir, glm::smoothstep(-.1f, .05f, lighting.sunDir.z));
    u.moonDisk = glm::vec4(lighting.moonDir, lighting.moonWeight);
    std::memcpy(fr.uniforms.mapped, &u, sizeof(u));

    VkCommandBuffer cmd = fr.cmd;
    vkResetCommandBuffer(cmd, 0);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    if (timing) {
        vkCmdResetQueryPool(cmd, timing, 3 * frameIndex, 3);
        vkCmdWriteTimestamp2(cmd, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, timing, 3 * frameIndex);
    }

    // --- Pending mesh uploads ---
    if (!pending.empty()) {
        for (auto &p : pending) {
            if (meshes[p.mesh].live) {
                VkBufferCopy copy{0, 0, p.size};
                vkCmdCopyBuffer(cmd, p.staging.buffer, meshes[p.mesh].buffer.buffer, 1, &copy);
            }
            fr.staging.push_back(p.staging);
        }
        pending.clear();
        VkMemoryBarrier2 mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
        mb.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        mb.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        mb.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
        mb.dstAccessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_2_INDEX_READ_BIT;
        barriers(cmd, nullptr, 0, &mb);
    }

    unsigned draws = 0;
    // Alpha look: the point lights a draw is lit by. Terrain takes the lights that reach its
    // bounds; models take the first of the frame-wide list (docs/specs/alpha-rendering.md 10.2).
    auto lightSetOf = [&](const DrawItem &item) {
        glm::uvec4 set(0);
        uint32_t count = 0;
        const GpuMesh &m = meshes[item.mesh];
        for (size_t i = 0; i < pointLights.size() && count < LIGHTS_PER_DRAW; i++) {
            const PointLight &l = pointLights[i];
            if (item.kind != DrawKind::Terrain) {
                if (i >= FRAME_LIGHTS)
                    break;
            } else if (glm::any(glm::lessThan(l.at + l.radius, m.min)) || glm::any(glm::greaterThan(l.at - l.radius, m.max)))
                continue;
            set[count / 4] |= (uint32_t)i << (8 * (count % 4));
            count++;
        }
        set.w = count;
        return set;
    };
    // shading: false for the shadow maps, which need no light.
    auto bindAndDraw = [&](const DrawItem &item, float cascade, bool shading = true) {
        const GpuMesh &m = meshes[item.mesh];
        VkDeviceSize zero = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &m.buffer.buffer, &zero);
        vkCmdBindIndexBuffer(cmd, m.buffer.buffer, m.indexOffset, VK_INDEX_TYPE_UINT32);
        PushConstants pc{item.model, glm::vec4((float)item.flags, cascade, item.height, 0), glm::vec4(1, 0, 0, 0),
                         glm::uvec4(0)};
        if (alphaLook && shading) {
            // Creatures and items take the light of the block just above where they are (11.2).
            // Zone models take the light where they stand too (vegetation-wind.md §4.5), which
            // the world works out with their zone (standingLight).
            float standing = item.standingLight >= 0 ? item.standingLight
                             : item.kind == DrawKind::Model ? lightOfBlock(glm::vec3(item.model[3]) + glm::vec3(0, 0, .1f))
                                                            : 1.f;
            pc.look = glm::vec4(standing, item.flash, item.shimmer ? 1.f : 0.f, 0);
            pc.lights = lightSetOf(item);
        }
        vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(pc), &pc);
        vkCmdDrawIndexed(cmd, m.indexCount, 1, 0, 0, 0);
        draws++;
    };
    auto sphereOf = [&](const DrawItem &item, glm::vec3 &center, float &radius) {
        const GpuMesh &m = meshes[item.mesh];
        center = glm::vec3(item.model * glm::vec4((m.min + m.max) * .5f, 1));
        float scale = glm::length(glm::vec3(item.model[0]));
        // (padded by the most a plant's top sways, vegetation-wind.md §4.1, or the leaf option's)
        radius = glm::length(m.max - m.min) * .5f * scale + std::max(PLANT_SWAY_REACH, settings.leafAmplitude);
    };

    // --- Shadow maps ---
    auto shadowPass = [&](Image &map, uint32_t res, const glm::mat4 &matrix, float cascade,
                          float radius) {
        auto toWrite = imageBarrier(map.image, VK_IMAGE_ASPECT_DEPTH_BIT,
                                    shadowsInitialized ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                                       : VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                    VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, 0,
                                    VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                                        VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                                    VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
        barriers(cmd, &toWrite, 1);
        VkRenderingAttachmentInfo depthAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        depthAttachment.imageView = map.view;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.clearValue.depthStencil = {1, 0};
        VkRenderingInfo ri{VK_STRUCTURE_TYPE_RENDERING_INFO};
        ri.renderArea = {{0, 0}, {res, res}};
        ri.layerCount = 1;
        ri.pDepthAttachment = &depthAttachment;
        vkCmdBeginRendering(cmd, &ri);
        if (shadowsOn) {
            VkViewport vp{0, 0, (float)res, (float)res, 0, 1};
            VkRect2D sc{{0, 0}, {res, res}};
            vkCmdSetViewport(cmd, 0, 1, &vp);
            vkCmdSetScissor(cmd, 0, 1, &sc);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &fr.set, 0, nullptr);
            Frustum lightFrustum(matrix);
            for (auto &item : opaque) {
                if (!item.castShadow || !meshes[item.mesh].indexCount)
                    continue;
                glm::vec3 c;
                float r;
                sphereOf(item, c, r);
                // Small props only cast inside the near map's radius.
                if (r < 2 && glm::length(glm::vec2(c - focus)) > std::min(radius, settings.sceneryDistance))
                    continue;
                // Light frustum, ignoring its near plane (depth clamp catches casters behind it).
                bool inside = true;
                for (int i = 0; i < 4 && inside; i++)
                    inside = glm::dot(glm::vec3(lightFrustum.planes[i]), c) + lightFrustum.planes[i].w >= -r;
                if (inside)
                    bindAndDraw(item, cascade, false);
            }
        }
        vkCmdEndRendering(cmd);
        auto toRead = imageBarrier(map.image, VK_IMAGE_ASPECT_DEPTH_BIT,
                                   VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                   VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                                   VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                   VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
        barriers(cmd, &toRead, 1);
    };
    // Maps are only redrawn while shadows are visible; otherwise they are
    // cleared once so the descriptors always reference valid contents.
    if (shadowsOn || !shadowsInitialized) {
        shadowPass(shadowFar, settings.resolution, u.lightFar, 0, farRadius);
        shadowPass(shadowNear, settings.cascades ? settings.nearResolution : 16, u.lightNear, 1, nearRadius);
        shadowsInitialized = true;
    }

    // --- The clouds of this frame (docs/specs/sky-and-clouds.md §A.5, §B.7) ---
    struct CloudDraw {
        uint32_t mesh;
        glm::mat4 model;
        float opacity;
    };
    std::vector<CloudDraw> cloudDraws;
    auto cloudMatrix = [&](const clouds::Cloud &c) {
        const glm::vec3 base = glm::vec3(c.base - worldProbe.origin);
        glm::mat4 m = glm::translate(glm::mat4(1), base);
        m = glm::scale(m, glm::vec3(c.scale));
        return glm::rotate(m, glm::radians(c.turnDegrees), glm::vec3(0, 0, 1));
    };
    const int32_t centreX = clouds::cellOf(eyeGame.x), centreY = clouds::cellOf(eyeGame.y);
    if (alphaLook && skyModels.cloud && worldProbe.column && worldProbe.zone) {
        // Alpha: a 13 x 13 grid, only while the player's column is moist and not on fire.
        const alpha::ZoneFactors player = worldProbe.zone(focus);
        if (clouds::alphaCloudsShown(player.moisture, player.fire))
            for (int i = -clouds::ALPHA_REACH; i <= clouds::ALPHA_REACH; i++)
                for (int j = -clouds::ALPHA_REACH; j <= clouds::ALPHA_REACH; j++)
                    if (const auto &c = cloudOfCell(centreX + i, centreY + j, true))
                        cloudDraws.push_back({skyModels.cloud, cloudMatrix(*c), 1});
    } else if (!alphaLook && !menuSky && skyModels.cloud && worldProbe.column) {
        // Steam: cells out past the view distance, faded near the camera.
        const float V = settings.fogDistance;
        const int reach = clouds::steamReach(V);
        for (int i = -reach; i <= reach; i++)
            for (int j = -reach; j <= reach; j++) {
                const auto &c = cloudOfCell(centreX + i, centreY + j, false);
                if (!c)
                    continue;
                const glm::vec3 base = glm::vec3(c->base - worldProbe.origin);
                const float distance = glm::length(base - camera.eye);
                if (distance > V + clouds::STEAM_CULL_BEYOND || !viewFrustum.sphere(base, clouds::STEAM_CULL_RADIUS))
                    continue;
                const float opacity = clouds::steamOpacity(distance);
                if (opacity < clouds::STEAM_HIDDEN_BELOW)
                    continue;
                const uint32_t mesh = c->dark && skyModels.darkCloud ? skyModels.darkCloud : skyModels.cloud;
                cloudDraws.push_back({mesh, cloudMatrix(*c), opacity});
            }
    }
    // One sky model with its own look constants (see common.glsl's Push).
    auto drawSkyModel = [&](uint32_t mesh, const glm::mat4 &model, glm::vec4 look) {
        const GpuMesh &m = meshes[mesh];
        if (!m.indexCount)
            return;
        VkDeviceSize zero = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &m.buffer.buffer, &zero);
        vkCmdBindIndexBuffer(cmd, m.buffer.buffer, m.indexOffset, VK_INDEX_TYPE_UINT32);
        PushConstants pc{model, glm::vec4(0), look, glm::uvec4(0)};
        vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);
        vkCmdDrawIndexed(cmd, m.indexCount, 1, 0, 0, 0);
        draws++;
    };
    auto modelInView = [&](uint32_t mesh, const glm::mat4 &model) {
        const GpuMesh &m = meshes[mesh];
        const glm::vec3 centre = glm::vec3(model * glm::vec4((m.min + m.max) * .5f, 1));
        return viewFrustum.sphere(centre, glm::length(m.max - m.min) * .5f * glm::length(glm::vec3(model[0])));
    };

    // --- Main pass (off-screen: the scene image and this frame's depth) ---
    Image &depth = depthImages[parity];
    VkImageMemoryBarrier2 toAttachment[2] = {
        imageBarrier(sceneColor.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, 0,
                     VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT),
        imageBarrier(depth.image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                     VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                     VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                     VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                     VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT),
    };
    barriers(cmd, toAttachment, 2);
    VkRenderingAttachmentInfo color{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    color.imageView = sceneColor.view;
    color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // The sky covers every pixel.
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkRenderingAttachmentInfo depthAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    depthAttachment.imageView = depth.view;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // TAA reprojects with it (now and next frame).
    depthAttachment.clearValue.depthStencil = {1, 0};
    VkRenderingInfo ri{VK_STRUCTURE_TYPE_RENDERING_INFO};
    ri.renderArea = {{0, 0}, extent};
    ri.layerCount = 1;
    ri.colorAttachmentCount = 1;
    ri.pColorAttachments = &color;
    ri.pDepthAttachment = &depthAttachment;
    vkCmdBeginRendering(cmd, &ri);
    VkViewport vp{0, 0, (float)extent.width, (float)extent.height, 0, 1};
    VkRect2D sc{{0, 0}, extent};
    vkCmdSetViewport(cmd, 0, 1, &vp);
    vkCmdSetScissor(cmd, 0, 1, &sc);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &fr.set, 0, nullptr);
    Frustum frustum(u.viewProj);
    auto visible = [&](const DrawItem &item) {
        if (!meshes[item.mesh].indexCount)
            return false;
        glm::vec3 c;
        float r;
        sphereOf(item, c, r);
        if (r < 2 && glm::length(c - camera.eye) > settings.sceneryDistance)
            return false;
        return frustum.sphere(c, r);
    };
    const VkPipeline worldPipeline = alphaLook ? alphaScenePipeline : scenePipeline;
    const VkPipeline mirroredPipeline = alphaLook ? alphaMirroredPipeline : mirroredScenePipeline;
    auto drawScene = [&](bool afterWater) {
        VkPipeline boundScene = worldPipeline;
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, boundScene);
        for (auto &item : opaque) {
            // Alpha's creatures follow the depth-writing water (water-rendering.md A.6).
            const bool late = alphaLook && item.kind == DrawKind::Model;
            if (late != afterWater || (item.flags & (NATIVE_MENU | WORLD_MAP)) || !visible(item))
                continue;
            const VkPipeline desired = item.mirrored() ? mirroredPipeline : worldPipeline;
            if (desired != boundScene) {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, desired);
                boundScene = desired;
            }
            bindAndDraw(item, 0);
        }
    };
    drawScene(false);
    // Alpha's clouds, opaque and lit as §4's cloud row (pc.look.w = 1 marks them).
    if (alphaLook && !cloudDraws.empty()) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, alphaCloudPipeline);
        for (const CloudDraw &c : cloudDraws)
            if (modelInView(c.mesh, c.model))
                drawSkyModel(c.mesh, c.model, glm::vec4(1, 0, 0, 1));
    }
    // The sky fills what the world left empty (it is drawn at the far plane).
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, alphaLook ? alphaSkyPipeline : skyPipeline);
    vkCmdDraw(cmd, 3, 1, 0, 0);
    if (alphaLook && starBuffer.buffer) {
        // The stars at night, then the sun sprite (docs/specs/alpha-rendering.md 8.3, 8.4).
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, alphaSpritePipeline);
        VkDeviceSize zero = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &starBuffer.buffer, &zero);
        // mode: 0 stars, 1 the sun, 2 the moon; `visibility` fades the moon.
        auto sprites = [&](float mode, uint32_t vertices, uint32_t instances, float visibility = 1) {
            PushConstants pc{glm::mat4(1), glm::vec4(mode, visibility, 0, 0), glm::vec4(0), glm::uvec4(0)};
            vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);
            vkCmdDraw(cmd, vertices, instances, 0, 0);
        };
        if (alpha::starsVisible(S, underwater))
            sprites(0, 12, starCount);
        sprites(1, 6, 1);
        // CubeVulkan's moon, over the stars (src/AlphaMoon.h).
        float moonVisible = settings.moon.enabled && !underwater ? alpha::moonDiscVisibility(dayMs) : 0.f;
        if (moonVisible > 0)
            sprites(2, 6, 1, moonVisible);
    }
    // Steam water writes nearest depth without colour before the blended pass (spec B.2).
    if (!alphaLook) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, waterDepthPipeline);
        for (auto &item : water)
            if (visible(item)) bindAndDraw(item, 0);
    }
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, alphaLook ? alphaWaterPipeline : waterPipeline);
    for (auto &item : water)
        if (visible(item))
            bindAndDraw(item, 0);
    if (alphaLook)
        drawScene(true);
    // Steam's clouds after the world (§B.7): the opaque ones, then the faded ones laid down at no
    // opacity (only their depth counts) and drawn again over that with their opacity, so each
    // shows only its nearest surface. The fog scale is CubeVulkan's choice (an open point of
    // the spec): a cloud is all sky by the time it is culled.
    if (!alphaLook && !cloudDraws.empty()) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, steamCloudPipeline);
        const float fogScale = 1 / (settings.fogDistance + clouds::STEAM_CULL_BEYOND);
        for (const CloudDraw &c : cloudDraws)
            if (c.opacity >= 1)
                drawSkyModel(c.mesh, c.model, glm::vec4(1, fogScale, 0, 0));
        for (const CloudDraw &c : cloudDraws)
            if (c.opacity < 1)
                drawSkyModel(c.mesh, c.model, glm::vec4(0, fogScale, 0, 0));
        for (const CloudDraw &c : cloudDraws)
            if (c.opacity < 1)
                drawSkyModel(c.mesh, c.model, glm::vec4(c.opacity, fogScale, 0, 0));
    }
    // Steam's stars last (§B.6): cube models on a dome round the camera, just inside the far
    // plane, fading in as the night deepens. (Whether the game's dome follows the camera, and
    // its radius unit R, are open points; CubeVulkan centres it and fits it to the far plane.)
    if (!alphaLook && !menuSky && skyModels.starCube && steamsky::starsShown(steamStarValue)) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, steamStarPipeline);
        const int radiusUnits = (int)(farPlane / STAR_DOME_UNIT);
        const glm::mat4 centre = glm::translate(glm::mat4(1), camera.eye);
        const float opacity = steamsky::starOpacity(steamStarValue);
        for (const steamsky::Star &star : steamStars) {
            const glm::mat4 model = centre * steamsky::starMatrix(star, radiusUnits);
            if (modelInView(skyModels.starCube, model))
                drawSkyModel(skyModels.starCube, model, glm::vec4(opacity, 0, 0, 0));
        }
    }
    // The original title and avatar use their own captured projection. Draw
    // them after the world, with a separate depth clear, as the native menu
    // does, so an unrelated terrain depth cannot hide the title foreground.
    bool menuDepthCleared = false;
    // This frame's jitter as a clip-space shift (x and y by jitter pixels, times w).
    glm::mat4 jitterClip(1);
    jitterClip[3][0] = 2 * jitter.x / extent.width, jitterClip[3][1] = 2 * jitter.y / extent.height;
    for (auto &item : opaque)
        if ((item.flags & NATIVE_MENU) && meshes[item.mesh].indexCount) {
            if (!menuDepthCleared) {
                VkClearAttachment clear{};
                clear.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                clear.clearValue.depthStencil = {1, 0};
                VkClearRect rect{{{0, 0}, extent}, 0, 1};
                vkCmdClearAttachments(cmd, 1, &clear, 1, &rect);
                menuDepthCleared = true;
            }
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              item.flags & MENU_BACKGROUND ? menuBackgroundPipeline : menuPipeline);
            // Their clip transform is captured whole, so they take this frame's TAA jitter here,
            // as the world's projection does: the resolve samples them back unshifted.
            DrawItem jittered = item;
            jittered.model = jitterClip * item.model;
            bindAndDraw(jittered, 0);
        }
    // The world map, over everything of the world (RendererMap.cpp).
    draws += drawMap(cmd, opaque, mapFog, glm::vec2(jitterClip[3][0], jitterClip[3][1]));
    vkCmdEndRendering(cmd);
    if (timing)
        vkCmdWriteTimestamp2(cmd, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, timing, 3 * frameIndex + 1);
    // TAA, SMAA and the interface, ending in the swapchain image.
    recordPost(cmd, imageIndex, jitter, clipToPrevious, true);
    if (timing) {
        vkCmdWriteTimestamp2(cmd, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, timing, 3 * frameIndex + 2);
        timingWritten[frameIndex] = true;
    }
    previousClip = u.viewProj;
    previousJitter = jitter;
    previousEye = camera.eye;
    previousTaa = settings.taa;
    historyValid = true;
    parity = 1 - parity;
    Buffer readback;
    bool capturing = !capturePath.empty() && canCapture;
    if (capturing) {
        readback = createBuffer((VkDeviceSize)extent.width * extent.height * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        auto toCopy = imageBarrier(swapImages[imageIndex], VK_IMAGE_ASPECT_COLOR_BIT,
                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                   VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COPY_BIT,
                                   VK_ACCESS_2_TRANSFER_READ_BIT);
        barriers(cmd, &toCopy, 1);
        VkBufferImageCopy region{};
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageExtent = {extent.width, extent.height, 1};
        vkCmdCopyImageToBuffer(cmd, swapImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               readback.buffer, 1, &region);
    }
    auto toPresent = imageBarrier(swapImages[imageIndex], VK_IMAGE_ASPECT_COLOR_BIT,
                                  capturing ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                                            : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                  capturing ? VK_PIPELINE_STAGE_2_COPY_BIT
                                            : VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                  capturing ? VK_ACCESS_2_NONE : VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                                  VK_PIPELINE_STAGE_2_NONE, 0);
    barriers(cmd, &toPresent, 1);
    vkEndCommandBuffer(cmd);
    lastDraws = draws;

    VkSemaphoreSubmitInfo wait{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    wait.semaphore = fr.acquired;
    wait.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSemaphoreSubmitInfo signal{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signal.semaphore = renderDone[imageIndex];
    signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    VkCommandBufferSubmitInfo cbi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    cbi.commandBuffer = cmd;
    VkSubmitInfo2 submit{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit.waitSemaphoreInfoCount = 1;
    submit.pWaitSemaphoreInfos = &wait;
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &cbi;
    submit.signalSemaphoreInfoCount = 1;
    submit.pSignalSemaphoreInfos = &signal;
    vkQueueSubmit2(queue, 1, &submit, fr.fence);
    if (capturing) {
        vkWaitForFences(device, 1, &fr.fence, VK_TRUE, UINT64_MAX);
        auto *px = (uint8_t *)readback.mapped;
        bool bgra = swapFormat == VK_FORMAT_B8G8R8A8_UNORM || swapFormat == VK_FORMAT_B8G8R8A8_SRGB;
        for (size_t i = 0; i < (size_t)extent.width * extent.height; i++) {
            if (bgra)
                std::swap(px[i * 4], px[i * 4 + 2]);
            px[i * 4 + 3] = 255;
        }
        stbi_write_png(capturePath.c_str(), extent.width, extent.height, 4, px, extent.width * 4);
        destroyBuffer(readback);
        capturePath.clear();
    }

    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &renderDone[imageIndex];
    present.swapchainCount = 1;
    present.pSwapchains = &swapchain;
    present.pImageIndices = &imageIndex;
    VkResult result = vkQueuePresentKHR(queue, &present);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        vkDeviceWaitIdle(device);
        destroySwapchain();
        createSwapchain();
    }
    frameIndex = (frameIndex + 1) % FRAMES;
}

void Renderer::shutdown() {
    if (!device)
        return;
    vkDeviceWaitIdle(device);
    for (auto &p : pending)
        destroyBuffer(p.staging);
    pending.clear();
    for (uint32_t i = 1; i < meshes.size(); i++)
        destroyMesh(i);
    for (auto &fr : frames) {
        for (auto &b : fr.staging)
            destroyBuffer(b);
        destroyBuffer(fr.uniforms);
        vkDestroyFence(device, fr.fence, nullptr);
        vkDestroySemaphore(device, fr.acquired, nullptr);
    }
    for (auto p : {scenePipeline, mirroredScenePipeline, menuPipeline, menuBackgroundPipeline, waterPipeline, waterDepthPipeline, shadowPipeline, skyPipeline,
                   alphaScenePipeline, alphaMirroredPipeline, alphaWaterPipeline, alphaSkyPipeline, alphaSpritePipeline,
                   alphaCloudPipeline, steamCloudPipeline, steamStarPipeline})
        vkDestroyPipeline(device, p, nullptr);
    destroyBuffer(starBuffer);
    destroyMapPipeline();
    vkDestroyPipelineLayout(device, layout, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, setLayout, nullptr);
    vkDestroySampler(device, shadowSampler, nullptr);
    destroyImage(shadowFar);
    destroyImage(shadowNear);
    destroyImage(overlayImage);
    for (auto &b : overlayBuffers)
        destroyBuffer(b);
    if (overlayPipeline)
        vkDestroyPipeline(device, overlayPipeline, nullptr);
    vkDestroyPipelineLayout(device, overlayLayout, nullptr);
    vkDestroyDescriptorPool(device, overlayPool, nullptr);
    vkDestroyDescriptorSetLayout(device, overlaySetLayout, nullptr);
    vkDestroySampler(device, overlaySampler, nullptr);
    destroySwapchain();
    destroyPostResources();
    if (timing)
        vkDestroyQueryPool(device, timing, nullptr);
    vkDestroyCommandPool(device, pool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    device = VK_NULL_HANDLE;
}
