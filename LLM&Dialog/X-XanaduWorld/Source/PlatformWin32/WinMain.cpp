#include <windows.h>
#include <Windowsx.h>

#include "../resource.h"
#include "EchoConfigFile.h"
#include "EchoProtocolComponent.h"

#include "../appview.h"
#include "../AppBase.h"
#include "../AsyncApp.h"
#include "EchoWorldSystem.h"
#include "EchoWorldManager.h"
#include "EchoCameraPathHelper.h"

#ifdef ECHO_EDITOR
#include "EchoEditorRoot.h"
#include "ImGui/imgui.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

#include "../AsyncApp.h"
#ifdef ECHO_EDITOR_MODE
#include "SceneBoxMain.h"
#endif

#include "../win32RemoteControl.h"

#pragma comment ( lib , "Imm32.lib" )

static char szTitle[] = "PadBO2";

// 自定义消息与名称 需与引擎的信息保持一致
#define WM_Client_Reload_Scene WM_USER + 10
#define SharedMemory_BufferSize 256

using namespace Echo;

static const ECHO_DEVICE_CLASS	DEVICE_CLASS = ECHO_DEVICE_DX9C;
//static const ECHO_DEVICE_CLASS	DEVICE_CLASS = ECHO_DEVICE_GLES3;
static Echo::ECHO_GRAPHIC_PARAM		g_graphicParam;


static HINSTANCE		g_Instance = NULL;
static HWND				g_Window = NULL;
static bool				g_isWindowActive = true;
static bool				g_LastTickPushR = false;
static bool				g_LastTickPushL = false;

static HWND				g_ConsoleWindow = NULL;		// 控制台子窗口
INT_PTR CALLBACK ConsoleDlgProc(HWND, UINT, WPARAM, LPARAM);
void ConsoleDlgUpdate();
const UINT WM_CSL_MSG = WM_USER + 5645;	// 控制台子窗口中的自定义消息

TCHAR szExePath[512] = { '\0' };	// 应用程序所在目录的路径


static DWORD	SCREEN_WIDTH	=	1280UL;
static DWORD	SCREEN_HEIGHT	=	720UL;

#define KEY_A  0x41
#define KEY_W  0x57
#define KEY_S  0x53
#define KEY_D  0x44
#define KEY_Q  0x51
#define KEY_E  0x45
#define KEY_K  0x4B
#define KEY_C  0x43
#define KEY_SHIFT	0x10
#define KEY_PLUS	107
#define KEY_MINUS	109

static POINT  nowMousePoint;
static POINT lastMousePoint;

extern bool g_runTest;
// const char* getAssetPath(void)
std::pair<const char*, const char*> getAssetPath()
{
	struct Initor
	{
		char szPath[_MAX_PATH] = { '\0' };
		char szShaderPath[_MAX_PATH] = { '\0' };

		Initor()
		{
			::GetModuleFileNameA(NULL, szPath, _MAX_PATH);
			int len = (int)strlen(szPath);
			for (int i = len - 1; i > 0; --i)
			{
				char c = szPath[i];
				if (c == '\\' || c == '/')
				{
					szPath[i + 1] = 0;
					break;
				}
			}

			for (int i = 0; szPath[i] != 0; ++i)
			{
				if (szPath[i] == '\\')
					szPath[i] = '/';
			}

			Echo::ConfigFile cf;
			std::string cfPath = szPath;
			cfPath.append("client_resources.cfg");
			try
			{
				cf.load(cfPath);
				strcpy(szShaderPath, szPath);
				strcat(szPath, cf.getSetting("FileSystem", "RootPath").c_str());
				strcat(szShaderPath, cf.getSetting("FileSystem", "ShaderRootPath").c_str());
			}
			catch (...)
			{
				strcpy(szShaderPath, szPath);
				strcat(szPath, "../../../../../asset/");
				strcat(szShaderPath, "../../../../../");
			}			
		}
	};
	static Initor g_Initor;
	return std::make_pair(g_Initor.szPath, g_Initor.szShaderPath);
}

