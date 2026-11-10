#include "EchoComponentStableHeaders.h"
#define  ELUA_LIB

#include "EchoComponentSystem.h"
#include "EchoScriptManager.h"
#include "EchoCommonStableHeaders.h"
#include "EchoScriptReg.h"
#include "EchoScriptComponent.h"

#include "EchoActorProFuncImp.h"
#include "EchoCameraProFunctionImp.h"
#include "EchoMaterialProFunctionImp.h"

#include "EchoEntityObjProFunctionImp.h"
#include "EchoParticleProFunctionImp.h"
#include "EchoRoleObjProFunctionImp.h"
#include "EchoResBaseProFunctionImp.h"
#include "EchoHelperObjProFuncImp.h"
#include "EchoTrajectoryComponent.h"
#include "EchoPolylineComponent.h"
#include "EchoTimelineComponent.h"
#include "Actor/EchoNodeComponent.h"
#include "EchoPiecewiseBezierComponent.h"
#include "EchoPavementComponent.h"
#include "EchoCrossRoadComponent.h"
#include "EchoRoadManager.h"
#include "Actor/EchoInOutsideDivisionComponent.h"
#include "EchoCurvedMeshComponent.h"
#include "EchoIKComponentInclude.h"
#include "EchoPhysicsComponentInclude.h"
//#include "Interactor/EchoInteractorManager.h"

#include "EchoActorEntityComponent.h"
#include "EchoUnknownComponent.h"
#include "EchoBoxComponent.h"
#include "EchoLightComponent.h"
#include "EchoDirecLightComponent.h"
#include "EchoDecalComponent.h"
#include "EchoParticleSystemComponent.h"
#include "EchoActorRoleComponent.h"
#include "EchoSoundComponent.h"
#include "EchoActorScriptComponent.h"
#include "EchoNodeProFuncImp.h"
#include "EchoPhysicsManager.h"
#include "EchoRoleTagPointPosComponent.h"
#include "EchoJointComponent.h"
#include "EchoFixedJointComponent.h"
#include "EchoConfigurableJointComponent.h"
#include "EchoShowTrajectoryComponent.h"
#include "EchoMusicControllerComponent.h"
//#include "EchoInteractorComponent.h" 
//#include "EchoInteractorIKComponent.h"
//#include "EchoInteractorTargetComponent.h"
//#include "EchoInteractorObjectComponent.h"
//#include "EchoInteractorAutoMoverComponent.h"
//#include "EchoInteractorPathMoverComponent.h"
#include "EchoPlanetConverterComponent.h"
#include "EchoBendHorizonComponent.h"
#include "EchoOceanComponent.h"
#include "EchoHLODComponent.h"
#include "EchoVegetationAreaComponent.h"
#include "EchoImposterComponent.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"

#include "EchoMaterialComponent.h"
#include "EchoControllerComponent.h"

#include "EchoRagdollData.h"
#include "EchoRagdollComponent.h"
#include "EchoRagdollCreatorComponent.h"
#include "EchoRagdollControllerComponent.h"
#include "EchoRagdollBehaviorManagerComponent.h"

#include "3DUI/Echo3DUIButtonComponent.h"
#include "3DUI/Echo3DUIComboBoxComponent.h"
#include "3DUI/Echo3DUIComponent.h"
#include "3DUI/Echo3DUINodeComponent.h"
#include "3DUI/Echo3DUIGraphComponent.h"
#include "3DUI/Echo3DUIGroupComponent.h"
#include "3DUI/Echo3DUIImageComponent.h"
#include "3DUI/Echo3DUIInputTextComponent.h"
#include "3DUI/Echo3DUILabelComponent.h"
#include "3DUI/Echo3DUIListComponent.h"
#include "3DUI/Echo3DUILoaderComponent.h"
#include "3DUI/Echo3DUIMovieclipComponent.h"
#include "3DUI/Echo3DUIProgressBarComponent.h"
#include "3DUI/Echo3DUIRichTextComponent.h"
#include "3DUI/Echo3DUIScrollBarComponent.h"
#include "3DUI/Echo3DUISliderComponent.h"
#include "3DUI/Echo3DUISWFComponent.h"
#include "3DUI/Echo3DUITextComponent.h"
#include "3DUI/Echo3DUITreeViewComponent.h"
#include "3DUI/Echo3DUIComponentManager.h"

