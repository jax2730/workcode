#include "EchoComponentStableHeaders.h"
#include "EchoGlobalLightComponent.h"
#include "EchoTODComponentManager.h"
#include "EchoTODComponentSystem.h"
#include "EchoTODDataUtil.h"
namespace Echo
{ 
	EDITOR_REFLECT_BEGIN(GlobalLightComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(mGlobalLightHandle.mGlobalSpecularRatio, "SpecularRatio", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mGlobalLightHandle.mGlobalReflectRatio, "ReflectRatio", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mGlobalLightHandle.mPriority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()
	DECLAREACTORCOMTYPE_CPP(GlobalLightComponent)

	Name globalLightSpecularRatio_ = Name("SpecularRatio");
	Name globalLightReflectRatio_ = Name("ReflectRatio");

	GlobalLightComponentInfo::GlobalLightComponentInfo(const Name& comType, ActorCOMMoveableType type) :ActorComponentInfo(comType, type)
	{

	}
	
	GlobalLightComponent::GlobalLightComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType) :ActorComponent(pActor, movableType)
	{
#ifdef ECHO_EDITOR
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_GLOBAL_SPECULAR_RATIO,		globalLightSpecularRatio_.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_GLOBAL_REFLECTION_RATIO,	globalLightReflectRatio_.toString(),	1);
#endif // ECHO_EDITOR
	}

	GlobalLightComponent::~GlobalLightComponent()
	{

	}

	void GlobalLightComponent::init(ActorComponentInfo* info)
	{
		TODCOMPONENT_INITMGR(getGlobalLightContext, m_tMgr, false);

		GlobalLightComponentInfo* infoPtr = static_cast<GlobalLightComponentInfo*>(info);
		if (infoPtr == nullptr)
			return;
		mContext = infoPtr->mGlobalLightHandle;
	}

	void GlobalLightComponent::unInit()
	{
		TODCOMPONENT_UNINITMGR(getGlobalLightContext, m_tMgr, T_End);
	}

	int GlobalLightComponent::getPriority() const 
	{ 
		return mContext.mPriority;
	}

	void GlobalLightComponent::visibleStateChanged(bool isVisible)
	{
		if (m_tMgr)
		{
			if (isVisible)
				m_tMgr->pushComponent(m_tMgr->getGlobalLightContext(), this);
			else
			    m_tMgr->popComponent(m_tMgr->getGlobalLightContext(), this);
		}
	}

	void GlobalLightComponent::use()
	{
		Echo::SceneManager* mainScene = getSceneManager();
		if (mainScene == nullptr)
			return;

		mainScene->setGlobalReflectionRatio(mContext.mGlobalReflectRatio);
		mainScene->setGlobalSpecularRatio(mContext.mGlobalSpecularRatio);
	}

	void GlobalLightComponent::unuse()
	{

	}

	bool GlobalLightComponent::isCurrentActiveCom()
	{
		if (!m_tMgr)
			return false;
		auto& Context = m_tMgr->getGlobalLightContext();
		if (Context.mActiveCom != nullptr && Context.mActiveCom == this)
			return true;
		return false;
	}

	void GlobalLightComponent::setPriority(const int& val)
	{
		int prior = val;
		if (prior > 255) prior = 255;
		else if (prior < 0) prior = 0;
		mContext.mPriority = prior;

		if (m_tMgr)
			m_tMgr->adjustComponentPriority(m_tMgr->getGlobalLightContext(), this);
	}

	void GlobalLightComponent::setTransFormTime(float time)
	{
		mContext.mTransFormTime = time;
	}

	void GlobalLightComponent::setSpecularRatio(const float& Ratio)
	{
		mContext.mGlobalSpecularRatio = Ratio;
		if (!isCurrentActiveCom())
			return;
		SceneManager* sMgr = getSceneManager();
		if (sMgr != nullptr)
		{
			sMgr->setGlobalSpecularRatio(Ratio);
		}
	}

	void GlobalLightComponent::setReflectRatio(const float& Ratio)
	{
		mContext.mGlobalReflectRatio = Ratio;
		if (!isCurrentActiveCom())
			return;
		SceneManager* sMgr = getSceneManager();
		if (sMgr != nullptr)
		{
			sMgr->setGlobalReflectionRatio(Ratio);
		}
	}

	void GlobalLightComponent::setGlobalRatio()
	{
		if (!isCurrentActiveCom())
			return;
		SceneManager* sMgr = getSceneManager();
		if (sMgr != nullptr)
		{
			sMgr->setGlobalSpecularRatio(mContext.mGlobalSpecularRatio);
			sMgr->setGlobalReflectionRatio(mContext.mGlobalReflectRatio);
		}
	}

	void GlobalLightComponent::setTimelineAttribute(std::vector<float>& data,const Name& name_)
	{
		if (m_tMgr)
			m_tMgr->setTodComChangeState(T_GlobalLight);

		if (name_ == globalLightSpecularRatio_ )
		{
			setSpecularRatio(data[0]);
		}
		else if (name_ == globalLightReflectRatio_ )
		{
			setReflectRatio(data[0]);
		}
	}

	void GlobalLightComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<GlobalLightComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void GlobalLightComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<GlobalLightComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void GlobalLightComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		reflect::TypeResolver<GlobalLightComponentInfo>::get()->copyTo(this, comInfo);
	}

	void GlobalLightComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<GlobalLightComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

}
