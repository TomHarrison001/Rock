#version 460

layout(location = 0) out vec4 f_colour;

layout(location = 0) in vec3 v_colour;
layout(location = 1) in vec2 v_texCoord;

layout(binding = 1) uniform sampler2D u_texture;

void main() {
    f_colour = texture(u_texture, v_texCoord);
}
