#pragma once

#include <glm/vec4.hpp>

class vk_camera
{
public:
    glm::vec4 pos;
    glm::vec4 right;
    glm::vec4 dir;

    float fov;

    void init();
};
