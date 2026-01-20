#define _CRT_SECURE_NO_WARNINGS
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <wincodec.h>
#include <cstdio>
#include <string>
#include <sstream>  
#include"WindowTool.h"
// Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// 背景图数据
static ID3D11ShaderResourceView* g_pBackgroundTexture = nullptr;
static int g_BackgroundWidth = 0;
static int g_BackgroundHeight = 0;

//缩放数据
static float g_Zoom = 1.0f;
static ImVec2 g_Pan = ImVec2(0, 0);


static const int GRID_WIDTH = 129;  // 2048/16 = 128, use 129 
static const int GRID_HEIGHT = 85;  // 1381/16.24 ≈ 85
static bool g_GridSelection[GRID_WIDTH * GRID_HEIGHT] = { false };


static std::string g_SelectedCoordsInfo = ""; // 选中数据  


bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);
void HandleInputs();
ImVec2 GetImageSize();
ImVec2 GetImagePos();
void ApplyPanBoundaries();
void RenderGrid();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


void RenderButton()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("ButtonOverlay", nullptr);

    // Gen button -生成坐标信息
    if (ImGui::Button("Gen", ImVec2(120, 40))) {
        std::ostringstream oss;
        oss << "Selected Coordinates:\n";

        int count = 0;
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                if (g_GridSelection[y * GRID_WIDTH + x]) {
                    oss << "Grid[" << x << "," << y << "]\n";
                    count++;
                }
            }
        }

        if (count == 0) {
            oss << "No tiles selected";
        }
        else {
            oss << "Total: " << count << " tiles";
        }

        g_SelectedCoordsInfo = oss.str();
    }

   

    // Reset button - 重置选中以及清楚信息
    if (ImGui::Button("Reset", ImVec2(120, 40))) {
        
        for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; i++) {
            g_GridSelection[i] = false;
        }
       
        g_SelectedCoordsInfo = "";
    }



   

 

    if (ImGui::Button("Save", ImVec2(120, 40)))
    {

    }
    if (ImGui::Button("Load", ImVec2(120, 40)))
    {

    }

    //显示
    if (!g_SelectedCoordsInfo.empty()) {
        ImGui::Separator();
        ImGui::TextWrapped("%s", g_SelectedCoordsInfo.c_str());
    }

    ImGui::End();
}

void RenderTileSelectView()
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Background", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);

    if (g_pBackgroundTexture) {
        ImVec2 image_size = GetImageSize();
        ImVec2 image_pos = GetImagePos();

        // 渲染图片
        ImGui::GetWindowDrawList()->AddImage(
            (ImTextureID)g_pBackgroundTexture,
            image_pos,
            ImVec2(image_pos.x + image_size.x, image_pos.y + image_size.y)
        );

        
        RenderGrid();
    }

    ImGui::End();
}

// 计算图片位置 -转化
ImVec2 GetImageSize()
{
    ImGuiIO& io = ImGui::GetIO();
    // scale 1:1
    return ImVec2((float)g_BackgroundWidth * g_Zoom, (float)g_BackgroundHeight * g_Zoom);
}

ImVec2 GetImagePos()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 image_size = GetImageSize();
    return ImVec2(
        (io.DisplaySize.x - image_size.x) * 0.5f + g_Pan.x,
        (io.DisplaySize.y - image_size.y) * 0.5f + g_Pan.y
    );
}

//限制图片被拖出窗口外，让图片完整覆盖整个窗口
void ApplyPanBoundaries()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 image_size = GetImageSize();

    
    float max_pan_x = (image_size.x - io.DisplaySize.x) * 0.5f;
    float max_pan_y = (image_size.y - io.DisplaySize.y) * 0.5f;

    g_Pan.x = (g_Pan.x > max_pan_x) ? max_pan_x : (g_Pan.x < -max_pan_x) ? -max_pan_x : g_Pan.x;
    g_Pan.y = (g_Pan.y > max_pan_y) ? max_pan_y : (g_Pan.y < -max_pan_y) ? -max_pan_y : g_Pan.y;
}

// 网格线， 网格选择
void RenderGrid()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 image_size = GetImageSize();
    ImVec2 image_pos = GetImagePos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float cell_width = image_size.x / GRID_WIDTH;
    float cell_height = image_size.y / GRID_HEIGHT;

    // 选择
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.WantCaptureMouse) {
        ImVec2 mouse_rel = ImVec2(io.MousePos.x - image_pos.x, io.MousePos.y - image_pos.y);
        if (mouse_rel.x >= 0 && mouse_rel.x <= image_size.x && mouse_rel.y >= 0 && mouse_rel.y <= image_size.y) {
            int grid_x = (int)(mouse_rel.x / cell_width);
            int grid_y = (int)(mouse_rel.y / cell_height);
            if (grid_x >= 0 && grid_x < GRID_WIDTH && grid_y >= 0 && grid_y < GRID_HEIGHT) {
                g_GridSelection[grid_y * GRID_WIDTH + grid_x] = !g_GridSelection[grid_y * GRID_WIDTH + grid_x];
            }
        }
    }

    // 画线
    ImU32 grid_color = IM_COL32(200, 200, 200, 80); 

   
    for (int x = 0; x <= GRID_WIDTH; x++) {
        float line_x = image_pos.x + x * cell_width;
        draw_list->AddLine(
            ImVec2(line_x, image_pos.y),
            ImVec2(line_x, image_pos.y + image_size.y),
            grid_color,
            1.0f
        );
    }

    
    for (int y = 0; y <= GRID_HEIGHT; y++) {
        float line_y = image_pos.y + y * cell_height;
        draw_list->AddLine(
            ImVec2(image_pos.x, line_y),
            ImVec2(image_pos.x + image_size.x, line_y),
            grid_color,
            1.0f
        );
    }

    // 
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (g_GridSelection[y * GRID_WIDTH + x]) {
                ImVec2 cell_pos = ImVec2(image_pos.x + x * cell_width, image_pos.y + y * cell_height);
                draw_list->AddRectFilled(cell_pos,
                    ImVec2(cell_pos.x + cell_width, cell_pos.y + cell_height),
                    IM_COL32(255, 255, 0, 120));
            }
        }
    }
}

