#ifndef _EchoAnimation_Timeline_component_h__
#define _EchoAnimation_Timeline_component_h__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoProtocolActorComponent.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"
#include "EchoAnimationTimelineInfo.h"
//#include "EchoHermite.h"

namespace Echo {

	class _EchoComponentExport AnimationTimelineComponentInfo : public ActorComponentInfo {
	public:
		AnimationTimelineComponentInfo() {}
		AnimationTimelineComponentInfo(const Name& name, ActorCOMMoveableType moveable);
		virtual          ~AnimationTimelineComponentInfo() override;

		virtual void     ExportData(cJSON* jsonNode) override;
		//读取json
		virtual void     ImportData(cJSON* jsonNode) override;
		virtual void     CopyTo(ActorComponentInfo* info) override;
		virtual void     Overlay(ActorComponentInfo* srcInfo) override;
		virtual void     OverWrite() override;

#ifdef ECHO_EDITOR
		virtual bool     getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
	protected:
		virtual void     _parseCutSceneFile(std::unordered_multimap<ResourceType, std::string>& resMap, const std::string& filePath);
#endif
	public:
		std::string                  mFilePath;
		float                        m_TickSpeed = 1.f;
		EDITOR_REFLECT
	};

#ifdef ECHO_EDITOR
	struct ClipData {
		float mRealLength;
		int   mSize;
	};
#endif

	enum TickState {
		state_begin,
		state_running,
		state_end,
		state_count
	};

	struct AnimTickDataHandle
	{
		float          m_Time = 0.f; // 片段走到的时间
		HermiteKey*    m_Key = nullptr;  // 当前片段的属性
		TickState      m_tickState = TickState::state_count;

#ifdef ECHO_EDITOR
		bool           m_IsEditorOperation = false;
#endif
    };

	struct LoadPercentData
	{
		int                          count      = 0;  //总数
		int                          succCount  = 0;  //完成的数量
		double                       percent    = 0.0;//百分比
		EchoTSet<ActorComponentPtr>  waitComs;        //未完成的组件
	};

	enum LoadLevel {
		Clip_Lv0,
		Clip_Lv1
	};

	using TimelineMap = std::map<ActorComponentPtr, bool>;

	class AnimationTimelineComponent;

	class _EchoComponentExport  filmActorListener :public ActorLoadListener
	{

	public:
		filmActorListener(ActorPtr actor, AnimationTimelineComponent* aniCom_,int idx,int frame) :mActor(actor),mAnimationCom(aniCom_), mIdx(idx),mFrame(frame){}
		virtual  ~filmActorListener() override { mActor = nullptr; }

		ActorPtr                     getActor() { return mActor; }
		void                         setActor(ActorPtr actor_) { mActor = actor_; }
		virtual  bool                OnActorCreateFinish() override;

	private:
		
		ActorPtr                     mActor;
		AnimationTimelineComponent*  mAnimationCom = nullptr;
		int                          mIdx = 0;
		int                          mFrame = 0;
	};

     
	class _EchoComponentExport clipLoadListener
	{
	public:
		clipLoadListener(){}
		virtual ~clipLoadListener() {}
		virtual  void               onClipPlayEnd() {};
		virtual  void               OnClipCreateFinish() {};
	};



	using  AnimTickFunc = std::function<void(void*, const std::string& value, const AnimTickDataHandle&)>;


	class _EchoComponentExport AnimationTimelineComponent : public ProtocolActorComponent
	{		
		DECLAREACTORCOMTYPE_H(AnimationTimelineComponent)

	public:
		REGISTER_LUA_CLASS_KEY;
		virtual void                init(ActorComponentInfo* info) override;

		virtual void                connectCom() override;
		virtual void                unInit() override;
		virtual void			    registerTickTimeBegin() override;
		virtual void		        prePhysicsUpdate(EventID, ProtocolHandle) override;
		virtual void				resourceArrived(const Name& resName, const Name& resType, bool bFail, const ResourceBasePtr& resourcePtr) override;

		void                        resetAnimationData(AnimationTimelineInfoPtr aniPtr_);
		void                        resetAnimationInfoData(AnimationTimelineInfoPtr aniPtr_);
		void                        loadFile(const std::string& filePath_, bool synchronous = true);
		void                        saveFile(const std::string& filePath_);
		const std::string&          getFilePath() { return mFilePath; }
		void                        setFilePath(const std::string& filePath) { mFilePath = filePath; }

