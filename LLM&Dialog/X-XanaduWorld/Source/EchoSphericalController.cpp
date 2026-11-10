#include "Actor/EchoActorManager.h"
#include "EchoCharactorControllerComponent.h"
#include "EchoSphericalController.h"
#include "EchoCameraControl.h"
#include "EchoSphericalTerrainComponent.h"
#include "Actor/EchoNodeComponent.h"
#include "EchoActorRoleComponent.h"
#include "EchoSphericalTerrainManager.h"
#include "EchoIKGrounderFBBIKComponent.h"
#include "EchoAreaManager.h"
#include "EchoWorldManager.h"

using namespace Echo;

class SphCctControllerCallBack : public EchoControllerCallBack {
public:
	static EchoControllerCallBack* instance() {
		static SphCctControllerCallBack gInstance;
		return &gInstance;
	}

	//返回是否缓存是否失效
	virtual bool OnShapeHit(const EchoControllerShapeHitParam& param) override { return false; };

	//返回是否缓存是否失效
	virtual bool OnControllerHit(const  EchoControllersHitParam& param) override { return false; };

	//返回控制器行为
	virtual  EchoControllerBehaviorFlag GetBehaviorFlag(const EchoControllerShapeBehaviorParam& param) override { return EchoControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT; }

	//返回控制器行为
	virtual EchoControllerBehaviorFlag GetBehaviorFlag(const EchoControllersBehaviorParam& param) override { return EchoControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT; }
};

void EchoSphericalController::SetCharlPosition(const Echo::Vector3& position) {
	if (m_pCharl) {
		m_pCharl->getParentComponentNode()->setWorldPosition(position);
		UpdateCamera();
	}
}

const Echo::Vector3 EchoSphericalController::GetCharlPosition()
{
	
	if (m_pCharl) {
		return m_pCharl->getWorldPosition();
	}
	return Echo::Vector3(0.0);
}

void EchoSphericalController::SetMaxSpeedMul(float value) {
	m_fMaxSpeedMul = Math::Max(1.0f, value);
}

Echo::ActorPtr EchoSphericalController::createSphericalTerrain(const Echo::String& terrainName, const Echo::Vector3& origin, float scale) {
	auto actorPtr = ActorSystem::instance()->getMainActorManager()->createActor();
	actorPtr->setActorID("SphericalTerrainController");
	actorPtr->setWorldPosition(origin);
	actorPtr->setWorldScale(Vector3(scale));
	auto terrain = actorPtr->AddComponent<SphericalTerrainComponent>("terrain", actorPtr->getRootNodeCom());
	String terrainPath = "biome_terrain/" + terrainName + ".terrain";
	terrain->SetIsUsePhysics(true);
	terrain->SetTerrainPath(terrainPath);
	return actorPtr;
}

