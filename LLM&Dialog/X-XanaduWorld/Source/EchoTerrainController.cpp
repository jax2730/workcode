#include "EchoTerrainController.h"
#include "EchoWorldSystem.h"
#include "EchoWorldManager.h"

#define ROLENAME "terrinControllerRole"
#define ROLERESPATH "role/common/common_a3/common_a3.role"
#define EQUIPRESNAME "a3_dze_cf"

EchoTerrainController::EchoTerrainController(Echo::Camera* camera)
{
	if (camera != nullptr)
		m_pCamera = camera;
	else {
		Echo::LogManager::instance()->logMessage("Error::EchoSphericalController::camera is nullptr!");
		return;
	}
	m_vCameraLocalPosition = Echo::Vector3(0.f, 3.0f, -4.0f);
	m_vCameraLocalRotation.FromEuler(Echo::Vector3(-20.0f, 180.0f, 0.0f));
	_initializeRoleInfo();
	_initializeCameraInfo();
}

EchoTerrainController::~EchoTerrainController()
{
}

void EchoTerrainController::tick()
{
	_setRolePosition();
	Echo::Vector3 move = m_pRoleObj->getDerivedOrientation() * m_vLastRoleMove.GetNormalized();
	m_pRoleObj->setDerivedPosition(m_pRoleObj->getDerivedPosition() + move * 0.1);

	m_pCamera->setPosition(m_pRoleObj->getDerivedPosition() + m_pRoleObj->getDerivedOrientation() * m_vCameraLocalPosition);
	m_pCamera->setOrientation(m_pRoleObj->getDerivedOrientation() * m_vCameraLocalRotation);
	if (m_vLastRoleMove == Echo::Vector3::ZERO) {
		m_eCurrentState = RoleState::motionless;
	}
	else if (m_vLastRoleMove.z >= 0.0f) {
		if (m_vLastRoleMove.x > 0.0f) {
			m_eCurrentState = RoleState::left;
		}
		else if (m_vLastRoleMove.x < 0.0f) {
			m_eCurrentState = RoleState::right;
		}
		else {
			m_eCurrentState = RoleState::forward;
		}
	}
	else if (m_vLastRoleMove.z < 0.0f) {
		if (m_vLastRoleMove.x > 0.0f) {
			m_eCurrentState = RoleState::left;
		}
		else if (m_vLastRoleMove.x < 0.0f) {
			m_eCurrentState = RoleState::right;
		}
		else {
			m_eCurrentState = RoleState::backward;
		}
	}
	_setRoleAnimation();
	m_eLastState = m_eCurrentState;
}

void EchoTerrainController::update(int type, float x, float y)
{
	switch (type) {
	case 0:
	{
		if (x == 0.0f) {
			m_vLastRoleMove.x = y * -1.0f;
		}
		//else if (x == 2.0f) {
		//	m_vLastRoleMove.y = y * -1.0f;
		//}
		else if (x == 1.0f) {
			m_vLastRoleMove.z = y;
		}
	}
		break;
	case 1:
	{
		float dy = y * 0.1f;
		float dx = x * 0.1f;
		Echo::Quaternion rot = m_pRoleObj->getDerivedOrientation();
		if (abs(dx) > 0.01f) {
			Echo::Vector3 upDir = Echo::Vector3::UP;
			Echo::Quaternion upRot = Echo::Quaternion::AngleAxis(dx, upDir);
			rot = rot * upRot;
		}
		if (abs(dy) > 0.01f) {
			Echo::Vector3 rightDir = Echo::Vector3::RIGHT;
			Echo::Quaternion rightRot = Echo::Quaternion::AngleAxis(dy, rightDir);
			m_vCameraLocalPosition = rightRot * m_vCameraLocalPosition;
			m_vCameraLocalRotation = rightRot * m_vCameraLocalRotation;
		}
		m_pRoleObj->setOrientation(rot);
		Echo::Vector3 cameraPos = m_pRoleObj->getDerivedPosition() + rot * m_vCameraLocalPosition;
		m_pCamera->setPosition(cameraPos);
		m_pCamera->setOrientation(rot * m_vCameraLocalRotation);
	}
		break;
	}

}

void EchoTerrainController::_setRolePosition()
{
	Echo::Vector3 rolePos{ m_pRoleObj->getDerivedPosition() };
	rolePos.y = _getTerrainHeight(rolePos.x, rolePos.z);
	m_pRoleObj->setDerivedPosition(rolePos);
}

