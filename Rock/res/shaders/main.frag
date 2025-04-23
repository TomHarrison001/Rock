#version 460

layout(location = 0) out vec4 f_colour;

layout(location = 0) in vec3 v_colour;

void main() {
    vec2 texCoord = gl_PointCoord - vec2(0.5f);
    f_colour = vec4(v_colour, 0.5f - length(texCoord));
}
