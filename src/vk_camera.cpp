#include "vk_camera.h"

void vk_camera::init()
{
    pos = glm::vec3(0.f, 0.f, 3.f);
    right = glm::vec3(1.f, 0.f, 0.f);
    dir = glm::vec3(0.f, 0.f, -1.f);
    up = glm::vec3(0.f, 1.f, 0.f);

    fov = 68.f;
    speed = .3f;
    sensitivity = .4f;
    yaw = 90.f;
};
