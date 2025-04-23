#version 460

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 colour;

layout(location = 0) out vec3 v_colour;

void main() {
    v_colour = colour.rgb;
    gl_PointSize = 14.f;
    gl_Position = vec4(position, 1.f, 1.f);
}
