#include "AsyncApp.h"
#include "appview.h"

#include "EchoEngineImpl.h"
#include "EchoRenderSystem.h"
#include "EchoWorldSystem.h"
#include "EchoWorldManager.h"
#include "EchoTODManager.h"
#include "EchoProtocolComponent.h"
#include "FairyGUI.h"
#include "EchoSystemConfigFile.h"
#include "EchoShadowmapManager.h"
#include "EchoAutoObjectManager.h"
#include "EchoMaterialCompileManager.h"
#include "EchoDefaultArchive.h"
#include "EchoVideoConfig.h"
#include "EchoStringConverter.h"
#include "EchoConsoleCommand.h"
#include "EchoCameraControl.h"
#include "EchoCharacterObj.h"
#include "EchoTriggerAreaManager.h"
#include "EchoGravityAreaManager.h"
#include "FrameControl.h"
#include "EchoRobotObjManager.h"
#include "EchoPlayerObjManager.h"
#include "EchoBellSystem.h"
#include "EchoComponentSystem.h"
#include "EchoLogManager.h"
#include "Actor/EchoActorSystem.h"
#include "EchoPhysicsManager.h"
#include "EchoSphericalController.h"
#include "EchoSphericalTerrainComponent.h"
#include "EchoNPCController.h"
#include "EchoMiniMap.h"
#if ECHO_PLATFORM == ECHO_PLATFORM_WIN32
#ifdef ECHO_EDITOR
#include "EchoEditorRoot.h"
#endif
#include "EchoShaderContentManager.h"
#endif

#include "SceneHeatMap.h"
#ifdef __ANDROID__
#include "Android/EchoAndroidJavaEnv.h"
#include "EchoMediaManager.h"
#endif

#include "BuildingTest.h"
#include "EchoFlyingController.h"

//#include "EchoShaderMan.h"

//#include "PlatformWin32/WinMain.cpp"
#ifdef __ANDROID__
extern void AttachNativeThreadFMOD();
extern void DetachNativeThreadFMOD();
#endif
//-----------------------------------------------------------------------------
// Echo Engine
//-----------------------------------------------------------------------------

using namespace Echo;

Echo::Root* g_pEchoRoot2 = nullptr;
Echo::SceneManager* g_pMainSceneManager = nullptr;
Echo::Camera* g_pMainCamera = nullptr;
Echo::WorldManager* g_pWorldManager = nullptr;
RobotObjManager* g_pRobotObjManager = nullptr;
PlayerObjManager* g_pPlayerObjManager = nullptr;
ComponentSystem* g_pComponentSystem = nullptr;
Echo::MiniMapManager* g_pMiniMapManager = nullptr;

#if ECHO_PLATFORM == ECHO_PLATFORM_WIN32
#ifdef ECHO_EDITOR
EditorRoot* g_pEditorRoot = nullptr;
#endif
#endif

static whframecontrol		g_wfc;
static SceneHeatMap heat_map;
static bool fReinit = false;
static bool g_showTodTimeHud = false;
static echofgui::GObject* g_todTimeUI = nullptr;
static echofgui::GObject* g_todTimeText = nullptr;

static void EnsureTodUI()
{
	if (!g_todTimeUI)
	{
		echofgui::UIPackage::addPackage("fgui/bytes/chinese_minimap");
		g_todTimeUI = echofgui::UIPackage::createObject("chinese_minimap", "text");
		if (g_todTimeUI)
		{
			echofgui::GRoot::defaultRoot()->addChild(g_todTimeUI);
			g_todTimeUI->setPosition(20.0f, 20.0f, 0.0f);
			g_todTimeText = g_todTimeUI->as<echofgui::GComponent>()->getChild("n0");
		}
	}
}

static void DestroyTodUI()
{
	if (g_todTimeUI)
	{
		if (g_todTimeUI->getParent()) g_todTimeUI->getParent()->removeChild(g_todTimeUI);
		g_todTimeUI = nullptr;
		g_todTimeText = nullptr;
	}
}

#define WFC_FPS	30

static InitInfo_t g_init_info;
static std::string g_mainXml;
static bool g_isInitialized = false;
bool		g_runTest = false;

Echo::Name mainWorld("Main");
Echo::Name mapWorld("MiniMap");

namespace
{
	std::map<std::string, std::tuple<Echo::Vector3, Echo::Quaternion>> g_LevelMap =
	{
		std::make_pair(std::string("test_ceshi01"),
				std::make_tuple(
					Echo::Vector3(-100.0f, 720.0f, -750.0f),
					Echo::Quaternion(-0.0451708846f, 0.49380821f, -0.0257615524f, -0.868126213f)
					)),
		std::make_pair(std::string("karst"),
				std::make_tuple(
					Echo::Vector3(390000.f, 4000.f, 370000.f),
					Echo::Quaternion(0.f, 0.f, 0.f,1.0f)
					)),
		std::make_pair(std::string("karst1"),
					std::make_tuple(
					Echo::Vector3(70569.0f, 400.0f, -19027.0f),
					Echo::Quaternion(0.f, 0.f, 0.f,1.0f)
					)),
		std::make_pair(std::string("karst2"),
					std::make_tuple(
					Echo::Vector3(128746.914668f, 2000.0f, -18458.449817f),
					Echo::Quaternion(0.f, 0.f, 0.f,1.0f)
					)),
		std::make_pair(std::string("test"),
				std::make_tuple(
					Echo::Vector3(0.f, 11.f, 2.f),
					Echo::Quaternion(0.f, 0.f, 0.f,1.0f)
					)),
		std::make_pair(std::string("karstworld"),
				std::make_tuple(
					Echo::Vector3(-365472.f, 200.f, 432224.f),
					Echo::Quaternion(0.f, 0.f, 0.f,1.0f)
					)),
		std::make_pair(std::string("testpef"),
				std::make_tuple(
					Echo::Vector3(0.f, 0.f, 0.f),
					Echo::Quaternion(0.f, 0.f, 0.f,1.0f)
					))
	};

	std::string g_InitLevel = "karstworld";
}

class CmnLogic
{
public:
	virtual ~CmnLogic() {};
	virtual void PreTick() = 0;
	virtual void OnTick(float _delta_s) = 0;
	virtual void OnTickEnd() = 0;
};

class UpdateLogic : public CmnLogic
{
public:
	UpdateLogic();
	~UpdateLogic();

	void PreTick() override;
	void OnTick(float _delta_s) override;
	void OnTickEnd() override;

private:
	std::chrono::nanoseconds GetLogicTime() const
	{
		return m_LogicTime;
	}
	void ClearLogicTime()
	{
		m_LogicTime = std::chrono::nanoseconds(0);
	}
	std::chrono::nanoseconds	m_LogicTime = std::chrono::nanoseconds(0);
};

UpdateLogic::UpdateLogic()
{
	// 帧率设置
	g_wfc.SetFR(WFC_FPS);
	g_wfc.Reset();
}

UpdateLogic::~UpdateLogic()
{

}

void UpdateLogic::PreTick()
{
}

void UpdateLogic::OnTick(float dt)
{

}

void UpdateLogic::OnTickEnd()
{

}

namespace
{
	int shaderCompilationProgress;
	void onShaderCompilationProgress(int value)
	{
		shaderCompilationProgress = value;
		if (value == 100)
		{
			Echo::MaterialCompileManager::instance()->setLoadingCallback(nullptr);
		}
	}
}

class applogic : public CmnLogic
{
public:
	applogic();
	~applogic();
	void PreTick() override;
	void OnTick(float _delta_s) override;
	void OnTickEnd() override;

private:
	unsigned int				m_tick;

	std::chrono::nanoseconds GetLogicTime() const
	{
		return m_LogicTime;
	}
	void ClearLogicTime()
	{
		m_LogicTime = std::chrono::nanoseconds(0);
	}
	std::chrono::nanoseconds	m_LogicTime = std::chrono::nanoseconds(0);
};


applogic::applogic()
	: m_tick(0)
{
	GetAsyncApp()->PostToEngine([]() {
		Echo::MaterialCompileManager::instance()->setLoadingCallback(onShaderCompilationProgress);
	Echo::MaterialCompileManager::instance()->startCompile();
		});

	g_mainXml.clear();

	// 帧率设置
	g_wfc.SetFR(WFC_FPS);
	g_wfc.Reset();
}

applogic::~applogic()
{

}

void applogic::PreTick()
{
	// 在逻辑帧最开始执行
	m_tick++;
}

void applogic::OnTick(float dt)
{

}

void applogic::OnTickEnd()
{

}


static std::unique_ptr<AsyncApp> g_appview;
AsyncApp* GetAsyncApp()
{
	return g_appview.get();
}

AsyncAppBase* getAppBase()
{
	return g_appview.get();
}


