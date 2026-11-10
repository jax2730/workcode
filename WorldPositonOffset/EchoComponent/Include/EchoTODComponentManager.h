#ifndef __ECHO_TODCOMPONENT_MANAGER_H__
#define __ECHO_TODCOMPONENT_MANAGER_H__
#include "EchoCommonStableHeaders.h"
#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"
#include "EchoProtocolComponent.h"
#include "EchoTextureManager.h"

namespace Echo {

	class TODComponentSystem;

	class FogComponent;
	class DirecLightComponent;
	class BoxComponent;
	class SkyComponent;
	class Sky;
	struct SkyComponentContext;
	class CloudSphereComponent;
	class CloudSphere;
	struct CloudSphereComponentContext;
	class CloudPlaneComponent;
	class CloudPlane;
	struct CloudPlaneComponentContext;
	class CloudMeshComponent;
	class CloudMesh;
	struct CloudMeshHandle;
	struct CloudMeshComponentContext;
	class SunComponent;
	class Sun;
	struct sunHandle;
	class EnvironmentLightComponent;
	struct EnvironmentLightHandle;
	class OceanComponent;
	class Ocean;
	struct OceanComponentContext;
	class WindPowerComponent;
	struct WindPowerHandle;
	class UnderWaterCausticsComponent;
	struct CausticsHandle;
	class LightningComponent;
	struct LightningHandle;
	class PointlightManagerComponent;
	struct pLightManagerHandle;
	class GlobalLightComponent;
	struct GlobalLightHandle;
	class HDRComponent;
	struct HDRHandle;
	class TodTimelineComponent;
	class EnvTriggerEventComponent;
	class DriftComponent;
	struct DriftPropData;
	struct TriggrtEventComponentData;
	class SceneTriggerData;
	class StarsComponent;
	class Stars;
	struct StarsComponentHandle;
	class VolumeLightComponent;
	struct VolumeLightComponentContext;
	class HDR;
	class SphericalOcean;
	struct SphericalOceanContext;
	class SphericalOceanComponent;

	class WeatherCoverColorComponent;
	struct WeatherCoverColorComData;

	class TODShadowComponent;
	struct TodShadowDataHandle;

	class HDRAdaptiveComponent;
	struct HDRAdaptiveHandle;

	class  SphereFogComponent;
	struct SphereFogComHandle;

	class  ControlTodChangeComponent;

	struct TodComManagerHandle {
		WorldManager*             mWorldManager       = nullptr;
		SceneManager*             mSceneManager       = nullptr;
		WeatherManager*           mWeatherManager     = nullptr;
		TODManager*               mTodManager         = nullptr;
		ProtocolComponentManager* mProtocolComManager    = nullptr;
	};

	template<typename ComponentType, typename TODType>  //ComponentType -- 组件类型, TODType -- Tod类型（例如 Sun）
	struct TODContext {
		std::vector<ComponentType*>	  mComponentVec;              //存储当前类型组件所有指针的优先队列  
		TODType*                      mDefaultSceneObj = nullptr; //记录组件效果生效时，场景中的Tod指针
		ComponentType*                mActiveCom = nullptr;       //记录当前生效的组件指针
		bool						  mIsImmediateCom = false;    //记录是否开启立即模式
		bool                          mIsUseCurrType = false;     //记录是否启用该类型的组件效果（默认不启用）
		bool                          mIsUse = true;              //用于标识调用updateContext的时候，该组件是否使用（用作客户端切换天气逻辑）
		bool                          mIsTranslation = false;     //标识是否启用过渡效果
	};

	struct DirecLightHandle {
		ColorValue                    mPBRMainLightColor;
		ColorValue                    mPBRSkyLightColor;
		ColorValue                    mPBRGroundLightColor;
		ColorValue                    mRoleMainLightColor;
		ColorValue	                  mRoleSkyLightColor;
		ColorValue                    mRoleGroundLightColor;
		Vector3                       mMainLightDirection;
		Vector3                       mMainLightShadowDirection;
		Vector3                       mSunDirection;
		ColorValue                    mMainLightColor;
		ColorValue                    mMainSkyLightColor;
		ColorValue                    mMainGroundLightColor;
		float                         mPBRAmbientBlendThreshold;
		float                         mRoleAmbientBlendThreshold;
		float                         m_ViewLightPercent = 0.05f;
		float						  m_PBRViewLightPercent = 0.05f;
		float						  m_RoleViewLightPercent = 0.05f;
	};
	struct FogHandle {
		ColorValue                    mFogColor[8];
		float                         mDensity[8];
		float                         mRampStart[8];
		float                         mRampEnd[8];
		float                         mTopHeight[8];
		FogMode                       mFogMode;
	};
	struct EnvironmentLightHandle{
		float		                  mEnvironmentExplosure;
		std::string                   mEnvironmentTextureName;
		std::string                   mEnvironmentShcFileName;
	};

	//新类型 -- 需要加到 T_DirecLight 与 T_End 之间
	enum   TodComType {
		//cloudMesh -- cloudPlane -- cloudSphere的枚举需要在一起
		T_CloudMesh,
		T_CloudPlane,
		T_CloudSphere,
		T_DirecLight,
		T_EnvironmentLight,
		T_GlobalLight,
		T_HDR,
		T_HDRAdaptive,
		T_Ocean,
		T_PointLight,
		T_Stars,
		T_UnderWaterCaustics,
		T_WeatherCoverColor,
		T_WindPower,
		T_Shadow,
		T_Fog,
		T_Sky,
		T_Sun,
		T_Drift,
		T_SphereFog,
		T_SphericalOcean,
		T_End
	};
	enum  TodComTransState {
		S_NoneTrans,
	    S_Trans
	};
	enum CloudFadingState {
		Cloud_None,//过渡结束
		Cloud_Pre,//准备开始过渡
		Cloud_Out,//淡出
		Cloud_In,//淡入
		Cloud_No_Out,
		Cloud_End
	};

