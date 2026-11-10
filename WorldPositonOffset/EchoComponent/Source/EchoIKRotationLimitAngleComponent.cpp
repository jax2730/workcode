#include "EchoComponentStableHeaders.h"
#include "EchoIKRotationLimitAngleComponent.h"
#include "IK/EchoRotationLimitAngle.h"
#include "IK/EchoIK.h"
#include "EchoWorldManager.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(IKRotationLimitAngleComponent)
	EDITOR_REFLECT_BEGIN(IKRotationLimitAngleComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Bone,"Bone",EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_AxisOffset,"AxisOffset",EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Limit,"Limit",EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_TwistLimit,"TwistLimit",EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_UseDefaultLocalRotation,"UseDefaultLocalRotation",EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN(IKRotationLimitAngleComponentInfo, IKBaseComponentInfo)

#define IKDATA(ptr) static_cast<IKRotationLimitAngleComponentData*>(ptr)
#define IKCOMDATA(ptr) static_cast<EchoIK::RotationLimitAngle*>(ptr)

	IKRotationLimitAngleComponent::IKRotationLimitAngleComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType)
		:IKBaseComponent(pActor,movableType)
	{
		m_pCompData = new IKRotationLimitAngleComponentData();
	}

	IKRotationLimitAngleComponent::~IKRotationLimitAngleComponent(){}

	void IKRotationLimitAngleComponent::unInit() {
#ifdef USE_IK_HELPEROBJ
		ReleaseHelper();
#endif // USE_IK_HELPEROBJ
	}

	void IKRotationLimitAngleComponent::SetIKInfo(FinalIKInfoBase* pInfo)
	{
		if (pInfo == nullptr) return;
		SetIKInfoData(*((RotationLimitAngleInfo*)pInfo));
	}

	bool IKRotationLimitAngleComponent::IsCanCreate() {
		RoleIKComponent* pRoleIK = GetRoleIKComponent();
		if (!pRoleIK) return false;
		if (GetBone() == Name::g_cEmptyName) return false;
		BoneNodeIK* pBone = (BoneNodeIK*)pRoleIK->getBoneNodeByName(GetBone());
		return pBone && pBone->m_pRotationLimit == nullptr;
	}

	const Echo::RotationLimitAngleInfo& IKRotationLimitAngleComponent::GetIKInfoData()
	{
		return IKDATA(m_pCompData)->mIKInfo;
	}

	void IKRotationLimitAngleComponent::showEditorHelper(bool isShow) {
		SetHelperVisible(isShow);
	}

	const Name& IKRotationLimitAngleComponent::GetBone() {
		return IKDATA(m_pCompData)->mIKInfo.bone;
	}
	void IKRotationLimitAngleComponent::SetBone(const Name& value) {
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.bone = value;
#ifdef USE_IK_HELPEROBJ
		mIsUpdateEditor = true;
#endif // USE_IK_HELPEROBJ
	}
	const Vector3& IKRotationLimitAngleComponent::GetAxisOffset() {
		return IKDATA(m_pCompData)->mIKInfo.axisOffset;
	}
	void IKRotationLimitAngleComponent::SetAxisOffset(const Vector3& value) {
		IKDATA(m_pCompData)->mIKInfo.axisOffset = value;
#ifdef USE_IK_HELPEROBJ
		mIsUpdateEditor = true;
#endif // USE_IK_HELPEROBJ
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->setAxisOffset(value);
	}
	float IKRotationLimitAngleComponent::GetLimit() {
		return IKDATA(m_pCompData)->mIKInfo.limit;
	}
	void IKRotationLimitAngleComponent::SetLimit(float value) {
		value = Math::Clamp(value, 0.0f, 180.0f);
		IKDATA(m_pCompData)->mIKInfo.limit = value;
#ifdef USE_IK_HELPEROBJ
		mIsUpdateEditor = true;
#endif // USE_IK_HELPEROBJ
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->limit = value;
	}
	float IKRotationLimitAngleComponent::GetTwistLimit() {
		return IKDATA(m_pCompData)->mIKInfo.twistLimit;
	}
	void IKRotationLimitAngleComponent::SetTwistLimit(float value) {
		value = Math::Clamp(value, 0.0f, 180.0f);
		IKDATA(m_pCompData)->mIKInfo.twistLimit = value;
#ifdef USE_IK_HELPEROBJ
		mIsUpdateEditor = true;
#endif // USE_IK_HELPEROBJ
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->twistLimit = value;
	}
	bool IKRotationLimitAngleComponent::GetUseDefaultLocalRotation() {
		return IKDATA(m_pCompData)->mIKInfo.useDefaultLocalRotation;
	}
	void IKRotationLimitAngleComponent::SetUseDefaultLocalRotation(bool value) {
		IKDATA(m_pCompData)->mIKInfo.useDefaultLocalRotation = value;
#ifdef USE_IK_HELPEROBJ
		mIsUpdateEditor = true;
#endif // USE_IK_HELPEROBJ
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->useDefaultLocalRotation = value;
	}

}

