// SMAA (Jimenez et al., MIT; third_party/smaa) in GLSL for Vulkan. Vulkan's
// texture origin is the top left as in Direct3D, so the official area and
// search textures are used as they are, without flips. Ultra preset, as the
// user's CWSMTAA uses.
#define SMAA_GLSL_4
#define SMAA_PRESET_ULTRA
layout(push_constant) uniform SmaaMetrics { vec4 rt; } smaaMetrics; // 1/w, 1/h, w, h
#define SMAA_RT_METRICS smaaMetrics.rt
#include "../third_party/smaa/SMAA.hlsl"
