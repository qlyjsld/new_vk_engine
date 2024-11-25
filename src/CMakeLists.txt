# add source file executable
file(GLOB SOURCE_FILES
	"*.cpp"
	"*.h")

add_executable(vk_engine ${SOURCE_FILES})

target_link_libraries(vk_engine volk SDL3::SDL3 vk-bootstrap GPUOpen::VulkanMemoryAllocator tinygltf imgui)

include_directories(
	"${PROJECT_SOURCE_DIR}/vendor/imgui"
	"${PROJECT_SOURCE_DIR}/vendor/imgui/backends"
	"${PROJECT_SOURCE_DIR}/vendor/volk"
	"${PROJECT_SOURCE_DIR}/vendor/glm")
