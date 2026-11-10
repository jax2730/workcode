#include "EchoComponentStableHeaders.h"
#include "EchoIKTargetPointComponent.h"
#include "EchoBoxComponent.h"
#include "EchoSkeletonInstance.h"
#include "IK/EchoIKSkeletonPosture.h"
#include "EchoResourceFolderManager.h"
#include "EchoBoneRenderComponent.h"

namespace Echo
{

	DECLAREACTORCOMTYPE_CPP(IKTargetPointComponent)
	
	IKTargetPointComponentInfo::IKTargetPointComponentInfo(const Name& mtype, ActorCOMMoveableType movableType)
		: IKPointComponentInfo(mtype, movableType)
	{
	}
	IKTargetPointComponentInfo::~IKTargetPointComponentInfo()
	{
	}

	EDITOR_REFLECT_BEGIN(IKTargetPointComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_SkeletonPostureName, "SkeletonPostureName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_IsUseSkpeTransform, "IsUseSkpeTransform", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()
	_DEFINEINFO_VIRFUN(IKTargetPointComponentInfo, IKPointComponentInfo)

	IKTargetPointComponent::IKTargetPointComponent(ActorPtr pActor, ActorCOMMoveableType movableType) : IKPointComponent(pActor, movableType){}

	IKTargetPointComponent::~IKTargetPointComponent(){}

	void IKTargetPointComponent::init(ActorComponentInfo * info)
	{
		if (info == nullptr) return;
		IKPointComponent::init(info);
		SetIsUseSkpeTransform(static_cast<IKTargetPointComponentInfo*>(info)->GetIsUseSkpeTransform());
		SetSkeletonPostureName(static_cast<IKTargetPointComponentInfo*>(info)->GetSkeletonPostureName());
	}
	void IKTargetPointComponent::unInit()
	{
		IKPointComponent::unInit();
		mSkeletonPosture.setNull();
	}
	void IKTargetPointComponent::replay(ActorComponentInfo * info) {
		if (info == nullptr) return;
		IKPointComponent::replay(info);
		SetIsUseSkpeTransform(static_cast<IKTargetPointComponentInfo*>(info)->GetIsUseSkpeTransform());
		SetSkeletonPostureName(static_cast<IKTargetPointComponentInfo*>(info)->GetSkeletonPostureName());
	}
	void IKTargetPointComponent::copyTo(const ActorComponentPtr& actCom) {
		if (!actCom) return;
		IKPointComponent::copyTo(actCom);
		auto other = actCom.getStaticCastWeakPtr<IKTargetPointComponent>();
		other->SetIsUseSkpeTransform(m_IsUseSkpeTransform);
		other->SetSkeletonPostureName(m_SkeletonPostureName);
	}
	void IKTargetPointComponent::connectCom() {
		IKPointComponent::connectCom();
	}

	void IKTargetPointComponent::disconnectCom() {
		IKPointComponent::disconnectCom();
	}
	const std::map<Name, Quaternion>& IKTargetPointComponent::GetSkeletonPosture() {
		if (mSkeletonPosture.isNull()) {
			static std::map<Name, Quaternion> nullMap;
			return nullMap;
		}
		return mSkeletonPosture->getSkeletonPosture();
	}
	FBBEffectorType IKTargetPointComponent::GetIKPartType() {
		if (mSkeletonPosture.isNull()) {
			return FBBEffectorType::NumEffectors;
		}
		return mSkeletonPosture->getIKPartType();
	}

	void IKTargetPointComponent::SetSkpeCallBack(SkpeCallBack _OnSetSkpe, SkpeCallBack _OnResetSkpe) {
		if (OnResetSkpe != nullptr) {
			OnResetSkpe(this);
		}
		OnResetSkpe = _OnResetSkpe;
		OnSetSkpe = _OnSetSkpe;
		if (OnSetSkpe != nullptr) {
			OnSetSkpe(this);
		}
	}

	void IKTargetPointComponent::AsyncLoadSkeletonPostureCallback(IKTargetPointComponentPtr& _thisPtr, EchoIK::IKSkeletonPostureBasePtr& resource) {
		if (_thisPtr.expired() || _thisPtr->mSkeletonPosture == resource) return;

		if (_thisPtr->OnResetSkpe != nullptr) {
			_thisPtr->OnResetSkpe(_thisPtr.getPointer());
		}

		_thisPtr->mSkeletonPosture = resource;
		if (_thisPtr->mSkeletonPosture.isNull()) {
#ifdef USE_IK_HELPEROBJ
			_thisPtr->UpdateHelper();
#endif // USE_IK_HELPEROBJ
			return;
		}

		if (_thisPtr->m_IsUseSkpeTransform && (_thisPtr->mSkeletonPosture->getName() != _thisPtr->mSkeletonPosture->getTemplateName())) {

			_thisPtr->setPosition(((EchoIK::IKSkeletonPosture*)_thisPtr->mSkeletonPosture.get())->getLocalPosition());
			Quaternion quat;
			quat.FromEuler(((EchoIK::IKSkeletonPosture*)_thisPtr->mSkeletonPosture.get())->getLocalEulerAngle());
			_thisPtr->setRotation(quat);

			if (Root::instance()->getRuntimeMode() == Root::eRM_Editor && _thisPtr->m_pActComInfo != nullptr) {
				_thisPtr->m_pActComInfo->SetPosition(((EchoIK::IKSkeletonPosture*)_thisPtr->mSkeletonPosture.get())->getLocalPosition());
				_thisPtr->m_pActComInfo->SetEulerAngles(((EchoIK::IKSkeletonPosture*)_thisPtr->mSkeletonPosture.get())->getLocalEulerAngle());
			}
		}

		if (_thisPtr->OnSetSkpe != nullptr) {
			_thisPtr->OnSetSkpe(_thisPtr.getPointer());
		}
#ifdef USE_IK_HELPEROBJ
		_thisPtr->UpdateHelper();
#endif // USE_IK_HELPEROBJ
	}

	void IKTargetPointComponent::SetSkeletonPostureName(const Name& value) {
		if (m_SkeletonPostureName == value) return;
		m_SkeletonPostureName = value;
		if (m_SkeletonPostureName.empty()) {
			IKTargetPointComponentPtr _thisPtr = getWeakPtr<IKTargetPointComponent>();
			EchoIK::IKSkeletonPostureBasePtr mSkeletonPostureNull;
			AsyncLoadSkeletonPostureCallback(_thisPtr, mSkeletonPostureNull);
		}
		else {
			IKTargetPointComponentPtr _thisPtr = getWeakPtr<IKTargetPointComponent>();
			EchoIK::IKSkeletonPostureManager::instance()->AsyncLoad(m_SkeletonPostureName, std::bind<void>(&IKTargetPointComponent::AsyncLoadSkeletonPostureCallback, _thisPtr, std::placeholders::_1));
		}
	}
	const Name& IKTargetPointComponent::GetSkeletonPostureName() {
		return m_SkeletonPostureName;
	}

	void IKTargetPointComponent::SetIsUseSkpeTransform(bool value) {
		m_IsUseSkpeTransform = value;
	}
	bool IKTargetPointComponent::GetIsUseSkpeTransform() {
		return m_IsUseSkpeTransform;
	}

#ifdef USE_IK_HELPEROBJ
	void IKTargetPointComponent::UpdateHelper() {
		if (!m_pHelperObj || !m_pHelperObj->getVisible()) return;

		if (mSkeletonPosture.isNull()) {
			IKPointComponent::UpdateHelper();
			return;
		}
		m_pHelperObj->setScale(Vector3::UNIT_SCALE);
		std::vector<Vector3> vbVec;
		std::vector<uint16> ibVec;
		std::vector<uint32> colors;
		if (BoneRenderComponent::gVB.empty()) { BoneRenderComponent::GenerateRenderBuffer(); }

		auto DrawCone = [&](
			std::vector<Vector3>& vbVec,
			std::vector<uint16>& ibVec,
			std::vector<uint32>& colors,
			const Transform& selfWorldTransform,
			const Vector3& parentWorldPosition) {

				uint16 startIB = (uint16)vbVec.size();
				float width = BoneRenderComponent::gBoneWidth;
				const std::vector<Vector3>& tVB = BoneRenderComponent::gVB;
				const std::vector<uint16>& tIB = BoneRenderComponent::gIB;
				const std::vector<uint32>& tColors = BoneRenderComponent::gColors;

				Vector3 tarAxis = selfWorldTransform.translation - parentWorldPosition;
				float length = tarAxis.Length();
				Vector3 parPos = selfWorldTransform.rotation.Inverse() * (parentWorldPosition - selfWorldTransform.translation);

				Quaternion quat;
				parPos.Normalize();
				Vector3 offset = parPos - Vector3::DOWN;
				if (offset.Length() > 0.0001f) {
					quat.FromAxesToAxes(Vector3::UNIT_Y, parPos);
				}
				else {
					quat = { 1.0f,0.0f,0.0f,0.0f };
				}
				quat = selfWorldTransform.rotation * quat;
				for (int i = 0; i != tVB.size(); ++i) {
					Vector3 temp(tVB[i].x * width, tVB[i].y * length, tVB[i].z * width);
					temp = quat * temp + selfWorldTransform.translation;
					vbVec.push_back(temp);
				}
				for (int i = 0; i != tIB.size(); ++i) {
					ibVec.push_back(tIB[i] + startIB);
				}
				for (int i = 0; i != tColors.size(); ++i) {
					colors.push_back(tColors[i]);
				}

		};

		std::vector<std::tuple<Name, Vector3, Quaternion, uint32>> posture;
		mSkeletonPosture->getSkeletonPosture(posture);
		if (posture.empty()) {
			IKPointComponent::UpdateHelper();
			return;
		}
		std::vector<Transform> transforms(posture.size() + 1);
		transforms[0] = Transform::IDENTITY;
		for (int i = 0; i != posture.size(); ++i) {
			Transform& parentTransform = transforms[std::get<3>(posture[i])];
			transforms[i + 1].translation = parentTransform.rotation * std::get<1>(posture[i]) + parentTransform.translation;
			transforms[i + 1].rotation = parentTransform.rotation * std::get<2>(posture[i]);
			DrawCone(vbVec, ibVec, colors, transforms[i + 1], parentTransform.translation);
		}
		m_pHelperObj->changeCustomPolygon(vbVec, ibVec, colors);
	}
#endif // USE_IK_HELPEROBJ

}
