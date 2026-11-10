#ifndef __EchoLightFlareComponent_h__ 
#define __EchoLightFlareComponent_h__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoProtocolActorComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoLensFlare.h"
#include "EchoPhysicsManager.h"

namespace Echo {

	struct LensFlareParam;

	enum LightFlareType{
		One_Photo = 1,
		Four_Photo = 4
	};
	enum LightFlareModel {
		M_NearBigFarSmall,
		M_NearSmallBigFar
	};

	const float minSizeFactor = 0.f;

	struct LightFlareComdata {
		Vector3                       mLightPosition = Vector3::ZERO; //世界位置
		bool                          mLightFlareVisible = true;
		std::vector<LensFlareParam>   mParamVec;
		Vector2                       mLightFlareScaleRange = Vector2(0.3f, 1.0f);
		std::string                   mTextureName = "";
		int                           mPriority = 0;
		LightFlareType                mLightFlareType = Four_Photo;

		Vector2                       mLightFlareRealSize =  Vector2::UNIT_SCALE;
		LightFlareModel               mLightFlareModel = M_NearBigFarSmall;

		float                         mAppearDistance = 3.f;
		float                         mDisAppearDistance = 2.f;
		
		float                         mFirstLevelDistance = 200.f;
		float                         mSecondLevelDistance = 600.f;

		std::vector<float>            mMinSizePercentVec;
		bool						  mObstructingOpacity = true;

		LightFlareComdata()
		{
			mParamVec.resize(4);
			mMinSizePercentVec.resize(4);
		}
	};
	class _EchoComponentExport LightFlareComponentInfo : public ActorComponentInfo
	{
	public:
		LightFlareComponentInfo() {}
		LightFlareComponentInfo(const Name& name, ActorCOMMoveableType isMoveable);
		virtual ~LightFlareComponentInfo() {}

		virtual void         ExportData(cJSON* jsonNode) override;
		virtual void         ImportData(cJSON* jsonNode) override;
		virtual void         CopyTo(ActorComponentInfo* info) override;
		virtual void         OverWrite() override;

#ifdef ECHO_EDITOR
		virtual bool         getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif
	public:
		LightFlareComdata    mContext;
		EDITOR_REFLECT
	};


	class LightFlareComponentManager;
	class _EchoComponentExport LightFlareComponent : public ActorComponent
	{
		friend class LightFlareComponentManager;
		DECLAREACTORCOMTYPE_H(LightFlareComponent)
	public:
		virtual void                  visibleStateChanged(bool isVisible) override;
		virtual void				  resourceArrived(const Name& resName, const Name& resType, bool bFail, const ResourceBasePtr& resourcePtr) override;

		void						  init(ActorComponentInfo* info) override;
		void                          unInit() override;

		virtual void                  initRenderObject(ActorComponentInfo* info) override;

		void                          setLightPosition(const Vector3 position_) { mContext.mLightPosition = position_; }
		const Vector3&                getLightPosition() { return mContext.mLightPosition; }

		void                          setLightFlareParamsVec(const std::vector<LensFlareParam>& lightFlarePaream_);
		const std::vector<LensFlareParam>& getLightFlareParamsVec();

		void                          setLightFlareTextureName(const std::string& textureName_,bool isSync = false);
		const std::string&            getLightFlareTextureName() { return mContext.mTextureName; }

		void                          setComLightFlareVisible(bool visible_);
		bool                          getComLightFlareVisible() { return mContext.mLightFlareVisible; }

		void                          setLightFlareScale(const Vector2& scale_);
		const Vector2&                getLightFlareScale() { return mContext.mLightFlareScaleRange; }
	    
		void                          setPriority(int priority_) { mContext.mPriority = priority_; }
		int                           getPriority() { return mContext.mPriority; }

		void                          setLightFlareType(LightFlareType type_);
		LightFlareType                getLightFlareType() { return mContext.mLightFlareType; }

		void                          setLightFlareModel(LightFlareModel model_) { mContext.mLightFlareModel = model_; }
		LightFlareModel               getLightFlareModel() { return mContext.mLightFlareModel; }

		void                          setLightFlareRealSize(const Vector2& size_) { mContext.mLightFlareRealSize = size_; }
		const Vector2&                getLightFlareRealSize() { return mContext.mLightFlareRealSize; }