#include "EchoOcclusionComponent.h"
#include "EchoSunComponent.h"
#include "EchoTodTimelineComponent.h"
#include "EchoSkyComponent.h"
#include "EchoCloudSphereComponent.h"
#include "EchoCloudPlaneComponent.h"
#include "EchoCloudMeshComponent.h"
#include "EchoTODComponentManager.h"
#include "EchoFogComponent.h"
#include "EchoWindPowerComponent.h"
#include "EchoUnderWaterCausticsComponent.h"
#include "EchoLightningComponent.h"
#include "Actor/EchoResLoaderComponent.h"
#include "EchoSpotLightComponent.h"
#include "EchoSheetComponent.h"
#include "EchoGlobalLightComponent.h"
#include "EchoHDRComponent.h"
#include "EchoVolumeLightComponent.h"

#include "EchoKeyBoard.h"
#include "Actor/EchoActorManager.h"
#include "EchoSceneComponentManager.h"
#include "EchoControlTodChangeComponent.h"

#include "EchoStarfieldComponent.h"
#include "EchoDriftComponent.h"
#include "EchoSSAOComponent.h"
#include "EchoEnvironmentLightComponent.h"
#include "EchoPointlightManagerComponent.h"
#include "EchoLensFlareComponent.h"
#include "EchoLensFlare.h"
#include "EchoEnvTriggerEventComponent.h"
#include "EchoEnvTriggerEvent.h"
#include "EchoSceneTriggerData.h"
#include "EchoLightFlareComponent.h"
#include "EchoLightFlareComponentManager.h"
#include "EchoEntityBatchComponent.h"
#include "EchoBuildingComponent.h"
#include "EchoWeatherCoverColorComponent.h"
#include "EchoStarsComponent.h"
#include "EchoEmitterComponent.h"
#include "EchoTODComponentSystem.h"
#include "EchoTODManager.h"
#include "EchoCameraBaseComponent.h"
#include "EchoCameraComponentManager.h"
#include "EchoPhysXManager.h"
#include "EchoPhysicsManager.h"
#include "EchoWorldSystem.h"
#include "EchoCurvedRoleComponent.h"
#include "EchoCSGComponent.h"
#include "EchoSphericalTerrainComponent.h"
#include "EchoAtlasSphericalTerrainComponent.h"
#include "EchoVoxelTerrainComponent.h"
#include "EchoTODShadowComponent.h"
#include "EchoMediaComponent.h"
#include "EchoAnimationTimelineComponent.h"
#include "EchoNewDecalComponent.h"
#include "EchoHDRAdaptiveComponent.h"
#include "EchoKarstTerrainComponent.h"
#include "EchoPCGGraphComponent.h"
#include "EchoKarstCityRoadComponent.h"

#include "Actor/EchoLocalQuadTreeComponent.h"
#include "EchoDestructibleComponent.h"
#include "EchoPlanetEntryControlComponent.h"
#include "EchoSphereFogComponent.h"
#include "EchoDistantBuildingComponent.h"
#include "EchoFunctionalBuildingComponent.h"
#include "EchoColorFilterComponent.h"
#include "EchoPCGGraphComponentManager.h"
#include "EchoHerdComponent.h"
#include "PCG/EchoInstancedStaticMeshComponent.h"
#include "PCG/EchoPCGComponent.h"
#include "PCG/Reflection/EchoPCGEnum.h"
#include "EchoFractalComponent.h"
#include "EchoLocalRadialBlurComponent.h"
#include "EchoSphericalOceanComponent.h"
#include "EchoNpcDialogComponent.h"

//-------------------SUToEchoCom----------------
#include "EchoAtmosphereComponent.h"
#include "SUToEchoCom/EchoExcelDataHelper.h"
#include "SUToEchoCom/SUWrapEntityComponent.h"
#include "SUToEchoCom/SUActorFactoryComponent.h"
#include "SUToEchoCom/SUDynamicFactorySwitcherComponent.h"
#include "SUToEchoCom/SUDynamicLoadComponent.h"
#include "EchoFlyingController.h"
#include "EchoSqliteMgr.h"
//----------------------------------------------

