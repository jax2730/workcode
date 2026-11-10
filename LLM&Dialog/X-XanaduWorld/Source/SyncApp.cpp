
#include "SyncApp.h"
#include "appview.h"

#include "EchoEngineImpl.h"
#include "EchoRenderSystem.h"
#include "EchoWorldManager.h"
#include "EchoDefaultArchive.h"
#include "EchoVideoConfig.h"
#include "EchoConsoleCommand.h"
#include "EchoRobotObjManager.h"
#include "EchoCameraControl.h"
#include "EchoCharacterObj.h"
#include "EchoTriggerAreaManager.h"
#include "EchoGravityAreaManager.h"
#include "EchoBellSystem.h"
#include "EchoComponentSystem.h"
#include "EchoProtocolComponent.h"
#include "EchoRoleController.h"
#include "EchoSphericalTerrainComponent.h"
#include "EchoNPCController.h"
#include "EchoMiniMap.h"

#include <fstream>

#if ECHO_PLATFORM == ECHO_PLATFORM_WIN32
	#ifdef ECHO_EDITOR
#include "EchoEditorRoot.h"
#include "EchoEditorControlSystem.h"
	#endif
#endif
#include "EchoCharactorControllerComponent.h"
#include "EchoFlyingController.h"

#include "EchoWorldSystem.h"

//-----------------------------------------------------------------------------
// Echo Engine
//-----------------------------------------------------------------------------
using namespace Echo;

struct roleInfo
{
	Name roleName;
	vector<Name>::type animList;
};

vector<roleInfo>::type RoleAnimVec;

#define WFC_FPS	60

////**********************************************************
////**********************************************************
////********************** appview ***************************
SyncApp::SyncApp()
{
	std::ifstream load_levels;
	load_levels.open("./load_levels.cfg", std::ios::in | std::ios::app);
	
	bool success_read_levels = false;
	if (load_levels.is_open()) {
		while (load_levels.good()) {
			char level[256]{};
			load_levels.read(level, sizeof(level));
			
			std::string str_level(level);
			if (str_level.empty() == false) {
				mSceneArray.push_back(str_level);
				success_read_levels = true;
			}
		}
		load_levels.close();
	}

	if (success_read_levels == false) {
		mSceneArray.push_back("test");
		mSceneArray.push_back("taikong");
	}
	//mSceneArray.push_back("scene01");
	//mSceneArray.push_back("EchoToday");
	//mSceneArray.push_back("shinei");
}

SyncApp::~SyncApp()
{
}

int SyncApp::init(const char* _assetPath, const char* _dataPath,const Echo::ECHO_GRAPHIC_PARAM* pGraphicParam, bool bClientMode, Echo::uint32 width, Echo::uint32 height)
{
	if (!GetEchoEngine()->getRoot())
	{
		std::string sLogFileName = "Echo.log";

		Echo::IArchive * pArchive = GetArchivePack();

#ifndef _WIN32
		sLogFileName.insert(0, _dataPath);
#endif

		bool bForceUseGxRender = false;

		Root::RuntimeMode mode = Root::eRM_Client;
		if (!bClientMode)
			mode = Root::eRM_Editor;

		GetEchoEngine()->init(_assetPath, pArchive, mode, sLogFileName, bForceUseGxRender, true, true);
		GetEchoEngine()->setShaderPath(_assetPath, _dataPath);
		GetEchoEngine()->setRenderSystem(pGraphicParam, _dataPath, width, height);
		GetEchoEngine()->initConfig();
		
		m_pRoot = Echo::GetEchoEngine()->getRoot();
		SceneManager* mainSceneManager = Echo::Root::instance()->getMainSceneManager();
		m_pCamera = mainSceneManager->getMainCamera();
		m_pWorldMgr = Echo::WorldSystem::instance()->GetMainWorldManager();
		m_pRenderSystem = GetEchoEngine()->getRenderSystem();
		m_pRobotObjManager = new RobotObjManager();
		m_pRobotObjManager->SetSceneManager(mainSceneManager);

		m_pComponentSystem = new ComponentSystem();
		m_pComponentSystem->init(PADBO2COMREGISTERLIST);
#if ECHO_PLATFORM == ECHO_PLATFORM_WIN32
	#ifdef ECHO_EDITOR
		m_pEditorRoot = new Echo::EditorRoot();
		m_pEditorRoot->isQtEditor = m_pComponentSystem->getIsQtInit();
		m_pEditorRoot->RootPostInit();
		Echo::GetEchoEngine()->setEditorRoot(m_pEditorRoot);
	#endif
#endif

		Echo::BellSystem * pBellSystem = Echo::BellSystem::instance();
		if (NULL != pBellSystem)
		{
			if (pBellSystem->initialise("Bank/Master.bank", "Bank/Master.strings.bank"))
				pBellSystem->setListenerCamera(m_pCamera);
		}

		EchoPhysicsSystem::instance()->GetMainManager()->EditorPlay();
	}

	resetMainCamera();

	m_sCurLevelName = mSceneArray[m_curScene];

	if (!loadNewLevel(m_sCurLevelName))
		return false;

	// 帧率设置
	m_wfc.SetFR(WFC_FPS);
	m_wfc.Reset();

	//initRoleList();

	m_isInitialized = true;
#if ECHO_PLATFORM == ECHO_PLATFORM_WIN32
#ifdef ECHO_EDITOR
	GetEchoEngine()->OpenMulScenePrefab("taikong", [this](const Name& worldName, bool bPrefab) 
		{
			GetEchoEngine()->ChangeWorld(worldName);

			auto* pctrl = m_pEditorRoot->GetEditorControlSystem();
			pctrl->setSceneManager(Root::instance()->getSceneManager(worldName));
		});
#endif
#endif
	return true;
}

