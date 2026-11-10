#include "EchoComponentStableHeaders.h"
#include "EchoEnvironmentLightComponent.h"
#include "EchoReflectCoeResource.h"
#include "EchoTODComponentManager.h"
#include "EchoTextureManager.h"
#include "EchoTODComponentSystem.h"
#include "EchoTODDataUtil.h"
#include "EchoStringTable.h"

namespace Echo {
	DECLAREACTORCOMTYPE_CPP(EnvironmentLightComponent)

	EDITOR_REFLECT_BEGIN(EnvironmentLightComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mGlobalEnvironmentTexName, "GlobalEnvironmentTexName", EditorUI_DEFUALT, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(mGlobalSHCoeFileName, "GlobalSHCoeName", EditorUI_DEFUALT, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mEnvironmentExplosure, "EnvironmentExplosure", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mPriority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()

	Name EnvironmentLightExplosureName_ = Name("EnvironmentLightExplosure");

	EnvironmentLightComponent::EnvironmentLightComponent(ActorPtr pActor, ActorCOMMoveableType movableType)
		:ActorComponent(pActor,movableType)
	{
		//mEnvLight = new Echo::EnvironmentLight();
		//isUse = false;
		//isRealTime = false;
		//if (mEnvLight)
		//{
		//	mEnvLight->setManager(Root::instance()->getMainSceneManager());
		//}

#ifdef ECHO_EDITOR
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_GLOBAL_ENVIRONMENT_EXPLOSURE, EnvironmentLightExplosureName_.toString(), 1);
#endif
	}

	EnvironmentLightComponent::~EnvironmentLightComponent() {

	}

	void EnvironmentLightComponent::unInit()
	{
		TODCOMPONENT_UNINITMGR(getEnvironmentLightContext, m_tMgr, T_End);
	}

	void EnvironmentLightComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->pushComponent(m_tMgr->getEnvironmentLightContext(), this);
		else
			m_tMgr->popComponent(m_tMgr->getEnvironmentLightContext(), this);
	}

	void EnvironmentLightComponent::use()
	{
		isUse = true;
		auto envInstance = EnvironmentLight::instance();
		envInstance->setGlobalEnvironmentExplosure(mEnvironmentExplosure, EnvironmentLight::DataFromComponent);
		envInstance->setGlobalEnvironmentTexName(mGlobalEnvironmentTexName, EnvironmentLight::DataFromComponent,false,true);
		envInstance->setGlobalSphericalHarmonicsFileName(mGlobalSHCoeFileName,EnvironmentLight::DataFromComponent,false,true);
	}

	void EnvironmentLightComponent::unuse()
	{
		isUse = false;
		auto envInstance = EnvironmentLight::instance();
		envInstance->setGlobalEnvironmentExplosure(0,EnvironmentLight::DataFromComponent);
		envInstance->setGlobalEnvironmentTexName("",EnvironmentLight::DataFromComponent);
		envInstance->setGlobalSphericalHarmonicsFileName("",EnvironmentLight::DataFromComponent);
	}


	EnvironmentLightComponentInfo::EnvironmentLightComponentInfo(const Name& comType, ActorCOMMoveableType type)
		:ActorComponentInfo(comType,type)
	{
		mGlobalEnvironmentTexName = StringTable::instance()->getString("Light", "GlobalEnvironmentTexture");
	}

	EnvironmentLightComponentInfo::~EnvironmentLightComponentInfo() 
	{
	}

	void EnvironmentLightComponentInfo::ExportData(cJSON* jsonNode) 
	{
		reflect::TypeResolver<EnvironmentLightComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void EnvironmentLightComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<EnvironmentLightComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void EnvironmentLightComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		EnvironmentLightComponentInfo* envInfo = static_cast<EnvironmentLightComponentInfo*>(comInfo);
		reflect::TypeResolver < EnvironmentLightComponentInfo > ::get()->copyTo(this, envInfo);
	}

	void EnvironmentLightComponentInfo::OverWrite() 
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<EnvironmentLightComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}
#ifdef ECHO_EDITOR
	bool EnvironmentLightComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		bool needCopy = false;
		if (!mGlobalEnvironmentTexName.empty())
		{
			resMap.insert(std::make_pair(ResourceType::NeedCopyRes, mGlobalEnvironmentTexName));
			needCopy = true;
			auto extPos = mGlobalEnvironmentTexName.rfind(".");
			if (!(extPos == std::string::npos)){
				std::string shcFileName = mGlobalEnvironmentTexName;
				shcFileName.erase(extPos);
				shcFileName.append(".shc");
				resMap.insert(std::make_pair(ResourceType::NeedCopyRes, shcFileName));
			}
		}
		//if (!mGlobalSHCoeFileName.empty())
		//{
		//	resMap.insert(std::make_pair(ResourceType::NeedCopyRes, mGlobalSHCoeFileName));
		//	needCopy = true;
		//}

		return needCopy;
	}

