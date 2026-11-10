#pragma once

#include "AppBase.h"

namespace Echo
{
	struct ECHO_GRAPHIC_PARAM;
}

AppBase* get_appview();

void                        init_appview(bool bAsyncApp, bool bClientMode, const char* _assetPath, const char* _dataPath, const Echo::ECHO_GRAPHIC_PARAM* pGraphicParam, Echo::uint32 width, Echo::uint32 height);
void						init_appview(bool bAsyncApp, bool bClientMode, const char* _assetPath, const char* _dataPath, const Echo::ECHO_GRAPHIC_PARAM* pGraphicParam, Echo::uint32 width, Echo::uint32 height, Echo::uint32 _left, Echo::uint32 _right, Echo::uint32 _bottom, Echo::uint32 _top);


namespace Echo
{
	class DefaultArchive* GetArchivePack(const char* _szRoot, const char* _dataPath);
	class IArchive* GetArchivePack();
}

