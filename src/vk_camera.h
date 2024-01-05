#pragma once

#include <glm/vec3.hpp>

class vk_camera
{
public:
    glm::vec3 pos;
    glm::vec3 right;
    glm::vec3 dir;

    float fov;
    float speed;
    float sensitivity;

    void init();
    void move(glm::vec3 velocity, float time) { pos += velocity * time; }
};
