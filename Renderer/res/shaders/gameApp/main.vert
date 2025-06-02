#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} u_camera;

layout(push_constant) uniform pushConstant {
    mat4 model;
} ps;

layout(location = 0) out vec3 fragmentPos;
layout(location = 1) out vec3 vertexNormal;
layout(location = 2) out vec2 v_texCoord;

void main()
{
    fragmentPos = vec3(ps.model * vec4(position, 1.f));
    vertexNormal = normalize(mat3(transpose(inverse(ps.model))) * normal);
    v_texCoord = texCoord;
    gl_Position = u_camera.proj * u_camera.view * vec4(fragmentPos, 1.f);
}
