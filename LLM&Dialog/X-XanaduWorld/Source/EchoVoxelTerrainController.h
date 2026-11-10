#pragma once

#include"EchoSceneManager.h"
#include"VoxelTerrain/EchoVoxelTerrain.h"
#include"VoxelTerrain/VoxelTerrainTool.h"

class EchoVoxelTerrainController
{
public :
	EchoVoxelTerrainController();
	EchoVoxelTerrainController(const EchoVoxelTerrainController&) = delete;
	EchoVoxelTerrainController& operator=(const EchoVoxelTerrainController&) = delete;
	~EchoVoxelTerrainController();

	void Create(Echo::SceneManager* sceneManager);

	void DoEdit();

	bool rayCastVoxelTerrain(const Echo::DVector3& origin, const Echo::Vector3& direction, float maxDistance, Echo::DVector3& outHitPoint, float outDistance);

	void Destroy();

	static bool ComputeMouseRay(
		const float& mousePointX, const float& mousePointY,
		const float& screenWidth, const float& screenHeight,
		const Echo::Camera* pCamera, Echo::DVector3& origin, Echo::Vector3& direction);

	void SetEditPosition(const Echo::Vector3& position);
	void SetEditBrushSize(const float& size);
	float GetEditBrushSize();
	void ApplyEditBrushSize();
	void SetEditStrength(const float& strength);
	void SetEditFallOff(const float& falloff);
	void SetReverseEdit(bool reverse) { mReverseEdit = reverse; }

	void SetVoxelEditMode();
	bool GetIsVoxelEditMode();
	void SetVoxelExist(bool value);
	bool GetIsVoxelExist();

	void SetMousePressedState(bool value);
	bool GetMousePressedState();

private:
	std::weak_ptr<Echo::VoxelTerrain> mPtrVoxelTerrain;
	std::shared_ptr<Echo::VoxelToolParams>		   mToolParams;
	std::unique_ptr<Echo::VoxelTerrainSurfaceTool> mPtrEditTool;
	Echo::SceneManager* m_Scene = nullptr;
	bool mVoxelTerrainEditMode = false;
	bool mIsVoxelExist = false;
	bool mIsMousePressedDown = false;
	bool mReverseEdit = false;
	float mEditBrushSize = 8.0f;
};
