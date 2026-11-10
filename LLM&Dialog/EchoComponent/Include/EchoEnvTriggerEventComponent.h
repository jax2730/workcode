#ifndef __ECHO_ENVTRIGGEREVENT_COMPONENT_H__
#define __ECHO_ENVTRIGGEREVENT_COMPONENT_H__

#include "EchoPrerequisites.h"
#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"
#include "Actor/EchoProtocolActorComponent.h"
 
namespace Echo {
	struct EnvTriggerEventDesc;
	class  EnvTriggerEvent;
	class  SceneHeightArea;

	using TriggerPair = std::pair<int, EnvTriggerEventDesc>;
	struct TriggrtEventComponentData {
		int		                               mUpdateMode = 0;
		std::vector<EnvTriggerEventDesc>       mTriggerEventData;
		bool                                   mIsRaining = false;
		bool                                   mIsSnowing = false;
		int                                    mPriority = 0;
		std::vector<float>                     mYrotateAngle;

		int                                    mSecondUpdateMode = 0;
		std::vector<EnvTriggerEventDesc>       mSecondTriggerEventData;
		std::vector<float>                     mSecondYrotateAngle;
#ifdef ECHO_EDITOR
		std::vector<std::string>               mTriggerEventName;
		std::vector<std::string>               mSecondTriggerEventName;
#endif
	};

	class TODComponentManager;

	class  _EchoComponentExport EnvTriggerEventComponentInfo : public ActorComponentInfo
	{
	public:
		EnvTriggerEventComponentInfo() = default;
		EnvTriggerEventComponentInfo(const Name& comType, ActorCOMMoveableType type);
		void								   ExportData(cJSON* jsonNode) override;
		void								   ImportData(cJSON* jsonNode) override;
		void								   CopyTo(ActorComponentInfo* comInfo) override;
		void								   OverWrite() override;

#ifdef ECHO_EDITOR
		virtual bool                           getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif
	public:
		TriggrtEventComponentData              mContext;
		EDITOR_REFLECT
	};

	class _EchoComponentExport EnvTriggerEventComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(EnvTriggerEventComponent)
			friend class TODComponentManager;
	public:
		virtual void					       init(ActorComponentInfo* info) override;
		virtual void                           unInit() override;
		virtual void                           visibleStateChanged(bool isVisible) override;

		void                                   use();
		void                                   unuse();
        

		//void                                   setParticleOverlayData(const Vector3& pos_, float inRadius_, float outRadius_);


		void                                   initEnvTriggerEventData();
 		void                                   update(float elapsedTime);
		void                                   resetEvent(int idx);

		void                                   resetSecondEvent(int idx);

		void                                   addTriggerEvent();
		void                                   deleteTriggerEvent(int idx);

		void                                   addSecondTriggerEvent();
		void                                   deleteSecondTriggerEvent(int idx);

		void                                   setRainState(bool isRain_);
		bool                                   getRainState() { return mContext.mIsRaining; }
		void                                   setSnowState(bool isSnow);
		bool                                   getSnowState() { return mContext.mIsSnowing; }

		void                                   setTriggerUpdateModel(int mode_);
		int                                    getTriggerUpdateModle() { return mContext.mUpdateMode; }

		void                                   setSecondTriggerUpdateModel(int mode_) { mContext.mSecondUpdateMode = mode_; }
		int                                    getSecondTriggerUpdateModle() { return mContext.mSecondUpdateMode; }

		void                                   setTriggerEventAllData(const std::vector<EnvTriggerEventDesc>& triggerData_);
		void                                   setTriggerEventAllData(const std::vector<EnvTriggerEventDesc>& triggerData_, int i);
		void                                   setTriggerEventAllData(const std::vector<EnvTriggerEventDesc>& triggerData_, bool isAdd, int i);

		void                                   setSecondTriggerEventAllData(const std::vector<EnvTriggerEventDesc>& triggerData_);
		void                                   setSecondTriggerEventAllData(const std::vector<EnvTriggerEventDesc>& triggerData_, int i);
		void                                   setSecondTriggerEventAllData(const std::vector<EnvTriggerEventDesc>& triggerData_, bool isAdd, int i);