AsyncApp::AsyncApp(const char* _assetPath, const char* _dataPath, const Echo::ECHO_GRAPHIC_PARAM* pGraphicParam, Echo::uint32 renderMode, bool bClientMode, Echo::uint32 width, Echo::uint32 height)
	: m_width(width)
	, m_height(height)
{
	m_uEngineRenderMode = renderMode;

	if (Echo::ECHO_THREAD_MT1 != m_uEngineRenderMode)
	{
		if (!Echo::GetEchoEngine()->getRoot())
		{
			std::string sLogFileName = "Echo.log";

			Echo::IArchive* pArchive = Echo::GetArchivePack();

#ifndef _WIN32
			sLogFileName.insert(0, _dataPath);
#endif

			bool bForceUseGxRender = false;

			Root::RuntimeMode mode = Root::eRM_Client;
			if (!bClientMode)
				mode = Root::eRM_Editor;

			Echo::GetEchoEngine()->init(_assetPath, pArchive, mode, sLogFileName, bForceUseGxRender, false, true);
#if ECHO_PLATFORM == ECHO_PLATFORM_WIN32 //TODO: Fix Shader Path Error on iOS Platform
			Echo::GetEchoEngine()->setShaderPath(_assetPath, _dataPath);
#endif
			Echo::GetEchoEngine()->setRenderSystem(pGraphicParam, _assetPath, width, height);
			Echo::GetEchoEngine()->initConfig();

			g_pEchoRoot2 = Echo::GetEchoEngine()->getRoot();
			g_pMainSceneManager = Echo::Root::instance()->getMainSceneManager();
			g_pMainCamera = g_pMainSceneManager->getMainCamera();
			g_pWorldManager = Echo::WorldSystem::instance()->GetMainWorldManager();

			g_pComponentSystem = new ComponentSystem();
			g_pComponentSystem->init(PADBO2COMREGISTERLIST);

			g_pRobotObjManager = new RobotObjManager();
			g_pRobotObjManager->SetSceneManager(g_pMainSceneManager);

			g_pPlayerObjManager = new PlayerObjManager();
			g_pPlayerObjManager->SetSceneManager(g_pMainSceneManager);
			g_pMiniMapManager = new MiniMapManager(g_pMainSceneManager, g_pWorldManager);
			Echo::BellSystem* pBellSystem = Echo::BellSystem::instance();
			if (NULL != pBellSystem)
			{
				if (pBellSystem->initialise("bank/Master.bank", "bank/Master.strings.bank"))
					pBellSystem->setListenerCamera(g_pMainCamera);
			}

			resetMainCamera();

			GetEchoEngine()->CreateWorld(mapWorld);
			GetEchoEngine()->changeLevel(mapWorld, "karst2mini");

			GetEchoEngine()->ChangeWorld(mainWorld);
			loadNewLevel(g_InitLevel);
			Echo::ConsoleCommand* pConsole = Echo::ConsoleCommand::instance();
			if (pConsole) {
				//karst场景的前置命令
				//if (g_InitLevel.compare("karst") == 0 || g_InitLevel.compare("karst1") == 0) {
#if ECHO_PLATFORM == ECHO_PLATFORM_ANDROID
					//视频配置
				CmdLineArg dlvCommand("dlv", "0", eCLAT_Normal);
				pConsole->executeCommand(dlvCommand);
				CmdLineArg lvCommand("lv", "0", eCLAT_Normal);
				pConsole->executeCommand(lvCommand);
				//渲染分辨率降低
				CmdLineArg tsCommand("ts", "0.5", eCLAT_Normal);
				pConsole->executeCommand(tsCommand);
				//远裁剪距离增加
				CmdLineArg farCommand("farclip", "100000", eCLAT_Normal);
				pConsole->executeCommand(farCommand);
				//page加载范围增加
				CmdLineArg viewrange("viewrange", "10000", eCLAT_Normal);
				pConsole->executeCommand(viewrange);
#endif
				//tod时间固定在中午
				/*CmdLineArg todCommand("todspeed", "0 0.5", eCLAT_Normal);
				pConsole->executeCommand(todCommand);*/
				//进入npc相机模式
				//new EchoNPCController(g_pMainCamera);
				new EchoSphericalController(g_pMainCamera);
				//assao
				CmdLineArg ssaoCommand("ssao", "2 8 1 1 3 3", eCLAT_Normal);
				pConsole->executeCommand(ssaoCommand);

				//}
			}
			EchoPhysicsManager::instance()->EditorPlay();

			heat_map.setPath(_dataPath);
			g_isInitialized = true;
			if (g_InitLevel.compare("karst1") == 0) {
				auto pActor = Echo::ActorSystem::instance()->getActiveActorManager()->createActor(Name("levels/karst1/[art][karst1]actor#32.act"), "", false, false);
				pActor->addActorLoadListener(g_pMiniMapManager);
			}
			if (g_InitLevel.compare("karstworld") == 0) {
				//auto pActor = Echo::ActorSystem::instance()->getActiveActorManager()->createActor(Name("levels/karstworld/[art][karstworld]actor#0.act"), "", false, false);
				//pActor->addActorLoadListener(g_pMiniMapManager);
			}
		}
	}
}

AsyncApp::~AsyncApp()
{
	if (Echo::ECHO_THREAD_MT1 != m_uEngineRenderMode)
	{
		g_isInitialized = false;
		SAFE_DELETE(g_pMiniMapManager);
		SAFE_DELETE(g_pRobotObjManager);
		SAFE_DELETE(g_pPlayerObjManager);

		//g_pComponentSystem->shutDown();
		Echo::GetEchoEngine()->shutdown();
		SAFE_DELETE(g_pComponentSystem);
		Echo::GetEchoEngine()->deinit();


		g_pEchoRoot2 = nullptr;
		g_pMainSceneManager = nullptr;
		g_pMainCamera = nullptr;
		g_pWorldManager = nullptr;
	}
}

void AsyncApp::onResize(Echo::uint32 w, Echo::uint32 h, Echo::uint32 safeAreaLeft, Echo::uint32 safeAreaTop, Echo::uint32 safeAreaRight, Echo::uint32 safeAreaBottom)
{
	m_width = w;
	m_height = h;
	if (w != 0 || h != 0)
	{
		g_init_info.width = w;
		g_init_info.height = h;
		g_init_info.safeAreaLeft = safeAreaLeft;
		g_init_info.safeAreaTop = safeAreaTop;
		g_init_info.safeAreaRight = safeAreaRight;
		g_init_info.safeAreaBottom = safeAreaBottom;
	}

	if (Echo::ECHO_THREAD_MT1 != m_uEngineRenderMode)
	{
		g_pEchoRoot2->getRenderSystem()->resizeWindow(w, h);
	}
	else
	{
		g_appview->PostToLogic([w, h]() {
			g_pEchoRoot2->getRenderSystem()->resizeWindow(w, h);
			});
	}
}

void AsyncApp::onStart()
{
	//This sound function can be called in the main thread
#if defined(__ANDROID__) || defined(__APPLE__)
	if (Echo::BellSystem::instance())
		Echo::BellSystem::instance()->resume();
#endif

#ifdef __ANDROID__
	if (Echo::MediaManager::instance())
		Echo::MediaManager::instance()->Start();
#endif
}

void AsyncApp::onPause()
{
	if (Echo::ECHO_THREAD_MT1 != m_uEngineRenderMode)
	{
		Echo::GetEchoEngine()->stopRendering();
	}
	else
	{
		g_appview->PostToLogic([]() {
			Echo::GetEchoEngine()->stopRendering();
			});
	}
}

void AsyncApp::onResume()
{
	if (Echo::ECHO_THREAD_MT1 != m_uEngineRenderMode)
	{
		Echo::GetEchoEngine()->resumeRendering();
	}
	else
	{
		g_appview->PostToLogic([]() {
			Echo::GetEchoEngine()->resumeRendering();
			});
	}
}

void AsyncApp::onStop()
{
	//This sound function can be called in the main thread
#if defined(__ANDROID__) || defined(__APPLE__)
	if (Echo::BellSystem::instance())
		Echo::BellSystem::instance()->suspend();
#endif

#ifdef __ANDROID__
	if (Echo::MediaManager::instance())
		Echo::MediaManager::instance()->Pause();
#endif
}

void AsyncApp::onResetWindow(void* handle)
{
	Echo::GetEchoEngine()->getRoot()->onRecreateSurface(handle);
}

void AsyncApp::getWindowSize(Echo::uint32& width, Echo::uint32& height)
{
	width = m_width;
	height = m_height;
}

unsigned int AsyncApp::getTick()
{
	return 0;
}

void AsyncApp::onRelease(bool releaseEngine /*= false*/)
{
	if (EchoSphericalController::Alive()) {
		delete EchoSphericalController::instance();
	}
	if (EchoNPCController::Alive()) {
		delete EchoNPCController::instance();
	}
	if (releaseEngine)
	{
		if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
		{
			if (g_pEchoRoot2)
				g_pEchoRoot2->renderMainThread();
		}

		g_appview->ShutDown();

		g_appview.reset();
	}
	else
	{
		g_appview->ShutDown();
	}
}

void AsyncApp::setFPS(float _fps)
{
	GetAsyncApp()->PostToLogic([_fps]() {
		g_wfc.SetFR(_fps);
	g_wfc.Reset();

		});
}


void AsyncApp::OnTick(float dt)
{
	// 引擎渲染
	if (Echo::ECHO_THREAD_MT1 != m_uEngineRenderMode)
	{
		//更新触发器对象
		Echo::PlayMode mode = Echo::CameraControl::instance()->GetPlayMode();

		Echo::Vector3 pos;
		if (mode == Echo::PlayMode::MODE_FREE)
		{
			pos = g_pMainCamera->getDerivedPosition();
		}
		else if (mode == Echo::PlayMode::MODE_PLAY)
		{
			Echo::CharacterObj* pcharobj = Echo::CameraControl::instance()->getCharObj();
			if (pcharobj)
			{
				Echo::RoleObject* prole = pcharobj->getRoleObj();
				if (prole)
				{
					pos = prole->getPosition();
				}
			}
		}
		Echo::TriggerAreaManager::instance()->update(pos, g_pMainCamera->getDerivedOrientation(), g_pMainCamera->getDerivedPosition());
		Echo::GravityAreaManager::instance()->update(pos);
		if (EchoSphericalController::Alive())
		{
			if (g_pMiniMapManager)
				g_pMiniMapManager->UpdatePlayerPosition(EchoSphericalController::instance()->GetCharlPosition());
		}
		else
		{
			if (g_pMiniMapManager)
				g_pMiniMapManager->UpdatePlayerPosition(pos);
		}
		g_pWorldManager->updateRolePos(pos);
		Echo::GetEchoEngine()->preRenderOneFrame();
		if (EchoSphericalController::Alive()) {
			EchoSphericalController::instance()->tick();
		}
		if (EchoNPCController::Alive()) {
			EchoNPCController::instance()->tick();
		}		Echo::GetEchoEngine()->renderOneFrame(/*dt*/);
		Echo::GetEchoEngine()->postRenderOneFrame();
		g_pRobotObjManager->Tick(g_pMainCamera->getPosition());

		g_pPlayerObjManager->Tick();
		if (g_pMiniMapManager)
			g_pMiniMapManager->Tick();
		// 常驻 TOD 时间 HUD 刷新（FGUI）
		if (g_showTodTimeHud && g_todTimeText)
		{
			Echo::TODManager* mgr = Echo::TODSystem::instance()->GetManager(Echo::WorldSystem::instance()->GetActiveWorld());
			if (mgr)
			{
				double tnow = mgr->calculateCurTodTime();
				int totalSec = (int)(tnow * 24.0 * 3600.0);
				int hh = (totalSec / 3600) % 24;
				int mm = (totalSec % 3600) / 60;
				int ss = totalSec % 60;
				char tbuf[16];
				snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d", hh, mm, ss);
				g_todTimeText->setText(tbuf);
			}
		}
	}
	else
	{
		if (g_pEchoRoot2)
			g_pEchoRoot2->renderMainThread(GetLogicThreadInited());
	}

}

