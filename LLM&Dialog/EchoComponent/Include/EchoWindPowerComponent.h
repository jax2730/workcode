#pragma once
#ifndef __ECHO_WINDPOWER_COMPONENT_H__
#define __ECHO_WINDPOWER_COMPONENT_H__
#include "EchoVector3.h"
#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"
#include "EchoMath.h"
namespace Echo{
	struct WindPowerHandle {
		Vector3									windDirection =Vector3::ZERO;
		float									windPower = 0.0;
		float									windAngle = 0.0;
		float									treeSwayAmp = 0.0;
		float									treeSwayFreq = 0.0;
		float									treeShakeAmp = 0.0;
		float									treeShakeFreq = 0.0;
		float									vegSwayAmp = 0.0;
		float									vegSwayFreq = 0.0;
		float									vegShakeAmp = 0.0;
		float									vegShakeFreq = 0.0;

		float									mTransFormTime = 0.f;
		int										mPriority = 0;
	};

	class TODComponentManager;
	class _EchoComponentExport WindPowerComponentInfo : public ActorComponentInfo
	{

	public:
		float									getWindPower() const { return mWindPowerHandle.windPower; }
		float									getWindAngle() const { return mWindPowerHandle.windAngle; }
		Vector3									getWindDirection() const { return mWindPowerHandle.windDirection; }
		void									setWindPower(const float& val) { mWindPowerHandle.windPower = val; }
		void									setWindAngle(const float& val) { mWindPowerHandle.windAngle = val; }
		void									setWindDirection(const Vector3& val) { mWindPowerHandle.windDirection = val; }

		float									getTreeSwayAmp() const { return mWindPowerHandle.treeSwayAmp; }
		float									getTreeSwayFreq() const { return mWindPowerHandle.treeSwayFreq; }
		float									getTreeShakeAmp() const { return mWindPowerHandle.treeShakeAmp; }
		float									getTreeShakeFreq() const { return mWindPowerHandle.treeShakeFreq; }
		void									setTreeSwayAmp(const float& val) { mWindPowerHandle.treeSwayAmp = val; }
		void									setTreeSwayFreq(const float& val) { mWindPowerHandle.treeSwayFreq = val; }
		void									setTreeShakeAmp(const float& val) { mWindPowerHandle.treeShakeAmp = val; }
		void									setTreeShakeFreq(const float& val) { mWindPowerHandle.treeShakeFreq = val; }

		float									getVegSwayAmp() const { return mWindPowerHandle.vegSwayAmp; }
		float									getVegSwayFreq() const { return mWindPowerHandle.vegSwayFreq; }
		float									getVegShakeAmp() const { return mWindPowerHandle.vegShakeAmp; }
		float									getVegShakeFreq() const { return mWindPowerHandle.vegShakeFreq; }
		void									setVegSwayAmp(const float& val) { mWindPowerHandle.vegSwayAmp = val; }
		void									setVegSwayFreq(const float& val) { mWindPowerHandle.vegSwayFreq = val; }
		void									setVegShakeAmp(const float& val) { mWindPowerHandle.vegShakeAmp = val; }
		void									setVegShakeFreq(const float& val) { mWindPowerHandle.vegShakeFreq = val; }
		int										getPriority() const { return mWindPowerHandle.mPriority; };
		void									setPriority(const int& _priority) { mWindPowerHandle.mPriority = _priority; }
		WindPowerComponentInfo() = default;
		WindPowerComponentInfo(const Name& comType, ActorCOMMoveableType type) ;
		~WindPowerComponentInfo() = default;
		WindPowerHandle							mWindPowerHandle;
		virtual void							ExportData(cJSON* jsonNode) override;
		virtual void							ImportData(cJSON* jsonNode) override;
		virtual void							CopyTo(ActorComponentInfo* comInfo) override;
		virtual void                            OverWrite() override;
	public:
		EDITOR_REFLECT
	};
	class  _EchoComponentExport WindPowerComponent : public ActorComponent {
		DECLAREACTORCOMTYPE_H(WindPowerComponent)
	public:
		void										init(ActorComponentInfo* info) override;
		virtual void                                visibleStateChanged(bool isVisible) override;
		bool										isCurrentActiveCom();
		virtual  void								unInit() override;
		int											getPriority() const;
		void										setPriority(const int& val);
		float										getTransFormTime() const { return mContext.mTransFormTime; }
		void										setTransFormTime(float time) { mContext.mTransFormTime = time; }
		void										use() ;
		void										unuse();
		virtual void								setTimelineAttribute(std::vector<float>& data, const Name& name) override;
		void										setGlobalWinderPower(const float& wp);
		void										setGlobalWinderAngle(const float& Angle);
		void										setGlobalTreeSwayAmp(const float& value);
		void										setGlobalTreeSwayFreq(const float& value);
		void										setGlobalTreeShakeAmp(const float& value);
		void										setGlobalTreeShakeFreq(const float& value);
		void										setGlobalVegSwayAmp(const float& value);
		void										setGlobalVegSwayFreq(const float& value);
		void										setGlobalVegShakeAmp(const float& value);
		void										setGlobalVegShakeFreq(const float& value);
		void										useWindParas();
		void										useTreeParas();
		void										useVegParas();
		TODComponentManager*                        getTodComManager() { return m_tMgr; }
		WindPowerHandle								getContext() const { return mContext; }
		
	private:
		WindPowerHandle								mContext;
		TODComponentManager*                        m_tMgr = nullptr;
	};
	DEFINECOMFACTORY(WindPowerComponent, UNMOVEABLE)
}
#endif 