	class _EchoComponentExport TODComponentManager : public ProtocolComponent  //public Singleton<TODComponentManager>,
	{
		friend  class EchoTodComponentMgrProFuncImp;
		friend  class TODComponentSystem;
		friend  class EnvTriggerEventComponent;
		friend  class ControlTodChangeComponent;
	public:
		TODComponentManager(const Name& name, ProtocolComponentManager* pMgr);

		TODComponentManager(const Name& name, const TodComManagerHandle& mgrHandle_);

		virtual                    ~TODComponentManager();

		//天气粒子第二个组相关
		bool                        getIsUseSecondGroup() { return mIsUseSecondGroup; }
		void                        setIsUseSecondGroup(bool isUse_);

		void						SetWorldManager(WorldManager* worldMgr) {mWorldManager = worldMgr;}
		void                        SetSceneManager(SceneManager* sceneManager) { mSceneManager = sceneManager; }
		void                        SetTodManager(TODManager* tMgr) { mTodManager = tMgr; }
		void                        SetWeatherManager(WeatherManager* wMgr) { mWeatherManager = wMgr; }

		void                        SetManagerByHandle(const TodComManagerHandle& mgrHandle_);

		SceneManager*               GetSceneManager() { return mSceneManager; }
		TODManager*                 GetTODManager() { return mTodManager; }
		WeatherManager*             GetWeatherManager() { return mWeatherManager; }
		WorldManager*               GetWorldManager() { return mWorldManager; }

		static TODComponentManager* instance();

		virtual void                getProtocolType(Name& type) override;
		virtual void                getProtocolTypeToString(std::string& typeStr) override;

		void						registerTickTimeBegin() override;
		void                        initTodTransData();
		void                        uninitTodTransData();
		void                        deinitTodComData();

		virtual void                beginFrame(EventID, ProtocolHandle) override;
		virtual void			    preUpdateScene(EventID, ProtocolHandle) override;
		//修改生效的天气
		//void                        setWeatherTypeByCom(int uType, float transTime , const std::string& weatherName, bool isChangeSameAttr = false, bool bTransition = true, bool isDay = true);
		void                        setWeatherTypeByCom(int uType, float transTime = 10.f,bool isChangeSameAttr = false, bool bTransition = true, bool isDay = true);
		void                        getWeatherTransStartState();
		//方向光
		void                        addDirecLight(DirecLightComponent* item);
		void                        deleteDireclight(DirecLightComponent*);
        void                        DirecLightPriorityChanged(DirecLightComponent* item);
		void                        setDirecLightIsUse(bool isUse_);
		bool                        getDirecLightIsUse() const { return mIsUseDirecLight; }
		void                        updateDirecLight(DirecLightComponent* direcLightCom_); //除了删除的情况，其他都传入一个空指针
		void                        setCurrSunDirection(const Vector3& dir_);
		void                        getDefaultDirecLightHandle();
		void                        setDefaultDirecLightHandle();

		void                        setDirecLightRealTime(bool isRealTime, DirecLightComponent* direcCom);
		bool                        getDirecLightRealTime(DirecLightComponent* direcCom);
		DirecLightComponent*        getCurrActiveDirecLightCom() { return mCurrActiveDirecCom; }
		void                        setCurrActiveDirecLightCom(DirecLightComponent* direcCom) { mCurrActiveDirecCom = direcCom; }

		//雾
		void                        addFog(FogComponent* item);
		void                        deleteFog(FogComponent*);
		void                        FogPriorityChanged(FogComponent* item);
        void                        setFogIsUse(bool isUse_);
		bool                        getFogIsUse() const { return mIsUseFog; }
        void                        updateFog(FogComponent* fogCom_);//除了删除的情况，其他都传入一个空指针
		void                        getDefaultFogHandle();
		void                        setDefauleFogHandle();

		void                        setFogRealTime(bool isRealTime, FogComponent* fogCom);
		bool                        getFogRealTime(FogComponent* fogCom_);
		FogComponent*               getCurrActiveFogCom() { return mCurrActiveFogCom; }
		void                        setCurrActiveFogCom(FogComponent* fogCom) { mCurrActiveFogCom = fogCom; }


		//控制tod系列组件是否打log
		void                        setIsWriteLog(bool isWrite_) { mIsWriteLog = isWrite_; }
		bool                        getIsWriteLog() { return mIsWriteLog; }

		void						setIsStopUpdate(bool b) { mIsStopUpdate = b; }

		//天气指针相关

		void                        createCloudSphere();
		void                        deleteCloudSphere();
		CloudSphere*                getComCloudSphere() { return mCloudSphere; }

		void                        createCloudPlane();
		void                        deleteCloudPlane();
		CloudPlane*                 getComCloudPlane() { return mCloudPlane; }

		void                        createCloudMesh();
		void                        deleteCloudMesh();
		CloudMesh*                  getCloudUpMesh() { return mCloudMeshUp; }
		CloudMesh*                  getCloudMidMesh() { return mCloudMeshMid; }
		CloudMesh*                  getCloudDownMesh() { return mCloudMeshDown; }

		void                        createSky();
		void                        deleteSky();
		Sky*                        getSky() { return mSky; }

		void                        createSun();
		void                        deleteSun();
		Sun*                        getSun() { return mSun; }

		void						createOcean();
		void						deleteOcean();
		Ocean*						getOcean() { return mOcean; }

		void                        createStars();
		void                        deleteStars();
		Stars*                      getStars() { return mStars; }

		HDR*						getHDR();
		SphericalOcean*				getCurrentSphericalOcean();

