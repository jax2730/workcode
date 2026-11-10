#ifndef __EchoRoleController_H__
#define __EchoRoleController_H__
#include "EchoComponentPrerequisites.h"
#include "EchoSingleton.h"
#include "EchoCharactorControllerComponent.h"
#include "Actor/EchoNodeComponent.h"
#include "EchoIKBaseComponent.h"
#include "EchoActorRoleComponent.h"

namespace Echo
{
	class ConsoleCommand;
	class _EchoComponentExport EchoRoleController : public Singleton<EchoRoleController>
	{
	public:
		static void ExecuteCommand(Echo::ConsoleCommand* pConsole);
	public:
		enum RoleStateType {
			eWalk_xx,
			eWalk_lf,
			eWalk_f,
			eWalk_rf,
			eWalk_lb,
			eWalk_b,
			eWalk_rb,
			eFly_xx,
			eFly_f,
			eFly_b,
			eFly_l,
			eFly_r,
			eFly_up,
			eFly_down,
			eCount,
			eWalk_Start = eWalk_xx,
			eWalk_End = eWalk_rb,
			eFly_Start = eFly_xx,
			eFly_End = eFly_down,
		};
		static const std::string mRoleInfoPath;
	public:
		enum OperateType {
			eXMove,
			eYMove,
			eZMove,
			eXRotate,
			eYRotate,
			eZRotate,
			eAccelerate,
			eZoom,
			eJump,
		};
	public:
		EchoRoleController(Echo::Camera* camera);
		~EchoRoleController();
	public:
		void SetCharlPosition(const Echo::DVector3& position);
		const Echo::DVector3& GetCharlPosition();
		void SetGravity(float value);
		void SetCameraAngleSpeed(float value) { m_fCameraAngleSpeed = value; }
		void SetRoleAngleSpeed(float value) { m_fRoleAngleSpeed = value; }
		void SetMaxSpeedMul(float value);
		void SetControllerModel(bool value) { m_bFly = value; }
		bool GetControllerModel() { return m_bFly; }
		void SetPhysXController(bool value) { m_bPhysX = value; }
		bool GetPhysXController() { return m_bPhysX; }
		void SetSpeed(float value) { m_fSpeed = value; }
		float GetSpeed() { return m_fSpeed; }
		const DVector3& getWorldPosition();
		const Quaternion& getWorldRotation();

		void						clearOperate();
		void						update(OperateType type, float value);
		void						update(int type, float valx, float valy);
		void						tick();
	private:
		void						tickWalk();
		void						tickFly();
		void						UpdateCamera();
		bool						InitializeInfo();
		void						DefaultInfo();
		void						Initialize();
		void						UpdateState(const Vector3& move);
		void						UpdateState(RoleStateType type);
		bool						StateIsWalk();
		bool						StateIsFly();
	private:
		Echo::Camera* m_pCamera = nullptr;
		Echo::Vector3						m_vCameraLocalPosition;
		Echo::Quaternion					m_vCameraLocalRotation;
		Echo::Vector3						m_vLastMove;
		Echo::CharatorControllerComponentPtr m_pCharl = nullptr;
		Echo::IKBaseComponentPtr m_pIK = nullptr;
		Echo::EchoWeakPtr<Echo::RoleComponent> m_pRole = nullptr;
		Echo::EchoWeakPtr<Echo::NodeComponent> m_pUpdateNode = nullptr;
		Echo::EchoWeakPtr<Echo::NodeComponent> m_pRoleNode = nullptr;
		Echo::EchoWeakPtr<Echo::NodeComponent> m_pCameraNode = nullptr;
		Echo::ActorPtr					m_TestActorPtr = nullptr;
		Echo::String					m_sCurTemplateTOD;
		RoleStateType					m_CurState = eCount;
		float							m_fGravity = 2.0f;
		float							m_fGravityVelocity = 0.0f;
		float							m_fSpeedMul = 1.0f;
		float							m_fSpeed = 1.0f;
		float							m_fMinSpeedMul = 0.2f;
		float							m_fMaxSpeedMul = 5.0f;
		float							m_fCameraAngleSpeed = 90.0f;
		float							m_fRoleAngleSpeed = 0.0f;
		bool							m_bFly = false;
		bool							m_bPhysX = true;

		float							m_fJumpingInitialVelocity = 10.0f;
		float							m_fJumpingVelocity = 10.0f;
		bool							m_bJump = false;

		float							mRoleHeight = 2.0f;
		float							mRoleRadius = 0.3f;
		std::string						mRolePath;
		std::string						mRoleEquip;
		std::array<std::array<std::string, 2>, RoleStateType::eCount> mRoleAnimations;
	};
}


#endif // !__EchoRoleController_H__