namespace Echo {

	void IKRotationLimitAngleComponent::SetHelperVisible(bool value) {
#ifdef USE_IK_HELPEROBJ
		mIsHelperVisible = value;
		if (mIsHelperVisible) CreateHelper();
		if (!m_pHelperObj || !m_pHelperObj2) return;
		m_pHelperObj->setVisible(value);
		m_pHelperObj2->setVisible(value);
		mIsUpdateEditor = true;
#endif // USE_IK_HELPEROBJ
	}
	bool IKRotationLimitAngleComponent::GetHelperVisible() {
#ifdef USE_IK_HELPEROBJ
		return mIsHelperVisible;
#else
		return false;
#endif // USE_IK_HELPEROBJ
	}

	void IKRotationLimitAngleComponent::SetHelperScale(float value) {
#ifdef USE_IK_HELPEROBJ
		m_fHelperScale = Math::Clamp(value, 0.0f, 10.0f);
		mIsUpdateEditor = true;
#endif // USE_IK_HELPEROBJ
	}
	float IKRotationLimitAngleComponent::GetHelperScale() {
#ifdef USE_IK_HELPEROBJ
		return m_fHelperScale;
#else
		return 1.0f;
#endif // USE_IK_HELPEROBJ
	}

	void IKRotationLimitAngleComponent::SetHelperOffset(const Vector3& offset) {
#ifdef USE_IK_HELPEROBJ
		m_HelperOffset = offset;
		mIsUpdateEditor = true;
#endif // USE_IK_HELPEROBJ
	}
	const Vector3& IKRotationLimitAngleComponent::GetHelperOffset() {
#ifdef USE_IK_HELPEROBJ
		return m_HelperOffset;
#else
		return Vector3::ZERO;
#endif // USE_IK_HELPEROBJ
	}

#ifdef USE_IK_HELPEROBJ

