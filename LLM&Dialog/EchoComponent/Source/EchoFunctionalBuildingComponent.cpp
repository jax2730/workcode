#include "EchoComponentStableHeaders.h"
#include "EchoFunctionalBuildingComponent.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(FunctionalBuildingComponent)

	EDITOR_REFLECT_BEGIN(FunctionalBuildingComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_type, "建筑类型", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()

	FunctionalBuildingComponentInfo::FunctionalBuildingComponentInfo(const Name& comType, const ActorCOMMoveableType type) :
		ActorComponentInfo(comType, type) {}

	void FunctionalBuildingComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<FunctionalBuildingComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void FunctionalBuildingComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<FunctionalBuildingComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void FunctionalBuildingComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)return;

		const auto info = dynamic_cast<FunctionalBuildingComponentInfo*>(comInfo);
		reflect::TypeResolver<FunctionalBuildingComponentInfo>::get()->copyTo(this, info);
	}

	FunctionalBuildingComponent::FunctionalBuildingComponent(ActorPtr pActor, const ActorCOMMoveableType movableType) :
		ActorComponent(std::move(pActor), movableType) { }

	FunctionalBuildingComponent::~FunctionalBuildingComponent()
	{

	}

	void FunctionalBuildingComponentInfo::Overlay(ActorComponentInfo* srcInfo) {}

	void FunctionalBuildingComponentInfo::OverWrite()
	{
		if (mOriginComInfo == nullptr) return;

		reflect::TypeResolver<FunctionalBuildingComponentInfo>::get()->overWrite(mOriginComInfo, this);
	}

	void FunctionalBuildingComponent::init(ActorComponentInfo* info)
	{
		if (info == nullptr) return;
		m_Info = dynamic_cast<FunctionalBuildingComponentInfo*>(info);
		if (!m_Info) return;
	}

	void FunctionalBuildingComponent::connectCom()
	{
		if (m_pActorPtr && m_pActorPtr->getActorType() == ActorType::ActorType_Logic)
		{
			m_pActorPtr->notifyLogicActor(static_cast<LogicType>(m_Info->m_type), true);
		}
	}

	void FunctionalBuildingComponent::disconnectCom()
	{
		if (m_pActorPtr && m_pActorPtr->getActorType() == ActorType::ActorType_Logic)
		{
			m_pActorPtr->notifyLogicActor(static_cast<LogicType>(m_Info->m_type), false);
		}
	}

	void FunctionalBuildingComponent::copyTo(const ActorComponentPtr& actorComponentPtr)
	{
	}
}
