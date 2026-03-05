#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"
#include "WindowTool.h"
#include <d3d11.h>
#include <tchar.h>
#include <wincodec.h>
#include <cstdio>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>
#include <fstream>
#include <ctime>
#include <algorithm>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

// Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool g_SwapChainOccluded = false;
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
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
static std::vector<std::string> g_SavedFiles; // 保存的文件列表
static int g_SelectedFileIndex = -1; // 当前选中的文件索引
static const std::string SAVE_DIRECTORY = "saved_selections/"; // 保存目录
static char g_SaveFileName[256] = ""; // 自定义保存文件名

// 2的整次幂输入
static int g_PowerOfTwo = 7; // 默认2^10 = 1024

// 文件路径选择相关变量
static std::string g_HeightPath = "D:/svn-sync/Echo_SU_Develop/Tools/xxsjMaptools/height_maps/";
static std::string g_TexturePath = "F:/xxsj_echo/tiles/texture/";
inline std::string g_ResPath = "D:\\svn-sync\\EchoAsset\\xxsj\\asset\\client\\levels\\karstworld\\GISRes\\";

// PCG数据路径
static std::string g_PCGDataDir = "E:/Echo_SU_Develop/Tools/xxsjMaptools/chaina_shp";
static std::string g_PCGOutputDir = "E:/Echo_SU_Develop/Tools/xxsjMaptools/pcg_output";

// 文件对话框实例
static FileDialog g_FileDialog;

// 进度条相关变量
struct ProcessingProgress {
	std::atomic<bool> isProcessing{ false };
	std::atomic<bool> isComplete{ false };
	std::atomic<bool> shouldCancel{ false };
	std::atomic<float> progress{ 0.0f };           // 0.0 到 1.0
	std::atomic<int> currentStep{ 0 };
	std::atomic<int> totalSteps{ 0 };
	std::string currentTask = "";                // 需要mutex保护
	std::string resultMessage = "";              // 需要mutex保护
	std::atomic<bool> hasError{ false };
	std::chrono::steady_clock::time_point startTime;
	std::string estimatedTime = "";              // 需要mutex保护

	std::mutex stringMutex;                      // 保护字符串成员
	std::thread workerThread;                    // 工作线程
};

static ProcessingProgress g_Progress;
// 本次运行的输出目录与跟踪选项（用于进度统计）
static std::string g_SessionOutputDir;
static std::atomic<bool> g_SessionTrackHeight{ false };
static std::atomic<bool> g_SessionTrackTexture{ false };
static std::filesystem::file_time_type g_SessionStartFileTime; // 记录会话开始时间（用于过滤旧文件）

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
// 核心处理函数
void ConvertWorldHeightMapData(const std::string& filePath, const std::string& resPath, bool* selectedGrid, float KarstSize = 32768.0f, int row = GRID_WIDTH, int col = GRID_HEIGHT, int targetPixPerPage = 256);
void ConvertWorldTextureMapData(const std::string& texfilePath, const std::string& heightfilePath, const std::string& resPath, bool* selectedGrid, int row = GRID_WIDTH, int col = GRID_HEIGHT, int targetPixPerPage = 256);
void ProcRoadsLineData(std::string dataPath, std::string savePath, bool* selectedGrid, int rowSize, int colSize);
void CreateWaterData(std::string dataPath, std::string outputPath, int rowSize, int colSize, bool* selectedGrid);
void CreateWaterLineMesh(std::string dataPath, std::string outputPath, int rowSize, int colSize, bool* selectedGrid);
void ProcessMaskTiledTiff(const std::string& inputDir, const std::string& outputPath, bool* selectedGrid, int tileRows, int tileCols, const std::string& tilePattern = "tile_%d_%d.tif");
void ProcessAreaIDTiledTiff(const std::string& inputDir, const std::string& outputPath, bool* selectedGrid, int tileRows, int tileCols, const std::string& tilePattern = "tile_%d_%d.tif");
void CreateRoadData(std::string dataPath, std::string outputPath, int rowSize, int colSize, bool* selectedGrid);
void ConvertCityData(const std::filesystem::path& cityDir, const std::filesystem::path& cityIndexPath);
void GeneratePCGLineData(const std::string& inputDir, const std::string& outputDir);
// 进度条相关函数
void UpdateProgress(float progress, const std::string& task);
void StartProcessing(int totalSteps);
void CompleteProcessing(const std::string& message, bool hasError = false);
void RenderProgressBar();

