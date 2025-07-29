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

/*
    Each of the functions below initialize a compute shader
    for running, follwing the structure:

        comp_allocator allocator(_device, _allocator);

        allocator.create_img(..., img_name);
        allocator.create_buffer(..., buffer_name);
        allocator.load_img(img_name, ...);

    comp_allocator has static class member for storing buffers and images
    in unordered_map with their name as key, it is common to share resources
    within multiple shaders. By default, _target, "target" is the framebuffer
    we draw to.

        std::vector<descriptor> descriptors = {
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, img_name},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, buffer_name},
        };

        cs compute_shader_example(allocator,
                                descriptors,
                                "../shaders/example.spv",
                                _min_buffer_alignment);

    Descriptors binding is done with vector of "descriptor" which is std::pair
   of <VkDescriptorType, std::string>, provided the name and arguments, create
   class cs. Then start building pipeline using info from struct cs and
   comp_allocator.

        PipelineBuilder pb = {};
        pb._shader_stage_infos.push_back(vk_boiler::shader_stage_create_info(
            VK_SHADER_STAGE_COMPUTE_BIT, compute_shader_example.module));

        pb.build_comp(...);

    Finally, add draw commands. At this point, you have mutiple options, you
   have to run cc_init(...) the first time. After that, you could push_back(...)
   to have it executed in the main loop, or call comp_immediate_submit(...) to
   execute immediately, the latter one is ofter used for preparing texture or
   data used later.

        compute_shader_example.draw = [=](VkCommandBuffer cmd_buffer, cs *cs) {
            vkCmdBindPipeline(...);
            vkCmdBindDescriptorSets(...);
            vkCmdDispatch(...);
        };

        cs::cc_init(_comp_index, _device);
        cs::push_back(compute_shader_example);
        cs::comp_immediate_submit(_device, _queue,
   &compute_shader_example);

*/

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

    // cloudtex_init();
    // weather_init();
    // cloud_init();
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
