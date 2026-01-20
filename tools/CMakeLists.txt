cmake_minimum_required(VERSION 3.10)
project(ImGuiDemo)

# 添加ImGui核心源码
file(GLOB IMGUI_SOURCES "imgui/*.cpp")

# 添加DirectX 11后端文件
file(GLOB IMGUI_BACKENDS 
    "imgui/backends/imgui_impl_win32.cpp"
    "imgui/backends/imgui_impl_dx11.cpp"
)

# 创建可执行文件
add_executable(ImGuiDemo main.cpp ${IMGUI_SOURCES} ${IMGUI_BACKENDS})

# 设置包含目录
target_include_directories(ImGuiDemo PRIVATE 
    imgui
    imgui/backends
)

# 链接DirectX 11库
target_link_libraries(ImGuiDemo 
    d3d11 
    dxgi
)

# 设置C++标准
set_target_properties(ImGuiDemo PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