void EchoTerrainController::_setRoleOrientation()
{
	Echo::Vector3 roleDir{ m_pRoleObj->getDirection() };
	Echo::Vector3 cameraDir{ m_pCamera->getDerivedDirection() };
	Echo::Quaternion qua = Echo::Quaternion::FromToRotation(roleDir, cameraDir);
	m_pRoleObj->yaw(qua.getYaw());
}

void EchoTerrainController::_setCameraState()
{
	
}

void EchoTerrainController::_setRoleAnimation()
{
	if (m_eCurrentState == m_eLastState) {
		return;
	}
	m_eLastState = m_eCurrentState;
	switch (m_eCurrentState)
	{
	case RoleState::noChange:
		break;
	case RoleState::motionless:
	{
		m_pRoleObj->getAnimationComponent()->playAnimation(Echo::Name{ "zc_tp_dm_stand_xx_01_ca" }, true, 0u);
		m_pRoleObj->getAnimationComponent()->playAnimation(Echo::Name{ "zc_tp_dm_stand_xx_01_tk" }, true, 1u);
	}
	break;
	case RoleState::forward:
	{
		m_pRoleObj->getAnimationComponent()->playAnimation(Echo::Name{ "zc_tp_dm_stand_run_f_01_ca" }, true, 0u);
		m_pRoleObj->getAnimationComponent()->playAnimation(Echo::Name{ "zc_tp_dm_stand_run_f_01_tk" }, true, 1u);
	}
	break;
	case RoleState::backward:
	{
		m_pRoleObj->getAnimationComponent()->playAnimation(Echo::Name("zc_tp_dm_stand_run_b_01_ca"), true, 0u);
		m_pRoleObj->getAnimationComponent()->playAnimation(Echo::Name("zc_tp_dm_stand_run_b_01_tk"), true, 1u);
	}
	break;
	case RoleState::left:
	{
		m_pRoleObj->getAnimationComponent()->playAnimation(Echo::Name("zc_tp_dm_stand_run_lf_01_ca"), true, 0u);
		m_pRoleObj->getAnimationComponent()->playAnimation(Echo::Name("zc_tp_dm_stand_run_lf_01_tk"), true, 1u);
	}
		break;
	case RoleState::right:
	{
		m_pRoleObj->getAnimationComponent()->playAnimation(Echo::Name("zc_tp_dm_stand_run_rf_01_ca"), true, 0u);
		m_pRoleObj->getAnimationComponent()->playAnimation(Echo::Name("zc_tp_dm_stand_run_rf_01_tk"), true, 1u);
	}
		break;
	}
}

float EchoTerrainController::_getTerrainHeight(float x, float y)
{
	auto pManager = Echo::WorldSystem::instance()->GetMainWorldManager();
	float height;
	if (pManager->getKarstHeightAtWorldPosition(Echo::Vector2(x, y), height)) {
		return height;
	}
	else {
		return 0.0f;
	}
}

void EchoTerrainController::_initializeRoleInfo()
{
	m_pRoleObj = new Echo::RoleObject(Echo::Name(ROLENAME), Echo::Name(ROLERESPATH), Echo::Root::instance()->getMainSceneManager(), true);
	m_pRoleObj->getSkeletonComponent()->changeEquip(Echo::Name(EQUIPRESNAME), false);
	m_pRoleObj->getAnimationComponent()->playAnimation(Echo::Name{ "zc_tp_dm_stand_xx_01_ca" }, true, 0u);
	m_pRoleObj->getAnimationComponent()->playAnimation(Echo::Name{ "zc_tp_dm_stand_xx_01_tk" }, true, 1u);

	Echo::Vector3 rolePos{ m_pCamera->getDerivedPosition() };
	rolePos.y = _getTerrainHeight(rolePos.x, rolePos.z);
	m_pRoleObj->setPosition(rolePos);

	Echo::Vector3 roleDir{ m_pRoleObj->getDirection() };
	Echo::Vector3 cameraDir{ m_pCamera->getDerivedDirection() };
	Echo::Quaternion qua = Echo::Quaternion::FromToRotation(roleDir, cameraDir);
	m_pRoleObj->yaw(qua.getYaw());
}

void EchoTerrainController::_initializeCameraInfo() 
{
	Echo::Vector3 cameraPos{ m_pCamera->getDerivedPosition() };
	Echo::Vector3 rolePos{ m_pRoleObj->getDerivedPosition() };

	cameraPos.y = rolePos.y + _getRoleHeight();
	cameraPos = cameraPos - m_pCamera->getDerivedDirection() * 10.0f;
	m_pCamera->setPosition(cameraPos);
}

float EchoTerrainController::_getRoleHeight()
{
	return 5.0f;
}




