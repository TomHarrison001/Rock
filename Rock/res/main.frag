#version 460

layout(location = 0) out vec4 colour;

layout(location = 0) in vec3 fragColour;

void main() {
    colour = vec4(fragColour, 1.f);
}