void HandleInputs()
{
    ImGuiIO& io = ImGui::GetIO();
    static bool is_dragging = false;
    static ImVec2 drag_start_pos, drag_start_pan;

    if (!io.WantCaptureMouse) {
        // Mouse wheel zoom
        if (io.MouseWheel != 0.0f) {
            float old_zoom = g_Zoom;
            g_Zoom += io.MouseWheel * 0.1f;
            g_Zoom = (g_Zoom < 1.0f) ? 1.0f : (g_Zoom > 4.0f) ? 4.0f : g_Zoom;

            // Zoom towards mouse position
            float zoom_ratio = g_Zoom / old_zoom;

            // Calculate mouse offset from image center and adjust pan
            ImVec2 image_center = ImVec2(io.DisplaySize.x * 0.5f + g_Pan.x, io.DisplaySize.y * 0.5f + g_Pan.y);
            ImVec2 mouse_offset = ImVec2(io.MousePos.x - image_center.x, io.MousePos.y - image_center.y);
            g_Pan.x += mouse_offset.x * (1.0f - zoom_ratio);
            g_Pan.y += mouse_offset.y * (1.0f - zoom_ratio);

            ApplyPanBoundaries();
        }

        // Right mouse button for panning/dragging
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            is_dragging = true;
            drag_start_pos = io.MousePos;
            drag_start_pan = g_Pan;
        }

        if (is_dragging && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            ImVec2 drag_delta = ImVec2(io.MousePos.x - drag_start_pos.x, io.MousePos.y - drag_start_pos.y);
            g_Pan.x = drag_start_pan.x + drag_delta.x;
            g_Pan.y = drag_start_pan.y + drag_delta.y;
            ApplyPanBoundaries();
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
            is_dragging = false;
        }

        // Left mouse button is handled by grid selection in RenderGrid()
    }
    else {
        is_dragging = false;
    }
}

// Main code
int main(int, char**)
{
    // Create application window - match image resolution
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX11 Example", WS_OVERLAPPEDWINDOW, 100, 100, 2048, 1381, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load background image
    const char* paths[] = { "worldmap.bmp", "../worldmap.bmp", "../../worldmap.bmp" };
    for (auto path : paths) {
        if (LoadTextureFromFile(path, &g_pBackgroundTexture, &g_BackgroundWidth, &g_BackgroundHeight)) break;
    }

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Handle input for zoom and pan
        HandleInputs();

        // Render background first, then buttons
        RenderTileSelectView();
        RenderButton();

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    if (g_pBackgroundTexture) { g_pBackgroundTexture->Release(); g_pBackgroundTexture = nullptr; }
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
    FILE* file = fopen(filename, "rb");
    if (!file) return false;

    unsigned char header[54];
    if (fread(header, 1, 54, file) != 54) { fclose(file); return false; }

    int width = *(int*)&header[18], height = *(int*)&header[22];
    if (*(short*)&header[28] != 24) { fclose(file); return false; }

    int row_size = ((width * 3 + 3) / 4) * 4;
    unsigned char* bmp_data = new unsigned char[row_size * abs(height)];
    unsigned char* rgba_data = new unsigned char[width * abs(height) * 4];

    fseek(file, *(int*)&header[10], SEEK_SET);
    fread(bmp_data, 1, row_size * abs(height), file);
    fclose(file);

    // BGR to RGBA conversion
    for (int y = 0; y < abs(height); y++) {
        for (int x = 0; x < width; x++) {
            int src_y = (height > 0) ? (abs(height) - 1 - y) : y;
            int src = src_y * row_size + x * 3, dst = (y * width + x) * 4;
            rgba_data[dst] = bmp_data[src + 2];     // R
            rgba_data[dst + 1] = bmp_data[src + 1]; // G
            rgba_data[dst + 2] = bmp_data[src];     // B
            rgba_data[dst + 3] = 255;               // A
        }
    }
    delete[] bmp_data;

    // Create D3D texture
    D3D11_TEXTURE2D_DESC desc = { (UINT)width, (UINT)abs(height), 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, {1, 0}, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE };
    D3D11_SUBRESOURCE_DATA sub = { rgba_data, (UINT)(width * 4), 0 };

    ID3D11Texture2D* tex = nullptr;
    g_pd3dDevice->CreateTexture2D(&desc, &sub, &tex);

    D3D11_SHADER_RESOURCE_VIEW_DESC srv = { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_SRV_DIMENSION_TEXTURE2D, {0, 1} };
    g_pd3dDevice->CreateShaderResourceView(tex, &srv, out_srv);
    tex->Release();

    *out_width = width; *out_height = abs(height);
    delete[] rgba_data;
    return true;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