void AsyncApp::onFrameEnd()
{
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		return;
	}
}

//void AsyncApp::setSwapBufferTime(unsigned int time)
//{
//	// 引擎渲染
//	if (Echo::ECHO_THREAD_MT1 != m_uEngineRenderMode)
//	{
//		Echo::GetEchoEngine()->getRoot()->setSwapBufferTime(time);
//	}
//	else
//	{
//		if (g_pEchoRoot2)
//			g_pEchoRoot2->setSwapBufferTime(time);
//	}
//}

bool AsyncApp::isHasImGuiWindowHovered() const {

#if ECHO_PLATFORM == ECHO_PLATFORM_WIN32
	//if (Echo::ECHO_THREAD_MT1 != m_uEngineRenderMode)
	//{
	//	return EditorRoot::instance()->HasImGuiWindowHovered();
	//}
	//else
	//{
	//	g_appview->PostToLogic([]() {
	//		return EditorRoot::instance()->HasImGuiWindowHovered();
	//	});
	//}
	return false;
#else
	return false;
#endif
}

//////////////////////////////////////////////////////////////////////////
void AsyncApp::commandParse(const std::string& command)
{
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([command]() {
			Echo::ConsoleCommand* pConsole = Echo::ConsoleCommand::instance();
		if (!pConsole)
		{
			return;
		}

		Echo::LogManager::instance()->logMessage(command);

		pConsole->executeCommand(command);

		const Echo::CmdLineArg* btest = pConsole->findArg("btest", Echo::eCLAT_Normal, true);
		if (btest != nullptr)
		{
			Echo::vector<Echo::String>::type params = Echo::StringUtil::split(btest->getValue(), "\t ");
			if (params.empty())
				return;

			if (params[0] == "i")
			{
				if (params.size() > 1)
				{
					int build_num = Echo::StringConverter::parseInt(params[1], 0);
					if (0 == build_num)
						return;
					BuildingTest::instance().init(g_pMainCamera->getPosition(), build_num);
				}
			}
			else if (params[0] == "c") {
				BuildingTest::instance().clear();
			}
			else if (params[0] == "f") {
				if (params.size() > 1)
					BuildingTest::instance().setTestFlags(params[1].c_str());
			}
			return;
		}

		const Echo::CmdLineArg* heat = pConsole->findArg("heat", Echo::eCLAT_Normal, true);
		if (heat != nullptr)
		{

			const char* cmd = heat->getValue();
			heat_map.setup();
			heat_map.parsecommand(cmd);
			heat_map.generate(g_pWorldManager);
			return;
		}
		/*
					const Echo::CmdLineArg * c3DUI = pConsole->findArg("c3DUI", Echo::eCLAT_Normal, true);
					if (c3DUI != nullptr)
					{

						const char * cmd = c3DUI->getValue();
						Echo::CameraControl::instance()->create3DUIComponentTest(cmd);
						return;
					}

					const Echo::CmdLineArg * d3DUI = pConsole->findArg("d3DUI", Echo::eCLAT_Normal, true);
					if (d3DUI != nullptr)
					{

						const char * cmd = d3DUI->getValue();
						Echo::CameraControl::instance()->delete3DUIComponentTest(cmd);
						return;
					}

					const Echo::CmdLineArg * dALL3DUI = pConsole->findArg("dALL3DUI", Echo::eCLAT_Normal, true);
					if (dALL3DUI != nullptr)
					{
						Echo::CameraControl::instance()->deleteAll3DUIComponentTest();
						return;
					}
		*/
		const Echo::CmdLineArg* rtest = pConsole->findArg("rtest", Echo::eCLAT_Normal, true);
		if (rtest != nullptr)
		{
			const Echo::uint32 id = rtest->getUIValue();
			if (id == 0xFFFFFFFF)
			{
				PlayerObjManager::instance()->Destroy();
				g_pWorldManager->stopPlayingCameraPath();
			}
			else
			{
				if (id == 88888888)
				{
					Echo::Vector3 posi = g_pMainCamera->getPosition();
					posi.x = posi.x + 20;
					posi.y = g_pWorldManager->getHeightAtWorldPosition(posi);
					PlayerObjManager::instance()->CreatePlayerObject(posi);
				}
				g_pWorldManager->startPlayingCameraPath(id, 3);
			}

			return;
		}

		const Echo::CmdLineArg* fpsArg = pConsole->findArg("fps", Echo::eCLAT_Normal, true);
		if (fpsArg)
		{
			g_wfc.SetFR(fpsArg->getFValue());
			g_wfc.Reset();
			return;
		}

		const Echo::CmdLineArg* todtime = pConsole->findArg("todtime", Echo::eCLAT_Normal, true);
		if (todtime)
		{
			Echo::TODManager* mgr = Echo::TODSystem::instance()->GetManager(Echo::WorldSystem::instance()->GetActiveWorld());
			if (!mgr) { return; }
			bool hasParam = false; int passed = 0;
			{
				Echo::vector<Echo::String>::type toks = Echo::StringUtil::split(command, "\t ");
				for (size_t i = 0; i < toks.size(); ++i) if (toks[i] == "todtime") {
					if (i + 1 < toks.size() && !toks[i + 1].empty()) { hasParam = true; passed = Echo::StringConverter::parseInt(toks[i + 1], 0); }
					break;
				}
			}
			if (hasParam) { mgr->setTimePassed(passed); g_showTodTimeHud = true; EnsureTodUI(); }
			else
			{
				g_showTodTimeHud = !g_showTodTimeHud;
				if (!g_showTodTimeHud) { DestroyTodUI(); return; }
				EnsureTodUI();
			}
			if (g_todTimeText)
			{
				double tnow = mgr->calculateCurTodTime();
				int totalSec = (int)(tnow * 24.0 * 3600.0);
				int hh = (totalSec / 3600) % 24;
				int mm = (totalSec % 3600) / 60;
				int ss = totalSec % 60;
				char tbuf[16];
				snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d", hh, mm, ss);
				g_todTimeText->setText(tbuf);
			}
			return;
		}
		const Echo::CmdLineArg* nl = pConsole->findArg("nl", Echo::eCLAT_Normal, true);
		if (nl != nullptr)
		{
			if (strlen(nl->getValue()) > 0)
			{
				loadNewLevel(nl->getValue());
			}
			else
			{
				const Echo::String& curLevel = g_pWorldManager->getLevelName();
				auto it = g_LevelMap.find(curLevel);
				if (it != g_LevelMap.end()) {
					++it;
				}
				if (it == g_LevelMap.end()) {
					it = g_LevelMap.begin();
				}
				loadNewLevel(it->first);
			}
			return;
		}

		const Echo::CmdLineArg* cs = pConsole->findArg("cs", Echo::eCLAT_Normal, true);
		if (cs != nullptr)
		{
			Echo::CameraControl::instance()->setMoveSpeed(cs->getFValue());
			return;
		}
		{
			float D_value{ 0.0f };
			if (EchoNPCController::Alive() || EchoSphericalController::Alive()) {
				D_value = 5.0f;
			}
			else {
				D_value = 100.0f;
			}
			const Echo::CmdLineArg* csUp = pConsole->findArg("csUp", Echo::eCLAT_Normal, true);
			if (csUp != nullptr) {
				float camSpeed{ Echo::CameraControl::instance()->getMoveSpeed() };
				Echo::CameraControl::instance()->setMoveSpeed(camSpeed + D_value);
				return;
			}
			const Echo::CmdLineArg* csDown = pConsole->findArg("csDown", Echo::eCLAT_Normal, true);
			if (csDown != nullptr) {
				float camSpeed{ Echo::CameraControl::instance()->getMoveSpeed() };
				Echo::CameraControl::instance()->setMoveSpeed(
					((camSpeed - D_value) < 0) ? 1.0f : (camSpeed - D_value));
				return;
			}
		}

		const Echo::CmdLineArg* cr = pConsole->findArg("cr", Echo::eCLAT_Normal, true);
		if (cr != nullptr)
		{
			Echo::Vector3 posi = g_pMainCamera->getPosition();
			g_pRobotObjManager->CreateTestRobotObject(posi, cr->getIValue());
			return;
		}
		const Echo::CmdLineArg* showmap = pConsole->findArg("showmap", Echo::eCLAT_Normal, true);
		if (showmap != nullptr)
		{
			g_pMiniMapManager->SetShowMap(!g_pMiniMapManager->GetShowMap());
		}
		const Echo::CmdLineArg* showcityname = pConsole->findArg("showcityname", Echo::eCLAT_Normal, true);
		if (showcityname != nullptr)
		{
			g_pMiniMapManager->SetShowCityName(!g_pMiniMapManager->GetShowCityName());
		}
		const Echo::CmdLineArg* changemapsize = pConsole->findArg("changemapsize", Echo::eCLAT_Normal, true);
		if (changemapsize != nullptr)
		{
			g_pMiniMapManager->SetShowFullMap(!g_pMiniMapManager->GetShowFullMap());
		}
		const Echo::CmdLineArg* showsceneui = pConsole->findArg("showsceneui", Echo::eCLAT_Normal, true);
		if (showsceneui != nullptr)
		{
			g_pMiniMapManager->SetShowSceneUI(!g_pMiniMapManager->GetShowSceneUI());
		}
		const Echo::CmdLineArg* sceneuidis = pConsole->findArg("sceneuidis", Echo::eCLAT_Normal, true);
		if (sceneuidis != nullptr)
		{
			float dis = sceneuidis->getFValue();
			g_pMiniMapManager->SetSceneUIRenderDistance(dis);
		}
			});
	}
	else
	{
		Echo::ConsoleCommand* pConsole = Echo::ConsoleCommand::instance();
		if (!pConsole)
		{
			return;
		}

		Echo::LogManager::instance()->logMessage(command);

		pConsole->executeCommand(command);

		const Echo::CmdLineArg* heat = pConsole->findArg("heat", Echo::eCLAT_Normal, true);
		if (heat != nullptr)
		{

			const char* cmd = heat->getValue();
			heat_map.setup();
			heat_map.parsecommand(cmd);
			heat_map.generate(g_pWorldManager);
			return;
		}
		/*
				const Echo::CmdLineArg * c3DUI = pConsole->findArg("c3DUI", Echo::eCLAT_Normal, true);
				if (c3DUI != nullptr)
				{

					const char * cmd = c3DUI->getValue();
					Echo::CameraControl::instance()->create3DUIComponentTest(cmd);
					return;
				}

				const Echo::CmdLineArg * d3DUI = pConsole->findArg("d3DUI", Echo::eCLAT_Normal, true);
				if (d3DUI != nullptr)
				{

					const char * cmd = d3DUI->getValue();
					Echo::CameraControl::instance()->delete3DUIComponentTest(cmd);
					return;
				}

				const Echo::CmdLineArg * dALL3DUI = pConsole->findArg("dALL3DUI", Echo::eCLAT_Normal, true);
				if (dALL3DUI != nullptr)
				{
					Echo::CameraControl::instance()->deleteAll3DUIComponentTest();
					return;
				}
		*/
		const Echo::CmdLineArg* rtest = pConsole->findArg("rtest", Echo::eCLAT_Normal, true);
		if (rtest != nullptr)
		{
			const Echo::uint32 id = rtest->getUIValue();
			if (id == 0xFFFFFFFF)
			{
				PlayerObjManager::instance()->Destroy();
				g_pWorldManager->stopPlayingCameraPath();
			}
			else
			{
				if (id == 88888888)
				{
					Echo::Vector3 posi = g_pMainCamera->getPosition();
					posi.x = posi.x + 20;
					posi.y = g_pWorldManager->getHeightAtWorldPosition(posi);
					PlayerObjManager::instance()->CreatePlayerObject(posi);
				}
				g_pWorldManager->startPlayingCameraPath(id, 3);
			}
			return;
		}

		const Echo::CmdLineArg* fpsArg = pConsole->findArg("fps", Echo::eCLAT_Normal, true);
		if (fpsArg)
		{
			setFPS(fpsArg->getFValue());
			return;
		}


		const Echo::CmdLineArg* todtime = pConsole->findArg("todtime", Echo::eCLAT_Normal, true);
		if (todtime)
		{
			Echo::TODManager* mgr = Echo::TODSystem::instance()->GetManager(Echo::WorldSystem::instance()->GetActiveWorld());
			if (!mgr) { return; }
			bool hasParam = false; int passed = 0;
			{
				Echo::vector<Echo::String>::type toks = Echo::StringUtil::split(command, "\t ");
				for (size_t i = 0; i < toks.size(); ++i) if (toks[i] == "todtime") {
					if (i + 1 < toks.size() && !toks[i + 1].empty()) { hasParam = true; passed = Echo::StringConverter::parseInt(toks[i + 1], 0); }
					break;
				}
			}
			if (hasParam) { mgr->setTimePassed(passed); g_showTodTimeHud = true; EnsureTodUI(); }
			else
			{
				g_showTodTimeHud = !g_showTodTimeHud;
				if (!g_showTodTimeHud)
				{
					DestroyTodUI();
					return;
				}
				EnsureTodUI();
			}
			// 初次触发也立即显示一次（强制刷新TOD以立刻反映偏移或当前状态）
			mgr->updateData(0.0, true);
			if (g_todTimeText)
			{
				int totalSec = (int)(mgr->getCurrentTime() * 24.0 * 3600.0);
				int hh = (totalSec / 3600) % 24;
				int mm = (totalSec % 3600) / 60;
				int ss = totalSec % 60;
				char tbuf[16];
				snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d", hh, mm, ss);
				g_todTimeText->setText(tbuf);
			}
			return;
		}

		const Echo::CmdLineArg* nl = pConsole->findArg("nl", Echo::eCLAT_Normal, true);
		if (nl != nullptr)
		{
			if (strlen(nl->getValue()) > 0)
			{
				loadNewLevel(nl->getValue());
			}
			else
			{
				const Echo::String& curLevel = g_pWorldManager->getLevelName();
				auto it = g_LevelMap.find(curLevel);
				if (it != g_LevelMap.end()) {
					++it;
				}
				if (it == g_LevelMap.end()) {
					it = g_LevelMap.begin();
				}
				loadNewLevel(it->first);
			}
			return;
		}

		const Echo::CmdLineArg* cs = pConsole->findArg("cs", Echo::eCLAT_Normal, true);
		if (cs != nullptr)
		{
			Echo::CameraControl::instance()->setMoveSpeed(cs->getFValue());
			return;
		}
		{
			float D_value{ 0.0f };
			if (EchoNPCController::Alive() || EchoSphericalController::Alive()) {
				D_value = 5.0f;
			}
			else {
				D_value = 100.0f;
			}
			const Echo::CmdLineArg* csUp = pConsole->findArg("csUp", Echo::eCLAT_Normal, true);
			if (csUp != nullptr) {
				float camSpeed{ Echo::CameraControl::instance()->getMoveSpeed() };
				Echo::CameraControl::instance()->setMoveSpeed(camSpeed + D_value);
				return;
			}
			const Echo::CmdLineArg* csDown = pConsole->findArg("csDown", Echo::eCLAT_Normal, true);
			if (csDown != nullptr) {
				float camSpeed{ Echo::CameraControl::instance()->getMoveSpeed() };
				Echo::CameraControl::instance()->setMoveSpeed(
					((camSpeed - D_value) < 0) ? 1.0f : (camSpeed - D_value));
				return;
			}
		}

		const Echo::CmdLineArg* createCCTArg = pConsole->findArg("testcct", eCLAT_Normal, true);
		if (createCCTArg)
		{
			int roleNum = createCCTArg->getIValue();
			if (roleNum > 0)
			{
				if (!Echo::EchoFlyingController::isCreated && g_pMainCamera != nullptr)
					new Echo::EchoFlyingController(g_pMainCamera);
				else if (Echo::EchoFlyingController::isCreated)
					Echo::EchoFlyingController::instance()->createFlyCCT(true);
			}
			else
			{
				if (Echo::EchoFlyingController::isCreated)
					delete Echo::EchoFlyingController::instance();
			}
		}

		const Echo::CmdLineArg* createSphericalCCTArg = pConsole->findArg("sphcct", eCLAT_Normal, true);
		if (createSphericalCCTArg)
		{
			if (EchoSphericalController::Alive()) {
				delete EchoSphericalController::instance();
			}
			else {
				new EchoSphericalController(g_pMainCamera);
			}
		}

		const Echo::CmdLineArg* setSphericalCCTPositionArg = pConsole->findArg("scp", eCLAT_Normal, true);
		if (setSphericalCCTPositionArg)
		{
			vector<String>::type value = StringUtil::split(setSphericalCCTPositionArg->getValue());
			Vector3 position = Vector3::ZERO;
			if (value.size() == 3) {
				position = Vector3((float)std::atof(value[0].c_str()), (float)std::atof(value[1].c_str()), (float)std::atof(value[2].c_str()));
			}
			if (EchoSphericalController::Alive()) {
				EchoSphericalController::instance()->SetCharlPosition(position);
			}
			else if (g_pMainCamera) {
				g_pMainCamera->setPosition(position);
			}
		}

		const Echo::CmdLineArg* setSphericalCCTGravityArg = pConsole->findArg("scg", eCLAT_Normal, true);
		if (setSphericalCCTGravityArg)
		{
			float gravity = setSphericalCCTGravityArg->getFValue();
			if (EchoSphericalController::Alive()) {
				EchoSphericalController::instance()->SetGravity(gravity);
			}
		}

		const Echo::CmdLineArg* setSphericalCCTCameraDelayArg = pConsole->findArg("scas", eCLAT_Normal, true);
		if (setSphericalCCTCameraDelayArg)
		{
			float cameraDelay = setSphericalCCTCameraDelayArg->getFValue();
			if (EchoSphericalController::Alive()) {
				EchoSphericalController::instance()->SetCameraAngleSpeed(cameraDelay);
			}
		}

		const Echo::CmdLineArg* setSphericalCCTRoleDelayArg = pConsole->findArg("sras", eCLAT_Normal, true);
		if (setSphericalCCTRoleDelayArg)
		{
			float roleDelay = setSphericalCCTRoleDelayArg->getFValue();
			if (EchoSphericalController::Alive()) {
				EchoSphericalController::instance()->SetRoleAngleSpeed(roleDelay);
			}
		}

		const Echo::CmdLineArg* setSphericalCCTSpeedModeArg = pConsole->findArg("scsg", eCLAT_Normal, true);
		if (setSphericalCCTSpeedModeArg)
		{
			float speed = setSphericalCCTSpeedModeArg->getFValue();
			CameraControl::instance()->setMoveSpeed(speed * 30.0f);
			if (EchoSphericalController::Alive()) {
				EchoSphericalController::instance()->SetGravity(speed * 0.4f);
			}
		}

		const Echo::CmdLineArg* setSphericalCCTMaxSpeedMulArg = pConsole->findArg("scmsm", eCLAT_Normal, true);
		if (setSphericalCCTMaxSpeedMulArg)
		{
			float maxSpeedMul = setSphericalCCTMaxSpeedMulArg->getFValue();
			if (EchoSphericalController::Alive()) {
				EchoSphericalController::instance()->SetMaxSpeedMul(maxSpeedMul);
			}
		}

		const Echo::CmdLineArg* sphericalTerrainMoveTestModeArg = pConsole->findArg("sttm", eCLAT_Normal, true);
		if (sphericalTerrainMoveTestModeArg)
		{
			String terrainName = "default_3dprocedural_terrain0";
			Vector3 origin = Vector3(0.0f, 22000.0f, 0.0f);
			if (EchoSphericalController::Alive()) {
				delete EchoSphericalController::instance();
			}
			new EchoSphericalController(g_pMainCamera, terrainName, origin);
			float speed = sphericalTerrainMoveTestModeArg->getFValue();
			CameraControl::instance()->setMoveSpeed(speed * 30.0f);
			if (EchoSphericalController::Alive()) {
				EchoSphericalController::instance()->SetGravity(speed * 0.4f);
			}
		}

		const Echo::CmdLineArg* createSphericalTerrainSArg = pConsole->findArg("testsph", eCLAT_Normal, true);
		if (createSphericalTerrainSArg)
		{
			vector<String>::type value = StringUtil::split(createSphericalTerrainSArg->getValue());
			String terrainName = "default_3dprocedural_terrain0";
			Vector3 origin = Vector3(0.0f, 22000.0f, 0.0f);
			if (value.size() >= 3) {
				origin = Vector3((float)std::atof(value[0].c_str()), (float)std::atof(value[1].c_str()), (float)std::atof(value[2].c_str()));
				if (value.size() == 4) {
					terrainName = value[3];
				}
			}
			CameraControl::instance()->setMoveSpeed(100.0f);
			if (EchoSphericalController::Alive()) {
				delete EchoSphericalController::instance();
			}
			new EchoSphericalController(g_pMainCamera, terrainName, origin);
		}

		const Echo::CmdLineArg* addSphericalTerrainSArg = pConsole->findArg("addsph", eCLAT_Normal, true);
		if (addSphericalTerrainSArg)
		{
			vector<String>::type value = StringUtil::split(addSphericalTerrainSArg->getValue());
			String terrainName = "default_3dprocedural_terrain0";
			Vector3 origin = Vector3::ZERO;
			float scale = 1.0f;
			if (value.size() >= 3) {
				origin = Vector3((float)std::atof(value[0].c_str()), (float)std::atof(value[1].c_str()), (float)std::atof(value[2].c_str()));
				if (value.size() >= 4) {
					scale = (float)std::atof(value[3].c_str());
					if (value.size() == 5) {
						terrainName = value[4];
					}
				}
			}
			EchoSphericalController::createSphericalTerrain(terrainName, origin, scale);
		}

		const Echo::CmdLineArg* showSphericalVoronoiRegion = pConsole->findArg("ssvr", eCLAT_Normal, true);
		if (showSphericalVoronoiRegion)
		{
			int value = showSphericalVoronoiRegion->getIValue();
			if (g_pMainSceneManager->getPlanetManager())
			{
				//g_pMainSceneManager->getPlanetManager()->displaySphericalVoronoiRegion((bool)value);
			}
		}

		const Echo::CmdLineArg* createNPCControllerArg = pConsole->findArg("npc", eCLAT_Normal, true);
		if (createNPCControllerArg)
		{
			if (EchoNPCController::Alive()) {
				delete EchoNPCController::instance();
			}
			else {
				new EchoNPCController(g_pMainCamera);
			}
		}
		const Echo::CmdLineArg* createTransmitPosArg = pConsole->findArg("transmit", eCLAT_Normal, true);
		if (createTransmitPosArg)
		{
			vector<String>::type vec = StringUtil::split(createTransmitPosArg->getValue(), ",");
			float x{}, y{}, z{};
			if (vec.size() == 3) {
				x = StringConverter::parseFloat(vec[0]);
				y = StringConverter::parseFloat(vec[1]);
				z = StringConverter::parseFloat(vec[2]);
				if (EchoNPCController::Alive()) {
					EchoNPCController::instance()->setRolePosition(x, z);
				}
				else if (EchoSphericalController::Alive()) {
					EchoSphericalController::instance()->SetCharlPosition(Vector3{ x, y, z });
				}
				else {
					SceneManager* mainSceneManager = Root::instance()->getMainSceneManager();
					if (mainSceneManager)
					{
						Camera* pCamera = mainSceneManager->getMainCamera();
						if (pCamera)
							pCamera->setPosition(DVector3{ x, y, z });
					}
				}
			}
		}
		const Echo::CmdLineArg* testMulArg = pConsole->findArg("testMul", eCLAT_Normal, true);
		if (testMulArg)
		{
			std::string world = testMulArg->getValue();
			Name worldName = Name(world);
			if (!GetEchoEngine()->HasWorld(worldName))
			{
				GetEchoEngine()->CreateWorld(worldName);
				//test
				if (worldName == "Next")
					GetEchoEngine()->changeLevel(worldName, "testMul");
			}
			GetEchoEngine()->ChangeWorld(worldName);
		}

		const Echo::CmdLineArg* changeWorldArg = pConsole->findArg("changeWorld", eCLAT_Normal, true);
		if (changeWorldArg)
		{
			std::string world = changeWorldArg->getValue();
			Name worldName = Name(world);
			if (GetEchoEngine()->HasWorld(worldName))
			{
				GetEchoEngine()->ChangeWorld(worldName);
#if ECHO_PLATFORM == ECHO_PLATFORM_WIN32
#ifdef ECHO_EDITOR
				//EditorRoot::instance()->ChangeWorldImpl();
#endif
#endif
			}
		}
		const Echo::CmdLineArg* createWorldArg = pConsole->findArg("createWorld", eCLAT_Normal, true);
		if (createWorldArg)
		{
			vector<String>::type vec = StringUtil::split(createWorldArg->getValue());
			std::string world = vec[0];
			Name worldName = Name(world);
			if (!GetEchoEngine()->HasWorld(worldName))
			{
				GetEchoEngine()->CreateWorld(worldName);
				if (vec.size() > 1) {
					GetEchoEngine()->changeLevel(worldName, vec[1]);
				}
				else {
					GetEchoEngine()->changeLevel(worldName, "taikong");
				}
				GetEchoEngine()->SetActiveWorld(worldName, true);
			}
		}

		const Echo::CmdLineArg* removeWorldArg = pConsole->findArg("removeWorld", eCLAT_Normal, true);
		if (removeWorldArg)
		{
			std::string world = removeWorldArg->getValue();
			Name worldName = Name(world);
#if ECHO_PLATFORM == ECHO_PLATFORM_WIN32
#ifdef ECHO_EDITOR
			//EditorRoot::instance()->RemoveWorldImpl(worldName);
#endif
#endif
			GetEchoEngine()->RemoveWorld(worldName);
#if ECHO_PLATFORM == ECHO_PLATFORM_WIN32
#ifdef ECHO_EDITOR
			//EditorRoot::instance()->ChangeWorldImpl();
#endif
#endif
		}
		const Echo::CmdLineArg* showmap = pConsole->findArg("showmap", Echo::eCLAT_Normal, true);
		if (showmap != nullptr)
		{
			g_pMiniMapManager->SetShowMap(!g_pMiniMapManager->GetShowMap());
		}
		const Echo::CmdLineArg* showcityname = pConsole->findArg("showcityname", Echo::eCLAT_Normal, true);
		if (showcityname != nullptr)
		{
			g_pMiniMapManager->SetShowCityName(!g_pMiniMapManager->GetShowCityName());
		}
		const Echo::CmdLineArg* changemapsize = pConsole->findArg("changemapsize", Echo::eCLAT_Normal, true);
		if (changemapsize != nullptr)
		{
			g_pMiniMapManager->SetShowFullMap(!g_pMiniMapManager->GetShowFullMap());
		}
		const Echo::CmdLineArg* showsceneui = pConsole->findArg("showsceneui", Echo::eCLAT_Normal, true);
		if (showsceneui != nullptr)
		{
			g_pMiniMapManager->SetShowSceneUI(!g_pMiniMapManager->GetShowSceneUI());
		}
		const Echo::CmdLineArg* sceneuidis = pConsole->findArg("sceneuidis", Echo::eCLAT_Normal, true);
		if (sceneuidis != nullptr)
		{
			float dis = sceneuidis->getFValue();
			g_pMiniMapManager->SetSceneUIRenderDistance(dis);
		}

		const Echo::CmdLineArg* changeSphericalTerrainControllerModelSArg = pConsole->findArg("scctm", eCLAT_Normal, true);
		if (changeSphericalTerrainControllerModelSArg)
		{
			if (EchoSphericalController::Alive()) {
				EchoSphericalController::instance()->SetControllerModel(!EchoSphericalController::instance()->GetControllerModel());
			}
		}

		const Echo::CmdLineArg* changeSphericalTerrainControllerPhysXSArg = pConsole->findArg("scctp", eCLAT_Normal, true);
		if (changeSphericalTerrainControllerPhysXSArg)
		{
			if (EchoSphericalController::Alive()) {
				EchoSphericalController::instance()->SetPhysXController(!EchoSphericalController::instance()->GetPhysXController());
			}
		}
	}

}

