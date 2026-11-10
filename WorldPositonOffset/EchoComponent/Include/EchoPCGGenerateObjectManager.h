//-----------------------------------------------------------------------------
// File:   EchoPCGGraphComponentManager.h
//
// Author: qiao_yuzhi
//
// Date:   2026-3-2
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once

#include "EchoComponentPrerequisites.h"

namespace Echo
{
	class _EchoComponentExport PCGGenerateObjectManager : public Singleton<PCGGenerateObjectManager>, public FrameListener
	{
	public:
		PCGGenerateObjectManager();
		virtual ~PCGGenerateObjectManager();

		virtual bool frameStarted(const FrameEvent& evt) override;
	private:

	};
}