		//getContext
		TODContext<SkyComponent, Sky>&                     getSkyContext() { return mSkyContext; }
		TODContext<CloudSphereComponent, CloudSphere>&     getCloudSphereContext() { return mCloudSphereContext; }
		TODContext<CloudPlaneComponent, CloudPlane>&       getCloudPlaneContext() { return mCloudPlaneContext; }
		TODContext<CloudMeshComponent, CloudMeshHandle>&   getCloudMeshContext() { return mCloudMeshContext; }
		TODContext<SunComponent, Sun>&                     getSunContext() { return mSunContext; }
		TODContext<EnvironmentLightComponent, EnvironmentLightHandle>& getEnvironmentLightContext() { return mEnvironmentLightContext; }
		TODContext<OceanComponent, Ocean>&				   getOceanContext() { return mOceanContext; }
		TODContext<WindPowerComponent, WindPowerHandle>&   getWindPowerContext() { return mWindPowerContext; }
		TODContext<UnderWaterCausticsComponent, CausticsHandle>& getUnderWaterCausticsContext(){ return mUnderWaterCausticsContext;}
		TODContext<LightningComponent, LightningHandle>&   getLightningContext() { return mLightningContext; }
		TODContext<PointlightManagerComponent, pLightManagerHandle>& getPointlightMgrContext() { return mPointlightMgrContext; }
		TODContext<GlobalLightComponent, GlobalLightHandle>& getGlobalLightContext() { return mGlobalLightContext; }
		TODContext<HDRComponent, HDRHandle>&				getHDRContext(){ return mHDRContext;}
		TODContext<EnvTriggerEventComponent, TriggrtEventComponentData>& getEnvTriggerContext() { return mEnvTriggerContext; }
		TODContext<DriftComponent, DriftPropData>&          getDriftContext() { return mDriftContext; }
		TODContext<StarsComponent, Stars>&                  getStarsContext() { return mStarsContext; }
		TODContext<WeatherCoverColorComponent, WeatherCoverColorComData>& getWeatherCoverColorContext() { return  mCoverColorContext; }
		TODContext<VolumeLightComponent, VolumeLightComponentContext>& getVolumeLightContext() { return mVolumeLightContext; }
		TODContext<TODShadowComponent, TodShadowDataHandle>& getTODShadowContext() { return mTODShadowContext; }
		TODContext<SphereFogComponent, SphereFogComHandle>&  getSphereFogContext() { return mSphereFogContext; }
		TODContext<HDRAdaptiveComponent, HDRAdaptiveHandle>& getHDRAdaptiveContext() { return mHDRAdaptiveContext; }
		TODContext<SphericalOceanComponent, SphericalOceanContext>& getSphericalOceanContext() { return mSphericalOceanContext; }
	
	private:
		//保持场景对象与使用场景对象的函数
		void						saveDefaultObj(TODContext<SkyComponent, Sky>& context);
		void						useDefaultObj(TODContext<SkyComponent, Sky>& context);

		void						saveDefaultObj(TODContext<CloudSphereComponent, CloudSphere>& context);
		void						useDefaultObj(TODContext<CloudSphereComponent, CloudSphere>& context);

		void						saveDefaultObj(TODContext<CloudPlaneComponent, CloudPlane>& context);
		void						useDefaultObj(TODContext<CloudPlaneComponent, CloudPlane>& context);

		void						saveDefaultObj(TODContext<CloudMeshComponent, CloudMeshHandle>& context);
		void						useDefaultObj(TODContext<CloudMeshComponent, CloudMeshHandle>& context);

		void						saveDefaultObj(TODContext<SunComponent, Sun>& context);
		void						useDefaultObj(TODContext<SunComponent, Sun>& context);

		void						saveDefaultObj(TODContext<EnvironmentLightComponent, EnvironmentLightHandle>& context);
		void						useDefaultObj(TODContext<EnvironmentLightComponent, EnvironmentLightHandle>& context);
	
		void						saveDefaultObj(TODContext<OceanComponent, Ocean>& context);
		void						useDefaultObj(TODContext<OceanComponent, Ocean>& context);

		void						saveDefaultObj(TODContext<WindPowerComponent, WindPowerHandle>& context);
		void						useDefaultObj(TODContext<WindPowerComponent, WindPowerHandle>& context);

		void						saveDefaultObj(TODContext<UnderWaterCausticsComponent, CausticsHandle>& context);
		void						useDefaultObj(TODContext<UnderWaterCausticsComponent, CausticsHandle>& context);

		void                        saveDefaultObj(TODContext<PointlightManagerComponent, pLightManagerHandle>& context);
		void						useDefaultObj(TODContext<PointlightManagerComponent, pLightManagerHandle>& context);

		void						saveDefaultObj(TODContext<LightningComponent, LightningHandle>& context);
		void						useDefaultObj(TODContext<LightningComponent, LightningHandle>& context);

		void						saveDefaultObj(TODContext<GlobalLightComponent, GlobalLightHandle>& context);
		void						useDefaultObj(TODContext<GlobalLightComponent, GlobalLightHandle>& context);

		void						saveDefaultObj(TODContext<HDRComponent, HDRHandle>& context);
		void						useDefaultObj(TODContext<HDRComponent, HDRHandle>& context);
		
		void                        saveDefaultObj(TODContext<EnvTriggerEventComponent, TriggrtEventComponentData>& context);
		void                        useDefaultObj(TODContext<EnvTriggerEventComponent, TriggrtEventComponentData>& context);

		void						saveDefaultObj(TODContext<DriftComponent, DriftPropData>& context);
		void						useDefaultObj(TODContext<DriftComponent, DriftPropData>& context);

