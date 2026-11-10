
#include "EchoDefaultArchive.h"
#include "appview.h"

#ifdef __ANDROID__
extern const char* GetAssetPath();
extern const char* GetDataPath();
extern AAssetManager * GetAssetManager();
#endif
#ifdef __OHOS__
extern NativeResourceManager* GetAssetManager();
#endif

#include "AsyncApp.h"
#include "SyncApp.h"

bool g_bAsyncApp = true;

AppBase * get_appview()
{
	if (g_bAsyncApp)
		return GetAsyncApp();
	else
		return GetSyncApp();

}

void init_appview(bool bAsyncApp, bool bClientMode, const char* _assetPath, const char* _dataPath, const Echo::ECHO_GRAPHIC_PARAM* pGraphicParam, Echo::uint32 width, Echo::uint32 height)
{
	g_bAsyncApp = bAsyncApp;

	if(bAsyncApp)
		InitAsyncApp(_assetPath, _dataPath, pGraphicParam, bClientMode, width, height);
	else
		InitSyncApp(_assetPath, _dataPath, pGraphicParam, bClientMode, width, height);
}

void init_appview(bool bAsyncApp, bool bClientMode, const char* _assetPath, const char* _dataPath, const Echo::ECHO_GRAPHIC_PARAM* pGraphicParam, Echo::uint32 width, Echo::uint32 height, Echo::uint32 _left, Echo::uint32 _right, Echo::uint32 _bottom, Echo::uint32 _top)
{
	g_bAsyncApp = bAsyncApp;

	if (bAsyncApp)
		InitAsyncApp(_assetPath, _dataPath, pGraphicParam, bClientMode, width, height, _left, _right, _bottom, _top);
	else
		InitSyncApp(_assetPath, _dataPath, pGraphicParam, bClientMode, width, height, _left, _right, _bottom, _top);
}

namespace Echo
{
	DefaultArchive* _pDefaultArchive = nullptr;
	DefaultArchive* GetArchivePack(const char* _szRoot, const char* _dataPath)
	{
#if defined (__ANDROID__) || defined (__OHOS__)
		static DefaultArchive _ArchivePack(_szRoot, _dataPath, GetAssetManager());
#else
		static DefaultArchive _ArchivePack(_szRoot);
#endif

		_pDefaultArchive = &_ArchivePack;

		return _pDefaultArchive;
	}

	IArchive* GetArchivePack()
	{
		if (!_pDefaultArchive)
		{
			assert(false && "GetArchivePack is NULL");
			return nullptr;
		}
		return _pDefaultArchive;
	}
}