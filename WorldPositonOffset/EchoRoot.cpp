
#include "EchoStableHeaders.h"
#include "EchoRoot.h"
#include "EchoVersion.h"
#include "EchoIArchive.h"
#include "EchoRenderSystem.h"
#include "EchoException.h"
#include "EchoControllerManager.h"
#include "EchoLogManager.h"
#include "EchoMath.h"
#include "EchoTextureHelper.h"
#include "EchoTextureManager.h"
#include "EchoStringConverter.h"
#include "EchoResourceBackgroundQueue.h"
#include "EchoMaterialBuildHelper.h"
#include "EchoLight.h"
#include "EchoWindowEventUtilities.h"
#include "EchoResourceWorkQueue.h"
#include "EchoResourceManagerFactory.h"
#include "EchoManualObject.h"
#include "EchoGpuParamsManager.h"
#include "EchoSceneCommonResourceManager.h"
#include "EchoMeshMgr.h"
#include "EchoRoadResourceManager.h"
#include "EchoWaterResourceManager.h"
#include "EchoWaterGlobalMaterialHelper.h"
#include "EchoBiomeVegGroup.h"
#include "EchoVegetationResourceManager.h"
#include "EchoVegetationAreaMeshResource.h"
#include "EchoVegetationAreaResourceManager.h"
#include "EchoVegetationLayer.h"
#include "EchoBiomeVegOpt.h"
#include "EchoSkeletonManager.h"
#include "EchoSoftbodyManager.h"
#include "EchoCustomSkeletonManager.h"
#include "EchoBatchManager.h"
#include "EchoTODManager.h"
#include "EchoWeatherManager.h"
#include "EchoTimer.h"
#include "EchoRetargetingSkeletonManager.h"

#include "EchoAnimationManager.h"
#include "Effect/EchoEffectResourceManager.h"
#include "Effect/EchoEffectManager.h"
#include "EchoShaderContentManager.h"
#include "EchoShaderMan.h"
#include "Effect/EchoEffectManager.h"
#include "EchoTerrainSystem.h"
#include "EchoTerrainUtil.h"
#include "EchoKarstSystem.h"
#include "EchoLandformResourceManager.h"
#include "EchoRoleManager.h"
#include "EchoRenderStrategy.h"
#include "EchoComponentManager.h"
//#include "Actor/EchoActorManager.h"
//#include "Actor/EchoActorInfoManager.h"
#include "EchoSystemConfigFile.h"
#include "EchoShareRoleConfig.h"
#include "EchoRenderQueue.h"
#include "EchoPlanetRegionResource.h"
#include "EchoRenderer.h"

#if ECHO_NO_FREEIMAGE == 0
#include "EchoFreeImageCodec.h"
#endif
#if ECHO_NO_DDS_CODEC == 0
#include "EchoDDSCodec.h"
#endif
#include "EchoImage.h"
#include "EchoStreamResourceManager.h"
#include "EchoMaterialAnimManager.h"
#include "EchoViewport.h"
#include "EchoLookAtCfg.h"
#include "EchoSurfaceType.h"
//#include "Effect/EchoPtFactoryManager.h"
#include "Effect/EchoEffectFactoryManager.h"
#include "JobSystem/EchoJobSystem.h"
#include "EchoSkeletonSyncManager.h"
#include "EchoRoleAnimationLoadManager.h"
//#include "Actor/EchoActorInfoLoadManager.h"
#include "EchoBuildLodMeshManager.h"
#include "EchoRoleMeshLoadManager.h"
#ifdef __APPLE__
#else
#include "EchoShaderClientManager.h"
#endif

#include "EchoMaterialManager.h"
#include "EchoMaterialV2Manager.h"
#include "EchoPassManager.h"
#include "EchoOcclusionPlaneManager.h"
#include "EchoMountainManager.h"
#include "EchoBuildingLodManager.h"
#include "EchoSceneManager.h"
#include "EchoMaterialCompileManager.h"
#include "EchoDynamicVideoParamManager.h"
#include "EchoMaterialObjectManager.h"
#include "EchoMediaManager.h"
#include "EchoRoleBatchManager.h"
#include "EchoPetBatchMaterialManager.h"
#include "EchoObjectPool.h"
#include "EchoDataStreamManager.h"
#include "EchoIOCP.h"
#include "EchoOceanManager.h"
#include "EchoPlatformInformation.h"
#include "EchoMaterialShaderPassManager.h"
#ifndef _WIN32
#include <sys/time.h>
#endif

//TODO(yanghang): 3D UI Font Glyph header
#include "EchoGlyphManager.h"
#include "EchoFreeTypeResourceManager.h"

#include "EchoEntityBatchPageResManager.h"
#include "EchoProtocolComponent.h"

#include "EchoProfilerManager.h"
#include "EchoAtlasInfoManager.h"

#define SleepTimeCalFreq 30

//NOTE(yanghang): Cluster forward light
#include "EchoClusteForwardLighting.h"
//NOTE(yanghang): Utility debug rendering
#include "EchoDebugDrawManager.h"
//NOTE: 3D UI low-level system.
#include "EchoUIRenderSystem.h"
//TODO(yanghang):Remove the deprecate code.
#include "EchoScreen3DUIEntityManager.h"
#include "EchoUI2DResourceManager.h"

//NOTE:Shadow map parameter calculate stuff.
#include "EchoShadowmapManager.h"
#include "EchoRainMaskManager.h"
#include "EchoEntity.h"

//#include "Actor/EchoResLoadManager.h"
//#include "Actor/EchoActorComFactoryManager.h"
#include "EchoSheetManager.h"
#include "EchoResourceFolderManager.h"
#include "EchoSqliteMgr.h"
#include "EchoThreadPool.h"

#include "EchoEnvironmentLight.h"

//#include "Actor/EchoActorLayerPageManager.h"
#include "EchoGpuDriven.h"
#include "EchoGpuBatchManager.h"
#include "EchoReflectCoeResource.h"
#include "EchoGpuPoolManagerImpl.h"

#include "FGUIParser/FairyGuiWrapper.h"

#include "EchoBufferManager.h"
#include "EchoPointLightShadowManager.h"
#include "EchoRoleCollisionManager.h"
#include "EchoRoleLocatorManager.h"
#include "EchoBlendMaskManager.h"
#include "EchoTextureStreamingHelper.h"
#include "Effect/EchoEffectSystem.h"
#ifdef _WIN32
#include <filesystem>
#include <corecrt_io.h>
#include "Win32/EchoHotUpdateAssemblies.h"
#endif
#ifdef ECHO_EDITOR
#include "EchoAIGCManager.h"
#endif
#include "EchoProfilerInfoManager.h"
#include "EchoResourceDataMemoryManager.h"

#include "JobSystem/EchoCommonJobManager.h"
#include "EchoEasyAsynHandler.h"
#include "EchoHWMediaPlayer.h"
#include "Profiler/StatusGather/EchoCoreStatusData.h"

#include "EchoSphericalTerrainAtmosphere.h"
#include "Cloth/EchoClothAssetResourceManager.h"
#include "EchoSDFResManagerFactory.h"
#include "EchoDataAcqusition.h"
#include "EchoOAESystem.h"
#include "EchoPlaneVegetation.h"

#include "EchoWorldOriginListener.h"
#include "DeformSnow.h"
#include "EchoSphericalOcean.h"
#include "EchoPlanetRoadResource.h"
#include "EchoBiomeGenObjectLibrary.h"
#include "VoxelTerrain/EchoVoxelTerrainFractal.h"
#include "EchoStringTable.h"

namespace Echo {

	static const Name g_MainSceneManagerName = Name("Main");

	static const Name g_UISceneManagerName = Name("UI SceneManager");

	int Root::g_lodLevel = -1;

#define DEFAULT_FORCE_FIRST_PERSON_NEAR_CLIP_DISTANCE 0.001f
#define DEFAULT_FORCE_SCENE_NEAR_CLIP_DISTANCE 0.1f
	//-----------------------------------------------------------------------
	Root::Root(const String& rootResPath, IArchive* pArchive, RuntimeMode mode /*= eRM_Client*/, const String& logFileName /*= "Echo.log"*/,
		bool bForceUseGxRender /*= false*/, bool bForceUseRcSingleThread/* = false*/)
		: mRootResourcePath(rootResPath)
		, mArchive(pArchive)
		, mRuntimeMode(mode)
		, m_DeviceType(ECHO_DEVICE_NULL)
		, m_RenderTextureType(ECHO_TEX_TYPE_ASTC)
		, m_bInitialise(false)
		, m_pRenderSystem(nullptr)
		, m_pRenderer(nullptr)
		, m_context(nullptr)
		, m_threadMode(ECHO_THREAD_ST)
		, m_bUseRenderCommand(false)
		, m_bForceUseGxRender(bForceUseGxRender)
		, m_bForceUseRcSingleThread(bForceUseRcSingleThread)
		, m_bPreRenderOneFrame(false)
		, m_bPreSyncSkeleton(false)
		, m_checkFloatExceptionMask(0)
		, m_pResourceManagerFactory(nullptr)
		, mLogManager(0)
		, mActiveSceneMgr(nullptr)
		, mNextFrame(0)
		, mFrameSmoothingTime(0.0f)
		, mTimeElapsed(0.0)
		, mTimeSinceLastFrame(0.0f)
		, mTimeSinceLastFrameClient(0.0f)
		, mFps(0)
		, mFramCount(0)
		, mFrameStartedTime(0)
		, mFrameEndTime(0)
		, mSceneUpdateTime(0)
		, mWaitRenderTime(0)
		, mWaitUpdateTime(0)
		, mSwapBufferTime(0)
		, mRenderMainTime(0)
		, mExpectFps(30)
		, mAvgSceneUpdateTime(0)
		, mAvgRenderMainTime(0)
		, mAvgTotalRenderTime(0)
		, mAvgWaitRenderTime(0)
		, mAvgWaitUpdateTime(0)
		, mAvgSwapBufferTime(0)
		, m_triangleCount(0)
		, m_drawCallCount(0)
		, m_batchDrawCallCount(0)
		, m_particleDrawCallCount(0)
		, m_particleRoleCount(0)
		, m_HlodTextureSizeMaximum(2048)
		, m_objectTriangleCount(0)
		, m_shadowTriangleCount(0)
		, m_UITriangleCount(0)
		, m_renderGroupMask(0xffffffff)
		, m_bShowLodColor(false)
		, m_bShowSelectColor(false)
		, m_renderStrategyType(0)
		, m_bUseGeneralRenderStrategy(false)
		, m_bEnableMatV2CompatibilityMode(false)
		, m_bEnableVisibilityBuffer(false)
		, m_bEnableShaderDebugFlag(false)
		, m_UseInstanceTec(false)
		, m_UseRoleInstanceTec(true)
		, m_UseTextureDensity(false)
		, m_bCanCompileShaderInResourceThread(true)
		, m_bResourceBaseDestroyByFrame(true)
		//, mRaycastReturnNormalFun(NULL)
		, mOutEventMemInfoFun(NULL)
		, useOverDrawFlag(0)
		, overDrawValue(0.0f)
		, mIsParticleTickAsync(true)
		, m_bUseSpringSoft(false)
		, m_bEnablePetBatch(false)
		, m_bSoundProfileEnabled(false)
		, m_bTMagicProfileEnabled(false)
		, m_bTMagicEventAsynchronously(true)
		, m_bSoundSysResCacheCount(50)
		, m_bReleaseBuildingFreeEntity(true)
		, m_bCreateLowestLodFreeEntity(true)
		, m_bMetalShaderCacheEnable(true)
		, m_bD3DXShaderCacheEnable(false)
		, m_bMultiCameraRendering(false)
		, m_dwReleaseEntityCnt(100)
		, m_bRebuildSkeleton(false)
		, m_bReleaseMeshData(true)
		, m_bFFTOcean(false)
		, m_bParticleAtlasHashVerify(false)
		, m_HDRRTFormat(ECHO_FMT_R11F_G11F_B10F)
		, m_bEnableClusterForwardLight(true)
		, m_clusterLightLevel(ClusteForwardLighting::CFL_High)
		, m_forceFirstPersonNearClipDistance(DEFAULT_FORCE_FIRST_PERSON_NEAR_CLIP_DISTANCE)
		, m_forceSceneNearClipDistance(DEFAULT_FORCE_SCENE_NEAR_CLIP_DISTANCE)
		, m_enableReverseDepth(false)
		, m_enable32BitDepth(false)
		, m_enableGLDepthZeroToOne(false)
		, m_depthFormat(ECHO_FMT_D24S8_PACKED)
	{
		EchoZoneScoped;

		m_version = StringConverter::toString(ECHO_VERSION_MAJOR) + "." +
			StringConverter::toString(ECHO_VERSION_MINOR) + "." +
			StringConverter::toString(ECHO_VERSION_PATCH) + " " +
			"(" + ECHO_VERSION_NAME + ")";

		UniformNameIDUtil::init();
		SamplerNameIDUtil::init();
		BufferNameIDUtil::init();

		if (mRuntimeMode == eRM_Editor)
		{
			mIsParticleTickAsync = false;
		}
		srand((unsigned int)time(0));

#if _WIN32
		memset(m_dwTreadID, 0, sizeof(m_dwTreadID));
		m_dwTreadID[EM_THREAD_MAIN] = GetCurrentThreadId();
#endif

		// Create log manager and default log file if there is no log manager yet
		if (LogManager::getSingletonPtr() == 0)
		{
			mLogManager = new LogManager();
			mLogManager->createLog(logFileName, true, true);
		}

		LogManager::getSingleton().logMessage("*-*-* ECHO Initialising", Echo::LML_CRITICAL);
		LogManager::getSingleton().logMessage("*-*-* Version " + m_version, Echo::LML_CRITICAL);

		//////////////////////////////////////////////////////////////////////////
		new ResourceFolderManager();
		ResourceFolderManager::instance()->LoadConfigFile(mRootResourcePath);

		new StringTable();
		if (!StringTable::instance()->loadTable("echo/EchoStringTable.cfg"))
		{
			LogManager::instance()->logMessage(LML_CRITICAL,
				"String table load failed: echo/EchoStringTable.cfg.");
		}

		new IOCPController(); // init IO complete port controller

		m_pResourceManagerFactory = new ResourceManagerFactory();
		new MaterialObjectManager();
		new ControllerManager();
		new TextureManager();
		new GpuParamsManager();
		new ShaderContentManager();
		new MaterialAnimManager();
		new ShareRoleConfig();
		new LookAtCfg();
		new PetBatchMaterialManager();
		new EffectSystem();

		ResourceWorkQueue* defaultQ = new ResourceWorkQueue(Name("EchoResourceWorker"));
		defaultQ->setResponseProcessingTimeLimit(8);
		mWorkQueue = defaultQ;
		//mWorkQueue->startup();

		// ResourceBackgroundQueue
		new ResourceBackgroundQueue();
		ResourceBackgroundQueue::instance()->initialise();

		new ResourceDataMemoryManager();

		new MaterialShaderPassManager();
		MaterialShaderPassManager::instance()->initialise();

		new MaterialBuildHelper();
		MaterialBuildHelper::instance()->initialise();

		new RoleAnimationLoadManager();
		RoleAnimationLoadManager::instance()->initialise();

		//new ActorInfoLoadManager();
		//ActorInfoLoadManager::instance()->initialise();

		new RoleMeshLoadManager();
		RoleMeshLoadManager::instance()->initialise();

		new MaterialCompileManager();
		MaterialCompileManager::instance()->initialise();

		TextureManager::instance()->initialise();

		new BufferManager();
		BufferManager::instance()->initialise();

		new ReflectCoeResourceManager();
		new SceneCommonResourceManager();
		//new PageResourceManager();
		new MeshMgr();
		new RoadResourceManager();
		//new ActorInfoManager();
		new StreamResourceManager();
		new WaterResourceManager();
		new WaterGlobalMaterialHelper();
		BiomeVegGroup::InitDefaultGroup();
		new VegetationResourceManager();
		new VegMeshResManager();
		new VegetationAreaResourceManager();
		new VegetationLayerManager();
		VegetationLayerManager::instance()->initialise();
		new SkeletonManager();
		new SoftbodyManager();
		new ClothAssetResourceManager();
		new CustomSkeleonManager();
		new BatchManager();
		new RoleBatchManager();
		//new WeatherManager();
		//new TODManager();
		new CShaderMan();
		new MaterialManager();
		new MatConfigParseMan();
		new MaterialV2Manager();
		new PassManager();
		new DataStreamManager();

		new EffectResourceManager();
		new TerrainSystem();
		new TerrainRenderMesh();
		new LandformResourceManager();
		new KarstSystem();

		new AnimationManager();
		new RoleManager();
		new BlendMaskManager();
		new RoleLocatorManager();
		new RoleCollisionManager();
		new ComponentManager();
		new MountainManager();

		//new PtFactoryManager();
		new EffectFactoryManager();

		new JobSystem();
		new SkeletonSyncManager();
		new BuildLodMeshManager();
		new OcclusionPlaneManager();
		new BuildingLodManager();
		new EffectTickJobManager();
		new DynamicVideoParamManager();
		new MediaManager();
		new OceanTickJobManager();
		new EntityBatchPageResManager();
		new RetargetingSkeletonManager();
		new AtlasInfoManager();
		new CommonJobManager();

#ifdef ECHO_EDITOR
		new ErrorMessageGroupManager();
#endif // ECHO_EDITOR

		Async::AsyncTaskQueueInit();

		PlatformInformation::log(LogManager::getSingleton().getDefaultLog());

		mTimer = new Timer();
		mTimer->reset();

		mMath = new Math();

		mFirstTimePostWindowInit = false;
		mtimePerFram = (uint32)(1000.0 / 30.0);

		initSurfaceType();

		TextureManager::instance()->loadTextureLimitConfigFile();

		//new ProtocolComponentManager();
		//new ActorManager();
		new FreeTypeResourceManager();
		new FontManager();
		new UIRenderSystem();

		new PlanetRegionManager;
		new PlanetRoadResourceManager;
		new SphericalTerrainResourceManager;
		new StampTerrain3D::HeightMapDataManager;
		new VoxelTerrainFractalManager;
		new PlanetTextureResourceManager;
		UIRenderSystem::instance()->initAsync();

		new PCG::BiomeGenObjectLibraryManager();
		new PCG::GenObjectSchedulerManager();
		new OAE::OAESystem();
		new PlaneVegetationSystem();

		new efgui::FGUITickListener();

		new DebugDrawManager();
		new UIAlterImageRenderHelperManager();
		new PortraitManager();
		PortraitManager::instance()->init();

		new EchoProfilerManager();

		//new ResLoadManager();

		new SheetManager();
		//#if defined( __WIN32__ ) || defined( _WIN32 )
				//new SqliteMgr();
				//SqliteMgr::instance()->init("config/SqliteConfig.cfg", true);	//暂时移到componentsystem中初始化,如有需求在考虑
		//#endif
		new ThreadPool();

#if ECHO_NO_FREEIMAGE == 0
		// Register image codecs
		FreeImageCodec::startup();
#endif

#if ECHO_NO_DDS_CODEC == 0
		DDSCodec::startup();
#endif

#ifndef __APPLE__
		new ShaderClientManager();
		ShaderClientManager::instance()->Init();
#endif

		new EnvironmentLight();

		new GpuPoolManager();
		new GpuPoolManagerImpl();
		new GpuCullManager();
		new GpuBatchManager();
		GpuBatchManager::instance()->Init();
		new IndirectDrawManager();
		new TextureStreamingHelper();
		new EchoTrace::EchoTraceManager(rootResPath);
		new ProfilerInfoManager();
#ifdef _WIN32
		new HotUpdateAssemblies();
#endif // _WIN32
		new HWMediaPlayer();
		new EchoCoreStatusData();

		new EchoDataAcqusition();

#ifdef ECHO_EDITOR
		new AIGCManager();
#endif
		new SDFResManagerFactory();
		new DeformSnowMgr();
	}