//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------
namespace Echo
{
//#define SCRIPT_INIT_S			R"(function class(super, iscpp)
//									local class_type = {}
//									local class_type_metatable = { __index = class_type }
//									class_type.ctor = false
//									class_type.super = super
//									class_type.iscpp = iscpp
//									class_type.delete = function(t)
//										t.__safe_shield = nil
//										do
//											local del
//											del = function(c)
//												local dtor = rawget(c, 'dtor')
//												if dtor then
//													dtor(t)
//												end
//												if c.super and not c.iscpp then
//													del(c.super)
//												end
//											end
//											del(class_type)
//										end
//									end
//									class_type.lnew = function(cinst, ...)
//										local tbl = setmetatable({}, class_type_metatable)
//										tbl.this = cinst
//										tbl.__safe_shield=true
//										local vtbl = setmetatable({}, {
//											__index = function(t, k)
//												if not tbl[k] then
//													local v = tbl.this[k]
//													if v then
//														if type(v) == 'function' then
//															tbl[k] = function(obj, ...) return v(tbl.this, ...) end
//														else
//															tbl[k] = v
//														end
//													end
//												end
//												return tbl[k]
//											end
//											,
//											__newindex = function(t, k, v)
//												if tbl[k] then
//													tbl[k] = v
//												elseif tbl.this[k] then
//													tbl.this[k] = v
//												else
//													tbl[k] = v
//												end
//											end
//											})
//										vtbl.lua_class = class_type
//										do
//											local create
//											create = function(c, ...)
//												if c.super and not c.iscpp then
//													create(c.super, ...)
//												end
//												local ctor = rawget(c, 'ctor')
//												if ctor then
//													ctor(vtbl, ...)
//												end
//											end
//											create(class_type, ...)
//										end
//										return vtbl
//									end
//									if iscpp then
//										class_type.new = function(...)
//											if not class_type.super then
//												error('class error : super class is not exist')
//											end
//											local tbl = setmetatable({}, class_type_metatable)
//											tbl.this = class_type.super(...)
//											tbl.__safe_shield=true
//											local vtbl = setmetatable({}, {
//												__index = function(t, k)
//													if not tbl[k] then
//														local v = tbl.this[k]
//														if v then
//															if type(v) == 'function' then
//																tbl[k] = function(obj, ...) return v(tbl.this, ...) end
//															else
//																tbl[k] = v
//															end
//														end
//													end
//												return tbl[k]
//												end
//												,
//												__newindex = function(t, k, v)
//													if tbl[k] then
//														tbl[k] = v
//													elseif tbl.this[k] then
//														tbl.this[k] = v
//													else
//														tbl[k] = v
//													end
//												end
//												})
//											do
//												local create
//												create = function(c, ...)
//													if c.super and not c.iscpp then
//														create(c.super, ...)
//													end
//													local ctor = rawget(c, 'ctor')
//													if ctor then
//														ctor(obj, ...)
//													end
//												end
//												create(class_type, ...)
//											end
//											return vtbl
//										end
//									else
//										class_type.new = function(...)
//											local obj = setmetatable({}, class_type_metatable)
//											obj.__safe_shield=true
//											do
//												local create
//												create = function(c, ...)
//													if c.super and not c.iscpp then
//														create(c.super, ...)
//													end
//													local ctor = rawget(c, 'ctor')
//													if ctor then
//														ctor(obj, ...)
//													end
//												end
//												create(class_type, ...)
//											end
//											return obj
//										end
//										if super then
//											setmetatable(class_type, { __index = function(t,k)
//												local ret = class_type.super[k]
//												class_type[k] = ret
//												return ret
//											end
//											})
//										end
//									end
//									return class_type
//								end)"




