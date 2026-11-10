#include "EchoComponentStableHeaders.h"
#include "EchoIKRecoilComponent.h"
#include "IK/EchoRecoil.h"
#include "IK/EchoAimIK.h"

namespace Echo {

	static const std::vector<IKRecoilInfo::Offsets>& GetDefaultRecoilOffset()
	{
		/*static bool inite = false;
		static EchoIK::Recoil::RecoilOffset::EffectorLink resEffectorLink1;
		static EchoIK::Recoil::RecoilOffset::EffectorLink resEffectorLink2;
		static EchoIK::Recoil::RecoilOffset::EffectorLink resEffectorLink3;
		static EchoIK::Recoil::RecoilOffset resOffset;
		static std::vector<EchoIK::Recoil::RecoilOffset> res;

		if (inite == false) {
			resEffectorLink1.effector = EchoIK::FullBodyBipedEffector::RightHand;
			resEffectorLink1.weight = 1.0f;

			resEffectorLink2.effector = EchoIK::FullBodyBipedEffector::RightShoulder;
			resEffectorLink2.weight = 0.3f;

			resEffectorLink3.effector = EchoIK::FullBodyBipedEffector::Body;
			resEffectorLink3.weight = 0.1f;

			resOffset.offset = Vector3(0.02f, 0.0f, -0.3f);
			resOffset.additivity = 0.5f;
			resOffset.maxAdditiveOffsetMag = 0.2f;
			resOffset.effectorLinks.push_back(resEffectorLink1);
			resOffset.effectorLinks.push_back(resEffectorLink2);
			resOffset.effectorLinks.push_back(resEffectorLink3);

			res.push_back(resOffset);

			inite = true;
		}

		return res;*/
		static bool inite = false;
		static std::vector<IKRecoilInfo::Offsets> gDefault;
		if (!inite) {
			gDefault.emplace_back();
			IKRecoilInfo::Offsets& data = gDefault.back();

			EffectorLinkInfo resEffectorLink1;
			resEffectorLink1.effector = EchoIK::FullBodyBipedEffector::RightHand;
			resEffectorLink1.weight = 1.0f;

			data.offset = Vector3(0.0f, 0.0f, -0.5f);
			data.additivity = 1.0f;
			data.maxAdditiveOffsetMag = 0.5f;
			data.effectorLinks.push_back(resEffectorLink1);

			inite = true;
		}
		return gDefault;
	}

	static const std::vector<Vector2>& GetDefaultRecoilWeight()
	{
		static std::vector<Vector2> gDefault{ {0.0f,0.0f},{0.15f,0.3f},{0.3f,0.0f} };
		return gDefault;
	}

	static void TransInfoEffToIKEff(const EffectorLinkInfo& infoEff, EchoIK::Recoil::RecoilOffset::EffectorLink& ikEff)
	{
		ikEff.effector = EchoIK::FullBodyBipedEffector(infoEff.effector);
		ikEff.weight = infoEff.weight;
	}

	static void TransIKEffToInfoEff(EffectorLinkInfo& infoEff, const EchoIK::Recoil::RecoilOffset::EffectorLink& ikEff)
	{
		infoEff.effector = ikEff.effector;
		infoEff.weight = ikEff.weight;
	}

	static void TransIKOffToInfoOff(IKRecoilInfo::Offsets& infoOffsets, const EchoIK::Recoil::RecoilOffset& ikOffsets)
	{
		infoOffsets.offset = ikOffsets.offset;
		infoOffsets.additivity = ikOffsets.additivity;
		infoOffsets.maxAdditiveOffsetMag = ikOffsets.maxAdditiveOffsetMag;

		infoOffsets.effectorLinks.clear();
		infoOffsets.effectorLinks.resize(ikOffsets.effectorLinks.size());
		for (int i = 0; i < infoOffsets.effectorLinks.size(); ++i) {
			TransIKEffToInfoEff(infoOffsets.effectorLinks[i], ikOffsets.effectorLinks[i]);
		}
	}

