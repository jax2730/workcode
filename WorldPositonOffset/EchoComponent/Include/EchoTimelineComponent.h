//-----------------------------------------------------------------------------
// File:   EchoTimelineComponent.h
//
// Author: anhaoxiang
//
// Date:   2022-6-24
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef _EchoTimelineComponent_h__
#define _EchoTimelineComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"

#include "EchoTimeLineInfo.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"

namespace Echo
{
	//=============================================================================
	class TimelineComponent;
	class CameraBaseComponent;
	class _EchoComponentExport TimelineComListener {
	public:
		TimelineComListener(){}
		TimelineComListener(const TimeLineInfoPtr& pTimeline):m_pTimelineInfo(pTimeline){}
		virtual ~TimelineComListener() { m_pTimelineInfo = nullptr; }
		virtual   void  TimelineEndPlay(TimelineComponent* ){}
		virtual   void  TimelineInfoLoadComplete(const TimelineComData& timelineData) {}

		void            setTimelineInfo(const TimeLineInfoPtr& pInfo) { m_pTimelineInfo = pInfo; }
	protected:
		TimeLineInfoPtr  m_pTimelineInfo;
	};


	struct CollectResourceData {
		std::vector<std::string>   m_StrResourceVec;
		std::vector<float>         m_IdResourceVec;
	};

	class _EchoComponentExport TimelineComponentInfo : public ActorComponentInfo {
	public:
		TimelineComponentInfo() {}
		TimelineComponentInfo(const Name& name, ActorCOMMoveableType moveable);
		virtual ~TimelineComponentInfo();
		//导出json
		virtual void     ExportData(cJSON* jsonNode) override;
		//读取json
		virtual void     ImportData(cJSON* jsonNode) override;
		virtual void     CopyTo(ActorComponentInfo * info) override;
		virtual void     Overlay(ActorComponentInfo* srcInfo) override;
		virtual void     OverWrite() override;

#ifdef ECHO_EDITOR
		virtual bool     getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
	protected:
		void             _parseTimelineFile(std::unordered_multimap<ResourceType, std::string>& resMap,const std::string& filePath);
#endif

	public:
		std::string      m_TimelineFilePath;
		EDITOR_REFLECT
	public:
		//用于存储关键帧数据
		std::vector<EchoHermite*>				   mHermites;
		//记录当前的最大桢值
		int										   mMaxWeight = 0;
		//记录一秒对应多少桢
		float								       mDeltaTime = 60.f;
		//记录时间轴的开始时间
		float								       startSceneTime = 0.f;
		//记录时间轴的结束时间
		float								       endSceneTime = 1.f;

		int                                        currentPlayWay = ONCE_PLAY;
		//记录时间轴播放状态
		bool								       bPlayScene = false;
	
		float                                      mWidthMultiple = 1.f;

		bool                                       isLoadHermite = false;
		bool                                       isLoadDeltaTime = false;
		bool                                       isLoadStartTime = false;
		bool                                       isLoadEndTime = false;
		bool                                       isLoadPlayWay = false;
	};
	//============================================================================

	class _EchoComponentExport TimelineComponent : public ProtocolActorComponent
	{
		DECLAREACTORCOMTYPE_H(TimelineComponent)
	public:
		REGISTER_LUA_CLASS_KEY;

	public:
		virtual void                            init(ActorComponentInfo* info) override;
		virtual void                            unInit() override;
		virtual void                            connectCom() override;
		virtual void			                registerTickTimeBegin() override;
		virtual void		                    prePhysicsUpdate(EventID, ProtocolHandle) override;
		virtual void							visibleStateChanged(bool visible) override;


		virtual void				            copyTo(const Echo::ActorComponentPtr& p_actor_component) override;
		virtual void				            resourceArrived(const Name& resName, const Name& resType, bool bFail, const ResourceBasePtr& resourcePtr) override;

		virtual bool                            isCreateFinish() override;
		void                                    initOldTimeData();
	public:
		//处理旋转中四元数部分

