#pragma once
#include "EchoSingleton.h"
#include "EchoCamera.h"
#include "EchoRoleObject.h"

class EchoNPCController : public Echo::Singleton<EchoNPCController>
{
	enum class RoleState {
		noChange = 0,
		motionless,
		left,
		right,
		forward,
		backward
	};

public:
	EchoNPCController(Echo::Camera* camera);
	~EchoNPCController();

public:
	void tick();
	void update(int type, float x, float y);
	void setRolePosition(float x, float z);
private:
	void _setRoleOrientation();

	void _initializeRoleInfo();
	void _initializeCameraInfo();
	void _setRoleAnimation();
	float _getTerrainHeight(float x, float y);
	float _getRoleHeight();
public:
    bool m_bInit{false};
private:
	Echo::Camera* m_pCamera{ nullptr };
	Echo::RoleObject* m_pRoleObj{ nullptr };
	RoleState m_eLastState{ RoleState::motionless };
	RoleState m_eCurrentState{ RoleState::motionless };
	Echo::Vector3 m_vLastRoleMove{};
	Echo::Vector3 m_vCameraLocalPosition{};
	Echo::Quaternion m_vCameraLocalRotation{};
};