LRESULT CALLBACK WndProc (HWND hWnd, UINT message,WPARAM wParam, LPARAM lParam)
{
#ifdef _WIN32_REMOTE_CONTOL_
	if (RemoteControl::Runing())
		return 0;
#endif // _WIN32_REMOTE_CONTOL_


	//输入法处理
	if(ImmGetOpenStatus(ImmGetContext(hWnd)))
		ImmSetOpenStatus(ImmGetContext(hWnd), false);
	TCHAR szEdit[800]={0};
#ifdef ECHO_EDITOR
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;
#endif
	
    switch (message)
    {
    case WM_CLOSE:
        PostQuitMessage (0);
        return 0;

	case WM_ACTIVATE:
		if (get_appview())
		{
			get_appview()->setMoveDir(0,0);
			get_appview()->setTurnDir(0,0);
		}
	break;
    case WM_DESTROY:
        return 0;
	case WM_MOUSEMOVE:
		{
			GetCursorPos(&nowMousePoint);
			
			float offsetX = float(lastMousePoint.x - nowMousePoint.x);
			float offsetY = float(lastMousePoint.y - nowMousePoint.y);

			if (g_LastTickPushR)
			{
				SetCursorPos(lastMousePoint.x,lastMousePoint.y);
				OutputDebugString("process cursor \r\n"); char msg[128] = { 0 };
				snprintf(msg, 128, "%f,%f", offsetX, offsetY);
				OutputDebugString(msg);
				if (get_appview())
					get_appview()->cameraTurn(offsetX, offsetY);
			}
			else
			{
				lastMousePoint = nowMousePoint;
			}
			//get_appview()->Tick();

			// 为了在摄像机移动时控制台窗口也会更新
			ConsoleDlgUpdate();
			
			//TODO(yanghang):Remove.For FGUI temporary test.
			{
				Echo::Vector2 point;
				point.x = LOWORD(lParam);
				point.y = HIWORD(lParam);
				if (get_appview())
				{
					get_appview()->onMouseMove(point);

					short x = LOWORD(lParam);
					short y = HIWORD(lParam);
					get_appview()->OnMouseMove(Vector2(x, y), 0);
				}					
			}
		}
		break;
	case WM_KEYUP:
		if (g_isWindowActive && get_appview()) {
			if (get_appview()->isHasImGuiWindowHovered()) {
				return 1;
			}
			get_appview()->OnKeyUp(wParam);
		}
        switch (wParam)
		{
		case KEY_A:
			get_appview()->cameraMove(0,0);
			return 0;
		case KEY_D:
			get_appview()->cameraMove(0,0);
			return 0;
		case KEY_W:
			get_appview()->cameraMove(1,0);
			return 0;
		case KEY_S:
			get_appview()->cameraMove(1,0);
			return 0;
		case KEY_SHIFT:
			get_appview()->cameraMove(3, 0);
			return 0;
		case KEY_Q:
		case KEY_E:
			get_appview()->cameraMove(2, 0);
			get_appview()->cameraMove(2, 0);
			return 0;
		}
		return 0;

    case WM_KEYDOWN:
		if (g_isWindowActive && get_appview()) {
			if (get_appview()->isHasImGuiWindowHovered()) {
				return 1;
			}
			get_appview()->OnKeyDown(wParam);
		}
        switch (wParam)
        {
        case VK_ESCAPE:
            PostQuitMessage(0);
            return 0;
		case VK_SPACE:
			{
				//g_Camera->setPosition(0.0f, 30.0f, -60.0f);
				//g_Camera->lookAt(0.0f, 0.0f, 0.0f);
			get_appview()->sphericalControlerJump();
			}
            return 0;
		case VK_F2:
			{

			}
			return 0;
		case VK_F3:
			{
				if (g_ConsoleWindow == NULL)
				{
					g_ConsoleWindow = CreateDialog(g_Instance, MAKEINTRESOURCE(IDD_DLG_CONSOLE), hWnd, ConsoleDlgProc);
					// 控制台的子窗口停靠在主窗口的右边
					RECT rcParent;
					GetWindowRect(hWnd, &rcParent);
					RECT rcCslWnd;
					GetWindowRect(g_ConsoleWindow, &rcCslWnd);
					MoveWindow(g_ConsoleWindow, rcParent.right, rcParent.top, 
							   rcCslWnd.right - rcCslWnd.left, rcCslWnd.bottom - rcCslWnd.top, FALSE);
				}
				ShowWindow(g_ConsoleWindow, SW_SHOW);
			}
			return 0;
		case VK_F4:
			get_appview()->commandParse("btest i 5");
			return 0;
		case VK_F5:
			{
				get_appview()->switchPlayMode();
			}
			return 0;
		case VK_F7:
		{
			get_appview()->createRobotObject();
		}
		return 0;
		case VK_F8:
		{
			get_appview()->create3DUIGunTest();
		}
		return 0;
		case KEY_A:
			get_appview()->cameraMove(0,-1);
			return 0;
		case KEY_D:
			get_appview()->cameraMove(0,1);
			return 0;
		case KEY_W:
			get_appview()->cameraMove(1,1);
			return 0;
		case KEY_S:
			get_appview()->cameraMove(1,-1);
			return 0;
		case KEY_Q:
			get_appview()->cameraMove(2, -1);
			return 0;
		case KEY_E:
			get_appview()->cameraMove(2, 1);
			return 0;
		case KEY_SHIFT:
			get_appview()->cameraMove(3, 1);
			return 0;
//		case KEY_PLUS:
//			g_pGameObj->incCameraSpeed();
//			return 0;
//		case KEY_MINUS:
//			g_pGameObj->decCameraSpeed();
//			return 0;
		case KEY_K:
			get_appview()->attack();
			return 0;
		case VK_LEFT:
			get_appview()->cameraTurn(2, 0);
			return 0;
		case VK_RIGHT:
			get_appview()->cameraTurn(-2, 0);
			return 0;
		case VK_UP:
			get_appview()->cameraTurn(0, 2);
			return 0;
		case VK_DOWN:
			get_appview()->cameraTurn(0, -2);
			return 0;
		}
        return 0;
	case WM_CHAR:
		switch (wParam)
		{
		case '-':
			get_appview()->decCameraSpeed();
			break;
		case '+':
			get_appview()->incCameraSpeed();
			break;
		default:	break;
		}
		get_appview()->OnInputText(wParam);
		break;
	case WM_IME_CHAR:
	{
		DWORD wChar = (DWORD)wParam;
		if (wChar <= 127)
		{
			get_appview()->OnInputText((char16_t)wChar);
		}
		else
		{
			// swap lower and upper part.
			BYTE low = (BYTE)(wChar & 0x00FF);
			BYTE high = (BYTE)((wChar & 0xFF00) >> 8);
			wChar = MAKEWORD(high, low);

			wchar_t ch[6];
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&wChar, 4, ch, 3);
			get_appview()->OnInputText((char16_t)ch[0]);
		}
		return 0;
	}
   case WM_SIZE:	
	{
	   SCREEN_WIDTH = LOWORD(lParam); // width of client area
	   SCREEN_HEIGHT = HIWORD(lParam); // height of client area
	   if (get_appview())
	   {
		   get_appview()->onResize(SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, SCREEN_HEIGHT, SCREEN_WIDTH);

		   if (wParam == SIZE_MINIMIZED)
		   {
			   if (g_isWindowActive)
			   {
				   get_appview()->onPause();
				   get_appview()->onStop();
				   g_isWindowActive = false;
			   }
		   }
		   else if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
		   {
			   if (!g_isWindowActive)
			   {
				   get_appview()->onStart();
				   get_appview()->onResume();
				   g_isWindowActive = true;
			   }
		   }
	   }
	   return 0;
	 }
	case WM_RBUTTONDOWN:
		SetFocus(g_Window);
		g_LastTickPushR = true;
		ShowCursor(false);
		
		if (get_appview())
		{
			Echo::Vector2 point;
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);

			if (g_isWindowActive) {
				get_appview()->OnMouseRightDown(point);
			}
		}

		return 0;
	case WM_RBUTTONUP:
		SetFocus(g_Window);
		ShowCursor(true);
		g_LastTickPushR = false;


		if (get_appview())
		{
			Echo::Vector2 point;
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);

			if (g_isWindowActive) {
				get_appview()->OnMouseRightUp(point);
			}
		}

		return 0;

	case WM_LBUTTONDOWN:
	{
		if (get_appview() && get_appview()->isHasImGuiWindowHovered()) {
			return 1;
		}
		SetFocus(g_Window);
		g_LastTickPushL = true;
		//NOTE(yanghang):FGUI input.
		if (get_appview())
		{
			Echo::Vector2 point;
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			get_appview()->onMouseLeftDown(point);

			if (g_isWindowActive) {
				get_appview()->OnMouseLeftDown(point);
			}
		}

		return 0;
	}
	case WM_LBUTTONUP:
	{
		if (get_appview() && get_appview()->isHasImGuiWindowHovered()) {
			return 1;
		}
		SetFocus(g_Window);
		g_LastTickPushL = false;
		//NOTE(yanghang):FGUI input.
		if (get_appview())
		{
			Echo::Vector2 point;
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			get_appview()->onMouseLeftUp(point);

			if (g_isWindowActive) {
				get_appview()->OnMouseLeftUp(point);
			}
		}
		return 0;
	}
	case WM_MOUSEWHEEL:
		{
			short zDelta = (short)HIWORD(wParam);
			get_appview()->cameraDisChg(zDelta);


			//if (g_pGameObj->GetPlayMode() == MODE_FREE)
			//{
			//	short zDelta = (short)HIWORD(wParam);   
			//	if( zDelta >= 0 )
			//	{
			//		Echo::Vector3 forward = g_Camera->getDirection();
			//		g_Camera->move(forward * MOVE_SPEED * 5.0f);

			//		float radius = g_pGameObj->GetRadius();
			//		radius -= MOVE_SPEED * 5.0f;
			//		g_pGameObj->SetRadius(radius);
			//	}
			//	else
			//	{
			//		Echo::Vector3 retreat = -g_Camera->getDirection();
			//		g_Camera->move(retreat * MOVE_SPEED * 5.0f);

			//		float radius = g_pGameObj->GetRadius();
			//		radius += MOVE_SPEED * 5.0f;
			//		g_pGameObj->SetRadius(radius);
			//	}
			//}
		}

        return 0;

	case WM_KILLFOCUS:
	{
		AppBase* app = get_appview();
		if (app) {
			app->OnKillFocus();
		}
	
		return 0;
	}
	break;
	case WM_Client_Reload_Scene:
	{
		// 引擎的共享内存名称，需与引擎保持一致
		const char sharedMemoryName[] = "EchoSharedMemoryForClientLoadSence";

		HANDLE hMapFile = ::OpenFileMapping(FILE_MAP_ALL_ACCESS,
			FALSE,
			sharedMemoryName);

		if (hMapFile) {
			
			void* pBuf = ::MapViewOfFile(hMapFile,
				FILE_MAP_ALL_ACCESS,
				0,
				0,
				SharedMemory_BufferSize);

			if (pBuf) {
				// 接收场景名称重新加载
				String senceName = (char*)(pBuf);

				get_appview()->loadSenceByName(senceName);
				// 解除映射
				::UnmapViewOfFile(pBuf);
			}
			else {
				::MessageBox(hWnd, "MapViewOfFile failed!", "Error", NULL);
			}

			// 关闭句柄
			::CloseHandle(hMapFile);
		}
		else {
			::MessageBox(hWnd, "OpenFileMapping failed!", "Error", NULL);
		}

		return 0;
	}
	break;
    }

	return DefWindowProc(hWnd, message, wParam, lParam);
}