		bool									ProcessRotationSeparately(int id = -1, bool remove = false);	// 将旋转欧拉角转换为四元数，并重新计算欧拉角曲线对应的四元数斜率
		float                                   QuaternionInterPolate(int vIdx, float frame_, Vector3& result);	// 输出四元数插值，与当前id，frame对应四元数转换为欧拉角的值

		//获取插值函数

		float								    floatInterpolation(size_t vIdx, float weight); //获取float型时间轴插值，前后不存在帧值时返回0.f
		int                                     intInterpolation(size_t vIdx, float weight); //获取int型时间轴插值
		bool                                    boolInterpolation(size_t vIdx, float weight); //获取fragment类型时间轴插值
		float                                   colorInterpolation(size_t vIdx, float weight);//获取颜色属性的插值
		void                                    stringInterpolation(size_t vIdx, float weight, std::string& value_);//获取string型插值


		float								    floatInterpolation(size_t vIdx, float weight, int& preFrame, int& nextFrame); //获取float型时间轴插值，前后不存在帧值时返回0.f
		float                                   getActorTimeValue(size_t vIdx);//获取float型时间轴插值，（获取当前时间指定行数的float插值）
		int                                     intInterpolation(size_t vIdx, float weight, int& preFrame,int& nextFrame); //获取int型时间轴插值
		bool                                    boolInterpolation(size_t vIdx, float weight, int& preFrame, int& nextFrame); //获取fragment类型时间轴插值
		float                                   colorInterpolation(size_t vIdx, float weight ,int& preFrame, int& nextFrame);//获取颜色属性的插值
		void                                    stringInterpolation(size_t vIdx, float weight,std::string& value_ , int& preFrame);//获取string型插值

		template<class HermiteType_, class KeyType, class valueType>
		void                                    NoInterpolation(size_t vIdx, float frame, std::pair<valueType, bool>& noInterData,int& preFrame);
		template<class HermiteType_,class KeyType,class valueType>
		bool                                    getNextKeyValue(size_t vIdx, float frame, valueType& resultValue_, int& resultFrame_);

		bool                                    getLastKeyValue(size_t vIdx, float frame, float& resultValue,int& preFrame);

		Echo::HermiteKey*						GetKey(size_t vIdx, int keyVal);
		int                                     cameraStateInterpolation(const size_t& vIdx, float weight);

        //判断当前的索引是否存在关键帧
		bool                                    judgeIsExitKeyFrame(int idx,int frame);
		//清除指定行的所有关键帧
		void                                    deleteRowKey(int idx);

		//增删时间轴行数接口
		virtual void							AddProperty(const EchoHermite::HermiteType&, const Name& typeName = Name::g_cEmptyName, const FragmentType& type_ = FragmentType::Default_); //增加时间轴行数
		virtual void							AddKey(size_t vIdx, int weight, float value,bool flag_ = true);//bool量用于标识是否在添加的同时设置属性
		void									AddKey(size_t vIdx, int frame, bool value);
		void									AddKey(size_t vIdx, int frame, int value);
		virtual void                            AddKey(size_t vIdx, int frame, const std::string& value);
		void                                    AddKeyWithHandTangent(size_t vIdx, int frame, float value, float leftHandTangent, float rightHandTangent);

		//扩展类型
		//void                                    AddKey(size_t vIdx, int frame, HermiteKey* key_);

		virtual void							DeleteKey(size_t vIdx, int weight);
		virtual void							SetKeyValue(size_t vIdx,int keyVal, float value);
		void									SetKeyValue(size_t vIdx, int keyVal, bool value);
		void									SetKeyValue(size_t vIdx, int keyVal, const HermiteFragmentKey::FragmentObject& value);
		virtual void                            SetKeyValue(size_t vIdx, int keyVal, const std::string& value);
		//移动关键帧接口
		//移动关键帧，第一个参数为关键帧所在的行数，第二个参数为原先的帧数，第三个参数为要移动到的帧数
		void									MoveKey(size_t vIdx, int oldT, int newT);  
		//移动关键帧并赋予新值，第一个参数为关键帧所在的行数，第二个参数为原先的帧数，第三个参数为要移动到的帧数，第四个参数为移动后的值
		virtual void						    MoveKey(size_t vIdx, int oldT, int newT, float value);
		//移动一组关键帧，前两个参数表示要移动的关键帧所在的行数与帧数，第三个参数表示帧数更改的差值，第四个参数表示桢值更改的差值
		void                                    moveGroupKey(const std::vector<int>& , const std::vector<int>&, const int& ,const float&);
		//缩放关键帧
		void                                    scaringKey(const std::map<int, std::pair<int, std::set<int>>>& scaringSetMap);