		void						saveDefaultObj(TODContext<StarsComponent, Stars>& context);
		void						useDefaultObj(TODContext<StarsComponent, Stars>& context);

		void						saveDefaultObj(TODContext<WeatherCoverColorComponent, WeatherCoverColorComData>& context);
		void						useDefaultObj(TODContext<WeatherCoverColorComponent, WeatherCoverColorComData>& context);

		void						saveDefaultObj(TODContext<VolumeLightComponent, VolumeLightComponentContext>& context);
		void						useDefaultObj(TODContext<VolumeLightComponent, VolumeLightComponentContext>& context);

		void						saveDefaultObj(TODContext<TODShadowComponent, TodShadowDataHandle>& context);
		void						useDefaultObj(TODContext<TODShadowComponent, TodShadowDataHandle>& context);


		void						saveDefaultObj(TODContext<SphereFogComponent, SphereFogComHandle>& context);
		void						useDefaultObj(TODContext<SphereFogComponent, SphereFogComHandle>& context);

		void						saveDefaultObj(TODContext<HDRAdaptiveComponent, HDRAdaptiveHandle>& context);
		void						useDefaultObj(TODContext<HDRAdaptiveComponent, HDRAdaptiveHandle>& context);

		void						saveDefaultObj(TODContext<SphericalOceanComponent, SphericalOceanContext>& context);
		void						useDefaultObj(TODContext<SphericalOceanComponent, SphericalOceanContext>& context);
	public:
		template<class T>
		void                        addTODItem(T* item, std::vector<T*>& resVec);
		template<class T>
		void                        TODItemPriorityChanged(T* item, std::vector<T*>& resVec);
		//将 TOD 组件加入管理类的队列
		template<typename ComponentType, typename TODType>
		void						pushComponent(TODContext<ComponentType, TODType>& context, ComponentType* com);

		//将 TOD 组件从管理类的队列移除
		template<typename ComponentType, typename TODType>
		void						popComponent(TODContext<ComponentType, TODType>& context, ComponentType* com);

		// 调整 TOD 组件优先级
		template<typename ComponentType, typename TODType>
		void						adjustComponentPriority(TODContext<ComponentType, TODType>& context, ComponentType* com);

		//更新上下文信息, 决定组件是否 unuse, use。
		template<typename ComponentType, typename TODType>
		void						updateContext(TODContext<ComponentType, TODType>& context, ComponentType* com = nullptr);
		
		//返回是否是立即模式
		template<typename ComponentType, typename TODType>
		bool						getIsImmediateComponent(const TODContext<ComponentType, TODType>& context, ComponentType* com) const;

		//设置 com 组件立即模式
		template<typename ComponentType, typename TODType>
		void						enableImmediateComponent(TODContext<ComponentType, TODType>& context, ComponentType* com); 
		//取消组件立即模式
		template<typename ComponentType, typename TODType>
		void						cancelImmediateComponent(TODContext<ComponentType, TODType>& context);
		//启用该类型组件效果
		template<typename ComponentType, typename TODType>
		void                        setIsUseThisType(TODContext<ComponentType, TODType>& context,bool isUse);
	
	public:
		//AreaManager当前生效的数据
		void                        setTodComMgrSceneTriggerData(SceneTriggerData* sceneDataHelper_);
		//角色位置
		const Vector3&              getRolePosition() { return mRolePosition; }
		void                        setRolePosition(const Vector3& rolePosition_) { mRolePosition = rolePosition_; }
		//过渡效果相关
		void                        setTransState(TodComTransState  state_);
		void						setTransStateEnd();
		void                        setFinalTodCom(ActorComponent*  todCom_, TodComType  type_);
		void                        updateCloud();
		void                        updateTod();

		void						clearComInFinalComVec(ActorComponent* todCom_, TodComType  type_);
		void                        clearFinalComVec();
		void                        setFogTranslation(bool isTrans_) { mIsFogTranslation = isTrans_; }
		void                        setDirecLightTranslation(bool isTrans_) { mIsDirecLightTranslation = isTrans_; }
		
		float                       getTodTransTime();
		float                       getCloudTransTime();

		void                        addTodComVec(TodTimelineComponent* todTime_);
		void                        delTodComVec(TodTimelineComponent* todTime_);
		void                        clearAllTodComVec();

		//缓存texture贴图指针
		std::map<std::string, TexturePtr>& getTodTexturePtrMap() { return mTexturePtrMap; }
		void                        addTodTexturePtr(const std::string& name_,const TexturePtr& texturePtr);
		void                        deleteTodTexturePtr(const std::string& name_);
		void                        clearUpTodTexturePtrMap();
		void                        getTodTexturePtrByName(const std::string& name_,TexturePtr& texPtr);

		//缓存Mesh指针
		void						addCloudMeshPtr(const String& name, const MeshPtr& mesh);
		void						deleteCloudMeshPtr(const String& name);
		void						clearCloudMeshPtrMap();
		void						getCloudMeshPtrPtrByName(const String& name, MeshPtr& mesh);

		//客户端使用接口
		void                        setTodDayTime(double t);
		void                        setOverlayPosAndRadius(const Vector3& pos_,float inRadius_,float outRadius_);
		void                        setIsUseOverlay(bool isUse);

		void                        setTodPlaySpeed(float s);
		std::vector<TodTimelineComponent*>& getTodComVec();
		void                        enterOrLeaveClientArea();
		void                        closeAllTodCom();
		void                        setTodComIsUse(Echo::ActorComponent* comp, bool b);

		void                        setTodComChangeState(TodComType type_);

