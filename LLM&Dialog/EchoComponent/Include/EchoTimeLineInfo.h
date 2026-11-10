//-----------------------------------------------------------------------------
// File:   EchoTimeLineInfo.h
//
// Author: anhaoxiang
//
// Date:   2023-2-27
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef _EchoTimeLineInfo_h__
#define _EchoTimeLineInfo_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"
#include "EchoMaterialComponent.h"
#include "EchoHermiteCurve.h"
#include "EchoHermite.h"


namespace Echo {

	class TimelineComponent;
	class TimelineComListener;

	struct TimelineComData {
		EchoWeakPtr<TimelineComponent> timelineComPtr;
		bool                           isReSetTime = true; // 标识是否需要重置时间
		TimelineComListener*           timelineListener;
	};

	enum timelineType {
		Default,
		Color
	};
	enum PlayVideoType {
		ONCE_PLAY,
		LOOP_PLAY,
		PING_PANG_PLAY
	};

#ifdef ECHO_EDITOR
	struct selectedAttributeData {
		std::string actorName;
		std::string comName;
		Name attributeName;
		int st = 0;
		int en = 0;
		void reset()
		{
			actorName = "";
			comName = "";
			attributeName = "";
			st = 0;
			en = 0;
		}
	};
#endif

	enum RootMotionDataType {
		M_Position,
		M_Rotation,
		M_Scale,
		M_Quaternion
	};

	struct MotionNodeData {
		std::string   comName;
		std::vector<std::pair<int, int>> idx; //记录开始与结束索引
		std::vector<RootMotionDataType> type;
	};

	struct HermiteMotionData {
		Vector4                 value;
		Vector4                 leftHandTangent;
		Vector4                 rightHandTangent;

		Vector3					isLeftWeighted;
		Vector3					isRightWeighted;
		Vector3					leftWeight;
		Vector3					rightWeight;
	};
	using  TimelineMotionMap = std::map<int, HermiteMotionData>;
	struct EchoMotionData {
		RootMotionDataType type;
		TimelineMotionMap  motionData;
	};
	using  TimelineMotionVec = std::vector<EchoMotionData>;

	struct attributeType {
		uint8  isColor = 0;
		uint8  isCamera = 0;
		uint8  isNoInterpolate = 0;
		uint8  isAnimation = 0;
		uint8  isTruncation = 1;
		uint8  isQuaternion = 0;
	};

	struct attributeData {
		attributeData(const int& st_ = 0, const int& en_ = 0, const std::string& attributeName_ = "", const std::string& text_name_ = "",
			bool isShow_ = true, uint8 isColor_ = 0, uint8 isCamera_ = 0, uint8 isNoInterpolate_ = 0, uint8 isAnimation_ = 0,
			uint8 isTruncation_ = 1, uint8 isQuaternion_ = 0)
			: st(st_),
			en(en_),
			attributeName(attributeName_),
#ifdef ECHO_EDITOR
			text_name(text_name_),
			isShow(isShow_),
#endif
			isColor(isColor_),
			isCamera(isCamera_),
			isNoInterpolate(isNoInterpolate_),
			isAnimation(isAnimation_),
			isTruncation(isTruncation_),
			isQuaternion(isQuaternion_)
		{ }
	public:
		int st;
		int en;
		Name attributeName;
#ifdef ECHO_EDITOR
		std::string text_name;
		bool  isShow;
#endif
		uint8 isColor;
		uint8 isCamera;
		uint8 isNoInterpolate;
		uint8 isAnimation;
		uint8 isTruncation;
		uint8 isQuaternion;
	};


	//client回调初始化
	struct ActorIdDataHandle {
		std::map<int, ActorPtr>  mActorIdToActor;
	};

	using   actorMapping = std::map<std::string, std::vector<std::string>>;
	using   comMapping = std::map<std::string, std::vector<attributeData>>;
	using   ActorMap = std::map<std::string, ActorPtr>;
	using   comMap = std::map<std::string, EchoWeakPtr<ActorComponent>>;
	using   comTypeMapping = std::map<std::string, std::string>;


#ifdef ECHO_EDITOR

	using   FrameMapping = std::map<int, std::set<std::string>>;
	using   AttrFrameSetMapping = std::map<std::string, std::map<int, std::set<int>>>;
	using   AttrMapping = std::map<int,std::pair<std::string,int>>;

#endif


	class TimeLineInfo;
	using  TimeLineInfoPtr = SmartPtr<TimeLineInfo>;


	class _EchoComponentExport TimeLineInfoManager : public ResourceManagerBase, public Singleton<TimeLineInfoManager>
	{
	public:
		TimeLineInfoManager();
		~TimeLineInfoManager();

		TimeLineInfoPtr                        createManual(const Name& name, ManualResourceLoader* loader = 0);
	protected:
		virtual                                ResourceBase* createImpl(const Name& name, bool isManual, ManualResourceLoader* loader,const NameValuePairList* createParams) override;
	};