void SyncApp::onRelease(bool releaseEngine)
{
	if (EchoRoleController::Alive()) {
		delete EchoRoleController::instance();
	}
	if (EchoNPCController::Alive()) {
		delete EchoNPCController::instance();
	}
#if ECHO_PLATFORM == ECHO_PLATFORM_WIN32
#ifdef ECHO_EDITOR
	GetEchoEngine()->CloseMulScenePrefab();
#endif
#endif
	m_isInitialized = false;

	SAFE_DELETE(m_pRobotObjManager);
	m_pWorldMgr->setLoadingState(false);

#if ECHO_PLATFORM == ECHO_PLATFORM_WIN32
	#ifdef ECHO_EDITOR
	m_pEditorRoot->ReleaseRootPost();

	SAFE_DELETE(m_pEditorRoot);
	Echo::GetEchoEngine()->setEditorRoot(nullptr);
	#endif
#endif

	if (releaseEngine)
	{
		GetEchoEngine()->shutdown();
		delete ComponentSystem::instance();
		GetEchoEngine()->deinit();
	}

	delete this;

	SetSynacAppNull();
}

unsigned int SyncApp::getTick()
{
	return m_tick;
}

void SyncApp::setNextLogicCmdInfo(const char* file, int line)
{

}

void SyncApp::setNextEngineCmdInfo(const char* file, int line)
{

}

void SyncApp::postToLogic_(const std::function<void()>& cmd)
{
	cmd();
}

void SyncApp::postToLogic_(std::function<void()>&& cmd)
{
	cmd();
}

void SyncApp::postToEngine_(const std::function<void()>& cmd)
{
	cmd();
}

void SyncApp::postToEngine_(std::function<void()>&& cmd)
{
	cmd();
}

void SyncApp::postToCache_(const std::function<void()>& cmd)
{
	cmd();
}

void SyncApp::postToCache_(std::function<void()>&& cmd)
{
	cmd();
}

void SyncApp::postToMain_(std::function<void()>&& cmd)
{
	cmd();
}

void SyncApp::postToMain_(const std::function<void()>& cmd)
{
	cmd();
}

void SyncApp::resetMainCamera()
{
	//m_pCamera->setPosition(-15057.1885f, 20.0f, -13519.1182f);
	//m_pCamera->setOrientation(Quaternion(0.218596041f, 0.0419984907f, 0.957402110f, -0.183945835f));
	m_pCamera->setPosition(0.0f, 100.0f, 0.0f);
	m_pCamera->setOrientation(Quaternion(-0.0451708846f, 0.49380821f, -0.0257615524f, -0.868126213f));
	m_pCamera->setFarClipDistance(10000.0f);
	m_pCamera->setNearClipDistance(0.1f);
	m_pCamera->setCullingFactor(0.003f);
	m_pCamera->setFOVy(Radian(Degree(50)));
}

