#pragma once
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <string>


// 简化版文件对话框

class FileDialog {
public:
	struct DialogInfo {
		std::string initialDir;
		std::string filter;
		std::string defaultFileName;
		std::string title;
	};

	FileDialog();
	~FileDialog();


	bool ShowOpenFileDialog(const DialogInfo& info, std::string& selectedFile);
	bool ShowSaveFileDialog(const DialogInfo& info, const std::string& defaultExt, std::string& selectedFile);
	bool ShowFolderDialog(const DialogInfo& info, std::string& selectedFolder);
	void SetParentWindow(HWND hWnd) { m_hParent = hWnd; }

private:
	HWND m_hParent;
	OPENFILENAMEA m_ofn;
	char m_filename[MAX_PATH];
	char m_foldername[MAX_PATH];

	void InitOpenFileName(const DialogInfo& info);
	static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
};