	class _EchoComponentExport TimeLineInfoLoadingTood : public Singleton<TimeLineInfoLoadingTood>
	{
	   public:
		   virtual ~TimeLineInfoLoadingTood();
		   //同步异步加载文件
		   TimeLineInfoPtr                        loadFile(const Name& filePath, bool sync = false);
		   //检查指针是否已经加载完数据，如果没有就同步加载出数据
		   bool                                   checkTimelineInfo(TimeLineInfoPtr&  pTimelineInfo);
		   void                                   registerTimelineComData(const Name& fileName_, const EchoWeakPtr<TimelineComponent>& timelinePtr_, bool isResetTime_, TimelineComListener* listener = nullptr);
		   void                                   unRegisterTimelineComData(TimelineComListener* listener = nullptr);

		   void                                   refreshMissingTimeline(const ActorPtr& actorPtr_);
		   void                                   addMissingTimeline(const ActorPtr& actPtr_,TimelineComponent* timelineCom_);
		   void                                   eraseMissingTimeline(TimelineComponent* timelineCom_);
	protected:
		class TimeLineInfoResourceHandleListener : public ResourceBackgroundQueue::Listener {
		public:
			TimeLineInfoResourceHandleListener(){}
			~TimeLineInfoResourceHandleListener() { }

			void                                requestResource(const Name& resName, const Name& resType, ResourceLoadPriority priority);
			void								clearRequest();
			void								operationCompleted(BackgroundProcessTicket ticket, const ResourceBackgroundQueue::ResourceRequest& request, const ResourceBasePtr& resourcePtr) override;
		protected:
			std::set<BackgroundProcessTicket>   mMapTickets;
		};
	private:
		void                                    requestResource(const Name& resName, const Name& resType, ResourceLoadPriority priority);
		void                                    timelineInfoLoadCompleted(const Name& resName,const TimeLineInfoPtr& TimelineInfo);
	private:
		TimeLineInfoResourceHandleListener*     mResourceHandleListener = nullptr;
		std::multimap<Name, TimelineComData>    mTimelineComDataMap;
		std::unordered_map<ActorPtr, std::unordered_set<TimelineComponent*>> mMissingTimelineMap;//存储带missing属性的时间轴
	};


	class _EchoComponentExport TimeLineInfo : public ResourceBase {
		friend class TimeLineInfoManager;
		friend class TimelineComponent;
		friend class TodTimelineComponent;
	public:
		TimeLineInfo(ResourceManagerBase* creator, const Name& actFileName, bool isManual, ManualResourceLoader* loader);
		~TimeLineInfo();
		
		void                                     saveTimelineMotionFile(const std::string& filePath, const TimelineMotionVec& motionVec_);

		void                                     loadTimelineFile(const std::string& filePath);
		void                                     saveTimelineFile(const std::string& filePath);
		void                                     loadEditorData(cJSON* filePath);
		void                                     loadClientData(cJSON* filePath);
		//获取时间轴开始时间与结束时间的差值（单位为秒）
		float                                    getTimeLength();

		const std::vector<EchoHermite*>&		 getHermiteData() { return mHermites; }
		const comMapping&                        getComToAttribute() { return mComponentToAttributes;}
		
	protected:
		void                                     loadNewData(cJSON* filePath);
		void                                     loadNewTodData(cJSON* filePath);
		void                                     saveNewData(cJSON* filePath);
		void                                     saveNewTodData(cJSON* pRoot);
		void                                     saveEditorData(cJSON* pRoot);
		void                                     saveClientData(cJSON* pRoot);
		bool									 dataDetection(cJSON* pRoot, const std::string& filePath);

	protected:
		virtual void prepareImpl(void) override;
		virtual void unprepareImpl(void) override {};
		virtual void loadImpl(void) override {};
		virtual void unloadImpl(void)override {};
		size_t calcMemUsage(void) const override;
	private:
		std::vector<EchoHermite*>				    mHermites ;
		float								        mDeltaTime = 30.f;
		float								        startSceneTime = 0.f;
		float								        endSceneTime = 1.f;
		int                                         currentPlayWay = ONCE_PLAY;
		std::vector<std::string>                    AttributeName;
		std::vector<int>                            AttributeBegin;
		std::vector<int>                            AttributeEnd;
		actorMapping                                mActorToComponents;
		comMapping                                  mComponentToAttributes;
		int                                         OrderCnt = 0;
		float                                       mVer = 1.3f;
		//editor Data
		std::vector<int>                            showComponents;
		std::vector<std::vector<std::string>>       AttributeToName;
		timelineType                                recordColorTimelineType = Default;
		std::vector<std::string>                    HermiteNames;
		Vector2                                     contentSize_ = Vector2(500, 20);
		bool								        bPlayScene = false;
		float                                       mWidthMultiple = 1.f;
		std::string                                 TimelineName = "";
		std::vector<timelineType>                   AttributeType;
		int											mMaxWeight = 0;

		std::map<std::string, int>                  mActorToId;
		MotionNodeData                              mRootMotionData;

#ifdef ECHO_EDITOR
		//记录控制相机生效的关键帧的该帧对应哪一行的生效关键帧数据
		std::map<int, int>                         mCameraFrameToIdx;
		int                                        mCameraMgrIdx = -1;
#endif
	};

};

#endif
