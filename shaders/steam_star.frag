#version 450
// The Steam look's stars (docs/specs/sky-and-clouds.md §B.6): small cube models on a dome round
// the camera, in their own colour, fading in as the night deepens (pc.look.x). Used with
// scene.vert; they are drawn after the sky and behind the world.
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
layout(location = 2) in vec4 vColor;
layout(location = 0) out vec4 outColor;

void main() { outColor = vec4(vanillaGrade(vColor.rgb), pc.look.x); }