		//设置当前时间轴组件的开始时间
		void                                    setTimelineStartTime(float startTime_);
		//设置当前时间轴组件的结束时间
		void                                    setTimelineEndTime(float endTime_);
		//获取当前时间轴组件的开始时间
		float                                   getTimelineStartTime();
		//获取当前时间轴组件的结束时间
		float                                   getTimelineEndTime();
		//获取当前时间轴组件的timeline时间长短（单位为秒）
		float                                   getTimelineTimeLength();
		//设置时间轴的播放模式 (loop,once,ping-pang)
		void                                    setPlayWay(PlayVideoType  playWay_);
		//获取时间轴的播放模式
		PlayVideoType                           getPlayWay();

		virtual	void							setActor(const ActorPtr& pActor) override;

		//获取时间轴的最大帧数
		int									    getMaxLength();
		//将时间轴时间更新至指定的时间
		void                                    timelineUpdate(float time);
		//将当前时间轴时间更新为0
		void                                    remakeTime();
		//获取当前的时间轴帧数
		int                                     getTimeLineFrame();

		float                                   getRealTime(); // 获取转帧速率之后的真实速度
		//获取当前的时间
		inline float                            getTimelineTime() { return currTime; }

		//三种播放模式
		//播放模式更改为once
		virtual void                            runOnceVideo();
		//播放模式更改为loop
		virtual void                            runLoopVideo();
		//播放模式更改为ping―pang
		virtual void                            runPingPangVideo();

		//获取，设置播放速度
		inline void                             setTimelinePlaySpeed(float speed_) { mTimelinePlaySpeed = speed_; }
		inline float                            getTimelinePlaySpeed() { return mTimelinePlaySpeed; }
        //获取、设置逆播放状态
		inline void                             setTimelineReversePlay(bool isReversePlay_) { mIsReversePlay = isReversePlay_; }
		inline bool                             getTimelineReversePlayState() { return mIsReversePlay; }

		//设置时间轴是否播放 
		virtual void                            setIsPlay(bool isplay_);
		void									playEnd();
                   
		void                                    setPlayState(bool isPlay_) { bPlayScene = isPlay_; }
		//获取时间轴的播放状态
		bool                                    getIsPlayScene();

		//获取当前播放的百分比
		float                                   getTimelinePlayPercent();
		//设置时间轴文件路径
		void                                    setTimelineFilePath(const std::string& filePath_);

		//实时关键帧
		virtual void                            setComponentAttribute();
		uint8                                   judgeCameraComIsActive(int vIdx, int frame_);
		void                                    setAttributeData(const size_t& vIdx);
		
		HermiteStringKey*                       TimeConsuming(size_t vIdx, float frame, float& time);
		bool                                    getHermiteData(const std::vector<Echo::attributeData>::const_iterator::value_type& iter,bool& isString, TimelineTickData& pTimelineTickData);

		//加载Timeline文件，第一个参数为路径，第二个参数为是否为同步加载，第三个参数为是否更改播放信息（播放状态与模式）
		void                                    GetTimeLineFile(const char* path, bool synchronous = true, bool isChangeState = true);
		//创建一个时间轴文件，参数为路径
		void                                    CreateTimeLineFile(const char* path);
		//创建一个Motion文件
		void                                    CreateTimelineMotionFile(const char* path);
		void                                    SetMotionNode(const std::string& comName_);
		void                                    eraseMotionNode();
		//设置是否使用rootMotion模式
		void                                    setRootMotionMode(bool isRootMotionMode_) { mIsRootMotionMode = isRootMotionMode_; }
		//获取是否是同rootMotion模式的状态
		bool                                    getRootMotionMode() { return mIsRootMotionMode; }
		void                                    deleteMotionAttribute(const std::string& attrName_);