	//-----------------------------------------------------------------------
	Root::~Root()
	{
		delete SDFResManagerFactory::instance();
		delete DeformSnowMgr::instance();
		EchoZoneScoped;
		delete HWMediaPlayer::instance();
#ifdef _WIN32
		delete HotUpdateAssemblies::instance();
#endif // _WIN32
#ifdef ECHO_EDITOR
		delete AIGCManager::instance();
#endif

		GpuCullManager::instance()->uninit();
		if (Root::instance()->m_bUseGpuPool)
		{
			GpuPoolManagerImpl::instance()->uninit();
		}
		delete IndirectDrawManager::instance();
		delete GpuCullManager::instance();
		delete GpuPoolManager::instance();
		delete GpuPoolManagerImpl::instance();
		GpuBatchManager::instance()->UnInit();
		delete GpuBatchManager::instance();

		delete EnvironmentLight::instance();

		//delete ActorManager::instance();
		//delete ProtocolComponentManager::instance();
		delete EchoProfilerManager::instance();

		// NOTE(yanghang):Debug help render memory release.
		{
			auto debugDrawManager = DebugDrawManager::instance();
			SAFE_DELETE(debugDrawManager);

			auto renderHelperManager = UIAlterImageRenderHelperManager::instance();
			SAFE_DELETE(renderHelperManager);
			auto portraitManager = PortraitManager::instance();
			SAFE_DELETE(portraitManager);

			auto uiRenderSystem = UIRenderSystem::instance();
			SAFE_DELETE(uiRenderSystem);

			delete PlanetTextureResourceManager::instance();
			delete SphericalTerrainResourceManager::instance();
			delete PlanetRoadResourceManager::instance();
			delete StampTerrain3D::HeightMapDataManager::instance();
			delete VoxelTerrainFractalManager::instance();
			delete PlanetRegionManager::instance();
			// delete SphericalVoronoiRegionLibrary::instance();

			auto fguiTickListener = efgui::FGUITickListener::instance();
			SAFE_DELETE(fguiTickListener);

		}

		//IMPORTANT(yanghang): Basic component dependent by UI system.
		// so need to release after them.
		delete FontManager::instance();
		delete FreeTypeResourceManager::instance();
		delete AtlasInfoManager::instance();
		SceneManagerMap::iterator itb, ite;
		itb = mSceneManagerMap.begin();
		ite = mSceneManagerMap.end();
		for (; itb != ite; ++itb)
		{
			delete itb->second;
		}
		mSceneManagerMap.clear();
		delete EffectTickJobManager::instance();
		delete EffectSystem::instance();
		delete OceanTickJobManager::instance();


		//delete TODManager::instance();

		delete PetBatchMaterialManager::instance();

		//delete WeatherManager::instance();
		delete SkeletonSyncManager::instance();
		delete BuildLodMeshManager::instance();
		delete CommonJobManager::instance();

		delete MediaManager::instance();


		delete ResourceDataMemoryManager::instance();

		//delete IFGlypher::instance();
		delete ReflectCoeResourceManager::instance();
		delete SceneCommonResourceManager::instance();
		delete RoleBatchManager::instance();
		delete BatchManager::instance();
		delete ComponentManager::instance();
		//delete ActorInfoManager::instance();
		delete EffectResourceManager::instance();
		delete RoleManager::instance();
		delete RoleLocatorManager::instance();
		delete RoleCollisionManager::instance();
		delete MeshMgr::instance();
		delete MaterialManager::instance();
		delete MaterialV2Manager::instance();
		delete RoadResourceManager::instance();
		delete StreamResourceManager::instance();
		delete WaterResourceManager::instance();
		delete WaterGlobalMaterialHelper::instance();
		BiomeVegGroup::UnitDefaultGroup();
		delete VegetationResourceManager::instance();
		delete VegMeshResManager::instance();
		delete VegetationAreaResourceManager::instance();
		delete VegetationLayerManager::instance();
		delete ResourceBackgroundQueue::instance();
		delete MaterialBuildHelper::instance();
		delete RoleAnimationLoadManager::instance();
		delete RoleMeshLoadManager::instance();
		//delete ActorInfoLoadManager::instance();
		delete SkeletonManager::instance();
		delete BlendMaskManager::instance();
		delete RetargetingSkeletonManager::instance();
		delete CustomSkeleonManager::instance();
		delete SoftbodyManager::instance();
		delete ClothAssetResourceManager::instance();
		delete AnimationManager::instance();
		delete ShaderContentManager::instance();
		delete TerrainSystem::instance();
		delete TerrainRenderMesh::instance();
		delete KarstSystem::instance();
		delete PlaneVegetationSystem::instance();
		delete OAE::OAESystem::instance();
		delete PCG::GenObjectSchedulerManager::instance();
		delete PCG::BiomeGenObjectLibraryManager::instance();
		delete GpuParamsManager::instance();
		delete TextureStreamingHelper::instance();
		delete TextureManager::instance();
		delete ControllerManager::instance();
		delete MountainManager::instance();
		delete MaterialCompileManager::instance();
		delete EntityBatchPageResManager::instance();
		delete DataStreamManager::instance();
		delete LandformResourceManager::instance();
		//#if defined( __WIN32__ ) || defined( _WIN32 )
				//delete SqliteMgr::instance();
		//#endif
		delete SheetManager::instance();
		delete ThreadPool::instance();

		Async::AsyncTaskQueueUninit();

		delete mWorkQueue;
		delete m_pResourceManagerFactory;
		delete CShaderMan::instance();
		delete MatConfigParseMan::instance();
		delete ShareRoleConfig::instance();
		delete LookAtCfg::instance();
		//delete PtFactoryManager::instance();
		delete EffectFactoryManager::instance();
		mQueueVisiable.clear();


		delete EchoDataAcqusition::instance();

		delete JobSystem::instance();
		delete OcclusionPlaneManager::instance();
		delete BuildingLodManager::instance();
		delete DynamicVideoParamManager::instance();
		delete MaterialObjectManager::instance();
		delete MaterialAnimManager::instance();
		//delete ResLoadManager::instance();
		delete BufferManager::instance();
		delete PassManager::instance();

		MaterialShaderPassManager::instance()->shutdown();
		delete MaterialShaderPassManager::instance();

		delete mMath;
		delete mTimer;


		delete ProfilerInfoManager::instance();
		delete EchoCoreStatusData::instance();
#ifdef ECHO_EDITOR
		delete ErrorMessageGroupManager::instance();
#endif //ECHO_EDITOR
		if (NULL != m_pRenderSystem)
		{
			delete m_pRenderSystem;
			m_pRenderSystem = NULL;
		}

		if (m_pRenderer)
		{
			if (ECHO_THREAD_ST != m_threadMode)
			{
				char* pCacheRootPath = (char*)m_context->pCacheRootPath;
				free(pCacheRootPath);
			}

			m_pRenderer->uninit(m_context);
			delete m_pRenderer;
			m_pRenderer = nullptr;
		}

		delete StringTable::instance();

		delete ResourceFolderManager::instance();

		m_bInitialise = false;

#if ECHO_NO_FREEIMAGE == 0
		FreeImageCodec::shutdown();
#endif
#if ECHO_NO_DDS_CODEC == 0
		DDSCodec::shutdown();
#endif

		LogManager::getSingleton().logMessage("*-*-* ECHO Destroyed", LML_CRITICAL);
		delete LogManager::getSingletonPtr();
#ifdef __APPLE__
#else
		delete ShaderClientManager::instance();
#endif

		delete IOCPController::instance();
		delete EchoTrace::EchoTraceManager::instance();
	}

	const char* Root::GetVersion()
	{
		EchoZoneScoped;

		class VersionString
		{
		public:
			VersionString()
			{
				EchoZoneScoped;

				snprintf(mVersionStr, sizeof(mVersionStr), "%d.%d.%d(%s)",
					ECHO_VERSION_MAJOR, ECHO_VERSION_MINOR, ECHO_VERSION_PATCH, ECHO_VERSION_NAME);
			}

			char mVersionStr[32] = { '\0' };
		};

		static const VersionString ver_str;
		return ver_str.mVersionStr;
	}

	//bool Root::RaycastReturnNormal(const Echo::Vector3& origin, const Echo::Vector3& dir, float& outDistance, Echo::Vector3& outDir)
	//{
	//	if (NULL == mRaycastReturnNormalFun)
	//	{
	//		return false;
	//	}
	//	return mRaycastReturnNormalFun(origin,dir,outDistance,outDir);
	//}

	void Root::OutEventMemInfo(DataStreamPtr& stream)
	{
		EchoZoneScoped;

		if (mOutEventMemInfoFun == NULL)
			return;
		mOutEventMemInfoFun(stream);
	}

	void Root::OutEventPerpareResourceInfo(DataStreamPtr& out)
	{
		//LandformDataHandler::instance()->OutPutPrepareResourceLog(out);
		MaterialBuildHelper::instance()->OutPutPrepareResourceLog(out);
		PlanetTextureResourceManager::instance()->OutPutPrepareResourceLog(out);
	}

	void Root::setClusterForwardLightEnable(bool enable, ClusteForwardLighting::Level level)
	{
		EchoZoneScoped;

		m_bEnableClusterForwardLight = enable;

		if (m_bEnableClusterForwardLight == false)
		{
			// NOTE: Level parameter is ignored.
			m_clusterLightLevel = ClusteForwardLighting::CFL_Off;
		}
		else
		{
			m_clusterLightLevel = level;
		}

		for (auto mapIte : mSceneManagerMap)
		{
			SceneManager* sceneManager = mapIte.second;
			ClusteForwardLighting* cfl = sceneManager->getClusterForwardLighting();
			cfl->setLevel(m_clusterLightLevel);
		}
	}

	bool isUseHDRFormatFP16(std::string _vendor, std::string _renderer, std::string _model, std::string _cpu)
	{
		EchoZoneScoped;

		bool ans = false;
		//统一转为小写
		std::transform(_vendor.begin(), _vendor.end(), _vendor.begin(), ::tolower);
		std::transform(_renderer.begin(), _renderer.end(), _renderer.begin(), ::tolower);
		std::transform(_model.begin(), _model.end(), _model.begin(), ::tolower);
		std::transform(_cpu.begin(), _cpu.end(), _cpu.begin(), ::tolower);

		String strDeviceFeatureLimitPath = "config/DeviceFeatureLimit.json";
		DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(strDeviceFeatureLimitPath.c_str()));
		if (pDataStream.isNull())
			return false;

		size_t nSize = pDataStream->size();
		char* pData = new char[nSize + 1];
		memset(pData, 0, nSize + 1);
		pDataStream->read(pData, nSize);

		JSNODE Root = cJSON_Parse(pData);
		delete[] pData;

		if (nullptr == Root)
			return false;

		JSNODE hRootNode = cJSON_GetObjectItem(Root, "DeviceFeatureLimit");
		assert(nullptr != hRootNode);

		JSNODE itemHdrFormat = cJSON_GetObjectItem(hRootNode, "hdrFormatFP16");
		if (nullptr == itemHdrFormat)
		{
			return false;
		}

		assert(cJSON_Array == itemHdrFormat->type);
		if (String("hdrFormatFP16") != itemHdrFormat->string)
		{
			return false;
		}

		for (JSNODE pCurrent = itemHdrFormat->child; pCurrent != nullptr; pCurrent = pCurrent->next)
		{
			bool result = false;
			String vendor = "";
			String renderer = "";
			String model = "";
			String cpu = "";

			result = READJS(pCurrent, vendor);
			result = READJS(pCurrent, renderer);
			result = READJS(pCurrent, model);
			result = READJS(pCurrent, cpu);

			//统一转为小写
			std::transform(vendor.begin(), vendor.end(), vendor.begin(), ::tolower);
			std::transform(renderer.begin(), renderer.end(), renderer.begin(), ::tolower);
			std::transform(model.begin(), model.end(), model.begin(), ::tolower);
			std::transform(cpu.begin(), cpu.end(), cpu.begin(), ::tolower);

			if (!model.empty())
			{
				if (std::string::npos != _model.find(model))
				{
					ans = true;
					break;
				}
			}
			if (!renderer.empty())
			{
				if (std::string::npos != _renderer.find(renderer) && _vendor == vendor)
				{
					ans = true;
					break;
				}
			}
			if (!cpu.empty())
			{
				if (std::string::npos != _cpu.find(cpu))
				{
					ans = true;
					break;
				}
			}
		}