		//重力相关
		void                        setTODGravityData(const Vector3& gravityDir_ = Vector3::UNIT_Y, float gravityHeight_ = 0.f, const Vector3& gravityPos_ = Vector3::ZERO);
		void                        setGravity(const Vector3& gravity_);
		void                        setIsUseGravityDirByType(int type_,bool isUse_ = true);
		void                        setIsUseGravityPosByType(int type_,bool isUse_ = true);
		void                        setIsUseGravityHeightByType(int type_, bool isUse_ = true);
		void                        useAllGravityType();
		void                        unUseAllGravityType();
		void                        setGravityData();

		//设置过渡时间（客户端）
		void                        setTransFormTimeFromClient(float time_);
		float                       getTransFormTimeFromClient() { return mTransTimeByClient; }

		void                        setTodTransTime(float time_);
		void                        setCloudTransTime(float time_);

		bool                        getIsEffectCoverColor() { return mIsEffectCoverColor; }
		void                        setIsEffectCoverColor(bool isEffect_) { mIsEffectCoverColor = isEffect_; }

		bool						getIsUseTodCommodify() { return mIsUseTodComModify; }

		void                        setAllTodComIsUse(bool isUse_);
		template <typename T>
		T comlerp(const T& t1, const T& t2, float factor);

		//获取当前生效的天气id
		int                         getWeatherId() { return mTodId; }
		//tick相关
		void                        setIsUseEchoTick(bool isUse_) { mIsUseEchoTick = isUse_; }
		bool                        getIsUseEchoTick() { return mIsUseEchoTick; }
		//log
		void                        showTodActiveData();

		bool						isUseMaskHdrData() { return mIsUseMaskHdrData; }
		void						setUseMaskHdrData(bool isUse) { mIsUseMaskHdrData = isUse; }

		void                        processCacheData();

		void                        setSceneArtData(SceneArtData* artData_) { mCurrSceneArtData = artData_; }

		bool*						getEffectiveModifyState() { return &mIsEffectiveModify; }
	private:
		void                         _setTodDayTimeAttribute(bool bSetTod_ = false);

		bool                         _judgeIsUseCloudMesh(Cloud* pCloud);

		void                         _dealWithFirstTick();
		void                         _getTransFinalState(bool isGetAll = true);

		void                         _setSphereFogAttribute(float percent);
		void                         _getSphereFogAttribute(SphereFogComHandle& pData);

		void                         _setSkyAttribute(float percent);
		void                         _setFogAttribute(float percent);
		void                         _setDirecLightAttribute(float percent);
		void                         _setSunAttribute(float percent);
		void						 _setDriftAttribute(float percent);
		void                         _cloudSphereFadeIn(float percent,Cloud* pCloud);
		void                         _cloudSphereFadeOut(float percent, Cloud* pCloud);
		void                         _setCloudSphereAttribute(CloudSphereComponentContext* sphereContext,Cloud* pCloud);


		void                         _cloudPlaneFadeIn(float percent, Cloud* pCloud);
		void                         _cloudPlaneFadeOut(float percent, Cloud* pCloud);
		void                         _setCloudPlaneAttribute(CloudPlaneComponentContext* planeContext,Cloud* pCloud);

		void                         _setCloudMeshData(const CloudMeshComponentContext& rhs,float percent,Cloud* pCloud);
		void                         _setCloudMeshDataBytype(const CloudMeshComponentContext& pData, int type,Cloud* pCloud);
		void                         _setCloudMeshTemplateDataByType(const CloudMeshComponentContext& pData, int type, Cloud* pCloud);
		void                         _getCloudMeshDataByType(CloudMeshComponentContext& pData,int type,Cloud* pCloud,bool isGetAllData = true);

		void                         _cloudMeshFadeIn(float percent, Cloud* pCloud);
		void                         _cloudMeshFadeOut(float percent, Cloud* pCloud);

		void                         _getSceneAttributeData();

		void                         _getSkyAttribute(SkyComponentContext& mSkyData_);
		void                         _getFogAttribute(FogHandle& mFogData_);
		void                         _getSceneDirecLightData(DirecLightHandle&);
		void                         _getSunAttribute(sunHandle& mSunData_);

		void                         _setCloudTodTimelineState(bool isUse);

		void                         _getCloudData(Cloud* pCloud);
		//参数为true时会获取当前云海的所有属性值，为false时只获取tod属性值
		void                         _getCloudMeshData(CloudMeshComponentContext&  cloudMeshData_, Cloud* pCloud, bool isGetAllData = true);
		void                         _getCloudSphereData(CloudSphereComponentContext& cloudSphereData_, Cloud* pCloud);
		void                         _getCloudPlaneData(CloudPlaneComponentContext& cloudPlaneData_,Cloud* pCloud);

		void                         _setTodComAttributeInFirstTrans(); //用来处理不用过渡的组件的生效状态更新

		void                         _setCloudSeaParamByType(Cloud* pCloud, int type, uint32 index, SceneArtData* pData);

		void                         _StfLoadFinishCallBack(void* data);


		void                         _AddControlTodComVec(ControlTodChangeComponent* com);
		void                         _RemoveControlTodComVec(ControlTodChangeComponent* com);
		ControlTodChangeComponent*   _GetFirstControlCom();

		// 仅记录与设置TODManager内进行过渡的参数
#define SETTODCOMTRANS(Name, Type)	\
		void	_set##Name##Attribute(float percent);	\
		void	_get##Name##Attribute(Type& data);	\
		Type*	m##Name##Handle = nullptr;	\
		Type*	mFinal##Name##Handle = nullptr;

