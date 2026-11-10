#include "EchoComponentStableHeaders.h"
#include "EchoWheelColliderComponent.h"
#include "EchoRigidBodyComponent.h"
#include "EchoMatrix3.h"

#include "EchoPhysXAsset.h"
#include "EchoPhysXVehicleNoDrive.h"

#include "EchoActorProFuncImp.h"
#include "EchocJsonHelper.h"
//#define WHEELSHAPE_ACCURACY 16 
//#define GET_DATA() ((WheelColliderComponentData*)(m_pCompData))
//#define EWF_TO_PWF(EWF) (Echo::PhysXVehicleNoDriveDesc::WheelDesc::WheelFriction{EWF.extremumSlip,EWF.extremumValue,EWF.asymptoteSlip,EWF.asymptoteValue,EWF.stiffness}) 
//#define RIGIDBODY_WAKE_UP() {if (m_pAttachRigidBody != nullptr) m_pAttachRigidBody->WakeUp(); }
//
//static Echo::PhysXGroupEnum gWheelGroup = ePhysXGroup_VehicleWheel;
//static Echo::PhysXGroupEnum gWheelCanInteractorGroup = ePhysXGroup_Vehicle;
//
//namespace Echo
//{
//
//	constexpr float PI = 3.1415926535897932384626433832795028841971f;
//
//	static std::vector<Vector3> initCirlePoint()
//	{
//		std::vector<Vector3> result(360);
//		for (int i = 0; i < 360; ++i)
//			result[i] = Matrix3{ 1,	0,	0,
//								0	,std::cos(i * PI / 180),	std::sin(i * PI / 180),
//								0	,-std::sin(i * PI / 180),	std::cos(i * PI / 180) } *Vector3(0, 0, 1);
//		return result;
//	}
//
//	static const std::vector<Vector3> gCirclePoint = initCirlePoint(); // 半径为1 圆心为0，0，0 的圆周的点的集合 共360个 
//
//	RigidBodyComponent* findFatherRigidBody(ActorComponentPtr pac)
//	{
//		RigidBodyComponentPtr res = nullptr;
//		if (pac == nullptr)
//			return res.get();
//		ActorComponentPtr pf = pac->getParentNodeCom();
//		while (pf != nullptr)
//		{
//			res = pf->GetFirstChildComByType<RigidBodyComponent>(Name("RigidBodyComponent"));
//			if (res != nullptr)
//				return res.get();
//			pf = pf->getParentNodeCom();
//		}
//		return res.get();
//	}
//	WheelColliderComponentData::WheelColliderComponentData()
//	{
//		ColliderComponentData::filterGroupType = gWheelGroup;// 车轮类型为 ePhysXGroup_VehicleWheel
//		ColliderComponentData::isTrigger = false;
//	};
//
//	DECLAREACTORCOMTYPE_CPP(WheelColliderComponent)
//
//		WheelColliderComponent::WheelColliderComponent(ActorPtr pActor, Echo::ActorCOMMoveableType movableType) :
//		ColliderComponent(pActor, movableType),
//		m_wheelId(-1),
//		m_shapeId(-1),
//		m_pVehicleNoDrive(nullptr)
//	{
//		m_pCompData = new WheelColliderComponentData();
//		{
//			std::vector<Vector3> verts;
//
//			for (int i = 0; i < WHEELSHAPE_ACCURACY; ++i)
//			{
//				const float cosTheta = std::cos(PI * i * 2 / WHEELSHAPE_ACCURACY);
//				const float sinTheta = std::sin(PI * i * 2 / WHEELSHAPE_ACCURACY);
//				verts.push_back({ 0.5f , cosTheta, sinTheta });
//				verts.push_back({ -0.5f , cosTheta, sinTheta });
//			}
//			m_pPhysXConvexMesh = PhysXSystem::instance()->MeshDataToConvexMesh(verts);
//		}
//	}
//	WheelColliderComponent::~WheelColliderComponent()
//	{
//		SAFE_DELETE(m_pPhysXConvexMesh);
//	}
//	void WheelColliderComponent::GenerateVBIBCB(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec, std::vector<uint32>& cbVec)
//	{
//		// TODO: 1.将helper 挂载到刚体的父节点上 调整偏移和旋转
//		//		 2.增加滚动和悬挂效果
//
//		vbVec.clear();
//		ibVec.clear();
//		cbVec.clear();
//		//  仅在拥有父刚体时显示helper
//		RigidBodyComponent* pRB = findFatherRigidBody(this->weak_from_this());
//		if (pRB == nullptr)
//		{
//			vbVec.emplace_back(0.f, 0.f, 0.f);
//			ibVec.emplace_back(0);
//			ibVec.emplace_back(0);
//			cbVec.emplace_back(0);
//			return;
//		}
//
//		//Vector3 center = getPosition();
//		float		targetPos = getSuspTargetPos();				// 车轮在悬架的位置
//		float		radius = getRadius();						// 车轮的半径
//		float		suspDis = getSuspensionDistance();			// 悬架的长度
//		float		forceAppPoint = getForceAppPointDistance();	// 施力点
//		Transform	wheelPos;									// 车轮的变换
//		if (IsPhysicsStaut())
//		{
//			targetPos = tirePos();
//		}
//		// 轮胎轮廓 [0~360) 
//		for (int i = 0; i < 360; ++i)
//			vbVec.emplace_back(gCirclePoint[i] * radius - Vector3{ 0, (1.f - targetPos) * suspDis ,0 }), cbVec.emplace_back(Vector4(0.f, 1.f, 0.f, 0.f).ToRGBA8888());
//		for (int i = 0; i < 359; ++i)
//			ibVec.emplace_back(i), ibVec.emplace_back(i + 1);
//		ibVec.emplace_back(0), ibVec.emplace_back(359);
//		ibVec.emplace_back(0), ibVec.emplace_back(180);
//		// 悬挂 [360,366)
//		vbVec.emplace_back(), vbVec.emplace_back(Vector3{ 0,0,0.1f }), vbVec.emplace_back(Vector3{ 0,0,-0.1f });
//		vbVec.emplace_back(-Vector3{ 0,suspDis,0 }), vbVec.emplace_back(-Vector3{ 0,suspDis,0.1f }), vbVec.emplace_back(-Vector3{ 0,suspDis,-0.1f });
//		for (int i = 0; i < 6; ++i)
//			cbVec.emplace_back(Vector4(180 / 255.f, 100 / 255.f, 0.f, 0.f).ToRGBA8888());
//		ibVec.emplace_back(360), ibVec.emplace_back(363);
//		ibVec.emplace_back(361), ibVec.emplace_back(362);
//		ibVec.emplace_back(364), ibVec.emplace_back(365);
//		// 施力点 [366,372)
//		Vector3 forcePoint = -Vector3{ 0, (1.f - targetPos) * suspDis ,0 } - Vector3{ 0,radius,0 } + Vector3{ 0,forceAppPoint ,0 };
//		vbVec.emplace_back(forcePoint + Vector3{ 0.05f,0,0 }), vbVec.emplace_back(forcePoint - Vector3{ 0.05f,0,0 });
//		vbVec.emplace_back(forcePoint + Vector3{ 0,0.05f,0 }), vbVec.emplace_back(forcePoint - Vector3{ 0,0.05f,0 });
//		vbVec.emplace_back(forcePoint + Vector3{ 0,0,0.05f }), vbVec.emplace_back(forcePoint - Vector3{ 0,0,0.05f });
//		for (int i = 0; i < 6; ++i)
//			cbVec.emplace_back(Vector4(0.5f, 0.5f, 0.0f, 0.f).ToRGBA8888());
//		ibVec.emplace_back(366), ibVec.emplace_back(367);
//		ibVec.emplace_back(368), ibVec.emplace_back(369);
//		ibVec.emplace_back(370), ibVec.emplace_back(371);
//
//		Quaternion steerDir = Quaternion(Radian(getSteerAngle()), Vector3::UP);
//		Quaternion worldDir = pRB->convertLocalToWorldRotation(steerDir);
//		Quaternion RigidDir = convertWorldToLocalRotation(worldDir);
//
//		Vector3 scale = getWorldScale();
//		scale = { 1.f / scale.x , scale.y ,1.f / scale.z };
//		convertWorldToLocalScale(scale);
//
//		for (Vector3& Vec3 : vbVec)
//		{
//			// 将车轮的正方向固定在刚体的方向 (包含车轮自身的转向)
//			Vec3 = Vec3 * RigidDir;
//			// 取消x轴z轴缩放 将y的缩放运用到全身
//			Vec3.x *= scale.x;
//			Vec3.z *= scale.z;
//			Vec3.x *= scale.y;
//			Vec3.z *= scale.y;
//		}
//	}
//
//	EchoGeometry* WheelColliderComponent::GenerateGeometry(void* buffer)
//	{
//		float radius = getRealRadius();
//		float width = getRealWidth();
//		EchoConvexMeshGeometry* pGeom = new(buffer) EchoConvexMeshGeometry();
//		pGeom->convex = m_pPhysXConvexMesh;
//		pGeom->scale.x = width;
//		pGeom->scale.y = radius;
//		pGeom->scale.z = radius;
//		return pGeom;
//	}
//
//	//void WheelColliderComponent::ReleasePhysics()
//	//{
//	//	ColliderComponent::ReleasePhysics();
//	//	mIsUpdateHelper = true;
//	//}
//
//	void WheelColliderComponent::ChangeShape()
//	{
//		ColliderComponent::ChangeShape(); 
//		// 以下用来更新车辆信息
//		if (m_pVehicleNoDrive)
//		{
//			m_pVehicleNoDrive->changeWheelRadius(m_wheelId, getRealRadius());
//
//			m_pVehicleNoDrive->changeWheelWidth(m_wheelId, getRealWidth());
//
//			float maxCompression, maxDroop;
//			maxDroop = getRealSuspensionDistance() * getSuspTargetPos();
//			maxCompression = getRealSuspensionDistance() - maxDroop;
//			m_pVehicleNoDrive->changeMaxCompressionAndMaxDroop(m_wheelId, maxCompression, maxDroop);
//
//			m_pVehicleNoDrive->changeForceAppPos(m_wheelId, getRealForceAppPointDistance());
//		}
//	}
//
//	void WheelColliderComponent::ChangeAttach()
//	{
//		RigidBodyComponent* pRB = EchoPhysicsSystem::instance()->FindNewRigidBodyComp(getParentNodeCom(), nullptr);
//		if (pRB != nullptr)
//			AttachShape(pRB);
//		else
//			ReleasePhysics();
//	}
//
//	void WheelColliderComponent::AttachShapeEx(RigidBodyComponent* pRb)
//	{
//		m_shapeId = pRb->GetPhysics()->GetShapeIndex(m_pShape);
//		EchoPhysicsSystem::instance()->RegisterWheel(pRb, this);
//	}
//
//	void WheelColliderComponent::DetachShapeEx()
//	{
//		EchoPhysicsSystem::instance()->revokeWheel(this);
//		m_shapeId = -1;
//	}
//
//	bool WheelColliderComponent::IsCanPhysics()
//	{
//		return (EchoPhysicsSystem::instance()->FindNewRigidBodyComp(getParentNodeCom(), nullptr) != nullptr) &&
//			(getRadius() != 0);
//	}
//
//	bool WheelColliderComponent::IsPhysicsStaut()
//	{
//		return m_pVehicleNoDrive != nullptr;
//	}
//
//	void WheelColliderComponent::UpdateComponentInEditor()
//	{
//		UpdateHelper();
//		RemoveFromEditorUpdateQueue();
//	}
//
//	void WheelColliderComponent::ChangeLocalPose()
//	{
//		if (!m_pShape) return;
//		if (m_pAttachRigidBody != nullptr)
//		{
//			if (!m_pVehicleNoDrive)
//				return;
//			//Vector3 wheelWorldCenter = convertLocalToWorldPosition({ 0.f, (1 - getSuspTargetPos()) * getSuspensionDistance(), 0.f });
//			m_pVehicleNoDrive->changeWheelActorCenterOffset(m_wheelId, computeWheelCenterRigidBodyOffset(m_pAttachRigidBody.get(), this));
//			RIGIDBODY_WAKE_UP();
//		}
//		else
//		{
//			if (m_pAttachStatic) m_pAttachStatic->SetTransform(getWorldPosition(), getWorldRotation());
//		}
//	}
//
//	//void WheelColliderComponent::ChangeIsTrigger()
//	//{
//	//	GET_DATA()->isTrigger = false;
//	//}
//
//	void WheelColliderComponent::ChangeFilterGroup()
//	{
//		GET_DATA()->filterGroupType = gWheelGroup;
//		GET_DATA()->canInteractFilterGroupType = gWheelCanInteractorGroup;
//		if (!IsExistPhysics()) return;
//		m_pShape->SetFilterData(gWheelGroup, gWheelCanInteractorGroup);
//	}
//
//	void WheelColliderComponent::SetCompData(ColliderComponentData* pData)
//	{
//		if (!pData)
//			return;
//		SetCompDataBase(pData);
//		AddToEditorUpdateQueue();
//
//		WheelColliderComponentData* pWCData = static_cast<WheelColliderComponentData*> (pData);
//		setMass(pWCData->mass);
//		setRadius(pWCData->radius);
//		setWheelDampingRate(pWCData->wheelDampingRate);
//		setSuspensionDistance(pWCData->suspensionDistance);
//		setForceAppPointDistance(pWCData->forceAppPointDistance);
//		setSuspSpring(pWCData->suspSpring);
//		setSuspDamper(pWCData->suspDamper);
//		setSuspTargetPos(pWCData->suspTargetPos);
//		setForwardFriction(pWCData->forwardFriction);
//		//SetSidewaysFriction(pWCData->sidewaysFriction);
//	}
//
//	void WheelColliderComponent::setMass(float mass)
//	{
//		GET_DATA()->mass = mass;
//
//		if (IsExistPhysics() && m_pVehicleNoDrive)
//			m_pVehicleNoDrive->changeWheelMass(m_wheelId, mass);
//		RIGIDBODY_WAKE_UP();
//	}
//
//	float WheelColliderComponent::getMass()
//	{
//		return GET_DATA()->mass;
//	}
//
//	void WheelColliderComponent::setRadius(float radius)
//	{
//		GET_DATA()->radius = radius;
//		AddToEditorUpdateQueue();
//		ChangeShape();	// 此处会修改车辆参数
//		RIGIDBODY_WAKE_UP();
//	}
//
//	float WheelColliderComponent::getRadius()
//	{
//		return GET_DATA()->radius;
//	}
//
//	void WheelColliderComponent::setWheelDampingRate(float dampingRate)
//	{
//		GET_DATA()->wheelDampingRate = dampingRate;
//		if (IsPhysicsStaut())
//			m_pVehicleNoDrive->changeWheelDampingRate(m_wheelId, dampingRate);
//		RIGIDBODY_WAKE_UP();
//	}
//
//	float WheelColliderComponent::getWheelDampingRate()
//	{
//		return GET_DATA()->wheelDampingRate;
//	}
//
//	void WheelColliderComponent::setSuspensionDistance(float distance)
//	{
//		GET_DATA()->suspensionDistance = distance;
//		AddToEditorUpdateQueue();
//
//		if (IsPhysicsStaut())
//		{
//			float maxCompression, maxDroop;
//			maxDroop = getRealSuspensionDistance() * getSuspTargetPos();
//			maxCompression = getRealSuspensionDistance() - maxDroop;
//			m_pVehicleNoDrive->changeMaxCompressionAndMaxDroop(m_wheelId, maxCompression, maxDroop);
//		}
//		RIGIDBODY_WAKE_UP();
//	}
//
//	float WheelColliderComponent::getSuspensionDistance()
//	{
//		return GET_DATA()->suspensionDistance;
//	}
//
//	void WheelColliderComponent::setForceAppPointDistance(float distance)
//	{
//		GET_DATA()->forceAppPointDistance = distance;
//
//		AddToEditorUpdateQueue();
//
//		if (IsPhysicsStaut())
//			m_pVehicleNoDrive->changeForceAppPos(m_wheelId, getRealForceAppPointDistance());
//
//		RIGIDBODY_WAKE_UP();
//	}
//
//	float WheelColliderComponent::getForceAppPointDistance()
//	{
//		return GET_DATA()->forceAppPointDistance;
//	}
//
//	void WheelColliderComponent::setSuspSpring(float spring)
//	{
//		GET_DATA()->suspSpring = spring;
//		if (IsPhysicsStaut())
//			m_pVehicleNoDrive->changeSuspSpring(m_wheelId, spring);
//		RIGIDBODY_WAKE_UP();
//	}
//
//	float WheelColliderComponent::getSuspSpring()
//	{
//		return GET_DATA()->suspSpring;
//	}
//
//	void WheelColliderComponent::setSuspDamper(float damper)
//	{
//		GET_DATA()->suspDamper = damper;
//		if (IsPhysicsStaut())
//			m_pVehicleNoDrive->changeSuspDamper(m_wheelId, damper);
//		RIGIDBODY_WAKE_UP();
//	}
//
//	float WheelColliderComponent::getSuspDamper()
//	{
//		return GET_DATA()->suspDamper;
//	}
//
//	void WheelColliderComponent::setSuspTargetPos(float pos)
//	{
//		if (pos > 1.f || pos < 0.f)
//			return;
//
//		GET_DATA()->suspTargetPos = pos;
//
//		AddToEditorUpdateQueue();
//		if (IsPhysicsStaut())
//		{
//			float maxCompression, maxDroop;
//			float suspDis = getRealSuspensionDistance();
//			maxCompression = (1 - pos) * suspDis;
//			maxDroop = pos * suspDis;
//			m_pVehicleNoDrive->changeMaxCompressionAndMaxDroop(m_wheelId, maxCompression, maxDroop);
//		}
//		RIGIDBODY_WAKE_UP();
//	}
//
//	float WheelColliderComponent::getSuspTargetPos()
//	{
//		return GET_DATA()->suspTargetPos;
//	}
//
//	void WheelColliderComponent::setForwardFriction(const EchoWheelFriction& friction)
//	{
//		GET_DATA()->forwardFriction = friction;
//		if (IsPhysicsStaut())
//			m_pVehicleNoDrive->changeForwardFrition(m_wheelId, EWF_TO_PWF(friction));
//		RIGIDBODY_WAKE_UP();
//	}
//
//	EchoWheelFriction WheelColliderComponent::getForwardFriction()
//	{
//		return GET_DATA()->forwardFriction;
//	}
//
//	//void WheelColliderComponent::SetSidewaysFriction(const EchoWheelFriction& friction)
//	//{
//	//	GET_DATA()->sidewaysFriction = friction;
//	//	if (IsPhysicsStaut())
//	//		m_pVehicleNoDrive->changeSidewaysFriction(m_wheelId, EWF_TO_PWF(friction));
//	//	RIGIDBODY_WAKE_UP();
//	//}
//
//	//EchoWheelFriction WheelColliderComponent::getSidewaysFriction()
//	//{
//	//	return GET_DATA()->sidewaysFriction;
//	//}
//
//	float WheelColliderComponent::getRealRadius()
//	{
//		return Math::Abs(getRadius() * getWorldScale().y);
//	}
//
//	float WheelColliderComponent::getRealWidth()
//	{
//		return  Math::Abs(getRadius() * getWorldScale().x * 0.8f);
//	}
//
//	float WheelColliderComponent::getRealSuspensionDistance()
//	{
//		return Math::Abs(getSuspensionDistance() * getWorldScale().y);
//	}
//
//	float WheelColliderComponent::getRealForceAppPointDistance()
//	{
//		return getForceAppPointDistance() * Math::Abs(getWorldScale().y);
//	}
//
//	void WheelColliderComponent::getWheelCenterWorldPostion(Vector3& postion, Quaternion& rotate)
//	{
//		if (!IsPhysicsStaut())
//			return;
//		Transform transform;
//		m_pShape->GetLocalPose(transform);
//		postion = transform.translation;
//		rotate = transform.rotation;
//	}
//
//	bool WheelColliderComponent::isInAir()
//	{
//		if (!IsPhysicsStaut()) return false;
//		return m_pVehicleNoDrive->isInAir(m_wheelId);
//	}
//
//	Vector3 WheelColliderComponent::tireContactPoint()
//	{
//		if (!IsPhysicsStaut()) return Vector3();
//		return m_pVehicleNoDrive->getTireContactPoint(m_wheelId);
//	}
//
//	Vector3 WheelColliderComponent::tireContactNormal()
//	{
//		if (!IsPhysicsStaut()) return Vector3();
//		return m_pVehicleNoDrive->getTireContactNormal(m_wheelId);
//	}
//
//	float WheelColliderComponent::tireFrition()
//	{
//		if (!IsPhysicsStaut()) return 0.0f;
//		return m_pVehicleNoDrive->getTireFirction(m_wheelId);
//	}
//
//	float WheelColliderComponent::tirePos()
//	{
//		if (!IsPhysicsStaut()) return 0.0f;
//		float jounce = m_pVehicleNoDrive->getSuspJounce(m_wheelId);
//
//		return (jounce + getSuspTargetPos() * getRealSuspensionDistance()) / getRealSuspensionDistance();
//	}
//
//	Vector3 WheelColliderComponent::tireLongitudinalDir()
//	{
//		if (!IsPhysicsStaut()) return Vector3();
//		return m_pVehicleNoDrive->getTireLongitudinalDir(m_wheelId);
//	}
//
//	Vector3 WheelColliderComponent::tireLateralDir()
//	{
//		if (!IsPhysicsStaut()) return Vector3();
//		return m_pVehicleNoDrive->getTireLateralDir(m_wheelId);
//	}
//
//	float WheelColliderComponent::longitudinalSlip()
//	{
//		if (!IsPhysicsStaut()) return 0.0f;
//		return m_pVehicleNoDrive->getLongitudinalSlip(m_wheelId);
//	}
//
//	float WheelColliderComponent::lateralSlip()
//	{
//		if (!IsPhysicsStaut()) return 0.0f;
//		return m_pVehicleNoDrive->getLateralSlip(m_wheelId);
//	}
//
//	float WheelColliderComponent::rpm()
//	{
//		if (!IsPhysicsStaut())return 0.0f;
//		return m_pVehicleNoDrive->getRadiansPerSecond(m_wheelId) * 60.f;
//	}
//
//	float WheelColliderComponent::rotationAngle()
//	{
//		if (!IsPhysicsStaut())return 0.0f;
//		return m_pVehicleNoDrive->getWheelRotationAngle(m_wheelId);
//	}
//
//	//float WheelColliderComponent::steerAngle()
//	//{
//	//	if (!IsExistPhysics()) return 0.0f;
//	//	return m_pVehicleNoDrive->getSteerAngle(m_wheelId);
//	//}
//
//	void WheelColliderComponent::setBrakeTorque(float torque)
//	{
//		if (IsPhysicsStaut())
//			m_pVehicleNoDrive->setBrakeTorque(m_wheelId, torque);
//		RIGIDBODY_WAKE_UP();
//	}
//
//	void WheelColliderComponent::setDriveTorque(float torque)
//	{
//		if (IsPhysicsStaut())
//			m_pVehicleNoDrive->setDriveTorque(m_wheelId, torque);
//		RIGIDBODY_WAKE_UP();
//	}
//
//	void WheelColliderComponent::setSteerAngle(float angle)
//	{
//		if (IsPhysicsStaut())
//			m_pVehicleNoDrive->setSteerAngle(m_wheelId, angle);
//		RIGIDBODY_WAKE_UP();
//	}
//
//	float WheelColliderComponent::getBrakeTorque()
//	{
//		if (IsPhysicsStaut())
//			return m_pVehicleNoDrive->getSteerAngle(m_wheelId);
//		return 0.0f;
//	}
//
//	float WheelColliderComponent::getDriveTorque()
//	{
//		if (IsPhysicsStaut())
//			return m_pVehicleNoDrive->getDriveTorque(m_wheelId);
//		return 0.0f;
//	}
//
//	float WheelColliderComponent::getSteerAngle()
//	{
//		if (IsPhysicsStaut())
//			return m_pVehicleNoDrive->getSteerAngle(m_wheelId);
//		return 0.0f;
//	}
//
//	WheelColliderComponentInfo::WheelColliderComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) :ColliderComponentInfo(comType, MoveType)
//	{
//	}
//
//	WheelColliderComponentInfo::~WheelColliderComponentInfo()
//	{
//	}
//
//	void WriteJSNode(JSNODE pRoot, const char* name, const EchoWheelFriction& friction)
//	{
//		WriteJSNode(pRoot, (name + std::string(".extremumSlip")).c_str(), friction.extremumSlip);
//		WriteJSNode(pRoot, (name + std::string(".extremumValue")).c_str(), friction.extremumValue);
//		WriteJSNode(pRoot, (name + std::string(".asymptoteSlip")).c_str(), friction.asymptoteSlip);
//		WriteJSNode(pRoot, (name + std::string(".asymptoteValue")).c_str(), friction.asymptoteValue);
//		WriteJSNode(pRoot, (name + std::string(".stiffness")).c_str(), friction.stiffness);
//	}
//	bool ReadJSNode(JSNODE pRoot, const char* name, EchoWheelFriction& friction)
//	{
//		bool res = true;
//		res &= ReadJSNode(pRoot, (name + std::string(".extremumSlip")).c_str(), friction.extremumSlip);
//		res &= ReadJSNode(pRoot, (name + std::string(".extremumValue")).c_str(), friction.extremumValue);
//		res &= ReadJSNode(pRoot, (name + std::string(".asymptoteSlip")).c_str(), friction.asymptoteSlip);
//		res &= ReadJSNode(pRoot, (name + std::string(".asymptoteValue")).c_str(), friction.asymptoteValue);
//		res &= ReadJSNode(pRoot, (name + std::string(".stiffness")).c_str(), friction.stiffness);
//		return res;
//	}
//
//
//	EDITOR_REFLECT_BEGIN(WheelColliderComponentInfo)
//	EDITOR_REFLECT_MEMBER_UPDATE(m_Mass, "mass", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
//	EDITOR_REFLECT_MEMBER_UPDATE(m_Radius, "radius", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
//	EDITOR_REFLECT_MEMBER_UPDATE(m_WheelDampingRate, "wheelDampingRate", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
//	EDITOR_REFLECT_MEMBER_UPDATE(m_SuspensionDistance, "suspensionDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
//	EDITOR_REFLECT_MEMBER_UPDATE(m_ForceAppPointDistance, "forceAppPointDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
//	EDITOR_REFLECT_MEMBER_UPDATE(m_SuspSpring, "suspSpring", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
//	EDITOR_REFLECT_MEMBER_UPDATE(m_SuspDamper, "suspDamper", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
//	EDITOR_REFLECT_MEMBER_UPDATE(m_SuspTargetPos, "suspTargetPos", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
//	EDITOR_REFLECT_MEMBER_UPDATE(m_ForwardFriction, "forwardFriction", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
//	EDITOR_REFLECT_END();
//	_DEFINEINFO_VIRFUN(WheelColliderComponentInfo, ColliderComponentInfo)
//
//	void WheelColliderComponentInfo::CopyToData(ColliderComponentData* pData)
//	{
//		if (!pData)
//			return;
//		ColliderComponentInfo::CopyToData(pData);
//
//		WheelColliderComponentData* wheelData = static_cast<WheelColliderComponentData*>(pData);
//
//		wheelData->mass = GetMass();
//		wheelData->radius = GetRadius();
//		wheelData->wheelDampingRate = GetWheelDampingRate();
//		wheelData->suspensionDistance = GetSuspensionDistance();
//		wheelData->forceAppPointDistance = GetForceAppPointDistance();
//		wheelData->suspSpring = GetSuspSpring();
//		wheelData->suspDamper = GetSuspDamper();
//		wheelData->suspTargetPos = GetSuspTargetPos();
//		wheelData->forwardFriction = GetForwardFriction();
//		//wheelData->sidewaysFriction = GetsidewaysFriction();
//	}
//
//	WheelColliderComponent* EchoWheelColliderScript::Script_GetWheelColliderComponent(ActorPtr pActor, const std::string& name)
//	{
//		ProtocolComponentManager* pMgr = ProtocolComponentManager::instance();
//		ProtocolHandle comHandle = 0;
//		ActorComTypeDesc desc;
//		desc.actHandle = (Echo::ProtocolHandle)pActor.get();
//		desc.comType = COMPONENTTYPE(WheelColliderComponent);
//		desc.comName = name;
//		ProtocolHandle descHandle = (ProtocolHandle)&desc;
//		ProtocolResult result = pMgr->callProtocolFunc(PFID_Actor_GetComponent, &comHandle, descHandle);
//
//		if (result != PR_OK)
//			return nullptr;
//		if (comHandle == 0)
//			return nullptr;
//
//		return  (WheelColliderComponent*)comHandle;
//	}
//
//	Vector3 computeWheelCenterRigidBodyOffset(RigidBodyComponent* pRB, WheelColliderComponent* pWC)
//	{
//		Vector3 wheelWorldCenter = pWC->convertLocalToWorldPosition({ 0, -(1 - pWC->getSuspTargetPos()) * pWC->getSuspensionDistance(), 0 });
//		return 	pRB->getWorldRotation().Inverse() * (wheelWorldCenter - pRB->getWorldPosition());
//	}
//
//}