	ComponentSystem::ComponentSystem()
		: mRegisterList(COMSYSTEMREMEMPTYLIST)
	{
	  new TODComponentSystem();
	  new TimeLineInfoManager();
	  new SceneComponentManager();
	  new TimeLineInfoLoadingTood();
	  new AnimationTimeLineInfoManager();
	  new RoadHelper();
	  new RoadManager();
	  new LightFlareComSystem();
	  new CameraComponentManager();

	  new EchoPhysicsSystem();
	  new EchoIKSystem();

	  new RagdollDataManager();
	  new PCGSystem();
	  new PCGV2::PCGGraphManager();
	}
	ComponentSystem::~ComponentSystem()
	{
		deinit();
		delete TimeLineInfoManager::instance();
		delete SceneComponentManager::instance();
		delete TimeLineInfoLoadingTood::instance();

		delete AnimationTimeLineInfoManager::instance();
		delete CameraComponentManager::instance();
		if (mRegisterList & SUTOECHOCOMREGISTER)
		{
			delete ExcelDataManager::instance();
			delete SqliteMgr::instance();
		}

		delete RoadManager::instance();
		delete RoadHelper::instance();
		//delete RoadManager::instance();
		if (EchoFlyingController::isCreated)
			delete EchoFlyingController::instance();

		delete TODComponentSystem::instance();
		delete LightFlareComSystem::instance();
		delete EchoPhysicsSystem::instance();
		delete EchoIKSystem::instance();

		delete RagdollDataManager::instance();
		delete PCGSystem::instance();
		delete PCGV2::PCGGraphManager::instance();

		PCGV2::PCGEnumGlobals::ReleaseAllRegisteredEnums();
	}

	void ComponentSystem::init(bool qtInit/* = false*/, bool echo3DUI/* = false*/)
	{
		if (echo3DUI)
			Echo3DUIRegisterFactory();
		registerFactory();

		//LogManager::instance()->logMessage(LML_CRITICAL, "Echo STSCRIPT register!");
		//GetScriptManager()->init();
		//GetScriptManager()->executeScript(SCRIPT_INIT_S);
		//scriptRegCommon();
		
		//ActorSystem::instance()->getMainActorManager()->registerAdminCom(Name("InOutsideDivisionComponent"), &InOutsideDivisionComponent::EventProc);
		initProtocolFunction();
		if (!qtInit)
		{
			qtNotInitProtocolFunction();
			//多场景相关
			initManager(Echo::Name("Main"));
		}
		
		ActorSystem::instance()->isQtEditor = qtInit;
		if (qtInit)
			mRegisterList |= QTANDECHODIFFPROFUNCTION;
		if (echo3DUI)
			mRegisterList |= ECHO3UICOMREGISTER;

		new EchoKeyBoard();
	}

	void ComponentSystem::init(ComSystemRegisterList list)
	{
		mRegisterList = list;
		if(mRegisterList & ECHO3UICOMREGISTER)
			Echo3DUIRegisterFactory();

		if (mRegisterList & SUTOECHOCOMREGISTER)
		{
			new SqliteMgr();
			SqliteMgr::instance()->init("config/SqliteConfig.cfg", true);
			new ExcelDataManager(); //SU组件获取表数据管理类
			registerSUToEchoComFactory();
		}

		registerFactory();

		if (mRegisterList & ECHOTESTSCRIPTREGISTER)
		{
			LogManager::instance()->logMessage(LML_CRITICAL, "Echo STSCRIPT register!");
			/*GetScriptManager()->init();
			GetScriptManager()->executeScript(SCRIPT_INIT_S);
			scriptRegCommon();*/
		}

		initProtocolFunction();

		ActorSystem::instance()->isQtEditor = getIsQtInit();
		if (!getIsQtInit())
		{
			qtNotInitProtocolFunction();
			//多场景相关
			initManager(Echo::Name("Main"));
		}

		if (mRegisterList & ECHOCOMKEYBOARDREGISTER)
		{
			LogManager::instance()->logMessage(LML_CRITICAL, "Echo com keyboard register!");
			new EchoKeyBoard();
		}
	}

	void ComponentSystem::shutDown()
	{
		ActorSystem::instance()->shutDown();
	}

	void ComponentSystem::deinit()
	{
		if (!getIsQtInit())
		{
			//多场景相关
			deinitManager(Echo::Name("Main"));
			qtNotUninitProtocolFunction();
		}
			
		if (getIsEcho3DUI())
			delete UI3DComponentManager::instance();
			//Echo3DUIComponentProFuncImp::uninitProtocolFunction();
		uninitProtocolFunction();
		if (mRegisterList & ECHOCOMKEYBOARDREGISTER)
		{
			LogManager::instance()->logMessage(LML_CRITICAL, "uninit Echo com keyboard register!");
			delete EchoKeyBoard::instance();
		}
		
	}