		const MotionNodeData&                   getRootMotionData() {return  mRootMotionData;}

		//注册监听器
		void                                    registerTimelineComListener(TimelineComListener* p_ComListener);
		//取消注册监听器
		void                                    unRegisterTimelineComListener(TimelineComListener* p_ComListener);
		//获取文件加载错误信息
		const std::vector<std::string>&         getErrorData() { return mErrorInfoVec; }

		//给actorId赋值
		void                                    setActorId(const std::string& actorName_,int actorId_);
		int                                     getActorId(const std::string& actorName_);
		const std::map<std::string, int>&       getActorToId() { return mActorToId; } 
		void                                    initActorIdData();
		void                                    resetDataByActorId();

		void                                    remakeAttributeState();
		void                                    setAttributeName(const int& idx, const std::string& name);
		//将.timeline文件加载的数据(TimeLineInfo指针中的数据)覆盖到Timeline组件,第二个参数指是否修改时间轴的播放状态（播放 or 不播放），第三个参数是指是否修改时间轴的播放方式--（循环or单次or往复）
		void                                    resetTimelineData(TimeLineInfoPtr tInfo, bool isChangePlayState_ = true, bool isChangePlayWayState = true);
		//将timeline组件信息赋给TimelineInfo指针
		void                                    resetTimelineInfoData(TimeLineInfoPtr tInfo);
		void                                    copyHermiteDatas(const std::vector<EchoHermite*>& oriData_, std::vector<EchoHermite*>& resData_);

		void                                    addActorAndComMaps(const std::string& actName,ActorPtr act,const std::string& comName, EchoWeakPtr<ActorComponent> com);
		void									revokeRemovalActAndComMaps(const std::string& actName, ActorPtr act, const std::string& comName, EchoWeakPtr<ActorComponent> com);

		void                                    importItem(const EchoHermite::HermiteType& type , const Name& typeName = Name::g_cEmptyName, const FragmentType& type_ = FragmentType::Default_);
		void									insertItem(const EchoHermite::HermiteType& type, const Name& typeName = Name::g_cEmptyName, const FragmentType& type_ = FragmentType::Default_, int num = -1);

		const int&                              getOrderCnt() { return OrderCnt; }
		const actorMapping&                     getActorToComponents(){ return mActorToComponents; }
		const comMapping&                       getComToAttribute() { return mComponentToAttributes; }

		void                                    dealwithCameraComEndPlay();

		bool									refreshExitAttributeActor();
		bool									refreshExitAttributeCom();


		bool                                    judgeIsExitMissingAttribute();
#ifdef ECHO_EDITOR
		const comTypeMapping&                   getComTypeMappingMaps() { return mComTypeMappingMaps; }
		const std::map<std::string, int>&       getActorShows() { return mActorShows; }
		void                                    setComTypeMappingState(const std::string typeName, bool isShow);
		void									setExitAttributeQuaternion(int id, bool isQuaternion);
		void									setExitAttributeQuaternion(const std::string& combineName, const std::string& attributeName, bool isQuaternion);
		bool                                    isExitAttribute(const std::string& combineName, const std::string& attributeName);
		bool                                    isExitAttributeActor(const std::string& actName_);
		bool                                    isExitAttributeCom(const std::string& iterName_, const std::string& comName_);
		void                                    remakeCurveState();
		void                                    addTypeMapItem(const std::string& comName, EchoWeakPtr<ActorComponent> com);
		
		//创建一个新的属性组
		void                                    createAttributeMapping(const std::string& actName_, const std::string& combineName_, const std::string& attributeName_, const std::string& tex_name, const int& size_,attributeType attrType);
		//删除属性组
		int                                     deleteAttributeMapping(const std::string& actName_, const std::string& combineName_, const std::string& attributeName_);
		//增加额外属性组
		void                                    addExtraAttribue(const std::string& combineName_,const std::string& attributeName_);
		//删除额外属性组
		void                                    deleteExtraAttribute(const std::string& combineName_, const std::string& attributeName_, int idx);
		//插入额外属性
		void									insertExtraAttribute(const std::string& actName_, const std::string& combineName_, const std::string& attributeName_, const std::string& tex_name,
			                                                         const int& size_, attributeType attrType, const EchoHermite::HermiteType& type, const Name& typeName, const FragmentType& type_, int num);
		void                                    dealWithRootMotionIdx(int st,int size);
		void                                    dealWithRootMotionIdxAdd(int en);

