///////////////////////////////////////////////////////////////////////////////
//
// @file EchoCurvedRoleUtil.h
// @author leixu	
// @date 2024/2/28
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef __ECHO_CURVED_ROLE_UTIL_H__
#define __ECHO_CURVED_ROLE_UTIL_H__
#include "EchoComponentPrerequisites.h"
#include "EchoName.h"
#include "ControlHelperRenderable.h"
#include "EchoMatrix3.h"
#include "EchoSceneNode.h"
#include "EchoPiecewiseBezierComponent.h"
#include "EchoRay.h"
#include "EchoVector3.h"
#include "EchoRoot.h"
#include "EchoCurve.h"
namespace Echo
{
	class _EchoComponentExport CurvedRoleUtil
	{
		
	public:
		CurvedRoleUtil(Echo::SceneManager* sMgr);
		~CurvedRoleUtil();
		void updateHelper(PiecewiseTool::ActionType action,const Vector3& point, const Vector3& offset, size_t index, bool isVisiable);
		void UpdateCurveLine(Curve* curve);
		void DefaultHelper(Vector3 pos = Vector3::ZERO);
		void clearHelper();
		void updatePointer(size_t index = -1);
		bool PickHelper(const Ray& ray, size_t& pickIndex);
		void setVisible(bool isVisible);
		bool isVisible();
		void setHighlightPoint(const Ray& ray);
		void setHelperPosition(size_t index, Vector3 point);
		Echo::SceneNode* getSceneNode() { return mNode; }
		TrafficObject* getCurveLine() { return mCurveLine; }
		void updateNode();
		AxisAlignedBox getBox() { return mAABB; }

		void keepScale(const Vector3& cam_pos);
	private:
		TrafficObject*									mCurveLine = nullptr;
		std::vector<EchoBox::BoxHelper*>				mPointHelpers;
		Echo::SceneNode*								mNode = nullptr;
		AxisAlignedBox									mAABB;
		Echo::SceneManager*								mSceneManager = nullptr;
	};

}
#endif // !__ECHO_CURVED_ROLE_UTIL_H__