bool SyncApp::loadNewLevel(const std::string & szLevelName)
{
	//if (m_pWorldMgr->isLevelLoaded())
	//{
	//	m_pWorldMgr->unloadLevel();
	//}

	//bool b = m_pWorldMgr->loadLevel(szLevelName);
	//m_pWorldMgr->setSceneSeamlessEnable(false);
	bool b = Echo::GetEchoEngine()->loadNewLevel(szLevelName);
	return b;
}

void SyncApp::createRoleTest(Vector3 pos, int num)
{
	pos.y = 0.0f;
	Vector3 newPos;
	for (int i = 0; i < num; ++i)
	{
		float xoffset = Math::RangeRandom(-20.0f, 30.0f);
		float zoffset = Math::RangeRandom(-30.0f, 30.0f);
		newPos = pos + Vector3(xoffset, 0.0f, zoffset);

		Vector3 dir = newPos - pos;
		dir.normalise();

		float height = m_pWorldMgr->getHeightAtWorldPosition(newPos);
		newPos.y = height;

		int roleIndex = Math::RangeRandom(0, (int)RoleAnimVec.size() - 1);
		int animindex = Math::RangeRandom(0, (int)RoleAnimVec[roleIndex].animList.size() - 1);

		RoleObject* prole = new RoleObject(RoleAnimVec[roleIndex].roleName, RoleAnimVec[roleIndex].roleName);//  m_pWorldMgr->getSceneManager()->createRoleObject(RoleAnimVec[roleIndex].roleName);
		prole->getAnimationComponent()->playAnimation(RoleAnimVec[roleIndex].animList[animindex], true, ANI_TYPE_FULL);

		prole->setPosition(newPos);
		//SceneNode* pNode = m_pWorldMgr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
		//pNode->setPosition(newPos);

		//prole->setParentNode(pNode);

		Quaternion qua;
		qua.FromAxesToAxes(prole->getDirection(), dir);
		prole->setOrientation(qua);
	}
}

void SyncApp::createParticleTest(Vector3 pos)
{
	//pos.y = 0.0f;
	//Vector3 newPos;
	//Particle* p = m_pWorldMgr->getSceneManager()->createParticle((Name)"gfx/mgx/tx_htslizi_001.mgx", true);
	//SceneNode* node = m_pWorldMgr->getSceneManager()->createSceneNode();
	//node->attachObject((p));
	//node->setPosition(m_pCamera->getPosition());
	//node->_update();
}

void SyncApp::initRoleList()
{
	roleInfo info;
	info.roleName = Name("role/dw_029_yang/dw_029_yang.role");
	info.animList.push_back(Name("xx01"));
	info.animList.push_back(Name("xdz01"));

	RoleAnimVec.push_back(info);


	info.animList.clear();
	info.roleName = Name("role/dw_003_ma/dw_003_ma.role");
	info.animList.push_back(Name("sj_pbty_01"));
	info.animList.push_back(Name("sj_pbty_02"));
	info.animList.push_back(Name("sj_pbty_03"));
	info.animList.push_back(Name("sj_pbzxz_01"));
	info.animList.push_back(Name("zc_pb_01"));
	info.animList.push_back(Name("zc_pbt_01"));
	info.animList.push_back(Name("zc_xx_01"));
	RoleAnimVec.push_back(info);


	info.animList.clear();
	info.roleName = Name("role/zj_001_shaoxia/zj_001_shaoxia.role");
	info.animList.push_back(Name("ck_pb_01_ca"));
	info.animList.push_back(Name("ck_xx_01_ca"));
	info.animList.push_back(Name("sj_pbty_03_ca"));
	info.animList.push_back(Name("zc_dm_pb_yq_01_ca"));
	info.animList.push_back(Name("zc_dm_pb_zq_01_tk"));
	info.animList.push_back(Name("zc_dm_ty_08_tk"));
	info.animList.push_back(Name("zc_dm_ty_11_tk"));
	info.animList.push_back(Name("zc_dm_ty_xl_02"));
	info.animList.push_back(Name("zc_dm_ty_xl_04_tk"));
	info.animList.push_back(Name("zc_sm_yd_s_qd_01_ca"));
	RoleAnimVec.push_back(info);

	info.animList.clear();
	info.roleName = Name("role/zj_001_naihai/zj_001_naihai.role");
	info.animList.push_back(Name("pbz02"));
	info.animList.push_back(Name("szxx01"));
	info.animList.push_back(Name("ty03b"));
	info.animList.push_back(Name("xx01"));
	info.animList.push_back(Name("ysxx_01"));
	info.animList.push_back(Name("ty02a"));
	info.animList.push_back(Name("fx01"));
	info.animList.push_back(Name("kzyd01"));
	info.animList.push_back(Name("ysld_01"));
	RoleAnimVec.push_back(info);
}

