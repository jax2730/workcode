//-----------------------------------------------------------------------------
// File:   EchoBoxColliderComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-7-27
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoBoxColliderComponent_h__ 
#define __EchoBoxColliderComponent_h__

#include "EchoColliderComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoComInfoDefineMacro.h"

namespace Echo {

	struct BoxColliderComponentData : public ColliderComponentData {
		Vector3 halfSize = { 0.5f, 0.5f, 0.5f };
	};

	class _EchoComponentExport BoxColliderComponent : public NodeColliderComponent {
	public:
		DECLAREACTORCOMTYPE_H(BoxColliderComponent);
		REGISTER_LUA_CLASS_KEY;
	private:
		void GenerateVBIBCB(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec) override;
		EchoGeometry* GenerateGeometry(void* buffer) override;
	protected:
		virtual void ResetHelperColor() override;
	public:
		void SetHalfSize(const Vector3& size);
		const Vector3& GetHalfSize();
		virtual void SetCompData(ColliderComponentData* pData) override;
		virtual bool IsCanPhysics() override;
		Vector3 GetRealHalfSize();
	};

	class _EchoComponentExport BoxColliderComponentInfo : public ColliderComponentInfo {
	public:
		BoxColliderComponentInfo() {}
		BoxColliderComponentInfo(const Name& comType, ActorCOMMoveableType MoveType);
		virtual ~BoxColliderComponentInfo();
		virtual void CopyToData(ColliderComponentData* pData) override;
		_DECLCOMINFO_VIRFUN
	
	public://´æ´¢Êý¾Ý
		_DECLCOMINFO_PARAM_BEGIN(BoxColliderComponent)
		_DECLCOMINFO_PARAM(Vector3, HalfSize, Vector3(0.5f, 0.5f, 0.5f), SetHalfSize)
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(BoxColliderComponent, ALL_MOVEABLE)


}



#endif /*__EchoBoxColliderComponent_h__*/
