#include "vk_engine.h"

#include <cstring>
#include <iostream>

#include "vk_boiler.h"
#include "vk_cmd.h"
#include "vk_pipeline.h"
#include "vk_type.h"

static VkDescriptorSet pcomp_set;
static VkDescriptorSetLayout pcomp_set_layout;
static VkShaderModule pcomp;
static VkPipelineLayout pcomp_layout;
static VkPipeline pcomp_pipeline;
static allocated_buffer extent_buffer;

int main(int argc, char *argv[])
{
    vk_engine engine = {};
    engine.init();
    engine.comp_draw_init();
    engine.run();
    engine.cleanup();
    return 0;
}

void vk_engine::comp_draw_init()
{
    extent_buffer = create_buffer(pad_uniform_buffer_size(sizeof(glm::vec2)),
                                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                  VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

    _deletion_queue.push_back([=]() {
        vmaDestroyBuffer(_allocator, extent_buffer.buffer, extent_buffer.allocation);
    });

    glm::vec2 extent = glm::vec2{_resolution.width, _resolution.height};

    void *data;
    vmaMapMemory(_allocator, extent_buffer.allocation, &data);
    std::memcpy(data, &extent, sizeof(glm::vec2));
    vmaUnmapMemory(_allocator, extent_buffer.allocation);

    VkDescriptorSetLayoutBinding pcomp_binding_0 = {};
    pcomp_binding_0.binding = 0;
    pcomp_binding_0.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    pcomp_binding_0.descriptorCount = 1;
    pcomp_binding_0.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding pcomp_binding_1 = {};
    pcomp_binding_1.binding = 1;
    pcomp_binding_1.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    pcomp_binding_1.descriptorCount = 1;
    pcomp_binding_1.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    std::vector<VkDescriptorSetLayoutBinding> pcomp_layout_bindings = {pcomp_binding_0,
                                                                       pcomp_binding_1};

    VkDescriptorSetLayoutCreateInfo pcomp_layout_info =
        vk_boiler::descriptor_set_layout_create_info(pcomp_layout_bindings.size(),
                                                     pcomp_layout_bindings.data());

    VK_CHECK(vkCreateDescriptorSetLayout(_device, &pcomp_layout_info, nullptr,
                                         &pcomp_set_layout));

    _deletion_queue.push_back(
        [=]() { vkDestroyDescriptorSetLayout(_device, pcomp_set_layout, nullptr); });

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info =
        vk_boiler::descriptor_set_allocate_info(_descriptor_pool, &pcomp_set_layout);

    VK_CHECK(
        vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info, &pcomp_set));

    VkDescriptorImageInfo descriptor_img_info = {};
    descriptor_img_info.imageView = _target.img_view;
    descriptor_img_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet write_set = vk_boiler::write_descriptor_set(
        &descriptor_img_info, pcomp_set, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    vkUpdateDescriptorSets(_device, 1, &write_set, 0, nullptr);

    VkDescriptorBufferInfo descriptor_buffer_info = {};
    descriptor_buffer_info.buffer = extent_buffer.buffer;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = pad_uniform_buffer_size(sizeof(glm::vec2));

    write_set = vk_boiler::write_descriptor_set(
        &descriptor_buffer_info, pcomp_set, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

    vkUpdateDescriptorSets(_device, 1, &write_set, 0, nullptr);

    load_shader_module("../shaders/perlin.comp.spv", &pcomp);

    PipelineBuilder pcomp_pipeline_builder = {};
    pcomp_pipeline_builder._shader_stage_infos.push_back(
        vk_boiler::shader_stage_create_info(VK_SHADER_STAGE_COMPUTE_BIT, pcomp));

    VkPushConstantRange u_time_push_constant = {};
    u_time_push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    u_time_push_constant.offset = 0;
    u_time_push_constant.size = sizeof(float);

    std::vector<VkDescriptorSetLayout> layouts = {pcomp_set_layout};
    std::vector<VkPushConstantRange> push_constants = {u_time_push_constant};
    pcomp_pipeline_builder.build_layout(_device, layouts, push_constants);
    pcomp_pipeline_builder.build_comp(_device);
    pcomp_pipeline = pcomp_pipeline_builder.value();
    pcomp_layout = pcomp_pipeline_builder._pipeline_layout;

    _deletion_queue.push_back([=]() {
        vkDestroyPipelineLayout(_device, pcomp_layout, nullptr);
        vkDestroyPipeline(_device, pcomp_pipeline, nullptr);
    });
}

void vk_engine::draw_comp(frame *frame)
{
    vk_cmd::vk_img_layout_transition(frame->cmd_buffer, _target.img,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                     VK_IMAGE_LAYOUT_GENERAL, _comp_queue_family_index);

    vkCmdBindPipeline(frame->cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pcomp_pipeline);

    uint32_t doffset = 0;
    vkCmdBindDescriptorSets(frame->cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                            pcomp_layout, 0, 1, &pcomp_set, 1, &doffset);

    float u_time = _last_frame / 1000000000.f;
    vkCmdPushConstants(frame->cmd_buffer, pcomp_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(float), &u_time);

    vkCmdDispatch(frame->cmd_buffer, _resolution.width / 8, _resolution.height / 8, 1);

    vk_cmd::vk_img_layout_transition(
        frame->cmd_buffer, _target.img, VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, _comp_queue_family_index);
}
