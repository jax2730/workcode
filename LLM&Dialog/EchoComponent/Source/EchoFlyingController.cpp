#include "EchoComponentStableHeaders.h"
#include "EchoFlyingController.h"
#include "Actor/EchoActorManager.h"
#include "EchoCharactorControllerComponent.h"
#include "../../GameEngine/Include/EchoCameraControl.h"

namespace Echo
{
	bool EchoFlyingController::isCreated = false;

	EchoFlyingController::EchoFlyingController(Camera* camera)
	{
		if (camera != nullptr)
			m_pCamera = camera;
		else
			LogManager::instance()->logMessage("Error::EchoFlyingController::camera is nullptr!");
		isCreated = true;
		createFlyCCT(true);
	}

	EchoFlyingController::~EchoFlyingController()
	{
		isCreated = false;
	}

	void EchoFlyingController::updateFlyCCT(int type, float valx, float valy)
	{
		if (!m_pCamera || !Echo::ActorManager::mCharatorControllerActor)
			return;
		Echo::CharatorControllerComponent* pCharl = Echo::EchoPhysicsSystem::instance()->GetCurrentCharactorController();
		if (!pCharl)
			return;


		Vector3 oldPos = pCharl->getActor()->getWorldPosition();

		if (type == 0)
		{
			Vector3 movex = m_pCamera->getRight() * (valx == 0.0f ? valy : 0.0f);
			Vector3 movey = Vector3::UNIT_Y * (valx == 2.0f ? valy : 0.0f);
			Vector3 movez = m_pCamera->getDirection() * (valx == 1.0f ? valy : 0.0f);

			Vector3 move = movex + movey + movez;

			Vector3 offset = move * CameraControl::instance()->getMoveSpeed() * Echo::Root::instance()->getTimeSinceLastFrameClient();


			//Vector3 offset = newPos - oldPos;
			pCharl->Move(Echo::Root::instance()->getTimeSinceLastFrameClient(), offset);
			m_pCamera->move(Echo::ActorManager::mCharatorControllerActor->getWorldPosition() - oldPos);
		}
		if (type == 1)
		{
			float dy = valy * 0.003f;
			float dx = valx * 0.003f;

			m_pCamera->yaw(Radian(dx));
			m_pCamera->pitch(Radian(dy));

			Vector3 campos = m_pCamera->getPosition();
			Vector3 dir = m_pCamera->getDirection();
			dir.y -= 0.4f;
			dir.normalise();
			Vector3 newPos = campos + dir * 10;
			Vector3 offset = newPos - oldPos;
			pCharl->Move(Echo::Root::instance()->getTimeSinceLastFrameClient(), offset);
		}

		Echo::ActorManager::mCharatorControllerActor->setWorldRotation(m_pCamera->getOrientation());
	}

	void EchoFlyingController::createFlyCCT(bool enable)
	{
		if (!m_pCamera)
			return;
		if (!Echo::ActorManager::mCharatorControllerActor)
			Echo::ActorSystem::instance()->getMainActorManager()->createCharatorControllerActor();
		Echo::CharatorControllerComponent* pCharl = (Echo::CharatorControllerComponent*)Echo::ActorManager::mCharatorControllerActor->getComponent("chactrl").get();
		if (pCharl && isCreated)
		{
			pCharl->SetIsCurrentController(enable);

			Vector3 campos = m_pCamera->getPosition();
			Vector3 dir = m_pCamera->getDirection();
			dir.y -= 0.4f;
			dir.normalise();
			Vector3 newPos = campos + dir * 10;

			Echo::ActorManager::mCharatorControllerActor->setWorldPosition(newPos);
			Echo::ActorManager::mCharatorControllerActor->setWorldRotation(m_pCamera->getOrientation());
		}
	}
}
