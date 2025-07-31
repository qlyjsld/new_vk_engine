#include "vk_engine.h"

#include <cstring>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "vk_cmd.h"
#include "vk_comp.h"
#include "vk_type.h"

int main(int argc, char *argv[])
{
    vk_engine engine = {};
    engine.init();
    engine.run();
    engine.cleanup();
    return 0;
}

void vk_engine::comp_init()
{
    _comp_allocator.device = _device;
    _comp_allocator.vma_allocator = _allocator;
    _comp_allocator.init();

    _comp_allocator.create_buffer(pad_uniform_buffer_size(sizeof(camera_data)),
                                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                  VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
                                  "camera");

    _comp_allocator.load_img("target", _target);

    // cloudtex_shader_init();
    // weather_shader_init();
    // cloud_shader_init();
}

void vk_engine::draw_comp(frame *frame)
{
    vk_cmd::vk_img_layout_transition(frame->cmd_buffer, _target.img,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                     VK_IMAGE_LAYOUT_GENERAL, _family_index);

    for (const auto &draw : cs_draw)
        draw(frame->cmd_buffer);

    vk_cmd::vk_img_layout_transition(
        frame->cmd_buffer, _target.img, VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, _family_index);
}
