#pragma once

#include <stdio.h>
#include "EchoRoot.h"
#include "AsyncAppBase.h"


namespace Echo
{
	class Root;
	class SceneManager;
	class Camera;
	class WorldManager;
}

class RobotObjManager;

struct InitInfo_t
{
	std::string assetPath;
	std::string dataPath;
	const Echo::ECHO_GRAPHIC_PARAM* pView;
	Echo::uint32 renderMode;
	Echo::uint32 width;
	Echo::uint32 height;
	Echo::uint32 safeAreaLeft, safeAreaTop, safeAreaRight, safeAreaBottom;
};
class AsyncApp : public AsyncAppBase
{
public:
	AsyncApp(const char* _assetPath, const char* _dataPath, const Echo::ECHO_GRAPHIC_PARAM*pGraphicParam, Echo::uint32 renderMode, bool bClientMode, Echo::uint32 width, Echo::uint32 height);
	~AsyncApp();

	
	void						onRelease(bool releaseEngine = false) override;
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
	void						commandParse(const std::string& command) override;
	
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
	void						sphericalControlerJump() override;

	int							getFps() override;
	int							getDrawCall() override;
	int							getTriangleCnt() override;
	std::string					getCameraPosition() override;
	std::string					getCameraOrientation() override;
	std::string					getResMemUsageStr() override;
	void						create3DUIGunTest() override;
	void						loadSenceByName(const std::string& name) override;
	std::string					getGameData() override;

	void						OnKeyDown(unsigned int key) override {}
	void						OnKeyUp(unsigned int key) override {}
	void						OnMouseMove(Echo::Vector2 point, unsigned int buttonsFlag) override {}
	void						OnMouseLeftDown(Echo::Vector2 point) override {}
	void						OnMouseLeftUp(Echo::Vector2 point) override {}
	void						OnMouseRightDown(Echo::Vector2 point) override {}
	void						OnMouseRightUp(Echo::Vector2 point) override {}
	void						OnInputText(char16_t c) override;
	void						OnKillFocus() override {}

protected:

	virtual void				OnTick(float dt) override;
	virtual void				onFrameEnd() override;
	//virtual void				setSwapBufferTime(unsigned int time) override;
protected:
	
	//void						createRoleTest(Echo::Vector3 pos, int num);
	//void						createParticleTest(Echo::Vector3 pos);
	//void						initRoleList();
	bool						isHasImGuiWindowHovered() const override;
private:
	Echo::uint32				m_width = 0;
	Echo::uint32				m_height = 0;
	bool						m_runTest = false;

};


void						resetMainCamera();
bool						loadNewLevel(const std::string & szLevelName);

AsyncApp *	GetAsyncApp();

void                        InitAsyncApp(const char* _assetPath, const char* _dataPath, const Echo::ECHO_GRAPHIC_PARAM* pGraphicParam, bool bClientMode, Echo::uint32 width, Echo::uint32 height);
void						InitAsyncApp(const char* _assetPath, const char* _dataPath, const Echo::ECHO_GRAPHIC_PARAM* pGraphicParam, bool bClientMode, Echo::uint32 width, Echo::uint32 height, Echo::uint32 _left, Echo::uint32 _right, Echo::uint32 _bottom, Echo::uint32 _top);

unsigned int				GetEngineRenderMode();
unsigned int				GetAsyncAppTick();

void						SetAsyncAppReinitFlag(const char* mainXml);

void						SetInitLevel(const std::string& levelName);
void						SetRunTest(const bool);

extern Echo::Root* g_pEchoRoot2;
extern Echo::SceneManager* g_pMainSceneManager;
extern Echo::Camera* g_pMainCamera;
extern Echo::WorldManager* g_pWorldManager;
extern RobotObjManager*	g_pRobotObjManager;
extern bool g_testEnd;