	void ComponentSystem::initManager(const Name& name)
	{
		ProtocolComponentManager* mgrPtr = ProtocolComponentSystem::instance()->GetManager(name);
		TODComponentManager* TODMgr = TODComponentSystem::instance()->createTODComponentManager(name, mgrPtr);
		TODMgr->SetWorldManager(WorldSystem::instance()->GetWorldManager(name));
		LightFlareComSystem::instance()->createLightFlareComponentManager(name, mgrPtr);
		EchoIKSystem::instance()->CreateManager(name);
		EchoPhysicsSystem::instance()->CreateManager(name);
		if (!KarstRoadManager::Alive()) {
			new KarstRoadManager();
		}
	}

	void ComponentSystem::deinitManager(const Name& name)
	{
		if (KarstRoadManager::Alive()) {
			delete KarstRoadManager::instance();
		}
		EchoPhysicsSystem::instance()->DestroyManager(name);
		EchoIKSystem::instance()->DestroyManager(name);
		LightFlareComSystem::instance()->destroyLightFlareComponentManager(name);
		TODComponentSystem::instance()->destroyTODComponentManager(name);
	}

	void ComponentSystem::initProtocolFunction()
	{
		//LogManager::instance()->logMessage(LML_CRITICAL, "init echo protocol funciton register!");
		EchoActorProFuncImp::initProtocolFunction();
		CameraProFunctionImp::initProtocolFunction();
		MaterialProFunctionImp::initProtocolFunction();
		EchoTrajectoryProFuncImp::initProtocolFunction();
		EchoTimelineProFuncImp::initProtocolFunction();
		EchoControllerProFuncImp::initProtocolFunction();
		RoleObjProFunctionImp::initProtocolFunction();

		//REGISTEROMFACTORY(DecalComponent);
		/*EchoBoxColliderProFuncImp::initProtocolFunction();
		EchoCapsuleColliderProFuncImp::initProtocolFunction();
		EchoSphereColliderProFuncImp::initProtocolFunction();
		EchoRigidBodyProFuncImp::initProtocolFunction();
		EchoPhysicsSystemProFuncImp::initProtocolFunction();		
		EchoIKManagerProFuncImp::initProtocolFunction();
		EchoInteractorManagerProFuncImp::initProtocolFunction();*/
		//PiecewiseBezierComProFunc::initProtocolFunction();
		//BezierCurveComProFunc::initProtocolFunction();
		CrossRoadComProFunc::initProtocolFunction();
		//PavementComProFunc::initProtocolFunction();
		CurvedMeshComponentProFunc::initProtocolFunction();
		//EchoMeshColliderProFuncImp::initProtocolFunction();
		//EchoCharatorControllerProFuncImp::initProtocolFunction();
		//EchoConstantForceProFuncImp::initProtocolFunction();
		//EchoBoxProFuncImp::initProtocolFunction();
		if (Echo::Root::instance()->getRuntimeMode() == Echo::Root::eRM_Editor)
			ResBaseProFunctionImp::initProtocolFunction();
		//auto p = ProtocolComponentManager::instance();
		EchoNodeProFuncImp::initProtocolFunction();
		//REGISTEROMFACTORY(ConfigurableJointComponent);
		
		EchoBoxComponentProFuncImp::initProtocolFunction();
		EchoTodComponentMgrProFuncImp::initProtocolFunction();
	}

	void ComponentSystem::uninitProtocolFunction()
	{
		//LogManager::instance()->logMessage(LML_CRITICAL, "uninit  echo protocol funciton register!");
		if (Echo::Root::instance()->getRuntimeMode() == Echo::Root::eRM_Editor)
			ResBaseProFunctionImp::uninitProtocolFunction();
		EchoTrajectoryProFuncImp::uninitProtocolFunction();
		EchoTimelineProFuncImp::uninitProtocolFunction();
		MaterialProFunctionImp::uninitProtocolFunction();
		CameraProFunctionImp::uninitProtocolFunction();
		EchoActorProFuncImp::uninitProtocolFunction();
		//EchoBoxProFuncImp::uninitProtocolFunction();
		//EchoPhysicsSystemProFuncImp::uninitProtocolFunction();
		//EchoIKManagerProFuncImp::uninitProtocolFunction();
		//EchoInteractorManagerProFuncImp::uninitProtocolFunction();
		
		EchoBoxComponentProFuncImp::uninitProtocolFunction();
		EchoTodComponentMgrProFuncImp::uninitProtocolFunction();
		RoleObjProFunctionImp::uninitProtocolFunction();

	}

