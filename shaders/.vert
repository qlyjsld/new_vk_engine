#version 460

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

layout (location = 0) out vec3 outColor;

void main()
{
    const vec3 positions[3] = vec3[3](
        vec3(-.5f, .5f, 0.f),
        vec3(.5f, .5f, 0.f),
        vec3(0.f, -.5f, 0.f)
    );

    const vec3 colors[3] = vec3[3](
        vec3(1.f, 0.f, 0.f),
        vec3(0.f, 1.f, 0.f),
        vec3(0.f, 0.f, 1.f)
    );

    gl_Position = vec4(vPos, 1.f);
    outColor = vColor;
}