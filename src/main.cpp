#include "vk_engine.h"

#include <cstring>
#include <utility>

#include "vk_boiler.h"
#include "vk_cmd.h"
#include "vk_comp.h"
#include "vk_pipeline.h"
#include "vk_type.h"

static std::vector<cs> css;

int main(int argc, char *argv[])
{
    vk_engine engine = {};
    engine.init();
    engine.skybox_init();
    engine.texture_draw_init();
    engine.run();
    engine.cleanup();
    return 0;
}

void vk_engine::skybox_init()
{
    comp_allocator allocator(_device, _allocator);
    allocator.load_img("target", _target);

    std::vector<std::pair<VkDescriptorType, std::string>> descriptors = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "target"},
    };

    cs skybox(allocator, descriptors, "../shaders/skybox.comp.spv",
              _minUniformBufferOffsetAlignment, _device);

    PipelineBuilder pb = {};
    pb._shader_stage_infos.push_back(
        vk_boiler::shader_stage_create_info(VK_SHADER_STAGE_COMPUTE_BIT, skybox.module));

    std::vector<VkDescriptorSetLayout> layouts = {
        skybox.layout,
    };

    std::vector<VkPushConstantRange> push_constants = {};

    pb.build_layout(_device, layouts, push_constants, &skybox.pipeline_layout);

    pb.build_comp(_device, &skybox.pipeline_layout, &skybox.pipeline);

    skybox.draw = [=](VkCommandBuffer cmd_buffer, cs *cs) {
        vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, cs->pipeline);

        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                cs->pipeline_layout, 0, 1, &cs->set, 0, nullptr);

        vkCmdDispatch(cmd_buffer, _resolution.width / 8, _resolution.height / 8, 1);
    };

    css.push_back(skybox);
}

void vk_engine::texture_draw_init()
{
    /* to init a cs you need an allocator (custom struct) */
    comp_allocator allocator(_device, _allocator);

    allocator.create_buffer(pad_uniform_buffer_size(sizeof(glm::vec2)),
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT, "extent");

    glm::vec2 extent = glm::vec2{
        _resolution.width,
        _resolution.height,
    };

    allocated_buffer buffer = allocator.get_buffer("extent");

    void *data;
    vmaMapMemory(_allocator, buffer.allocation, &data);
    std::memcpy(data, &extent, sizeof(glm::vec2));
    vmaUnmapMemory(_allocator, buffer.allocation);

    allocator.load_img("target", _target);

    /* match set binding */
    std::vector<std::pair<VkDescriptorType, std::string>> descriptors = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "target"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "extent"},
    };

    cs perlinworley(allocator, descriptors, "../shaders/perlinworley.comp.spv",
                    _minUniformBufferOffsetAlignment, _device);

    /* start building pipeline using info from struct cs and comp_allocator */
    PipelineBuilder pb = {};
    pb._shader_stage_infos.push_back(vk_boiler::shader_stage_create_info(
        VK_SHADER_STAGE_COMPUTE_BIT, perlinworley.module));

    VkPushConstantRange u_time_push_constant = {};
    u_time_push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    u_time_push_constant.offset = 0;
    u_time_push_constant.size = sizeof(float);

    std::vector<VkDescriptorSetLayout> layouts = {
        perlinworley.layout,
    };

    std::vector<VkPushConstantRange> push_constants = {
        u_time_push_constant,
    };

    pb.build_layout(_device, layouts, push_constants, &perlinworley.pipeline_layout);

    pb.build_comp(_device, &perlinworley.pipeline_layout, &perlinworley.pipeline);

    perlinworley.draw = [=](VkCommandBuffer cmd_buffer, cs *cs) {
        vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, cs->pipeline);

        uint32_t doffset = 0;
        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                cs->pipeline_layout, 0, 1, &cs->set, 1, &doffset);

        float u_time = _last_frame / 1000000000.f;
        vkCmdPushConstants(cmd_buffer, cs->pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT,
                           0, sizeof(float), &u_time);

        vkCmdDispatch(cmd_buffer, _resolution.width / 8, _resolution.height / 8, 1);
    };

    // css.push_back(perlinworley);
}

void vk_engine::draw_comp(frame *frame)
{

    vk_cmd::vk_img_layout_transition(frame->cmd_buffer, _target.img,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                     VK_IMAGE_LAYOUT_GENERAL, _comp_queue_family_index);

    for (cs &cs : css)
        cs.draw(frame->cmd_buffer, &cs);

    vk_cmd::vk_img_layout_transition(
        frame->cmd_buffer, _target.img, VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, _comp_queue_family_index);
}
