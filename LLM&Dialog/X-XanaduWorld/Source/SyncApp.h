#pragma once

#include <stdio.h>
#include "Echo.h"
#include "FrameControl.h"
#include "AppBase.h"

namespace Echo
{
	class Root;
	class SceneManager;
	class Camera;
	class WorldManager;
	class ComponentSystem;
	class EditorRoot;
}

class RobotObjManager;

class SyncApp : public AppBase
{
public:
	SyncApp();
	~SyncApp();

	int							init(const char* _assetPath, const char* _dataPath, const Echo::ECHO_GRAPHIC_PARAM* pGraphicParam, bool bClientMode, Echo::uint32 width, Echo::uint32 height);
	void						onRelease(bool releaseEngine = false) override;
	
	void						onTick() override;
	void						onResize(Echo::uint32, Echo::uint32, Echo::uint32 safeAreaLeft, Echo::uint32 safeAreaTop, Echo::uint32 safeAreaRight, Echo::uint32 safeAreaBottom) override;

	void						onStart() override;
	void						onPause() override;
	void						onResume() override;
	void						onStop() override;
	void						onResetWindow(void* handle) override;

	void						setFPS(float _fps) override;
	void						getWindowSize(Echo::uint32& width, Echo::uint32& height) override;
	unsigned int				getTick() override;

	//////////////////////////////////////////////////////////////////////////
	void						setNextLogicCmdInfo(const char* file, int line) override;
	void						postToLogic_(const std::function<void()>& cmd) override;
	void						postToLogic_(std::function<void()>&& cmd) override;

	void						setNextEngineCmdInfo(const char* file, int line) override;
	void						postToEngine_(const std::function<void()>& cmd) override;
	void						postToEngine_(std::function<void()>&& cmd) override;

	void						postToCache_(const std::function<void()>& cmd) override;
	void						postToCache_(std::function<void()>&& cmd) override;
	void						postToMain_(const std::function<void()>& cmd) override;
	void						postToMain_(std::function<void()>&& cmd) override;

	void						commandParse(const std::string& command) override;

	//////////////////////////////////////////////////////////////////////////

	void						onMouseMove(const Echo::Vector2& point) override;
	void						onMouseLeftDown(const Echo::Vector2& point) override;
	void						onMouseLeftUp(const Echo::Vector2& point) override;

	void						cameraMove(int type, int val) override;
	void						cameraTurn(float x, float y) override;
	void						cameraDisChg(int val) override;
	void						setMoveDir(float x, float y) override;
	void						setTurnDir(float x, float y) override;
	void						incCameraSpeed() override;
	void						decCameraSpeed() override;
	void						attack() override;
	void						switchPlayMode() override;
	void						createRobotObject() override;

	int							getFps() override;
	int							getDrawCall() override;
	int							getTriangleCnt() override;
    std::string					getCameraPosition() override;
	std::string					getCameraOrientation() override;
	std::string					getResMemUsageStr() override;
	void						create3DUIGunTest() override;
	void						loadSenceByName(const std::string& name) override;
	std::string					getGameData() override;

	void						OnKeyDown(unsigned int key) override;
	void						OnKeyUp(unsigned int key) override;
	void						OnMouseMove(Echo::Vector2 point, unsigned int buttonsFlag) override;
	void						OnMouseLeftDown(Echo::Vector2 point) override;
	void						OnMouseLeftUp(Echo::Vector2 point) override;
	void						OnMouseRightDown(Echo::Vector2 point) override;
	void						OnMouseRightUp(Echo::Vector2 point) override;
	void						OnKillFocus() override;
	void						OnInputText(char16_t c) override;

protected:
	void						resetMainCamera();
	bool						loadNewLevel(const std::string & szLevelName);
	void						createRoleTest(Echo::Vector3 pos, int num);
	void						createParticleTest(Echo::Vector3 pos);
	void						initRoleList();

	bool						isHasImGuiWindowHovered() const override;
private:
	unsigned int				m_tick = 0;
	whframecontrol				m_wfc;
	bool						m_isInitialized = false;
	
	Echo::uint32				m_width = 0;
	Echo::uint32				m_height = 0;

	std::string					m_sCurLevelName;

	Echo::Root*					m_pRoot = nullptr;
	Echo::Camera*				m_pCamera = nullptr;
	Echo::RenderSystem*			m_pRenderSystem = nullptr;
	Echo::WorldManager*			m_pWorldMgr = nullptr;
	RobotObjManager*			m_pRobotObjManager = nullptr;
	Echo::ComponentSystem*		m_pComponentSystem = nullptr;
	Echo::EditorRoot*			m_pEditorRoot = nullptr;

	std::vector<std::string>	mSceneArray;
	unsigned int				m_curScene = 0;
};

SyncApp *					GetSyncApp();

void						SetSynacAppNull();

void                        InitSyncApp(const char* _assetPath, const char* _dataPath, const Echo::ECHO_GRAPHIC_PARAM* pGraphicParam, bool bClientMode, Echo::uint32 width, Echo::uint32 height);
void						InitSyncApp(const char* _assetPath, const char* _dataPath, const Echo::ECHO_GRAPHIC_PARAM* pGraphicParam, bool bClientMode, Echo::uint32 width, Echo::uint32 height, Echo::uint32 _left, Echo::uint32 _right, Echo::uint32 _bottom, Echo::uint32 _top);

void						ReInitSyncApp();



