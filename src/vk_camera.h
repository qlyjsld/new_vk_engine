#pragma once

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

class vk_camera
{
public:
    vk_camera()
        : pos(glm::vec3(0.f, 0.f, 3.f)), up(glm::vec3(0.f, 1.f, 0.f)), fov(68.f),
          speed(.003f), sensitivity(.003f), yaw(90.f)
    {
        dir = glm::vec3(cos(yaw), 0.f, -sin(yaw));
        right = -glm::cross(up, dir);
        init();
    }

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
    void rotate_yaw(float angle, float time)
    {
        yaw += angle * time;
        dir = glm::vec3(cos(yaw), 0.f, -sin(yaw));
        right = -glm::cross(up, dir);
    };
};
