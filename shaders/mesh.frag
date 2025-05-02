#version 460

layout (location = 0) in per_vertex_data
{
    vec2 texcrood;
} v_in;

layout (location = 0) out vec4 frag_color;

layout (set = 1, binding = 0) uniform sampler2D tex;

void main()
{
    vec3 color = texture(tex, texcrood).xyz;
    frag_color = v_in.color;
}