INT_PTR CALLBACK ConsoleDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM
						   lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			char szCmdLog[256] ={ '\0' };
			strcpy(szCmdLog, szExePath);
			strcat(szCmdLog, "cmd.log");
			FILE *fp = fopen(szCmdLog, "rt");
			if (fp != nullptr)
			{
				HWND hWndCmds = GetDlgItem(hDlg, IDC_CMB_CMDS);

				char szBuff[2048] ={ '\0' };
				while (fgets(szBuff, sizeof(szBuff), fp) != nullptr)
				{
					size_t szLen = strlen(szBuff);
					if (szLen <= 1)
						continue;
					if (szBuff[szLen - 1] == '\n')
						szBuff[szLen - 1] = '\0';
					SendMessage(hWndCmds, CB_ADDSTRING, 0, (LPARAM)szBuff);
				}
				fclose(fp);
			}
		}
		return 0;
	case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDC_BTN_OK)
			{
				HWND hWndCmds = GetDlgItem(hDlg, IDC_CMB_CMDS);
				char szCmd[1024] = { '\0' };
				DWORD iCount = (DWORD)SendMessage(hWndCmds, WM_GETTEXT, sizeof(szCmd), (LPARAM)szCmd);
				if (iCount > 0)
				{
					get_appview()->commandParse(szCmd);
					
					// 把当前的命令加到下拉列表的第一行
					// 查找列表中是否已经有相同的命令, 如果有则删除这一项
					DWORD iFind = (DWORD)SendMessage(hWndCmds, CB_FINDSTRING, 0, (LPARAM)szCmd);
					if (iFind != CB_ERR) {
						SendMessage(hWndCmds, CB_DELETESTRING, iFind, 0);
					}
					// 把当前处理的名利插入到下拉列表的第一行
					SendMessage(hWndCmds, CB_INSERTSTRING, 0, (LPARAM)szCmd);

					// 要把自己清空
					SendMessage(hWndCmds, WM_SETTEXT, 0, 0);
				}
			}
			else if (LOWORD(wParam) == IDC_BTN_CLEAR)
			{
				HWND hWndCmds = GetDlgItem(hDlg, IDC_CMB_CMDS);
				DWORD iCount = (DWORD)SendMessage(hWndCmds, CB_GETCOUNT, 0, 0);
				for (DWORD i=0; i<iCount; ++i) {
					SendMessage(hWndCmds, CB_DELETESTRING, 0, 0);
				}
			}
			else if (LOWORD(wParam) == 2) {
				ShowWindow(hDlg, SW_HIDE);
			}
		}
		return 0;
	case WM_KEYDOWN:
		{
			switch (wParam)
			{
			case VK_ESCAPE:
				ShowWindow(hDlg, SW_HIDE);
				break;
			default:
				break;
			}
		}
		return 0;
	case WM_CSL_MSG:
		{
			std::string szUsage = get_appview()->getResMemUsageStr();
			HWND hWndResMem = GetDlgItem(hDlg, IDC_STATIC_RESMEM);
			SendMessage(hWndResMem, WM_SETTEXT, 0, (LPARAM)(szUsage.c_str()));
		}
		return 0;
	case WM_DESTROY:
		{
			HWND hWndCmds = GetDlgItem(hDlg, IDC_CMB_CMDS);
			DWORD iCount = (DWORD)SendMessage(hWndCmds, CB_GETCOUNT, 0, 0);
			if (iCount > 0)
			{
				char szCmdLog[256] ={ '\0' };
				strcpy(szCmdLog, szExePath);
				strcat(szCmdLog, "cmd.log");
				FILE *fp = fopen(szCmdLog, "wt");
				if (fp != nullptr)
				{
					char szBuff[1024] ={ '\0' };
					for (DWORD i=0; i<iCount; ++i)
					{
						SendMessage(hWndCmds, CB_GETLBTEXT, i, (LPARAM)szBuff);
						if (strlen(szBuff) > 0)
						{
							fputs(szBuff, fp);
							fputs("\n", fp);
						}
					}
					fclose(fp);
				}
			}
		}
		return 0;
	}
	return 0;
}

