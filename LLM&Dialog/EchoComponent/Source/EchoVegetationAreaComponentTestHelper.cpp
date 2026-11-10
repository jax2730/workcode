///////////////////////////////////////////////////////////////////////////////
//
// @file EchoVegetationAreaComponentTestHelper.cpp
// @author chensheng
// @date 2024/04/17 Wednesday
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
#include "EchoComponentStableHeaders.h"
#include "EchoVegetationAreaComponentTestHelper.h"
#include "EchoVegetationAreaComponent.h"
#include "EchoVegetationArea.h"
#include "EchoSubVegetationAreaResource.h"

#include "EchoVegetationAreaTestHelper.h"

namespace Echo
{
	struct VegAreaCompTestHelper::Private
	{
		VegAreaComp* mHandle			= nullptr;

		VegTestHelper* mCompArea		= nullptr;
		VegTestHelper* mVegAreaBox		= nullptr;
		VegTestHelper* mVegAreaResBox	= nullptr;
		std::vector<VegTestHelper*> mRayHitObjVec;

		VegTestHelper* mVegAreaCullResult		= nullptr;

		struct _SubVegShow
		{
			~_SubVegShow()
			{
				delete mBox;
				for (int i = 0; i < 2; i++)
				{
					delete mBlock[i];
					delete mInfo[i];
				}
			}

			VegTestHelper* mBox			= nullptr;
			VegTestHelper* mBlock[2]	= {};
			VegTestHelper* mInfo[2]		= {};
		};
		_SubVegShow	mSubVegShow[3]		= {};

		~Private()
		{
			delete mCompArea;
			delete mVegAreaBox;
			delete mVegAreaResBox;
			for (VegTestHelper* it : mRayHitObjVec)
				delete it;
		}
	};

	VegAreaCompTestHelper::VegAreaCompTestHelper() :_dp(new Private) {};
	VegAreaCompTestHelper::~VegAreaCompTestHelper() { delete _dp; };

	void VegAreaCompTestHelper::setHandle(VegAreaComp* handle)					  { _dp->mHandle = handle; };
	bool VegAreaCompTestHelper::isShowCompArea()							const { return _dp->mCompArea; };
	bool VegAreaCompTestHelper::isShowVegArea()								const { return _dp->mVegAreaBox; };
	bool VegAreaCompTestHelper::isShowRayHit()								const { return !_dp->mRayHitObjVec.empty(); };
	bool VegAreaCompTestHelper::isShowVegAreaRes()							const { return _dp->mVegAreaResBox; };
	bool VegAreaCompTestHelper::isShowSubVegRes(uint32 sub)					const { return _dp->mSubVegShow[sub].mBox; };
	bool VegAreaCompTestHelper::isShowSubVegBlock(uint32 sub, uint32 LOD)	const { return _dp->mSubVegShow[sub].mBlock[LOD]; };
	bool VegAreaCompTestHelper::isShowSubVegInfo(uint32 sub, uint32 LOD)	const { return _dp->mSubVegShow[sub].mInfo[LOD]; }
	bool VegAreaCompTestHelper::isShowVegAreaCull()							const { return _dp->mVegAreaCullResult != nullptr;}
	;

	void VegAreaCompTestHelper::update()
	{
		if (isShowCompArea())	showCompArea(true);
		if (isShowVegArea())	showVegArea(true);
		if (isShowVegAreaRes()) showVegAreaRes(true);
		if (isShowRayHit())		showRayHit(true);
		if (isShowVegAreaCull())showVegAreaCull(true);

		for (uint32 subIndex = 0; subIndex < 3; subIndex++)
		{
			if (isShowSubVegRes(subIndex)) showSubVegRes(subIndex, true);
			for (uint32 lod = 0; lod < 2; lod++)
			{
				if (isShowSubVegInfo(subIndex, lod))  showSubVegInfo(subIndex, lod, true);
				if (isShowSubVegBlock(subIndex, lod)) showSubVegBlock(subIndex, lod, true);
			}
		}
	}

#define _SafeDelete(ptr) if(ptr){ delete ptr; ptr = nullptr;};

	void VegAreaCompTestHelper::showCompArea(bool flag)
	{
		if (nullptr == _dp->mHandle) return;
		_SafeDelete(_dp->mCompArea);
		if (flag)
		{
			std::vector<Vector3> area; Vector3 dir; Vector2 range;
			if (_dp->mHandle->getAreaDesc(area, dir, range))
				_dp->mCompArea = new VegTestHelper(area, dir, range);
		}
	}

	void VegAreaCompTestHelper::showVegArea(bool flag)
	{
		if (nullptr == _dp->mHandle) return;
		VegetationArea* vegArea = _dp->mHandle->getVegHandleForUI();
		if (nullptr == vegArea) return;
		_SafeDelete(_dp->mVegAreaBox);
		if (flag)
		{
			const AxisAlignedBox box = vegArea->getBox();
			if (!box.isNull())
				_dp->mVegAreaBox = new VegTestHelper(box, VegTestHelper::R);
		}
	}