bool SyncApp::isHasImGuiWindowHovered() const {
#if ECHO_PLATFORM == ECHO_PLATFORM_WIN32
	#ifdef ECHO_EDITOR
	return EditorRoot::instance()->HasImGuiWindowHovered();
	#else
	return false;
	#endif
#else
	return false;
#endif
}

struct InitInfo_t_
{
	std::string assetPath;
	std::string dataPath;
	const Echo::ECHO_GRAPHIC_PARAM* pView;
	bool  bClientMode;
	Echo::uint32 width;
	Echo::uint32 height;
	Echo::uint32 safeAreaLeft, safeAreaTop, safeAreaRight, safeAreaBottom;
};
static InitInfo_t_ initInfo;

void SyncApp::onResize(Echo::uint32 w, Echo::uint32 h, Echo::uint32 safeAreaLeft, Echo::uint32 safeAreaTop, Echo::uint32 safeAreaRight, Echo::uint32 safeAreaBottom)
{
	m_width = w;
	m_height = h;
	if (w != 0 || h != 0)
	{
		initInfo.width = w;
		initInfo.height = h;
		initInfo.safeAreaLeft = safeAreaLeft;
		initInfo.safeAreaTop = safeAreaTop;
		initInfo.safeAreaRight = safeAreaRight;
		initInfo.safeAreaBottom = safeAreaBottom;
	}

	m_pRenderSystem->resizeWindow(w, h);
}

// tick
void SyncApp::onTick()
{
	//更新触发器对象
	PlayMode mode = CameraControl::instance()->GetPlayMode();

	Vector3 pos;
	if (mode == PlayMode::MODE_FREE)
	{
		pos = m_pCamera->getDerivedPosition();
	}
	else if (mode == PlayMode::MODE_PLAY)
	{
		CharacterObj* pcharobj = CameraControl::instance()->getCharObj();
		if (pcharobj)
		{
			RoleObject* prole = pcharobj->getRoleObj();
			if (prole)
			{
				pos = prole->getPosition();
			}
		}
	}
	TriggerAreaManager::instance()->update(pos, m_pCamera->getDerivedOrientation(), m_pCamera->getDerivedPosition());
	GravityAreaManager::instance()->update(pos);

	GetEchoEngine()->getWorldManager()->getProtocolComponentManager()->tick(TT_PreClientUpdate);
	if(m_pWorldMgr)
		m_pWorldMgr->updateRolePos(pos);

	GetEchoEngine()->preRenderOneFrame();
	if (EchoRoleController::Alive()) {
		EchoRoleController::instance()->tick();
	}
	if (EchoNPCController::Alive()) {
		EchoNPCController::instance()->tick();
	}
	GetEchoEngine()->renderOneFrame();
	//m_pRoot->renderOneFrame(/*current_time - pre_time*/);
	GetEchoEngine()->postRenderOneFrame();

	m_wfc.Tick();
	m_pRobotObjManager->Tick(m_pCamera->getPosition());

	ProtocolComponentSystem::instance()->callProtocolFunc(Name("ChangeScene"), nullptr, 0);
}

void SyncApp::setFPS(float _fps)
{
	m_wfc.SetFR(_fps);
	m_wfc.Reset();
}


////**********************************************************
void SyncApp::onResume()
{	
	if (m_isInitialized)
	{
		GetEchoEngine()->resumeRendering();
	}
}

void SyncApp::onStart()
{
	if (m_isInitialized)
	{
#if defined(__ANDROID__) || defined(__APPLE__)
		if (Echo::BellSystem::instance())
			Echo::BellSystem::instance()->resume();
#endif
	}
}

void SyncApp::onPause()
{	
	if (m_isInitialized)
	{
		GetEchoEngine()->stopRendering();
	}
}

void SyncApp::onStop()
{
	if (m_isInitialized)
	{
#if defined(__ANDROID__) || defined(__APPLE__)
		if (Echo::BellSystem::instance())
			Echo::BellSystem::instance()->suspend();
#endif
	}
}

void SyncApp::onResetWindow(void* handle)
{
	GetEchoEngine()->getRoot()->onRecreateSurface(handle);
}

