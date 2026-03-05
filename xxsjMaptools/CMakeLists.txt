cmake_minimum_required(VERSION 3.10)
project(KarstResConverter)

# 添加ImGui核心源码
file(GLOB IMGUI_SOURCES "third/imgui/*.cpp")

# 添加DirectX 11后端文件
file(GLOB IMGUI_BACKENDS 
    "third/imgui/backends/*.cpp"
)

file(GLOB KARSTCONVERTER_SOURCES "ConvertKarstData.cpp" "WindowTool.cpp" "ProcLineData.cpp" "ProcMaskData.cpp" "CreateRoadMesh.cpp" "ProcCityData.cpp" "ProcPCGData.cpp")

# 创建可执行文件
add_executable(KarstResConverter main.cpp  ${KARSTCONVERTER_SOURCES} ${IMGUI_SOURCES} ${IMGUI_BACKENDS})
source_group("imgui" FILES ${IMGUI_SOURCES} ${IMGUI_BACKENDS})

# 设置包含目录
target_include_directories(KarstResConverter PRIVATE 
    third/imgui
    third/imgui/backends
	third
)

target_link_directories(KarstResConverter PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/$<CONFIG>
)

target_link_libraries(KarstResConverter PRIVATE
    $<$<CONFIG:Debug>: FreeImaged>
    $<$<CONFIG:Release>: FreeImage>
	d3d11 
    dxgi
)

# 设置C++标准
set_target_properties(KarstResConverter PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
	RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)

add_custom_command(TARGET KarstResConverter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/libs/$<CONFIG>
        $<TARGET_FILE_DIR:KarstResConverter>/
)