		void                                   setTriggerEventData(const std::pair<int, EnvTriggerEventDesc>& triggerDataDesc_);
		void                                   setSecondTriggerEventData(const std::pair<int, EnvTriggerEventDesc>& triggerDataDesc_);

		
		const std::vector<EnvTriggerEventDesc>& getTriggerEventData() { return mContext.mTriggerEventData; }
		const std::vector<EnvTriggerEventDesc>& getSecondTriggerEventData() { return mContext.mSecondTriggerEventData; }

		bool                                   getIsUseSecondTriggerGroup();

#ifdef ECHO_EDITOR
		void                                   setTriggerEventName(const std::vector<std::string>& triggerEventName_);
		const std::vector<std::string>&        getTriggerEventName() { return  mContext.mTriggerEventName; }

		void                                   setSecondTriggerEventName(const std::vector<std::string>& triggerEventName_){ mContext.mSecondTriggerEventName = triggerEventName_; }
		const std::vector<std::string>&        getSecondTriggerEventName() { return  mContext.mSecondTriggerEventName; }
#endif
		void                                   setYRotateAngle(const std::vector<float>& yRotateAngle_);
		const std::vector<float>&              getYRotateAngle() { return mContext.mYrotateAngle; }

		void                                   setSecondYRotateAngle(const std::vector<float>& yRotateAngle_) { mContext.mSecondYrotateAngle = yRotateAngle_; }
		const std::vector<float>&              getSecondYRotateAngle() { return mContext.mSecondYrotateAngle; }

		int                                    getPriority() { return mContext.mPriority; }
		void                                   setPriority(int priority_);
		TODComponentManager*                   getTodComManager() { return m_tMgr; }
		const TriggrtEventComponentData&	   getContext() { return mContext; }

	private:
		void                                   _initEnvTriggerEventData(std::vector<EnvTriggerEvent*>& envTriggerEvent_,const std::vector<EnvTriggerEventDesc>& triggerDesc);
		void                                   _use(std::vector<EnvTriggerEvent*>& envTriggerEvent_, int& currIdx_);
		void                                   _unuse(std::vector<EnvTriggerEvent*>& envTriggerEvent_);
		void                                   _update(float elapsedTime, Vector3 updatePos, Vector3 cameraPos, const Quaternion& cameraQua, std::vector<EnvTriggerEvent*>& envTriggerEvent_,int updateModel_,int& currIdx);
		int                                    _getRandomIdx(const std::vector<EnvTriggerEvent*>& envTriggerEvent_);
		int                                    _getNextIdx(const std::vector<EnvTriggerEvent*>& envTriggerEvent_, int currIdx, int updateModel);
		void                                   _resetEvent(int idx, std::vector<EnvTriggerEvent*>& envTriggerEvent_,int currIdx, int updateModel );
		void                                   _setTriggerEventDataByIdx(const EnvTriggerEventDesc& triggerDataDesc_,std::vector<EnvTriggerEvent*>& envTriggerEvent_, std::vector<EnvTriggerEventDesc>& triggerDesc_, int idx);
		void                                   _createTriggerEventObjectByIdx(std::vector<EnvTriggerEvent*>& envTriggerEvent_, int idx);
		void                                   _setIsUseSecondTriggerGroup(bool isUse_);

		bool                                   _setParticleOverlayData(EnvTriggerEvent* pEvent);
	private:
		std::vector<EnvTriggerEvent*>          mEnvTriggerEventVec;
		TriggrtEventComponentData              mContext;
		int                                    mCurrIdx = -1;
		TODComponentManager*                   m_tMgr = nullptr;

		std::vector<EnvTriggerEvent*>          mSecondEnvTriggerEventVec;
		int                                    mSecondCurrIdx = -1;

		bool                                   mIsActiveCom = false;
	};

	void _EchoComponentExport WriteJSNode(JSNODE jsonNode, const char* Name, const EnvTriggerEventDesc& triggerEventDesc);
	bool _EchoComponentExport ReadJSNode(cJSON* jsonNode, const char* Name, EnvTriggerEventDesc& triggerEventDesc);

	DEFINECOMFACTORY(EnvTriggerEventComponent, UNMOVEABLE)
}
#endif