void AsyncApp::onMouseMove(const Echo::Vector2& point)
{
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([point]() {
			auto protocolMgr = ProtocolComponentSystem::instance();
		struct MouseInfo
		{
			float x, y;
		};
		MouseInfo mouse = { point.x, point.y };
		protocolMgr->callProtocolFunc(Echo::Name("fguiOnMouseLeftMove"), &mouse, 0); }
		);
	}
	else
	{
		auto protocolMgr = ProtocolComponentSystem::instance();
		struct MouseInfo
		{
			float x, y;
		};
		MouseInfo mouse = { point.x, point.y };
		protocolMgr->callProtocolFunc(Echo::Name("fguiOnMouseLeftMove"), &mouse, 0);
	}
}

void AsyncApp::onMouseLeftDown(const Echo::Vector2& point)
{
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([point]() {
			auto protocolMgr = ProtocolComponentSystem::instance();
		struct MouseInfo
		{
			float x, y;
		};
		MouseInfo mouse = { point.x, point.y };
		protocolMgr->callProtocolFunc(Echo::Name("fguiOnMouseLeftDown"), &mouse, 0); }
		);
	}
	else
	{
		auto protocolMgr = ProtocolComponentSystem::instance();
		struct MouseInfo
		{
			float x, y;
		};
		MouseInfo mouse = { point.x, point.y };
		protocolMgr->callProtocolFunc(Echo::Name("fguiOnMouseLeftDown"),
			&mouse,
			0);
	}
}