void ConsoleDlgUpdate()
{
	if (g_ConsoleWindow && get_appview())
	{
		char msg[256] = {};
		sprintf_s(msg, "%d", get_appview()->getDrawCall());
		HWND hwnd = GetDlgItem(g_ConsoleWindow, IDC_DRAWCALL);
		SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)(msg));

		memset(msg, 0, 256);
		sprintf_s(msg, "%d", get_appview()->getTriangleCnt());
		hwnd = GetDlgItem(g_ConsoleWindow, IDC_TRANGLE);
		SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)(msg));

		memset(msg, 0, 256);
		sprintf_s(msg, "%d", get_appview()->getFps());
		hwnd = GetDlgItem(g_ConsoleWindow, IDC_FPS);
		SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)(msg));

		std::string szCamPos = get_appview()->getCameraPosition();
		hwnd = GetDlgItem(g_ConsoleWindow, IDC_EDIT_CAMERA_POSITION);
		char szOldCamPos[256] = { '\0' };
		SendMessage(hwnd, WM_GETTEXT, sizeof(szOldCamPos) - 1, (LPARAM)szOldCamPos);
		if (szCamPos != szOldCamPos)
			SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)(szCamPos.c_str()));

		std::string szCamOri = get_appview()->getCameraOrientation();
		hwnd = GetDlgItem(g_ConsoleWindow, IDC_EDIT_CAMERA_ORIENTATION);
		char szOldCamOri[256] = { '\0' };
		SendMessage(hwnd, WM_GETTEXT, sizeof(szOldCamOri) - 1, (LPARAM)szOldCamOri);
		if (szCamOri != szOldCamOri)
			SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)(szCamOri.c_str()));
	}


	if (g_ConsoleWindow != NULL && IsWindowVisible(g_ConsoleWindow) == TRUE)
		PostMessage(g_ConsoleWindow, WM_CSL_MSG, 0, 0);
	
}

