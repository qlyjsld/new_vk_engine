# add source file executable
add_executable(vk_engine
    main.cpp
    vk_engine.cpp
    vk_engine.h
    vk_type.h
    vk_init.cpp
    vk_init.h
    vk_cmd.cpp
    vk_cmd.h)

target_link_libraries(vk_engine Vulkan::Vulkan SDL3::SDL3 vk-bootstrap)
