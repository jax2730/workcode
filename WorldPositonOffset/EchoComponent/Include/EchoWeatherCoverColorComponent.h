#ifndef _EchoWeatherCoverColorComponent_h__
#define _EchoWeatherCoverColorComponent_h__
#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActor.h"
#include "EchoMovableObject.h"

#include "EchoComInfoDefineMacro.h"
#include "EchoComDefineMacro.h"

namespace Echo {
	
	struct WeatherCoverColorComData {
		ColorValue    mCoverColor = ColorValue::White;
		float         mCoverColorIntensity = 1.f;
		bool          mIsUseCoverColor = false;
		float         mTransTime = 0.f;
		Vector3       mOverlayPos = Vector3::ZERO;
		float         mOverlayRadius = 0.f;
		int           mPriority = 0;
	};
	class TODComponentManager;

	class _EchoComponentExport WeatherCoverColorComponentInfo : public ActorComponentInfo {
	public:
		WeatherCoverColorComponentInfo() {}
		WeatherCoverColorComponentInfo(const Name& comType, ActorCOMMoveableType type);
		virtual ~WeatherCoverColorComponentInfo();
		virtual void									ExportData(cJSON* jsonNode) override;
		virtual void									ImportData(cJSON* jsonNode) override;
		virtual void									CopyTo(ActorComponentInfo* comInfo) override;
		virtual void                                    Overlay(ActorComponentInfo* srcInfo) override;
		virtual void                                    OverWrite() override;
	public:
		WeatherCoverColorComData                        mContext;
		EDITOR_REFLECT
	};

	class _EchoComponentExport WeatherCoverColorComponent : public ActorComponent {
		DECLAREACTORCOMTYPE_H(WeatherCoverColorComponent)
	public:
		void						            init(ActorComponentInfo* info) override;
		virtual void                            unInit() override;
		virtual void							copyTo(const ActorComponentPtr&) override;
		virtual void                            setTimelineAttribute(std::vector<float>& data,const Name& name) override;
		virtual void                            visibleStateChanged(bool isVisible) override;

		void                                    use();
		void                                    unuse();


		void                                    resetCoverColor();
		void                                    setOverlayPosition(const Vector3& position_);
		const Vector3&                          getOverlayPosition() { return mContext.mOverlayPos; }

		void                                    setOverlayRadius(float radiu_);
		float                                   getOverlayRadius() { return mContext.mOverlayRadius; }

		void                                    setOverlayData();

		void                                    setTransFormTime(float transTime_);
		float                                   getTransFormTime() { return mContext.mTransTime; }
		void                                    setIsUseCoverColor(bool isUseCoverColor_);
		bool                                    getIsUseCoverColor() { return mContext.mIsUseCoverColor; }
		void                                    setCoverColor(const ColorValue& coverColor_);
		const ColorValue&                       getCoverColor() { return mContext.mCoverColor; }
		void                                    setCoverColorIntensity(float intensity_);
		float                                   getCoverColorIntensity() { return mContext.mCoverColorIntensity; }
		void                                    setPriority(int priority_);
		int                                     getPriority() { return mContext.mPriority; }
		TODComponentManager*                    getTodComManager() { return m_tMgr; }

		const WeatherCoverColorComData&			getContext() { return mContext; }
	private:
		WeatherCoverColorComData                mContext;
		TODComponentManager*                    m_tMgr = nullptr;
	};

	DEFINECOMFACTORY(WeatherCoverColorComponent, UNMOVEABLE)
}
#endif
