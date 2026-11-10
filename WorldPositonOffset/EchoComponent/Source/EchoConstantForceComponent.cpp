#include "EchoComponentStableHeaders.h"
#include "EchoConstantForceComponent.h"
#include "Effect/EchoEffectHolder.h"
#include "EchoActorProFuncImp.h"
#include "EchoPhysXAsset.h"
#include "Actor/EchoNodeComponent.h"

namespace Echo {

DECLAREACTORCOMTYPE_CPP(ConstantForceComponent)


EDITOR_REFLECT_BEGIN(ConstantForceComponentInfo)
EDITOR_REFLECT_MEMBER_UPDATE(m_Force, "Force", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_RelativeForce, "RelativeForce", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_Torque, "Torque", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_RelativeTorque, "RelativeTorque", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_ForceMode, "ForceMode", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_END();


	ConstantForceComponentInfo::ConstantForceComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) : ActorComponentInfo(comType, MoveType) {}
	ConstantForceComponentInfo::~ConstantForceComponentInfo() {}

	_DEFINEINFO_VIRFUN_BASE(ConstantForceComponentInfo)

	void ConstantForceComponentInfo::CopyToCompData(ConstantForceComponentData* pData) {
		if (!pData) return;
		pData->Force = m_Force;
		pData->RelativeForce = m_RelativeForce;
		pData->Torque = m_Torque;
		pData->RelativeTorque = m_RelativeTorque;
		pData->ForceMode = m_ForceMode;
	}

	ConstantForceComponent::ConstantForceComponent(ActorPtr pActor, ActorCOMMoveableType type) : PhysicsBaseComponent(pActor, type){}
	ConstantForceComponent::~ConstantForceComponent() {}

	void ConstantForceComponent::init(ActorComponentInfo* info) { 
		PhysicsBaseComponent::init(info);
		ConstantForceComponentInfo* pInfo = (ConstantForceComponentInfo*)info;
		if (pInfo) {
			pInfo->CopyToCompData(&mCompData);
		}
	}
	void ConstantForceComponent::unInit() {
		PhysicsBaseComponent::unInit();
	}
	void ConstantForceComponent::connectCom() {
		PhysicsBaseComponent::connectCom();
		ChangeRigidBody();
	}
	void ConstantForceComponent::disconnectCom() {
		m_pAttachRigidBodyComp = nullptr;
		PhysicsBaseComponent::disconnectCom();
	}
	void ConstantForceComponent::replay(ActorComponentInfo* info) {
		ConstantForceComponentInfo* pInfo = (ConstantForceComponentInfo*)info;
		if (pInfo) {
			pInfo->CopyToCompData(&mCompData);
		}
	}
	void ConstantForceComponent::copyTo(const ActorComponentPtr& actCom) {
		if (!actCom) return;
		actCom.getDynamicCastWeakPtr<ConstantForceComponent>()->SetCompData(&mCompData);
	}

	void ConstantForceComponent::ChangeRigidBody() {
		m_pAttachRigidBodyComp = getParentNodeCom()->getComponentByType<RigidBodyComponent>();
	}

	void ConstantForceComponent::CreatePhysics() {
		m_pAttachRigidBodyComp = getParentNodeCom()->getComponentByType<RigidBodyComponent>();
	}
	void ConstantForceComponent::ReleasePhysics() {
		m_pAttachRigidBodyComp = nullptr;
	}
	void ConstantForceComponent::ApplyPhysics() {
		if (m_pAttachRigidBodyComp == nullptr) return;
		if (!mCompData.Force.IsZero())m_pAttachRigidBodyComp->AddForce(mCompData.Force, mCompData.ForceMode);
		if (!mCompData.RelativeForce.IsZero())m_pAttachRigidBodyComp->AddRelativeForce(mCompData.RelativeForce, mCompData.ForceMode);
		if (!mCompData.Torque.IsZero())m_pAttachRigidBodyComp->AddTorque(mCompData.Torque, mCompData.ForceMode);
		if (!mCompData.RelativeTorque.IsZero())m_pAttachRigidBodyComp->AddRelativeTorque(mCompData.RelativeTorque, mCompData.ForceMode);
	}
	bool ConstantForceComponent::IsCanPhysics() {
		return getParentNodeCom()->getComponentByType<RigidBodyComponent>() != nullptr;
	}
	bool ConstantForceComponent::IsExistPhysics() {
		return m_pAttachRigidBodyComp != nullptr;
	}

	void ConstantForceComponent::SetForce(const Vector3& force) {
		mCompData.Force = force;
	}
	void ConstantForceComponent::SetRelativeForce(const Vector3& force) {
		mCompData.RelativeForce = force;
	}
	void ConstantForceComponent::SetTorque(const Vector3& torque) {
		mCompData.Torque = torque;
	}
	void ConstantForceComponent::SetRelativeTorque(const Vector3& torque) {
		mCompData.RelativeTorque = torque;
	}
	void ConstantForceComponent::SetForceMode(int value) {
		mCompData.ForceMode = value;
	}

	void ConstantForceComponent::GetForce(Vector3& force) {
		force = mCompData.Force;
	}
	void ConstantForceComponent::GetRelativeForce(Vector3& force) {
		force = mCompData.RelativeForce;
	}
	void ConstantForceComponent::GetTorque(Vector3& torque) {
		torque = mCompData.Torque;
	}
	void ConstantForceComponent::GetRelativeTorque(Vector3& torque) {
		torque = mCompData.RelativeTorque;
	}
	const Vector3& ConstantForceComponent::GetForce(){
		return mCompData.Force;
	}
	const Vector3& ConstantForceComponent::GetRelativeForce(){
		return mCompData.RelativeForce;
	}
	const Vector3& ConstantForceComponent::GetTorque(){
		return mCompData.Torque;
	}
	const Vector3& ConstantForceComponent::GetRelativeTorque(){
		return mCompData.RelativeTorque;
	}
	int ConstantForceComponent::GetForceMode() {
		return mCompData.ForceMode;
	}
	void ConstantForceComponent::SetCompData(ConstantForceComponentData* pData) {
		if (!pData) return;
		SetForce(pData->Force);
		SetRelativeForce(pData->RelativeForce);
		SetTorque(pData->Torque);
		SetRelativeTorque(pData->RelativeTorque);
		SetForceMode(pData->ForceMode);
	}
	const ConstantForceComponentData* ConstantForceComponent::GetCompData() {
		return &mCompData;
	}


	void EchoConstantForceProFuncImp::initProtocolFunction() {
		//REGISTEROMFACTORY(ConstantForceComponent);
	}
	void EchoConstantForceProFuncImp::uninitProtocolFunction() {
		
	}


}
