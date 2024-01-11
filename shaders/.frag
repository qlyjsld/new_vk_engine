#version 460

layout (location = 0) in vec2 texcrood;

layout (location = 0) out vec4 out_color;

layout (set = 1, binding = 0) uniform sampler2D tex1;

void main()
{
    vec3 color = texture(tex1, texcrood).xyz;
    out_color = vec4(color, 1.f);
}