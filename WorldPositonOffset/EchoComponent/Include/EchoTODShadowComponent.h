#ifndef _Echo_TOD_ShadowComponent_h__
#define _Echo_TOD_ShadowComponent_h__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActor.h"
#include "EchoMovableObject.h"
#include "EchoComInfoDefineMacro.h"
#include "EchoComDefineMacro.h"
	
namespace Echo {

	struct TodShadowDataHandle {
		float         mShadowWeight = 0.9f;

		float         m_AssaoShadowStrength = 4.3f;
		float         m_AssaoShadowPower = 1.5f;
		float		  mTransFormTime = 0.f;
		int           mPriority = 0;
	};

	class TODComponentManager;

	static Name ShadowWeightName_ = Name("TODShadowWeight");

	class _EchoComponentExport TODShadowComponentInfo : public ActorComponentInfo {
	public:
		TODShadowComponentInfo() {}
		TODShadowComponentInfo(const Name& comType, ActorCOMMoveableType type);
		virtual ~TODShadowComponentInfo();
		virtual void							 ExportData(cJSON* jsonNode) override;
		virtual void							 ImportData(cJSON* jsonNode) override;
		virtual void						     CopyTo(ActorComponentInfo* comInfo) override;
		virtual void                             Overlay(ActorComponentInfo* srcInfo) override;
		virtual void                             OverWrite() override;
	public:
		TodShadowDataHandle                      mContext;
		EDITOR_REFLECT
	};

	class _EchoComponentExport TODShadowComponent : public ActorComponent {
		DECLAREACTORCOMTYPE_H(TODShadowComponent)
	public:
		void						            init(ActorComponentInfo* info) override;
		virtual void                            unInit() override;
		virtual void							copyTo(const ActorComponentPtr&) override;
		virtual void                            setTimelineAttribute(std::vector<float>& data, const Name& name) override;
		virtual void                            visibleStateChanged(bool isVisible) override;

		void                                    use();
		void                                    unuse();


		void                                    setShadowWeight(float shadowWeight_);
		float                                   getShadowWeight() { return mContext.mShadowWeight; }

		void                                    setAssaoShadowStrength(float strength_);
		float                                   getAssaoShadowStrength() { return mContext.m_AssaoShadowStrength; }
		void                                    setAssaoShadowPower(float power_);
		float                                   getAssaoShadowPower() { return mContext.m_AssaoShadowPower; }

		void                                    setPriority(int priority_);
		int                                     getPriority() { return mContext.mPriority; }
		void									setTransFormTime(float time) { mContext.mTransFormTime = time; }
		float									getTransFormTime() const { return mContext.mTransFormTime; }
		TODComponentManager*                    getTodComManager() { return m_tMgr; }
	private:
		TODComponentManager*                    m_tMgr = nullptr;
		bool                                    mIsActiveCom = false;
	public:
		TodShadowDataHandle                     mContext;
	};

	DEFINECOMFACTORY(TODShadowComponent, UNMOVEABLE)
}

#endif