		SETTODCOMTRANS(EnvironmentLight, EnvironmentLightHandle)
		SETTODCOMTRANS(GlobalLight, GlobalLightHandle)
		SETTODCOMTRANS(HDR, HDRHandle)
		SETTODCOMTRANS(HDRAdaptive, HDRAdaptiveHandle)
		SETTODCOMTRANS(Ocean, OceanComponentContext)
		SETTODCOMTRANS(PointLight, pLightManagerHandle)
		SETTODCOMTRANS(Stars, StarsComponentHandle)
		SETTODCOMTRANS(UnderWaterCaustics, CausticsHandle)
		SETTODCOMTRANS(WeatherCoverColor, WeatherCoverColorComData)
		SETTODCOMTRANS(WindPower, WindPowerHandle)
		SETTODCOMTRANS(TODShadow, TodShadowDataHandle)
		SETTODCOMTRANS(SphericalOcean, SphericalOceanContext)
#undef SETTODCOMTRANS


	private:
        //角色位置		
		Vector3                                                 mRolePosition;
        
		//客户端设置过来的过渡时间
		float                                                   mTransTimeByClient = 0.f;

		//是否影响覆盖色过渡时间
		bool                                                    mIsEffectCoverColor = true;

		TodComTransState                                        mTransState = S_NoneTrans;
		std::vector<ActorComponent*>                            mFinalComVec;
		std::vector<TodTimelineComponent*>                      mTodTimelineVec;

  		//过渡已流逝的时间
		float                                                   mCloudElapsed = 0.f;
		float                                                   mTodElapsed = 0.f;

		//控制云的淡入淡出 0 - cloudMesh | 1 - cloudPlane | 2 - cloudSphere
		CloudFadingState                                        mCloudState[3];

		//控制是否更新
		bool                                                    mIsUpdateCloud = false;
		bool                                                    mIsUpdateTod = false;

		//记录过渡开始时的tod组件的属性值
		SkyComponentContext*                                    mSkyData = nullptr;
		FogHandle*                                              mFogData = nullptr;
		DirecLightHandle*                                       mDirecData = nullptr;
		sunHandle*                                              mSunData = nullptr;
		CloudMeshComponentContext*                              mCloudMeshHandle = nullptr;
		CloudSphereComponentContext*                            mCloudSphereHandle = nullptr;
		CloudPlaneComponentContext*                             mCloudPlaneHandle = nullptr;
		SphereFogComHandle*                                     mSphereFogData = nullptr;

		//记录过渡状态最终的属性值
		SkyComponentContext*                                    mFinalSkyData = nullptr;
		FogHandle*                                              mFinalFogData = nullptr;
		DirecLightHandle*                                       mFinalDirecData = nullptr;
		sunHandle*                                              mFinalSunData = nullptr;
		CloudMeshComponentContext*                              mFinalCloudMeshHandle = nullptr;
		CloudSphereComponentContext*                            mFinalCloudSphereHandle = nullptr;
		CloudPlaneComponentContext*                             mFinalCloudPlaneHandle = nullptr;
		SphereFogComHandle*                                     mFinalSphereFogData = nullptr;

		//记录过渡开始时三层云海的使用情况
		bool                                                    mIsUpCloudMeshUse = false;
		bool                                                    mIsMidCloudMeshUse = false;
		bool                                                    mIsDownCloudMeshUse = false;

		//过渡时间
		std::vector<float>                                      mTodTransTime;
		bool                                                    mIsTodTrans[TodComType::T_End];    // 记录上次的过渡状态
		bool                                                    mTodTransState[TodComType::T_End]; // 记录当前的过渡状态
		bool                                                    mTodComIsChange[TodComType::T_End];
		bool                                                    mIsFogTranslation = true;
		bool                                                    mIsDirecLightTranslation = true;

		//标识是否为过渡开始时的第一次tick
		bool                                                    mIsFirstTodTick = false;
    private:
		//标识是否打log
		bool                                                    mIsWriteLog = false;
		bool													mIsStopUpdate = false;
		std::vector<DirecLightComponent*>						mDirecVec;
		bool													mIsUseDirecLight = false;
		DirecLightHandle*										mDirecLightHandle = nullptr;
		ColorValue												mSunDefaultColor;
		
		FogHandle*												mFogHandle = nullptr;
		std::vector<FogComponent*>								mFogVec;
		bool													mIsUseFog = false;
        //处理切换
		bool                                                    mIsFogActive = true;
		bool                                                    mIsDirecActive = true;

		ColorValue												mWaterShallowColor;
		ColorValue												mWaterDeepColor;
		ColorValue												mWaterFoamColor;
		ColorValue												mWaterReflectColor;
		ColorValue												mUnderWaterColor;

		//当前生效的组件
		DirecLightComponent*                                    mCurrActiveDirecCom = nullptr;
		FogComponent*                                           mCurrActiveFogCom = nullptr;
		bool                                                    mIsDirecRealTime = false;
		bool                                                    mIsFogRealTime = false;

		bool													mIsUseTodComModify = false;
		bool													mIsEffectiveModify = false;		// 是否修改当前生效的天气效果
		bool													mUnUseAllGravity = false;
		bool                                                    mIsChangeTodStfID = false;
		bool													mIsOcean = true;
		bool													mIsOceanInit = false;
		bool													mHaveSphericalOcean = false;

		float                                                   mElapsedTime = 0.f;

		//缓存
		bool                                                    mIsTodDataLoadFinish = true; //记录场景是否初始化完成