EchoSphericalController::EchoSphericalController(Echo::Camera* camera) {
	m_vLastMove = Vector3::ZERO;
	m_vCameraLocalPosition = Vector3(0.f, 3.0f, -4.0f).GetNormalized() * 10.0f;
	m_vCameraLocalRotation.FromEuler(Vector3(-20.0f, 180.0f, 0.0f));
	if (camera != nullptr)
		m_pCamera = camera;
	else {
		LogManager::instance()->logMessage("Error::EchoSphericalController::camera is nullptr!");
		return;
	}
    Echo::CameraControl::instance()->setMoveSpeed(5.0f);
	Vector3 pos = Vector3::ZERO;
	Quaternion rot = Quaternion::IDENTITY;
	{
		Quaternion cameraQuat;
		cameraQuat.FromEuler(Vector3(0.0f, 180.0f, 0.0f));
		cameraQuat = m_pCamera->getOrientation() * cameraQuat;

		Vector3 forward = cameraQuat * Vector3::FORWARD;
		forward = Vector3::ProjectOnPlane(forward, Vector3::UP);
		forward.normalise();
		rot = Quaternion::FromToRotation(Vector3::FORWARD, forward);

		Quaternion quat = Quaternion::IDENTITY;
		Vector3 forF = rot * Vector3::FORWARD;
		Vector3 forT = cameraQuat * Vector3::FORWARD;
		float angle = Vector3::Angle(forF, forT);
		if (forT.y > 0.0f) {
			angle = -angle;
		}
		quat = quat * Quaternion::AngleAxis(20.0f - angle, Vector3::RIGHT);

		m_vCameraLocalPosition = quat * m_vCameraLocalPosition;
		m_vCameraLocalRotation = quat * m_vCameraLocalRotation;

		pos = m_pCamera->getPosition() - rot * m_vCameraLocalPosition;
	}
	m_TestActorPtr = ActorSystem::instance()->getMainActorManager()->createActor();
	m_TestActorPtr->setActorID("SphericalTerrainController");
	m_TestActorPtr->setWorldPosition(pos);
    //m_TestActorPtr->setWorldRotation(rot);
	m_pCharl = m_TestActorPtr->AddComponent<CharatorControllerComponent>("cct", m_TestActorPtr->getRootNodeCom());
	m_pCharl->SetCenter(Vector3::ZERO);
	m_pCharl->SetCenterModel(CCTCenterModel::CCT_Bottom);
	m_pCharl->SetSlopeLimit(70.0f);
	m_pCharl->SetFilterGroupType(ePhysXGroup_4);
	m_pCharl->SetControllerCallBack(SphCctControllerCallBack::instance());

	m_pRoleNode = m_TestActorPtr->AddComponent<NodeComponent>("roleNode", m_TestActorPtr->getRootNodeCom());
	auto role = m_TestActorPtr->AddComponent<RoleComponent>("role", m_pRoleNode);
	role->ChangeRole("role/common/common_a3/common_a3.role");
	role->GetRole()->getSkeletonComponent()->changeEquip(Echo::Name("a3_dze_cf"), false);

	m_pCharl->GetPhysicsManager()->SetPhysXPageLoadMainActor(m_pCharl->getParentNodeCom());

	auto ik = m_TestActorPtr->AddComponent<IKGrounderFBBIKComponent>("ik", m_pRoleNode);
	ik->SetRoleCompName("role");
	ik->UseDefaultInitialize();
	GroundingInfo groundingInfo = ik->GetSolver();
	groundingInfo.pelvisDamper = 0.0f;
	ik->SetSolver(groundingInfo);
	ik->CreateIK();

	m_pCameraNode = m_TestActorPtr->AddComponent<NodeComponent>("cameraNode", m_TestActorPtr->getRootNodeCom());
	m_pUpdateNode = m_TestActorPtr->AddComponent<NodeComponent>("updateNode", m_TestActorPtr->getRootNodeCom());
	m_pUpdateNode->setWorldRotation(rot);
	m_pCameraNode->setWorldRotation(rot);
	m_pRoleNode->setWorldRotation(rot);
	UpdateCamera();
}

