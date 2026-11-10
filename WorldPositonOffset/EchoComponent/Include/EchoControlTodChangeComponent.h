#ifndef __ECHO_CONTROLTODCHANGECOMPONENT_H__
#define __ECHO_CONTROLTODCHANGECOMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoProtocolActorComponent.h"
#include "EchoComDefineMacro.h"
//#include "EchoTODComponentManager.h"

namespace Echo 
{

	class ParticleSystemComponent;
	class TODComponentManager;
	struct  TodDataHandle {
		String              mCloudMeshName;
		String              mCloudPlaneName;
		String              mCloudSphereName;
		String              mDirecLightName;
		String              mFogName;
		String              mSkyName;
		String              mSunName;
		String				mOceanName;
		String              mTodTimelineName;
		String              mBoxName;
		String              mSceneName;
		String              mParticleName;
		String				mEnvironmentLightName;
		String				mStarfieldName;
		String				mWindPowerName;
		String				mUnderWaterCausticsName;
		String				mLightningName;
		String              mPointlightMgrName;
		String				mGlobalLightName;
		String				mHDRName;
		String              mEnvTriggerName;
		std::vector<String> mParticleNameVec;
		Vector3             mCameraPosition = Vector3::ZERO;
		uint32	            mSceneID = 0;
		float               mTodTransTime = 0.f;
		float               mCloudTransTime = 0.f;
		bool                mIsChangeScene = false;

		std::string         m_TodActorName = "";
		int                 m_StfID = -1;
		int                 m_Priority = 0;

		bool                m_isEffectTod = false;
	};


	class _EchoComponentExport ControlTodChangeComponentInfo : public ActorComponentInfo
	{
	public:
		ControlTodChangeComponentInfo() {}
		ControlTodChangeComponentInfo(const Name& comType, ActorCOMMoveableType type);
		virtual ~ControlTodChangeComponentInfo();
		virtual void									ExportData(cJSON* jsonNode) override;
		virtual void									ImportData(cJSON* jsonNode) override;
		virtual void									CopyTo(ActorComponentInfo* comInfo) override;
		virtual void                                    Overlay(ActorComponentInfo* srcInfo) override;
		virtual void                                    OverWrite() override;
	public:
		TodDataHandle                                   mContext;
		EDITOR_REFLECT
	};

	class _EchoComponentExport ControlTodChangeComponent : public ProtocolActorComponent
	{
		DECLAREACTORCOMTYPE_H(ControlTodChangeComponent)
	public:
		virtual AxisAlignedBox					getLoadBoundingBox() override;
		void						            init(ActorComponentInfo* info) override;
		void                                    unInit() override;
		virtual void							copyTo(const ActorComponentPtr&) override;
		void						            registerTickTimeBegin() override;

		virtual void                            connectCom() override;
		virtual void			                beginFrame(EventID, ProtocolHandle) override;

		void                                    initParticleCom();

		void                                    setIsUseImmediateMode(bool isUse);
		void                                    setIsUseTod(bool isUse);

		void                                    setIsUseCloud(bool isUse);
		void                                    setIsUseCloudImmediateMode(bool isUse);

		void									setStarfieldName(const std::string& starfieldName_);
		inline const std::string&				getStarfieldName() { return mContext.mStarfieldName; }
		void								 	setEnvironmentLightName(const std::string& envLightName_);
		inline const std::string& 				getEnvironmentLightName() { return mContext.mEnvironmentLightName; }
		void                                    setSceneName(const std::string& sceneName_);
		inline const String&                    getSceneName()  { return mContext.mSceneName; }
		void                                    setCloudMeshName(const std::string& cloudMeshName_);
		inline const String&                    getCloudMeshName() { return mContext.mCloudMeshName; }
		void                                    setCloudPlaneName(const std::string& cloudPlaneName_);
		inline const String&                    getCloudPlaneName() { return mContext.mCloudPlaneName; }
		void                                    setCloudSphereName(const std::string& sphereName_);
		inline const String&                    getCloudSphereName() { return mContext.mCloudSphereName; }
		void                                    setDirecLightName(const std::string& direcLightName_);
		inline const String&                    getDirecLightName() { return mContext.mDirecLightName; }
		void                                    setFogName(const std::string& fogName_);
		inline const String&                    getFogName() { return mContext.mFogName; }
		void                                    setSkyName(const std::string& skyName_);
		inline const String&                    getSkyName() { return mContext.mSkyName; }
		void                                    setSunName(const std::string& sunName);
		inline const String&                    getSunName() { return mContext.mSunName; }
		void									setWindPowerName(const String& windPowerName);
		inline const String&					getWindPowerName() { return mContext.mWindPowerName; }
		void									setCausticsName(const String& CausticsName);
		inline const String&					getCausticsName() { return mContext.mUnderWaterCausticsName; }
		void									setLightningName(const String& LightningName);
		inline const String&					getLightningName() { return mContext.mLightningName; }
		void                                    setTodTimelineName(const std::string& todTimelineName_);
		inline const String&                    getTodTimelineName() { return mContext.mTodTimelineName; }
		void                                    setBoxName(const std::string& boxName);
		inline const String&                    getBoxName() { return mContext.mBoxName; }
		void                                    setOceanName(const String& oceanName);
		inline const String&                    getOceanName() { return mContext.mOceanName; }
		void									setGlobalLightName(const String& globalLightName);
		inline const String&					getGlobalLightName() const { return mContext.mGlobalLightName; }
		void									setHDRName(const String& HDRName);
		inline const String&					getHDRName() const { return mContext.mHDRName; }
		