		bool                                                    mIsTransSameAttr = false;
		//记录每帧的变化时间
		float                                                   mTimeElapsed = 0.f;
	private:
		TODContext<SkyComponent, Sky>						    mSkyContext;
		TODContext<CloudSphereComponent, CloudSphere>		    mCloudSphereContext;
		TODContext<CloudPlaneComponent, CloudPlane>			    mCloudPlaneContext;
		TODContext<CloudMeshComponent, CloudMeshHandle>		    mCloudMeshContext;
		TODContext<SunComponent, Sun>							mSunContext;
		TODContext<EnvironmentLightComponent, EnvironmentLightHandle> mEnvironmentLightContext;    
		TODContext<OceanComponent, Ocean>						mOceanContext;
		TODContext<WindPowerComponent, WindPowerHandle>         mWindPowerContext;
		TODContext<UnderWaterCausticsComponent, CausticsHandle> mUnderWaterCausticsContext;
		TODContext<PointlightManagerComponent, pLightManagerHandle> mPointlightMgrContext;
		TODContext<LightningComponent, LightningHandle>			mLightningContext;
		TODContext<GlobalLightComponent, GlobalLightHandle>		mGlobalLightContext;
		TODContext<HDRComponent, HDRHandle>						mHDRContext;
		TODContext<EnvTriggerEventComponent, TriggrtEventComponentData> mEnvTriggerContext;
		TODContext<DriftComponent, DriftPropData>               mDriftContext;
		TODContext<StarsComponent, Stars>                       mStarsContext;
		TODContext<WeatherCoverColorComponent, WeatherCoverColorComData> mCoverColorContext;
		TODContext<VolumeLightComponent, VolumeLightComponentContext>	mVolumeLightContext;
		TODContext<TODShadowComponent, TodShadowDataHandle>      mTODShadowContext;
		TODContext<HDRAdaptiveComponent, HDRAdaptiveHandle>		mHDRAdaptiveContext;

		TODContext<SphereFogComponent, SphereFogComHandle>      mSphereFogContext;
		TODContext<SphericalOceanComponent, SphericalOceanContext> mSphericalOceanContext;

		//天气指针
		CloudSphere*                                            mCloudSphere = nullptr;
		CloudPlane*                                             mCloudPlane = nullptr;
		CloudMesh*                                              mCloudMeshUp = nullptr;
		CloudMesh*                                              mCloudMeshMid = nullptr;
		CloudMesh*                                              mCloudMeshDown = nullptr;
		Sky*                                                    mSky = nullptr;
		Sun*                                                    mSun = nullptr;
		Ocean*													mOcean = nullptr;
		Stars*                                                  mStars = nullptr;

		//缓存当前Areamanager生效的SceneTriggerData		
		SceneTriggerData*                                       mSceneTriggerData = nullptr;

		bool                                                    mIsChangeWeatherByClient = false;
		double                                                  mTodTime = 0.0;

		std::map<std::string, TexturePtr>                       mTexturePtrMap;
		std::map<std::string, MeshPtr>							mMeshPtrMap;

		bool                                                    mIsUseEchoTick = true;
		int                                                     mTodId = -1;

		//多场景相关
		Name                                                    mName = Name::g_cEmptyName;
		WorldManager*											mWorldManager = nullptr;
		SceneManager*                                           mSceneManager = nullptr;
		WeatherManager*                                         mWeatherManager = nullptr;
		TODManager*                                             mTodManager = nullptr;

		bool                                                    mIsUseSecondGroup = true;


		Vector3                                                 mOverlayPosition = Vector3::ZERO;
		float                                                   mOverlayInRadiu = 0.f;
		float                                                   mOverlayOutRadiu = 40.f;
		bool                                                    mIsUseOverlay = false;

		bool													mIsUseMaskHdrData = false;

		//记录当前生效的模板指针
		SceneArtData*                                           mCurrSceneArtData = nullptr;

		//球体雾 平面雾相关
		bool                                                    mIsUseTodSphereFog = false;
		bool                                                    mIsNotifyFogMgr = false; //是否要通知球体雾管理器（通知条件 ：从无到有 || 从有到无）

		//tod控制器组件相关
		std::vector<ControlTodChangeComponent*>                 mControlComVec;
    };


	template<class T>
	void       TODComponentManager::addTODItem(T* item,  std::vector<T*>& resVec)
	{
		if (item == nullptr)
			return;
		auto iter = std::find(resVec.begin(), resVec.end(), item);
		if (iter != resVec.end())
			return;
		resVec.resize(resVec.size() + 1);
		int currPriority = item->getPriority();
		if (resVec.size() == 1)
		{
			resVec[0] = item;
			return;
		}
		int sz = static_cast<int>(resVec.size());
		for (int i = sz - 2; i >= 0; i--)
		{
			int tempPriority = resVec[i]->getPriority();
			if (tempPriority <= currPriority)
			{
				resVec[i + 1] = resVec[i];
			}
			else
			{
				resVec[i + 1] = item;
				return;
			}
		}
		resVec[0] = item;
	}

	template<class T>
	inline void TODComponentManager::TODItemPriorityChanged(T * item, std::vector<T*>& resVec)
	{
		if (item == nullptr)
			return;

		if (resVec.size() == 1)
			return;
		auto iter = std::find(resVec.begin(), resVec.end(), item);
		if (iter == resVec.end())
			return;
		int idx = static_cast<int>(iter - resVec.begin());
		int sz = static_cast<int>(resVec.size());

		for (int i = idx; i >= 1; i--)
		{
			int currPriority = resVec[i]->getPriority();
			int prePriority = resVec[i - 1]->getPriority();
			if (currPriority >=  prePriority)
			{
				auto temp = resVec[i];
				resVec[i] = resVec[i - 1];
				resVec[i - 1] = temp;
				temp = nullptr;
			}
			else
			{
				if (i != idx)
					return;
			}
		}

		for (int i = idx; i < sz - 1; i++)
		{
			int currPriority = resVec[i]->getPriority();
			int lastPriority = resVec[i + 1]->getPriority();
			if (currPriority < lastPriority)
			{
				auto temp = resVec[i];
				resVec[i] = resVec[i + 1];
				resVec[i + 1] = temp;
				temp = nullptr;
			}
			else
				return;
		}
	}