void AsyncApp::onMouseLeftUp(const Echo::Vector2& point)
{
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([point]() {
			auto protocolMgr = ProtocolComponentSystem::instance();
		struct MouseInfo
		{
			float x, y;
		};
		MouseInfo mouse = { point.x, point.y };
		protocolMgr->callProtocolFunc(Echo::Name("fguiOnMouseLeftUp"), &mouse, 0); }
		);
	}
	else
	{
		auto protocolMgr = ProtocolComponentSystem::instance();
		struct MouseInfo
		{
			float x, y;
		};
		MouseInfo mouse = { point.x, point.y };
		protocolMgr->callProtocolFunc(Echo::Name("fguiOnMouseLeftUp"),
			&mouse,
			0);
	}
}

void AsyncApp::OnInputText(char16_t c)
{

	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([c]()
			{
				auto protocolMgr = ProtocolComponentSystem::instance();
		struct KeyInfo
		{
			char16_t key;
		};
		KeyInfo keyInfo = { c };
		protocolMgr->callProtocolFunc(Echo::Name("fguiOnKeyDown"), &keyInfo, 0);
			}
		);
	}
	else
	{
		auto protocolMgr = ProtocolComponentSystem::instance();
		struct KeyInfo
		{
			char16_t key;
		};
		KeyInfo keyInfo = { c };
		protocolMgr->callProtocolFunc(Echo::Name("fguiOnKeyDown"),
			&keyInfo,
			0);
	}
}