#endif

	void EnvironmentLightComponent::init(ActorComponentInfo* info)
	{
		TODCOMPONENT_INITMGR(getEnvironmentLightContext, m_tMgr, false);

		if (info == nullptr)return;
		EnvironmentLightComponentInfo* mEnvInfo = static_cast<EnvironmentLightComponentInfo*>(info);

		//auto extPos = mEnvInfo->mGlobalEnvironmentTexName.rfind(".");
		//if (extPos == std::string::npos)
		//{
		//	Echo::LogManager::instance()->logMessage("EnvironmentLightComponent: Failed to get shc file name, default shc will be used", LML_CRITICAL);
		//	setGlobalSHCoeFileName("");
		//}
		//else {
		//	std::string shcFileName = mEnvInfo->mGlobalEnvironmentTexName;
		//	shcFileName.erase(extPos);
		//	shcFileName.append(".shc");
		//	setGlobalSHCoeFileName(shcFileName);
		//}
		setEnvironmentExplosure(mEnvInfo->mEnvironmentExplosure);
		setGlobalEnvironmentTextureName(mEnvInfo->mGlobalEnvironmentTexName);		
	}

	void EnvironmentLightComponent::setRealTime(bool isUse_)
	{
		//isRealTime = isUse_;
	}

	void EnvironmentLightComponent::setPriority(int priority)
	{
		auto mEnvInfo = static_cast<EnvironmentLightComponentInfo*>(m_pActComInfo);
		if (mEnvInfo == NULL)
			return;
		mEnvInfo->mPriority = priority;
		mPriority = priority;
		if (m_tMgr)
			m_tMgr->adjustComponentPriority(m_tMgr->getEnvironmentLightContext(), this);
	}

	void EnvironmentLightComponent::setTransFormTime(float TransFormTime)
	{
		mTransFormTime = TransFormTime;
	}

	void EnvironmentLightComponent::updateTodItemState(bool flag_)
	{
	//	TODComponentManager* mgr = TODComponentManager::instance();
		if (flag_ == false)
		{
			setRealTime(false);
		}
	}


	EchoEnvironmentLightScript::EchoEnvironmentLightScript(const EchoActorScript& pActor, const std::string& name) 
	:EchoActorComponentScript(pActor,name)
	{
	}

	EchoEnvironmentLightScript EchoEnvironmentLightScript::Script_GetEnvironmentConponent(const EchoActorScript& pActor, const std::string& name)
	{
		return EchoEnvironmentLightScript(pActor, name);
	}

	void EnvironmentLightComponent::setTimelineAttribute(std::vector<float>& data, const Name& name)
	{
		if (m_pActComInfo == nullptr)
			return;
		m_tMgr->setTodComChangeState(T_EnvironmentLight);
		if (name == EnvironmentLightExplosureName_)
		{
			//mEnvInfo->mEnvironmentExplosure = data[0];
			setEnvironmentExplosure(data[0]);
		}
		setGlobalEnvironmentTextureName(mGlobalEnvironmentTexName, false);
		//setGlobalSHCoeFileName(mGlobalSHCoeFileName, false);
	}

	void EnvironmentLightComponent::copyTo(const ActorComponentPtr& newObj)
	{
	}

	void EnvironmentLightComponent::setGlobalEnvironmentTextureName(const std::string& value, bool forceUpdate /* = false */)
	{
		mGlobalEnvironmentTexName = value;
		auto extPos = mGlobalEnvironmentTexName.rfind(".");
		mGlobalSHCoeFileName = value;

		if (extPos == std::string::npos)
		{
			mGlobalSHCoeFileName = "";
		}
		else {
			mGlobalSHCoeFileName.erase(extPos);
			mGlobalSHCoeFileName.append(".shc");
		}

		if (!m_tMgr)
			return;
		const auto& enContext = m_tMgr->getEnvironmentLightContext();
		if (enContext.mActiveCom == this) {
			EnvironmentLight::instance()->setGlobalEnvironmentTexName(mGlobalEnvironmentTexName, EnvironmentLight::DataFromComponent, false, forceUpdate);
			EnvironmentLight::instance()->setGlobalSphericalHarmonicsFileName(mGlobalSHCoeFileName, EnvironmentLight::DataFromComponent, false, forceUpdate);
		}
	}

	//void EnvironmentLightComponent::setGlobalSHCoeFileName(const std::string& value, bool forceUpdate /* = false */)
	//{
	//	mGlobalSHCoeFileName = value;

	//	if (!m_tMgr)
	//		return;
	//	const auto& enContext = m_tMgr->getEnvironmentLightContext();
	//	if (enContext.mActiveCom == this)
	//		EnvironmentLight::instance()->setGlobalSphericalHarmonicsFileName(mGlobalSHCoeFileName,EnvironmentLight::DataFromComponent,false,forceUpdate);
	//}

	void EnvironmentLightComponent::setEnvironmentExplosure(float value)
	{
		mEnvironmentExplosure = value;

		if (!m_tMgr)
			return;
		auto enContext = m_tMgr->getEnvironmentLightContext();
		if (enContext.mActiveCom == this)
			EnvironmentLight::instance()->setGlobalEnvironmentExplosure(mEnvironmentExplosure, EnvironmentLight::DataFromComponent);
	}

	void EchoEnvironmentLightScript::setTodComImmediateComponent(bool isUse)
	{
		EnvironmentLightComponent* currCom = static_cast<EnvironmentLightComponent*>(m_Ptr.get());
		if (currCom == nullptr)return;

	}

	void EchoEnvironmentLightScript::setPriority(int priority)
	{
	}
}