	static void TransInfoOffToIKOff(const IKRecoilInfo::Offsets& infoOffsets, EchoIK::Recoil::RecoilOffset& ikOffsets)
	{
		ikOffsets.offset = infoOffsets.offset;
		ikOffsets.additivity = infoOffsets.additivity;
		ikOffsets.maxAdditiveOffsetMag = infoOffsets.maxAdditiveOffsetMag;

		ikOffsets.effectorLinks.clear();
		ikOffsets.effectorLinks.resize(infoOffsets.effectorLinks.size());
		for (int i = 0; i < infoOffsets.effectorLinks.size(); ++i) {
			TransInfoEffToIKEff(infoOffsets.effectorLinks[i], ikOffsets.effectorLinks[i]);
		}
	}

	static void TransInfoOffVecToIKOffVec(std::vector<IKRecoilInfo::Offsets>& infoVec, std::vector<EchoIK::Recoil::RecoilOffset>& ikVec)
	{
		//static std::vector<EchoIK::Recoil::RecoilOffset> ikoffset;
		ikVec.clear();
		ikVec.resize(infoVec.size());

		for (int i = 0; i < infoVec.size(); ++i) {
			TransInfoOffToIKOff(infoVec[i], ikVec[i]);
		}
	}


	static std::vector<Echo::EchoIK::Recoil::RecoilOffset>& GetIKOffset(IKRecoilComponentData* pData)
	{
		auto& mIKInfo = pData->mIKInfo;
		static std::vector<EchoIK::Recoil::RecoilOffset> ikoffset;

		TransInfoOffVecToIKOffVec(mIKInfo.offsets, ikoffset);
		/*ikoffset.clear();
		ikoffset.resize(mIKInfo.offsets.size());

		for (int i = 0; i < mIKInfo.offsets.size(); ++i) {
			TransInfoOffToIKOff(mIKInfo.offsets[i], ikoffset[i]);
		}*/
		return ikoffset;
	}

	static void SetInfoOffsetByIKOffset(IKRecoilComponentData* pData, const std::vector<EchoIK::Recoil::RecoilOffset>& ikoffset)
	{
		auto& mIKInfo = pData->mIKInfo;
		mIKInfo.offsets.clear();
		mIKInfo.offsets.resize(ikoffset.size());
		for (int i = 0; i < ikoffset.size(); ++i) {
			TransIKOffToInfoOff(mIKInfo.offsets[i], ikoffset[i]);
		}
	}

}

namespace Echo
{

	DECLAREACTORCOMTYPE_CPP(IKRecoilComponent)

