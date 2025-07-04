#version 460

layout (location = 0) in per_vertex_data
{
    vec2 texcrood;
} v_in;

layout (location = 0) out vec4 frag_color;

layout (set = 3, binding = 0) uniform sampler2D tex;

void main()
{
    vec3 color = texture(tex, v_in.texcrood).xyz;
    frag_color = vec4(color, 1.f);
}