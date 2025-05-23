#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 colour;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec3 v_colour;
layout(location = 1) out vec2 v_texCoord;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    v_colour = colour;
    v_texCoord = texCoord;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(position, 1.f);
}