EchoSphericalController::EchoSphericalController(Camera* camera, const String& terrainName, const Vector3& origin) {
	m_vLastMove = Vector3::ZERO;
	m_vCameraLocalPosition = Vector3(0.f, 3.0f, -4.0f);
	m_vCameraLocalRotation.FromEuler(Vector3(-20.0f, 180.0f, 0.0f));
	if (camera != nullptr)
		m_pCamera = camera;
	else {
		LogManager::instance()->logMessage("Error::EchoSphericalController::camera is nullptr!");
		return;
	}

	m_TestActorPtr = ActorSystem::instance()->getMainActorManager()->createActor();
	m_TestActorPtr->setActorID("SphericalTerrainController");
	m_TestActorPtr->setWorldPosition(origin);
	auto cctNode = m_TestActorPtr->addComponent(NodeComponent::mType, "cctNode", m_TestActorPtr->getRootNodeCom());
	m_pCharl = m_TestActorPtr->AddComponent<CharatorControllerComponent>("cct", cctNode);
	m_pCharl->SetCenter(Vector3::ZERO);
	m_pCharl->SetCenterModel(CCTCenterModel::CCT_Bottom);
	m_pCharl->SetSlopeLimit(70.0f);
	m_pCharl->SetFilterGroupType(ePhysXGroup_4);
	m_pCharl->SetControllerCallBack(SphCctControllerCallBack::instance());
	m_pCharl->getParentComponentNode()->setPosition(DVector3(0.0, 1000.0, 0.0));

	m_pRoleNode = m_TestActorPtr->AddComponent<NodeComponent>("roleNode", cctNode);
	auto role = m_TestActorPtr->AddComponent<RoleComponent>("role", m_pRoleNode);
	role->ChangeRole("role/common/common_a3/common_a3.role");
	role->GetRole()->getSkeletonComponent()->changeEquip(Echo::Name("a3_dze_cf"), false);

	m_pCameraNode = m_TestActorPtr->AddComponent<NodeComponent>("cameraNode", cctNode);
	m_pUpdateNode = m_TestActorPtr->AddComponent<NodeComponent>("updateNode", cctNode);

	auto terrainNode = m_TestActorPtr->addComponent(NodeComponent::mType, "terrainNode", m_TestActorPtr->getRootNodeCom());
	auto terrain = m_TestActorPtr->AddComponent<SphericalTerrainComponent>("terrain", terrainNode);
	String terrainPath = "biome_terrain/" + terrainName + ".terrain";
	terrain->SetIsUsePhysics(true);
	terrain->SetTerrainPath(terrainPath);

	m_pCharl->GetPhysicsManager()->SetPhysXPageLoadMainActor(m_pCharl->getParentNodeCom());

	auto ik = m_TestActorPtr->AddComponent<IKGrounderFBBIKComponent>("ik", m_pRoleNode);
	ik->SetRoleCompName("role");
	ik->UseDefaultInitialize();
	GroundingInfo groundingInfo = ik->GetSolver();
	groundingInfo.pelvisDamper = 0.0f;
	ik->SetSolver(groundingInfo);
	ik->CreateIK();

	UpdateCamera();
}

EchoSphericalController::~EchoSphericalController()
{
	if (m_pCamera) {
		Vector3 dir = m_pCamera->getDirection();
		m_pCamera->setOrientation(Quaternion::IDENTITY);
		m_pCamera->setDirection(dir);
	}
	if (m_pCharl) {
		m_pCharl->SetUpDirection(Vector3::UP);
		m_pCharl->getParentComponentNode()->setWorldRotation(Quaternion::IDENTITY);
		m_pCharl->GetPhysicsManager()->SetPhysXPageLoadMainActor(ActorComponent::NULLPTR);
	}
	ActorSystem::instance()->getMainActorManager()->deleteActor(m_TestActorPtr);
}


void EchoSphericalController::update(int type, float valx, float valy)
{
	if (!m_pCamera || !m_pCharl)
		return;

	if (type == 0)
	{
		if (valx == 0.0f) {
			m_vLastMove.x = valy * -1.0f;
		}
		else if (valx == 2.0f) {
			m_vLastMove.y = valy * 1.0f;
		}
		else if (valx == 1.0f) {
			m_vLastMove.z = valy;
		}
		else if (valx == 3.0f) {
			if (valy > 0.0f) {
				m_fSpeedMul = Math::Min(m_fSpeedMul + 0.2f, m_fMaxSpeedMul);
			}
			else {
				m_fSpeedMul = 1.0f;
			}
		}
	}
	if (type == 1)
	{
		float dy = valy * 0.1f;
		float dx = valx * 0.1f;

		if (abs(dx) > 0.01f) {
			Vector3 upDir = Vector3::UP;
			Quaternion upRot = Quaternion::AngleAxis(dx, upDir);
			m_pUpdateNode->setWorldRotation(m_pUpdateNode->getWorldRotation() * upRot);
			m_pCameraNode->setWorldRotation(m_pCameraNode->getWorldRotation() * upRot);
			m_pRoleNode->setWorldRotation(m_pRoleNode->getWorldRotation() * upRot);
		}
		if (abs(dy) > 0.01f) {
			Vector3 rightDir = Vector3::RIGHT;
			Quaternion rightRot = Quaternion::AngleAxis(dy, rightDir);
			m_vCameraLocalPosition = rightRot * m_vCameraLocalPosition;
			m_vCameraLocalRotation = rightRot * m_vCameraLocalRotation;
		}
		UpdateCamera();
	}
	if (type == 2) {
		float length = m_vCameraLocalPosition.Length();
		if (valy > 0.0f) {
			length -= 1.0f;
			if (length < 4.0f) {
				length = 4.0f;
			}
		}
		else {
			length += 1.0f;
			if (length > 30.0f) {
				length = 30.0f;
			}
		}
		m_vCameraLocalPosition = m_vCameraLocalPosition.GetNormalized() * length;
		UpdateCamera();
	}
	if (type == 3) {
		if (!m_bJump && m_CurState != 1) {
			m_fHeightBeforeJumping = (float)m_pCharl->getWorldPosition().y;
			m_bJumpState = true;
			m_bJump = true;
			m_vLastMove.y = 1.0f;
		}
	}
}