	void IKRotationLimitAngleComponent::CreateHelper() {
		if (m_pHelperObj) return;
		pHelperRootNode = getSceneManager()->createSceneNode();

		Name helperName = Name(getCombinedName().append("||").append("HelperObj"));
		m_pHelperObj = new HelperObject(helperName, HelperObjType_Box, false, nullptr, getSceneManager(), true);
		m_pHelperObj->setVisible(true);
		m_pHelperObj->setRenderType(ECHO_DRAW::ECHO_TRIANGLE_LIST);
		m_pHelperObj->setHelperColorType(HelperObjColorType_Green);
		m_pHelperObj->setOrientation(Quaternion::IDENTITY);
		m_pHelperObj->setScale(Vector3::UNIT_SCALE);
		pHelperRootNode->addChild(m_pHelperObj->getSceneNode());

		Name helperName2 = Name(getCombinedName().append("||").append("HelperObj2"));
		m_pHelperObj2 = new HelperObject(helperName2, HelperObjType_Box, false, nullptr, getSceneManager(), true);
		m_pHelperObj2->setVisible(mIsHelperVisible);
		m_pHelperObj2->setRenderType(ECHO_DRAW::ECHO_TRIANGLE_LIST);
		m_pHelperObj2->setHelperColorType(HelperObjColorType_Green);
		m_pHelperObj2->setOrientation(Quaternion::IDENTITY);
		m_pHelperObj2->setScale(Vector3::UNIT_SCALE);
		pHelperRootNode->addChild(m_pHelperObj2->getSceneNode());

		EchoIKSystem::instance()->GetManager(this)->AddEditorUpdateCompoennt(getWeakPtr<IKBaseComponent>());
	}
	void IKRotationLimitAngleComponent::ReleaseHelper() {
		if (pHelperRootNode) {
			pHelperRootNode->detachAllObject();
			if (pHelperRootNode->getParent())
			{
				pHelperRootNode->getParent()->destroyChild(pHelperRootNode);
				pHelperRootNode = nullptr;
			}
			else
				SAFE_DELETE(pHelperRootNode);
		}

		SAFE_DELETE(m_pHelperObj);
		SAFE_DELETE(m_pHelperObj2);
	}
	void IKRotationLimitAngleComponent::UpdateHelper() {
		if (!m_pHelperObj || !m_pHelperObj->getVisible()) return;

		std::vector<Vector3> vbVec;
		std::vector<uint16> ibVec;
		std::vector<uint32> colors;
		auto DrawCone = [&](float Angle1/*0~180*/, float Angle2/*0~-180*/, uint32 Color, float Length = 1.0f, uint32 numSides = 40) {
			Angle1 = Deg2Rad(Angle1) * 0.5f;
			Angle2 = Deg2Rad(Angle2) * 0.5f;
			float a = Math::Sin(Angle1);
			float b = Math::Sin(Angle2);
			uint16 startIB = (uint16)vbVec.size();

			std::vector<Vector3> tVB(numSides);

			vbVec.push_back(Vector3::ZERO);
			colors.push_back(Color);

			for (int i = 0; i != numSides; ++i) {
				float curAngle = 2 * ECHO_PI * i / numSides;
				float sTheta = Math::Sin(curAngle);
				float cTheta = Math::Cos(curAngle);

				auto phi = Math::ATan2(a * sTheta, b * cTheta);
				float sqr = a * a * sTheta * sTheta + b * b * cTheta * cTheta;
				float rsq = Math::Sqrt(1 - sqr);
				float r = Math::Sqrt(sqr);
				float z = 1 - 2 * sqr;
				float x = 2 * r * rsq * Math::Cos(phi);
				float y = 2 * r * rsq * Math::Sin(phi);
				tVB[i] = (Vector3(x, y, z) * Length);
			}
			for (int i = 0; i != tVB.size() - 1; ++i) {
				vbVec.push_back(tVB[i]);
				colors.push_back(Color);
				ibVec.push_back(startIB + i + 1);
				ibVec.push_back(startIB);
				ibVec.push_back(startIB + i + 2);

				ibVec.push_back(startIB + i + 2);
				ibVec.push_back(startIB);
				ibVec.push_back(startIB + i + 1);
			}
			vbVec.push_back(tVB[tVB.size() - 1]);
			colors.push_back(Color);
			ibVec.push_back(startIB + (uint16)tVB.size());
			ibVec.push_back(startIB);
			ibVec.push_back(startIB + 1);

			ibVec.push_back(startIB + 1);
			ibVec.push_back(startIB);
			ibVec.push_back(startIB + (uint16)tVB.size());
		};

		auto AddIndexBuffer = [](std::vector<uint16>& _ibVec, const std::vector<uint16>& _newIBVec, uint16 _startIB) {
			if (_newIBVec.empty()) return;
			size_t curSize = _ibVec.size();
			int newIBSize = (int)_newIBVec.size();
			_ibVec.resize(curSize + newIBSize);
			for (int i = 0; i != newIBSize; ++i) {
				_ibVec[curSize + i] = _startIB + _newIBVec[i];
			}
		};

		//绘制方线
		auto DrawBox = [&](uint32 Color, float LengthP = 1.0f, float LengthN = 1.0f, float Width = 0.01f, bool IsDrawArrow = false) {

			uint16 startIB = (uint16)vbVec.size();


			std::vector<Vector3> tVB(8);
			tVB[0] = Vector3(-Width, -Width, -LengthN);	//0
			tVB[1] = Vector3(-Width, Width, -LengthN);	//1
			tVB[2] = Vector3(Width, Width, -LengthN);	//2
			tVB[3] = Vector3(Width, -Width, -LengthN);	//3
			tVB[4] = Vector3(-Width, -Width, LengthP);	//4
			tVB[5] = Vector3(-Width, Width, LengthP);	//5
			tVB[6] = Vector3(Width, Width, LengthP);	//6
			tVB[7] = Vector3(Width, -Width, LengthP);	//7

			static std::vector<uint16> indexBuffer1 = {
				0,1,2,
				0,2,3,
				4,6,5,
				4,7,6,
				2,6,7,
				2,7,3,
				0,4,1,
				4,5,1,
				1,5,6,
				1,6,2,
				0,3,7,
				0,7,4
			};

			AddIndexBuffer(ibVec, indexBuffer1, startIB);

			if (IsDrawArrow) {
				startIB += 8;
				tVB.push_back(Vector3(0.f, 0.f, LengthP + 3.f * Width));
				tVB.push_back(Vector3(-3.f * Width, -3.f * Width, LengthP - 3.f * Width));
				tVB.push_back(Vector3(-3.f * Width, 3.f * Width, LengthP - 3.f * Width));
				tVB.push_back(Vector3(3.f * Width, 3.f * Width, LengthP - 3.f * Width));
				tVB.push_back(Vector3(3.f * Width, -3.f * Width, LengthP - 3.f * Width));

				static std::vector<uint16> indexBuffer2 = {
					0,1,2,
					0,2,3,
					0,3,4,
					0,4,1,
					0,4,1,
					1,2,3,
					2,3,4
				};

				AddIndexBuffer(ibVec, indexBuffer2, startIB);

			}

			for (int i = 0; i != tVB.size(); ++i) {
				tVB[i] = tVB[i];
			}

			vbVec.insert(vbVec.end(), tVB.begin(), tVB.end());
			colors.insert(colors.end(), tVB.size(), Color);


		};


		Vector4 colorV4;

		colorV4 = { 1.0f, 0.0f, 0.0f, 0.3f };
		uint32 colorR = colorV4.ToRGBA8888();
		colorV4 = { 0.0f, 0.0f, 1.0f, 0.3f };
		uint32 colorB = colorV4.ToRGBA8888();

		float angle = IKDATA(m_pCompData)->mIKInfo.limit;
		//float angle2 = IKDATA(m_pCompData)->mIKInfo.twistLimit;
		angle = Math::Clamp(angle, 0.5f, 179.5f);
		//angle2 = Math::Clamp(angle2, 0.5f, 179.5f);
		DrawCone(angle, angle, colorR, 0.5f * m_fHelperScale);
		//DrawSector(-angle2, angle2, rotation* Vector3::LEFT, rotation* Vector3::FORWARD, colorR, 0.1f);
		m_pHelperObj->changeCustomPolygon(vbVec, ibVec, colors);

		vbVec.clear();
		ibVec.clear();
		colors.clear();
		DrawBox(colorB, 0.5f * m_fHelperScale, 0.0f, 0.01f * m_fHelperScale, true);
		m_pHelperObj2->changeCustomPolygon(vbVec, ibVec, colors);
	}

