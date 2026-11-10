#ifndef __ECHOHITREACTIONCOMPONENT_H__
#define __ECHOHITREACTIONCOMPONENT_H__

#include "EchoIKBaseComponent.h"
#include "EchoRole.h"
#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"

namespace Echo
{
	struct IKHitReactionComponentData : public IKBaseComponentData {
		HitReactionInfo mIKInfo;
		FinalIKInfoBase* GetIKInfo() override { return &mIKInfo; }
	};

	class _EchoComponentExport IKHitReactionComponent : public IKBaseComponent{
		DECLAREACTORCOMTYPE_H(IKHitReactionComponent)
			REGISTER_LUA_CLASS_KEY;
	public:
		virtual void SetIKInfo(FinalIKInfoBase* pInfo) override;
		virtual void UseDefaultInitialize() override;
		virtual bool IsCanCreate() override;
		void SetIKInfoData(const HitReactionInfo& info);
		const HitReactionInfo& GetIKInfoData();
		inline IKBaseComponentData* CreateCompData() override
		{
			return new IKHitReactionComponentData();
		}
	public:
		void SetFullBodyBipedIKName(const Name& ikName);
		const Name& GetFullBodyBipedIKName();
		const std::vector<HitReaHitPointEffectorInfo>& GetEffectorHitPoints();
		void SetEffectorHitPoints(const std::vector<HitReaHitPointEffectorInfo>& value);
		const std::vector<HitReaHitPointBoneInfo>& GetBoneHitPoints();
		void SetBoneHitPoints(const std::vector<HitReaHitPointBoneInfo>& value);

		EchoIK::FullBodyBipedIK* GetFullBodyBipedIK();

		void Hit(const Name& colliderName, const Vector3& force, const Vector3& point);
	protected:
		void PostCreateIK() override;
		void setEffCollider();
		void setBoneColliderAndBone();
	};

	class _EchoComponentExport IKHitReactionComponentInfo : public IKBaseComponentInfo {
	public:
		IKHitReactionComponentInfo();
		IKHitReactionComponentInfo(const Name& comType, ActorCOMMoveableType moveable);

	public:
		virtual ~IKHitReactionComponentInfo();
		virtual void CopyToCompData(IKBaseComponentData* pData) override;
		virtual void CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp) override;

		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(IKHitReactionComponent)
		_DECLCOMINFO_PARAM(Name, FullBodyBipedIKName, Name::g_cEmptyName, SetFullBodyBipedIKName)
		_DECLCOMINFO_PARAM(std::vector<HitReaHitPointEffectorInfo>, EffectorHitPoints, {}, SetEffectorHitPoints);
		_DECLCOMINFO_PARAM(std::vector<HitReaHitPointBoneInfo>, BoneHitPoints, {}, SetBoneHitPoints);
		_DECLCOMINFO_PARAM_END()
	};//Class IKHitReactionComponentInfo

	DEFINECOMFACTORY(IKHitReactionComponent, UNMOVEABLE)

}//Namespace echo
#endif


