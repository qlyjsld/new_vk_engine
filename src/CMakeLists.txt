# add source file executable

file(GLOB SOURCE_FILES
	"*.cpp"
	"*.h")

add_executable(vk_engine ${SOURCE_FILES})

target_link_libraries(vk_engine Vulkan::Vulkan SDL3::SDL3 vk-bootstrap GPUOpen::VulkanMemoryAllocator tinygltf)
