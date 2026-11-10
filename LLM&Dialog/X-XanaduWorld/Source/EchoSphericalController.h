#pragma once
#include "EchoSingleton.h"
#include "EchoCamera.h"
#include "EchoVector3.h"
#include "EchoQuaternion.h"
#include "EchoCharactorControllerComponent.h"
#include "Actor/EchoActor.h"
#include "Actor/EchoNodeComponent.h"

class EchoSphericalController : public Echo::Singleton<EchoSphericalController>
{
public:
	static Echo::ActorPtr createSphericalTerrain(const Echo::String& terrainName, const Echo::Vector3& origin, float scale);
public:
	EchoSphericalController(Echo::Camera* camera);
	EchoSphericalController(Echo::Camera* camera, const Echo::String& terrainName, const Echo::Vector3& origin);
	~EchoSphericalController();
	void SetCharlPosition(const Echo::Vector3& position);
	const Echo::Vector3 GetCharlPosition();
	void SetGravity(float value) { m_fGravity = value; }
	void SetCameraAngleSpeed(float value) { m_fCameraAngleSpeed = value; }
	void SetRoleAngleSpeed(float value) { m_fRoleAngleSpeed = value; }
	void SetMaxSpeedMul(float value);
	void SetControllerModel(bool value) { m_bFly = value; }
	bool GetControllerModel() { return m_bFly; }
	void SetPhysXController(bool value) { m_bPhysX = value; }
	bool GetPhysXController() { return m_bPhysX; }
public:
	void						update(int type, float valx, float valy);
	void						tick();
private:
	void						tickWalk();
	void						tickFly();
	void						UpdateCamera();
private:
	Echo::Camera* m_pCamera = nullptr;
	Echo::Vector3						m_vCameraLocalPosition;
	Echo::Quaternion					m_vCameraLocalRotation;
	Echo::Vector3						m_vLastMove;
	Echo::CharatorControllerComponentPtr m_pCharl = nullptr;
	Echo::EchoWeakPtr<Echo::NodeComponent> m_pUpdateNode = nullptr;
	Echo::EchoWeakPtr<Echo::NodeComponent> m_pRoleNode = nullptr;
	Echo::EchoWeakPtr<Echo::NodeComponent> m_pCameraNode = nullptr;
	Echo::ActorPtr					m_TestActorPtr = nullptr;
	Echo::String					m_sCurTemplateTOD;
	int								m_CurState = -1;
	float							m_fGravity = 2.0f;
	float							m_fSpeedMul = 1.0f;
	float							m_fMaxSpeedMul = 5.0f;
	float							m_fCameraAngleSpeed = 90.0f;
	float							m_fRoleAngleSpeed = 0.0f;
	bool							m_bFly = false;
	bool							m_bPhysX = true;

	//float							m_fJumpInitialVelocity = 10.0f;
	float							m_fHeightBeforeJumping = 0.0f;
	float							m_fJumpingHeight = 15.0f;
	bool							m_bJump = false;
    bool                            m_bJumpState = false;
};
