#version 460

layout (location = 0) in per_vertex_data
{
    vec4 color;
} frag_in;

layout (location = 0) out vec4 frag_color;

void main()
{
    frag_color = frag_in.color;
}