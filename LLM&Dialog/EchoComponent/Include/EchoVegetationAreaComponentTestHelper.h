///////////////////////////////////////////////////////////////////////////////
//
// @file EchoVegetationAreaComponentTestHelper.h
// @author chensheng
// @date 2024/04/17 Wednesday
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _EchoVegetationAreaComponentTestHelper_H__
#define _EchoVegetationAreaComponentTestHelper_H__

#include "EchoComponentPrerequisites.h"  
namespace Echo
{
	class _EchoComponentExport VegAreaCompTestHelper
	{
	public:
		VegAreaCompTestHelper();
		~VegAreaCompTestHelper();

		void setHandle(class VegetationAreaComponent* handle);
		bool isShowCompArea()										const;
		bool isShowVegArea()										const;
		bool isShowRayHit()											const;
		bool isShowVegAreaRes()										const;
		bool isShowSubVegRes(uint32 sub)							const;
		bool isShowSubVegBlock(uint32 sub, uint32 LOD)				const;
		bool isShowSubVegInfo(uint32 sub, uint32 LOD)				const;
		bool isShowVegAreaCull()									const;

		void showCompArea(bool flag);
		void showVegArea(bool flag);
		void showVegAreaRes(bool flag);
		void showRayHit(bool flag);
		void showSubVegRes(uint32 sub, bool flag);
		void showSubVegBlock(uint32 sub, uint32 LOD, bool flag);
		void showSubVegInfo(uint32 sub, uint32 LOD, bool flag);
		void showVegAreaCull(bool flag);

		void update();
	private:
		struct Private;
		Private* _dp = nullptr;
	private:
		VegAreaCompTestHelper(const VegAreaCompTestHelper&)				= delete;
		VegAreaCompTestHelper& operator=(const VegAreaCompTestHelper&)	= delete;
	};
}
#endif
