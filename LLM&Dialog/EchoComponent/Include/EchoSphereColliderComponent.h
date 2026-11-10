//-----------------------------------------------------------------------------
// File:   EchoSphereColliderComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-7-27
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef EchoSphereColliderComponent_h__
#define EchoSphereColliderComponent_h__

#include "EchoColliderComponent.h"
#include "EchoCapsuleColliderComponent.h"
#include "EchoComDefineMacro.h"
namespace Echo
{

	

	struct SphereColliderComponentData : public ColliderComponentData {
		float radius = 1.0f;
	};

	class _EchoComponentExport SphereColliderComponent : public NodeColliderComponent
	{
	public:
		DECLAREACTORCOMTYPE_H(SphereColliderComponent)
		REGISTER_LUA_CLASS_KEY;
	private: 
		virtual void			GenerateVBIBCB(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec)	override;
		virtual EchoGeometry*	GenerateGeometry(void* buffer)		override;
	protected:
		virtual void ResetHelperColor() override;
	public:
		float GetRadius();
		void SetRadius(float radius);
		virtual void SetCompData(ColliderComponentData* pData) override;
		virtual bool IsCanPhysics() override;
		float GetRealRadius();
	};
	
	class _EchoComponentExport SphereColliderComponentInfo : public ColliderComponentInfo{
	public:
		SphereColliderComponentInfo() {}
		SphereColliderComponentInfo(const Name& comType, ActorCOMMoveableType MoveType);
		~SphereColliderComponentInfo();
		virtual void CopyToData(ColliderComponentData* pData) override;
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(SphereColliderComponent)
		_DECLCOMINFO_PARAM_SIM(float, Radius, 0.5f, SetRadius)
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(SphereColliderComponent,ALL_MOVEABLE)

}// end namespace Echo 
#endif // EchoSphereColliderComponent_h__