void AsyncApp::cameraMove(int type, int val)
{
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([type, val]() {
			if (EchoSphericalController::Alive()) {
				EchoSphericalController::instance()->update(0, (float)type, (float)val);
				return;
			}
		if (EchoNPCController::Alive()) {
			EchoNPCController::instance()->update(0, (float)type, (float)val);
			return;
		}
		if (!Echo::EchoFlyingController::isCreated)
			CameraControl::instance()->setMoveDir(type, val);
		else
			Echo::EchoFlyingController::instance()->updateFlyCCT(0, (float)type, (float)val);
			});
	}
	else
	{
		if (EchoSphericalController::Alive()) {
			EchoSphericalController::instance()->update(0, (float)type, (float)val);
			return;
		}
		if (EchoNPCController::Alive()) {
			EchoNPCController::instance()->update(0, (float)type, (float)val);
			return;
		}
		if (!Echo::EchoFlyingController::isCreated)
			CameraControl::instance()->setMoveDir(type, val);
		else
			Echo::EchoFlyingController::instance()->updateFlyCCT(0, (float)type, (float)val);
	}
}

#if ECHO_PLATFORM == ECHO_PLATFORM_WIN32
#define CAMERA_TURN_SCALE 1.0f
#else
#define CAMERA_TURN_SCALE 0.7f
#endif

void AsyncApp::cameraTurn(float x, float y)
{
	if (Echo::ECHO_THREAD_MT1 == g_appview->GetEngineRenderMode())
	{
		g_appview->PostToLogic([x, y]() {
			if (EchoSphericalController::Alive()) {
				EchoSphericalController::instance()->update(1, x, y);
				return;
			}
		if (EchoNPCController::Alive()) {
			EchoNPCController::instance()->update(1, x, y);
			return;
		}
		if (!Echo::EchoFlyingController::isCreated)
			CameraControl::instance()->cameraTurn(x * CAMERA_TURN_SCALE, y * CAMERA_TURN_SCALE);
		else
			Echo::EchoFlyingController::instance()->updateFlyCCT(1, x * CAMERA_TURN_SCALE, y * CAMERA_TURN_SCALE);
			});
	}
	else
	{
		if (EchoSphericalController::Alive()) {
			EchoSphericalController::instance()->update(1, x, y);
			return;
		}
		if (EchoNPCController::Alive()) {
			EchoNPCController::instance()->update(1, x, y);
			return;
		}
		if (!Echo::EchoFlyingController::isCreated)
			CameraControl::instance()->cameraTurn(x * CAMERA_TURN_SCALE, y * CAMERA_TURN_SCALE);
		else
			Echo::EchoFlyingController::instance()->updateFlyCCT(1, x * CAMERA_TURN_SCALE, y * CAMERA_TURN_SCALE);
	}
}

void AsyncApp::cameraDisChg(int val)
{
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([val]() {
			if (EchoSphericalController::Alive()) {
				EchoSphericalController::instance()->update(2, 0, (float)val);
				return;
			}
		if (EchoNPCController::Alive()) {
			EchoNPCController::instance()->update(2, 0, (float)val);
			return;
		}
		Echo::CameraControl::instance()->cameraDisChg(val);
			});
	}
	else
	{
		if (EchoSphericalController::Alive()) {
			EchoSphericalController::instance()->update(2, 0, (float)val);
			return;
		}
		if (EchoNPCController::Alive()) {
			EchoNPCController::instance()->update(2, 0, (float)val);
			return;
		}
		Echo::CameraControl::instance()->cameraDisChg(val);
	}
}

void AsyncApp::setMoveDir(float x, float y)
{
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([x, y]() {
			if (EchoSphericalController::Alive()) {
				EchoSphericalController::instance()->update(0, 0.0f, x);
				EchoSphericalController::instance()->update(0, 1.0f, y);
				return;
			}
		if (EchoNPCController::Alive()) {
			EchoNPCController::instance()->update(0, 0.0f, x);
			EchoNPCController::instance()->update(0, 1.0f, y);
			return;
		}
		Echo::CameraControl::instance()->setMoveDir(x, y);
			});
	}
	else
	{
		if (EchoSphericalController::Alive()) {
			EchoSphericalController::instance()->update(0, 0.0f, x);
			EchoSphericalController::instance()->update(0, 1.0f, y);
			return;
		}
		if (EchoNPCController::Alive()) {
			EchoNPCController::instance()->update(0, 0.0f, x);
			EchoNPCController::instance()->update(0, 1.0f, y);
			return;
		}
		Echo::CameraControl::instance()->setMoveDir(x, y);
	}
}

void AsyncApp::setTurnDir(float x, float y)
{
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([x, y]() {
			if (EchoSphericalController::Alive()) {
				EchoSphericalController::instance()->update(1, x, y);
				return;
			}
		if (EchoNPCController::Alive()) {
			EchoNPCController::instance()->update(1, x, y);
			return;
		}
		Echo::CameraControl::instance()->setTurnDir(x, y);
			});
	}
	else
	{
		if (EchoSphericalController::Alive()) {
			EchoSphericalController::instance()->update(1, x, y);
			return;
		}
		if (EchoNPCController::Alive()) {
			EchoNPCController::instance()->update(1, x, y);
			return;
		}
		Echo::CameraControl::instance()->setTurnDir(x, y);
	}
}

void AsyncApp::incCameraSpeed()
{
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([]() {
			float fSpeed = Echo::CameraControl::instance()->getMoveSpeed();
		fSpeed *= 1.1f;
		if (fSpeed < 0.01f)
			fSpeed = 0.01f;
		Echo::CameraControl::instance()->setMoveSpeed(fSpeed);
			});
	}
	else
	{
		float fSpeed = Echo::CameraControl::instance()->getMoveSpeed();
		fSpeed *= 1.1f;
		if (fSpeed < 0.01f)
			fSpeed = 0.01f;
		Echo::CameraControl::instance()->setMoveSpeed(fSpeed);
	}
}

void AsyncApp::decCameraSpeed()
{
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([]() {
			float fSpeed = Echo::CameraControl::instance()->getMoveSpeed();
		fSpeed /= 1.1f;
		if (fSpeed < 0.01f)
			fSpeed = 0.01f;
		Echo::CameraControl::instance()->setMoveSpeed(fSpeed);
			});
	}
	else
	{
		float fSpeed = Echo::CameraControl::instance()->getMoveSpeed();
		fSpeed /= 1.1f;
		if (fSpeed < 0.01f)
			fSpeed = 0.01f;
		Echo::CameraControl::instance()->setMoveSpeed(fSpeed);
	}
}