void EchoSphericalController::tick() {
	if (m_bFly) {
		tickFly();
	}
	else {
		tickWalk();
	}
}

void EchoSphericalController::tickWalk() {
	if (!m_pCamera || !m_pCharl)
		return;
	EchoPhysicsManager* pMgr = m_pCharl->GetPhysicsManager();
	Vector3 pos1 = m_pCharl->getWorldPosition();

	float upSpeed {};
    if(m_bJump) {
        if (pos1.y - m_fHeightBeforeJumping >= m_fJumpingHeight) {
            m_bJump = false;
            m_vLastMove.y = 0.0f;
        }
        upSpeed = -m_fGravity * 3.0f * Echo::Root::instance()->getTimeSinceLastFrameClient();
    }
    else if(m_bJumpState){
        upSpeed = m_fGravity * 3.0f * Echo::Root::instance()->getTimeSinceLastFrameClient();
        if (pos1.y <= m_fHeightBeforeJumping || (m_CurState != 1 && m_CurState != 8)) {
            m_bJumpState = false;
        }
    }else{
        upSpeed = m_fGravity * 30.0f * Echo::Root::instance()->getTimeSinceLastFrameClient();
    }

	float dirSpeed = CameraControl::instance()->getMoveSpeed() * Echo::Root::instance()->getTimeSinceLastFrameClient() * m_fSpeedMul;
	Vector3 move = m_pUpdateNode->getWorldRotation() * m_vLastMove.GetNormalized() * dirSpeed - m_pCharl->GetUpDirection() * upSpeed;
	m_pCharl->Move(Echo::Root::instance()->getTimeSinceLastFrameClient(), move, true);

	Vector3 upDir = Vector3::UP;
	Vector3 cctPos = m_pCharl->getWorldPosition();

	SphericalTerrain* pCurrentTerrain = nullptr;
	{
		SphericalTerrain* pTerrain = m_pCharl->getSceneManager()->getPlanetManager()->getNearestPlanet();
		while (pTerrain)
		{
			if (!pTerrain->isCreateFinish()) break;
			if (!pTerrain->getIsNearbyWs(cctPos)) break;

			upDir           = -pTerrain->getGravityWs(cctPos);
			pCurrentTerrain = pTerrain;
			break;
		}
	}
	Vector3 dif = m_pCharl->GetUpDirection() - upDir;
	if (!dif.isEqual(Vector3::ZERO, 1e-4f)) {
		m_pCharl->SetUpDirection(upDir);
		{
			Vector3 forwardDir = m_pUpdateNode->getWorldRotation() * Vector3::UNIT_Z;
			Vector3::OrthoNormalize(upDir, forwardDir);
			Quaternion rot = Quaternion::LookRotation(forwardDir, upDir);
			m_pUpdateNode->setWorldRotation(rot);
		}
	}

	{
		Quaternion updateRot = m_pUpdateNode->getWorldRotation();
		Vector3 updateDir = updateRot * Vector3::UP;

		auto smoothRotation = [&](Echo::EchoWeakPtr<Echo::NodeComponent>& _Node, float speed) {
			if (speed > 0.0f) {
				float maxAngle = speed * Echo::Root::instance()->getTimeSinceLastFrameClient();
				Quaternion roleRot = _Node->getWorldRotation();
				Vector3 roleDir = roleRot * Vector3::UP;
				float angle = Math::Abs(Vector3::Angle(roleDir, updateDir));
				if (angle <= maxAngle) {
					_Node->setWorldRotation(updateRot);
				}
				else {
					_Node->setWorldRotation(Quaternion::QSlerp(roleRot, updateRot, maxAngle / angle));
				}
			}
			else {
				_Node->setWorldRotation(updateRot);
			}
		};
		smoothRotation(m_pRoleNode, m_fRoleAngleSpeed);
		smoothRotation(m_pCameraNode, m_fCameraAngleSpeed);

	}

	UpdateCamera();

	if (pCurrentTerrain) {
		Vector3 faceNormal;
		Vector3 surfacePos = pCurrentTerrain->getSurfaceFinestWs(cctPos, &faceNormal);
		Vector3 posNor = cctPos - surfacePos;
		if (m_pCharl->GetUpDirection().dotProduct(posNor) < 0.0f && posNor.LengthSq() > 0.1f) {
			cctPos = surfacePos;
			SetCharlPosition(cctPos);
		}
	}

	Vector3 pos2 = m_pCharl->getWorldPosition();
	auto role = m_pCharl->getActor()->GetFirstComByType<RoleComponent>();
	if (role != nullptr) {
		int newState = 0;
		Vector3 posOffset = pos2 - pos1;
		if (!posOffset.AlmostZero()) {
			if (!m_bJump && !m_pCharl->GetPhysicsManager()->Raycast(m_pCharl->GetWorldFootPosition(false), -m_pCharl->GetUpDirection(), 0.2f, ~ePhysXGroup_4)) {
				newState = 1;
			}
			else {
				if (m_vLastMove.z >= 0.0f) {
					if (m_bJump) {
						newState = 8;
					}
					else if (m_vLastMove.x > 0.0f) {
						newState = 2;
					}
					else if(m_vLastMove.x < 0.0f) {
						newState = 4;
					}
					else {
						if (m_vLastMove.z == 0.0f) {
							newState = 0;
						}
						else {
							newState = 3;
						}
					}
				}
				else if(m_vLastMove.z < 0.0f) {
					if (m_bJump) {
						newState = 8;
					}
					else if (m_vLastMove.x > 0.0f) {
						newState = 5;
					}
					else if (m_vLastMove.x < 0.0f) {
						newState = 7;
					}
					else {
						newState = 6;
					}
				}
			}
		}
		if (newState != m_CurState) {
			m_CurState = newState;
			switch (m_CurState) {
			case 0: {
				role->SetRoleAnimation(Echo::Name("zc_tp_dm_stand_xx_01_ca"), true, 0u);
				role->SetRoleAnimation(Echo::Name("zc_tp_dm_stand_xx_01_tk"), true, 1u);
				break;
			}
			case 1: {
				role->SetRoleAnimation(Echo::Name("zc_tp_kz_fly_down_01_ca"), true, 0u);
				role->SetRoleAnimation(Echo::Name("zc_tp_kz_fly_down_01_tk"), true, 1u);
				break;
			}
			case 2: {
				role->SetRoleAnimation(Echo::Name("zc_tp_dm_stand_run_lf_01_ca"), true, 0u);
				role->SetRoleAnimation(Echo::Name("zc_tp_dm_stand_run_lf_01_tk"), true, 1u);
				break;
			}
			case 3: {
				role->SetRoleAnimation(Echo::Name("zc_tp_dm_stand_run_f_01_ca"), true, 0u);
				role->SetRoleAnimation(Echo::Name("zc_tp_dm_stand_run_f_01_tk"), true, 1u);
				break;
			}
			case 4: {
				role->SetRoleAnimation(Echo::Name("zc_tp_dm_stand_run_rf_01_ca"), true, 0u);
				role->SetRoleAnimation(Echo::Name("zc_tp_dm_stand_run_rf_01_tk"), true, 1u);
				break;
			}
			case 5: {
				role->SetRoleAnimation(Echo::Name("zc_tp_dm_stand_run_lb_01_ca"), true, 0u);
				role->SetRoleAnimation(Echo::Name("zc_tp_dm_stand_run_lb_01_tk"), true, 1u);
				break;
			}
			case 6: {
				role->SetRoleAnimation(Echo::Name("zc_tp_dm_stand_run_b_01_ca"), true, 0u);
				role->SetRoleAnimation(Echo::Name("zc_tp_dm_stand_run_b_01_tk"), true, 1u);
				break;
			}
			case 7: {
				role->SetRoleAnimation(Echo::Name("zc_tp_dm_stand_run_rb_01_ca"), true, 0u);
				role->SetRoleAnimation(Echo::Name("zc_tp_dm_stand_run_rb_01_tk"), true, 1u);
				break;
			}
			case 8: {
				role->SetRoleAnimation(Echo::Name("zc_tp_dm_stand_jump_01-4_ca"), false, 0u);
				role->SetRoleAnimation(Echo::Name("zc_tp_dm_stand_jump_01-4_tk"), false, 1u);
			}
			}
		}
	}

	String newTemplateTOD;
	if (pCurrentTerrain) {
		int regionIndex = pCurrentTerrain->computeRegionInformation(cctPos);
		if (regionIndex > -1 && regionIndex < (int)pCurrentTerrain->m_terrainData->region.biomeArray.size()) {
			newTemplateTOD = pCurrentTerrain->m_terrainData->region.biomeArray[regionIndex].TemplateTOD;
		}
	}
	if (m_sCurTemplateTOD != newTemplateTOD) {
		m_sCurTemplateTOD = newTemplateTOD;
		WorldManager* pWorldManager = m_pCharl->getWorldManager();
		AreaManager* pAreaManager = pWorldManager ? pWorldManager->getAreaManager() : nullptr;
		if (pAreaManager) {
			pAreaManager->applySceneArtTemplate(m_sCurTemplateTOD);
		}
	}
}

