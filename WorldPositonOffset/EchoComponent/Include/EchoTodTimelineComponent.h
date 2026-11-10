//-----------------------------------------------------------------------------
// File:   EchoTimelineComponent.h
//
// Author: anhaoxiang
//
// Date:   2022-6-24
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef _EchoTodTimelineComponent_h__
#define _EchoTodTimelineComponent_h__
#include "EchoTimelineComponent.h"

namespace Echo {

	class TODComponentManager;

	class _EchoComponentExport TodTimelineComponentInfo : public TimelineComponentInfo {
	public:
		TodTimelineComponentInfo(const Name& name, ActorCOMMoveableType moveable);
		virtual          ~TodTimelineComponentInfo();
	public:
		
	};

	class _EchoComponentExport TodTimelineComponent : public TimelineComponent {
		  friend class TodTimelineComponentFactory;
		  DECLAREACTORCOMTYPE_H(TodTimelineComponent)
	public:
		enum DayTimeNode {
			SUNRISE,     // 0
			MORNING,     //6
			MID_MORNING, //8
			NOON,        //12
			AFTERNOON,   //17
			Banwan,      //18
			EVENING,     //20
			SUNSET       //24
		};
	public:
		REGISTER_LUA_CLASS_KEY;
		virtual void                init(ActorComponentInfo* info) override;
		virtual void                unInit() override;
		virtual void				copyTo(const ActorComponentPtr& p_actor_component) override;
		virtual bool				isCreateFinish() override;
		virtual void				resourceArrived(const Name& resName, const Name& resType, bool bFail, const ResourceBasePtr& resourcePtr) override;
		virtual void                visibleStateChanged(bool isVisible) override;

		virtual void			    preUpdateScene(EventID, ProtocolHandle) override;
		void                        setDayNode(const DayTimeNode& node_);
		DayTimeNode                 getDayNode() { return mTimeNode; }
		void                        setDayTime(const int& dayTime_,const int& minuteTime_);
		int                         getHour() { return mCurrHour; }
		int                         getMinutes() { return mCurrMinutes; }
		int                         getSeconds() { return mCurrSeconds; }
		void                        updateTime();
		void                        remakeDayTime();
		
		virtual void			    AddKey(size_t vIdx, int weight, float value,bool flag_ = true) override;
		virtual void                AddKey(size_t vIdx, int frame, const std::string& value) override;
		virtual void				SetKeyValue(size_t vIdx, int keyVal, float value) override;
		virtual void                SetKeyValue(size_t vIdx, int keyVal, const std::string& value) override;
		virtual void				MoveKey(size_t vIdx, int oldT, int newT, float value) override;
		virtual void			    DeleteKey(size_t vIdx, int weight) override;

		virtual void                setComponentAttribute() override;
		virtual void                runLoopVideo() override;
		virtual void                setIsPlay(bool isplay_) override;

		void                        getTimeNodeSpeed();
		void                        updataTodPlayState(bool isReduce = false);
		void                        setTodPlaySpeed(float speed_);
		const float&                getTodPlaySpeed() const { return mTodPlaySpeed; }
#ifdef ECHO_EDITOR
		bool                        setTodAttributeData(const std::string& bindName, const std::string& attrName, float* data, const int& size,float& offect,bool isItemHover = false);
		bool                        setTodStringAttributeData(const std::string& comName, const std::string& attrName, const std::string& value, bool isItemHover = false);
		bool                        getSaveState() { return mIsSaveFile; }
		void                        setSaveState(bool state_) { mIsSaveFile = state_; }
		void                        mirrorReplicationOneKey(const size_t& idx);
		void                        mirrorReplicationGroupKey(const size_t& idx);

		void                        mirrorReplicationOneKey();
		void                        mirrorReplicationGroupKey();

		bool                        judgeAttributeIsSave(const std::string& comName_,const Name& attrName_);//判断一个属性是否被保存
#endif  	
	protected:
		virtual void                dealWithOldData() override;
		virtual void                initActorAndComData() override;
	public:
		float                          todProportion = 60.f;
		float                          mPlayStep = 0.f;
	private:
		float                          mTodPlaySpeed = 1.f;
		int                            mCurrHour = 0;
		int                            mCurrMinutes = 0;
		int                            mCurrSeconds = 0;
		DayTimeNode                    mTimeNode;
		std::vector<float>             mTimeNodeSpeeds;
		bool                           isTodEditorPlay = false;
		bool                           mIsControlSun = false;
		TODComponentManager*           m_tMgr = nullptr;

		// 资源加载
		std::map<String, bool>		   m_bLoadTexture;
#ifdef ECHO_EDITOR
		bool                           mIsSaveFile = true;
#endif
	};

	using TodTimelineComponentPtr = EchoWeakPtr<TodTimelineComponent>;

	DEFINECOMFACTORY(TodTimelineComponent, UNMOVEABLE)



	class EchoTodTimelineScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;
		static TodTimelineComponent* GetTodTimeLineCom(ActorPtr pActor, const std::string& name);
	};

};

#endif