	void IKRotationLimitAngleComponent::UpdateComponentInEditor() {
		if (m_pHelperObj && m_pHelperObj->getVisible()) {
			RoleIKComponent* pRoleIKComp = GetRoleIKComponent();
			if (!pRoleIKComp) return;
			WorldNode* pBone = pRoleIKComp->getBoneNodeByName(GetBone());
			if (!pBone) return;

			pHelperRootNode->setPosition(pBone->_getDerivedPosition());
			Quaternion rotationOffset;
			rotationOffset.FromEuler(m_HelperOffset);
			pHelperRootNode->setOrientation(rotationOffset);
			pHelperRootNode->_update(true, true);

			Quaternion rotation;
			rotation.FromEuler(IKDATA(m_pCompData)->mIKInfo.axisOffset);
			if (pBone->getParent()) {
				m_pHelperObj->setOrientation(pBone->getParent()->_getDerivedOrientation() * rotation);
			}
			else {
				m_pHelperObj->setOrientation(rotation);
			}
			m_pHelperObj2->setOrientation(pBone->_getDerivedOrientation());

		}
		if (mIsUpdateEditor) {
			UpdateHelper();
			mIsUpdateEditor = false;
		}
	}
#else 
	void IKRotationLimitAngleComponent::UpdateComponentInEditor() {}
#endif // USE_IK_HELPEROBJ
}

namespace Echo {

	void IKRotationLimitAngleComponent::UseDefaultInitialize()
	{
		SetBone(Name::g_cEmptyName);
		SetAxisOffset(Vector3::ZERO);
		SetLimit(0.0f);
		SetTwistLimit(0.0f);
		SetUseDefaultLocalRotation(false);
	}

	void IKRotationLimitAngleComponent::SetIKInfoData(const RotationLimitAngleInfo& info)
	{
		SetBone(info.bone);
		SetAxisOffset(info.axisOffset);
		SetLimit(info.limit);
		SetTwistLimit(info.twistLimit);
		SetUseDefaultLocalRotation(info.useDefaultLocalRotation);
	}

	void IKRotationLimitAngleComponentInfo::CopyToCompData(IKBaseComponentData* pData)
	{
		if (!pData) return;
		IKBaseComponentInfo::CopyToCompData(pData);
		IKDATA(pData)->mIKInfo.bone = GetBone();
		IKDATA(pData)->mIKInfo.axisOffset = GetAxisOffset();
		IKDATA(pData)->mIKInfo.limit = GetLimit();
		IKDATA(pData)->mIKInfo.twistLimit = GetTwistLimit();
		IKDATA(pData)->mIKInfo.useDefaultLocalRotation = GetUseDefaultLocalRotation();
	}

	void IKRotationLimitAngleComponentInfo::CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp)
	{
		if (!pData) return;
		IKBaseComponentInfo::CompDataToInfo(pData, pComp);
		IKRotationLimitAngleComponent* _pComp = static_cast<IKRotationLimitAngleComponent*>(pComp);
		SetBone(IKDATA(pData)->mIKInfo.bone, _pComp);
		SetAxisOffset(IKDATA(pData)->mIKInfo.axisOffset, _pComp);
		SetLimit(IKDATA(pData)->mIKInfo.limit, _pComp);
		SetTwistLimit(IKDATA(pData)->mIKInfo.twistLimit, _pComp);
		SetUseDefaultLocalRotation(IKDATA(pData)->mIKInfo.useDefaultLocalRotation, _pComp);
	}
}
