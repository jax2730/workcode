#include "EchoVoxelTerrainController.h"

#include <EchoColliderComponent.h>
#include <EchoPhysicsManager.h>
#include <EchoRoot.h>

#include"VoxelTerrain/EchoVoxelTerrain.h"

void EchoVoxelTerrainController::Create(Echo::SceneManager* sceneManager)
{
	const auto& terrains = Echo::GetVoxelTerrains(sceneManager);
	if (!terrains.empty())
	{
		mPtrVoxelTerrain = terrains.front();
		mToolParams = std::make_shared<Echo::VoxelToolParams>();
		mPtrEditTool = std::make_unique<Echo::VoxelTerrainSurfaceTool>(mToolParams);
		m_Scene = sceneManager;
	}
}

void EchoVoxelTerrainController::DoEdit()
{
	const auto voxelTerrain = mPtrVoxelTerrain.lock();
	if (voxelTerrain && mPtrEditTool)
	{
		float fFactor = mReverseEdit ? 1.f : -1.0f;
		mPtrEditTool->setReverseFactor(fFactor);
		if(mPtrEditTool)
		{
			Echo::VoxelIntBoxWithValidity ModifiedBounds = mPtrEditTool->Apply(voxelTerrain.get());
			if (ModifiedBounds.IsValid())
			{
				voxelTerrain->notifyLODManagerUpdateMeshes(ModifiedBounds.GetBox());
				voxelTerrain->notifyPhyComponentUpdateMeshes(ModifiedBounds.GetBox());
				voxelTerrain->notifyDataListeners(ModifiedBounds.GetBox());
			}
		}
	}
}

bool EchoVoxelTerrainController::rayCastVoxelTerrain(const Echo::DVector3& origin, const Echo::Vector3& direction, float maxDistance, Echo::DVector3& outHitPoint, float outDistance)
{
	if (!m_Scene) return false;
	auto* physics = Echo::EchoPhysicsSystem::instance()->GetPhysicsManager(m_Scene);
	Echo::EchoRaycastHit hit;

	if (physics->Raycast(origin, direction, hit, 100.f, Echo::ePhysXGroup_Terrain))
	{
		// if (hit.collider && hit.collider->GetFilterGroupType() == Echo::ePhysXGroup_Terrain)
		{
			outHitPoint = Echo::DVector3(hit.distance * direction) + origin;
			return true;
		}
	}

	return false;
}

void EchoVoxelTerrainController::Destroy()
{
	mPtrVoxelTerrain.reset();
}

EchoVoxelTerrainController::EchoVoxelTerrainController() = default;

EchoVoxelTerrainController::~EchoVoxelTerrainController()
{
	Destroy();
}

bool EchoVoxelTerrainController::ComputeMouseRay(
	const float& mousePointX, const float& mousePointY,
	const float& screenWidth, const float& screenHeight,
	const Echo::Camera* pCamera, Echo::DVector3& origin, Echo::Vector3& direction)
{
	if (!pCamera)
		return false;

	float screenX = mousePointX / screenWidth;
	float screenY = mousePointY / screenHeight;

	origin = pCamera->getDerivedPosition();
	direction = pCamera->getCameraToViewportRay(screenX, screenY).getDirection();

	return true;
}

void EchoVoxelTerrainController::SetEditPosition(const Echo::Vector3& position)
{
	if (mPtrEditTool)
	{
		mPtrEditTool->setBrushPosition(position);
	}
}

void EchoVoxelTerrainController::SetEditBrushSize(const float& size)
{
	mEditBrushSize = size;
}

float EchoVoxelTerrainController::GetEditBrushSize()
{
	return mEditBrushSize;
}

void EchoVoxelTerrainController::ApplyEditBrushSize()
{
	if (mPtrEditTool)
	{
		mPtrEditTool->setBrushRadius(mEditBrushSize);
	}
}

void EchoVoxelTerrainController::SetEditStrength(const float& strength)
{
	if (mPtrEditTool)
	{
		mPtrEditTool->setSculptStrength(strength);
	}
}

void EchoVoxelTerrainController::SetEditFallOff(const float& falloff)
{
	if (mPtrEditTool)
	{
		mPtrEditTool->setFallOffStrength(falloff);
	}
}

void EchoVoxelTerrainController::SetVoxelEditMode()
{
	mVoxelTerrainEditMode = !mVoxelTerrainEditMode;
}

bool EchoVoxelTerrainController::GetIsVoxelEditMode()
{
	return mVoxelTerrainEditMode;
}

void EchoVoxelTerrainController::SetVoxelExist(bool value)
{
	mIsVoxelExist = value;
}

bool EchoVoxelTerrainController::GetIsVoxelExist()
{
	return mIsVoxelExist;
}

void EchoVoxelTerrainController::SetMousePressedState(bool value)
{
	mIsMousePressedDown = value;
}

bool EchoVoxelTerrainController::GetMousePressedState()
{
	return mIsMousePressedDown;
}
