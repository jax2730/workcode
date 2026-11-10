#pragma once
#ifndef __ECHO_GLOBAL_LIGHT_COMPONENT_H__
#define __ECHO_GLOBAL_LIGHT_COMPONENT_H__
#include "EchoPrerequisites.h"
#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"
namespace Echo {
	struct GlobalLightHandle 
	{
		float mGlobalSpecularRatio = 0.8f;
		float mGlobalReflectRatio = 10.0f;

		float mTransFormTime = 0.f;
		int	  mPriority = 0;
	};

	class TODComponentManager;
	class _EchoComponentExport GlobalLightComponentInfo : public ActorComponentInfo
	{

	public:
		float									getGlobalSpecularRatio() const { return mGlobalLightHandle.mGlobalSpecularRatio; };
		void									setGlobalSpecularRatio(const float& Ratio) { mGlobalLightHandle.mGlobalSpecularRatio = Ratio; };
		float									getGlobalReflectRatio() const { return mGlobalLightHandle.mGlobalReflectRatio; };
		void									setGlobalReflectRatio(const float& Ratio) { mGlobalLightHandle.mGlobalReflectRatio = Ratio; };
		int										getPriority() const { return mGlobalLightHandle.mPriority; };
		void									setPriority(const int& _priority) { mGlobalLightHandle.mPriority = _priority; }
		
		virtual void							ExportData(cJSON* jsonNode) override;
		virtual void							ImportData(cJSON* jsonNode) override;
		virtual void							CopyTo(ActorComponentInfo* comInfo) override;
		virtual void                            OverWrite() override;
		GlobalLightComponentInfo() = default;
		GlobalLightComponentInfo(const Name& comType, ActorCOMMoveableType type);
		~GlobalLightComponentInfo() = default;
		GlobalLightHandle							mGlobalLightHandle;
	public:
		EDITOR_REFLECT
	};
	class  _EchoComponentExport GlobalLightComponent : public ActorComponent {
		DECLAREACTORCOMTYPE_H(GlobalLightComponent)
	public:
		void										init(ActorComponentInfo* info) override;
		virtual  void								unInit() override;
		GlobalLightHandle							getContext() const { return mContext; };
		virtual void                                visibleStateChanged(bool isVisible) override;
		bool										isCurrentActiveCom();
		int											getPriority() const ;
		void										setPriority(const int& val);
		float										getTransFormTime() const { return mContext.mTransFormTime; }
		void										setTransFormTime(float time);
		void										setSpecularRatio(const float& Ratio);
		void										setReflectRatio(const float& Ratio);
		void										setGlobalRatio();
		void										use();
		void										unuse();
		virtual void								setTimelineAttribute(std::vector<float>& data,const Name& name) override;
		TODComponentManager*                        getTodComManager() { return m_tMgr; }
	private:
		GlobalLightHandle							mContext;
		TODComponentManager*                        m_tMgr = nullptr;
	};
	DEFINECOMFACTORY(GlobalLightComponent, UNMOVEABLE)
}
#endif // !__ECHO_GLOBAL_LIGHT_COMPONENT_H__