void AsyncApp::attack()
{
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([]() {
			Echo::CameraControl::instance()->attack();
			});
	}
	else
	{
		Echo::CameraControl::instance()->attack();
	}
}

void AsyncApp::switchPlayMode()
{
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([]() {
			if (Echo::CameraControl::instance()->GetPlayMode() == Echo::MODE_PLAY)
			{
				Echo::CameraControl::instance()->SetPlayMode(Echo::MODE_FREE);
			}
			else
			{
				Echo::CameraControl::instance()->SetPlayMode(Echo::MODE_PLAY);
				Echo::CameraControl::instance()->AttachCamera();
			}
			});
	}
	else
	{
		if (Echo::CameraControl::instance()->GetPlayMode() == Echo::MODE_PLAY)
		{
			Echo::CameraControl::instance()->SetPlayMode(Echo::MODE_FREE);
		}
		else
		{
			Echo::CameraControl::instance()->SetPlayMode(Echo::MODE_PLAY);
			Echo::CameraControl::instance()->AttachCamera();
		}
	}
}

void AsyncApp::createRobotObject()
{
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([]() {
			Echo::Vector3 posi = g_pMainCamera->getPosition();
		RobotObjManager::instance()->CreateRobotObject(posi);
			});
	}
	else
	{
		Echo::Vector3 posi = g_pMainCamera->getPosition();
		RobotObjManager::instance()->CreateRobotObject(posi);
	}
}

void AsyncApp::sphericalControlerJump()
{
	if (!EchoSphericalController::Alive()) {
		return;
	}
	EchoSphericalController::instance()->update(3, 0.0f, 0.0f);
}

int AsyncApp::getFps()
{
	static unsigned int result = 0;
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([]() {
			result = g_pEchoRoot2->GetFps();
			});
	}
	else
	{
		result = g_pEchoRoot2->GetFps();
	}

	return result;
}

int AsyncApp::getDrawCall()
{
	static unsigned int result = 0;
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([]() {
			result = g_pEchoRoot2->getDrawCount();
			});
	}
	else
	{
		result = g_pEchoRoot2->getDrawCount();
	}

	return result;
}

int AsyncApp::getTriangleCnt()
{
	static unsigned int result = 0;
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([]() {
			result = g_pEchoRoot2->getTriangleCount();
			});
	}
	else
	{
		result = g_pEchoRoot2->getTriangleCount();
	}

	return result;
}

std::string AsyncApp::getCameraPosition()
{
	static std::string result = "<null>";
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([]() {
			if (g_pMainCamera != nullptr) {
				const Echo::Vector3& pos = g_pMainCamera->getPosition();
				char szBuf[64] = { '\0' };
				snprintf(szBuf, sizeof(szBuf) - 1, "%.2f %.2f %.2f",
					pos.x, pos.y, pos.z);
				result = szBuf;
			}
			});
	}
	else
	{
		if (g_pMainCamera != nullptr) {
			const Echo::Vector3& pos = g_pMainCamera->getPosition();
			char szBuf[64] = { '\0' };
			snprintf(szBuf, sizeof(szBuf) - 1, "%.2f %.2f %.2f",
				pos.x, pos.y, pos.z);
			result = szBuf;
		}
	}

	return result;
}

std::string AsyncApp::getCameraOrientation()
{
	static std::string result = "<null>";
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([]() {
			if (g_pMainCamera != nullptr) {
				const Echo::Quaternion& ori = g_pMainCamera->getOrientation();
				char szBuf[64] = { '\0' };
				snprintf(szBuf, sizeof(szBuf) - 1, "%f %f %f %f",
					ori.w, ori.x, ori.y, ori.z);
				result = szBuf;
			}
			});
	}
	else
	{
		if (g_pMainCamera != nullptr) {
			const Echo::Quaternion& ori = g_pMainCamera->getOrientation();
			char szBuf[64] = { '\0' };
			snprintf(szBuf, sizeof(szBuf) - 1, "%f %f %f %f",
				ori.w, ori.x, ori.y, ori.z);
			result = szBuf;
		}
	}

	return result;
}

std::string AsyncApp::getResMemUsageStr()
{
	static std::string result = "";
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([]() {
			if (g_pWorldManager != nullptr && g_pWorldManager->isDrawResMemUsage())
			{
				result = g_pWorldManager->getResMemUsage(g_pWorldManager->isDrawResMemUsageDetail());
			}
			});
	}
	else
	{
		if (g_pWorldManager != nullptr && g_pWorldManager->isDrawResMemUsage())
		{
			result = g_pWorldManager->getResMemUsage(g_pWorldManager->isDrawResMemUsageDetail());
		}
	}

	return result;
}

void AsyncApp::create3DUIGunTest()
{
	//if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	//{
	//	g_appview->PostToLogic([]() {
	//		Echo::CameraControl::instance()->createSpaceshipBeforeCamera(Echo::Name("role/jx_102_xingjian/jx_102_xingjian.role"));
	//		});
	//}
	//else
	//{
	//	Echo::CameraControl::instance()->createSpaceshipBeforeCamera(Echo::Name("role/jx_102_xingjian/jx_102_xingjian.role"));
	//}

}

void AsyncApp::loadSenceByName(const std::string& name) {
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_appview->PostToLogic([name]() {
			Echo::GetEchoEngine()->loadNewLevel(name);
			});
	}
	else
	{
		Echo::GetEchoEngine()->loadNewLevel(name);
	}
}

void resetMainCamera()
{
	//g_pMainCamera->setPosition(0,100,0);  //test
	g_pMainCamera->setPosition(-543.349304f, 343.618347f, 259.909821f);  // test02
	//g_pMainCamera->setPosition(-1105.41f, 15.82f, 11082.0f);  // scene01
	g_pMainCamera->setOrientation(Echo::Quaternion(0.002693f, -0.992807f, -0.117517f, -0.022738f));
	g_pMainCamera->setFarClipDistance(50000.0f);
	g_pMainCamera->setNearClipDistance(0.1f);
	g_pMainCamera->setCullingFactor(0.003f);
	g_pMainCamera->setFOVy(Echo::Radian(Echo::Degree(50)));
}

void SetInitLevel(const std::string& levelName) {
	g_InitLevel = levelName;
}

void SetRunTest(const bool isRunTest) {
	g_runTest = isRunTest;
}

bool loadNewLevel(const std::string& szLevelName)
{
	//if (g_pWorldManager->isLevelLoaded())
	//{
	//	g_pWorldManager->unloadLevel();
	//}

	//bool b = g_pWorldManager->loadLevel(szLevelName);
	bool b = Echo::GetEchoEngine()->loadNewLevel(szLevelName);
	g_pWorldManager->setSceneSeamlessEnable(false);

	const auto& it = g_LevelMap.find(szLevelName);
	if (it != g_LevelMap.end())
	{
		g_pMainCamera->setPosition(std::get<0>(it->second));
		g_pMainCamera->setOrientation(std::get<1>(it->second));
	}
	return b;
}

//////////////////////////////////////////////////////////////////////////
class GameLogic
{
public:
	GameLogic(const char* _assetPath, const char* _dataPath, const Echo::ECHO_GRAPHIC_PARAM* pGraphicParam, Echo::uint32 renderMode, bool bClientMode, Echo::uint32 width, Echo::uint32 height);
	~GameLogic();
	void PreTick();
	void OnTick(float _delta_s);
	void OnTickEnd();
	void InitEditorRoot();
	void ReleaseEditorRoot();

private:
	unsigned int    m_uEngineRenderMode;
	CmnLogic* m_curLogic;
	Echo::uint32        m_width;
	Echo::uint32        m_height;
};

GameLogic::GameLogic(const char* _assetPath, const char* _dataPath, const Echo::ECHO_GRAPHIC_PARAM* pGraphicParam, Echo::uint32 renderMode, bool bClientMode, Echo::uint32 width, Echo::uint32 height)
	: m_uEngineRenderMode(renderMode)
	, m_curLogic(nullptr)
	, m_width(width)
	, m_height(height)
{
	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		if (!Echo::GetEchoEngine()->getRoot())
		{
			std::string sLogFileName = "Echo.log";

			Echo::IArchive* pArchive = Echo::GetArchivePack();

#ifndef _WIN32
			sLogFileName.insert(0, _dataPath);
#endif

			bool bForceUseGxRender = false;

			Root::RuntimeMode mode = Root::eRM_Client;
			if (!bClientMode)
				mode = Root::eRM_Editor;

			Echo::GetEchoEngine()->init(_assetPath, pArchive, mode, sLogFileName, bForceUseGxRender, false, true);
#if ECHO_PLATFORM == ECHO_PLATFORM_WIN32
			Echo::GetEchoEngine()->setShaderPath(_assetPath, _dataPath);
			Echo::GetEchoEngine()->setRenderSystem(pGraphicParam, _assetPath, m_width, m_height);
#else
			Echo::GetEchoEngine()->setRenderSystem(pGraphicParam, _dataPath, m_width, m_height);
#endif
			//Echo::CShaderMan::instance()->setShaderMaskEditorEnable(true);
			Echo::GetEchoEngine()->initConfig();

			g_pEchoRoot2 = Echo::GetEchoEngine()->getRoot();
			g_pMainSceneManager = Echo::Root::instance()->getMainSceneManager();
			g_pMainCamera = g_pMainSceneManager->getMainCamera();
			g_pWorldManager = Echo::WorldSystem::instance()->GetMainWorldManager();

			InitEditorRoot();

			g_pRobotObjManager = new RobotObjManager();
			g_pRobotObjManager->SetSceneManager(g_pMainSceneManager);

			g_pPlayerObjManager = new PlayerObjManager();
			g_pPlayerObjManager->SetSceneManager(g_pMainSceneManager);

#ifdef __ANDROID__
			AttachNativeThreadFMOD();
#endif

			Echo::BellSystem* pBellSystem = Echo::BellSystem::instance();
			if (NULL != pBellSystem)
			{
				if (pBellSystem->initialise("Bank/Master.bank", "Bank/Master.strings.bank"))
					pBellSystem->setListenerCamera(g_pMainCamera);
			}

#ifdef __ANDROID__
			DetachNativeThreadFMOD();
#endif

			resetMainCamera();

			loadNewLevel(g_InitLevel);

			g_isInitialized = true;
			heat_map.setPath(_dataPath);

		}
	}
}