/*
bool callback(const wchar_t *dump_path, const wchar_t *id,
	void *context, EXCEPTION_POINTERS *exinfo,
	MDRawAssertionInfo *assertion,
	bool succeeded)
{
	wchar_t szMsg[MAX_PATH] = { 0 };

	if (succeeded)
	{
		wsprintfW(szMsg, L"程序崩溃，生成dump文件.\r\ndump file : %s", dump_path);
		FatalAppExitW(-1, szMsg);
	}
	else
	{
		wsprintfW(szMsg, L"程序崩溃，生成dump文件.\r\ndump file : %s", dump_path);
		FatalAppExitW(-1, szMsg);
	}

	return succeeded;
}
*/
bool g_testEnd;
void callBack(const Vector3& cameraNewPos, int nPlayPercent)
{
	if (nPlayPercent >= 99) {

		g_testEnd = true;
	}
}
int WINAPI WinMain(_In_ HINSTANCE hInstance,
					_In_opt_ HINSTANCE hPrevInstance,
					_In_ LPSTR lpCmdLine,
					_In_ int iCmdShow)
{
#ifndef ECHO_EDITOR
	bool bEditorMode = false;
#else
	bool bEditorMode = true;
#endif
	bool runAutomation = false;
	std::string scene = "";
	std::vector<std::string> cmdVec = StringUtil::split(lpCmdLine);
	if (!cmdVec.empty())
	{
		for (size_t i = 0; i < cmdVec.size(); i++)
		{
			if ("-t" == cmdVec[i])
			{
				SetRunTest(true);
			}

			if ("-s" == cmdVec[i])
			{
				runAutomation = true;
				if (i < cmdVec.size())
				{
					scene = cmdVec[i+1];
					SetInitLevel(scene);
				}
			}
		}
	}

	// 适应高DPI屏幕
	FARPROC SetProcessDPIAwareFunc = GetProcAddress(GetModuleHandle(TEXT("user32")), "SetProcessDPIAware");
	if (SetProcessDPIAwareFunc != NULL) {
		SetProcessDPIAwareFunc();
	}

	GetModuleFileName(NULL, szExePath, sizeof(szExePath) - sizeof(szExePath[0]));

	//init remote controler
	{
#ifdef _WIN32_REMOTE_CONTOL_
		initRemoteControl();
#endif // _WIN32_REMOTE_CONTOL_

	}


#ifdef ECHO_EDITOR_MODE
	if (bEditorMode)
	{		
		int argc = (int)cmdVec.size() + 1;
		char ** argv = new char *[argc];
		argv[0] = new char[strlen(szExePath) + 1];
		strcpy(argv[0], szExePath);
		for (int i = 0; i < cmdVec.size(); i++)
		{
			argv[i + 1] = new char[cmdVec[i].length() + 1];
			strcpy(argv[i + 1], cmdVec[i].c_str());
		}
		onStartEditor(argc, argv);
		return 0;
	}
	else
#endif
	{

		g_Instance = hInstance;
		TCHAR *p = strrchr(szExePath, '\\');
		if (*p != NULL) {
			p[1] = '\0';
		}

		WNDCLASS wc;
		HWND hWnd;
		MSG msg;
		BOOL bQuit = FALSE;

		//AllocConsole();
		//freopen("CONOUT$","w+t",stdout);
		//freopen(("CONIN$"), "r+t",stdin);


		/* register window class */
		wc.style = CS_OWNDC | CS_DBLCLKS;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon (NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor (NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject (BLACK_BRUSH);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = szTitle;
		RegisterClass (&wc);

		// 将主窗口定位到主屏幕的正中
		RECT rcDesk;
		GetWindowRect(GetDesktopWindow(), &rcDesk);
		int iWndWidth = SCREEN_WIDTH + 16;
		int iWndHeight = SCREEN_HEIGHT + 39;
		int iPosX = rcDesk.left + (((rcDesk.right - rcDesk.left) - iWndWidth) >> 1);
		int iPosZ = rcDesk.top + (((rcDesk.bottom - rcDesk.top) - iWndHeight) >> 1);

		hWnd = CreateWindow (
			szTitle, szTitle,
			WS_OVERLAPPEDWINDOW,
			iPosX, iPosZ, iWndWidth, iWndHeight,
			NULL, NULL, hInstance, NULL);
		ShowWindow(hWnd, iCmdShow);
		/*ImmDisableIME(0xFFFFFFFF);*/
		g_Window = hWnd;

		//************************************************************************************
		// 游戏初始化
		{
			printf("****************************\n");
			printf("game init begin\n");

			g_graphicParam.eClass = DEVICE_CLASS;
			g_graphicParam.pHandle = (void*)hWnd;
			g_graphicParam.pInstance = (void*)hInstance;
			g_graphicParam.cpuCoreNumber = std::thread::hardware_concurrency();

			printf("game init appview\n");
			auto pathRoot = getAssetPath();
			GetArchivePack(pathRoot.first, pathRoot.first);
			init_appview(!bEditorMode,!bEditorMode,pathRoot.first, pathRoot.second, &g_graphicParam, SCREEN_WIDTH, SCREEN_HEIGHT, 0, SCREEN_WIDTH, 0, SCREEN_HEIGHT);
		}
		//************************************************************************************
		if (g_runTest)
		{
			Echo::Name activeWorldName = Echo::WorldSystem::instance()->GetActiveWorld();
			WorldManager* activeWorldManager = Echo::WorldSystem::instance()->GetWorldManager(activeWorldName);
			activeWorldManager->setLoadingPrepare();
			activeWorldManager->setLoadingStart();
			activeWorldManager->setLoadingCallback([](const Name& worldName, int _pct) {
				if (_pct > 99)
				{
					Echo::CameraPathHelper::instance()->startPlayingByCurveID(1, Echo::CameraPathMode::e_FOllOW_MODE, callBack);
				}
				});
		}

		::GetCursorPos(&lastMousePoint);
		ScreenToClient(g_Window, &lastMousePoint);

		/* program main loop */
		while (!bQuit)
		{
			/* check for messages */
			if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
			{
				/* handle or dispatch messages */
				if (msg.message == WM_QUIT || g_testEnd == true)
				{
					bQuit = TRUE;
				}
				else
				{
					if (g_ConsoleWindow == NULL || !IsDialogMessage(g_ConsoleWindow, &msg))
					{
						TranslateMessage (&msg);
						DispatchMessage (&msg);
					}
				}
			}
			else
			{
				if (get_appview())
				{
					if (g_isWindowActive)
						get_appview()->onTick();
					else
						Sleep(200);
				}

				ConsoleDlgUpdate();

				Sleep (1);
			}
			{
#ifdef _WIN32_REMOTE_CONTOL_
				onUpdate();
#endif // _WIN32_REMOTE_CONTOL_

			}
		}

		//************************************************************************************
		// 释放游戏
		printf("game end\n");
		if (get_appview())
		{
			get_appview()->onRelease(true);
		}
		//************************************************************************************
		{
#ifdef _WIN32_REMOTE_CONTOL_
			destoryRemoteControl();
#endif // _WIN32_REMOTE_CONTOL_

		}

	   //FreeConsole();

		/* destroy the window explicitly */
		DestroyWindow (hWnd);

		return (int)msg.wParam;
	}
}


