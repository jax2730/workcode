
#ifndef _EchoSunComponent_h__
#define _EchoSunComponent_h__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActor.h"
#include "EchoMovableObject.h"

#include "EchoComInfoDefineMacro.h"
#include "EchoComDefineMacro.h"

namespace Echo {
	class SunComponentInfo;
	const size_t  MAX_COM_MOON_PHASE_CNT = 10;
	struct sunHandle
	{
		ColorValue    mSunColor;
		ColorValue    mEmmittingColor;
		float         mInRadius = 0.25f;
		float         mOutRadius = 10;
		float         mOpacity = 1;
	};

	struct SunDataContext {
		Vector4       mSunColor = Vector4(1.f, 1.f, 1.f, 1.f);
		float         mSunIntensity = 1.f;
		Vector4       mEmmittingColor = Vector4(1.f, 1.f, 1.f, 1.f);
		float         mInRadius = 0.25f;
		float         mOutRadius = 10;
		float         mOpacity = 1;
		float         mHDRIntensity = 0.5f;
		float         mTransFormTime = 0.f;
		int           mPriority = 0;

		std::string   mSunTextureName;
		std::vector<std::string> mMoonTextureName;

		SunDataContext()
		{
			mMoonTextureName.reserve(MAX_COM_MOON_PHASE_CNT);
			mMoonTextureName.resize(MAX_COM_MOON_PHASE_CNT);
		}
	};

	class TODComponentManager;
	class _EchoComponentExport SunComponent : public ActorComponent {
		DECLAREACTORCOMTYPE_H(SunComponent)
	public:
		REGISTER_LUA_CLASS_KEY;
		void						            init(ActorComponentInfo* info) override;
		virtual void                            unInit() override;
		virtual void							copyTo(const ActorComponentPtr& ) override;
		virtual void                            setTimelineAttribute(std::vector<float>& data,const Name& name) override;
		virtual void                            visibleStateChanged(bool isVisible) override;

		void                                    use();
		void                                    unuse();
		void                                    setSunComColor(const Vector4& sunColor_);
		const Vector4&                          getSunComColor() { return mContext.mSunColor; }
		void                                    setSunIntensity(float intensity_);
		float                                   getSunIntensity() { return mContext.mSunIntensity; }
		void                                    setHDRIntensity(float HdrIntensity_);
		float                                   getHDRIntensity() { return mContext.mHDRIntensity; }
		void                                    setSunColor();
		Vector4                                 getSunColor();
		void                                    setEmmittingColor(const Vector4& color_);
		Vector4                                 getEmmittingColor();
		void                                    setInRadius(const float& inRadius_);
		float                                   getInRadius();
		void                                    setOutRadius(const float& outRadius_);
		float                                   getOutRadius();
		void                                    setOpacity(const float& Opacity_);
		float                                   getOpacity();
		void                                    setPriority(const int& priority_);
		int                                     getPriority();
		void                                    setDirection(const Vector3& dir_);
		void                                    setTransFormTime(float transTime_); 
		float                                   getTransFormTime();

		void                                    setSonTexture(const std::string& sunTextureName_);
		std::string&                            getSunTexture() { return mContext.mSunTextureName; }
		void                                    setMoonTextures(const std::vector<std::string>& moonTextures);
		const std::vector<std::string>&         getMoonTextures() { return mContext.mMoonTextureName; }
		void                                    _setComMoonTexture();
		TODComponentManager*                    getTodComManager() { return m_tMgr; }
	public:
		SunDataContext                          mContext;
	private:
		Sun*                                    mSun = nullptr;
		TODComponentManager*                    m_tMgr = nullptr;
	};

	class _EchoComponentExport SunComponentInfo : public ActorComponentInfo {
	public:
		SunComponentInfo() {}
		SunComponentInfo(const Name& comType, ActorCOMMoveableType type);
		virtual ~SunComponentInfo();
		virtual void									ExportData(cJSON* jsonNode) override;
		virtual void									ImportData(cJSON* jsonNode) override;
		virtual void									CopyTo(ActorComponentInfo* comInfo) override;
		virtual void                                    Overlay(ActorComponentInfo* srcInfo) override;
		virtual void                                    OverWrite() override;

#ifdef ECHO_EDITOR
		virtual bool                                    getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif
	public:
		SunDataContext                                  mContext;
		EDITOR_REFLECT
	};

	DEFINECOMFACTORY(SunComponent, UNMOVEABLE)

};

#endif