		void                        initFilmActorData(ActorPtr actorPtr_,int idx, int frame);
		void                        setFilmListenerState(filmActorListener* filmListener_, bool state_); // true -- 加载完毕 

		void                        setIsPlay(bool isPlay);
		bool                        getIsPlay() { return mIsPlaying; }
		void                        updateAnimationData();


		void                        setKeyValue(int idx, int frame, const std::string& value_ , float startTime,float length,bool isAsync = true);
		void                        setKeyValue(int idx, int frame, float realLength);
		void                        setKeyValue(int idx, int frame, float startTime, float length);
		void                        setKeyLength(int idx, int frame, float length);

		void                        copyKeyRealLengthToLength(int idx,int frame);

		void                        createKey(int idx, int frame);
		void                        getKey(int idx , int frame, std::string& value , float& startTime, float& length);

		int				            copyKey(int oriIdx, int oriFrame , int resultIdx , int resultFrame);
		void                        deleteKey(int idx, int frame);

		bool						forcedMoveKey(int oriIdx, int oriFrame, int resultIdx, int resultFrame);
		bool                        moveKey(int oriIdx, int oriFrame, int resultIdx, int resultFrame);
		bool						calculateProximityKey(float frameLen, int oriIdx, int oriFrame, int resultIdx, int& resultFrame, int& direction, std::set<int>& frame, bool noChange);

		void                        clipKey(int idx,int keyFrame, int currFrame); // 数组索引 -- 关键帧 帧数  -- 要截断的帧数

		void                        createClip(int type_);
		void                        createClip(int clipType, const ActorComponentPtr& comPtr,const std::string& clipName,const Name& hermiteType);
		void                        deleteClip(int idx);
		void						resetCreateClipIdx(int idx);

		int                         getFrameByTime(float time_);
		float                       getTimeByFrame(int frame_);

		int                         getCurrentFrame() { return getFrameByTime(mCurrTime); }
		void                        setCurrentTimeByFrame(int frame);
		
		int                         getStartTimeByFrame() { return getFrameByTime(mContext.mStartTime); }
		int                         getEndTimeByFrame() { return getFrameByTime(mContext.mEndTime); }

		void                        setStartTimeByFrame(int frame);
		void                        setEndTimeByFrame(int frame);

		bool                        getHermiteKeyData(int idx, int frame, float& startTime, float& length);
		
	    HermiteKey* const           getHermiteKey(int idx, int frame);//通过索引和桢值获取相应的hermiteKey

		void                        refreshTimelineComData(); //重新刷新片段中的missing属性
		bool						setHermiteKeyStartTime(int idx, int frame, float startTime);

		const std::vector<std::pair<int, std::string>>& getClipTypeVec() { return mContext.mClipType; }
		const std::vector<EchoHermite*>& getHermiteVec() { return mContext.mAnimationVec; }
		const std::vector<std::map<int, ActorPtr>>& getActorIdxToPtr() { return  mIdxToPtr; }
		const std::vector<ActorComponentPtr>& getIdxtoPtr() { return mIdxToComPtr; }

		
		//添加删除监听器
		void                        addClipLoadListener(clipLoadListener* listener_);
		void                        deleteClioLoadListener(clipLoadListener* listener_);	// 旧接口不再使用
		void						deleteClipLoadListener(clipLoadListener* listener_);

		//注册tick相关的函数
		static void                 registerTickFunc(int type, const AnimTickFunc& func);
		static void                 unRegisterTickFunc(int type);

		//注册过场动画的加速列表
		void                        registerTickCom(const ActorComponentPtr& p_Com_);
		void                        unRegisterTickCom(const ActorComponentPtr& p_Com_);
		static  void                registerTickComByActor(const ActorPtr& actorPtr_, const ActorComponentPtr& p_Com_);
		static  void                unRegisterTickComByActor(const ActorPtr& actorPtr_, const ActorComponentPtr& p_Com_);

		//处理注册列表的速率更新
		void                        updateTickComSpeed();

		int                         getClipTypeByIdx(int idx);

		void                        setSpeed(float speed);
		float                       getSpeed() { return m_TickSpeed; }

		const std::map<ActorPtr, std::set<ActorComponentPtr>>& getActorToTimelineMap() { return mActorToCom;  }


	    //收集资源
		int                         getLoadPercent();
		int                         getLoadPercent(LoadLevel level);

		void                        resourceLoadFinish(ProtocolHandle handle);

		void                        addToTimelineList(const ActorComponentPtr& timelineCom_);//timeline uninit时需要移除

