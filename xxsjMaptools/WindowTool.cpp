#include<direct.h>
#include "WindowTool.h"
#include<iostream>

FileDialog::FileDialog()
{
	memset(&m_ofn, 0, sizeof(m_ofn));
	memset(m_filename, 0, sizeof(m_filename));
	memset(m_foldername, 0, sizeof(m_foldername));
}

FileDialog::~FileDialog()
{
}

bool FileDialog::ShowOpenFileDialog(const DialogInfo& info, std::string& selectedFile)
{
	char* oldCurDir = _getcwd(0, 0); // Save current working directory

	InitOpenFileName(info);

	// Set dialog title
	std::string title = info.title.empty() ? "Select a file" : info.title;
	m_ofn.lpstrTitle = title.c_str();

	// Set flags
	m_ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	BOOL result = GetOpenFileNameA(&m_ofn);

	if (result)
	{
		selectedFile = m_filename;
	}

	// Restore working directory
	if (oldCurDir)
	{
		_chdir(oldCurDir);
		free(oldCurDir);
	}

	return (result == TRUE);
}

bool FileDialog::ShowSaveFileDialog(const DialogInfo& info, const std::string& defaultExt, std::string& selectedFile)
{
	char* oldCurDir = _getcwd(0, 0);

	InitOpenFileName(info);

	// Set default extension
	m_ofn.lpstrDefExt = defaultExt.c_str();

	// Set dialog title
	std::string title = info.title.empty() ? "Save File" : info.title;
	m_ofn.lpstrTitle = title.c_str();

	// Set flags
	m_ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

	BOOL result = GetSaveFileNameA(&m_ofn);

	if (result)
	{
		selectedFile = m_filename;
	}

	if (oldCurDir)
	{
		_chdir(oldCurDir);
		free(oldCurDir);
	}

	return (result == TRUE);
}

bool FileDialog::ShowFolderDialog(const DialogInfo& info, std::string& selectedFolder)
{
	char* oldCurDir = _getcwd(0, 0);

	std::string defaultPath = info.initialDir.empty() ? (oldCurDir ? oldCurDir : "") : info.initialDir;

	BROWSEINFOA bi = { 0 };

	// Set dialog title
	std::string title = info.title.empty() ? "Select a folder" : info.title;
	bi.lpszTitle = title.c_str();
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.hwndOwner = m_hParent;
	bi.lParam = reinterpret_cast<LPARAM>(defaultPath.c_str());
	bi.lpfn = BrowseCallbackProc;

	LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
	BOOL result = FALSE;

	if (pidl != nullptr)
	{
		if (SHGetPathFromIDListA(pidl, m_foldername))
		{
			selectedFolder = m_foldername;
			result = TRUE;
		}
		CoTaskMemFree(pidl);
	}

	if (oldCurDir)
	{
		_chdir(oldCurDir);
		free(oldCurDir);
	}

	return (result == TRUE);
}

void FileDialog::InitOpenFileName(const DialogInfo& info)
{
	memset(&m_ofn, 0, sizeof(m_ofn));

	m_ofn.lStructSize = sizeof(OPENFILENAMEA);
	m_ofn.hwndOwner = m_hParent;
	m_ofn.nFilterIndex = 1;

	// Set filename
	strncpy_s(m_filename, sizeof(m_filename), info.defaultFileName.c_str(), _TRUNCATE);
	m_ofn.lpstrFile = m_filename;
	m_ofn.nMaxFile = sizeof(m_filename);

	// Set initial directory
	m_ofn.lpstrInitialDir = info.initialDir.empty() ? nullptr : info.initialDir.c_str();

	// Set filter
	m_ofn.lpstrFilter = info.filter.empty() ? "All Files\0*.*\0" : info.filter.c_str();
}

int FileDialog::BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if (uMsg == BFFM_INITIALIZED)
	{
		// Set default path
		SendMessageA(hwnd, BFFM_SETSELECTION, TRUE, lpData);
	}
	return 0;
}