void SyncApp::commandParse(const std::string& command)
{
	Echo::ConsoleCommand *pConsole = Echo::ConsoleCommand::instance();
	if (!pConsole)
	{
		return;
	}

	pConsole->executeCommand(command);

	const Echo::CmdLineArg * rtest = pConsole->findArg("rtest", eCLAT_Normal, true);
	if (rtest != nullptr)
	{
		const uint32 id = rtest->getUIValue();
		if (id == 0xFFFFFFFF)
			m_pWorldMgr->stopPlayingCameraPath();
		else
			m_pWorldMgr->startPlayingCameraPath(id, 3);
		return;
	}

	const Echo::CmdLineArg * fpsArg = pConsole->findArg("fps", eCLAT_Normal, true);
	if (fpsArg)
	{
		m_wfc.SetFR(fpsArg->getFValue());
		return;
	}

	const Echo::CmdLineArg* createRoleArg = pConsole->findArg("createrole", eCLAT_Normal, true);
	if (createRoleArg)
	{
		int roleNum = createRoleArg->getIValue();
		if (roleNum > 0)
		{
			Vector3 campos = m_pCamera->getPosition();
			Vector3 dir = m_pCamera->getDirection();
			dir.normalise();

			//在相机前面20米的位置随机创建角色
			Vector3 newPos = campos + dir * 20;

			createRoleTest(newPos, roleNum);
		}
	}

	const Echo::CmdLineArg* createCCTArg = pConsole->findArg("testcct", eCLAT_Normal, true);
	if (createCCTArg)
	{
		int roleNum = createCCTArg->getIValue();
		if(roleNum > 0)
		{
			if (!Echo::EchoFlyingController::isCreated && m_pCamera != nullptr)
				new Echo::EchoFlyingController(m_pCamera);
			else if (Echo::EchoFlyingController::isCreated)
				Echo::EchoFlyingController::instance()->createFlyCCT(true);
		}
		else
		{
			if (Echo::EchoFlyingController::isCreated)
				delete Echo::EchoFlyingController::instance();
		}
	}

	const Echo::CmdLineArg* createNPCArg = pConsole->findArg("npc", eCLAT_Normal, true);
	if (createNPCArg)
	{
		if (EchoNPCController::Alive()) {
			delete EchoNPCController::instance();
		}
		else {
			new EchoNPCController(m_pCamera);
		}
	}

	EchoRoleController::ExecuteCommand(pConsole);

	const Echo::CmdLineArg* createRobotArg = pConsole->findArg("cr", eCLAT_Normal, true);
	if (createRobotArg)
	{
		int roleNum = createRobotArg->getIValue();
		if (roleNum > 0)
		{
			Vector3 campos = m_pCamera->getPosition();
			for (int i = 0; i < roleNum; i++)
			{
				float distancex = Echo::Math::RangeRandom(-50.0f, 50.0f);
				float distancez = Echo::Math::RangeRandom(-50.0f, 50.0f);
				m_pRobotObjManager->CreateRobotObject(campos + Echo::Vector3(distancex, 0.0f, distancez));
			}
		}
	}

	const Echo::CmdLineArg* SetMoveArg = pConsole->findArg("sm", eCLAT_Normal, true);
	if (SetMoveArg)
	{
		int setMove = SetMoveArg->getIValue();
		if (setMove > 0)
		{
			m_pRobotObjManager->SetMove(true);
		}
		else
		{
			m_pRobotObjManager->SetMove(false);
		}
	}

	const Echo::CmdLineArg* playAnimationArg = pConsole->findArg("pa", eCLAT_Normal, true);
	if (playAnimationArg)
	{
		int playAnimation = playAnimationArg->getIValue();
		if (playAnimation > 0)
		{
			m_pRobotObjManager->SetPlayAnimation(true);
		}
		else
		{
			m_pRobotObjManager->SetPlayAnimation(false);
		}
	}

	const Echo::CmdLineArg* ppArg = pConsole->findArg("pp", eCLAT_Normal, true);
	if (ppArg)
	{
		int playAnimation = ppArg->getIValue();
		if (playAnimation > 0)
		{
			m_pRobotObjManager->SetPlayParticle(true);
		}
		else
		{
			m_pRobotObjManager->SetPlayParticle(false);
		}
	}

	const Echo::CmdLineArg* deleteAllRoleArg = pConsole->findArg("dr", eCLAT_Normal, true);
	if (deleteAllRoleArg)
	{
		m_pRobotObjManager->ClearAllRobotObject();
	}

	const Echo::CmdLineArg* createParticleArg = pConsole->findArg("createparticle", eCLAT_Normal, true);
	if (createParticleArg)
	{
		Vector3 campos = m_pCamera->getPosition();
		Vector3 dir = m_pCamera->getDirection();
		dir.normalise();

		Vector3 newPos = campos + dir * 10;
		createParticleTest(newPos);
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
				GetEchoEngine()->changeLevel(worldName,"testMul");
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
		GetEchoEngine()->RemoveWorld(worldName);	
	}

}