// 带进度的处理函数
void ProcessWithProgress(bool genHeight, bool genTexture);

// 保存和加载功能
void SaveGridSelection(const std::string& filename);
bool LoadGridSelection(const std::string& filename);
void RefreshSavedFilesList();
std::string GenerateTimestampFilename();
void ClearSavedFiles();

// 新的文件路径选择功能
void RenderModernPathSelector();

// City HeightMap processing
void RenderCitySelector();

// 获取2的整次幂值的接口函数
int GetPowerOfTwoValue();

void RenderButton() {
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(450, 700), ImGuiCond_FirstUseEver);  // 增加窗口大小以容纳新功能
	ImGui::Begin("ButtonOverlay", nullptr);
	static bool genHeight, genTexture;
	ImGui::Checkbox("generate height map", &genHeight);
	ImGui::Checkbox("generate texture", &genTexture);

	// 文件路径选择
	ImGui::Separator();
	ImGui::Text("File Paths Configuration:");
	RenderModernPathSelector();

	ImGui::Separator();
	ImGui::Text("City Selector:");
	RenderCitySelector();

	// 2的整次幂输入
	ImGui::Separator();
	ImGui::Text("Power of 2 Setting:");
	ImGui::InputInt("2^n", &g_PowerOfTwo, 1, 1);

	// 限制范围在合理区间内
	if (g_PowerOfTwo < 5) g_PowerOfTwo = 5;
	if (g_PowerOfTwo > 10) g_PowerOfTwo = 10;

	int powerValue = 1 << g_PowerOfTwo; // 计算2^n
	ImGui::Text("Value: 2^%d = %d", g_PowerOfTwo, powerValue);

	// 进度条显示
	RenderProgressBar();

	// Gen button -生成坐标信息
	bool canProcess = !g_Progress.isProcessing;
	if (!canProcess) {
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
	}

	// Reset button - 重置选中以及清楚信息
	if (ImGui::Button("Reset", ImVec2(100, 30))) {
		for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; i++) {
			g_GridSelection[i] = false;
		}
		g_SelectedCoordsInfo = "";
	}

	if (ImGui::Button("Generate map", ImVec2(100, 30)) && canProcess) {
		std::ostringstream oss;
		oss << "Selected Coordinates:\n";

		int count = 0;
		for (int x = 0; x < GRID_WIDTH; x++) {
			for (int y = 0; y < GRID_HEIGHT; y++) {
				if (g_GridSelection[y * GRID_WIDTH + x]) {
					oss << "Grid[" << x << "," << y << "]\n";
					count++;
				}
			}
		}

		if (count == 0) {
			oss << "No tiles selected";
			g_SelectedCoordsInfo = oss.str();
		}
		else {
			oss << "Total: " << count << " tiles";
			int powerValue = GetPowerOfTwoValue();
			oss << "\nPower of 2: 2^" << g_PowerOfTwo << " = " << powerValue;
			g_SelectedCoordsInfo = oss.str();

			// 启动带进度的处理
			ProcessWithProgress(genHeight, genTexture);
		}
	}
	if (ImGui::Button("ProcRoadLine", ImVec2(100, 30))) {
		ProcRoadsLineData("D:/Echo_SU_Main/Tools/xxsjMaptools/china_shp/road_lines_with_height.bin", "G:/EchoAsset/xxsj/asset/client/levels/karstworld/road1", g_GridSelection, GRID_WIDTH, GRID_HEIGHT);
	}
	if (ImGui::Button("ProcRoadData", ImVec2(100, 30))) {
		CreateRoadData("D:/Echo_SU_Main/Tools/xxsjMaptools/china_shp/road_lines_with_height.bin", "G:/EchoAsset/xxsj/asset/client/levels/karstworld/road1", GRID_WIDTH, GRID_HEIGHT, g_GridSelection);
	}
	if (ImGui::Button("ProcWaterData", ImVec2(100, 30))) {
		CreateWaterData("D:/Echo_SU_Main/Tools/xxsjMaptools/china_shp/water_lines_with_height.bin", "G:/EchoAsset/xxsj/asset/client/levels/karstworld/waterway1", GRID_WIDTH, GRID_HEIGHT, g_GridSelection);
	}
	if (ImGui::Button("ProcWaterMesh", ImVec2(100, 30))) {
		CreateWaterLineMesh("D:/Echo_SU_Main/Tools/xxsjMaptools/china_shp/water_lines_with_height.bin", "G:/EchoAsset/xxsj/asset/client/levels/karstworld/waterway1", GRID_WIDTH, GRID_HEIGHT, g_GridSelection);
	}
	if (ImGui::Button("ProcMaskData", ImVec2(100, 30))) {
		ProcessMaskTiledTiff("F:/HeightField/global/tiles/mask/", "G:/EchoAsset/xxsj/asset/client/levels/karstworld/GISRes", g_GridSelection, GRID_WIDTH, GRID_HEIGHT, "Cover_aligned1_%03d_%03d.tif");
	}
	if (ImGui::Button("ProcAreaIDData", ImVec2(100, 30))) {
		ProcessAreaIDTiledTiff("F:/HeightField/global/tiles/area/", "G:/EchoAsset/xxsj/asset/client/levels/karstworld/GISRes/area.mask", g_GridSelection, GRID_WIDTH, GRID_HEIGHT, "rasterize_district_%03d_%03d.tif");
	}
	if (ImGui::Button("GenPCGLineData", ImVec2(120, 30))) {
		GeneratePCGLineData(g_PCGDataDir, g_PCGOutputDir);
	}
	if (ImGui::Button("ProcAllData", ImVec2(100, 30))) {
		ProcRoadsLineData("D:/Echo_SU_Main/Tools/xxsjMaptools/china_shp/road_lines.bin", "G:/EchoAsset/xxsj/asset/client/levels/karstworld/road1", g_GridSelection, GRID_WIDTH, GRID_HEIGHT);
		CreateWaterData("D:/Echo_SU_Main/Tools/xxsjMaptools/china_shp/water_lines_with_height.bin", "G:/EchoAsset/xxsj/asset/client/levels/karstworld/waterway1", GRID_WIDTH, GRID_HEIGHT, g_GridSelection);
		CreateWaterLineMesh("D:/Echo_SU_Main/Tools/xxsjMaptools/china_shp/water_lines_with_height.bin", "G:/EchoAsset/xxsj/asset/client/levels/karstworld/waterway1", GRID_WIDTH, GRID_HEIGHT, g_GridSelection);
		ProcessMaskTiledTiff("F:/HeightField/global/tiles/mask/", "G:/EchoAsset/xxsj/asset/client/levels/karstworld/GISRes/surface.mask", g_GridSelection, GRID_WIDTH, GRID_HEIGHT, "Cover_aligned1_%03d_%03d.tif");
		ProcessWithProgress(true, true);
	}


	if (!canProcess) {
		ImGui::PopStyleVar();
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Processing...");
	}

	ImGui::Separator();
	ImGui::Text("Save/Load Selection:");

	// 文件名输入和保存
	ImGui::InputTextWithHint("##SaveFileName", "Enter filename (auto-generated if empty)", g_SaveFileName, sizeof(g_SaveFileName));
	ImGui::SameLine();
	if (ImGui::Button("Save", ImVec2(60, 0))) {
		std::string filename = strlen(g_SaveFileName) > 0 ?
			std::string(g_SaveFileName) + ".sel" : GenerateTimestampFilename();
		SaveGridSelection(filename);
		RefreshSavedFilesList();
		g_SaveFileName[0] = '\0';  // 清空输入框
	}

	// 加载下拉框和按钮
	const char* preview = g_SelectedFileIndex >= 0 ? g_SavedFiles[g_SelectedFileIndex].c_str() : "Select file...";
	ImGui::SetNextItemWidth(-80);  // 留出Load按钮的空间
	if (ImGui::BeginCombo("##LoadCombo", preview)) {
		for (size_t i = 0; i < g_SavedFiles.size(); i++) {
			if (ImGui::Selectable(g_SavedFiles[i].c_str(), g_SelectedFileIndex == (int)i)) {
				g_SelectedFileIndex = (int)i;
			}
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	if (ImGui::Button("Load", ImVec2(60, 0)) && g_SelectedFileIndex >= 0) {
		const std::string& filename = g_SavedFiles[g_SelectedFileIndex];
		g_SelectedCoordsInfo = LoadGridSelection(filename) ?
			"Loaded: " + filename : "Failed to load: " + filename;
	}

	// 清除保存的文件
	if (ImGui::Button("Clear All Saved", ImVec2(120, 0))) {
		ClearSavedFiles();
		RefreshSavedFilesList();
		g_SelectedFileIndex = -1;
		g_SelectedCoordsInfo = "Cleared all saved files";
	}

	//显示
	if (!g_SelectedCoordsInfo.empty()) {
		ImGui::Separator();
		ImGui::TextWrapped("%s", g_SelectedCoordsInfo.c_str());
	}

	ImGui::End();
}

void RenderTileSelectView() {
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
ImVec2 GetImageSize() {
	ImGuiIO& io = ImGui::GetIO();
	// scale 1:1
	return ImVec2((float)g_BackgroundWidth * g_Zoom, (float)g_BackgroundHeight * g_Zoom);
}

ImVec2 GetImagePos() {
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 image_size = GetImageSize();
	return ImVec2(
		(io.DisplaySize.x - image_size.x) * 0.5f + g_Pan.x,
		(io.DisplaySize.y - image_size.y) * 0.5f + g_Pan.y
	);
}

//限制图片被拖出窗口外，让图片完整覆盖整个窗口
void ApplyPanBoundaries() {
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 image_size = GetImageSize();

	float max_pan_x = (image_size.x - io.DisplaySize.x) * 0.5f;
	float max_pan_y = (image_size.y - io.DisplaySize.y) * 0.5f;

	g_Pan.x = (g_Pan.x > max_pan_x) ? max_pan_x : (g_Pan.x < -max_pan_x) ? -max_pan_x : g_Pan.x;
	g_Pan.y = (g_Pan.y > max_pan_y) ? max_pan_y : (g_Pan.y < -max_pan_y) ? -max_pan_y : g_Pan.y;
}

// 网格线， 网格选择
void RenderGrid() {
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

	// 绘制选中的格子
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

void HandleInputs() {
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
int main(int, char**) {
	// Create application window - match image resolution
	//ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
	::RegisterClassExW(&wc);
	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX11 Example", WS_OVERLAPPEDWINDOW, 100, 100, 2048, 1381, nullptr, nullptr, wc.hInstance, nullptr);

	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd)) {
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
	const char* paths[] = { "worldmap.bmp", "../worldmap.bmp", "../../worldmap.bmp", "../../../worldmap.bmp" };
	for (auto path : paths) {
		if (LoadTextureFromFile(path, &g_pBackgroundTexture, &g_BackgroundWidth, &g_BackgroundHeight)) break;
	}

	// 初始化保存文件列表
	RefreshSavedFilesList();

	// 设置文件对话框的父窗口
	g_FileDialog.SetParentWindow(hwnd);

	// Our state
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// Main loop
	bool done = false;
	while (!done) {
		// Poll and handle messages (inputs, window resize, etc.)
		// See the WndProc() function below for our to dispatch events to the Win32 backend.
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;

		// Handle window being minimized or screen locked
		if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
			::Sleep(10);
			continue;
		}
		g_SwapChainOccluded = false;

		// Handle window resize (we don't resize directly in the WM_SIZE handler)
		if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
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

bool CreateDeviceD3D(HWND hWnd) {
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

void CleanupDeviceD3D() {
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
	pBackBuffer->Release();
}

void CleanupRenderTarget() {
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height) {
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
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg) {
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

// 保存和加载功能实现
void SaveGridSelection(const std::string& filename) {
	std::filesystem::create_directories(SAVE_DIRECTORY);
	std::ofstream file(SAVE_DIRECTORY + filename, std::ios::binary);

	if (!file.is_open()) {
		g_SelectedCoordsInfo = "Failed to save: " + filename;
		return;
	}

	// 写入网格尺寸和选中状态
	file.write(reinterpret_cast<const char*>(&GRID_WIDTH), sizeof(GRID_WIDTH));
	file.write(reinterpret_cast<const char*>(&GRID_HEIGHT), sizeof(GRID_HEIGHT));
	file.write(reinterpret_cast<const char*>(g_GridSelection), sizeof(g_GridSelection));

	auto fullPath = std::filesystem::absolute(SAVE_DIRECTORY + filename);
	g_SelectedCoordsInfo = "Saved to: " + fullPath.string();
}

bool LoadGridSelection(const std::string& filename) {
	std::ifstream file(SAVE_DIRECTORY + filename, std::ios::binary);
	if (!file.is_open()) return false;

	// 验证网格尺寸
	int savedWidth, savedHeight;
	file.read(reinterpret_cast<char*>(&savedWidth), sizeof(savedWidth));
	file.read(reinterpret_cast<char*>(&savedHeight), sizeof(savedHeight));

	if (savedWidth != GRID_WIDTH || savedHeight != GRID_HEIGHT) {
		g_SelectedCoordsInfo = "File format mismatch: " + filename;
		return false;
	}

	file.read(reinterpret_cast<char*>(g_GridSelection), sizeof(g_GridSelection));
	return true;
}

void RefreshSavedFilesList() {
	g_SavedFiles.clear();
	if (!std::filesystem::exists(SAVE_DIRECTORY)) return;

	for (const auto& entry : std::filesystem::directory_iterator(SAVE_DIRECTORY)) {
		if (entry.is_regular_file() && entry.path().extension() == ".sel") {
			g_SavedFiles.push_back(entry.path().filename().string());
		}
	}
	std::sort(g_SavedFiles.begin(), g_SavedFiles.end());
}

std::string GenerateTimestampFilename() {
	auto now = std::time(nullptr);
	auto tm = *std::localtime(&now);
	char buffer[64];
	std::strftime(buffer, sizeof(buffer), "selection_%Y%m%d_%H%M%S.sel", &tm);
	return std::string(buffer);
}

void ClearSavedFiles() {
	if (!std::filesystem::exists(SAVE_DIRECTORY)) return;

	for (const auto& entry : std::filesystem::directory_iterator(SAVE_DIRECTORY)) {
		if (entry.is_regular_file() && entry.path().extension() == ".sel") {
			std::filesystem::remove(entry.path());
		}
	}
}

// 获取2的整次幂值的接口函数
int GetPowerOfTwoValue() {
	return 1 << g_PowerOfTwo; // 返回2^g_PowerOfTwo
}

// 现代文件路径选择器实现
void RenderModernPathSelector() {
	ImGui::Text("Current Paths:");

	// Height Path
	ImGui::Text("Height Path:");
	ImGui::SameLine();
	if (ImGui::Button("Browse##HeightPath", ImVec2(80, 0))) {
		FileDialog::DialogInfo info;
		info.initialDir = g_HeightPath;
		info.title = "Select Height Map Directory";

		std::string selectedPath;
		if (g_FileDialog.ShowFolderDialog(info, selectedPath)) {
			// 确保路径以斜杠结尾
			g_HeightPath = selectedPath;
			if (g_HeightPath.back() != '/' && g_HeightPath.back() != '\\') {
				g_HeightPath += "/";
			}
		}
	}
	ImGui::TextWrapped("  %s", g_HeightPath.c_str());

	// Texture Path
	ImGui::Text("Texture Path:");
	ImGui::SameLine();
	if (ImGui::Button("Browse##TexturePath", ImVec2(80, 0))) {
		FileDialog::DialogInfo info;
		info.initialDir = g_TexturePath;
		info.title = "Select Texture Map Directory";

		std::string selectedPath;
		if (g_FileDialog.ShowFolderDialog(info, selectedPath)) {
			g_TexturePath = selectedPath;
			if (g_TexturePath.back() != '/' && g_TexturePath.back() != '\\') {
				g_TexturePath += "/";
			}
		}
	}
	ImGui::TextWrapped("  %s", g_TexturePath.c_str());

	// Result Path
	ImGui::Text("Result Path:");
	ImGui::SameLine();
	if (ImGui::Button("Browse##ResultPath", ImVec2(80, 0))) {
		FileDialog::DialogInfo info;
		info.initialDir = g_ResPath;
		info.title = "Select Result Directory";

		std::string selectedPath;
		if (g_FileDialog.ShowFolderDialog(info, selectedPath)) {
			g_ResPath = selectedPath;
			if (g_ResPath.back() != '/' && g_ResPath.back() != '\\') {
				g_ResPath += "/";
			}
		}
	}
	ImGui::TextWrapped("  %s", g_ResPath.c_str());

	ImGui::Separator();
}

void RenderCitySelector() {
	ImGui::Text("Current City Paths:");
	ImGui::Text("City index json path:");
	ImGui::SameLine();
	static std::filesystem::path s_CityIndexJsonPath = std::filesystem::current_path().parent_path().parent_path().parent_path() / "Cities" / "CityIndex.json";
	if (ImGui::Button("Browse##CityIndexJsonPath", ImVec2(80, 0))) {
		FileDialog::DialogInfo info;
		info.initialDir = s_CityIndexJsonPath.parent_path().string();
		info.filter = "JSON Files (*.json)";
		info.title = "Select City Index Json Path";

		std::string selectedPath;
		if (g_FileDialog.ShowOpenFileDialog(info, selectedPath)) {
			s_CityIndexJsonPath = selectedPath;
		}
	}
	ImGui::TextWrapped("  %s", s_CityIndexJsonPath.string().c_str());
	// Height Path
	ImGui::Text("Height Path:");
	ImGui::SameLine();
	static std::filesystem::path s_CityHeightPath = std::filesystem::current_path().parent_path().parent_path().parent_path() / "Cities";
	if (ImGui::Button("Browse##CityHeightPath", ImVec2(80, 0))) {
		FileDialog::DialogInfo info;
		info.initialDir = s_CityHeightPath.string();
		info.title = "Select Height Map Directory";

		std::string selectedPath;
		if (g_FileDialog.ShowFolderDialog(info, selectedPath)) {
			// 确保路径以斜杠结尾
			s_CityHeightPath = selectedPath;
			// if (s_CityHeightPath.back() != '/' && s_CityHeightPath.back() != '\\') {
			// 	s_CityHeightPath += "/";
			// }
		}
	}
	ImGui::TextWrapped("  %s", s_CityHeightPath.string().c_str());

	if (ImGui::Button("Process City")) {
		// iterate over all *.tif files in the directory
		ConvertCityData(s_CityHeightPath, s_CityIndexJsonPath);
	}
}

// 进度条相关函数实现
void UpdateProgress(float progress, const std::string& task) {
	g_Progress.progress = progress;
	g_Progress.currentTask = task;
	g_Progress.currentStep++;
}

void StartProcessing(int totalSteps) {
	g_Progress.isProcessing = true;
	g_Progress.isComplete = false;
	g_Progress.shouldCancel = false;
	g_Progress.progress = 0.0f;
	g_Progress.currentStep = 0;
	g_Progress.totalSteps = totalSteps;
	g_Progress.currentTask = "Initializing...";
	g_Progress.resultMessage = "";
	g_Progress.hasError = false;
	g_Progress.startTime = std::chrono::steady_clock::now();
	// 文件时间戳使用文件系统时钟，便于与 last_write_time 比较
	g_SessionStartFileTime = std::filesystem::file_time_type::clock::now();
}

void CompleteProcessing(const std::string& message, bool hasError) {
	g_Progress.isProcessing = false;
	g_Progress.isComplete = true;
	g_Progress.progress = 1.0f;
	g_Progress.resultMessage = message;
	g_Progress.hasError = hasError;
}

void RenderProgressBar() {
	if (g_Progress.isProcessing || g_Progress.isComplete) {
		// 在处理时，周期性扫描输出目录中文件数量以估算进度（不阻塞UI）
		if (g_Progress.isProcessing && g_Progress.totalSteps > 0) {
			static auto s_lastScan = std::chrono::steady_clock::now();
			auto now = std::chrono::steady_clock::now();
			if (std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastScan).count() > 300) {
				int done = 0;
				try {
					if (!g_ResPath.empty()) {
						for (const auto& entry : std::filesystem::directory_iterator(g_ResPath)) {
							if (!entry.is_regular_file()) continue;
							auto ext = entry.path().extension().string();
							auto wt = entry.last_write_time();
							if (wt < g_SessionStartFileTime) continue; // 仅统计本次运行产生的文件
							if (ext == ".hm" && g_SessionTrackHeight) { ++done; continue; }
							if (ext == ".tex" && g_SessionTrackTexture) { ++done; continue; }
						}
					}
				}
				catch (...) { /* 路径可能不存在或正在创建，忽略 */ }
				g_Progress.currentStep = done;
				g_Progress.progress = std::min(1.0f, (g_Progress.totalSteps > 0) ? (done / (float)g_Progress.totalSteps) : 0.0f);
				s_lastScan = now;
			}
		}
		ImGui::Separator();
		ImGui::Text("Processing Status:");

		// 进度条
		ImGui::ProgressBar(g_Progress.progress, ImVec2(-1, 0),
			(std::to_string((int)(g_Progress.progress * 100)) + "%").c_str());

		// 当前任务和详细信息
		if (g_Progress.isProcessing) {
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Current: %s", g_Progress.currentTask.c_str());
			ImGui::Text("Step %d / %d", (int)g_Progress.currentStep, (int)g_Progress.totalSteps);

			// 计算已用时间和预估剩余时间
			auto now = std::chrono::steady_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - g_Progress.startTime);

			if (g_Progress.progress > 0.01f) {
				auto totalEstimated = elapsed.count() / g_Progress.progress;
				auto remaining = totalEstimated - elapsed.count();

				ImGui::Text("Elapsed: %llds", elapsed.count());
				if (remaining > 0) {
					ImGui::Text("Estimated remaining: %llds", remaining);
				}
			}

			// 取消按钮
			ImGui::Spacing();
			if (ImGui::Button("Cancel Processing", ImVec2(150, 0))) {
				g_Progress.shouldCancel = true;
			}
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Click Cancel to stop processing");
		}

		// 完成状态
		if (g_Progress.isComplete) {
			ImVec4 color = g_Progress.hasError ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
			std::string status = g_Progress.hasError ? "ERROR: " : "COMPLETED: ";
			ImGui::TextColored(color, "%s%s", status.c_str(), g_Progress.resultMessage.c_str());

			// 显示总用时
			if (!g_Progress.hasError) {
				auto now = std::chrono::steady_clock::now();
				auto totalTime = std::chrono::duration_cast<std::chrono::seconds>(now - g_Progress.startTime);
				ImGui::Text("Total processing time: %llds", totalTime.count());
			}

			// 重置按钮
			if (ImGui::Button("Clear Status", ImVec2(100, 0))) {
				g_Progress.isComplete = false;
				g_Progress.resultMessage = "";
			}
		}

		ImGui::Separator();
	}
}

void ProcessWithProgress(bool genHeight, bool genTexture) {
	// 本工具会处理所有瓦片（选中与未选中），选中仅影响输出分辨率
	const int totalTiles = GRID_WIDTH * GRID_HEIGHT;
	int totalSteps = (genHeight ? totalTiles : 0) + (genTexture ? totalTiles : 0);
	if (totalSteps == 0) { CompleteProcessing("Nothing to do", true); return; }

	StartProcessing(totalSteps);
	// 记录本轮需要统计的文件类型
	g_SessionTrackHeight = genHeight;
	g_SessionTrackTexture = genTexture;

	int powerValue = GetPowerOfTwoValue();

	// 在后台线程执行重任务，UI线程保持流畅
	if (g_Progress.workerThread.joinable()) g_Progress.workerThread.join();
	g_Progress.workerThread = std::thread([=]() {
		try {
			{
				std::scoped_lock lk(g_Progress.stringMutex);
				g_Progress.currentTask = "Preparing...";
			}
			if (genHeight && !g_Progress.shouldCancel) {
				{
					std::scoped_lock lk(g_Progress.stringMutex);
					g_Progress.currentTask = "Generating height map...";
				}
				ConvertWorldHeightMapData(g_HeightPath, g_ResPath, g_GridSelection, 32768.0f, GRID_WIDTH, GRID_HEIGHT, powerValue);
			}

			if (genTexture && !g_Progress.shouldCancel) {
				{
					std::scoped_lock lk(g_Progress.stringMutex);
					g_Progress.currentTask = "Generating texture map...";
				}
				ConvertWorldTextureMapData(g_TexturePath, g_HeightPath, g_ResPath, g_GridSelection, GRID_WIDTH, GRID_HEIGHT, powerValue);
			}

			if (g_Progress.shouldCancel) CompleteProcessing("Processing cancelled by user", true);
			else                          CompleteProcessing("Generation completed", false);
	}
	catch (const std::exception& e) { CompleteProcessing(std::string("Processing failed: ") + e.what(), true); }
	catch (...) { CompleteProcessing("Unknown error occurred during processing", true); }
		});
}