void EchoSphericalController::tickFly() {
	if (!m_pCamera || !m_pCharl)
		return;
	EchoPhysicsManager* pMgr = m_pCharl->GetPhysicsManager();

	float dirSpeed = CameraControl::instance()->getMoveSpeed() * Echo::Root::instance()->getTimeSinceLastFrameClient() * m_fSpeedMul;
	Vector3 move = (m_pCamera->getOrientation() * Vector3(-m_vLastMove.x, 0.0f, -m_vLastMove.z) + m_pUpdateNode->getWorldRotation() * Vector3(0.0f, m_vLastMove.y, 0.0f)).GetNormalized() * dirSpeed;

	DVector3 cctPos = DVector3::ZERO;

	if (m_bPhysX) {
		m_pCharl->Move(Echo::Root::instance()->getTimeSinceLastFrameClient(), move, true);
		cctPos = m_pCharl->getWorldPosition();
	}
	else {
		cctPos = m_pCharl->getWorldPosition() + move;
		m_pCharl->getParentComponentNode()->setWorldPosition(cctPos);
	}
	

	Vector3 upDir = Vector3::UP;
	SphericalTerrain* pCurrentTerrain = nullptr;
	{
		SphericalTerrain* pTerrain = m_pCharl->getSceneManager()->getPlanetManager()->getNearestPlanet();
		while (pTerrain)
		{
			if (!pTerrain->isCreateFinish()) break;
			if (!pTerrain->getIsNearbyWs(cctPos)) break;

			upDir = -pTerrain->getGravityWs(cctPos);
			pCurrentTerrain = pTerrain;
			break;
		}
	}
	Vector3 dif = m_pCharl->GetUpDirection() - upDir;
	if (!dif.isEqual(Vector3::ZERO, 1e-4f)) {
		m_pCharl->SetUpDirection(upDir);
		{
			Vector3 forwardDir = m_pUpdateNode->getWorldRotation() * Vector3::UNIT_Z;
			Vector3::OrthoNormalize(upDir, forwardDir);
			Quaternion rot = Quaternion::LookRotation(forwardDir, upDir);
			m_pUpdateNode->setWorldRotation(rot);
		}
	}

	{
		Quaternion updateRot = m_pUpdateNode->getWorldRotation();
		Vector3 updateDir = updateRot * Vector3::UP;

		auto smoothRotation = [&](Echo::EchoWeakPtr<Echo::NodeComponent>& _Node, float speed) {
			if (speed > 0.0f) {
				float maxAngle = speed * Echo::Root::instance()->getTimeSinceLastFrameClient();
				Quaternion roleRot = _Node->getWorldRotation();
				Vector3 roleDir = roleRot * Vector3::UP;
				float angle = Math::Abs(Vector3::Angle(roleDir, updateDir));
				if (angle <= maxAngle) {
					_Node->setWorldRotation(updateRot);
				}
				else {
					_Node->setWorldRotation(Quaternion::QSlerp(roleRot, updateRot, maxAngle / angle));
				}
			}
			else {
				_Node->setWorldRotation(updateRot);
			}
			};
		smoothRotation(m_pRoleNode, m_fRoleAngleSpeed);
		smoothRotation(m_pCameraNode, m_fCameraAngleSpeed);

	}

	UpdateCamera();

	if (pCurrentTerrain && m_bPhysX) {
		Vector3 faceNormal;
		Vector3 surfacePos = pCurrentTerrain->getSurfaceFinestWs(cctPos, &faceNormal);
		Vector3 posNor = cctPos - surfacePos;
		if (m_pCharl->GetUpDirection().dotProduct(posNor) < 0.0f && posNor.LengthSq() > 0.1f) {
			cctPos = surfacePos;
			SetCharlPosition(cctPos);
		}
	}

	if (1 != m_CurState) {
		m_CurState = 1;
		auto role = m_pCharl->getActor()->GetFirstComByType<RoleComponent>();
		if (role != nullptr) {
			role->SetRoleAnimation(Echo::Name("zc_tp_kz_fly_down_01_ca"), true, 0u);
			role->SetRoleAnimation(Echo::Name("zc_tp_kz_fly_down_01_tk"), true, 1u);
		}
	}

	String newTemplateTOD;
	if (pCurrentTerrain) {
		int regionIndex = pCurrentTerrain->computeRegionInformation(cctPos);
		if (regionIndex > -1 && regionIndex < (int)pCurrentTerrain->m_terrainData->region.biomeArray.size()) {
			newTemplateTOD = pCurrentTerrain->m_terrainData->region.biomeArray[regionIndex].TemplateTOD;
		}
	}
	if (m_sCurTemplateTOD != newTemplateTOD) {
		m_sCurTemplateTOD = newTemplateTOD;
		WorldManager* pWorldManager = m_pCharl->getWorldManager();
		AreaManager* pAreaManager = pWorldManager ? pWorldManager->getAreaManager() : nullptr;
		if (pAreaManager) {
			pAreaManager->applySceneArtTemplate(m_sCurTemplateTOD);
		}
	}
}

void EchoSphericalController::UpdateCamera() {
    //if(m_bJump || m_bJumpState){
	//	//LogManager::instance()->logMessage("UpdateCamera none, mstate=" + std::to_string(m_CurState));
    //    return;
    //}
	if (m_pCameraNode == nullptr || m_pCamera == nullptr) return;
	DVector3 pos{ m_pCameraNode->getWorldPosition() + m_pCameraNode->getWorldRotation() * m_vCameraLocalPosition };
	m_pCamera->setPosition(pos);
	m_pCamera->setOrientation(m_pCameraNode->getWorldRotation() * m_vCameraLocalRotation);
	if (m_pCharl == nullptr) return;
	DVector3 cameraPos = m_pCamera->getPosition();
	DVector3 charlPos = m_pCharl->getWorldPosition() + m_pCharl->GetUpDirection();
	Vector3 dir = cameraPos - charlPos;
	float distance = dir.normalise();
	EchoRaycastHit hit;
	if (m_pCharl->GetPhysicsManager()->SphereCast(charlPos, dir, hit, 0.5f, distance, ~ePhysXGroup_4)) {
		m_pCamera->setPosition(hit.point);
	}
}
