#include "vk_camera.h"

void vk_camera::init()
{
    pos = glm::vec4(0.f, 0.f, 128.f, 1.f);
    right = glm::vec4(1.f, 0.f, 0.f, 0.f);
    dir = glm::vec4(0.f, 0.f, -1.f, 0.f);
};
