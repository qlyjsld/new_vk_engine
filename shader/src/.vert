#version 460

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec2 v_texcrood;

layout (location = 0) out vec2 out_texcrood;

layout (set = 0, binding = 0) uniform readonly RENDER_MAT
{
    mat4 view;
    mat4 proj;
    mat4 model;
} render_mat;

void main()
{
    gl_Position = render_mat.proj * render_mat.view * render_mat.model * vec4(v_pos, 1.f);
    out_texcrood = v_texcrood;
}