		void                                    addRootMotionIdx(const std::string& combineName, const std::string& attrName, int size);
																
		void									getAttributeData(int pHermiteIndex, attributeData& attrData_);
		void                                    getAttributeData(const std::string& combineName, const std::string& attrName, attributeData& attrData_);
		void                                    getAllActorData(ActorPtr currActor,bool isFindNoTimelineActor);
		void                                    getCurrAtrorData(ActorPtr currActor);
		void                                    clearEditorActorData();
		//为指定行的时间轴设置一个名字
		void                                    setHermiteNames(int i, const std::string& name);
		//获取指定行时间轴的名字,编辑器使用
		std::string                             getHermiteNames(size_t i);
		virtual void							RemoveProperty(const size_t& vIdx);                               //删除时间轴行数


		void                                    RemoveGroupProperty(size_t st, size_t en);
		void                                    addCameraBaseComData(const EchoWeakPtr<ActorComponent>& cameraCom_);
		void                                    deleteCameraBaseComData(const EchoWeakPtr<ActorComponent>& cameraCom_);

		void                                    addCameraMgrKey(int frame_, int vIdx_,const ActorComponentPtr& cameraCom_ );
		void                                    deleteCameraMgrKey(int frame_);
		int                                     getCameraComIdx(const ActorComponentPtr& cameraCom_ );
		void                                    getCameraComCombineNameByIdx(int idx, std::string& name_);
		void                                    getCameraComNameByIdx(int idx, std::string& name_);
		 
		//总集选帧--子集选帧相关
		void                                    addKeySetData(int idx_, int frame_);
		void                                    deleteKeySetData(int idx_, int frame_);
		void                                    initKeySetData();
		void                                    resetKeySetData();
		void                                    initIdxToAttr();
		void                                    resetIdxToAttr();
		void                                    moveKeySetData(int idx_,int oriFrame_,int resultFrame_);
		void                                    moveKeySetData(const std::vector<int>& idxVec_,const std::vector<int>& oriFrame_, int diffFrame_);
		void                                    moveKeyByAllKeySet(int oldFrame_,int newFrame_);
		void                                    deleteKeyByGroupKeySet(int frame_,const std::string& attrName_);
		void                                    addKeyByGroupKeySet(int frame_,const std::string& attrName_);
		void                                    moveKeyByGroupKeySet(int oldFrame_,int newFrame_,  const std::string& attrName_);
		void                                    deleteKeyByAllKeySet(int frame_);
		void                                    addKeyByAllKeySet(int frame_);

		const FrameMapping&                     getFrameMapping() { return  mFrameToAttr; }                 
		const AttrFrameSetMapping&              getAttrFrameSetMapping() { return  mAttrToFrameSet; }
		const AttrMapping&                      getAttrMapping() { return  mIdxToAttr; }
		const std::map<int, int>&				getCameraFrameIdx() { return mCameraFrameToIdx; }

		
		//编辑器下物体偏移相关

		bool                                    recoredRootMotionData(const attributeData& attrData_, std::vector<float>& data);
		bool                                    judgeIsRootMotionNode(const attributeData& attrData_);
		void                                    increaseRootMotionNodeSkew(ActorComponent* currCom);
		void                                    decreaseRootMotionNodeSkew(const attributeData& attrData_, std::vector<float>& data, ActorComponent* currCom);

		void                                    setSkewPosition(const Vector3& skewPos_);
		void                                    setSkewRotation(const Vector3& skewRot_);
		void                                    setSkewScale(const Vector3& skewScale_);
		void                                    setIsUseRootMotionSkew(bool isUse) { mIsUseRootMotionSkew = isUse; }