	void ComponentSystem::qtNotInitProtocolFunction()
	{
		//LogManager::instance()->logMessage(LML_CRITICAL, "init qtNot protocol funciton register!");
		HelperObjProFuncImp::initProtocolFunction();
		EntityObjProFunctionImp::initProtocolFunction();
		ParticleProFunctionImp::initProtocolFunction();
		//EchoBoxProFuncImp::initProtocolFunction();
		EchoNodeProFuncImp::qtNotInitProtocolFunction();
		EchoActorProFuncImp::qtNotInitProtocolFunction();
		
		EchoSceneComManagerProFuncImp::qtNotInitProtocolFunction();
	}

	void ComponentSystem::qtNotUninitProtocolFunction()
	{
		//LogManager::instance()->logMessage(LML_CRITICAL, "Uninit qtNot protocol funciton register!");
		ParticleProFunctionImp::uninitProtocolFunction();
		EntityObjProFunctionImp::uninitProtocolFunction();
		HelperObjProFuncImp::uninitProtocolFunction();

		EchoSceneComManagerProFuncImp::qtNotUninitProtocolFunction();
	}

	void ComponentSystem::registerFactory()
	{
		//LogManager::instance()->logMessage(LML_CRITICAL, "Echo component register!");
		REGISTEROMFACTORY(NodeComponent)
			REGISTEROMFACTORY(EntityComponent)
			REGISTEROMFACTORY(RoleComponent)
			REGISTEROMFACTORY(ActorScriptComponent)
			REGISTEROMFACTORY(BoxColliderComponent)
			REGISTEROMFACTORY(BoxComponent)
			REGISTEROMFACTORY(CapsuleColliderComponent)
			REGISTEROMFACTORY(CharatorControllerComponent)
			REGISTEROMFACTORY(ConfigurableJointComponent)
			REGISTEROMFACTORY(ConstantForceComponent)
			REGISTEROMFACTORY(ControllerComponent)
			REGISTEROMFACTORY(DecalComponent)
			REGISTEROMFACTORY(DirecLightComponent)
			REGISTEROMFACTORY(FixedJointComponent)
			//REGISTEROMFACTORY(GroupUIComponent)
			//REGISTEROMFACTORY(HingeJointComponent)
			REGISTEROMFACTORY(IKGrounderFBBIKComponent)
			REGISTEROMFACTORY(IKGrounderIKComponent)
			REGISTEROMFACTORY(IKGrounderQuadrupedComponent)
			REGISTEROMFACTORY(IKAimIKComponent)
			REGISTEROMFACTORY(IKFBBIKHeadEffectorComponent)
			REGISTEROMFACTORY(IKTargetFBBIKComponent)
			REGISTEROMFACTORY(BoneRenderComponent)
			REGISTEROMFACTORY(IKPointComponent)
			REGISTEROMFACTORY(IKTargetPointComponent)
			REGISTEROMFACTORY(IKSkeletonPostureComponent)
			REGISTEROMFACTORY(IKLookAtIKComponent)
			REGISTEROMFACTORY(IKEffectorOffsetComponent)
			REGISTEROMFACTORY(IKRecoilComponent)
			REGISTEROMFACTORY(IKInertiaComponent)
			REGISTEROMFACTORY(IKLimbIKComponent)
			REGISTEROMFACTORY(IKCCDIKComponent)
			REGISTEROMFACTORY(IKHitReactionComponent)
			REGISTEROMFACTORY(IKRotationLimitAngleComponent)
			REGISTEROMFACTORY(IKRotationLimitHingeComponent)
			REGISTEROMFACTORY(IKBodyTiltComponent);
			REGISTEROMFACTORY(IKPenetrationAvoidanceComponent);
			REGISTEROMFACTORY(AimControllerComponent);
			REGISTEROMFACTORY(TerrainColliderComponent);
			
			
			//REGISTEROMFACTORY(ImageUIComponent)
			REGISTEROMFACTORY(LightComponent)
			REGISTEROMFACTORY(MaterialComponent)
			REGISTEROMFACTORY(MeshColliderComponent)
			REGISTEROMFACTORY(MeshColliderUNMOVEABLEComponent)
			REGISTEROMFACTORY(ParticleSystemComponent)
			REGISTEROMFACTORY(RigidBodyComponent)
			//REGISTEROMFACTORY(RigidBodyGroupComponent)
			REGISTEROMFACTORY(AggregateComponent)
			REGISTEROMFACTORY(TagPointPosComponent)

			REGISTEROMFACTORY(RagdollComponent)
			REGISTEROMFACTORY(RagdollCreatorComponent)
			REGISTEROMFACTORY(RagdollControllerComponent)
			REGISTEROMFACTORY(RagdollBehaviorManagerComponent)

			REGISTEROMFACTORY(ShowTrajectoryComponent)
			REGISTEROMFACTORY(SoundComponent)
			REGISTEROMFACTORY(SphereColliderComponent)
			//REGISTEROMFACTORY(WheelColliderComponent)
			//REGISTEROMFACTORY(SpringJointComponent)
			//REGISTEROMFACTORY(TextUIComponent)
			REGISTEROMFACTORY(TimelineComponent)
			REGISTEROMFACTORY(TrajectoryComponent)
			REGISTEROMFACTORY(PolylineComponent)
			REGISTEROMFACTORY(MusicControllerComponent)
			REGISTEROMFACTORY(PiecewiseBezierComponent)
			//REGISTEROMFACTORY(BezierCurveComponent)
			REGISTEROMFACTORY(PavementComponent)
			REGISTEROMFACTORY(CurvedMeshComponent)
			REGISTEROMFACTORY(CrossRoadComponent)
			REGISTEROMFACTORY(CurvedRoleComponent)

			REGISTEROMFACTORY(EchoOcclusionComponent)
			REGISTEROMFACTORY(InOutsideDivisionComponent)

			//REGISTEROMFACTORY(InteractorComponent)
			//REGISTEROMFACTORY(InteractorIKComponent)
			//REGISTEROMFACTORY(InteractorTargetComponent)
			//REGISTEROMFACTORY(InteractorObjectComponent)
			//REGISTEROMFACTORY(InteractorAutoMoverComponent)
			//REGISTEROMFACTORY(InteractorPathMoverComponent)

			REGISTEROMFACTORY(UnknownComponent)
			REGISTEROMFACTORY(SunComponent)
			REGISTEROMFACTORY(TodTimelineComponent)

			REGISTEROMFACTORY(SkyComponent)
			REGISTEROMFACTORY(CloudSphereComponent)
			REGISTEROMFACTORY(CloudPlaneComponent)
			REGISTEROMFACTORY(CloudMeshComponent)
			REGISTEROMFACTORY(FogComponent)
			REGISTEROMFACTORY(ResLoaderComponent)
			REGISTEROMFACTORY(SpotLightComponent)
			REGISTEROMFACTORY(SheetComponent)
			REGISTEROMFACTORY(ControlTodChangeComponent)
			REGISTEROMFACTORY(WindPowerComponent)
			REGISTEROMFACTORY(UnderWaterCausticsComponent)
			REGISTEROMFACTORY(LightningComponent)
			REGISTEROMFACTORY(GlobalLightComponent)
			REGISTEROMFACTORY(HDRComponent)
			REGISTEROMFACTORY(VolumeLightComponent)
			REGISTEROMFACTORY(HDRAdaptiveComponent)
			REGISTEROMFACTORY(LocalRadialBlurComponent)
				
			REGISTEROMFACTORY(PlanetConverterComponent)
			REGISTEROMFACTORY(BendHorizonComponent)
			REGISTEROMFACTORY(OceanComponent)
			REGISTEROMFACTORY(HLODComponent)
			REGISTEROMFACTORY(EnvironmentLightComponent)
			REGISTEROMFACTORY(StarfieldComponent)
			REGISTEROMFACTORY(DriftComponent)
			REGISTEROMFACTORY(PointlightManagerComponent)
			if (Root::instance()->m_useSSAODebugCom) { REGISTEROMFACTORY(SSAOComponent) }
			REGISTEROMFACTORY(VegetationAreaComponent)
			REGISTEROMFACTORY(ImposterComponent)
			REGISTEROMFACTORY(LensFlareComponent)
			REGISTEROMFACTORY(EnvTriggerEventComponent)
			REGISTEROMFACTORY(LightFlareComponent)
			REGISTEROMFACTORY(EntityBatchComponent)
			REGISTEROMFACTORY(BuildingComponent)
			REGISTEROMFACTORY(StarsComponent)
			REGISTEROMFACTORY(WeatherCoverColorComponent)
			REGISTEROMFACTORY(EmitterComponent)
			REGISTEROMFACTORY(CameraBaseComponent)
			REGISTEROMFACTORY(SphericalTerrainComponent)
			REGISTEROMFACTORY(PCGGraphComponent)
			REGISTEROMFACTORY(AtlasSphericalTerrainComponent)
			REGISTEROMFACTORY(VoxelTerrainComponent)
			REGISTEROMFACTORY(CSGComponent)
			REGISTEROMFACTORY(AtmosphereComponent)
			REGISTEROMFACTORY(TODShadowComponent)
			REGISTEROMFACTORY(MediaComponent)
			REGISTEROMFACTORY(AnimationTimelineComponent)
			REGISTEROMFACTORY(NewDecalComponent)
			REGISTEROMFACTORY(LocalQuadTreeComponent)
			REGISTEROMFACTORY(DestructibleComponent)
			REGISTEROMFACTORY(PlanetEntryControlComponent)
			REGISTEROMFACTORY(SphereFogComponent)
			REGISTEROMFACTORY(KarstRoadComponent)
			REGISTEROMFACTORY(KarstVoxelComponent)
			REGISTEROMFACTORY(DistantBuildingComponent)
			REGISTEROMFACTORY(FunctionalBuildingComponent)
			REGISTEROMFACTORY(ColorFilterComponent)
			REGISTEROMFACTORY(HerdComponent)
			REGISTEROMFACTORY(InstancedStaticMeshComponent)
			REGISTEROMFACTORY(PCGComponent)
			REGISTEROMFACTORY(KarstCityRoadComponent)
			REGISTEROMFACTORY(SphericalOceanComponent)
			REGISTEROMFACTORY(NpcDialogComponent)
				
			REGISTEROMFACTORY(FractalComponent)
			ActorComFactoryManager::getSingleton().isRegiseterEnd = true;
	}

