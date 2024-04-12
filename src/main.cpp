#include "vk_engine.h"

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
    VkDescriptorSetLayoutBinding pcomp_binding_0 = {};
    pcomp_binding_0.binding = 0;
    pcomp_binding_0.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    pcomp_binding_0.descriptorCount = 1;
    pcomp_binding_0.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    std::vector<VkDescriptorSetLayoutBinding> pcomp_layout_bindings = {pcomp_binding_0};

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

    load_shader_module("../shaders/pcomp.comp.spv", &pcomp);

    PipelineBuilder pcomp_pipeline_builder = {};
    pcomp_pipeline_builder._shader_stage_infos.push_back(
        vk_boiler::shader_stage_create_info(VK_SHADER_STAGE_COMPUTE_BIT, pcomp));

    std::vector<VkDescriptorSetLayout> layouts = {pcomp_set_layout};
    pcomp_pipeline_builder.build_layout(_device, layouts);
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

    vkCmdBindDescriptorSets(frame->cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                            pcomp_layout, 0, 1, &pcomp_set, 0, nullptr);

    vkCmdDispatch(frame->cmd_buffer, 1600 / 8, 900 / 8, 1);

    vk_cmd::vk_img_layout_transition(
        frame->cmd_buffer, _target.img, VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, _comp_queue_family_index);
}