		EDITOR_REFLECT_BEGIN(IKRecoilComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_FullBodyBipedIKName, "FullBodyBipedIKName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AimIKName, "AimIKName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(m_AimIKSolverLast, "AimIKSolverLast", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Handness, "Handness", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_TwoHanded, "TwoHanded", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_MagnitudeRandom, "MagnitudeRandom", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_RotationRandom, "RotationRandom", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_HandRotationOffset, "HandRotationOffset", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BlendTime, "BlendTime", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Offsets,"Offsets", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_RecoilWeight, "RecoilWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();

	_DEFINEINFO_VIRFUN(IKRecoilComponentInfo, IKBaseComponentInfo)

#define IKDATA(ptr) static_cast<IKRecoilComponentData*>(ptr)
#define IKCOMDATA(ptr) static_cast<EchoIK::Recoil*>(ptr)

	IKRecoilComponent::IKRecoilComponent(ActorPtr pActor, ActorCOMMoveableType movableType)
		: IKBaseComponent(pActor, movableType)
	{
		m_pCompData = new IKRecoilComponentData();
	}

	IKRecoilComponent::~IKRecoilComponent(){}

	//Set component data(IK###ComponentData) by a info(IK###Info)
	void IKRecoilComponent::SetIKInfo(FinalIKInfoBase* pInfo)
	{
		if (pInfo == nullptr) return;
		SetIKInfoData(*((IKRecoilInfo*)pInfo));
	}

	bool IKRecoilComponent::IsCanCreate() {
		return !GetOffsets().empty() && !GetRecoilWeight().empty() && CheckValidFBBIK(IKDATA(m_pCompData)->mIKInfo.ik);
	}

	const IKRecoilInfo& IKRecoilComponent::GetIKInfoData()
	{
		return IKDATA(m_pCompData)->mIKInfo;
	}

	void IKRecoilComponent::SetFullBodyBipedIKName(const Name& ikName) {
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.ik = ikName;
	}
	const Name& IKRecoilComponent::GetFullBodyBipedIKName() {
		return IKDATA(m_pCompData)->mIKInfo.ik;
	}

	const String& IKRecoilComponent::GetAimIKName()
	{
		return IKDATA(m_pCompData)->AimIKName;
	}

	/*bool IKRecoilComponent::GetAimIKSolverLast()
	{
		return IKDATA(m_pCompData)->mIKInfo.aimIKSolvedLast;
	}*/

	uint32 IKRecoilComponent::GetHandness()
	{
		return EchoIK::Recoil::Handedness(IKDATA(m_pCompData)->mIKInfo.handness);
	}

	bool IKRecoilComponent::GetTwoHanded()
	{
		return IKDATA(m_pCompData)->mIKInfo.twoHanded;
	}

	float IKRecoilComponent::GetMagnitudeRandom()
	{
		return IKDATA(m_pCompData)->mIKInfo.magnitudeRandom;
	}

	const Vector3& IKRecoilComponent::GetRotationRandom()
	{
		return IKDATA(m_pCompData)->mIKInfo.rotationRandom;
	}

	const Vector3& IKRecoilComponent::GetHandRotationOffset()
	{
		return IKDATA(m_pCompData)->mIKInfo.handRotationOffset;
	}

	float IKRecoilComponent::GetBlendTime()
	{
		return IKDATA(m_pCompData)->mIKInfo.blendTime;
	}

	const std::vector<IKRecoilInfo::Offsets>& IKRecoilComponent::GetOffsets()
	{
		return IKDATA(m_pCompData)->mIKInfo.offsets;
	}

	const std::vector<Vector2>& IKRecoilComponent::GetRecoilWeight()
	{
		return IKDATA(m_pCompData)->mIKInfo.recoilWeight;
	}


	void IKRecoilComponent::SetAimIKName(const String& value)
	{
		IKDATA(m_pCompData)->AimIKName = value;
		IKAimIKComponentPtr compPtr = FindComponent<IKAimIKComponent>(value);
		SetAimIKTarget(compPtr);
	}


	void IKRecoilComponent::SetHandness(uint32 value)
	{
		IKDATA(m_pCompData)->mIKInfo.handness = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->handedness = (EchoIK::Recoil::Handedness)value;
	}

	void IKRecoilComponent::SetTwoHanded(bool value)
	{
		IKDATA(m_pCompData)->mIKInfo.twoHanded = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->twoHanded = value;
	}

	void IKRecoilComponent::SetMagnitudeRandom(float value)
	{
		IKDATA(m_pCompData)->mIKInfo.magnitudeRandom = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->magnitudeRandom = value;
	}

	void IKRecoilComponent::SetRotationRandom(const Vector3& value)
	{
		IKDATA(m_pCompData)->mIKInfo.rotationRandom = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->rotationRandom = value;
	}

	void IKRecoilComponent::SetHandRotationOffset(const Vector3& value)
	{
		IKDATA(m_pCompData)->mIKInfo.handRotationOffset = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->handRotationOffset = value;
	}

	void IKRecoilComponent::SetBlendTime(float value)
	{
		IKDATA(m_pCompData)->mIKInfo.blendTime = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->blendTime = value;
	}

	void IKRecoilComponent::SetOffsets(const std::vector<IKRecoilInfo::Offsets>& value)
	{
		if (!IsExistIK()) {
			IKDATA(m_pCompData)->mIKInfo.offsets = value;
		}
		else if(IKDATA(m_pCompData)->mIKInfo.offsets.size() == value.size()) {
			std::vector<IKRecoilInfo::Offsets>& _offsets = IKDATA(m_pCompData)->mIKInfo.offsets;
			size_t size = _offsets.size();
			for (int i = 0; i != size; ++i) {
				if (_offsets[i].effectorLinks.size() != value[i].effectorLinks.size()) {
					return;
				}
			}
			_offsets = value;
			std::vector<EchoIK::Recoil::RecoilOffset>& _ikoffsets = IKCOMDATA(m_pIKComponent)->offsets;
			for (int i = 0; i != size; ++i) {
				_ikoffsets[i].offset = _offsets[i].offset;
				_ikoffsets[i].additivity = _offsets[i].additivity;
				_ikoffsets[i].maxAdditiveOffsetMag = _offsets[i].maxAdditiveOffsetMag;

				_ikoffsets[i].effectorLinks.resize(_offsets[i].effectorLinks.size());
				for (int j = 0; j < _offsets[i].effectorLinks.size(); ++j) {
					_ikoffsets[i].effectorLinks[j].effector = EchoIK::FullBodyBipedEffector(_offsets[i].effectorLinks[j].effector);
					_ikoffsets[i].effectorLinks[j].weight = _offsets[i].effectorLinks[j].weight;
				}
			}
		}
	}

	void IKRecoilComponent::SetAimIKTarget(const IKAimIKComponentPtr& pComp)
	{
		if (!IsExistIK()) {
			mAimIKPtr = pComp;
		}
		else {
			mAimIKPtr = pComp;
			ApplyAimIKTargetPtr();
		}

	}
	void IKRecoilComponent::ApplyAimIKTargetPtr() {
		if (mAimIKPtr != nullptr) {
			IKCOMDATA(m_pIKComponent)->SetAimIKtarget(mAimIKPtr->GetIKComponent());
		}
		else {
			IKCOMDATA(m_pIKComponent)->SetAimIKtarget(nullptr);
		}
	}

	void IKRecoilComponent::SetRecoilWeight(const std::vector<Vector2>& value)
	{
		if (!IsExistIK()) {
			IKDATA(m_pCompData)->mIKInfo.recoilWeight = value;
		}
		else if (IKDATA(m_pCompData)->mIKInfo.recoilWeight.size() == value.size()) {
			IKDATA(m_pCompData)->mIKInfo.recoilWeight = value;
			std::sort(IKDATA(m_pCompData)->mIKInfo.recoilWeight.begin(), IKDATA(m_pCompData)->mIKInfo.recoilWeight.end(), [](const Vector2& v1, const Vector2& v2) -> bool { return v1.x < v2.x; });
			AnimationCurve& recoilWeight = IKCOMDATA(m_pIKComponent)->recoilWeight;
			recoilWeight.RemoveKeys(recoilWeight.begin(), recoilWeight.end());
			for (int i = 0; i != value.size(); ++i) {
				AnimationCurveTpl<float>::Keyframe key(value[i].x, value[i].y);
				recoilWeight.AddKey(key);
			}
		}
	}

	void IKRecoilComponent::Fire(float magnitude=1.f)
	{
		if (!IsExistIK()) return;

		IKCOMDATA(m_pIKComponent)->Fire(magnitude);
	}

	EchoIK::FullBodyBipedIK* IKRecoilComponent::GetFullBodyBipedIK() {
		if (!IsExistIK()) return nullptr;
		return IKCOMDATA(m_pIKComponent)->ik;
	}

	void IKRecoilComponent::PostCreateIK()
	{
		ApplyAimIKTargetPtr();
	}

	void IKRecoilComponent::PreDestroyIK()
	{
		IKCOMDATA(m_pIKComponent)->SetAimIKtarget(nullptr);
	}

};

namespace Echo {


	void IKRecoilComponent::UseDefaultInitialize()
	{
		SetAimIKName(String(""));

		SetFullBodyBipedIKName(Name::g_cEmptyName);
		SetHandness(EchoIK::Recoil::Handedness::Right);
		SetTwoHanded(true);
		SetMagnitudeRandom(0.1f);
		SetRotationRandom(Vector3(90.0f, 90.0f, 90.0f));
		SetHandRotationOffset(Vector3(0.0f, 0.0f, 0.0f));
		SetBlendTime(0.1f);
		SetOffsets(GetDefaultRecoilOffset());
		SetRecoilWeight(GetDefaultRecoilWeight());
	}

	void IKRecoilComponent::SetIKInfoData(const IKRecoilInfo& info)
	{
		SetFullBodyBipedIKName(info.ik);
		SetHandness(info.handness);
		SetTwoHanded(info.twoHanded);
		SetMagnitudeRandom(info.magnitudeRandom);
		SetHandRotationOffset(info.handRotationOffset);
		SetBlendTime(info.blendTime);
		SetOffsets(info.offsets);
		SetRotationRandom(info.rotationRandom);
		SetRecoilWeight(info.recoilWeight);
	}

	void IKRecoilComponent::SetCompData(IKBaseComponentData* pData)
	{
		if (pData == nullptr) return;
		IKBaseComponent::SetCompData(pData);
		SetAimIKName(IKDATA(pData)->AimIKName);
	}

	void IKRecoilComponentInfo::CopyToCompData(IKBaseComponentData* pData)
	{
		if (!pData) return;
		IKBaseComponentInfo::CopyToCompData(pData);

		IKDATA(pData)->AimIKName = GetAimIKName();

		IKDATA(pData)->mIKInfo.ik = GetFullBodyBipedIKName();
		IKDATA(pData)->mIKInfo.rotationRandom = GetRotationRandom();
		IKDATA(pData)->mIKInfo.recoilWeight = GetRecoilWeight();
		IKDATA(pData)->mIKInfo.handness = GetHandness();
		IKDATA(pData)->mIKInfo.twoHanded = GetTwoHanded();
		IKDATA(pData)->mIKInfo.magnitudeRandom = GetMagnitudeRandom();
		IKDATA(pData)->mIKInfo.handRotationOffset = GetHandRotationOffset();
		IKDATA(pData)->mIKInfo.blendTime = GetBlendTime();
		IKDATA(pData)->mIKInfo.offsets = GetOffsets();
	}

	void IKRecoilComponentInfo::CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* _pComp)
	{
		if (!pData) return;
		IKBaseComponentInfo::CompDataToInfo(pData, _pComp);
		IKRecoilComponent* pComp = (IKRecoilComponent*)_pComp;

		SetAimIKName(IKDATA(pData)->AimIKName, pComp);

		SetFullBodyBipedIKName(IKDATA(pData)->mIKInfo.ik, pComp);
		SetHandness(IKDATA(pData)->mIKInfo.handness, pComp);
		SetTwoHanded(IKDATA(pData)->mIKInfo.twoHanded, pComp);
		SetMagnitudeRandom(IKDATA(pData)->mIKInfo.magnitudeRandom, pComp);
		SetHandRotationOffset(IKDATA(pData)->mIKInfo.handRotationOffset, pComp);
		SetBlendTime(IKDATA(pData)->mIKInfo.blendTime, pComp);
		SetOffsets(IKDATA(pData)->mIKInfo.offsets, pComp);
		SetRotationRandom(IKDATA(pData)->mIKInfo.rotationRandom, pComp);
		SetRecoilWeight(IKDATA(pData)->mIKInfo.recoilWeight, pComp);
	}

}
