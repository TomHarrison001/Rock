#version 460

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 colour;

layout(location = 0) out vec3 v_colour;

void main() {
    gl_Position = vec4(position, 0.f, 1.f);
    v_colour = colour;
}
