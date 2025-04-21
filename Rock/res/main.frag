#version 460

layout(location = 0) out vec4 f_colour;

layout(location = 0) in vec3 v_colour;

void main() {
    f_colour = vec4(v_colour, 1.f);
}
