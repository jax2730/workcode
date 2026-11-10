#ifndef _EchoLensFlareComponent_h__
#define _EchoLensFlareComponent_h__
#include "EchoComponentPrerequisites.h"
#include "Actor/EchoProtocolActorComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoLensFlare.h"

namespace Echo{
	const size_t MAX_LENFLARE_COM_CNT = 4;
	class LensFlare;

	struct LensFlareComHandle
	{
		Vector3                       mCameraDirection = Vector3::ZERO;
		Vector3                       mCameraPosition = Vector3::ZERO;
		bool                          mLensFlareVisible = true;
		bool							mLensFlareDefaultPosition = true;
		std::vector<LensFlareParam>   mParamVec;
		Vector2                       mLensFlareScaleRange = Vector2(0.3f, 1.0f);
		std::string                   mTextureName = "";
		std::vector<float>            mLocationMin;
		std::vector<float>            mLocationMax;
		std::vector<float>            mSizeMin;
		std::vector<float>            mSizeMax;
		std::vector<float>            mOpacityMin;
		std::vector<float>            mOpacityMax;
		std::vector<int>              mRandomCount;

		std::vector<uint32>           mTextureIdx;
		std::vector<ColorValue>       mColor;
		bool                          mIsRotate0 = false;
		bool                          mIsRotate1 = false;
		bool                          mIsRotate2 = false;
		bool                          mIsRotate3 = false;

		Vector3                       mSunPosition = Vector3::ZERO;
		Vector3                       mSunDirection = Vector3::ZERO;

		LensFlareComHandle()
		{
			mLocationMin.reserve(MAX_LENFLARE_COM_CNT);
			mLocationMin.resize(MAX_LENFLARE_COM_CNT, 0.f);
			mLocationMax.reserve(MAX_LENFLARE_COM_CNT);
			mLocationMax.resize(MAX_LENFLARE_COM_CNT, 0.f);
			mSizeMin.reserve(MAX_LENFLARE_COM_CNT);
			mSizeMin.resize(MAX_LENFLARE_COM_CNT, 0.f);
			mSizeMax.reserve(MAX_LENFLARE_COM_CNT);
			mSizeMax.resize(MAX_LENFLARE_COM_CNT, 0.f);
			mOpacityMin.reserve(MAX_LENFLARE_COM_CNT);
			mOpacityMin.resize(MAX_LENFLARE_COM_CNT, 1.f);
			mOpacityMax.reserve(MAX_LENFLARE_COM_CNT);
			mOpacityMax.resize(MAX_LENFLARE_COM_CNT, 1.f);
			mRandomCount.reserve(MAX_LENFLARE_COM_CNT);
			mRandomCount.resize(MAX_LENFLARE_COM_CNT, 1);

			mTextureIdx.reserve(MAX_LENFLARE_COM_CNT);
			mTextureIdx.resize(MAX_LENFLARE_COM_CNT);
			mColor.reserve(MAX_LENFLARE_COM_CNT);
			mColor.resize(MAX_LENFLARE_COM_CNT);

			mParamVec.reserve(MAX_LENFLARE_COM_CNT);
			mParamVec.resize(MAX_LENFLARE_COM_CNT);
		}
	};

	class _EchoComponentExport LensFlareComponentInfo : public ActorComponentInfo
	{
	public:
		LensFlareComponentInfo() {}
		LensFlareComponentInfo(const Name& name, ActorCOMMoveableType isMoveable);
		virtual ~LensFlareComponentInfo() {}

		virtual void         ExportData(cJSON* jsonNode) override;
		virtual void         ImportData(cJSON* jsonNode) override;
		virtual void         CopyTo(ActorComponentInfo* info) override;
		virtual void         OverWrite() override;

#ifdef ECHO_EDITOR
		virtual bool         getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif
	public:
		LensFlareComHandle   mContext;
		EDITOR_REFLECT
	};

