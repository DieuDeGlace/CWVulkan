#version 450
layout(location = 0) in vec4 reflectance;
layout(location = 0) out vec4 colour;
void main() {
    colour = reflectance;
}