	void ComponentSystem::Echo3DUIRegisterFactory()
	{
		//LogManager::instance()->logMessage(LML_CRITICAL, "Echo 3DUI component register!");
		// 3DUI
		REGISTEROMFACTORY(Echo3DUIButtonComponent)
		REGISTEROMFACTORY(Echo3DUIComboBoxComponent)
		REGISTEROMFACTORY(Echo3DUIComponent)
		REGISTEROMFACTORY(Echo3DUINodeComponent)
		REGISTEROMFACTORY(Echo3DUIGraphComponent)
		REGISTEROMFACTORY(Echo3DUIGroupComponent)
		REGISTEROMFACTORY(Echo3DUIImageComponent)
		REGISTEROMFACTORY(Echo3DUIInputTextComponent)
		REGISTEROMFACTORY(Echo3DUILabelComponent)
		REGISTEROMFACTORY(Echo3DUIListComponent)
		REGISTEROMFACTORY(Echo3DUILoaderComponent)
		REGISTEROMFACTORY(Echo3DUIMovieclipComponent)
		REGISTEROMFACTORY(Echo3DUIProgressBarComponent)
		REGISTEROMFACTORY(Echo3DUIRichTextComponent)
		REGISTEROMFACTORY(Echo3DUIScrollBarComponent)
		REGISTEROMFACTORY(Echo3DUISliderComponent)
		REGISTEROMFACTORY(Echo3DUISWFComponent)
		REGISTEROMFACTORY(Echo3DUITextComponent)
		REGISTEROMFACTORY(Echo3DUITreeViewComponent)
		//Echo3DUIComponentProFuncImp::initProtocolFunction();
		new UI3DComponentManager();
	}

	void ComponentSystem::registerSUToEchoComFactory()
	{
		//REGISTEROMFACTORY(ActorFactoryComponent)
		//REGISTEROMFACTORY(WrapEntityComponent)
		REGISTEROMFACTORY(DynamicLoadComponent)
		REGISTEROMFACTORY(DynamicFactorySwitcherComponent)
	}
	
} // namespace Echo

//-----------------------------------------------------------------------------
//	Cpp File End
//-----------------------------------------------------------------------------