void SyncApp::onMouseMove(const Echo::Vector2& point)
{
	auto protocolMgr = ProtocolComponentSystem::instance();
	struct MouseInfo
	{
		float x, y;
	};
	MouseInfo mouse = { point.x, point.y };
	protocolMgr->callProtocolFunc(Echo::Name("fguiOnMouseLeftMove"), &mouse, 0);
}

void SyncApp::onMouseLeftDown(const Echo::Vector2& point)
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

void SyncApp::onMouseLeftUp(const Echo::Vector2& point)
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

void SyncApp::getWindowSize(Echo::uint32& width, Echo::uint32& height)
{
	width = m_width;
	height = m_height;
}

void SyncApp::cameraMove(int type, int val)
{
	if (m_isInitialized)
	{
		if (EchoRoleController::Alive()) {
			EchoRoleController::instance()->update(0, (float)type, (float)val);
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
#define CAMERA_TURN_SCALE 0.1f
#endif

void SyncApp::cameraTurn(float x, float y)
{
	if (m_isInitialized)
	{
		if (EchoRoleController::Alive()) {
			EchoRoleController::instance()->update(1, x, y);
			return;
		}
		if (EchoNPCController::Alive()) {
			EchoNPCController::instance()->update(0, x, y);
			return;
		}
		if (!Echo::EchoFlyingController::isCreated)
			CameraControl::instance()->cameraTurn(x * CAMERA_TURN_SCALE, y * CAMERA_TURN_SCALE);
		else
			Echo::EchoFlyingController::instance()->updateFlyCCT(1, x * CAMERA_TURN_SCALE, y * CAMERA_TURN_SCALE);
	}
}

void SyncApp::cameraDisChg(int val)
{
	if (m_isInitialized) {
		if (EchoRoleController::Alive()) {
			EchoRoleController::instance()->update(2, 0, (float)val);
			return;
		}
		if (EchoNPCController::Alive()) {
			EchoNPCController::instance()->update(2, 0, (float)val);
			return;
		}
		CameraControl::instance()->cameraDisChg(val);
	}
}

void SyncApp::setMoveDir(float x, float y)
{
	if (m_isInitialized) {
		if (EchoRoleController::Alive()) {
			EchoRoleController::instance()->update(0, 0.0f, x);
			EchoRoleController::instance()->update(0, 1.0f, y);
			return;
		}
		if (EchoNPCController::Alive()) {
			EchoNPCController::instance()->update(0, 0.0f, x);
			EchoNPCController::instance()->update(0, 1.0f, y);
			return;
		}
		CameraControl::instance()->setMoveDir(x, y);
	}
}

void SyncApp::setTurnDir(float x, float y)
{
	if (m_isInitialized) {
		if (EchoRoleController::Alive()) {
			EchoRoleController::instance()->update(1, x, y);
			return;
		}
		if (EchoNPCController::Alive()) {
			EchoNPCController::instance()->update(1, x, y);
			return;
		}
		CameraControl::instance()->setTurnDir(x, y);
	}
}

void SyncApp::incCameraSpeed()
{
	if (!m_isInitialized)
		return;

	float fSpeed = CameraControl::instance()->getMoveSpeed();
	fSpeed *= 1.1f;
	if (fSpeed < 0.01f)
		fSpeed = 0.01f;
	CameraControl::instance()->setMoveSpeed(fSpeed);
}

void SyncApp::decCameraSpeed()
{
	if (!m_isInitialized)
		return;

	float fSpeed = CameraControl::instance()->getMoveSpeed();
	fSpeed /= 1.1f;
	if (fSpeed < 0.01f)
		fSpeed = 0.01f;
	CameraControl::instance()->setMoveSpeed(fSpeed);
}

void SyncApp::attack()
{
	if (m_isInitialized)
		CameraControl::instance()->attack();
}

void SyncApp::switchPlayMode()
{
	if (m_isInitialized)
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

void SyncApp::createRobotObject()
{
	if (m_isInitialized)
	{
		Echo::Vector3 posi = m_pCamera->getPosition();
		RobotObjManager::instance()->CreateRobotObject(posi);
	}
}

int SyncApp::getFps()
{
	if (m_pRoot)
		return m_pRoot->GetFps();

	return 0;
}

int SyncApp::getDrawCall()
{
	if (m_pRoot)
		return m_pRoot->getDrawCount();

	return 0;
}

int SyncApp::getTriangleCnt()
{
	if (m_pRoot)
		return m_pRoot->getTriangleCount();

	return 0;
}

std::string  SyncApp::getCameraPosition()
{
	static std::string result = "<null>";
	if (m_pCamera)
	{
		const Echo::Vector3 & pos = m_pCamera->getPosition();
		char szBuf[64] = { '\0' };
		snprintf(szBuf, sizeof(szBuf) - 1, "%.2f %.2f %.2f",
			pos.x, pos.y, pos.z);
		result = szBuf;
	}
	return result;
}

std::string  SyncApp::getCameraOrientation()
{
	static std::string result = "<null>";
	if (m_pCamera) 
	{
		const Echo::Quaternion & ori = m_pCamera->getOrientation();
		char szBuf[64] = { '\0' };
		snprintf(szBuf, sizeof(szBuf) - 1, "%f %f %f %f",
			ori.w, ori.x, ori.y, ori.z);
		result = szBuf;
	}
	return result;
}

std::string SyncApp::getResMemUsageStr()
{
	if (m_pWorldMgr != nullptr && m_pWorldMgr->isDrawResMemUsage())
	{
		return m_pWorldMgr->getResMemUsage(m_pWorldMgr->isDrawResMemUsageDetail());
	}
	return std::string();
}

void SyncApp::create3DUIGunTest()
{

}

void SyncApp::loadSenceByName(const std::string& name) {
	if (m_isInitialized) {
		Echo::GetEchoEngine()->loadNewLevel(name);
	}
}

std::string SyncApp::getGameData()
{
	std::string gameData;
	auto sanitize = [](const std::string& source) {
		std::string result;
		result.reserve(source.size());
		for (char ch : source)
		{
			switch (ch)
			{
			case ';': result.push_back(' '); break;
			case '=': result.push_back(':'); break;
			case '|': result.push_back('/'); break;
			case '\n':
			case '\r':
				result.push_back(' ');
				break;
			default:
				result.push_back(ch);
				break;
			}
		}
		return result;
	};

	if (m_pCamera)
	{
		const Echo::DVector3& pos = m_pCamera->getDerivedPosition();
		char xBuffer[64] = { '\0' };
		char yBuffer[64] = { '\0' };
		snprintf(xBuffer, sizeof(xBuffer) - 1, "%.6f", pos.x);
		snprintf(yBuffer, sizeof(yBuffer) - 1, "%.6f", pos.z);
		gameData += "||x=";
		gameData += xBuffer;
		gameData += ";y=";
		gameData += yBuffer;
	}

	std::string weatherName;
	std::string weatherDesc;
	std::string weatherState;
	std::string weatherCity;
	std::string weatherCounty;
	double temperature = 0.0;
	if (Echo::GetLatestWeatherSnapshot(weatherName, weatherDesc, temperature, weatherState, weatherCity, weatherCounty))
	{
		char temperatureBuffer[64] = { '\0' };
		snprintf(temperatureBuffer, sizeof(temperatureBuffer) - 1, "%.2f", temperature);
		if (gameData.find("||") == std::string::npos)
		{
			gameData += "||";
		}
		else
		{
			gameData += ";";
		}
		gameData += "weather_name=";
		gameData += sanitize(weatherName);
		gameData += ";weather_desc=";
		gameData += sanitize(weatherDesc);
		if (!weatherState.empty())
		{
			gameData += ";state=";
			gameData += sanitize(weatherState);
		}
		if (!weatherCity.empty())
		{
			gameData += ";city=";
			gameData += sanitize(weatherCity);
		}
		if (!weatherCounty.empty())
		{
			gameData += ";county=";
			gameData += sanitize(weatherCounty);
		}
		gameData += ";temperature=";
		gameData += temperatureBuffer;
	}

	return gameData;
}

//#if ECHO_PLATFORM == ECHO_PLATFORM_WIN32
#ifdef ECHO_EDITOR
void SyncApp::OnKeyDown(unsigned int key)
{
	EditorRoot::instance()->_OnKeyDown(key);
}

void SyncApp::OnKeyUp(unsigned int key)
{
	EditorRoot::instance()->_OnKeyUp(key);
}

void SyncApp::OnMouseMove(Echo::Vector2 point, unsigned int buttonsFlag)
{
	EditorRoot::instance()->_OnMouseMove(point, buttonsFlag);
}

void SyncApp::OnMouseLeftDown(Echo::Vector2 point)
{
	EditorRoot::instance()->_OnMouseLeftDown(point);
}

void SyncApp::OnMouseLeftUp(Echo::Vector2 point)
{
	EditorRoot::instance()->_OnMouseLeftUp(point);
}

void SyncApp::OnMouseRightDown(Echo::Vector2 point)
{
	EditorRoot::instance()->_OnMouseRightDown(point);
}

void SyncApp::OnMouseRightUp(Echo::Vector2 point)
{
	EditorRoot::instance()->_OnMouseRightUp(point);
}

void SyncApp::OnKillFocus() {
	EditorRoot::instance()->_OnKillFocus();
}
void SyncApp::OnInputText(char16_t c)
{

}
#else

void SyncApp::OnKeyDown(unsigned int key)
{
}

void SyncApp::OnKeyUp(unsigned int key)
{
}

void SyncApp::OnMouseMove(Echo::Vector2 point, unsigned int buttonsFlag)
{
}


void SyncApp::OnMouseLeftDown(Echo::Vector2 point)
{
}

void SyncApp::OnMouseLeftUp(Echo::Vector2 point)
{
}

void SyncApp::OnMouseRightDown(Echo::Vector2 point)
{

}

void SyncApp::OnMouseRightUp(Echo::Vector2 point)
{

}

void SyncApp::OnKillFocus() {
	
}

void SyncApp::OnInputText(char16_t c)
{

}

#endif

////**********************************************************

SyncApp * g_appview = NULL;
SyncApp * GetSyncApp()
{
	return g_appview;
}

void SetSynacAppNull()
{
	g_appview = nullptr;
}

void InitSyncApp(const char* _assetPath, const char* _dataPath, const Echo::ECHO_GRAPHIC_PARAM* pGraphicParam, bool bClientMode, Echo::uint32 width, Echo::uint32 height)
{
	if (g_appview)
	{
		return;
	}

	initInfo.assetPath = _assetPath;
	initInfo.dataPath = _dataPath;
	initInfo.pView = pGraphicParam;
	initInfo.width = width;
	initInfo.height = height;

	g_appview = new SyncApp;

	g_appview->init(_assetPath,_dataPath, pGraphicParam, bClientMode, width, height);
}

void InitSyncApp(const char* _assetPath, const char* _dataPath, const Echo::ECHO_GRAPHIC_PARAM* pGraphicParam, bool bClientMode, Echo::uint32 width, Echo::uint32 height, Echo::uint32 _left, Echo::uint32 _right, Echo::uint32 _bottom, Echo::uint32 _top)
{
	if (g_appview)
	{
		return;
	}

	initInfo.assetPath = _assetPath;
	initInfo.dataPath = _dataPath;
	initInfo.pView = pGraphicParam;
	initInfo.bClientMode = bClientMode;
	initInfo.width = width;
	initInfo.height = height;

	initInfo.safeAreaLeft = _left;
	initInfo.safeAreaRight = _right;
	initInfo.safeAreaBottom = _bottom;
	initInfo.safeAreaTop = _top;

	g_appview = new SyncApp;

	g_appview->init(_assetPath, _dataPath, pGraphicParam, bClientMode, width, height);
}

void ReInitSyncApp()
{
	if (g_appview)
	{
		g_appview->onRelease();
		g_appview = NULL;
		init_appview(false, initInfo.bClientMode, initInfo.assetPath.c_str(), initInfo.dataPath.c_str(), initInfo.pView, initInfo.width, initInfo.height);
		g_appview->onResize((int)initInfo.width, (int)initInfo.height, (int)initInfo.safeAreaLeft, (int)initInfo.safeAreaTop, (int)initInfo.safeAreaRight, (int)initInfo.safeAreaBottom);
	}
}