		void                                    setEnvTriggerName(const String& envTriggerName);
		inline const String&                    getEnvTriggerName() { return mContext.mEnvTriggerName; }

		
		void                                    setCameraPosition(const Vector3& cameraPosition_);
		inline const Vector3&                   getCameraPosition() { return mContext.mCameraPosition; }

		void                                    setPointlightManagerName(const String& pLightMgrName);
		inline const String&                    getPointlightManagerName() { return mContext.mPointlightMgrName; };
		void                                    setSceneId(uint32 sceneId_);
		uint32                                  getSceneId() { return mContext.mSceneID; }
		//粒子
		const std::vector<String>&              getParticleVec() { return mContext.mParticleNameVec; }
		void                                    setParticleName(const std::pair<std::string, size_t>& particleData);
		void                                    setParticleSize(size_t sz);
		void                                    setParticleNameVec(const std::vector<std::string>& particleName_);

		float                                   getTransTime() { return mContext.mTodTransTime; }
		void                                    setTransTime(float transTime_) { mContext.mTodTransTime = transTime_; }
		float                                   getCloudTransTime() { return mContext.mCloudTransTime; }
		void                                    setCloudTransTime(float transTime) { mContext.mCloudTransTime = transTime; }
		void                                    initTodVecState(ActorComponentInfo* info);
		//用tod控制器的过渡时间重置tod组件的过渡时间
		void                                    initTransTime();

		inline bool                             getIsCanChangeScene() { return mContext.mIsChangeScene; }
		inline void                             setIsCanChangeScene(bool isCanChange) { mContext.mIsChangeScene = isCanChange; }
		void                                    changeTodEffection(bool isUse);

		void									switchEffect(bool isUse);

		void                                    changeCloudMeshEffection(bool isUse);
		void                                    changeCloudEffection(bool isUse);
		void                                    changeCloudOtherEffection(bool isUse);

		void                                    changeScene();
		bool                                    isPosInBox();
		bool                                    isPosInEpitaxyBox();
		bool									isPosInTripleEpitaxyBox();

		void                                    SetPriority(const Echo::ActorPtr& ptr, int priority);

	    virtual void                            EditorPauseImp(bool pause) override;
		virtual void                            EditorStopImp() override;
		TODComponentManager*                    getTodComManager() { return m_tMgr; }
		template <typename T>
		T comlerp(const T& t1, const T& t2, float factor);

		void                                    setStfId(int id) { mContext.m_StfID = id; }
		int                                     getStfID() { return mContext.m_StfID; }
		void                                    setTodActorFilePath(const std::string& path_);
		const std::string&                      getTodActorFilePath() { return mContext.m_TodActorName; }
		int                                     getPriority() { return mContext.m_Priority; }
		void                                    setComPriority(int priority_) { mContext.m_Priority = priority_; }

		void                                    setIsEffectTod(bool isEffect) { mContext.m_isEffectTod = isEffect; }
		bool                                    getIsEffectTod() { return mContext.m_isEffectTod; }
		void                                    updateActiveTod();
    protected:
		void                                    _updateActiveTod(bool isUpdateActor);
	public:
		   enum {
			   CloudMesh_Type,
               CloudPlane_Type,
			   CloudSphere_Type,
			   DirecLight_Type,
			   Fog_Type,
			   Sky_Type,
			   Ocean_Type,
			   Starfield_Type,
			   EnvironmentLight_Type,
			   Sun_Type,
			   TodTime_Type,
			   Box_Type,
 			   Wind_type,
			   Caustics_type,
			   PointLightMgr_type,
			   Lightning_type,
			   GlobalLight_type,
			   HDR_type,
			   EnvTrigger_type,
			   Vec_End
			  
		   };
	   private:
		   std::vector<ActorComponent*>         mTodComVec;
		   std::vector<ParticleSystemComponent*> mParticleComVec;
		   bool                                 mIsTrigger = false;
		   bool									mIsTriggerStarfield = false;
		   TodDataHandle                        mContext;
		   TODComponentManager*                 m_tMgr = nullptr;
             
		   ActorPtr                             m_TodActor;
	};

	template <typename T>
	T ControlTodChangeComponent::comlerp(const T& t1, const T& t2, float factor)
	{
		return t1 + factor * (t2 - t1);
	}

	DEFINECOMFACTORY(ControlTodChangeComponent, UNMOVEABLE)
}

#endif
