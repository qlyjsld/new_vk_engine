#include "vk_engine.h"

#include <cstring>

#include <imgui.h>

#include "vk_boiler.h"
#include "vk_cmd.h"
#include "vk_comp.h"
#include "vk_pipeline.h"
#include "vk_type.h"

struct camera_data {
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 dir;
    alignas(16) glm::vec3 up;
    alignas(4) float fov;
};

struct cloud_data {
    alignas(16) glm::vec3 extent;
    alignas(16) glm::vec3 centre;
    alignas(16) glm::vec3 size;
    alignas(4) float sigma_a;
    alignas(4) float sigma_s;
    alignas(4) float step;
    alignas(4) float cutoff;
    alignas(4) float density;
};

static uint32_t texture_size = 512;
static bool cloud = true;
static cloud_data cloud_data;

int main(int argc, char *argv[])
{
    vk_engine engine = {};
    engine.init();
    engine.run();
    engine.cleanup();
    return 0;
}

void vk_engine::skybox_init()
{
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

    std::vector<descriptor> descriptors = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "target"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "extent"},
    };

    cs skybox(allocator, descriptors, "../shaders/skybox.comp.spv",
              _min_buffer_alignment);

    PipelineBuilder pb = {};
    pb._shader_stage_infos.push_back(
        vk_boiler::shader_stage_create_info(VK_SHADER_STAGE_COMPUTE_BIT, skybox.module));

    std::vector<VkDescriptorSetLayout> layouts = {
        skybox.layout,
    };

    std::vector<VkPushConstantRange> push_constants = {};

    pb.build_comp(_device, layouts, push_constants, &skybox.pipeline_layout,
                  &skybox.pipeline);

    skybox.draw = [=](VkCommandBuffer cbuffer, cs *cs) {
        vkCmdBindPipeline(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cs->pipeline);

        uint32_t doffset = 0;
        vkCmdBindDescriptorSets(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                cs->pipeline_layout, 0, 1, &cs->set, 1, &doffset);

        vkCmdDispatch(cbuffer, _resolution.width / 8, _resolution.height / 8, 1);
    };

    css.push_back(skybox);
}

void vk_engine::texture_init()
{
    /* to init a cs you need an allocator (custom struct) */
    comp_allocator allocator(_device, _allocator);

    allocator.create_img(VK_FORMAT_R16G16B16A16_SFLOAT,
                         VkExtent3D{texture_size, texture_size, texture_size},
                         VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_STORAGE_BIT, 0,
                         "cloudtex");

    /* match set binding */
    std::vector<descriptor> descriptors = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "cloudtex"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "extent"},
    };

    cs cloudtex(allocator, descriptors, "../shaders/cloudtex.comp.spv",
                _min_buffer_alignment);

    /* start building pipeline using info from struct cs and comp_allocator */
    PipelineBuilder pb = {};
    pb._shader_stage_infos.push_back(vk_boiler::shader_stage_create_info(
        VK_SHADER_STAGE_COMPUTE_BIT, cloudtex.module));

    VkPushConstantRange u_time = {};
    u_time.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    u_time.offset = 0;
    u_time.size = sizeof(float);

    std::vector<VkDescriptorSetLayout> layouts = {
        cloudtex.layout,
    };

    std::vector<VkPushConstantRange> push_constants = {
        u_time,
    };

    pb.build_comp(_device, layouts, push_constants, &cloudtex.pipeline_layout,
                  &cloudtex.pipeline);

    cloudtex.draw = [=](VkCommandBuffer cbuffer, cs *cs) {
        vk_cmd::vk_img_layout_transition(cbuffer, cs->allocator.get_img("cloudtex").img,
                                         VK_IMAGE_LAYOUT_UNDEFINED,
                                         VK_IMAGE_LAYOUT_GENERAL, _comp_index);

        vkCmdBindPipeline(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cs->pipeline);

        uint32_t doffset = 0;
        vkCmdBindDescriptorSets(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                cs->pipeline_layout, 0, 1, &cs->set, 1, &doffset);

        vkCmdDispatch(cbuffer, texture_size / 8, texture_size / 8, texture_size / 8);
    };

    cs::cc_init(_comp_index, _device);
    cs::comp_immediate_submit(_device, _comp_queue, &cloudtex);
}