		void                        addToLoadActorPercent(const ActorPtr& actPtr_, const ActorComponentPtr& timelineCom_);      //参数2为该actor要加上的收集的数量
		void                        notifyTimelineLoadPercent(const ActorComponentPtr& timeCom_);//调用该函数代表完成的数量 + 1

		void                        setIsUseLoadState(bool state_);
		void                        setLoadTimeOutTime(float time_);
		float                       getLoadTimeOutTime() { return m_LoadTimeoutData; }

#ifdef ECHO_EDITOR
		void                                 getClipComByIdx(int idx, ActorComponentPtr& ptr_);
		void                                 getClipTypeNameByIdx(int idx,std::string& name);
		const std::vector<std::string>&      getHermiteNameVec() { return mContext.mClipName;}
		const std::map<ActorPtr, ClipData>&  getActorToClipData() { return mActorToClipData; }
		void                                 setHermiteNameByidx(int idx, const std::string& name_);

		void                                 setIsEditorTick(bool isEditor);
		void								 LoadActorData(int idx, int frame, const String& actorPath);
#endif
	private:
		void                        _addKey(HermiteKey *key_,int type_,int idx, int frame);

		void                        _initAnimationData();
		void                        _clearFilmListener();
		void                        _updateTimelineGroupData(int idx,const std::string& keyValue_, int startFrame,float length,float startTime);

		//更新actor类型的片段
		void                        _updateActorClipData(int idx, const std::string& keyValue_, int startFrame, float length, float startTime,   float diffTime);
		
		//更新组件类型的片段
		void                        _updateComClipData(int idx, const std::string& keyValue_, int startFrame, float length, float startTime,  float diffTime);

		void                        _loadActorData(int idx, int frame,const std::string& actorPath_,bool isAsync = false);

		void                        _loadComData(int idx);
		void                        _clearOldData();
		void                        _moveIdxToActorKey(int oldIdx,int oldFrame,int resultIdx,int resultFrame);
		void                        _deleteIdxToActorKey(int Idx, int Frame);
		void                        _deleteIdxToActorRow(int idx);

		void                        _initTimelineComList(const ActorPtr& actPtr_);
		bool                        _judgeIsActorClipKey(int idx);
		void                        _updateTickComSpeed(const ActorComponentPtr& com_);
		void                        _loadFinish();
		void                        _updateLoadPercent();
	protected:

		//std::map<std::string,std::map<int,ActorPtr>>                   mActorPathToIdx; // actor路径 --> key的索引信息
		std::vector<std::map<int, ActorPtr>>                           mIdxToPtr;       // key的索引信息 --> 实例指针

		std::vector<ActorComponentPtr>                                 mIdxToComPtr;

		std::map<ActorPtr , std::set<ActorComponentPtr>>               mActorToCom;
		float                                                          mCurrTime;
		AnimationTimelineInfoHandle                                    mContext;
		std::string                                                    mFilePath;
		bool                                                           mIsPlaying;
		
		std::map<filmActorListener*,bool>                              mFilmListenerVec;
		AnimationTimelineInfoPtr                                       mAnimationPtr;
		
		//Tick函数的注册
		static std::unordered_map<int, AnimTickFunc>                   mTypeToTickFunc;

		AnimTickDataHandle                                             m_TickData;
		float                                                          m_TickSpeed = 1.f; // 播放倍率
		std::vector<ActorComponentPtr>                                 m_TickVec; //自创建组件的倍数播放列表

		//收集资源
		bool                                                           m_bUseLoadState = true; //为true时加载完成再播放，false时则不关系资源加载进度
		bool                                                           mLoadState = true;      //加载完成时是true
		std::vector<clipLoadListener*>                                 mLoadListenerVec;       //加载完成的回调容器


		std::set<ActorPtr>                                             m_UntreatedActor;  //未计数timeline数量的容器
		std::map<ActorPtr, TimelineMap>                                m_TimelineLoadList;//actor->timeline的列表
		 
		std::map<ProtocolHandle, LoadPercentData>                      m_LoadObjectSet;   //需要修改为 protocolHandle To 子百分比的容器  没有timeline的回调回来就按0算
		int                                                            m_CollectResourceCount = 0;
		float                                                          m_LoadObjectTickSecond = 0.f;
		float                                                          m_LoadTimeoutData = 240.f;

#ifdef ECHO_EDITOR
		std::map<ActorPtr, ClipData >                                  mActorToClipData;
#endif
 	};

	DEFINECOMFACTORY(AnimationTimelineComponent, UNMOVEABLE)

}
#endif
