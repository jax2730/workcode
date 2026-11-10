#include "EchoComponentStableHeaders.h"
#include "EchoBoneRenderComponent.h"
#include "Effect/EchoEffectHolder.h"
#include "EchoActorProFuncImp.h"
#include "Actor/EchoNodeComponent.h"
#include "EchoBone.h"
#include "IK/EchoIKSkeletonPosture.h"
#include "EchoResourceFolderManager.h"
#include "EchoIKSkeletonPostureComponent.h"
#include "EchoWorldManager.h"

namespace Echo {

	DECLAREACTORCOMTYPE_CPP(BoneRenderComponent)

	float BoneRenderComponent::gBoneWidth = 0.008f;
	std::vector<Vector3> BoneRenderComponent::gVB;
	std::vector<uint16> BoneRenderComponent::gIB;
	std::vector<uint32> BoneRenderComponent::gColors;

	void BoneRenderComponent::GenerateRenderBuffer() {
		std::vector<Vector3>& BoneVB = gVB;
		std::vector<uint16>& BoneIB = gIB;
		std::vector<uint32>& BoneColors = gColors;
		{
			//添加圆
			uint16 startIB = (uint16)BoneVB.size();
			int mT = 32;
			float mX = 0, mZ = 0.5f;
			float mSIN = std::sin(2 * ECHO_PI / float(mT)), mCOS = std::cos(2 * ECHO_PI / float(mT));
			Vector4 colorV4(0.0f, 1.0f, 0.0f, 1.0f);
			uint32 color = colorV4.ToRGBA8888();
			for (int i = 0; i != mT; ++i) {
				BoneVB.push_back(Vector3(mX, 0.f, mZ));
				BoneColors.push_back(color);
				float x = mZ * mSIN + mX * mCOS;
				mZ = mZ * mCOS - mX * mSIN;
				mX = x;
				BoneIB.push_back(startIB + i);
				BoneIB.push_back(startIB + i + 1);
			}
			BoneIB[BoneIB.size() - 1] = startIB;
		}
		{
			//添加盒子
			uint16 startIB = (uint16)BoneVB.size();
			Vector4 colorV4(0.0f, 0.0f, 0.0f, 1.f);
			uint32 color = colorV4.ToRGBA8888();
			Vector3 upA(0.5f, 1.0f, 0.5f);		//0
			Vector3 upB(-0.5f, 1.0f, 0.5f);		//1
			Vector3 upC(-0.5f, 1.0f, -0.5f);		//2
			Vector3 upD(0.5f, 1.0f, -0.5f);		//3
			Vector3 downA(0.5f, 0.0f, 0.5f);		//4
			Vector3 downB(-0.5f, 0.0f, 0.5f);	//5
			Vector3 downC(-0.5f, 0.0f, -0.5f);	//6
			Vector3 downD(0.5f, 0.0f, -0.5f);	//7

			BoneVB.push_back(upA); BoneColors.push_back(color);
			BoneVB.push_back(upB); BoneColors.push_back(color);
			BoneVB.push_back(upC); BoneColors.push_back(color);
			BoneVB.push_back(upD); BoneColors.push_back(color);
			BoneVB.push_back(downA); BoneColors.push_back(color);
			BoneVB.push_back(downB); BoneColors.push_back(color);
			BoneVB.push_back(downC); BoneColors.push_back(color);
			BoneVB.push_back(downD); BoneColors.push_back(color);

			BoneIB.push_back(startIB + 0); BoneIB.push_back(startIB + 1);
			BoneIB.push_back(startIB + 1); BoneIB.push_back(startIB + 2);
			BoneIB.push_back(startIB + 2); BoneIB.push_back(startIB + 3);
			BoneIB.push_back(startIB + 3); BoneIB.push_back(startIB + 0);
			BoneIB.push_back(startIB + 4); BoneIB.push_back(startIB + 5);
			BoneIB.push_back(startIB + 5); BoneIB.push_back(startIB + 6);
			BoneIB.push_back(startIB + 6); BoneIB.push_back(startIB + 7);
			BoneIB.push_back(startIB + 7); BoneIB.push_back(startIB + 4);
			BoneIB.push_back(startIB + 0); BoneIB.push_back(startIB + 4);
			BoneIB.push_back(startIB + 1); BoneIB.push_back(startIB + 5);
			BoneIB.push_back(startIB + 2); BoneIB.push_back(startIB + 6);
			BoneIB.push_back(startIB + 3); BoneIB.push_back(startIB + 7);

		}
		{
			//添加盒子10, 209, 186
			uint16 startIB = (uint16)BoneVB.size();
			Vector4 colorV4(0.0f, 1.0f, 1.0f, 1.0f);
			uint32 color = colorV4.ToRGBA8888();
			Vector3 upA(0.1f, 1.0f, 0.1f);		//0
			Vector3 upB(-0.1f, 1.0f, 0.1f);		//1
			Vector3 upC(-0.1f, 1.0f, -0.1f);		//2
			Vector3 upD(0.1f, 1.0f, -0.1f);		//3
			Vector3 downA(0.1f, 0.0f, 0.1f);		//4
			Vector3 downB(-0.1f, 0.0f, 0.1f);	//5
			Vector3 downC(-0.1f, 0.0f, -0.1f);	//6
			Vector3 downD(0.1f, 0.0f, -0.1f);	//7

			BoneVB.push_back(upA); BoneColors.push_back(color);
			BoneVB.push_back(upB); BoneColors.push_back(color);
			BoneVB.push_back(upC); BoneColors.push_back(color);
			BoneVB.push_back(upD); BoneColors.push_back(color);
			BoneVB.push_back(downA); BoneColors.push_back(color);
			BoneVB.push_back(downB); BoneColors.push_back(color);
			BoneVB.push_back(downC); BoneColors.push_back(color);
			BoneVB.push_back(downD); BoneColors.push_back(color);

			BoneIB.push_back(startIB + 0); BoneIB.push_back(startIB + 1);
			BoneIB.push_back(startIB + 1); BoneIB.push_back(startIB + 2);
			BoneIB.push_back(startIB + 2); BoneIB.push_back(startIB + 3);
			BoneIB.push_back(startIB + 3); BoneIB.push_back(startIB + 0);
			BoneIB.push_back(startIB + 4); BoneIB.push_back(startIB + 5);
			BoneIB.push_back(startIB + 5); BoneIB.push_back(startIB + 6);
			BoneIB.push_back(startIB + 6); BoneIB.push_back(startIB + 7);
			BoneIB.push_back(startIB + 7); BoneIB.push_back(startIB + 4);
			BoneIB.push_back(startIB + 0); BoneIB.push_back(startIB + 4);
			BoneIB.push_back(startIB + 1); BoneIB.push_back(startIB + 5);
			BoneIB.push_back(startIB + 2); BoneIB.push_back(startIB + 6);
			BoneIB.push_back(startIB + 3); BoneIB.push_back(startIB + 7);
		}
	}