		const Vector3&                          getSkewPosition() { return mSkewPosition; }
		const Vector3&                          getSkewRotation() { return mSkewRotation; }
		const Vector3&                          getSkewScale() { return mSkewScale; }
		bool                                    getIsUseRootMotionSkew() { return mIsUseRootMotionSkew; }


		const Vector3&                          getRootMotionRealPosition() { return mRootMotionPosition.second; } 
		const Vector3&                          getRootMotionRealRotation() { return mRootMotionRotation.second; }
		const Vector3&                          getRootMotionRealScale() { return mRootMotionScale.second; }
		void                                    setShowTimeMinByFrame(int frame);
		void                                    setShowTimeMaxByFrame(int frame);
		void									setUpdateTimelineUI(bool value) { mIsUpdateUI = value; }
#endif
		
		void                                    setTimelineFrame(int frame_);
		void                                    setOneSecondFrame(int  frame_);
		void                                    stopTodTimelinePlay();
		void                                    getTopRootNode();
		//卸载当前加载的时间轴数据
		void                                    shutDownTimelineData();
		const comMap&                           getComMapping() { return mComMaps; }

		//收集资源相关
		bool                                    getIsCollectResource() { return m_bIsCollectResource; }
		void                                    setIsCollectResource(bool isCollect_) { m_bIsCollectResource = isCollect_; }
		void                                    resourceLoadComplete(ProtocolHandle protolHandle_);

		void									OutputDebuggingInformation();
		//收集资源相关
		void                                    resourceLoadFinish(ProtocolHandle handle_);
		int                                     preLoadResource(ProtocolHandle handle);

		void                                    setLoadTimeOutTime(float time_);
		float                                   getLoadTimeOutTime() { return m_LoadTimeoutData; }

	   protected:
		//处理info数据        
		void                                    initComData(const std::string& actName,ActorPtr actPtr);
		void                                    initRootMotionData(const std::string& comName);
		void                                    initTimelineUIData();
		void                                    initOldData();
		virtual void                            dealWithOldData();
		virtual void                            initActorAndComData();
		void                                    deleteScenePrefix(std::string& absName);

		void                                    _getOldKey(HermiteKey** oldKey_,int idx,int frame);
		void                                    _addKey(HermiteKey* oldKey, int idx, int frame, int diffFrame, const float& diffValue = 0.f);
		//处理移除属性的索引关系
		void                                    _dealWithIdxRemove(int st,int en);
		void                                    _dealWithIdxExtend(int st, int en);

		//重设帧对索引的容器
		void                                    _reInitFrameToIdx();

		//对旋转数据操作
		// 默认传递Index为旋转属性的st，并且index对应的数据默认为旋转属性
		void									insertQuaternionData(int vIdx, int frame);

		void                                    _loadFinish();
		void                                    _prepareTimelineResource(const std::string& comName, const ActorComponentPtr& comPtr);
     public:
		//用于标识find表是否查找失败
		bool                                       mIsFindListError = false;
		bool                                       PingPangOrder = false;	// 是否为往返中的返操作
		float								       currTime = 0.f;

		std::vector<timelineType>                  AttributeType;		
		bool                                       isChangeState = false;
		timelineType                               recordColorTimelineType = Default;
		std::vector<int>                           showComponents;
		std::vector<std::vector<std::string>>      AttributeToName;

		std::map<std::string,std::pair<bool,int>>  mTypeMap;

		//用于存储关键帧数据
		std::vector<EchoHermite*>				   mHermites;
		//记录当前的最大桢值
		int										   mMaxWeight = 0;
		int										   mMinWeight = 0;
		//记录一秒对应多少桢
		float								       mDeltaTime = 30.f;
		//记录时间轴的开始时间
		float								       startSceneTime = 0.f;
		//记录时间轴的结束时间
		float								       endSceneTime = 1.f;
		int                                        currentPlayWay = ONCE_PLAY;
		//记录时间轴播放状态
		bool								       bPlayScene = false;
		std::string                                m_TimelineFilePath;
		float                                      mWidthMultiple = 1.f;

