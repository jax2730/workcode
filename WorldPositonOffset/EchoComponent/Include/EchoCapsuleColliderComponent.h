//-----------------------------------------------------------------------------
// File:   EchoCapsuleColliderComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-7-27
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _EchoCapsuleColliderComponent_h__
#define _EchoCapsuleColliderComponent_h__

#include "EchoColliderComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoComInfoDefineMacro.h"
namespace Echo
{
	struct CapsuleColliderComponentData : public ColliderComponentData {
		float radius = 0.5f;	//半径
		float height = 2.0f;	//总高
	};
	

	class _EchoComponentExport CapsuleColliderComponent : public NodeColliderComponent
	{
	public:
		DECLAREACTORCOMTYPE_H(CapsuleColliderComponent)
		REGISTER_LUA_CLASS_KEY
	private:
		virtual void			GenerateVBIBCB(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec)	override;
		virtual EchoGeometry*	GenerateGeometry(void* buffer)		override;
	protected:
		virtual void ResetHelperColor() override;
	public:
		float GetRadius();
		float GetHeight();
		void SetRadius(float radius);
		void SetHeight(float height);
		void SetDirectionX();
		void SetDirectionY();
		void SetDirectionZ();
		virtual void SetCompData(ColliderComponentData* pData) override;
		virtual bool IsCanPhysics() override;
		void GetRealRadiusHalfHeight(float& radius, float& halfHeight);
	};
	

	class _EchoComponentExport CapsuleColliderComponentInfo : public ColliderComponentInfo{
	public:
		CapsuleColliderComponentInfo() {}
		CapsuleColliderComponentInfo(const Name& comType, ActorCOMMoveableType MoveType);
		~CapsuleColliderComponentInfo();
		virtual void CopyToData(ColliderComponentData* pData) override;
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(CapsuleColliderComponent)
		_DECLCOMINFO_PARAM_SIM(float, Radius, 0.5f, SetRadius)// 半径
		_DECLCOMINFO_PARAM_SIM(float, Height, 2.0f, SetHeight)// 整体高度
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(CapsuleColliderComponent, ALL_MOVEABLE)


}; // end Echo

#endif // EchoCapsuleCcolliderComponent._h__