	class _EchoComponentExport LensFlareComponent : public ProtocolActorComponent
	{
		DECLAREACTORCOMTYPE_H(LensFlareComponent)
	public:
	
		virtual void                 registerTickTimeBegin() override;
		virtual void                 beginFrame(EventID, ProtocolHandle) override;
		virtual void					resourceArrived(const Name& resName, const Name& resType, bool bFail, const ResourceBasePtr& resourcePtr) override;

		void						 init(ActorComponentInfo* info) override;
		void                         unInit() override;
		void                         setLensFlareCamera(Camera* pCamera);
		void                         setLensFlareCameraDirection(const Vector3& direction_);
		void                         setLensFlareCameraPosition(const Vector3& position_);
		void                         setLensFlareCameraCameraAttribue(Camera* pCamera, const Vector3& direction_, const Vector3& position_);
		void                         setSunPosition(const Vector3& position_) { mContext.mSunPosition = position_; }
		const Vector3&               getSunPosition() { return mContext.mSunPosition; }
		void                         setSunDirection(const Vector3& direction) { mContext.mSunDirection = direction; }
		const Vector3&               getSunDirection() { return mContext.mSunDirection; }

		void                         setComLensFlareVisible(bool visible_);
		bool                         getComLensFlareVisible() { return mContext.mLensFlareVisible; }

		void							setComLensFlareDefaultPosition(bool default_) { mContext.mLensFlareDefaultPosition = default_; }
		bool							getComLensFlareDefaultPosition() { return mContext.mLensFlareDefaultPosition; }

		void                         setLensFlareScale(const Vector2& scale_);
		const Vector2&               getLensFlareScale() { return mContext.mLensFlareScaleRange; }
		void                         setLensFlareTexture(const std::string& textureName_, bool sync = false);
		const std::string&           getLensFlareTexture() { return mContext.mTextureName; }
		void                         setLensFlareParamsVec(const std::vector<LensFlareParam>& lensFlarePaream_);


		const std::vector<LensFlareParam>& getLensFlareParamsVec();
		void                         setLensFlareLocationMin(const std::vector<float>& locationMin_);
		const std::vector<float>&    getLensFlareLocationMin() { return mContext.mLocationMin; }
		void                         setLensFlareLocationMax(const std::vector<float>& locationMax_);
		const std::vector<float>&    getLensFlareLocationMax() { return mContext.mLocationMax; }
		void                         setLensFlareSizeMin(const std::vector<float>& sizeMin_);
		const std::vector<float>&    getLensFlareSizeMin() { return mContext.mSizeMin; }
		void                         setLensFlareSizeMax(const std::vector<float>& sizeMax_);
		const std::vector<float>&    getLensFlareSizeMax() { return mContext.mSizeMax; }
		void                         setLensFlareOpacityMin(const std::vector<float>& opacityMin);
		const std::vector<float>&    getLensFlareOpacityMin() { return mContext.mOpacityMin; }
		void                         setLensFlareOpacityMax(const std::vector<float>& opacityMax);
		const std::vector<float>&    getLensFlareOpacityMax() { return mContext.mOpacityMax; }
		void                         setLensFlareRandomCount(const std::vector<int>& randomCount_);
		const std::vector<int>&      getLensFlareRandomCount() { return mContext.mRandomCount; }

		void                         setLensFlareTextureIdx(int idx, uint32 lensFlareIdx_);
		void                         setLensFlareIsRotate(int idx, bool rotate_);
		void                         setLensFlareColor(int idx, const ColorValue& color_);

		bool							calculateDirection(const Vector3& position, Vector3& direction);
	private:
		void                         _resetComLensFlareElement();
		void                         _updateCameraLenFlare();
		void                         _initParamData();
	private:
		Camera*                      mCamera = nullptr;
		LensFlare*                   mLensFlare = nullptr;
	public:
		LensFlareComHandle           mContext;
	};

	DEFINECOMFACTORY(LensFlareComponent, ActorCOMMoveableType::UNMOVEABLE)
}


#endif
