#pragma once

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

class vk_camera
{
public:
    glm::vec3 pos;
    glm::vec3 right;
    glm::vec3 dir;
    glm::vec3 up;

    float fov;
    float speed;
    float sensitivity;
    float yaw;

    void init();
    void move(glm::vec3 velocity, float time) { pos += velocity * time; }
    void rotate_yaw(float angle)
    {
        yaw += angle;
        dir = glm::vec3(cos(yaw), 0.f, -sin(yaw));
        right = -glm::cross(up, dir);
    };
};
