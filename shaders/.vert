#version 460

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

layout (location = 0) out vec3 outColor;

layout (push_constant) uniform constants
{
    vec4 data;
    mat4 render_matrix;
} PushConstants;

void main()
{
    gl_Position = PushConstants.render_matrix * vec4(vPos, 1.f);
    outColor = vColor;
}