	BoneRenderComponentInfo::BoneRenderComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) : ActorComponentInfo(comType, MoveType) {}
	BoneRenderComponentInfo::~BoneRenderComponentInfo() {}

	EDITOR_REFLECT_BEGIN(BoneRenderComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_BoneName, "BoneName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()
	_DEFINEINFO_VIRFUN_BASE(BoneRenderComponentInfo)

	void BoneRenderComponent::init(ActorComponentInfo* info) { 
		CreateRender();
		if (!info) return;
		mBoneName = ((BoneRenderComponentInfo*)info)->GetBoneName();
	}
	void  BoneRenderComponent::unInit() {
		ReleaseHelper();
	}
	void BoneRenderComponent::replay(ActorComponentInfo* info) {  }
	void BoneRenderComponent::copyTo(const ActorComponentPtr& actCom) {
		if (!actCom) return;
		actCom.getStaticCastWeakPtr<BoneRenderComponent>()->mBoneName = mBoneName;
	}
	void BoneRenderComponent::connectCom() { 
		if (m_pHelperObj) {
			addChildToNode(m_pHelperObj->getSceneNode());
			UpdateComRootNode();
		}
		UpdateRender(); 
	}
	void BoneRenderComponent::disconnectCom() {
		if (m_pHelperObj) {
			RemoveChildFromNode(m_pHelperObj->getSceneNode());
		}
	}
	void BoneRenderComponent::showEditorHelper(bool isShow) {
		SetRenderVisible(isShow);
	}
	void BoneRenderComponent::updateCombinedName()
	{
		if (!m_pHelperObj) return;
		Name helperName = Name(getCombinedName().append("||").append("HelperObj"));
		m_pHelperObj->setName(helperName);
	}

	BoneRenderComponent::BoneRenderComponent(ActorPtr pActor, ActorCOMMoveableType movableType) : NodeActorComponent(pActor, movableType) {}
	BoneRenderComponent::~BoneRenderComponent() {}

	void BoneRenderComponent::UpdateBoneRenderTree() {
		if (getVisible()) {
			UpdateRender();
			for (auto& pChildComp : getChildActorComponents()) {
				if (pChildComp.second->getType() == getType()) {
					pChildComp.second.getStaticCastWeakPtr<BoneRenderComponent>()->UpdateBoneRenderTree();
				}
			}
		}
	}
	void BoneRenderComponent::UpdateRender() {
		if (!m_pHelperObj) { return; }
		ChangeVBIB();
	}
	void BoneRenderComponent::CreateRender() {
		if (m_pHelperObj) return;
		Name helperName = Name(getCombinedName().append("||").append("HelperObj"));
		m_pHelperObj = new HelperObject(helperName, HelperObjType_CustomPolygon, false, nullptr, getSceneManager());
		m_pHelperObj->setVisible(mIsVisible);
		m_pHelperObj->setRenderType(ECHO_DRAW::ECHO_LINE_LIST);
		m_pHelperObj->setHelperColorType(HelperObjColorType_Green);
		m_pHelperObj->setOrientation(Quaternion::IDENTITY);
		m_pHelperObj->setScale(Vector3::UNIT_SCALE);

		
		if (gVB.empty() || gIB.empty()) {
			GenerateRenderBuffer();
		}
	}
	void BoneRenderComponent::ReleaseHelper() {
		if (!m_pHelperObj) return;
		delete m_pHelperObj;
		m_pHelperObj = NULL;
	}

	bool BoneRenderComponent::IsRootBoneRender() {
		auto pParComp = getParentNodeCom();
		return !pParComp || (pParComp->getType() != BoneRenderComponent::mType && pParComp->getType() != IKSkeletonPostureComponent::mType);
	}

	void BoneRenderComponent::ChangeVBIB() {

		auto pParComp = getParentNodeCom();

		if (IsRootBoneRender()) {
			if (m_pHelperObj->getType() != HelperObjType_Sphere) {
				m_pHelperObj->changeHelperType(HelperObjType_Sphere);
				m_pHelperObj->setHelperColorType(HelperObjColorType_Red);
			}
			float width = gBoneWidth * 5.0f;
			m_pHelperObj->setScale({ width,width,width });
			return;
		}

		std::vector<Vector3> BoneVB(gVB.begin(),gVB.end());

		Quaternion NodeQuat(Quaternion::IDENTITY);
		
		Vector3 tarAxis = getWorldPosition() - pParComp->getWorldPosition();
		float len = tarAxis.Length();
		Vector3 parPos = pParComp->getWorldPosition();
		parPos = getWorldRotation().Inverse() * (parPos - getWorldPosition()) / getWorldScale();
		Vector3 selfAxis(0.f, 1.0f, 0.f);

		parPos.Normalize();
		Vector3 offset = parPos - Vector3::DOWN;
		if (offset.Length() > 0.0001f) {
			NodeQuat.FromAxesToAxes(selfAxis, parPos);
		}
		else {
			NodeQuat = { 1.0f,0.0f,0.0f,0.0f };
		}
		for (int i = 0; i != BoneVB.size(); ++i) {
			Vector3 v3(BoneVB[i].x * gBoneWidth, BoneVB[i].y * len, BoneVB[i].z * gBoneWidth);
			BoneVB[i] = NodeQuat * v3;
		}
		m_pHelperObj->changeCustomPolygon(BoneVB, gIB, gColors);

	}

	void BoneRenderComponent::SetRenderVisible(bool value) {
		mIsVisible = value;
		if (!m_pHelperObj) return;
		m_pHelperObj->setVisible(value);
	}
	bool BoneRenderComponent::GetRenderVisible() {
		if (!m_pHelperObj) return mIsVisible;
		return m_pHelperObj->getVisible();
	}

	void BoneRenderComponent::SetBone(Bone* pBone) {
		m_pBone = pBone;
		if (m_pBone) {
			
			mBoneName = m_pBone->getName();
			setPosition(m_pBone->getPosition());
			setRotation(m_pBone->getOrientation());
			setScale(m_pBone->getScale());
			
			if (Root::instance()->getRuntimeMode() == Root::eRM_Editor && m_pActComInfo != nullptr) {
				static_cast<BoneRenderComponentInfo*>(m_pActComInfo)->SetBoneName(m_pBone->getName());
				Quaternion quat = m_pBone->getOrientation();
				static_cast<BoneRenderComponentInfo*>(m_pActComInfo)->SetPosition(m_pBone->getPosition());
				static_cast<BoneRenderComponentInfo*>(m_pActComInfo)->SetEulerAngles(quat.ToEuler());
				static_cast<BoneRenderComponentInfo*>(m_pActComInfo)->SetScale(m_pBone->getScale());
			}
		}
	}
	Bone* BoneRenderComponent::GetBone() {
		return m_pBone;
	}
	const Name& BoneRenderComponent::GetBoneName() {
		return mBoneName;
	}
	void BoneRenderComponent::SetBoneName(const Name& name) {
		mBoneName = name;
	}

	void BoneRenderComponent::GetSkeletonPosturesData(std::map<Name, Quaternion>& posture, Vector3& selfWorldPos, Quaternion& selfWorldRot) {
		selfWorldPos = getWorldPosition();
		selfWorldRot = getWorldRotation();
		GetBonePosture(posture);
		auto iter = posture.find(GetBoneName());
		if (iter != posture.end()) posture.erase(iter);
	}

	void BoneRenderComponent::GetBonePosture(std::map<Name, Quaternion>& boneMap) {
		boneMap.insert(std::make_pair(GetBoneName(), getRotation()));
		for (auto& pChildComp : getChildActorComponents()) {
			if (pChildComp.second->getType() == getType()) {
				pChildComp.second.getStaticCastWeakPtr<BoneRenderComponent>()->GetBonePosture(boneMap);
			}
		}
	}

	void BoneRenderComponent::GetBonePosture(std::vector<std::tuple<Name, Vector3, Quaternion, uint32>>& posture, uint32 index) {
		uint32 selfIndex = (uint32)posture.size();
		posture.push_back(std::make_tuple(GetBoneName(), getPosition(), getRotation(), index));
		for (auto& pChildComp : getChildActorComponents()) {
			if (pChildComp.second->getType() == getType()) {
				pChildComp.second.getStaticCastWeakPtr<BoneRenderComponent>()->GetBonePosture(posture, selfIndex);
			}
		}
	}

	void BoneRenderComponent::ClearAllChild() {
		auto pActor = getActor();
		EchoTMap<String, ActorComponentPtr> childComps = getChildActorComponents();
		for (auto& pChildComp : childComps) {
			auto comInfo = pChildComp.second->m_pActComInfo;
			pActor->delComponent(pChildComp.second);
			pActor->delComponentInfo(comInfo);
		}
	}

	void BoneRenderComponent::SetBonePosture(std::vector<std::tuple<Name, Vector3, Quaternion, uint32>>& posture, uint32 index) {
		/*auto iter = std::find_if(posture.begin(), posture.end(), [&](const std::tuple<Name, Vector3, Quaternion, uint32>& tt) -> bool {
			Name name1 = std::get<0>(tt);
			Name name2 = GetBoneName();
			return name1 == name2;
			});
		if (iter != posture.end()) {
			Vector3 pos = std::get<1>(*iter);
			Quaternion quat = std::get<2>(*iter);

			setPosition(pos);
			setRotation(quat);

			static_cast<BoneRenderComponentInfo*>(m_pActComInfo)->SetPosition(pos);
			static_cast<BoneRenderComponentInfo*>(m_pActComInfo)->SetEulerAngles(quat.ToEulerPositive());
		}
		for (auto pChildComp : getChildActorComponents()) {
			if (pChildComp->getType() == getType()) {
				pChildComp.getDynamicCastWeakPtr<BoneRenderComponent>()->SetBonePosture(posture, index);
			}
		}*/
		ClearAllChild();
		auto pActor = getActor();
		std::vector<ActorComponentPtr> tempComps;
		tempComps.push_back(this->weak_from_this());
		for (int i = 0; i != posture.size(); ++i) {
			BoneRenderComponentPtr pComp = pActor->AddComponent<BoneRenderComponent>(std::get<0>(posture[i]).toString(), tempComps[std::get<3>(posture[i])], nullptr, getGroupID(), false);
			if (pComp != nullptr) {
				pComp->SetBoneName(std::get<0>(posture[i]));
				pComp->setPosition(std::get<1>(posture[i]));
				pComp->setRotation(std::get<2>(posture[i]));

				if (Root::instance()->getRuntimeMode() == Root::eRM_Editor && pComp->m_pActComInfo != nullptr) {
					static_cast<BoneRenderComponentInfo*>(pComp->m_pActComInfo)->SetBoneName(std::get<0>(posture[i]));
					static_cast<BoneRenderComponentInfo*>(pComp->m_pActComInfo)->SetPosition(std::get<1>(posture[i]));
					static_cast<BoneRenderComponentInfo*>(pComp->m_pActComInfo)->SetEulerAngles(std::get<2>(posture[i]).ToEuler());
				}
			}
			tempComps.push_back(pComp->weak_from_this());
		}

	}

	void BoneRenderComponent::exportToFile(const Name& file, EchoIK::FullBodyBipedEffector type) {
		
		EchoIK::IKSkeletonPostureTemplate* skel = EchoIK::IKSkeletonPostureManager::instance()->createEmptyIKSkeletonPostureTemplate();

		skel->setIKPartType(type);
		skel->setRootBoneName(GetBoneName());
		
		std::vector<std::tuple<Name, Vector3, Quaternion, uint32>> vec;
		GetBonePosture(vec, -1);
		vec.erase(vec.begin());

		skel->setSkeletonPosture(vec);
		skel->exportDataToFile(file.c_str());
		EchoIK::IKSkeletonPostureManager::instance()->releaseEmptyResource(skel);
	}
	EchoIK::FullBodyBipedEffector BoneRenderComponent::importFromFile(const Name& file) {

		EchoIK::IKSkeletonPostureTemplatePtr skel = EchoIK::IKSkeletonPostureManager::instance()->getPreparedResource(file);

		if (skel.isNull()) return EchoIK::FullBodyBipedEffector::NumEffectors;

		std::vector<std::tuple<Name, Vector3, Quaternion, uint32>> vec;
		skel->getSkeletonPosture(vec);

		SetBonePosture(vec, -1);

		return skel->getIKPartType();
	}
}