		//老数据使用 已弃用 
		std::vector<EchoWeakPtr<ActorComponent>>   allChildComponents;
		std::vector<std::string>                   AttributeName;
		std::vector<EchoWeakPtr<ActorComponent>>   AttributeComponents;
		std::vector<int>                           AttributeBegin;
		std::vector<int>                           AttributeEnd;
		std::map<std::string, int>                 AttributeToLocation;
#ifdef ECHO_EDITOR
		//editor
		//记录每一行时间轴的名字
		std::vector<std::string>                   HermiteNames;
		Vector2                                    contentSize_ = Vector2(500, 20);
		//记录时间轴组件别名
		std::string                                TimelineName = "";
		std::map<std::string, std::string>         mRelNameToAbsName;
		selectedAttributeData                      mSelectAttributeData;
		std::map<ActorPtr, std::string>            mAllActorsMaps;
		std::vector<unsigned int>                  showCurveVec;

		//自动关键帧的相应数据
		std::string                                selectedAttributeName = "";
		std::string                                selectComponentName = "";
		std::set<int>                              selectedHermitedNames;
		float								       mShowTimeMin = 0.f;
		float								       mShowTimeMax = 30.f;
		std::map<int, float>                       recordComponentStates; // 用于记录在非实时关键帧模式发生值修改时的数据
	
		//记录cameraBase组件
		std::map<EchoWeakPtr<ActorComponent>,int>  mCameraBaseComMap;
		//记录控制相机生效的关键帧的该帧对应哪一行的生效关键帧数据
		std::map<int, int>                         mCameraFrameToIdx;
		int                                        mCameraMgrIdx = -1;

		bool                                       mIsMoveGroupKey = false;
		bool										mIsUpdateUI = false;


		//编辑器时记录rootmotion数据
		std::pair<bool,Vector3>                    mRootMotionPosition =std::make_pair(false,Vector3(0.f, 0.f, 0.f));
		std::pair <bool,Vector3>                   mRootMotionRotation = std::make_pair(false, Vector3(0.f, 0.f, 0.f));
		std::pair <bool,Vector3>                   mRootMotionScale = std::make_pair(false, Vector3(1.f, 1.f, 1.f));
		bool                                       mIsUpdateRootMotionData = false;

		bool                                       mIsUseRootMotionSkew = true;
		Vector3                                    mSkewPosition = Vector3(0.f, 0.f, 0.f);
		Vector3                                    mSkewRotation = Vector3(0.f, 0.f, 0.f);
		Vector3                                    mSkewScale = Vector3(1.f, 1.f, 1.f);
#endif
    protected:
		TimeLineInfoPtr                            m_PTimelineInfo;
		int                                        OrderCnt = 0;
		actorMapping                               mActorToComponents;
		comMapping                                 mComponentToAttributes;
		ActorMap                                   mActorMaps;
		comMap                                     mComMaps;
		bool                                       mIsReversePlay = false;
		float                                      mTimelinePlaySpeed = 1.f;
		bool                                       mIsRootMotionMode = false;
		bool                                       mIsInitRootMotionData = false;
		MotionNodeData                             mRootMotionData;
		std::set<TimelineComListener*>             mComListenerVecs;
		//actorID
		std::map<std::string, int>                 mActorToId;
		ActorIdDataHandle                          mActorIdData;
		std::vector<std::string>                   mErrorInfoVec;
		
		//记录生效的相机组件的帧数
		int                                        mCameraActiveFrame = 0;
		int                                        mCameraActiveIdx = -1;
		float                                      mPreTime = 0.f;
		float                                      mVer = 1.3f;
		
		//记录旋转四元数
		std::map<int, std::map<int, Quaternion>>   mHermiteQuaternionData; // idx, frame, Quaternion
		std::map<int, std::map<int, Quaternion>>   mHermiteOutSlopeData;
		std::map<int, std::map<int, Quaternion>>   mHermiteInSlopeData;

