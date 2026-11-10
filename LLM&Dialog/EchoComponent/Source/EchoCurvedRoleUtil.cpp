///////////////////////////////////////////////////////////////////////////////
//
// @file EchoCurvedRoleUtil.cpp
// @author leixu	
// @date 2024/2/28
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
#include "EchoComponentStableHeaders.h"
#include "EchoCurvedRoleUtil.h"

namespace Echo
{
	CurvedRoleUtil::CurvedRoleUtil(Echo::SceneManager* sMgr)
	{
		mSceneManager = sMgr;
	}

	CurvedRoleUtil::~CurvedRoleUtil()
	{
		if (mCurveLine)
		{
			delete mCurveLine;
			mCurveLine = nullptr;
		}

		if(mNode)
		{
			mNode->detachAllObject();
			mSceneManager->destroySceneNode(mNode);
			mNode = nullptr;
		}
	}

	void CurvedRoleUtil::updateHelper(PiecewiseTool::ActionType action, const Vector3& point, const Vector3& offset, size_t index, bool isVisiable)
	{
			if(action == PiecewiseTool::CREATE_ACTION)
			{
				BoxHelper* _box = new BoxHelper(mSceneManager);
				_box->setBox(0.5f, 0.5f, 0.5f);
				_box->setPosition(point + offset);
				_box->setColor(Echo::ColorValue(0.0f, 1.f, 0.0f, 0.5f));
				_box->setVisible(isVisiable);
				mPointHelpers.push_back(_box);
				mNode->addChild(_box->GetSceneNode());
				if (mCurveLine != nullptr)
				{
					mCurveLine->setVisible(isVisiable);
				}
				if (mNode)
					mNode->_update(true, true);
			}
	}

	void CurvedRoleUtil::DefaultHelper(Vector3 pos)
	{
		if (mSceneManager == nullptr)
			return;

		if (mNode == nullptr)
		{
			mNode = mSceneManager->createSceneNode();
			mNode->setPosition(pos);
		}
		

		if (mCurveLine == nullptr)
		{
			mCurveLine = new TrafficObject(mSceneManager->generateMovableName());
			mNode->attachObject(mCurveLine);
		}

		clearHelper();
	}

	void CurvedRoleUtil::clearHelper()
	{
		if (mNode != nullptr)
			mNode->removeAllChildren();

		for (auto helper : mPointHelpers)
			delete helper;
		mPointHelpers.clear();

		if (mNode)
			mNode->_update(true, true);
	}

	void CurvedRoleUtil::UpdateCurveLine(Curve* curve)
	{
		if (curve == nullptr || mCurveLine == nullptr)
			return;

		mAABB.setNull();
		Echo::vector<Echo::Vector3>::type points;
		points.clear();
		Echo::uint32 size = curve->getNumPoints();
		for (size_t i = 0; i < size; i++)
		{
			uint32 index = uint32(i);
			points.emplace_back(curve->getPoint(index));
		}
		mCurveLine->clear();

		mCurveLine->begin("Helper_VS.txt", "Helper_PS.txt", Echo::ECHO_LINE_STRIP);
		{
			for (int i = 0; i < (int)(size - 1); ++i)
			{
				float fAllDis = points[i + 1].distance(points[i]);
				float fCurDis = 0.f;
				while (fCurDis < fAllDis)
				{
					Echo::Vector3 curPos = curve->interpolate(i, fCurDis / fAllDis);
					mCurveLine->position(curPos);
					fCurDis += 0.1f;
					mAABB.merge(curPos);
				}
				if (i == size - 2)
				{
					Echo::Vector3 curPos = curve->interpolate(i, 1.0f);
					mCurveLine->position(curPos);
					mAABB.merge(curPos);
				}

			}
			mCurveLine->colour(Echo::ColorValue::Red);
			mCurveLine->getSection(0)->setUsetMaterialDiffuse(true);
			mCurveLine->getSection(0)->setUseDepthWrite(false);
			mCurveLine->getSection(0)->generateShaderMask();
		}
		mCurveLine->end();
		if (mNode)
		{
			mNode->needUpdateBounds();
			mNode->_update(true, true);
		}
	}

	void CurvedRoleUtil::updatePointer(size_t index)
	{

	}

	bool CurvedRoleUtil::PickHelper(const Ray& ray, size_t& pickIndex)
	{
		ControlHelperRenderable* _helper = nullptr;
		float closest_distance = std::numeric_limits<float>::infinity();
		for (auto helper : mPointHelpers)
		{
			auto res = helper->hitTest(ray);
			if (res.first && res.second < closest_distance)
			{
				_helper = helper;
				closest_distance = res.second;
				pickIndex = std::find(mPointHelpers.begin(), mPointHelpers.end(), _helper) - mPointHelpers.begin();
			}
		}
		return pickIndex != mPointHelpers.size();
	}

	void CurvedRoleUtil::setVisible(bool isVisible)
	{
		if (mCurveLine != nullptr)
		{
			mCurveLine->setVisible(isVisible);
		}

		for (auto helper : mPointHelpers)
		{
			helper->setVisible(isVisible);
		}

		//if (mNode)
		//	mNode->_update(true, true);
	}

	bool CurvedRoleUtil::isVisible()
	{
		if (mCurveLine != nullptr)
		{
			for (auto helper : mPointHelpers)
			{
				if (helper->getVisible())
					return true;
			}
		}
		return false;
	}

	void CurvedRoleUtil::setHighlightPoint(const Ray& ray)
	{
		ControlHelperRenderable* _helper = nullptr;
		float closest_distance = std::numeric_limits<float>::infinity();
		for (auto helper : mPointHelpers)
		{
			helper->setHighlight(false);
			auto res = helper->hitTest(ray);
			if (res.first && res.second < closest_distance)
			{
				_helper = helper;
				closest_distance = res.second;
			}
		}
		if (_helper != nullptr)
			_helper->setHighlight(true);
	}

	void CurvedRoleUtil::setHelperPosition(size_t index, Vector3 point)
	{
		if (index > mPointHelpers.size() - 1)
			return;
		mPointHelpers[index]->setPosition(point);
	}

	void CurvedRoleUtil::updateNode()
	{
		if (mNode)
		{
			mNode->_update(true, true);
		}
	}

	void CurvedRoleUtil::keepScale(const Vector3& cam_pos)
	{
		for (size_t i = 0; i < mPointHelpers.size(); ++i)
		{
			auto& box = mPointHelpers[i];
			auto& point = box->getWorldPosition();
			float dist = cam_pos.distance(point);
			dist *= 0.05f;
			if (box->getScale() != dist)
				box->setScale(dist);
		}
	}
}