		void                          setLightFlareAppearDistance(float distance_) { mContext.mAppearDistance = distance_; }
		float                         getLightFlareAppearDistance() { return mContext.mAppearDistance; }
		
		void                          setLightFlareDisAppearDistance(float distance_);
		float                         getLightFlareDisAppearDistance() { return mContext.mDisAppearDistance; }

		void                          setLightFlareFilter(const EchoSceneQueryFilter& flareFilter_);
		void                          setLightFlareComMgr(LightFlareComponentManager* lMgr = nullptr);

		void                          setFirstLevelDistance(float distance_);
		float                         getFirstLevelDistance() { return mContext.mFirstLevelDistance; }

		void                          setSecondLevelDistance(float distance_);
		float                         getSecondLevelDistance() { return mContext.mSecondLevelDistance; }

		void                          setLightFlareMinSizePercent(const std::vector<float>& minSizePercent_);
		const std::vector<float>&     getLightFlareMinSizePercent() { return mContext.mMinSizePercentVec; }

		void						  setObstructingOpacity(bool isOpen) { mContext.mObstructingOpacity = isOpen; }
		bool						  getObstructingOpacity() { return mContext.mObstructingOpacity; }

		void                          setLightFlareMinSizePercentByIdx(int i ,float percent);
		float                         getLightFlareMinSizePercentByIdx(int i);

	private:
		void                          _initCamera();
		void                          _createLightFlare();
		void                          _deleteLightFlare();
		void                          _resetLightFlareData();

		//管理器更新使用
		void                          _computeLightFlareData();
		bool                          _getLightFlareIsAtConeOfVision();
		bool                          _getLightFlareIsVisible();
		void                          _computeLightFlareDegreeofOcclusion(int resultSize);
		void                          _computeLightPointDir(int resultSize);

		void                          _computeLightPointDir(int resultSize,const Vector3& unit_x,const Vector3& unit_y,const Vector2& lightSize_,const float transArray[][2]);

		void                          _setHalfRoundDirection(int size);
		float                         _getOcclusionPercent(int size);
		void                          _updateLightFlare();

		void                          _updateLightFlareOpacity();

		//当visible为false，或者mLensFlare为空指针时置为false
		bool                          _getLightFlareState();
		void                          _setLightFlareResultSize();
	private:
		LensFlare*                    mLensFlare = nullptr;
		Camera*                       mCamera = nullptr;
		LightFlareComdata             mContext;

		Vector3                       mLightDir = Vector3::ZERO;
		float                         mLightDistance = 0.f;
		Vector3                       mCameraPos = Vector3::ZERO;

		std::vector<float>            mDisVec;
		std::vector<Vector3>          mDirVec;

		float                         mOcclusionRatio = 1.f;
		float                         mRangeScaleByDistance = 1.f;

		EchoSceneQueryFilter          mLightFlareFilter = nullptr;

		LightFlareComponentManager*   mLightFlareMgr = nullptr;

		bool                          mIsHalfModel = false;
		Vector2                       mHalfRoundDirection = Vector2::UNIT_Y;
		uint8                         mShelterPointSet[16];
		
		float                         mGlobalOpacity = 1.f;
		////测试
		//std::vector<HelperObject*>    hel;
	};

	DEFINECOMFACTORY(LightFlareComponent, ActorCOMMoveableType::UNMOVEABLE)

	void _EchoComponentExport WriteJSNode(JSNODE jsonNode, const char* Name, const LensFlareParam& triggerEventDesc);
	bool _EchoComponentExport ReadJSNode(cJSON* jsonNode, const char* Name, LensFlareParam& triggerEventDesc);

	void _EchoComponentExport WriteJSNode(JSNODE jsonNode, const char* Name,  LightFlareType type);
	bool _EchoComponentExport ReadJSNode(cJSON* jsonNode, const char* Name, LightFlareType& type);

	void _EchoComponentExport WriteJSNode(JSNODE jsonNode, const char* Name, LightFlareModel model);
	bool _EchoComponentExport ReadJSNode(cJSON* jsonNode, const char* Name, LightFlareModel& model);

}

#endif
