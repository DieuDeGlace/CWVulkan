#ifndef CUBEVULKAN_STEAM_TRANSFORM_GLSL
#define CUBEVULKAN_STEAM_TRANSFORM_GLSL
// Independently expressed GPU matrix composition (map WORLD contract section6.3).
// Keep each component's Y product before the X/Z/homogeneous accumulations.
mat4 steamCompose(mat4 outer, mat4 inner) {
    mat4 result;
    for (int column = 0; column < 4; ++column) {
        vec4 value = outer[1] * inner[column].y;
        value = outer[0] * inner[column].x + value;
        value = outer[2] * inner[column].z + value;
        result[column] = outer[3] * inner[column].w + value;
    }
    return result;
}
#endif
