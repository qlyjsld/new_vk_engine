#pragma once

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class vk_camera
{
public:
    void w(float ms) { move(dir, ms); };
    void a(float ms) { move(glm::cross(up, dir), ms); };
    void s(float ms) { move(-dir, ms); };
    void d(float ms) { move(-glm::cross(up, dir), ms); };
    void space(float ms) { move(up, ms); };
    void ctrl(float ms) { move(-up, ms); };

    void motion(float x, float y)
    {
        yaw += x * sensitivity / 10.f;
        pitch -= y * sensitivity / 10.f;

        if (pitch >= 89.f)
            pitch = 89.f;

        if (pitch <= -89.f)
            pitch = -89.f;

        dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        dir.y = sin(glm::radians(pitch));
        dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    }

    inline glm::mat4 get_view_mat() { return glm::lookAt(pos, pos + dir, up); }

    inline glm::mat4 get_proj_mat()
    {
        return glm::perspective(glm::radians(fov), aspect, .01f, 100.f);
    }

    inline glm::vec3 get_pos() { return pos; };
    inline glm::vec3 get_dir() { return dir; };
    inline glm::vec3 get_up() { return up; };
    inline float get_fov() { return fov; };

private:
    glm::vec3 pos{glm::vec3(0.f, 0.f, 0.f)};
    glm::vec3 dir{glm::vec3(0.f, 0.f, -1.f)};
    glm::vec3 up{glm::vec3(0.f, 1.f, 0.f)};

    float fov{68.f};
    float speed{3.f};
    float sensitivity{.3f};
    float yaw{0.f};
    float pitch{0.f};
    float aspect{4.f / 3.f};

    void move(glm::vec3 dir, float time) { pos += speed * dir * time; }
};