	template<typename ComponentType, typename TODType>
	void	TODComponentManager::pushComponent(TODContext<ComponentType, TODType>& context, ComponentType* com) {
		if (nullptr == com) {
			return;
		}
		addTODItem<ComponentType>(com, context.mComponentVec);
		updateContext(context);
	}

	template<typename ComponentType, typename TODType>
	void	TODComponentManager::popComponent(TODContext<ComponentType, TODType>& context, ComponentType* com) {
		if (nullptr == com) {
			return;
		}

		// 尝试删除
		auto targetItor = std::find(context.mComponentVec.begin(), context.mComponentVec.end(), com);
	
		if (targetItor != context.mComponentVec.end()) {
			ComponentType* delCom = *targetItor;
			// 如果立即模式下删除的恰好是激活的，需取消立即模式
			if (delCom == context.mActiveCom) {
				context.mIsImmediateCom = false;
				context.mActiveCom->unuse();
				context.mActiveCom = nullptr;
			}
			context.mComponentVec.erase(targetItor);
		}
		else {
			return;
		}
		updateContext(context);
	}

	template<typename ComponentType, typename TODType>
	void	TODComponentManager::adjustComponentPriority(TODContext<ComponentType, TODType>& context, ComponentType* com) {
		if (nullptr == com) return;

		TODItemPriorityChanged<ComponentType>(com, context.mComponentVec);
		updateContext(context);
	}

	template<typename ComponentType, typename TODType>
	void	TODComponentManager::updateContext(TODContext<ComponentType, TODType>& context, ComponentType* com) {
		//若启用了过渡效果，且未开始过渡
		if ((context.mIsTranslation && mIsFirstTodTick) || !mIsEffectiveModify)
			return;
		if (context.mIsUseCurrType == false)
		{
			if (context.mActiveCom)
			{
				context.mActiveCom->unuse();
				context.mActiveCom = nullptr;
			}
			context.mIsImmediateCom = false;
			useDefaultObj(context);
			return;
		}

		// 保存场景默认设置
		if (context.mDefaultSceneObj == nullptr) {
			saveDefaultObj(context);
		}

		// 如果组件数组为空， 则判断当前激活组件是否为空，若不为空，应当 unuse 后，置空， 使用默认场景对象
		if (context.mComponentVec.empty()) {
			if (context.mActiveCom) {
				context.mActiveCom->unuse();
				context.mActiveCom = nullptr;
				context.mIsImmediateCom = false;
			}
			useDefaultObj(context);
		}
		else {
			if (context.mIsImmediateCom) {
				if (com) {
					if (com == context.mActiveCom) 
						return;
					if (context.mActiveCom) {
						context.mActiveCom->unuse();
					}

					context.mActiveCom = com;
					if (mIsChangeTodStfID == false)
						context.mActiveCom->use();
					else
						context.mIsUse = false;
				}
			}
			else {
				// 如果不是立即模式, 判断是否需更新激活组件
				if (context.mActiveCom != context.mComponentVec[0]) {
					if (context.mActiveCom)
					{
				       context.mActiveCom->unuse();
				       context.mActiveCom=nullptr;

			        }
					context.mActiveCom = context.mComponentVec[0];
					if (mIsChangeTodStfID == false)
						context.mActiveCom->use();
					else
						context.mIsUse = false;
				}
			}
		}
	}

	template<typename ComponentType, typename TODType>
	bool	TODComponentManager::getIsImmediateComponent(const TODContext<ComponentType, TODType>& context,ComponentType* com) const {
		return context.mIsImmediateCom && context.mActiveCom == com;
	}

	template<typename ComponentType, typename TODType>
	void	TODComponentManager::enableImmediateComponent(TODContext<ComponentType, TODType>& context, ComponentType* com) {
		if(context.mIsUseCurrType == false)
		   setIsUseThisType(context,true);
		context.mIsImmediateCom = true;
		if(context.mActiveCom != com)
		   updateContext(context, com);
	}

	template<typename ComponentType, typename TODType>
	void	TODComponentManager::cancelImmediateComponent(TODContext<ComponentType, TODType>& context) {
		context.mIsImmediateCom = false;
		updateContext(context);
	}

	template<typename ComponentType, typename TODType>
	inline void TODComponentManager::setIsUseThisType(TODContext<ComponentType, TODType>& context, bool isUse)
	{
		context.mIsUseCurrType = isUse;

		updateContext(context);	
	}

	template <typename T>
	T TODComponentManager::comlerp(const T& t1, const T& t2, float factor)
	{
		return t1 + factor * (t2 - t1);
	}




	class _EchoComponentExport EchoTodComponentMgrProFuncImp
	{
	public:
		static Echo::ProtocolResult  ProCom_TodChangeCompleted(void* data, ProtocolHandle);
		static Echo::ProtocolResult  ProCom_TodResetTodComDate(void* date, ProtocolHandle);
		static Echo::ProtocolResult  ProCom_CloseAllTodCom(void* data, ProtocolHandle);
		static Echo::ProtocolResult  ProCom_GetAreaManagerSceneDataHelper(void* data, ProtocolHandle);
		static Echo::ProtocolResult  ProCom_SetTodComTickWay(void* data,ProtocolHandle);
		static Echo::ProtocolResult  ProCom_ShowTodCurrActiveComData(void* data,ProtocolHandle);
		static Echo::ProtocolResult  ProCom_ProcessCacheData(void* data, ProtocolHandle);
		static Echo::ProtocolResult  ProCom_ShowLogData(void* data, ProtocolHandle);
		static Echo::ProtocolResult	 ProCom_ReplaceSTF(void* data, ProtocolHandle);
		static void                  initProtocolFunction();
		static void                  uninitProtocolFunction();
	};
};

#endif
