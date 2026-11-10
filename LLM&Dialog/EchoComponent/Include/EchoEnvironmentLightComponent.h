#ifndef __EchoSkyLightingComponent_h_
#define __EchoSkyLightingComponent_h_

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoMovableObject.h"

#include "EchoComInfoDefineMacro.h"
#include "EchoHelperObj.h"
#include "EchoHelperObjProFuncImp.h"
#include "EchoComDefineMacro.h"

#include "EchoEnvironmentLight.h"


namespace Echo {

	class TODComponentManager;

	class _EchoComponentExport EnvironmentLightComponentInfo :public ActorComponentInfo {
	public:
		EnvironmentLightComponentInfo() {}
		EnvironmentLightComponentInfo(const Name& comType, ActorCOMMoveableType type);
		virtual ~EnvironmentLightComponentInfo();
		virtual void						ExportData(cJSON* jsonNode)override;
		virtual void						ImportData(cJSON* jsonNode)override;
		virtual void						CopyTo(ActorComponentInfo* comInfo)override;
		virtual void                        Overlay(ActorComponentInfo* srcInfo) override {}
		virtual void						OverWrite()override;
	public:

		String		mGlobalEnvironmentTexName;	//= Echo::String("Textures/cube/sp_BonifacioStreet.dds");
		//String		mGlobalSHCoeFileName		= Echo::String("Textures/cube/sp_BonifacioStreet.shc");
		float		mEnvironmentExplosure		= 1.f;
		float       mTransFormTime				= 0.f;
		int			mPriority					= 0;

		EDITOR_REFLECT

#ifdef ECHO_EDITOR
	public:
		bool getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)override;
#endif
	};


	class _EchoComponentExport EnvironmentLightComponent : public ActorComponent {
		DECLAREACTORCOMTYPE_H(EnvironmentLightComponent)
	public:
		void								init(ActorComponentInfo* info)override;
		virtual  void                       unInit() override;


		virtual void                        visibleStateChanged(bool isVisible) override;

		void								copyTo(const ActorComponentPtr&)override;
		void								setTimelineAttribute(std::vector<float>& data, const Name& name)override;

		void								setGlobalEnvironmentTextureName(const std::string& value,bool forceUpdate = false);
		//void								setGlobalSHCoeFileName(const std::string& value, bool forceUpdate = false);
		void								setEnvironmentExplosure(float value);

		Echo::String						getGlobalEnvironmentTextureName()const { return mGlobalEnvironmentTexName; }
		Echo::String						getGlobalSHCoeFileName()const { return mGlobalSHCoeFileName; }
		float								getEnvironmentExplosure()const { return mEnvironmentExplosure; }

		void								setRealTime(bool isUse_);
		bool								getRealTime()const { return isRealTime; }

		void								setPriority(int priority);
		int									getPriority()const { return mPriority; }

		void								setTransFormTime(float TransFormTime);
		float								getTransFormTime() const { return mTransFormTime; }

		void								updateTodItemState(bool flag_);
		TODComponentManager*                getTodComManager() { return m_tMgr; }
	public:
		void								use();
		void								unuse();
	//public:	
	//	EnvironmentLightComponentInfo*		mEnvInfo = nullptr;
	private:
		bool								isRealTime = false;
		bool								isUse;
		float								mTransFormTime = 0.f;
		int									mPriority;
	private:
		Echo::Matrix4						mRMat;
		Echo::Matrix4						mGMat;
		Echo::Matrix4						mBMat;

		String								mGlobalEnvironmentTexName;
		String								mGlobalSHCoeFileName;
		float								mEnvironmentExplosure;

		TODComponentManager*                m_tMgr = nullptr;
	};

	DEFINECOMFACTORY(EnvironmentLightComponent,UNMOVEABLE)

	class EchoEnvironmentLightScript : public EchoActorComponentScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;
	public:
		EchoEnvironmentLightScript() = default;
		EchoEnvironmentLightScript(const EchoActorScript& pActor, const std::string& name);
	public:
		static		EchoEnvironmentLightScript Script_GetEnvironmentConponent(const EchoActorScript& pActor, const std::string& name);
		void		setTodComImmediateComponent(bool isUse);
		void		setPriority(int priority_);
		void		updateTodItemState(bool flag_) {}
	};
}


#endif
