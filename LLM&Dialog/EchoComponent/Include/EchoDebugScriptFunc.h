//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef __EchoDebugScriptFunc_H__
#define __EchoDebugScriptFunc_H__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "EchoProtocolComponentImp.h"
#include "EchoEntityObjProFunctionImp.h"
#include "EchoManualObject.h"
#include "Actor/EchoNodeComponent.h"


//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	static EntityObjMap mEntityObjMap;
	static ManualObject* mHelperObj;
	class EchoDebugScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;
		static void								Script_DebugInt(int value);
		static void								Script_DebugFloat(float value);
		static void								Script_DebugString(const std::string& value);
		static void								Script_DebugVector3(Vector3 value);
		static void								Script_DebugQuaternion(Quaternion value);
		static void								Script_DebugBool(bool value);
		static void								Script_DebugColorValue(ColorValue value);
		// 没有 transform 组件， 暂存于此 物体绕局部坐标系的 xyz 轴进行旋转的函数
		static void								RotateAround(const EchoActorComponentScript& p_com, const Vector3& axis, float angle);
		//没有3DUI组件，打字机函数暂存于此
		static float							CurrentTime();

		//道路生成组件
		//static bool								CreateEntity(std::string path, Vector3 pos, int idx);
		//static bool								DeleteEntity();					
		////运动轨迹脚本函数
		//static void								ShowTrajectory(Actor* m_pActor, TimelineComponent* TimeLineCom, bool isChange);
	};
}

#endif