	void VegAreaCompTestHelper::showVegAreaRes(bool flag)
	{
		if (nullptr == _dp->mHandle) return;
		VegetationArea* vegArea = _dp->mHandle->getVegHandleForUI();
		if (nullptr == vegArea) return;
		_SafeDelete(_dp->mVegAreaResBox);
		if (flag)
		{
			const AxisAlignedBox box = vegArea->getResBox();
			if (!box.isNull())
				_dp->mVegAreaResBox = new VegTestHelper(box, VegTestHelper::R);
		}
	}

	void VegAreaCompTestHelper::showSubVegRes(uint32 sub, bool flag)
	{
		if (nullptr == _dp->mHandle) return;
		VegetationArea* vegArea = _dp->mHandle->getVegHandleForUI();
		if (nullptr == vegArea) return;
		_SafeDelete(_dp->mSubVegShow[sub].mBox);
		if (flag)
		{
			const AxisAlignedBox box = vegArea->getSubResBox(sub);
			if (!box.isNull())
				_dp->mSubVegShow[sub].mBox = new VegTestHelper(box, VegTestHelper::R);
		}
	}

	void VegAreaCompTestHelper::showSubVegBlock(uint32 sub, uint32 LOD, bool flag)
	{
		if (nullptr == _dp->mHandle) return;
#ifndef ECHO_EDITOR
		return;
#else // ECHO_EDITOR

		VegetationArea* vegArea = _dp->mHandle->getVegHandleForUI();

		if (nullptr == vegArea) return;
		_SafeDelete(_dp->mSubVegShow[sub].mBlock[LOD]);
		if (flag)
		{
			const std::vector<AxisAlignedBox>& boxVec = vegArea->getCullBox(sub, LOD);
			if (boxVec.empty())
				return;
			_dp->mSubVegShow[sub].mBlock[LOD] = new VegTestHelper(boxVec, VegTestHelper::G);
		}
#endif
	}

	void VegAreaCompTestHelper::showSubVegInfo(uint32 sub, uint32 LOD, bool flag)
	{
		if (nullptr == _dp->mHandle) return;
		VegetationArea* vegArea = _dp->mHandle->getVegHandleForUI();
		if (nullptr == vegArea) return;
		_SafeDelete(_dp->mSubVegShow[sub].mInfo[LOD]);
		if (flag)
		{
			std::vector<AxisAlignedBox> boxVec;
			std::vector<Vector3> rectVec;
			if (vegArea->getSubResInfo(sub, LOD, boxVec, rectVec))
				_dp->mSubVegShow[sub].mInfo[LOD] = new VegTestHelper(boxVec, VegTestHelper::R);
		}
	}

	void VegAreaCompTestHelper::showVegAreaCull(bool flag)
	{
		if (nullptr == _dp->mHandle) return;
		VegetationArea* vegArea = _dp->mHandle->getVegHandleForUI();
		if (nullptr == vegArea) return;
		_SafeDelete(_dp->mVegAreaCullResult);
		if (flag)
		{
			vegArea->setCullRecord(true);
			vegArea->getCullBox();
			Vector4 color(0.5f, 0.1f, 0.1f, 0.1f);
			const std::vector<AxisAlignedBox>& cullAABBList = vegArea->getCullBox();
			if (cullAABBList.empty())return;
			_dp->mVegAreaCullResult = new VegTestHelper(cullAABBList, color);
		}else{
			vegArea->setCullRecord(flag);
		}
	}

	void VegAreaCompTestHelper::showRayHit(bool flag)
	{
		if (!_dp->mRayHitObjVec.empty())
		{
			for (VegTestHelper* it : _dp->mRayHitObjVec)
				delete it;
			_dp->mRayHitObjVec.clear();
		}

		if (flag && _dp->mHandle && _dp->mHandle->getVegHandleForUI())
		{
			static uint32 _MaxShowNum = 1000u;
			static float _RayLen = 10.f;

			Vector3 rayDir = Vector3::UNIT_Y;
			std::vector<Vector3> begP, endP;
			_dp->mHandle->getVegHandleForUI()->getRecordData(begP, rayDir, endP);

			std::vector<Vector3> rayLine;
			for (size_t i = 0; i < begP.size(); i++)
			{
				rayLine.push_back(begP.at(i));
				rayLine.push_back(begP.at(i) + rayDir * _RayLen);
			}

			auto divideShow = [&](
				const std::vector<Vector3>& pVec,
				VegTestHelper::Model model, const Vector4& color)
				{
					for (size_t beg_i = 0; beg_i < pVec.size(); beg_i += _MaxShowNum)
					{
						size_t end_i = beg_i + _MaxShowNum;
						if (end_i > pVec.size())end_i = pVec.size();
						std::vector<Vector3> tempVec(pVec.begin() + beg_i, pVec.begin() + end_i);
						_dp->mRayHitObjVec.push_back(new VegTestHelper(tempVec, model, color));
					}
				};

			divideShow(begP, VegTestHelper::Model::Point, VegTestHelper::R);
			divideShow(endP, VegTestHelper::Model::Point, VegTestHelper::G);
			divideShow(rayLine, VegTestHelper::Model::Line, VegTestHelper::B);
		}
	}
#undef _SafeDelete
}
