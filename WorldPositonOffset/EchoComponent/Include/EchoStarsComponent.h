#ifndef __ECHO_STARS_COMPONENT_H__
#define __ECHO_STARS_COMPONENT_H__

#include "EchoPrerequisites.h"
#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"

namespace Echo {

	class Stars;
	class TODComponentManager;

	struct StarsComponentHandle {
		float    mStarsSize = 0.f;
		float    mStarsIntensity = 0.f;
		float    mStarsHDRIntensity = 0.5f;
		bool     mIsShowLowerPart = false;

		float	 mTransFormTime = 0.f;
		int      mPriority = 0;
	};

	class _EchoComponentExport StarsComponentInfo : public ActorComponentInfo {
	public:
		StarsComponentInfo() = default;
		StarsComponentInfo(const Name& comType, ActorCOMMoveableType type);
		virtual ~StarsComponentInfo() = default;

		void									ExportData(cJSON* jsonNode) override;
		void									ImportData(cJSON* jsonNode) override;
		void									CopyTo(ActorComponentInfo* comInfo) override;
		void									OverWrite() override;
	public:
		StarsComponentHandle                    mContext;
		EDITOR_REFLECT
	};

	class _EchoComponentExport StarsComponent : public ActorComponent {
		DECLAREACTORCOMTYPE_H(StarsComponent)

	public:
		virtual void							init(ActorComponentInfo* info) override;
		virtual void                            unInit() override;
		virtual void                            visibleStateChanged(bool isVisible) override;
		virtual void							setTimelineAttribute(std::vector<float>& data,const Name& name) override;
	public:
		void									use();
		void									unuse();

		void                                    setStarsAttribute();
		void                                    setStarsSize(float size_);
		float                                   getStarsSize() { return mContext.mStarsSize; }
		void                                    setStarsIntensity(float intensity_);
		float                                   getStarsIntensity() { return mContext.mStarsIntensity; }
		void                                    setStarsHDRIntensity(float HDRIntensity_);
		float                                   getStarsHDRIntensity() { return mContext.mStarsHDRIntensity; }
		void                                    setIsUseLowerStarsPart(bool isUse_);
		bool                                    getIsUseLowerStarsPart() { return mContext.mIsShowLowerPart; }
		void                                    setPriority(int priority_);
		int                                     getPriority() { return mContext.mPriority; }
		void									setTransFormTime(float time) { mContext.mTransFormTime = time; }
		float									getTransFormTime() const { return mContext.mTransFormTime; }
		TODComponentManager*                    getTodComManager() { return m_tMgr; }
		const StarsComponentHandle&				getContext() { return mContext; }
	private:
		StarsComponentHandle                    mContext;
		Stars*                                  mStars = nullptr;
		TODComponentManager*                    m_tMgr = nullptr;
	};

	DEFINECOMFACTORY(StarsComponent, UNMOVEABLE)
}

#endif