		cJSON_Delete(Root);
		return ans;
	}

	void Root::setRenderSystem(const ECHO_GRAPHIC_PARAM* pGraphicParam, const char* hookPath)
	{
		EchoZoneScoped;

		if (m_bInitialise)
			return;

		m_DeviceType = pGraphicParam->eClass;
		m_threadMode = pGraphicParam->threadMode;
		m_bUseRenderCommand = (m_threadMode != ECHO_THREAD_NULL);

		ECHO_GRAPHIC_PARAM _graphicParam = *pGraphicParam;
		String sPlatform;

#ifdef _WIN32
		sPlatform = "Windows";
#elif defined(__ANDROID__)
		sPlatform = "Android";
#elif defined(__APPLE__)
		sPlatform = "IOS";
#elif defined(__OHOS__)
		sPlatform = "Harmony";
#endif
		loadEchoEngineCfg(_graphicParam, sPlatform);

		if (!m_bUseRenderCommand)
		{
			m_bForceUseRcSingleThread = true;
		}

		if (m_bForceUseRcSingleThread)
		{
			m_bUseRenderCommand = true;
			m_threadMode = ECHO_THREAD_ST;
			_graphicParam.threadMode = m_threadMode;
		}

		if (m_enable32BitDepth)
		{
			m_depthFormat = ECHO_FMT_D_FP32_S8_UINT;
		}

		char szBuff[512] = { '\0' };
		snprintf(szBuff, sizeof(szBuff) - 1, "-info-\tForceGx: %d   ForceRcST: %d   RenderAPI: %d   UseRC: %d   UseMode: %d   TexType: %d   UseIOCP: %d   UseNetIO: %d   SoundIOCP:%d\n",
			m_bForceUseGxRender, m_bForceUseRcSingleThread, m_DeviceType, m_bUseRenderCommand, m_threadMode, m_RenderTextureType, m_bUseIOCP, m_bUseNetAsynIO, m_bSoundUseIOCP);
		LogManager::getSingleton().logMessage(szBuff, LML_CRITICAL);

#ifdef _WIN32
		OutputDebugString("[Echo]\t");
		OutputDebugString(szBuff);
		OutputDebugString("\r\n");
#endif

		onOpenFloatExceptionCheck();

		if (m_bUseRenderCommand)
		{
			m_pRenderer = new Renderer();

			m_context = NEW_RcContext;
			m_context->bValidate = m_bValidate;
			m_context->bShaderDebugInfo = m_bShaderDebugInfo;
			m_context->bGPUResourceDebugLabel = m_bGPUResourceLabel;
			m_context->threadMode = m_threadMode;
			m_context->apiType = m_DeviceType;
			m_context->textureType = m_RenderTextureType;
			m_context->pNative = _graphicParam.pInstance;
			m_context->pCustom = _graphicParam.pHandle;
			m_context->pFunction = (void*)(&_onHandRenderCommandCallBack);
			m_context->depthFormat = m_depthFormat;
			m_context->pSamplerNameIDMap = &SamplerNameIDUtil::getSamplerNameToIDMap();
			m_context->pUniformNameIDMap = &UniformNameIDUtil::getUniformNameToIDMap();
			m_context->pBufferNameIDMap = &BufferNameIDUtil::getBufferNameToIDMap();
			m_context->bGLEnableDepthZeroToOne = m_enableGLDepthZeroToOne;
			m_context->pGraphicParam = &_graphicParam;
			m_context->engineVersion = /*(((uint32_t)(ECHO_VERSION_MAJOR)) << 22U) | (((uint32_t)(ECHO_VERSION_MINOR)) << 12U) | */((uint32_t)(ECHO_VERSION_PATCH));

			if (ECHO_THREAD_ST != m_threadMode)
			{
				size_t nPathLen = strlen(hookPath);
				char* pRootPath = (char*)malloc(nPathLen + 1);
				pRootPath[nPathLen] = '\0';
				memcpy(pRootPath, hookPath, nPathLen);
				m_context->pCacheRootPath = pRootPath;
			}
			else
			{
				m_context->pCacheRootPath = hookPath;
			}

			m_pRenderer->init(m_context);

			if (m_threadMode == ECHO_THREAD_MT0)
			{
				//wait for init.
				m_pRenderer->onWaitRenderThreadInit();
			}

			if (_graphicParam.eClass == ECHO_DEVICE_GLES3 && m_enableGLDepthZeroToOne)
			{
				if (!m_pRenderer->hasCapability(ECHO_RSC_GL_EXT_CLIP_CONTROL))
				{
					// not support GL_EXT_CLIP_CONTROL, disable reverse depth.
					m_enableGLDepthZeroToOne = false;
					m_enableReverseDepth = false;
					m_enable32BitDepth = false;
				}
			}

			if (ECHO_DEVICE_DX9C == m_DeviceType ||
				isUseHDRFormatFP16(_graphicParam.vendor, _graphicParam.renderer, _graphicParam.model, _graphicParam.cpuInfo))
			{
				m_HDRRTFormat = ECHO_FMT_RGBA_FP16;
			}

			// if device don't support 32bit depth, RC will change m_context->depthFormat to ECHO_FMT_D24S8_PACKED
			m_depthFormat = m_enable32BitDepth ? m_context->depthFormat : ECHO_FMT_D24S8_PACKED;
		}

		m_pRenderSystem = new RenderSystem(m_pRenderer, &_graphicParam, hookPath);
		m_pRenderSystem->useRenderStrategy(m_renderStrategyType);

		MaterialObjectManager::instance()->updateDeviceSupportFeature();

		mWorkQueue->startup();

		if (_graphicParam.cpuCoreNumber == 4) // default
		{
			_graphicParam.cpuCoreNumber = std::thread::hardware_concurrency(); // get real cpu core number
		}
		uint16 jobTreadCnt = _graphicParam.cpuCoreNumber / 2;

		JobSystem::instance()->StartWork(jobTreadCnt);

		EchoDataAcqusition::instance()->verify();

		TerrainRenderMesh::instance()->init();

		MaterialCompileManager::instance()->Init();
		EffectSystem::instance()->init();
		SphericalTerrainAtmosData::instance()->load();

		// Tell scene managers
		SceneManagerMap::iterator itb, ite;
		itb = mSceneManagerMap.begin();
		ite = mSceneManagerMap.end();
		for (; itb != ite; ++itb)
		{
			SceneManager* sm = itb->second;
			sm->_setDestinationRenderSystem(m_pRenderSystem);
		}

		// 粒子系统材质需要重新初始化
// 		EffectManager::DelInitParticleSys();
// 		EffectManager::InitParticleSys(m_pRenderSystem);


		setClusterForwardLightEnable(m_bEnableClusterForwardLight, m_clusterLightLevel);

		m_bInitialise = true;
	}

	void Root::loadEchoEngineCfg(ECHO_GRAPHIC_PARAM& graphicParam, const String& sPlatform)
	{
		SystemConfigFile cfg;
		cfg.load("echo/EchoEngine.cfg");

		if (eRM_Editor == mRuntimeMode)
		{
			m_RenderTextureType = ECHO_TEX_TYPE_DXT;

			const SettingsMultiMap* pSetMap = cfg.getSettingsMap(sPlatform);
			if (pSetMap)
			{
				SettingsMultiMap::const_iterator it = pSetMap->find("EditorRenderAPI");
				if (it != pSetMap->end())
				{
#ifndef __APPLE__
					m_DeviceType = (ECHO_DEVICE_CLASS)StringConverter::parseUnsignedInt(it->second);
					if (m_DeviceType == ECHO_DEVICE_DX9C)
						m_DeviceType = ECHO_DEVICE_DX11;

#ifdef WIN32
					if (m_DeviceType == ECHO_DEVICE_VULKAN)
					{
						const char u8str[] = "引擎配置文件被设为以Vulkan API启动，你确定要使用Vulkan启动吗？\n如果你不知道这是什么意思，请选择否。";
						UINT wLen = MultiByteToWideChar(CP_UTF8, NULL, u8str, sizeof(u8str), NULL, NULL);
						WCHAR* wstr = new WCHAR[wLen + 1];
						wLen = MultiByteToWideChar(CP_UTF8, NULL, u8str, sizeof(u8str), wstr, wLen);
						wstr[wLen] = 0;
						UINT mLen = WideCharToMultiByte(936, NULL, wstr, wLen, NULL, NULL, NULL, NULL);
						CHAR* mbstr = new CHAR[mLen + 1];
						mLen = WideCharToMultiByte(936, NULL, wstr, wLen, mbstr, mLen, NULL, NULL);
						mbstr[mLen] = 0;
						delete[]wstr;
						if (MessageBox(NULL, mbstr, "Render API", MB_YESNO) == IDYES)
						{
							m_DeviceType = ECHO_DEVICE_VULKAN;
						}
						else
						{
							m_DeviceType = ECHO_DEVICE_DX11;
						}
						delete[]mbstr;
					}
#endif // WIN32
					graphicParam.eClass = m_DeviceType;
#endif
				}

				it = pSetMap->find("EditorRenderMode");
				if (it != pSetMap->end())
				{
					m_threadMode = (ECHO_THREAD_MODE)StringConverter::parseUnsignedInt(it->second);
					m_bUseRenderCommand = (m_threadMode != ECHO_THREAD_NULL);
					graphicParam.threadMode = m_threadMode;
				}

				it = pSetMap->find("ParticleAtlasHash");
				if (it != pSetMap->end())
				{
					m_bParticleAtlasHashVerify = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("UseSpringSoft");
				if (it != pSetMap->end())
				{
					m_bUseSpringSoft = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("MultiCameraRendering");
				if (it != pSetMap->end())
				{
					m_bMultiCameraRendering = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("TMagicProfileEnable");
				if (it != pSetMap->end())
				{
					m_bTMagicProfileEnabled = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("SoundProfileEnable");
				if (it != pSetMap->end())
				{
					m_bSoundProfileEnabled = m_bTMagicProfileEnabled ? 0 : (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("TMagicLiveUpdate");
				if (it != pSetMap->end()) {
					m_bTMagicLiveUpdate = StringConverter::parseBool(it->second, m_bTMagicLiveUpdate);
				}
				it = pSetMap->find("BellProfileEnable");
				if (it != pSetMap->end())
				{
					m_bBellProfileEnabled = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("SoundProfileEnable");
				if (it != pSetMap->end())
				{
					m_bSoundProfileEnabled = m_bBellProfileEnabled ? 0 : (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("BellLiveUpdate");
				if (it != pSetMap->end()) {
					m_bBellLiveUpdate = StringConverter::parseBool(it->second, m_bBellLiveUpdate);
				}
				it = pSetMap->find("UseNewMountain");
				if (it != pSetMap->end())
				{
					m_bUseNewMountain = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("Profiler");
				if (it != pSetMap->end())
				{
					EchoProfilerManager::instance()->ProfilerOpen(StringConverter::parseBool(it->second));
				}
				it = pSetMap->find("Trace");
				if (it != pSetMap->end())
				{
					EchoTrace::EchoTraceManager::instance()->SetEchoTraceActive(StringConverter::parseBool(it->second));
				}
				it = pSetMap->find("ProtocolComFrameTimeInMilliSec");
				if (it != pSetMap->end())
				{
					m_bProtocolComFrameTimeInMilliSec = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("EchoRenderImGui");
				if (it != pSetMap->end())
				{
					m_bEchoRenderImGui = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("ShowRenderthreadInfo");
				if (it != pSetMap->end())
				{
					m_bShowRenderThreadInfo = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("UseValidationLayer");
				if (it != pSetMap->end())
				{
					m_bValidate = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("GenerateShaderDebugInfo");
				if (it != pSetMap->end())
				{
					m_bShaderDebugInfo = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("KeepGPUResourceDebugLabel");
				if (it != pSetMap->end())
				{
					m_bGPUResourceLabel = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("UseGpuPool");
				if (it != pSetMap->end())
				{
					m_bUseGpuPool = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("AsyncReleaseActor");
				if (it != pSetMap->end())
				{
					m_bAsyncReleaseActor = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("DefaultActorSync");
				if (it != pSetMap->end())
				{
					m_bDefaultActorSync = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("ResourceBaseDestroyByFrame");
				if (it != pSetMap->end())
				{
					m_bResourceBaseDestroyByFrame = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("CreateBufferInResourceThread");
				if (it != pSetMap->end())
				{
					m_bCreateBufferInResourceThread = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("MaxBindBoneCnt");
				if (it != pSetMap->end())
				{
					m_maxBindBoneCnt = StringConverter::parseUnsignedInt(it->second);
				}

				it = pSetMap->find("Enable3DUIBatchRender");
				if (it != pSetMap->end())
				{
					UIRenderSystem* uiSys = UIRenderSystem::instance();
					if (uiSys)
					{
						bool enable = StringConverter::parseBool(it->second);
						uiSys->setBatchRenderingEnable(enable);
					}
				}

				it = pSetMap->find("EnableNewDecalBatchRender");
				if (it != pSetMap->end())
				{
					m_bEnableNewDecalBatchRender = StringConverter::parseBool(it->second);
				}

				it = pSetMap->find("Force3DUIBatchUseUniform");
				if (it != pSetMap->end())
				{
					UIRenderSystem* uiSys = UIRenderSystem::instance();
					if (uiSys)
					{
						bool enable = StringConverter::parseBool(it->second);
						uiSys->m_force3DUIBatchUseUniform = enable;
					}
				}
				it = pSetMap->find("Enable3DUINewUpdateStyle");
				if (it != pSetMap->end())
				{
					UIRenderSystem* uiSys = UIRenderSystem::instance();
					if (uiSys)
					{
						bool enable = StringConverter::parseBool(it->second);
						uiSys->m_useNewUpdateStyle = enable;
					}
				}

				it = pSetMap->find("EnableReverseDepth");
				if (it != pSetMap->end())
				{
					m_enableReverseDepth = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("Enable32BitDepth");
				if (it != pSetMap->end())
				{
					m_enable32BitDepth = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("EnableGLDepthZeroToOne");
				if (it != pSetMap->end())
				{
					m_enableGLDepthZeroToOne = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("UseTextureStreaming");
				if (it != pSetMap->end())
				{
					m_bUseTextureStreaming = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("UseActorBuildingSimpleShadow");
				if (it != pSetMap->end())
				{
					m_bUseActorBuildingSimpleShadow = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("PCCustomShadow");
				if (it != pSetMap->end())
				{
					m_bPCCustomShadow = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("UseSSAODebugCom");
				if (it != pSetMap->end())
				{
					m_useSSAODebugCom = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("EditorVegSyncTask");
				if (it != pSetMap->end())
				{
					m_bVegSyncTaskEnable = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("VegPreZ");
				if (it != pSetMap->end())
				{
					BiomeVegOpt::SetPreZEnabled(StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("VegDynamicBatch");
				if (it != pSetMap->end())
				{
					BiomeVegOpt::SetDyBatchEnabled(StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("VegDyBatchLimit");
				if (it != pSetMap->end())
				{
					BiomeVegOpt::SetDyBatchLimit(StringConverter::parseUnsignedInt(it->second));
				}
				it = pSetMap->find("VegGeoFac");
				if (it != pSetMap->end())
				{
					BiomeVegOpt::SetGeoFactorEnabled(StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("VegOrthoBox");
				if (it != pSetMap->end())
				{
					BiomeVegOpt::SetOrthoBoxEnabled(StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("RenderStrategy");
				if (it != pSetMap->end())
				{
					m_renderStrategyType = StringConverter::parseUnsignedInt(it->second);
				}
				it = pSetMap->find("UseGeneralRS");
				if (it != pSetMap->end())
				{
					m_bUseGeneralRenderStrategy = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("EnableVisibilityBuffer");
				m_bEnableVisibilityBuffer = false;
				if (m_renderStrategyType == 1 && it != pSetMap->end())
				{
					m_bEnableVisibilityBuffer = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("EnableShaderDebugFlag");
				m_bEnableShaderDebugFlag = false;
				if (it != pSetMap->end())
				{
					m_bEnableShaderDebugFlag = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("EnableMatV2CompatibilityMode");
				if (it != pSetMap->end())
				{
					m_bEnableMatV2CompatibilityMode = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("DebugPassPost");
				if (it != pSetMap->end())
				{
					m_bEnableDebugPassPost = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("RoomPortalCull");
				if (it != pSetMap->end())
				{
					m_bEnableRoomPortalCull = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
#ifdef ECHO_EDITOR
				it = pSetMap->find("EditorAutoSaveActor");
				if (it != pSetMap->end())
				{
					m_bEnableEditorAutoSaveActor = (StringConverter::parseUnsignedInt(it->second) != 0);
				}

				it = pSetMap->find("EditorExportToEchoActor");
				if (it != pSetMap->end())
				{
					m_bEnableEditorExportToEchoActor = (StringConverter::parseUnsignedInt(it->second) != 0);
				}

				it = pSetMap->find("EditorEnableAIGC");
				if (it != pSetMap->end())
				{
					m_bEnableEditorAIGC = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
#endif

				it = pSetMap->find("ClusterLightLevel");
				if (it != pSetMap->end())
				{
					int light_level = StringConverter::parseUnsignedInt(it->second);
					ClusteForwardLighting::Level new_level;
					switch (light_level)
					{
					case 0:
						new_level = ClusteForwardLighting::CFL_Off;
						break;
					case 1:
						new_level = ClusteForwardLighting::CFL_Low;
						break;
					case 2:
						new_level = ClusteForwardLighting::CFL_Middle;
						break;
					case 3:
						new_level = ClusteForwardLighting::CFL_High;
						break;
					default:
						new_level = ClusteForwardLighting::CFL_Low;
					};
					m_bEnableClusterForwardLight = (light_level != 0);
					m_clusterLightLevel = new_level;
				}
				it = pSetMap->find("EnableSphericalVoronoiRegion");
				if (it != pSetMap->end())
				{
					m_useSphericalVoronoiRegion = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("EnablePlanetRegionVisual");
				if (it != pSetMap->end())
				{
					m_enablePlanetRegionVisual = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("EnableManualSphericalVoronoi");
				if (it != pSetMap->end())
				{
					m_bManualPlanetRegion = StringConverter::parseBool(it->second);
				}

				it = pSetMap->find("RoadMeshModel");
				if (it != pSetMap->end())
				{
					UsePavementTestNewMode = (StringConverter::parseUnsignedInt(it->second) != 0);
				}

				it = pSetMap->find("RemoteGatherInfo");
				if (it != pSetMap->end())
				{
					m_useRemoteGatherInfo = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("UseIK");
				if (it != pSetMap->end())
				{
					m_bUseIK = StringConverter::parseUnsignedInt(it->second);
				}
				it = pSetMap->find("UseOAE");
				if (it != pSetMap->end())
				{
					m_bUseOAE = StringConverter::parseUnsignedInt(it->second, 0);
				}
				PCG::GenObjectScheduler::ImportConfig(pSetMap);
				it = pSetMap->find("UseRagdoll");
				if (it != pSetMap->end())
				{
					m_bUseRagdoll = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("UseMDLodMesh");
				if (it != pSetMap->end())
				{
					m_LodMeshType = StringConverter::parseUnsignedInt(it->second);
				}
				it = pSetMap->find("RoleBatch");
				if (it != pSetMap->end())
				{
					int i = StringConverter::parseUnsignedInt(it->second);
					if (i == 0)
					{
						setRoleInstanceRenderEnable(false);
					}
					else
					{
						setRoleInstanceRenderEnable(true);
					}
				}
				it = pSetMap->find("SphOceanAsync");
				if (it != pSetMap->end())
				{
					int i = StringConverter::parseUnsignedInt(it->second, 0);
					if (i == 0)
					{
						SphericalOcean::s_asyncGenerateVertice = false;
					}
					else
					{
						SphericalOcean::s_asyncGenerateVertice = true;
					}
				}
				else {
					SphericalOcean::s_asyncGenerateVertice = true;
				}
				it = pSetMap->find("HlodTextureSizeMaximum");
				if (it != pSetMap->end())
				{
					m_HlodTextureSizeMaximum = StringConverter::parseUnsignedInt(it->second, 0);
				}
			}
		}
		else if (eRM_Client == mRuntimeMode)
		{
			const SettingsMultiMap* pSetMap = cfg.getSettingsMap(sPlatform);
			if (pSetMap)
			{
				SettingsMultiMap::const_iterator it = pSetMap->find("RenderAPI");
				if (it != pSetMap->end())
				{
#ifndef __APPLE__
					m_DeviceType = (ECHO_DEVICE_CLASS)StringConverter::parseUnsignedInt(it->second);
					if (m_DeviceType == ECHO_DEVICE_DX9C)
						m_DeviceType = ECHO_DEVICE_DX11;
#ifdef WIN32
					if (m_DeviceType == ECHO_DEVICE_VULKAN)
					{
						const char u8str[] = "引擎配置文件被设为以Vulkan API启动，你确定要使用Vulkan启动吗？\n如果你不知道这是什么意思，请选择否。";
						UINT wLen = MultiByteToWideChar(CP_UTF8, NULL, u8str, sizeof(u8str), NULL, NULL);
						WCHAR* wstr = new WCHAR[wLen + 1];
						wLen = MultiByteToWideChar(CP_UTF8, NULL, u8str, sizeof(u8str), wstr, wLen);
						wstr[wLen] = 0;
						UINT mLen = WideCharToMultiByte(936, NULL, wstr, wLen, NULL, NULL, NULL, NULL);
						CHAR* mbstr = new CHAR[mLen + 1];
						mLen = WideCharToMultiByte(936, NULL, wstr, wLen, mbstr, mLen, NULL, NULL);
						mbstr[mLen] = 0;
						delete[]wstr;
						if (MessageBox(NULL, mbstr, "Render API", MB_YESNO) == IDYES)
						{
							m_DeviceType = ECHO_DEVICE_VULKAN;
						}
						else
						{
							m_DeviceType = ECHO_DEVICE_DX11;
						}
						delete[]mbstr;
					}
#endif // WIN32
					graphicParam.eClass = m_DeviceType;
#endif
				}

				it = pSetMap->find("RenderMode");
				if (it != pSetMap->end())
				{
					m_threadMode = (ECHO_THREAD_MODE)StringConverter::parseUnsignedInt(it->second);
					m_bUseRenderCommand = (m_threadMode != ECHO_THREAD_NULL);
					graphicParam.threadMode = m_threadMode;
				}

				it = pSetMap->find("RenderTexture");
				if (it != pSetMap->end())
				{
					m_RenderTextureType = (ECHO_TEX_TYPE)StringConverter::parseUnsignedInt(it->second);
				}

				it = pSetMap->find("CollectCompileInfo");
				if (it != pSetMap->end())
				{
					MaterialCompileManager::instance()->m_bCollectInfo = (StringConverter::parseUnsignedInt(it->second) != 0);
				}

				it = pSetMap->find("CollectServerCompileInfo");
				if (it != pSetMap->end())
				{
					MaterialCompileManager::instance()->m_bCollectServerInfo = (StringConverter::parseUnsignedInt(it->second) != 0);
				}

				it = pSetMap->find("CheckShaderCache");
				if (it != pSetMap->end())
				{
					MaterialCompileManager::instance()->m_bCheckShaderCache = (StringConverter::parseUnsignedInt(it->second) != 0);
				}

				it = pSetMap->find("UseCompileInfo");
				if (it != pSetMap->end())
				{
					MaterialCompileManager::instance()->m_bCompileInfo = (StringConverter::parseUnsignedInt(it->second) != 0);
				}

				it = pSetMap->find("UseCompileInfoExt");
				if (it != pSetMap->end())
				{
					MaterialCompileManager::instance()->m_bCompileInfoExt = (StringConverter::parseUnsignedInt(it->second) != 0);
				}

				it = pSetMap->find("RenderCompileInfo");
				if (it != pSetMap->end())
				{
					MaterialCompileManager::instance()->m_bRenderInfo = (StringConverter::parseUnsignedInt(it->second) != 0);
				}

				it = pSetMap->find("RoleBatch");
				if (it != pSetMap->end())
				{
					int i = StringConverter::parseUnsignedInt(it->second);
					if (i == 0)
					{
						setRoleInstanceRenderEnable(false);
					}
					else
					{
						setRoleInstanceRenderEnable(true);
					}
				}
				it = pSetMap->find("SimpleAnimation");
				if (it != pSetMap->end())
				{
					RoleMovable::enableSimpleAnim(StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("VertexCompress");
				if (it != pSetMap->end())
				{
					bool enable = StringConverter::parseUnsignedInt(it->second) != 0;
					Material::enableCompress(enable);
					MaterialV2::setGlobalSupportVertexCompress(enable);
				}
				it = pSetMap->find("CheckFloatMask");
				if (it != pSetMap->end())
				{
					m_checkFloatExceptionMask = StringConverter::parseUnsignedInt(it->second);
				}
				it = pSetMap->find("CreateLowestLodFreeEntity");
				if (it != pSetMap->end())
				{
					m_bCreateLowestLodFreeEntity = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("EntityObjectAutoLodMesh");
				if (it != pSetMap->end())
				{
					m_bEntityObjectAutoLodMesh = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("ReleaseFreeEntity");
				if (it != pSetMap->end())
				{
					RoleMovable::setReleaseFreeEntity(StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("ShowLowerEntity");
				if (it != pSetMap->end())
				{
					RoleMovable::setShowLowerEntity(StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("ReleaseBuildingFreeEntity");
				if (it != pSetMap->end())
				{
					m_bReleaseBuildingFreeEntity = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("ReleaseEntityCnt");
				if (it != pSetMap->end())
				{
					m_dwReleaseEntityCnt = StringConverter::parseUnsignedInt(it->second);
				}
				it = pSetMap->find("RebuildSkeleton");
				if (it != pSetMap->end())
				{
					m_bRebuildSkeleton = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("ReleaseMeshData");
				if (it != pSetMap->end())
				{
					m_bReleaseMeshData = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("UseSpringSoft");
				if (it != pSetMap->end())
				{
					m_bUseSpringSoft = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("PetBatch");
				if (it != pSetMap->end())
				{
					m_bEnablePetBatch = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("TMagicProfileEnable");
				if (it != pSetMap->end())
				{
					m_bTMagicProfileEnabled = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("SoundProfileEnable");
				if (it != pSetMap->end())
				{
					m_bSoundProfileEnabled = m_bTMagicProfileEnabled ? 0 : (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("SoundEventAsynchronously");
				if (it != pSetMap->end())
				{
					m_bTMagicEventAsynchronously = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("TMagicLiveUpdate");
				if (it != pSetMap->end()) {
					m_bTMagicLiveUpdate = StringConverter::parseBool(it->second, m_bTMagicLiveUpdate);
				}
				it = pSetMap->find("BellProfileEnable");
				if (it != pSetMap->end())
				{
					m_bBellProfileEnabled = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("SoundProfileEnable");
				if (it != pSetMap->end())
				{
					m_bSoundProfileEnabled = m_bBellProfileEnabled ? 0 : (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("BellEventAsynchronously");
				if (it != pSetMap->end())
				{
					m_bBellEventAsynchronously = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("BellLiveUpdate");
				if (it != pSetMap->end()) {
					m_bBellLiveUpdate = StringConverter::parseBool(it->second, m_bBellLiveUpdate);
				}
				it = pSetMap->find("ExpectFps");
				if (it != pSetMap->end())
				{
					uint32 fps = StringConverter::parseUnsignedInt(it->second);

					DynamicVideoParamManager::instance()->setInitExpectFps(fps);
					setExpectFps(fps);

				}
				it = pSetMap->find("MetalShaderCache");
				if (it != pSetMap->end())
				{
					m_bMetalShaderCacheEnable = (StringConverter::parseUnsignedInt(it->second) != 0);
				}

				it = pSetMap->find("D3DXShaderCache");
				if (it != pSetMap->end())
				{
					m_bD3DXShaderCacheEnable = (StringConverter::parseUnsignedInt(it->second) != 0);
				}

				it = pSetMap->find("SoundSysResCacheCount");
				if (it != pSetMap->end())
				{
					m_bSoundSysResCacheCount = StringConverter::parseUnsignedInt(it->second);
				}
				it = pSetMap->find("FFTOcean");
				if (it != pSetMap->end())
				{
					m_bFFTOcean = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("ClusterLightLevel");
				if (it != pSetMap->end())
				{
					int light_level = StringConverter::parseUnsignedInt(it->second);
					ClusteForwardLighting::Level new_level;
					switch (light_level)
					{
					case 0:
						new_level = ClusteForwardLighting::CFL_Off;
						break;
					case 1:
						new_level = ClusteForwardLighting::CFL_Low;
						break;
					case 2:
						new_level = ClusteForwardLighting::CFL_Middle;
						break;
					case 3:
						new_level = ClusteForwardLighting::CFL_High;
						break;
					default:
						new_level = ClusteForwardLighting::CFL_Low;
					};
					m_bEnableClusterForwardLight = (light_level != 0);
					m_clusterLightLevel = new_level;
				}

				it = pSetMap->find("EnableSphericalVoronoiRegion");
				if (it != pSetMap->end())
				{
					m_useSphericalVoronoiRegion = StringConverter::parseBool(it->second);
				}

				it = pSetMap->find("EnablePlanetRegionVisual");
				if (it != pSetMap->end())
				{
					m_enablePlanetRegionVisual = StringConverter::parseBool(it->second);
				}

				it = pSetMap->find("forceFirstPersonNearClipDistance");
				if (it != pSetMap->end())
				{
					m_forceFirstPersonNearClipDistance =
						StringConverter::parseFloat(it->second,
							DEFAULT_FORCE_FIRST_PERSON_NEAR_CLIP_DISTANCE);
				}

				it = pSetMap->find("forceSceneNearClipDistance");
				if (it != pSetMap->end())
				{
					m_forceSceneNearClipDistance =
						StringConverter::parseFloat(it->second,
							DEFAULT_FORCE_SCENE_NEAR_CLIP_DISTANCE);
				}

				it = pSetMap->find("EnableReverseDepth");
				if (it != pSetMap->end())
				{
					m_enableReverseDepth = StringConverter::parseBool(it->second);
				}

				it = pSetMap->find("Enable3DUIBatchRender");
				if (it != pSetMap->end())
				{
					UIRenderSystem* uiSys = UIRenderSystem::instance();
					if (uiSys)
					{
						bool enable = StringConverter::parseBool(it->second);
						uiSys->setBatchRenderingEnable(enable);
					}
				}

				it = pSetMap->find("EnableNewDecalBatchRender");
				if (it != pSetMap->end())
				{
					m_bEnableNewDecalBatchRender = StringConverter::parseBool(it->second);
				}

				it = pSetMap->find("Force3DUIBatchUseUniform");
				if (it != pSetMap->end())
				{
					UIRenderSystem* uiSys = UIRenderSystem::instance();
					if (uiSys)
					{
						bool enable = StringConverter::parseBool(it->second);
						uiSys->m_force3DUIBatchUseUniform = enable;
					}
				}

				it = pSetMap->find("Enable3DUINewUpdateStyle");
				if (it != pSetMap->end())
				{
					UIRenderSystem* uiSys = UIRenderSystem::instance();
					if (uiSys)
					{
						bool enable = StringConverter::parseBool(it->second);
						uiSys->m_useNewUpdateStyle = enable;
					}
				}

				it = pSetMap->find("Enable32BitDepth");
				if (it != pSetMap->end())
				{
					m_enable32BitDepth = StringConverter::parseBool(it->second);
				}

				it = pSetMap->find("EnableGLDepthZeroToOne");
				if (it != pSetMap->end())
				{
					m_enableGLDepthZeroToOne = StringConverter::parseBool(it->second);
				}

				it = pSetMap->find("ParticleAtlasHash");
				if (it != pSetMap->end())
				{
					m_bParticleAtlasHashVerify = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("UseNetAsynIO");
				if (it != pSetMap->end())
				{
					m_bUseNetAsynIO = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("UseIOCP");
				if (it != pSetMap->end())
				{
					m_bUseIOCP = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("SoundUseIOCP");
				if (it != pSetMap->end())
				{
					m_bSoundUseIOCP = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("UseIK");
				if (it != pSetMap->end())
				{
					m_bUseIK = StringConverter::parseUnsignedInt(it->second);
				}
				it = pSetMap->find("UseOAE");
				if (it != pSetMap->end())
				{
					m_bUseOAE = StringConverter::parseUnsignedInt(it->second, 0);
				}
				PCG::GenObjectScheduler::ImportConfig(pSetMap);
				it = pSetMap->find("UseRagdoll");
				if (it != pSetMap->end())
				{
					m_bUseRagdoll = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("UseOcclusionPlane");
				if (it != pSetMap->end())
				{
					OcclusionPlaneManager::instance()->setEnable(StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("UseNewMountain");
				if (it != pSetMap->end())
				{
					m_bUseNewMountain = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("Profiler");
				if (it != pSetMap->end())
				{
					EchoProfilerManager::instance()->ProfilerOpen(StringConverter::parseBool(it->second));
				}
				it = pSetMap->find("Trace");
				if (it != pSetMap->end())
				{
					EchoTrace::EchoTraceManager::instance()->SetEchoTraceActive(StringConverter::parseBool(it->second));
				}
				it = pSetMap->find("ProtocolComFrameTimeInMilliSec");
				if (it != pSetMap->end())
				{
					m_bProtocolComFrameTimeInMilliSec = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("EnableActorPaged");
				if (it != pSetMap->end())
				{
					m_bEnableActorPaged = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("ShowRenderthreadInfo");
				if (it != pSetMap->end())
				{
					m_bShowRenderThreadInfo = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("UseValidationLayer");
				if (it != pSetMap->end())
				{
					m_bValidate = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("GenerateShaderDebugInfo");
				if (it != pSetMap->end())
				{
					m_bShaderDebugInfo = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("KeepGPUResourceDebugLabel");
				if (it != pSetMap->end())
				{
					m_bGPUResourceLabel = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("UseGpuPool");
				if (it != pSetMap->end())
				{
					m_bUseGpuPool = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("AsyncReleaseActor");
				if (it != pSetMap->end())
				{
					m_bAsyncReleaseActor = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("DefaultActorSync");
				if (it != pSetMap->end())
				{
					m_bDefaultActorSync = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("CreateBufferInResourceThread");
				if (it != pSetMap->end())
				{
					m_bCreateBufferInResourceThread = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("MaxBindBoneCnt");
				if (it != pSetMap->end())
				{
					m_maxBindBoneCnt = StringConverter::parseUnsignedInt(it->second);
				}
				it = pSetMap->find("TerrainAsync");
				if (it != pSetMap->end())
				{
					m_bTerrainAsync = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("UseTextureStreaming");
				if (it != pSetMap->end())
				{
					m_bUseTextureStreaming = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("VegSyncTask");
				if (it != pSetMap->end())
				{
					m_bVegSyncTaskEnable = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("VegPreZ");
				if (it != pSetMap->end())
				{
					BiomeVegOpt::SetPreZEnabled(StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("VegDynamicBatch");
				if (it != pSetMap->end())
				{
					BiomeVegOpt::SetDyBatchEnabled(StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("VegDyBatchLimit");
				if (it != pSetMap->end())
				{
					BiomeVegOpt::SetDyBatchLimit(StringConverter::parseUnsignedInt(it->second));
				}
				it = pSetMap->find("VegGeoFac");
				if (it != pSetMap->end())
				{
					BiomeVegOpt::SetGeoFactorEnabled(StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("VegOrthoBox");
				if (it != pSetMap->end())
				{
					BiomeVegOpt::SetOrthoBoxEnabled(StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("UseActorBuildingSimpleShadow");
				if (it != pSetMap->end())
				{
					m_bUseActorBuildingSimpleShadow = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("PCCustomShadow");
				if (it != pSetMap->end())
				{
					m_bPCCustomShadow = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("RenderStrategy");
				if (it != pSetMap->end())
				{
					m_renderStrategyType = StringConverter::parseUnsignedInt(it->second);
				}
				it = pSetMap->find("UseGeneralRS");
				if (it != pSetMap->end())
				{
					m_bUseGeneralRenderStrategy = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("EnableVisibilityBuffer");
				m_bEnableVisibilityBuffer = false;
				if (m_renderStrategyType == 1 && it != pSetMap->end())
				{
					m_bEnableVisibilityBuffer = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("EnableShaderDebugFlag");
				m_bEnableShaderDebugFlag = false;
				if (it != pSetMap->end())
				{
					m_bEnableShaderDebugFlag = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("EnableMatV2CompatibilityMode");
				if (it != pSetMap->end())
				{
					m_bEnableMatV2CompatibilityMode = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("DebugPassPost");
				if (it != pSetMap->end())
				{
					m_bEnableDebugPassPost = StringConverter::parseBool(it->second);
				}
				it = pSetMap->find("RoomPortalCull");
				if (it != pSetMap->end())
				{
					m_bEnableRoomPortalCull = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("RoadMeshModel");
				if (it != pSetMap->end())
				{
					UsePavementTestNewMode = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("RemoteGatherInfo");
				if (it != pSetMap->end())
				{
					m_useRemoteGatherInfo = (StringConverter::parseUnsignedInt(it->second) != 0);
				}
				it = pSetMap->find("UseMDLodMesh");
				if (it != pSetMap->end())
				{
					m_LodMeshType = StringConverter::parseUnsignedInt(it->second);
				}
				it = pSetMap->find("DelayUpdateLodLevel");
				if (it != pSetMap->end())
				{
					uint8 level = StringConverter::parseUnsignedInt(it->second);
					RoleMovable::setDelayUpdateLodLevel(level);
				}
				it = pSetMap->find("SphOceanAsync");
				if (it != pSetMap->end())
				{
					int i = StringConverter::parseUnsignedInt(it->second, 0);
					if (i == 0)
					{
						SphericalOcean::s_asyncGenerateVertice = false;
					}
					else
					{
						SphericalOcean::s_asyncGenerateVertice = true;
					}
				}
				else {
					SphericalOcean::s_asyncGenerateVertice = true;
				}
			}
		}
	}

	const String& Root::getRootResourcePath(const String& resType)
	{
		EchoZoneScoped;

		if (resType.empty())
			return ResourceFolderManager::instance()->GetDefaultRootPath();
		return ResourceFolderManager::instance()->GetResRootPath(Echo::Name(resType));
	}

	void Root::onOpenFloatExceptionCheck()
	{
		EchoZoneScoped;

#ifdef _WIN32
		if (m_checkFloatExceptionMask > 0)
		{
			//int cw = _controlfp(0, 0);
			//cw &= ~(EM_OVERFLOW | EM_UNDERFLOW | EM_ZERODIVIDE | EM_DENORMAL | EM_INVALID);
			//_controlfp(cw, MCW_EM);

			uint32 _mask = 0;

			if (m_checkFloatExceptionMask & EM_INEXACT)
				_mask |= EM_INEXACT;
			if (m_checkFloatExceptionMask & EM_UNDERFLOW)
				_mask |= EM_UNDERFLOW;
			if (m_checkFloatExceptionMask & EM_OVERFLOW)
				_mask |= EM_OVERFLOW;
			if (m_checkFloatExceptionMask & EM_ZERODIVIDE)
				_mask |= EM_ZERODIVIDE;
			if (m_checkFloatExceptionMask & EM_INVALID)
				_mask |= EM_INVALID;
			if (m_checkFloatExceptionMask & EM_DENORMAL)
				_mask |= EM_DENORMAL;


			unsigned int control_word = 0;
			int err = _controlfp_s(&control_word, 0, 0);
			(void)err;
			unsigned int cw = control_word & (~(_mask));
			_controlfp_s(&control_word, cw, MCW_EM);
		}
#endif
	}

	void Root::onUnderClocking(int type)
	{
		EchoZoneScoped;


	}

	//-----------------------------------------------------------------------
	RenderSystem* Root::getRenderSystem(void)
	{
		EchoZoneScoped;

		// Gets the currently active renderer
		return m_pRenderSystem;
	}
	//-----------------------------------------------------------------------------
	SceneManager* Root::createSceneManager(const Name& instanceName)
	{
		EchoZoneScoped;

		// Check name not used
		if (mSceneManagerMap.find(instanceName) != mSceneManagerMap.end())
		{
			ECHO_EXCEPT(
				Exception::ERR_DUPLICATE_ITEM,
				"A camera with the name " + instanceName.toString() + " already exists",
				"SceneManager::createSceneManager");
		}

		SceneManager* sm = new SceneManager(instanceName);
		sm->_setDestinationRenderSystem(m_pRenderSystem);
		mSceneManagerMap.insert(SceneManagerMap::value_type(instanceName, sm));

		if (m_enable3DUISceneQueryCullTick)
		{
			sm->addListener(efgui::FGUITickListener::instance());
		}
		else
		{
			//TODO(yanghang): Find nicer place to put the code
			// Just want to rendering once. So just add to main scene-manager.
			{
				if (instanceName == g_MainSceneManagerName)
				{
					//auto uiRenderManager = UIRenderManager::instance();
					//assert(uiRenderManager && "Forget to new UIRenderManager!");
					//sm->addListener(uiRenderManager);

					DebugDrawManager* debugDrawMgr = DebugDrawManager::instance();
					assert(debugDrawMgr && "Forget to new DebugDrawManager!");
					sm->addListener(debugDrawMgr);
				}
			}
		}
		return sm;
	}
	//-----------------------------------------------------------------------------
	void Root::destroySceneManager(SceneManager* sm)
	{
		EchoZoneScoped;

		if (sm == getActiveSceneManager())
		{
			setActiveSceneManager(nullptr);
		}

		SceneManagerMap::iterator ite = mSceneManagerMap.find(sm->getName());
		if (ite != mSceneManagerMap.end())
		{
			delete sm;
			mSceneManagerMap.erase(ite);
		}
	}
	//-----------------------------------------------------------------------------
	SceneManager* Root::getSceneManager(const Name& instanceName) const
	{
		EchoZoneScoped;

		SceneManagerMap::const_iterator ite = mSceneManagerMap.find(instanceName);
		if (ite != mSceneManagerMap.end())
		{
			return ite->second;
		}
		return NULL;
	}

	//-----------------------------------------------------------------------------
	bool Root::hasSceneManager(const Name& instanceName) const
	{
		EchoZoneScoped;

		SceneManagerMap::const_iterator iter = mSceneManagerMap.find(instanceName);
		if (iter == mSceneManagerMap.end())
		{
			return false;
		}
		return true;
	}



	Echo::SceneManager* Root::getMainSceneManager()
	{
		EchoZoneScoped;

		return getSceneManager(g_MainSceneManagerName);
	}

	Echo::SceneManager* Root::getUISceneManager()
	{
		EchoZoneScoped;

		return getSceneManager(g_UISceneManagerName);
	}

	//=============================================================================
	void Root::addFrameListener(FrameListener* listener)
	{
		EchoZoneScoped;

		if (std::find(mFrameListeners.begin(), mFrameListeners.end(), listener) == mFrameListeners.end() ||
			std::find(mRemovedFrameListeners.begin(), mRemovedFrameListeners.end(), listener) != mRemovedFrameListeners.end())
		{
			mAddedFrameListeners.push_back(listener);
			listener->m_bListenerRegisted = true;
		}
	}

	//=============================================================================
	void Root::removeFrameListener(FrameListener* listener)
	{
		EchoZoneScoped;

		if (std::find(mFrameListeners.begin(), mFrameListeners.end(), listener) != mFrameListeners.end())
		{
			mRemovedFrameListeners.push_back(listener);
			listener->m_bListenerRegisted = false;
			//return;
		}

		//在待添加队列中删除listen
		std::vector<FrameListener*>::iterator itFind = std::find(mAddedFrameListeners.begin(), mAddedFrameListeners.end(), listener);
		if (itFind != mAddedFrameListeners.end())
		{
			mAddedFrameListeners.erase(itFind);
			listener->m_bListenerRegisted = false;
		}

	}

	void Root::changeWorldOrigin(const DVector3& origin)
	{
		mNewWorldOrigin = origin;
	}

	DVector3 Root::getWorldOrigin() const
	{
		return mNewWorldOrigin;
	}

	DVector3 Root::toWorld(const DVector3& position) const {
		return position + mNewWorldOrigin;
	}
	DVector3 Root::toLocal(const DVector3& position) const {
		return position - mNewWorldOrigin;
	}

	void Root::addWorldOriginListener(WorldOriginListener* listener)
	{
		mWorldOriginListeners.push_back(listener);
	}

	void Root::removeWorldOriginListener(WorldOriginListener* listener)
	{
		auto it = std::remove_if(mWorldOriginListeners.begin(), mWorldOriginListeners.end(),
			[&listener](const WorldOriginListener* rhs) {return listener == rhs; });
		mWorldOriginListeners.erase(it);
	}

	//-----------------------------------------------------------------------
	bool Root::_fireFrameStarted(FrameEvent& evt)
	{
		EchoZoneScoped;

#ifdef PRINT_TIME_INFO
		uint64 frameStartTime0 = mTimer->getMicroseconds();
#endif
		// 先把要移除的Listener移除掉
		for (size_t i = 0; i < mRemovedFrameListeners.size(); ++i)
		{
			std::vector<FrameListener*>::iterator itFind = std::find(mFrameListeners.begin(), mFrameListeners.end(), mRemovedFrameListeners[i]);
			if (itFind != mFrameListeners.end())
				mFrameListeners.erase(itFind);
		}
		mRemovedFrameListeners.clear();

		// 真正地添加listener
		for (size_t i = 0; i < mAddedFrameListeners.size(); ++i)
		{
			std::vector<FrameListener*>::const_iterator itFind = std::find(mFrameListeners.begin(), mFrameListeners.end(), mAddedFrameListeners[i]);
			if (itFind == mFrameListeners.end())
				mFrameListeners.push_back(mAddedFrameListeners[i]);
		}
		mAddedFrameListeners.clear();

		m_pRenderSystem->destroyRcRenderPipelines();
		m_pRenderSystem->destoryRcTextures();
		m_pRenderSystem->destoryRcBuffers();

#ifdef PRINT_TIME_INFO
		uint64 frameStartTime1 = mTimer->getMicroseconds();
#endif
		//{
//	tthread::lock_guard<tthread::recursive_mutex> lock(mLock);
//	auto it = mDirtyNode.begin();
//	for (; it != mDirtyNode.end(); ++it)
//	{
//		//if (it->second)
//		//{
//		//	it->first->_updateAxisBox();
//		//}
//		(*it)->_update(true, false);
//		(*it)->setDirty(false);
//	}
//	mDirtyNode.clear();
		//}

		OceanTickJobManager::instance()->Synchro();

		for (size_t i = 0; i < mFrameListeners.size(); ++i)
		{
			mFrameListeners[i]->frameStarted(evt);
		}
		for (size_t i = 0; i < mFrameListeners.size(); ++i)
		{
			mFrameListeners[i]->onUpdate();
		}
#ifdef PRINT_TIME_INFO
		uint64 frameStartTime2 = mTimer->getMicroseconds();
#endif
		BuildLodMeshManager::instance()->Synchro();
		CommonJobManager::instance()->Synchro();
#ifdef PRINT_TIME_INFO
		uint64 frameStartTime3 = mTimer->getMicroseconds();
#endif
		SkeletonSyncManager::instance()->SynchroSkeletonInit();
		SkeletonSyncManager::instance()->PrepareSkeletonInitJob();

		EffectTickJobManager::instance()->SynchroParticleInit();
		EffectTickJobManager::instance()->PrepareParticleInit();
#ifdef PRINT_TIME_INFO
		uint64 frameStartTime4 = mTimer->getMicroseconds();
#endif
		SkeletonSyncManager::instance()->PrepareSkeletonModifyJob();
#ifdef PRINT_TIME_INFO
		uint64 frameStartTime5 = mTimer->getMicroseconds();
#endif
		//resetRoleMovableState();


#ifdef PRINT_TIME_INFO
		uint64 frameStartTime6 = mTimer->getMicroseconds();
#endif
		//{
		//	tthread::lock_guard<tthread::recursive_mutex> lock(mLock);
		//	auto it = mBoundDirtyNode.begin();
		//	for (; it != mBoundDirtyNode.end(); ++it)
		//	{
		//		if (it->second)
		//		{
		//			it->first->_updateAxisBox();
		//		}
		//		it->first->_updateBounds();
		//	}
		//	mBoundDirtyNode.clear();
		//}
		if (mIsParticleTickAsync)
		{
			EffectTickJobManager::instance()->PrepareJob();
		}
		//if (mIsParticleTickAsync)
		//{
		//	EffectTickJobManager::instance()->Synchro();
		//}
#ifdef PRINT_TIME_INFO
		uint64 frameStartTime7 = mTimer->getMicroseconds();


		uint64 time = frameStartTime7 - frameStartTime0;

		if (time > 3000)
		{
			char msg[256] = { 0 };
			sprintf(msg, "11111,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld", time, frameStartTime1 - frameStartTime0, frameStartTime2 - frameStartTime1, \
				frameStartTime3 - frameStartTime2, frameStartTime4 - frameStartTime3, frameStartTime5 - frameStartTime4, frameStartTime6 - frameStartTime5, frameStartTime7 - frameStartTime6);
			LogManager::instance()->logMessage(msg, LML_CRITICAL);
		}
#endif
		return true;
	}

	//-----------------------------------------------------------------------
	bool Root::_fireFrameEnded(FrameEvent& evt)
	{
		EchoZoneScoped;

#ifdef PRINT_TIME_INFO
		uint64 frameEndTime0 = mTimer->getMicroseconds();
#endif

		SystemSynchro();

#ifdef PRINT_TIME_INFO
		uint64 frameEndTime1 = mTimer->getMicroseconds();
#endif

		// 先把要移除的Listener移除掉
		for (size_t i = 0; i < mRemovedFrameListeners.size(); ++i)
		{
			std::vector<FrameListener*>::const_iterator itFind = std::find(mFrameListeners.begin(), mFrameListeners.end(), mRemovedFrameListeners[i]);
			if (itFind != mFrameListeners.end())
				mFrameListeners.erase(itFind);
		}
		mRemovedFrameListeners.clear();
#ifdef PRINT_TIME_INFO
		uint64 frameEndTime2 = mTimer->getMicroseconds();
#endif
		for (size_t i = 0; i < mFrameListeners.size(); ++i)
		{
			mFrameListeners[i]->frameEnded(evt);
		}
#ifdef PRINT_TIME_INFO
		uint64 frameEndTime3 = mTimer->getMicroseconds();
#endif
		// Tell the queue to process responses
		mWorkQueue->processResponses();
#ifdef PRINT_TIME_INFO
		uint64 frameEndTime4 = mTimer->getMicroseconds();
#endif
		//ActorManager::instance()->frameEndPhase();

		m_pResourceManagerFactory->destoryUnreferencedResource();

		//if (mIsParticleTickAsync)
		//{
		//	EffectTickJobManager::instance()->PrepareJob();
		//}
#ifdef PRINT_TIME_INFO
		uint64 frameEndTime5 = mTimer->getMicroseconds();


		uint64 time = frameEndTime5 - frameEndTime0;

		if (time > 3000)
		{
			char msg[256] = { 0 };
			sprintf(msg, "22222,%lld,%lld,%lld,%lld,%lld,%lld", time, frameEndTime1 - frameEndTime0, frameEndTime2 - frameEndTime1, \
				frameEndTime3 - frameEndTime2, frameEndTime4 - frameEndTime3, frameEndTime5 - frameEndTime4);
			LogManager::instance()->logMessage(msg, LML_CRITICAL);
		}
#endif

#ifdef _WIN32
		//if (m_FileMonitor)
		//{
		//	m_FileMonitor->updateMonitor();
		//}
#endif
		if (!m_bPreSyncSkeleton)
		{
			SynchroSkeleton();
		}
		m_bPreSyncSkeleton = false;



		OceanTickJobManager::instance()->PrepareJob();
		ProtocolComponentSystem::instance()->tick(TT_EndFrame);
		return true;
	}
	//-----------------------------------------------------------------------
	bool Root::_fireFrameStarted()
	{
		EchoZoneScoped;

		ProtocolComponentSystem::instance()->tick(TT_BeginFrame);
		FrameEvent evt;
		populateFrameEvent(FETT_STARTED, evt);
		return _fireFrameStarted(evt);
	}
	//-----------------------------------------------------------------------
	bool Root::_fireFrameEnded()
	{
		EchoZoneScoped;

		FrameEvent evt;
		populateFrameEvent(FETT_ENDED, evt);

		mTimeSinceLastFrame = evt.timeSinceLastFrame;
		mTimeElapsed += mTimeSinceLastFrame;//evt.timeElapsed;


		uint32 time = (uint32)(mTimeSinceLastFrame * 1000);
		CalFps(time);

		if (getRuntimeMode() == eRM_Client)
		{
			BuildingLodManager::instance()->calBuildingLod();
			//DynamicVideoParamManager::instance()->updateTime(m_drawCallCount);
		}


		return _fireFrameEnded(evt);
	}
	//---------------------------------------------------------------------
	void Root::populateFrameEvent(FrameEventTimeType type, FrameEvent& evtToUpdate)
	{
		EchoZoneScoped;

		unsigned long now = mTimer->getMilliseconds();
		evtToUpdate.timeElapsed = now / 1000.f;
		evtToUpdate.timeSinceLastFrame = calculateEventTime(now, type);
	}

	//-----------------------------------------------------------------------
	float Root::calculateEventTime(unsigned long now, FrameEventTimeType type)
	{
		EchoZoneScoped;

		// Calculate the average time passed between events of the given type
		// during the last mFrameSmoothingTime seconds.

		EventTimesQueue& times = mEventTimes[type];
		times.push_back(now);

		if (times.size() == 1)
			return 0;

		// Times up to mFrameSmoothingTime seconds old should be kept
		unsigned long discardThreshold =
			static_cast<unsigned long>(mFrameSmoothingTime * 1000.0f);

		// Find the oldest time to keep
		EventTimesQueue::iterator it = times.begin(),
			end = times.end() - 2; // We need at least two times
		while (it != end)
		{
			if (now - *it > discardThreshold)
				++it;
			else
				break;
		}

		// Remove old times
		times.erase(times.begin(), it);

		return float(times.back() - times.front()) / ((times.size() - 1) * 1000);
	}
	//-----------------------------------------------------------------------
	void Root::startRendering(void)
	{
		EchoZoneScoped;

		assert(m_pRenderSystem != 0);

		// Clear event times
		clearEventTimes();

		// Infinite loop, until broken out of by frame listeners
		// or break out by calling queueEndRendering()
		mQueuedEnd = false;

		while (!mQueuedEnd)
		{
			//Pump messages in all registered RenderWindow windows
			WindowEventUtilities::messagePump();

			if (!renderOneFrame())
				break;
		}
	}

	void Root::preRenderOneFrame(void)
	{
		EchoZoneScoped;
		ProfilerFrameData* currentFrameData = ProfilerInfoManager::instance()->GetCurrentFrameData();

		if (m_bUseRenderCommand)
		{
			std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> beginCommandStart = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock().now());
			m_pRenderer->beginCommand();
			std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> beginCommandEnd = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock().now());
			std::chrono::duration<long long, std::nano> result = beginCommandEnd - beginCommandStart;
			RenderSystem::m_uniformBufferTotalSize = 0;

			m_bPreRenderOneFrame = true;

			currentFrameData->renderOneFrameStatus.preRenderOneFramePreBeginCommandTimePoint = beginCommandStart.time_since_epoch().count();
			currentFrameData->renderOneFrameStatus.preRenderOneFramePostBeginCommandTimePoint = beginCommandEnd.time_since_epoch().count();
			currentFrameData->renderOneFrameStatus.preRenderOneFrameBeginCommandDuration = result.count();

		}

		SynchroSkeleton();
		m_bPreSyncSkeleton = true;
	}

	//-----------------------------------------------------------------------
	bool Root::renderOneFrame(void)
	{
		EchoZoneScoped;
		auto startUpdateTime = std::chrono::steady_clock::now();


		ProfilerFrameData* currentFrameData = ProfilerInfoManager::instance()->GetCurrentFrameData();
		currentFrameData->renderOneFrameStatus.renderOneFrameStartTimePoint = time_point_cast<std::chrono::nanoseconds>(startUpdateTime).time_since_epoch().count();

		{
			auto profileMgr = ProfilerInfoManager::instance();
			auto& frameData = profileMgr->GetCurrentFrameData()->profilerInfo;
			uint64 wallTime = getWallTime();
			frameData.frameBeginTime = wallTime;
		}

		if (m_bUseRenderCommand)
		{
			if (!m_bPreRenderOneFrame)
			{
				m_pRenderer->beginCommand();
				RenderSystem::m_uniformBufferTotalSize = 0;
			}
		}
		std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> finishedBeginCommandTimePoint = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock().now());
		currentFrameData->renderOneFrameStatus.finishedBeginCommandTimePoint = finishedBeginCommandTimePoint.time_since_epoch().count();

		//std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> startFireFrameStartTimePoint = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock().now());

		if (!_fireFrameStarted())
			return false;

		//RcContext* ctx = getRcContext();
		//mRcInfo = ctx->pDebugInfoStr;


		ProtocolComponentSystem::instance()->tick(TT_PreUpdateScene);
		ProtocolComponentSystem::instance()->tick(TT_PreUpdateAnimation);

		auto finishedProtocolTickTime = std::chrono::steady_clock::now();
		std::chrono::nanoseconds diffTime0 = finishedProtocolTickTime - startUpdateTime;
		mFrameStartedTime = (uint32)(diffTime0.count() / 1000);

		currentFrameData->renderOneFrameStatus.finishedProtocolTickTimePoint = time_point_cast<std::chrono::nanoseconds>(finishedProtocolTickTime).time_since_epoch().count();


		m_triangleCount = 0;
		m_drawCallCount = 0;
		m_batchDrawCallCount = 0;
		m_particleDrawCallCount = 0;
		m_particleBillBoardCount = 0;
		m_dwSceneNodeCount = 0;
		m_dwSceneNodeVisibleCount = 0;
		m_dwMovableCount = 0;
		m_dwMovableVisibleCount = 0;
		m_dwMovableNoCulledCount = 0;
		m_roleAnimationCnt = 0;
		BiomeVegOpt::StatisticsClear();

		m_objectTriangleCount = 0;
		m_shadowTriangleCount = 0;
		m_UITriangleCount = 0;
		m_roleLodNum.clear();
		m_roleLodNum.resize(ROLE_LOD_LEVEL_CNT);
		//m_particleRoleCount = 0;

		if (!m_enable3DUISceneQueryCullTick)
		{
			//IMPORTANT(yanghang): Update transform of 3D UI system.
			{
				UIRenderSystem::instance()->update();
			}

			//IMPORTANT(yanghang):Commit debug draw data. must finish push debug draw stuff before this.
			{
				DebugDrawManager::instance()->commitDataToGPUBuffer();
			}
		}

		//TODO(yanghang):Check whether we need this code that seemed useless.
		{
			ControllerManager::instance()->updateAllControllers();
		}

		//IMPORTANT(yanghang):Heavy code sequence dependent.
		// Must be call before SHADOW_CAMERA calculation and RenderTarget::Update process.
		// Render target skip frame update startegy.
		{
			RenderTargetMap::iterator itor = m_pRenderSystem->m_renderTargets.begin();
			RenderTargetMap::iterator end = m_pRenderSystem->m_renderTargets.end();

			for (; itor != end; ++itor)
			{
				RenderTarget* pRenderTarget = itor->second;
				if (pRenderTarget->isAutoUpdate() && pRenderTarget->isActive())
				{
					pRenderTarget->freshSkipFrameState();
				}
			}
		}

		//NOTE:Scene manager level update.
		{
			static float lastUpdateTime;
			float newtime = getTimeElapsed();
			float dttime = newtime - lastUpdateTime;

			std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> updateSceneManagerStart = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock().now());
			currentFrameData->renderOneFrameStatus.beginUpdateSceneManagerTimePoint = updateSceneManagerStart.time_since_epoch().count();

			for (auto mapIte : mSceneManagerMap)
			{
				SceneManager* sceneManager = mapIte.second;


				//IMPORTANT(yanghang):Update once shadow map of SceneManager calculation parameters in one frame.
				{
					ShadowmapManager* PsmShadowManager = sceneManager->getPsmshadowManager();
					//计算阴影相机参数
					if (PsmShadowManager->getShadowEnable())
					{
						Vector3 dir = sceneManager->getMainLightShadowDirection();
						//TODO(yanghang):Remove, debug.
						//dir = Vector3::UNIT_SCALE;
						PsmShadowManager->updateShadowCamera(dir);
					}

					PointLightShadowManager* plsMgr = sceneManager->getPointShadowManager();
					plsMgr->UpdateCamera(sceneManager->getGravityDir());

					RainMaskManager* rainShadowMananger = sceneManager->getRainMaskManager();
					if (rainShadowMananger->getUpdateState())
					{
						Vector3 rainDir = sceneManager->getGravityDir();
						rainDir *= -1.0;
						rainShadowMananger->UpdateCamera(rainDir);
					}
				}

				//IMPORTANT(yanghang):Update scene graph.
				{
					SceneNode* rootNode = sceneManager->getRootSceneNode();
					rootNode->_update(true);
				}

				//IMPORTANT(yanghang):Particle update
				//TODO(yanghang): Updating the particles that have be seen in camera of previous frame.
				// and what is worse, the particles has beed rendered of previous frame.
				// Check that whether the delay is reasonable or not ?
				{
					if (!mIsParticleTickAsync)
					{
						EffectManager* particleMgr = sceneManager->getEffectManager();
						particleMgr->tick(dttime);
					}
				}
			}//end for
			std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> updateSceneManagerEnd = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock().now());
			std::chrono::duration<long long, std::nano> updateSceneManagerResult = updateSceneManagerEnd - updateSceneManagerStart;
			lastUpdateTime = newtime;
			currentFrameData->renderOneFrameStatus.updateSceneManagerTimeDuration = updateSceneManagerResult.count();
			currentFrameData->renderOneFrameStatus.finishedUpdateSceneManagerTimePoint = updateSceneManagerEnd.time_since_epoch().count();

		}

		//TODO(yanghang):Here isn't best place to waiting skeleton modify job complete.
		// We only use the new skeleton data before commit rendering data of skeleton's mesh.
		//{
		//	SkeletonSyncManager::instance()->SynchroSkeletonModify();
		//}


		std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> new_updateAllRenderTargetStart = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock().now());
		currentFrameData->renderOneFrameStatus.newUpdateAllRenderTargetStartTimePoint = new_updateAllRenderTargetStart.time_since_epoch().count();

		m_pRenderSystem->_updateAllRenderTargets();

		std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> new_updateAllRenderTargetEnd = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock().now());
		currentFrameData->renderOneFrameStatus.newUpdateAllRenderTargetEndTimePoint = new_updateAllRenderTargetEnd.time_since_epoch().count();
		std::chrono::duration<long long, std::nano> new_updateAllRenderTargetResult = new_updateAllRenderTargetEnd - new_updateAllRenderTargetStart;
		currentFrameData->renderOneFrameStatus.newUpdateAllRenderTargetDuration = new_updateAllRenderTargetResult.count();

		//TODO(yanghang):Here isn't best place to post asynchronous skeleton calculate job.
		// We already know what skeletons need to calculate.
		//{
		//	SkeletonSyncManager::instance()->PrepareJob();
		//}

		if (m_enable3DUISceneQueryCullTick)
		{
			//IMPORTANT(yanghang):Only need delay free 3DUI renderable object.
			UIRenderSystem::instance()->delayFreeUIRenderable();
		}
		else
		{
			//IMPORTANT(yanghang):Delay free renderable after render system has used them.
			// and clean render UI item list each frame.
			{
				UIRenderSystem::instance()->cleanFrameState();
			}
			//IMPORTANT(yanghang):Clear debug draw frame data.
			{
				DebugDrawManager::instance()->clearFrameCachedData();
			}
		}

		++mNextFrame;

		auto frameEndTime = std::chrono::steady_clock::now();

		bool ret = _fireFrameEnded();

		mTimeSinceLastFrameClient = mTimeSinceLastFrame;

		if (EchoProfilerManager::instance()->GetTotalTime() >= (milliseconds)m_bProtocolComFrameTimeInMilliSec)
			LogManager::instance()->logMessage(EchoProfilerManager::instance()->ToString());

		auto endUpdateTime = std::chrono::steady_clock::now();
		std::chrono::nanoseconds diffTime1 = endUpdateTime - frameEndTime;
		mFrameEndTime = (uint32)(diffTime1.count() / 1000);

		std::chrono::nanoseconds diffTime2 = endUpdateTime - startUpdateTime;
		mSceneUpdateTime = (uint32)(diffTime2.count() / 1000);
		//currentFrameData->renderOneFrameStatus. = mSceneUpdateTime;
		currentFrameData->renderOneFrameStatus.fireFrameEndedBeginTimePoint = time_point_cast<std::chrono::nanoseconds>(frameEndTime).time_since_epoch().count();
		currentFrameData->renderOneFrameStatus.fireFrameEndedEndTimePoint = time_point_cast<std::chrono::nanoseconds>(endUpdateTime).time_since_epoch().count();

		if (m_bUseRenderCommand)
		{
			if (!m_bPreRenderOneFrame)
			{
				m_pRenderer->endCommand();

				auto endRenderTime = std::chrono::steady_clock::now();
				std::chrono::nanoseconds diffTime3 = endRenderTime - endUpdateTime;
				mWaitRenderTime = (uint32)(diffTime3.count() / 1000);

				if (ECHO_THREAD_MT0 == m_threadMode)
				{
					mWaitUpdateTime = m_pRenderer->getWaitworkTime();
					mSwapBufferTime = m_pRenderer->getSwapBufferTime();
					mRenderMainTime = m_pRenderer->getCommitTime() - mSwapBufferTime;

					onHandleDynamicVideo(mRenderMainTime + mSwapBufferTime);
					onCalculateAvgTime(mRenderMainTime + mSwapBufferTime);
				}
			}
		}

		// Submit data to the profiler
		currentFrameData->renderOneFrameStatus.waitRenderTime = mWaitRenderTime;
		currentFrameData->renderOneFrameStatus.renderMainTime = mWaitUpdateTime;
		currentFrameData->renderOneFrameStatus.waitUpdateTime = mWaitUpdateTime;
		// Last Function To Call in this function
		high_res_time_point renderOneFrameEndTime = time_point_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now());
		currentFrameData->renderOneFrameStatus.renderOneFrameEndTimePoint = renderOneFrameEndTime.time_since_epoch().count();
		currentFrameData->renderOneFrameStatus.renderOneFrameDuration = currentFrameData->renderOneFrameStatus.renderOneFrameEndTimePoint
			- currentFrameData->renderOneFrameStatus.renderOneFrameStartTimePoint;

		if (mNewWorldOrigin != mWorldOrigin)
		{
			for (const auto& listener : mWorldOriginListeners) {
				listener->worldOriginRebasing(mNewWorldOrigin.x, mNewWorldOrigin.y, mNewWorldOrigin.z);
			}
			mWorldOrigin = mNewWorldOrigin;
		}

		return ret;
	}

	//---------------------------------------------------------------------
	bool Root::renderOneFrame(float timeSinceLastFrame)
	{
		EchoZoneScoped;
		auto startUpdateTime = std::chrono::steady_clock::now();
		ProfilerFrameData* currentFrameData = ProfilerInfoManager::instance()->GetCurrentFrameData();

		if (m_bUseRenderCommand)
		{
			if (!m_bPreRenderOneFrame)
			{
				std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> beginCommandStart = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock().now());
				m_pRenderer->beginCommand();
				std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> beginCommandEnd = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock().now());
				std::chrono::duration<long long, std::nano> beginCommandResult = beginCommandEnd - beginCommandStart;
				RenderSystem::m_uniformBufferTotalSize = 0;

				currentFrameData->renderOneFrameStatus.preBeginCommandTimePoint = beginCommandStart.time_since_epoch().count();
				currentFrameData->renderOneFrameStatus.postBeginCommandTimePoint = beginCommandEnd.time_since_epoch().count();
				currentFrameData->renderOneFrameStatus.beginCommandDuration = beginCommandResult.count();
			}
		}

		std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> beginComponentTickTime = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock().now());
		if (!_fireFrameStarted())
			return false;
		ProtocolComponentSystem::instance()->tick(TT_PreUpdateScene);
		ProtocolComponentSystem::instance()->tick(TT_PreUpdateAnimation);
		std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> endComponentTickTime = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock().now());

		auto frameStartedTime = std::chrono::steady_clock::now();
		std::chrono::nanoseconds diffTime0 = frameStartedTime - startUpdateTime;
		mFrameStartedTime = (uint32)(diffTime0.count() / 1000);
		m_triangleCount = 0;
		m_drawCallCount = 0;
		m_batchDrawCallCount = 0;
		m_particleDrawCallCount = 0;
		m_particleBillBoardCount = 0;
		m_roleAnimationCnt = 0;
		m_dwSceneNodeCount = 0;
		m_dwSceneNodeVisibleCount = 0;
		m_dwMovableCount = 0;
		m_dwMovableVisibleCount = 0;
		m_dwMovableNoCulledCount = 0;
		BiomeVegOpt::StatisticsClear();

		m_objectTriangleCount = 0;
		m_shadowTriangleCount = 0;
		m_UITriangleCount = 0;
		m_roleLodNum.clear();
		m_roleLodNum.resize(ROLE_LOD_LEVEL_CNT);
		//m_particleRoleCount = 0;

		mTimeSinceLastFrameClient = timeSinceLastFrame;

		if (!m_enable3DUISceneQueryCullTick)
		{
			//IMPORTANT(yanghang): Update transform of 3D UI system.
			{
				UIRenderSystem::instance()->update();
			}

			//IMPORTANT(yanghang):Commit debug draw data. must finish push debug draw stuff before this.
			{
				DebugDrawManager::instance()->commitDataToGPUBuffer();
			}
		}

		//TODO(yanghang):Check whether we need this code that seemd useless.
		{
			ControllerManager::instance()->updateAllControllers();
		}

		//IMPORTANT(yanghang):Heavy code sequence dependent.
		// Must be call before SHADOW_CAMERA calculation and RenderTarget::Update process.
		// Render target skip frame update startegy.
		{
			RenderTargetMap::iterator itor = m_pRenderSystem->m_renderTargets.begin();
			RenderTargetMap::iterator end = m_pRenderSystem->m_renderTargets.end();

			for (; itor != end; ++itor)
			{
				RenderTarget* pRenderTarget = itor->second;
				if (pRenderTarget->isAutoUpdate() && pRenderTarget->isActive())
				{
					pRenderTarget->freshSkipFrameState();
				}
			}
		}

		//NOTE:Scene manager level update.
		{
			static float lastUpdateTime;
			float newtime = getTimeElapsed();
			float dttime = newtime - lastUpdateTime;

			std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> updateSceneManagerStart = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock().now());
			for (auto mapIte : mSceneManagerMap)
			{
				SceneManager* sceneManager = mapIte.second;
				//IMPORTANT(yanghang):Update once shadow map of SceneManager calculation parameters in one frame.
				{
					//计算阴影相机参数
					sceneManager->updateShadowCamera();

					PointLightShadowManager* plsMgr = sceneManager->getPointShadowManager();
					plsMgr->UpdateCamera(sceneManager->getGravityDir());
				}

				RainMaskManager* rainShadowMananger = sceneManager->getRainMaskManager();
				if (rainShadowMananger->getUpdateState())
				{
					Vector3 rainDir = sceneManager->getGravityDir();
					rainDir *= -1.0;
					rainShadowMananger->UpdateCamera(rainDir);
				}

				//IMPORTANT(yanghang):Update scene graph.
				SceneNode* rootNode = sceneManager->getRootSceneNode();
				rootNode->_update(true);

				//IMPORTANT(yanghang):Particle update
				//TODO(yanghang): Updating the particles that have be seen in camera of previous frame.
				// and what is worse, the particles has beed rendered of previous frame.
				// Check whether the delay is reasonable or not ?
				{
					if (!mIsParticleTickAsync)
					{
						EffectManager* particleMgr = sceneManager->getEffectManager();
						particleMgr->tick(dttime);
					}
				}
			}//end for
			std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> updateSceneManagerEnd = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock().now());
			std::chrono::duration<long long, std::nano> updateSceneManagerResult = updateSceneManagerEnd - updateSceneManagerStart;
			lastUpdateTime = newtime;
			currentFrameData->renderOneFrameStatus.updateSceneManagerTimeDuration = updateSceneManagerResult.count();
		}

		//TODO(yanghang):Here isn't best place to waiting skeleton modify job complete.
		// We only use the new skeleton data before commit rendering data of skeleton's mesh.
		//{
		//	SkeletonSyncManager::instance()->SynchroSkeletonModify();
		//}
		std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> new_updateAllRenderTargetsStart = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock().now());
		m_pRenderSystem->_updateAllRenderTargets();
		std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> new_updateAllRenderTargetsEnd = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock().now());
		std::chrono::duration<long long, std::nano> new_updateAllRenderTargetsResult = new_updateAllRenderTargetsEnd - new_updateAllRenderTargetsStart;
		currentFrameData->renderOneFrameStatus.newUpdateAllRenderTargetStartTimePoint = new_updateAllRenderTargetsStart.time_since_epoch().count();
		currentFrameData->renderOneFrameStatus.newUpdateAllRenderTargetEndTimePoint = new_updateAllRenderTargetsEnd.time_since_epoch().count();
		currentFrameData->renderOneFrameStatus.newUpdateAllRenderTargetDuration = new_updateAllRenderTargetsResult.count();

		//TODO(yanghang):Here isn't best place to post asynchronous skeleton calculate job.
		// We already know what skeletons need to calculate.
		//{
		//	SkeletonSyncManager::instance()->PrepareJob();
		//}

		if (m_enable3DUISceneQueryCullTick)
		{
			//IMPORTANT(yanghang):Only need delay free 3DUI renderable object.
			UIRenderSystem::instance()->delayFreeUIRenderable();
		}
		else
		{
			//IMPORTANT(yanghang):Delay free renderable after render system has used them.
			// and clean render UI item list each frame.
			{
				UIRenderSystem::instance()->cleanFrameState();
			}
			//IMPORTANT(yanghang):Clear debug draw frame data.
			{
				DebugDrawManager::instance()->clearFrameCachedData();
			}
		}

		++mNextFrame;

		auto frameEndTime = std::chrono::steady_clock::now();

		bool ret = _fireFrameEnded();


		//if (EchoProfilerManager::instance()->GetTotalTime() >= (milliseconds)m_bProtocolComFrameTimeInMilliSec)
		//	LogManager::instance()->logMessage(EchoProfilerManager::instance()->ToString());

		auto endUpdateTime = std::chrono::steady_clock::now();
		std::chrono::nanoseconds diffTime1 = endUpdateTime - frameEndTime;
		mFrameEndTime = (uint32)(diffTime1.count() / 1000);

		std::chrono::nanoseconds diffTime2 = endUpdateTime - startUpdateTime;
		mSceneUpdateTime = (uint32)(diffTime2.count() / 1000);

		if (m_bUseRenderCommand)
		{
			if (!m_bPreRenderOneFrame)
			{
				m_pRenderer->endCommand();

				auto endRenderTime = std::chrono::steady_clock::now();
				std::chrono::nanoseconds diffTime3 = endRenderTime - endUpdateTime;
				mWaitRenderTime = (uint32)(diffTime3.count() / 1000);

				if (ECHO_THREAD_MT0 == m_threadMode)
				{
					mWaitUpdateTime = m_pRenderer->getWaitworkTime();
					mSwapBufferTime = m_pRenderer->getSwapBufferTime();
					mRenderMainTime = m_pRenderer->getCommitTime() - mSwapBufferTime;

					onHandleDynamicVideo(mRenderMainTime + mSwapBufferTime);
					onCalculateAvgTime(mRenderMainTime + mSwapBufferTime);
				}
			}
		}


		if (mNewWorldOrigin != mWorldOrigin)
		{
			for (const auto& listener : mWorldOriginListeners) {
				listener->worldOriginRebasing(mNewWorldOrigin.x, mNewWorldOrigin.y, mNewWorldOrigin.z);
			}
			mWorldOrigin = mNewWorldOrigin;
		}

		// Submit data to the profiler
		currentFrameData->renderOneFrameStatus.waitRenderTime = mWaitRenderTime;
		currentFrameData->renderOneFrameStatus.renderMainTime = mWaitUpdateTime;
		currentFrameData->renderOneFrameStatus.waitUpdateTime = mWaitUpdateTime;
		currentFrameData->renderOneFrameStatus.beginComponentTickTimePoint = beginComponentTickTime.time_since_epoch().count();
		currentFrameData->renderOneFrameStatus.endComponentTickTimePoint = endComponentTickTime.time_since_epoch().count();

		// Last Function To Call in this function
		high_res_time_point renderOneFrameEndTime = time_point_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now());
		currentFrameData->renderOneFrameStatus.renderOneFrameEndTimePoint = renderOneFrameEndTime.time_since_epoch().count();
		currentFrameData->renderOneFrameStatus.renderOneFrameDuration = currentFrameData->renderOneFrameStatus.renderOneFrameEndTimePoint
			- currentFrameData->renderOneFrameStatus.renderOneFrameStartTimePoint;

		return ret;
	}

	bool Root::renderMainThread(bool _bInited)
	{
		EchoZoneScoped;
		if (!m_bUseRenderCommand)
			return false;

		if (ECHO_THREAD_MT1 != m_threadMode)
			return false;

		return true;
	}

	void Root::postRenderOneFrame(void)
	{
		EchoZoneScoped;
		std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> postRenderOneFrameStart = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock().now());
		if (m_bUseRenderCommand)
		{
			if (m_bPreRenderOneFrame)
			{
				auto beginTime = std::chrono::steady_clock::now();

				m_pRenderer->endCommand();
				auto endTime = std::chrono::steady_clock::now();

				std::chrono::nanoseconds diffTime = endTime - beginTime;

				mWaitRenderTime = (uint32)(diffTime.count() / 1000);


				if (ECHO_THREAD_MT0 == m_threadMode)
				{
					mWaitUpdateTime = m_pRenderer->getWaitworkTime();
					mSwapBufferTime = m_pRenderer->getSwapBufferTime();
					mRenderMainTime = m_pRenderer->getCommitTime() - mSwapBufferTime;

					onHandleDynamicVideo(mRenderMainTime + mSwapBufferTime);
					onCalculateAvgTime(mRenderMainTime + mSwapBufferTime);
				}
			}

			m_bPreRenderOneFrame = false;
		}
		std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> postRenderOneFrameEnd = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock().now());
		std::chrono::duration<long long, std::nano> postRenderOneFrameDuration = postRenderOneFrameEnd - postRenderOneFrameStart;

		ProfilerInfoManager::instance()->m_currentFrameData->renderOneFrameStatus.postRenderOneFrameBeginTimePoint = postRenderOneFrameStart.time_since_epoch().count();
		ProfilerInfoManager::instance()->m_currentFrameData->renderOneFrameStatus.postRenderOneFrameEndTimePoint = postRenderOneFrameEnd.time_since_epoch().count();
		ProfilerInfoManager::instance()->m_currentFrameData->renderOneFrameStatus.postRenderOneFrameDuration = postRenderOneFrameDuration.count();
		//String testInfo = getRendererDebugInfo();
		//LogManager::instance()->logMessage(LML_NORMAL, testInfo);
		uint32 time = (uint32)(mTimeSinceLastFrame * 1000);
		CalClientTime((uint32)time);
		CalRenderOneFrameLogicTime();
	}

	uint32	Root::getRenderOneFrameLogicTime() const
	{
		return m_thisFrameRenderOneFrameLogicTime;
	}

	uint32 Root::getCalculatedClientTime() const
	{
		return mCalculatedClientTime;
	}

	String Root::getRendererDebugInfo() const
	{
		String debugString = m_context->pDebugInfoStr;
		return debugString;
	}

	void Root::setExpectFps(uint32 fps)
	{
		EchoZoneScoped;


		uint32 initExpectFps = DynamicVideoParamManager::instance()->getInitExpectFps();
		if (fps > initExpectFps)
		{
			fps = initExpectFps;
		}
		mExpectFps = fps;
		DynamicVideoParamManager::instance()->setExpectFps(fps);
	}

	void Root::onHandleDynamicVideo(uint32 time)
	{
		EchoZoneScoped;

		static uint32 count = 0;
		static uint32 totalTime = 0;
		static uint32 updateTime = 0;
		static uint32 renderTime = 0;

		static uint32 avgTotalRenderTime = 0;
		static uint32 avgSceneUpdateTime = 0;
		static uint32 avgRenderMainTime = 0;

		static bool calAvg = false;

		count++;
		totalTime += time;
		updateTime += mSceneUpdateTime;
		renderTime += mRenderMainTime;

		totalTime -= avgTotalRenderTime;
		updateTime -= avgSceneUpdateTime;
		renderTime -= avgRenderMainTime;

		if (!calAvg && count >= 30)
		{
			calAvg = true;
		}

		if (calAvg)
		{
			avgTotalRenderTime = totalTime / 30;
			avgSceneUpdateTime = updateTime / 30;
			avgRenderMainTime = renderTime / 30;
			count = 0;
		}

		//if (count >= 30)
		//{
			//mAvgTotalRenderTime = totalTime / 30;
			//mAvgSceneUpdateTime = updateTime / 30;
			//mAvgRenderMainTime = renderTime / 30;

			//uint32 destTime = 1000000 / mExpectFps;

			//if (avgTime > destTime)
			//{
			//	bUseTs = true;
			//	m_pRenderSystem->useRenderStrategy(1);
			//}
			//else if( 2.0f * avgTime  < destTime)
			//{
			//	bUseTs = false;
			//	m_pRenderSystem->useRenderStrategy(0);
			//}

			//count = 0;
			//totalTime = 0;
			//renderTime = 0;
			//updateTime = 0;
		//}

		if (mFps >= mExpectFps)
		{
			DynamicVideoParamManager::instance()->updateTotalRenderTime(0);
			DynamicVideoParamManager::instance()->updateTime(0, 0);
			DynamicVideoParamManager::instance()->updateTriCount(0);
		}
		else
		{
			DynamicVideoParamManager::instance()->updateTotalRenderTime(avgTotalRenderTime / 1000);
			DynamicVideoParamManager::instance()->updateTime(avgSceneUpdateTime / 1000, avgRenderMainTime / 1000);
			DynamicVideoParamManager::instance()->updateTriCount(m_triangleCount);
		}

		uint64 lastCameraFrameNumber = getMainSceneManager()->getMainCamera()->getLastFrameNumber();
		int mode = DynamicVideoParamManager::instance()->getVideoParamMode(Param_Type_ViewportScale);

		if (0 != mode && mNextFrame - lastCameraFrameNumber > 10)
		{
			DynamicVideoParamManager::instance()->resetVideoParamMode(Param_Type_ViewportScale);
		}
	}


	void Root::onCalculateAvgTime(uint32 time)
	{
		EchoZoneScoped;

		static uint32 count = 0;
		static uint32 totalTime = 0;
		static uint32 updateTime = 0;
		static uint32 renderTime = 0;
		static uint32 waitUpdateTime = 0;
		static uint32 waitRenderTime = 0;
		static uint32 swapBufferTime = 0;

		count++;
		totalTime += time;
		updateTime += mSceneUpdateTime;
		renderTime += mRenderMainTime;
		waitUpdateTime += mWaitUpdateTime;
		waitRenderTime += mWaitRenderTime;
		swapBufferTime += mSwapBufferTime;

		if (count >= 30)
		{
			mAvgTotalRenderTime = totalTime / 30;
			mAvgSceneUpdateTime = updateTime / 30;
			mAvgRenderMainTime = renderTime / 30;
			mAvgWaitUpdateTime = waitUpdateTime / 30;
			mAvgWaitRenderTime = waitRenderTime / 30;
			mAvgSwapBufferTime = swapBufferTime / 30;

			totalTime = 0;
			updateTime = 0;
			renderTime = 0;
			waitUpdateTime = 0;
			waitRenderTime = 0;
			swapBufferTime = 0;
			count = 0;
		}
	}

	void Root::stopRendering()
	{
		EchoZoneScoped;

		mWorkQueue->setPaused(true);
	}

	void Root::resumeRendering()
	{
		EchoZoneScoped;

#ifdef __APPLE__
		uint32 width, height;
		m_pRenderSystem->getWindowSize(width, height);
		m_pRenderSystem->createRenderWindow(width, height, Root::instance()->getRcContext()->swapchainColorFormat, true);
		m_pRenderSystem->fireResizeEvent(width, height);
#endif
		mWorkQueue->setPaused(false);
	}

	//-----------------------------------------------------------------------
	void Root::convertColourValue(const ColorValue& colour, uint32* pDest)
	{
		EchoZoneScoped;

		assert(m_pRenderSystem != 0);
		m_pRenderSystem->convertColourValue(colour, pDest);
	}

	//-----------------------------------------------------------------------

	void Root::onRecreateSurface(void* newWindow)
	{
		EchoZoneScoped;

		m_context->pCustom = newWindow;
		m_pRenderer->setNeedRecreateSurface();
	}

	RenderTarget* Root::createRenderWindow(const String& name, uint32 width, uint32 height,
		bool fullScreen, const NameValuePairList* miscParams)
	{
		EchoZoneScoped;

		if (!m_pRenderSystem)
		{
			ECHO_EXCEPT(Exception::ERR_INVALID_STATE,
				"Cannot create window - no render "
				"system has been selected.", "Root::createRenderWindow");
		}

		RenderTarget* ret;
		ret = m_pRenderSystem->createRenderWindow(width, height, Root::instance()->getRcContext()->swapchainColorFormat, false);

		// Initialisation for classes dependent on first window created
		if (!mFirstTimePostWindowInit)
		{
			oneTimePostWindowInit();
		}

		return ret;

	}

	//-----------------------------------------------------------------------
	void Root::oneTimePostWindowInit(void)
	{
		EchoZoneScoped;

		if (!mFirstTimePostWindowInit)
		{
			// Background loader
			ResourceBackgroundQueue::instance()->initialise();
			MaterialBuildHelper::instance()->initialise();
			RoleAnimationLoadManager::instance()->initialise();
			RoleMeshLoadManager::instance()->initialise();
			TextureManager::instance()->initialise();
			BufferManager::instance()->initialise();
			//ActorInfoLoadManager::instance()->initialise();
			mWorkQueue->startup();
			mFirstTimePostWindowInit = true;
		}
	}

	Echo::uint64 Root::getSystemTime()
	{
		EchoZoneScoped;

#ifdef _WIN32
		return ::GetTickCount64();
#else
		struct timeval time;
		gettimeofday(&time, NULL);


		uint64 curTime = time.tv_sec * 1000 + time.tv_usec / 1000;

		return curTime;
#endif
	}

	//-----------------------------------------------------------------------
	void Root::clearEventTimes(void)
	{
		EchoZoneScoped;

		// Clear event times
		for (int i = 0; i < FETT_COUNT; ++i)
			mEventTimes[i].clear();
	}
	//---------------------------------------------------------------------
	void Root::setWorkQueue(WorkQueue* queue)
	{
		EchoZoneScoped;

		if (mWorkQueue != queue)
		{
			// delete old one (will shut down)
			delete mWorkQueue;

			mWorkQueue = queue;
			if (m_bInitialise)
				mWorkQueue->startup();
		}
	}

	int Root::GetFps()
	{
		EchoZoneScoped;

		return mFps;
	}

	void Root::CalFps(uint32 timeFromLastFram)
	{
		EchoZoneScoped;

		static uint32 timeEscape = 0;
		static uint32  lastFramCount = 0;
		static uint32  lastFramCountForSleep = 0;
		static uint32 timeForSleepCal = 0;
		static uint32 sleeptime = 0;

		mFramCount++;
		timeEscape += timeFromLastFram;
		timeForSleepCal += timeFromLastFram;

		if (timeEscape > 1000)
		{
			uint32 nowFPS = (uint32)((mFramCount - lastFramCount) / (float)(timeEscape) * 1000.0f);

			if (mFps == 0)
			{
				mFps = nowFPS;
			}
			else
			{
				mFps = (mFps + nowFPS) / 2;
			}

			lastFramCount = mFramCount;
			timeEscape = 0;
		}

		if (mFramCount - lastFramCountForSleep == SleepTimeCalFreq)
		{
			uint32 AvgTimePerFram = (timeForSleepCal) / SleepTimeCalFreq - sleeptime;

			if (AvgTimePerFram < mtimePerFram)
			{
				sleeptime = mtimePerFram - AvgTimePerFram;
			}
			else
			{
				sleeptime = 0;
			}

			lastFramCountForSleep = mFramCount;
			timeForSleepCal = 0;
		}

		//引擎底层取消限帧
		//if (sleeptime != 0)
		//{
		//	#ifdef _WIN32
		//		Sleep(sleeptime);
		//	#endif

		//	#ifdef __ANDROID__
		//		usleep(sleeptime * 1000);
		//	#endif
		//}
	}

	void Root::CalClientTime(uint32 timeSinceLastFrame)
	{
		const ProfilerFrameData* lastFrameData = ProfilerInfoManager::instance()->GetLastFrameData();
		if (nullptr == lastFrameData)
			return;
		long long longTimeSinceLastFrame = timeSinceLastFrame;
		long long logicTime = longTimeSinceLastFrame * 1000000
			- lastFrameData->renderOneFrameStatus.preRenderOneFrameBeginCommandDuration
			- lastFrameData->renderOneFrameStatus.renderOneFrameDuration
			- lastFrameData->renderOneFrameStatus.postRenderOneFrameDuration;
		mCalculatedClientTime = (uint32)logicTime / 1000000;
	}

	void Root::CalRenderOneFrameLogicTime()
	{
		uint32 preRenderOneFrameDuration = (uint32)ProfilerInfoManager::instance()->m_currentFrameData->renderOneFrameStatus.preRenderOneFrameBeginCommandDuration;
		uint32 renderOneFrameDuration = (uint32)ProfilerInfoManager::instance()->m_currentFrameData->renderOneFrameStatus.renderOneFrameDuration;
		uint32 postRenderOneFrameDuration = (uint32)ProfilerInfoManager::instance()->m_currentFrameData->renderOneFrameStatus.postRenderOneFrameDuration;
		m_thisFrameRenderOneFrameLogicTime = (uint32)(preRenderOneFrameDuration + renderOneFrameDuration + postRenderOneFrameDuration) / 1000;
	}

#ifdef _WIN32
	bool Root::CaseSensitive(const char* _szPath, const char* _szFile, const char* rootpath)
	{
		EchoZoneScoped;

		bool val = false;
		std::string respath = _szPath;
		HANDLE hFile = CreateFileA(respath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			CHAR szFilePath[MAX_PATH] = { 0 };
			DWORD dwRetVal = GetFinalPathNameByHandleA(hFile, szFilePath, MAX_PATH, 0);
			if (dwRetVal > 0)
			{
				//val = false;
				std::string temp = szFilePath;
				std::replace(temp.begin(), temp.end(), '\\', '/');


				std::string filePath = _szFile;
				std::replace(filePath.begin(), filePath.end(), '\\', '/');
				while (filePath.find("//") != std::string::npos)
				{
					filePath.replace(filePath.find("//"), 2, "/");
				}

				if (_szFile[1] == ':' || _szFile[0] == '\\' || _szFile[0] == '/')
				{
					std::string rootPath = rootpath;
					std::replace(rootPath.begin(), rootPath.end(), '\\', '/');
					if (filePath.find(rootPath) != std::string::npos)
					{
						filePath.erase(0, rootPath.size());
					}
					else
						val = true;
				}
				if (temp.find(filePath) != filePath.npos)
					val = true;
			}
			CloseHandle(hFile);
		}
		else
		{
			//无法成功打开文件的情况，这里默认是文件的访问权限被其他占用了，不作检测
			//CreateFileA需要文件的独占访问权限，这里不考虑文件不存在的情况
			val = true;
		}
		if (!val)
		{
			LogManager::instance()->logMessage("WARNING! File Case Sensitive!\t File path : \'" + std::string(_szFile), LML_NORMAL);
		}

		return val;
	}

#endif
	bool Root::TestResExist(const char* _szFile, bool atEngineDir, const char* _szfolder)
	{
		EchoZoneScoped;

		assert(mArchive != 0);

		if (0 == strlen(_szFile))
			return false;

		if (atEngineDir)
			return mArchive->TestExistAtEngine(_szFile, ResourceFolderManager::instance()->GetRootFolder(Name(_szfolder)).c_str());

		return mArchive->TestExist(_szFile, ResourceFolderManager::instance()->GetRootFolder(Name(_szfolder)).c_str());
	}

	DataStream* Root::GetFileDataStream(const char* _szFile, bool atEngineDir, const char* _szfolder)
	{
		EchoZoneScoped;

		assert(mArchive != 0);

		if (0 == strlen(_szFile))
			return nullptr;


		if (atEngineDir)
			return mArchive->GetFileDataStreamAtEngine(_szFile, ResourceFolderManager::instance()->GetRootFolder(Name(_szfolder)).c_str());

		return mArchive->GetFileDataStream(_szFile, ResourceFolderManager::instance()->GetRootFolder(Name(_szfolder)).c_str());
	}

	DataStream* Root::GetMemoryDataStream(const char* _szFile, bool atEngineDir, const char* _szfolder)
	{
		EchoZoneScoped;

		assert(mArchive != 0);

		if (0 == strlen(_szFile))
			return nullptr;


		if (atEngineDir)
			return mArchive->GetMemoryDataStreamAtEngine(_szFile, ResourceFolderManager::instance()->GetRootFolder(Name(_szfolder)).c_str());

		return mArchive->GetMemoryDataStream(_szFile, ResourceFolderManager::instance()->GetRootFolder(Name(_szfolder)).c_str());
	}

	DataStream* Root::CreateDataStream(const char* _szFile, const char* _szfolder)
	{
		EchoZoneScoped;

		assert(mArchive != 0);

		if (0 == strlen(_szFile))
			return nullptr;

		return mArchive->CreateDataStream(_szFile, ResourceFolderManager::instance()->GetRootFolder(Name(_szfolder)).c_str());
	}

	bool Root::OutputCollectResListInfo(String fileName)
	{
		EchoZoneScoped;

		if (fileName.empty())
			fileName = "info/roleFolderFilterFile_2.txt";

		DataStreamPtr outstream(CreateDataStream(fileName.c_str()));
		if (outstream.isNull())
		{
			assert(false);
			return false;
		}

		for (auto it = m_collectedResSet.begin(); it != m_collectedResSet.end(); ++it)
		{
			outstream->writeData(it->c_str(), it->length());
			outstream->writeData("\r\n", 2);
		}

		outstream->close();

		m_collectedResSet.clear();

		return true;
	}


	//void Root::requestIORequest(const IORequest& request)
	//{
	//	assert(mArchive != 0);


	//	mArchive->requestIORequest(request);
	//}

	void Root::requestMultiIORequest(const MultiIORequest& request)
	{
		EchoZoneScoped;

		assert(mArchive != 0);

		if (m_bCollectResList)
		{
			std::map<Name, uint8>::const_iterator it, ite = request.fileNameMap.end();
			for (it = request.fileNameMap.begin(); it != ite; ++it)
				m_collectedResSet.insert(it->first.toString());
		}

		mArchive->requestMultiIORequest(request);
	}

	std::tuple<unsigned int, unsigned int> Root::getBuildingRenderInfo() const
	{
		EchoZoneScoped;

		return BatchManager::instance()->getBuildingRenderInfo();
	}

	void Root::setRenderGroupMask(uint32 mask)
	{
		EchoZoneScoped;

		m_renderGroupMask = mask;
	}

	void Root::setFpsLimit(int _fpsLimit)
	{
		EchoZoneScoped;

		if (_fpsLimit < 0 || _fpsLimit > 120)
		{
			LogManager::getSingleton().logMessage("the setting of fpslimit " +
				StringConverter::toString(_fpsLimit) + " is not rational !");
			return;
		}
		mtimePerFram = 1000 / (_fpsLimit + 1);//设置的_fpsLimit参数表示最高能达到的帧速，帧速会小于等于这个值，加1为了模拟在当前设置的值周围波动的情况。
	}

	void Root::initConfig()
	{
		EchoZoneScoped;
#ifdef ECHO_EDITOR
		ErrorMessageGroupManager::instance()->registerBasicType();
#endif // ECHO_EDITOR
		ShareRoleConfig::instance()->init();
		BatchManager::instance()->initInstanceVB();
		RoleBatchManager::instance()->initInstanceVB();
		MaterialAnimManager::instance()->Init();
		PetBatchMaterialManager::instance()->initTextureArray();
		UIRenderSystem::instance()->prepareAllStyleAndTypeMaterials();
		PortraitManager::instance()->initConfig();
	}

	void Root::deinitConfig()
	{
		EchoZoneScoped;
#ifdef ECHO_EDITOR
		ErrorMessageGroupManager::instance()->unregisterBasicType();
#endif // ECHO_EDITOR
		ShareRoleConfig::instance()->deinit();
		BatchManager::instance()->deinitInstanceVB();
		RoleBatchManager::instance()->deinitInstanceVB();
		MaterialAnimManager::instance()->Deinit();
		UIRenderSystem::instance()->deInit();
		PortraitManager::instance()->uninitConfig();
	}

	void Root::initSurfaceType()
	{
		EchoZoneScoped;

		SystemConfigFile cfg;
		cfg.load("config/SurfaceType.cfg");
		const SettingsMultiMap* pSetMap = cfg.getSettingsMap("SurfaceType");
		if (pSetMap)
		{
			SettingsMultiMap::const_iterator it, ite = pSetMap->end();
			for (it = pSetMap->begin(); it != ite; ++it)
			{
				unsigned int id = StringConverter::parseUnsignedInt(it->first);
				SurfaceTypeUtil::registerSurfaceType(id, it->second);
			}
		}
		else
		{
			SurfaceTypeUtil::initDefault();
		}
	}

	void Root::captureScreen(const String& _filePath, uint32 _mode, uint32 _useCustomResMode, uint32 _customWidth /*= 1024*/, uint32 _customHeight /*= 1024*/, ECHO_CAPTURE_TYPE _type/* = ECHO_CAPTURE_COLOR*/)
	{
		EchoZoneScoped;

		ECHO_DEVICE_CLASS deviceType = Root::instance()->getDeviceType();
		if (_type == ECHO_CAPTURE_DEPTH && deviceType != ECHO_DEVICE_DX11)
		{
			LogManager::getSingleton().logMessage("captureScreen: Only support ECHO_CAPTURE_DEPTH in DX11 device.", LML_CRITICAL);
			return;
		}

		if (_customWidth == 0)
			_customWidth = 1024;
		if (_customHeight == 0)
			_customHeight = 1024;

		RcCaptureScreen pInfo;
		size_t len = _filePath.length();
		memcpy(pInfo.filePath, _filePath.c_str(), len);
		pInfo.filePath[len] = '\0';
		pInfo.mode = _mode;
		pInfo.type = _type;
		pInfo.pNative = nullptr;
		pInfo.useCustomRes = _useCustomResMode;
		pInfo.customWidth = _customWidth;
		pInfo.customHeight = _customHeight;
		pInfo.pCustom = (void*)(&_saveRenderTargetContentsToFile);

		mCaptureScreenInfoList.push_back(pInfo);
	}

	void Root::captureScreenAndGenerateTexture(const String& _filePath, uint32 _mode, CaptureScreenCallBackFun _callBackFun, uint32 _useCustomResMode, uint32 _customWidth /*= 1024*/, uint32 _customHeight /*= 1024*/, ECHO_CAPTURE_TYPE _type/* = ECHO_CAPTURE_COLOR*/)
	{
		EchoZoneScoped;

		ECHO_DEVICE_CLASS deviceType = Root::instance()->getDeviceType();
		if (_type == ECHO_CAPTURE_DEPTH && deviceType != ECHO_DEVICE_DX11)
		{
			LogManager::getSingleton().logMessage("captureScreen: Only support ECHO_CAPTURE_DEPTH in DX11 device.", LML_CRITICAL);
			return;
		}

		if (_customWidth == 0)
			_customWidth = 1024;
		if (_customHeight == 0)
			_customHeight = 1024;

		RcCaptureScreen pInfo;
		size_t len = _filePath.length();
		memcpy(pInfo.filePath, _filePath.c_str(), len);
		pInfo.filePath[len] = '\0';
		pInfo.mode = _mode;
		pInfo.type = _type;
		pInfo.pNative = (void*)_callBackFun;
		pInfo.useCustomRes = _useCustomResMode;
		pInfo.customWidth = _customWidth;
		pInfo.customHeight = _customHeight;
		pInfo.pCustom = (void*)(&_saveRenderTargetContentsToFile);

		mCaptureScreenInfoList.push_back(pInfo);
	}

	void  Root::_onHandleCaptureScreen(RenderTarget* pRenderWindow, bool _renderUI)
	{
		EchoZoneScoped;

		if (mCaptureScreenInfoList.empty())
			return;

		assert(m_pRenderSystem != 0);
		RenderStrategy* renderStrategy = m_pRenderSystem->getCurRenderStrategy();
		assert(renderStrategy != 0);

		vector<RcCaptureScreen>::type::iterator it = mCaptureScreenInfoList.begin();

		while (it != mCaptureScreenInfoList.end())
		{
			RcCaptureScreen& info = *it;
			if (false == _renderUI && 0 == info.mode) //0 BackBuffer截屏带UI模式
			{
				++it;
				continue;
			}

			RenderTarget* captureRT = pRenderWindow;
			if (1 == info.mode) //MainRT真实尺寸截屏模式
				captureRT = renderStrategy->getRenderTargetByType(RT_RealMain).get();

			if (!m_pRenderSystem->copyRenderTargetContentsToMemory(captureRT, info))
			{
				it = mCaptureScreenInfoList.erase(it);
				LogManager::getSingleton().logMessage("captureScreen: Copy render target contents to memory failed!!!", LML_CRITICAL);
				continue;
			}

			if (!m_bUseRenderCommand)
			{
				_saveRenderTargetContentsToFile(&info);
				if (info.dataSize > 0 && info.pData)
				{
					::free(info.pData);
					info.pData = nullptr;
				}
			}

			it = mCaptureScreenInfoList.erase(it);
		}
	}

	void Root::_saveRenderTargetContentsToFile(RcCaptureScreen* pInfo)
	{
		EchoZoneScoped;

		unsigned int imageWidth = pInfo->realWidth;
		unsigned int imageHeight = pInfo->realHeight;
		ECHO_FMT     imageEchoFmt = pInfo->dataFormat;

		ECHO_DEVICE_CLASS deviceType = Root::instance()->getDeviceType();

		if (ECHO_DEVICE_METAL == deviceType)
		{
			if (ECHO_FMT_BGRA8888_UNORM == imageEchoFmt)
			{
				ImageConvertToBGRA8888 convert(pInfo->pData, ECHO_FMT_BGRA8888_UNORM, imageWidth, imageHeight, false, true);
				memcpy(pInfo->pData, convert.pMem, pInfo->dataSize);
			}
		}

		unsigned char* pImageData = (unsigned char*)malloc(pInfo->dataSize);
		memcpy(pImageData, pInfo->pData, pInfo->dataSize); //pInfo->pData 统一在外部释放

		if (deviceType == ECHO_DEVICE_DX11 && pInfo->type == ECHO_CAPTURE_DEPTH)
		{
			CaptureScreenCallBackFun callFun = (CaptureScreenCallBackFun)pInfo->pNative;
			if (callFun)
			{
				callFun(pInfo);
			}

			FILE* fp = fopen(pInfo->filePath, "wb");
			if (fp)
			{
				fwrite(pImageData, 1, pInfo->dataSize, fp);
				fclose(fp);
			}
			else
			{
				LogManager::getSingleton().logMessage("captureScreen: Failed to open file for writing: " + String(pInfo->filePath), LML_CRITICAL);
			}

			::free(pImageData);
			return;
		}

		//先看看需不需要创建一张纹理
		if (pInfo->pNative)
		{
			CaptureScreenCallBackFun callFun = (CaptureScreenCallBackFun)pInfo->pNative;
			if (callFun)
			{
				_onHandRenderTargetData(pInfo); //需要把各个平台数据进行规整

				callFun(pInfo);
			}
		}


		//存磁盘文件
		Image image;

		if (ECHO_DEVICE_DX9C == deviceType)
		{
			PixelFormat pixelFormat = PF_X8R8G8B8;
			if (imageEchoFmt == ECHO_FMT_RGB888_UNORM) {
				pixelFormat = PF_R8G8B8;
			}
			image.loadFromData(pImageData, imageWidth, imageHeight, pixelFormat, true);
		}
		else if (ECHO_DEVICE_DX11 == deviceType)
		{
			PixelFormat pixelFormat = PF_X8B8G8R8;
			if (imageEchoFmt == ECHO_FMT_RGB888_UNORM) {
				pixelFormat = PF_B8G8R8;
			}
			image.loadFromData(pImageData, imageWidth, imageHeight, pixelFormat, true);
		}
		else if (ECHO_DEVICE_GLES3 == deviceType || ECHO_DEVICE_VULKAN == deviceType)
		{
			PixelFormat pixelFormat = PF_X8B8G8R8;
			if (imageEchoFmt == ECHO_FMT_RGB888_UNORM) {
				pixelFormat = PF_B8G8R8;
			}
			image.loadFromData(pImageData, imageWidth, imageHeight, pixelFormat, true);
			image.flipAroundX();
		}
		else if (ECHO_DEVICE_METAL == deviceType)
		{
			PixelFormat pixelFormat = PF_X8B8G8R8;
			if (imageEchoFmt == ECHO_FMT_BGRA8888_UNORM) {
				pixelFormat = PF_BYTE_BGRA;
			}
			image.loadFromData(pImageData, imageWidth, imageHeight, pixelFormat, true);
		}

		if (image.getData() == 0)
		{
			::free(pImageData);
			return;
		}

		if (pInfo->useCustomRes > 0)
		{
			if (pInfo->customWidth != imageWidth || pInfo->customHeight != imageHeight)
			{
				if (1 == pInfo->useCustomRes) //无需考虑宽高比
				{
					image.resize(pInfo->customWidth, pInfo->customHeight);
				}
				else if (2 == pInfo->useCustomRes) //按等宽高比缩放
				{
					if (imageWidth < imageHeight)
					{
						pInfo->customWidth = (unsigned int)(pInfo->customHeight * ((float)imageWidth / (float)imageHeight));
					}
					else
					{
						pInfo->customHeight = (unsigned int)(pInfo->customWidth * ((float)imageHeight / (float)imageWidth));
					}
					image.resize(pInfo->customWidth, pInfo->customHeight);
				}
			}
		}

		image.save(pInfo->filePath);
		image.freeMemory();

		pImageData = nullptr; //在image里释放了
	}

	void Root::_onHandRenderTargetData(RcCaptureScreen* pInfo)
	{
		EchoZoneScoped;

		unsigned int imageWidth = pInfo->realWidth;
		unsigned int imageHeight = pInfo->realHeight;
		ECHO_FMT     imageEchoFmt = pInfo->dataFormat;

		ECHO_DEVICE_CLASS deviceType = Root::instance()->getDeviceType();

		if (ECHO_DEVICE_DX9C == deviceType)
		{
			if (ECHO_FMT_RGBA8888_UNORM == imageEchoFmt) //需要把alpha通道填充为1
			{
				ImageConvertToRGBA8888 convert(pInfo->pData, ECHO_FMT_RGBA8888_UNORM, imageWidth, imageHeight, false, true);
				memcpy(pInfo->pData, convert.pMem, pInfo->dataSize);
			}
		}
		else if (ECHO_DEVICE_DX11 == deviceType)
		{
			if (ECHO_FMT_RGBA8888_UNORM == imageEchoFmt) //需要把alpha通道填充为1
			{
				ImageConvertToRGBA8888 convert(pInfo->pData, ECHO_FMT_RGBA8888_UNORM, imageWidth, imageHeight, false, true);
				memcpy(pInfo->pData, convert.pMem, pInfo->dataSize);
			}
		}
		else if (ECHO_DEVICE_GLES3 == deviceType || ECHO_DEVICE_VULKAN == deviceType)
		{
			size_t pixelSize = 4;
			if (imageEchoFmt == ECHO_FMT_RGB888_UNORM)
			{
				pixelSize = 3;
			}

			size_t rowSpan = imageWidth * pixelSize;

			uint8* pTempBuffer = (uint8*)malloc(rowSpan * imageHeight);
			uint8* ptr1 = (uint8*)pInfo->pData, * ptr2 = pTempBuffer + ((imageHeight - 1) * rowSpan);

			for (unsigned i = 0; i < imageHeight; i++)
			{
				memcpy(ptr2, ptr1, rowSpan);
				ptr1 += rowSpan; ptr2 -= rowSpan;
			}

			memcpy(pInfo->pData, pTempBuffer, rowSpan * imageHeight);

			free(pTempBuffer);
		}
		else if (ECHO_DEVICE_METAL == deviceType)
		{
		}
	}


	void Root::_onHandRenderCommandCallBack(unsigned int type, void* hwnd, void* pDevice, void* pContext)
	{
		EchoZoneScoped;

		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		if (pRenderSystem)
		{
			switch (type)
			{
			case 0:
				pRenderSystem->OnDeviceInit(hwnd, pDevice, pContext);
				break;
			case 1:
				pRenderSystem->OnDeviceLost();
				break;
			case 2:
				pRenderSystem->OnDeviceReset();
				break;
			case 3:
				//pRenderSystem->_fireBeginFrame();
				break;
			case 4:
				pRenderSystem->_fireEndFrame();
				break;
			default:
				break;
			}
		}
	}

	void Root::SetViewScale(float xscale, float yscale)
	{
		EchoZoneScoped;

		if (xscale < 0.0f || xscale > 1.0f)
		{
			return;
		}

		if (yscale < 0.0f || yscale > 1.0f)
		{
			return;
		}

		for (uint32 i = 0; i < RENDER_STRATEGY_CNT; i++)
		{
			RenderStrategy* pRenderStrategy = m_pRenderSystem->getRenderStrategy(i);
			if (pRenderStrategy)
				pRenderStrategy->setMainViewPortRange(xscale, yscale);
		}
	}

	void Root::addUpdateListener(UpdateListener* listener)
	{
		EchoZoneScoped;

		if (NULL == listener)
			return;
		mUpdateListeners.insert(listener);
	}

	void Root::removeUpdateListener(UpdateListener* listener)
	{
		EchoZoneScoped;

		mUpdateListeners.erase(listener);
	}

	void Root::notifyUpdateListener()
	{
		EchoZoneScoped;

		//SkeletonSyncManager::instance()->PrepareJob();

		//std::set<UpdateListener*>::iterator it = mUpdateListeners.begin();
		//for (;it!= mUpdateListeners.end();++it)
		//{
		//	(*it)->OnUpdateFinish(mTimeSinceLastFrameClient);
		//}
	}



	//void Root::addDirtyNode(WorldNode* pSceneNode,bool bUpdateBounds /*= false*/)
	//{
	//	if (NULL == pSceneNode)
	//	{
	//		return;
	//	}
	//	//tthread::lock_guard<tthread::recursive_mutex> lock(mLock);
	//	DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN)
	//	pSceneNode->setDirty(true);

	//	auto it = mDirtyNode.find(pSceneNode);
	//	if (it == mDirtyNode.end())
	//	{
	//		mDirtyNode.insert(pSceneNode);
	//	}
	//}

	//void Root::removeDirtyNode(WorldNode* pSceneNode)
	//{
	//	if (NULL == pSceneNode)
	//	{
	//		return;
	//	}
	//	pSceneNode->setDirty(false);
	//	DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN)
	//	//tthread::lock_guard<tthread::recursive_mutex> lock(mLock);
	//	mDirtyNode.erase(pSceneNode);
	//}

	void Root::addWorldNode(WorldNode* pNode)
	{
		EchoZoneScoped;

		if (NULL == pNode || pNode->getName().empty())
		{
			return;
		}
		m_mapWroldNodes[pNode->getName()] = pNode;
	}

	void Root::removeWorldNode(WorldNode* pNode)
	{
		EchoZoneScoped;

		if (NULL == pNode || pNode->getName().empty())
		{
			return;
		}
		auto it = m_mapWroldNodes.find(pNode->getName());
		if (it != m_mapWroldNodes.end() && it->second == pNode)
			m_mapWroldNodes.erase(it);
	}

	//void Root::addBoundDirtyNode(SceneNode* pSceneNode, bool bUpdateBounds /*= false*/)
	//{
	//	if (NULL == pSceneNode)
	//	{
	//		return;
	//	}
	//	DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN)
	//	//tthread::lock_guard<tthread::recursive_mutex> lock(mLock);
	//	mBoundDirtyNode[pSceneNode] = bUpdateBounds;
	//}

	//void Root::removeBoundDirtyNode(SceneNode* pSceneNode)
	//{
	//	if (NULL == pSceneNode)
	//	{
	//		return;
	//	}
	//	//tthread::lock_guard<tthread::recursive_mutex> lock(mLock);
	//	DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN)
	//	mBoundDirtyNode.erase(pSceneNode);
	//}

	//void Root::addRoleMovableNeedReset(RoleMovable* prole)
	//{
	//	if (NULL == prole)
	//	{
	//		return;
	//	}

	//	//tthread::lock_guard<tthread::recursive_mutex> lock(mLock);
	//	mRoleMovableNeedReset.insert(prole);
	//}

	//void Root::removeRoleMovableNeedReset(RoleMovable* prole)
	//{
	//	//tthread::lock_guard<tthread::recursive_mutex> lock(mLock);

	//	RoleMovableList::iterator iter = find(mRoleMovableNeedReset.begin(), mRoleMovableNeedReset.end(), prole);
	//	if (iter != mRoleMovableNeedReset.end())
	//	{
	//		mRoleMovableNeedReset.erase(iter);
	//	}
	//}

	//void Root::resetRoleMovableState()
	//{
	//	//tthread::lock_guard<tthread::recursive_mutex> lock(mLock);
	//	RoleMovableList::iterator iter = mRoleMovableNeedReset.begin(), iterend = mRoleMovableNeedReset.end();

	//	for (; iter != iterend; ++iter)
	//	{
	//		(*iter)->resetIsRenderState();
	//	}

	Echo::WorldNode* Root::getWorldNode(const Name& name)
	{
		EchoZoneScoped;

		auto it = m_mapWroldNodes.find(name);
		if (it != m_mapWroldNodes.end())
		{
			return it->second;
		}
		return NULL;
	}

	//	mRoleMovableNeedReset.clear();
	//}

	uint64 Root::getWallTime()
	{
		return mTimer->getMicroseconds();
	}
	void Root::SystemSynchro()
	{
		EchoZoneScoped;

		//auto itb1 = mSceneManagerMap.begin();
		//auto ite1 = mSceneManagerMap.end();
		//for (; itb1 != ite1; itb1++)
		//{
		//	itb1->second->getEffectManager()->Synchro();
		//}
#ifdef PRINT_TIME_INFO
		uint64 frameEndTime0 = mTimer->getMicroseconds();
#endif

#ifdef PRINT_TIME_INFO
		uint64 frameEndTime1 = mTimer->getMicroseconds();
#endif


		if (mIsParticleTickAsync)
		{
			EffectTickJobManager::instance()->Synchro();
		}
#ifdef PRINT_TIME_INFO
		uint64 frameEndTime2 = mTimer->getMicroseconds();
#endif

#ifdef PRINT_TIME_INFO
		uint64 frameEndTime3 = mTimer->getMicroseconds();
#endif
		auto itb = mSceneManagerMap.begin();
		auto ite = mSceneManagerMap.end();
		for (; itb != ite; itb++)
		{
			itb->second->getEffectManager()->exeParticleVanishListen();
			itb->second->getEffectManager()->destroyWaitParticle();
		}
#ifdef PRINT_TIME_INFO
		uint64 frameEndTime4 = mTimer->getMicroseconds();
#endif
		EffectResourceManager::instance()->Tick(0.033333f);
#ifdef PRINT_TIME_INFO
		uint64 frameEndTime5 = mTimer->getMicroseconds();

		uint64 time = frameEndTime5 - frameEndTime0;

		if (time > 3000)
		{
			char msg[256] = { 0 };
			sprintf(msg, "33333,%lld,%lld,%lld,%lld,%lld,%lld", time, frameEndTime1 - frameEndTime0, frameEndTime2 - frameEndTime1, \
				frameEndTime3 - frameEndTime2, frameEndTime4 - frameEndTime3, frameEndTime5 - frameEndTime4);
			LogManager::instance()->logMessage(msg, LML_CRITICAL);
		}
#endif
	}

	void Root::SynchroSkeleton()
	{
		EchoZoneScoped;

		SkeletonSyncManager::instance()->SynchroSkeletonModify();
		SkeletonSyncManager::instance()->Synchro();
		SkeletonSyncManager::instance()->destroyAllFreeInstance();
	}

	void Root::shutDown()
	{
		EchoZoneScoped;

		JobSystem::instance()->Shutdown();
		auto itb = mSceneManagerMap.begin();
		auto ite = mSceneManagerMap.end();
		for (; itb != ite; itb++)
		{
			itb->second->getEffectManager()->destroyAllParticle();
		}

		OAE::OAESystem::instance()->shutdown();

		//ActorComFactoryManager::getSingleton().ShutDown();
		EnvironmentLight::instance()->shutDown();
		ResourceBackgroundQueue::instance()->shutdown();
		MaterialBuildHelper::instance()->shutdown();
		RoleAnimationLoadManager::instance()->shutdown();
		RoleMeshLoadManager::instance()->shutdown();
		//ActorInfoLoadManager::instance()->shutdown();
		MaterialCompileManager::instance()->Deinit();
		EffectResourceManager::instance()->destroyAllDelayRes();
		TextureManager::instance()->shutdown();
		BufferManager::instance()->shutdown();
		if (m_pRenderSystem)
		{
			m_pRenderSystem->shutDownNew();
		}

		UIRenderSystem::instance()->uninitAsync();
		mWorkQueue->shutdown();


		m_pResourceManagerFactory->cleanupAll();
		DataStreamManager::instance()->shutdown();

		LogManager::getSingleton().logMessage("*-*-* ECHO Shutdown", LML_CRITICAL);
	}

	void Root::setMultiCameraRendering(bool value)
	{
		EchoZoneScoped;

		m_bMultiCameraRendering = value;
		if (!value)
			return;

		SceneManager* pSceneManager = getMainSceneManager();
		if (!pSceneManager)
			return;

		Camera* pSecondMainCamera = pSceneManager->getSecondMainCamera();
		if (!pSecondMainCamera)
		{
			pSecondMainCamera = pSceneManager->createCamera(Name("SecondMainCamera"));
		}
	}

#ifdef _WIN32
	bool Root::defThreadCheck(unsigned int flags)
	{
		EchoZoneScoped;

		DWORD dwCurThreadID = GetCurrentThreadId();
		uint8 threadIndex = 0;
		for (; threadIndex < EM_THREAD_CNT; ++threadIndex)
		{
			DWORD id = m_dwTreadID[threadIndex];
			if (id == dwCurThreadID)
			{
				break;
			}
		}
		bool check = false;
		if (threadIndex < EM_THREAD_CNT)
		{
			if (1 << threadIndex & flags)
			{
				check = true;
			}
		}
		if (!check)
		{
			char msg[256] = { 0 };
			snprintf(msg, 256, "Thread error,thread type:%d,%s", threadIndex, __FUNCTION__);
			LogManager::instance()->logMessage(msg, LML_CRITICAL);

			int* p = 0;
			*p = 1;
		}
		return check;
	}
#endif

	//-----------------------------------------------------------------------------
}