		//资源收集相关  卸载时间轴时需要处理一下
		std::set<ProtocolHandle>                   m_LoadTimelineObjectSet;
		int                                        m_LoadObjectCount = 0;
		bool                                       m_bIsCollectResource = false;
		//int                                        m_CollectResourceCount = 0;
		float                                      m_LoadObjectTickSecond = 0.f;
		float                                      m_LoadTimeoutData = 60.f;
		bool                                       m_isCreateFinish = false;

#ifdef ECHO_EDITOR
		comTypeMapping                             mComTypeMappingMaps;
		std::map<Name, std::string>                mComTypeMaps;
		std::map<std::string, int>                 mActorShows;


//总集选帧--子集选帧相关
		FrameMapping                               mFrameToAttr;
		AttrFrameSetMapping                        mAttrToFrameSet;
		AttrMapping                                mIdxToAttr;

		bool                                       mIsControlKeyByKeySet = false;
#endif
	};
	DEFINECOMFACTORY(TimelineComponent, UNMOVEABLE)

	class _EchoComponentExport EchoTimelineProFuncImp
	{
	public:
		//-----------------------------------------------------------------------------
		static void								initProtocolFunction();
		static void								uninitProtocolFunction();
	};

	class EchoTimelineScript : public EchoActorComponentScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;

	public:

		EchoTimelineScript() = default;
		EchoTimelineScript(const EchoActorScript& pActor, const std::string& name);

	public:
		static EchoTimelineScript GetTimeLineCom(const EchoActorScript& pActor, const std::string& name);
		float                                   getActorTimeValue(const size_t& vIdx);
		void                                    setIsPlay(bool flag);
		void                                    remakeTime();
		bool                                    getIsPlayScene();
		int                                     getTimeLineFrame();
		void                                    GetTimeLineFile(const char* path, bool synchronous = true, bool isChangeState = true);
		void                                    stopTodTimelinePlay();
	};




	template<class HermiteType_, class KeyType, class valueType>
	inline bool TimelineComponent::getNextKeyValue(size_t vIdx, float frame, valueType& resultValue_, int& resultFrame_)
	{
		if (vIdx >= mHermites.size())
			return false;
		HermiteType_* pHermite = static_cast<HermiteType_*>(mHermites[vIdx]);
		if (!pHermite)
			return false;

		std::map<int, Echo::HermiteKey*>::const_iterator iter = pHermite->mKeyMap.lower_bound(static_cast<int>(frame));

		if (iter == pHermite->mKeyMap.end())
			return false;

		resultFrame_ = iter->first;

		KeyType* key_ = static_cast<KeyType*>(iter->second);
		if (!key_)
			return false;

		resultValue_ = key_->value;
		return true;
	}
	template<class HermiteType_, class KeyType, class valueType>
	inline void TimelineComponent::NoInterpolation(size_t vIdx, float frame, std::pair<valueType, bool>& noInterData,int& preFrame)
	{
		int idx_ = static_cast<int>(vIdx);
		float minFrame_ = std::min(mPreTime * mDeltaTime, frame);
		float maxFrame_ = mPreTime * mDeltaTime == minFrame_ ? frame : mPreTime * mDeltaTime;
		valueType resultValue;
		int resultFrame_ = static_cast<int>(frame);

		if (getNextKeyValue<HermiteType_, KeyType, valueType>(idx_, minFrame_, resultValue, resultFrame_))
		{
			bool isReverse = mIsReversePlay != PingPangOrder;

			float temp = 1e-5f;
			bool flag1 = !isReverse && ((resultFrame_ >= minFrame_ - temp && resultFrame_ < maxFrame_ + temp)
				         || (currTime >= endSceneTime - temp && currTime <= endSceneTime + temp
						 && resultFrame_ >= minFrame_ - temp && resultFrame_ <= maxFrame_ + temp));

			bool flag2 = isReverse && ((resultFrame_ > minFrame_ - temp && resultFrame_ <= maxFrame_ + temp)
				         || (currTime >= startSceneTime - temp &&  currTime <= startSceneTime + temp
						 && resultFrame_ >= minFrame_ - temp && resultFrame_ <= maxFrame_ + temp));
			
			preFrame = resultFrame_;

			if (flag1 || flag2)
			{
				noInterData.first = resultValue;
				noInterData.second = true;
			}
		}
	}
}
// namespace Echo
#endif 