void vk_engine::sphere_init()
{
    comp_allocator allocator(_device, _allocator);

    allocator.create_buffer(pad_uniform_buffer_size(sizeof(camera_data)),
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT, "camera");

    std::vector<descriptor> descriptors = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "target"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "extent"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "camera"},
    };

    cs sphere(allocator, descriptors, "../shaders/sphere.comp.spv",
              _min_buffer_alignment);

    PipelineBuilder pb = {};
    pb._shader_stage_infos.push_back(
        vk_boiler::shader_stage_create_info(VK_SHADER_STAGE_COMPUTE_BIT, sphere.module));

    std::vector<VkDescriptorSetLayout> layouts = {
        sphere.layout,
    };

    std::vector<VkPushConstantRange> push_constants = {};

    pb.build_comp(_device, layouts, push_constants, &sphere.pipeline_layout,
                  &sphere.pipeline);

    sphere.draw = [=](VkCommandBuffer cbuffer, cs *cs) {
        vkCmdBindPipeline(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cs->pipeline);

        camera_data camera_data;
        camera_data.pos = _vk_camera.get_pos();
        camera_data.dir = _vk_camera.get_dir();
        camera_data.up = _vk_camera.get_up();
        camera_data.fov = _vk_camera.get_fov();

        void *data;
        vmaMapMemory(_allocator, cs->allocator.get_buffer("camera").allocation, &data);
        std::memcpy(data, &camera_data, pad_uniform_buffer_size(sizeof(camera_data)));
        vmaUnmapMemory(cs->allocator.allocator,
                       cs->allocator.get_buffer("camera").allocation);

        std::vector<uint32_t> doffsets = {
            0,
            0,
        };

        vkCmdBindDescriptorSets(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                cs->pipeline_layout, 0, 1, &cs->set, doffsets.size(),
                                doffsets.data());

        vkCmdDispatch(cbuffer, _resolution.width / 8, _resolution.height / 8, 1);
    };

    css.push_back(sphere);
}

void vk_engine::cloud_init()
{
    comp_allocator allocator(_device, _allocator);

    allocator.create_buffer(pad_uniform_buffer_size(sizeof(cloud_data)),
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT, "cloud");

    cloud_data.extent = glm::vec3(texture_size);
    cloud_data.centre = glm::vec3(0.f);
    cloud_data.size = glm::vec3(32.f);
    cloud_data.sigma_a = .1f;
    cloud_data.sigma_s = 16.f;
    cloud_data.step = .1f;
    cloud_data.cutoff = .3f;
    cloud_data.density = 1.f;

    std::vector<descriptor> descriptors = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "target"},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "cloudtex"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "extent"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "camera"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "cloud"},
    };

    cs cloud(allocator, descriptors, "../shaders/cloud.comp.spv", _min_buffer_alignment);

    PipelineBuilder pb = {};
    pb._shader_stage_infos.push_back(
        vk_boiler::shader_stage_create_info(VK_SHADER_STAGE_COMPUTE_BIT, cloud.module));

    std::vector<VkDescriptorSetLayout> layouts = {
        cloud.layout,
    };

    std::vector<VkPushConstantRange> push_constants = {};

    pb.build_comp(_device, layouts, push_constants, &cloud.pipeline_layout,
                  &cloud.pipeline);

    cloud.draw = [=](VkCommandBuffer cbuffer, cs *cs) {
        vkCmdBindPipeline(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cs->pipeline);

        camera_data camera_data;
        camera_data.pos = _vk_camera.get_pos();
        camera_data.dir = _vk_camera.get_dir();
        camera_data.up = _vk_camera.get_up();
        camera_data.fov = _vk_camera.get_fov();

        void *data;
        vmaMapMemory(_allocator, cs->allocator.get_buffer("camera").allocation, &data);
        std::memcpy(data, &camera_data, pad_uniform_buffer_size(sizeof(camera_data)));
        vmaUnmapMemory(cs->allocator.allocator,
                       cs->allocator.get_buffer("camera").allocation);

        vmaMapMemory(_allocator, cs->allocator.get_buffer("cloud").allocation, &data);
        std::memcpy(data, &cloud_data, pad_uniform_buffer_size(sizeof(cloud_data)));
        vmaUnmapMemory(cs->allocator.allocator,
                       cs->allocator.get_buffer("cloud").allocation);

        std::vector<uint32_t> doffsets = {
            0,
            0,
            0,
        };

        vkCmdBindDescriptorSets(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                cs->pipeline_layout, 0, 1, &cs->set, doffsets.size(),
                                doffsets.data());

        vkCmdDispatch(cbuffer, _resolution.width / 8, _resolution.height / 8, 1);
    };

    css.push_back(cloud);
}

void vk_engine::draw_comp(frame *frame)
{
    ImGui::SetNextWindowSize(ImVec2{300, 200});
    ImGui::SetNextWindowPos(ImVec2{30, 30});
    ImGui::Begin("cloud", &cloud, 0);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::SliderFloat("sigma_a", &cloud_data.sigma_a, 0.f, 100.f);
    ImGui::SliderFloat("sigma_s", &cloud_data.sigma_s, 0.f, 100.f);
    ImGui::SliderFloat("step", &cloud_data.step, .01f, 3.f);
    ImGui::SliderFloat("cutoff", &cloud_data.cutoff, 0.f, 3.f);
    ImGui::SliderFloat("density", &cloud_data.density, 0.f, 3.f);
    ImGui::End();

    vk_cmd::vk_img_layout_transition(frame->cbuffer, _target.img,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                     VK_IMAGE_LAYOUT_GENERAL, _comp_index);

    for (cs &cs : css)
        cs.draw(frame->cbuffer, &cs);

    vk_cmd::vk_img_layout_transition(frame->cbuffer, _target.img, VK_IMAGE_LAYOUT_GENERAL,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                     _comp_index);
}