GameLogic::~GameLogic()
{
	if (m_curLogic)
		delete m_curLogic;

	if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
	{
		g_isInitialized = false;

		BuildingTest::instance().clear();

		SAFE_DELETE(g_pRobotObjManager);
		SAFE_DELETE(g_pPlayerObjManager);

		Echo::GetEchoEngine()->shutdown();

		SAFE_DELETE(g_pComponentSystem);

		Echo::GetEchoEngine()->deinit();

		g_pEchoRoot2 = nullptr;
		g_pMainSceneManager = nullptr;
		g_pMainCamera = nullptr;
		g_pWorldManager = nullptr;
	}
}

void GameLogic::PreTick()
{
	if (m_curLogic == nullptr)
	{
#if !defined(CON_EDITOR) && !BUILDER
		if (g_mainXml.empty())
			m_curLogic = new UpdateLogic();
		else
#endif
			m_curLogic = new applogic();
	}

	m_curLogic->PreTick();
}
void callBack(const Vector3& cameraNewPos, int nPlayPercent);

void GameLogic::OnTick(float dt)
{
	m_curLogic->OnTick(dt);

	if (g_isInitialized)
	{
		//引擎更新
		if (Echo::ECHO_THREAD_MT1 == m_uEngineRenderMode)
		{
			//更新触发器对象
			Echo::PlayMode mode = Echo::CameraControl::instance()->GetPlayMode();

			Echo::Vector3 pos;
			if (mode == Echo::PlayMode::MODE_FREE)
			{
				pos = g_pMainCamera->getDerivedPosition();
			}
			else if (mode == Echo::PlayMode::MODE_PLAY)
			{
				Echo::CharacterObj* pcharobj = Echo::CameraControl::instance()->getCharObj();
				if (pcharobj)
				{
					Echo::RoleObject* prole = pcharobj->getRoleObj();
					if (prole)
					{
						pos = prole->getPosition();
					}
				}
			}

			Echo::TriggerAreaManager::instance()->update(pos, g_pMainCamera->getDerivedOrientation(), g_pMainCamera->getDerivedPosition());
			Echo::GravityAreaManager::instance()->update(pos);

			if (g_pWorldManager)
				g_pWorldManager->updateRolePos(pos);
			Echo::GetEchoEngine()->preRenderOneFrame();
			if (EchoSphericalController::Alive()) {
				EchoSphericalController::instance()->tick();
			}
			if (EchoNPCController::Alive()) {
				EchoNPCController::instance()->tick();
			}
			Echo::GetEchoEngine()->renderOneFrame(dt);
			Echo::GetEchoEngine()->postRenderOneFrame();
			g_pRobotObjManager->Tick(g_pMainCamera->getPosition());

			g_pPlayerObjManager->Tick();
		}
		// Debug code.

		// Debug code end
		g_wfc.Tick();
	}
}

void GameLogic::OnTickEnd()
{
	m_curLogic->OnTickEnd();

	if (fReinit)
	{
		delete m_curLogic;
		m_curLogic = nullptr;
		fReinit = false;
	}
}

void GameLogic::InitEditorRoot()
{
	g_pComponentSystem = new ComponentSystem();

	g_pComponentSystem->init(PADBO2COMREGISTERLIST);

#if ECHO_PLATFORM == ECHO_PLATFORM_WIN32
	//g_pEditorRoot = new Echo::EditorRoot();
	//g_pEditorRoot->isQtEditor = g_pComponentSystem->getIsQtInit();
	//g_pEditorRoot->RootPostInit(true);
	/*dll相关(plugin)
	QString exePath = QCoreApplication::applicationDirPath();
	std::set<Echo::Name> suffixSet;
	suffixSet.insert(Echo::Name("dll"));
	EditorAssetList dllList(suffixSet);
	dllList.SetAssetRoot(exePath.toStdString() + "/plugin");
	AssetFolder folders;
	dllList.GetAssetList(folders);
	for (auto& iter : folders.aesstFileList)
	{
		std::string dllPath = "plugin";
		dllPath += iter.fileAllPath.c_str();
		Echo::EditorRoot::instance()->LoadExDLL(Name(dllPath));
	}

	EditorObjProFunctionImp::initProtocolFunction();
	EditorParticleProFuncImp::initProtocolFunction();
	EditorRoleObjProFuncImp::initProtocolFunction();
	EditorHelperObjProFuncImp::initProtocolFunction();
	SceneObjAreaGroupProFuncImp::initProtocolFunction();
	*/
	//isEditorRoot = true;
#endif
}

void GameLogic::ReleaseEditorRoot()
{
	//SceneObjAreaGroupProFuncImp::uninitProtocolFunction();
	//EditorHelperObjProFuncImp::uninitProtocolFunction();
	//EditorRoleObjProFuncImp::uninitProtocolFunction();
	//EditorParticleProFuncImp::uninitProtocolFunction();
	//EditorObjProFunctionImp::uninitProtocolFunction();

	delete g_pComponentSystem;

#if ECHO_PLATFORM == ECHO_PLATFORM_WIN32
	//g_pEditorRoot->ReleaseRootPost();
	//delete g_pEditorRoot;
	//isEditorRoot = false;
#endif
}

unsigned int GetEngineRenderMode()
{
	Echo::IArchive* pArchive = Echo::GetArchivePack();

	Echo::SystemConfigFile cfg(pArchive);
	cfg.load("echo/EchoEngine.cfg");
	Echo::String sPlatform;

#ifdef _WIN32
	sPlatform = "Windows";
#elif defined(__ANDROID__)
	sPlatform = "Android";
#elif defined(__APPLE__)
	sPlatform = "IOS";
#elif defined(__OHOS__)
	sPlatform = "Harmony";
#endif

	const Echo::SettingsMultiMap* pSetMap = cfg.getSettingsMap(sPlatform);
	if (pSetMap)
	{
		Echo::SettingsMultiMap::const_iterator it = pSetMap->find("RenderMode");
		if (it != pSetMap->end())
		{
			return Echo::StringConverter::parseUnsignedInt(it->second);
		}
	}

	return Echo::ECHO_THREAD_ST;
}
//////////////////////////////////////////////////////////////////////////
void InitAsyncApp(const char* _assetPath, const char* _dataPath, const Echo::ECHO_GRAPHIC_PARAM* pGraphicParam, bool bClientMode, Echo::uint32 width, Echo::uint32 height)
{
	Echo::uint32 renderMode = GetEngineRenderMode();

	g_init_info.assetPath = _assetPath;
	g_init_info.dataPath = _dataPath;
	g_init_info.pView = pGraphicParam;
	g_init_info.width = width;
	g_init_info.height = height;
	g_init_info.renderMode = renderMode;

	fReinit = false;

	g_mainXml = "main.xml";

#ifdef __ANDROID__
	Echo::MulThreadAndroidJavaEnv::InitMainJavaEnv();
#endif

	g_appview = make_unique_ptr<AsyncApp>(_assetPath, _dataPath, pGraphicParam, renderMode, bClientMode, width, height);
	g_appview->StartUp<GameLogic>(_assetPath, _dataPath, pGraphicParam, renderMode, bClientMode, width, height);
}

void InitAsyncApp(const char* _assetPath, const char* _dataPath, const Echo::ECHO_GRAPHIC_PARAM* pGraphicParam, bool bClientMode, Echo::uint32 width, Echo::uint32 height, Echo::uint32 _left, Echo::uint32 _right, Echo::uint32 _bottom, Echo::uint32 _top)
{
	Echo::uint32 renderMode = GetEngineRenderMode();

	g_init_info.assetPath = _assetPath;
	g_init_info.dataPath = _dataPath;
	g_init_info.pView = pGraphicParam;
	g_init_info.width = width;
	g_init_info.height = height;
	g_init_info.renderMode = renderMode;

	g_init_info.safeAreaLeft = _left;
	g_init_info.safeAreaRight = _right;
	g_init_info.safeAreaBottom = _bottom;
	g_init_info.safeAreaTop = _top;

	fReinit = false;

#ifdef __ANDROID__
	Echo::MulThreadAndroidJavaEnv::InitMainJavaEnv();
#endif
	g_mainXml = "main.xml";

	g_appview = make_unique_ptr<AsyncApp>(_assetPath, _dataPath, pGraphicParam, renderMode, bClientMode, width, height);
	g_appview->StartUp<GameLogic>(_assetPath, _dataPath, pGraphicParam, renderMode, bClientMode, width, height);
}

void SetAsyncAppReinitFlag(const char* mainXml)
{
	if (mainXml && *mainXml)
	{
		g_mainXml = mainXml;
	}
	fReinit = true;
}


unsigned int GetAsyncAppTick()
{
	AsyncApp* _appview = GetAsyncApp();
	if (_appview)
	{
		return _appview->getTick();
	}
	return 0;
}

namespace sandbox
{
	int getShaderCompilationProgress()
	{
		return shaderCompilationProgress;
	}
}


