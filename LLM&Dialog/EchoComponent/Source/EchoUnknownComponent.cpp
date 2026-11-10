#include "EchoComponentStableHeaders.h"
#include "EchoUnknownComponent.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(UnknownComponent)
	UnknownComponentInfo::UnknownComponentInfo(const Name& type, ActorCOMMoveableType movable)
		: ActorComponentInfo(type,movable)
		, mCopyJson(NULL)
	{
	}

	UnknownComponentInfo::~UnknownComponentInfo()
	{
		delete[] mCopyJson;
	}

	void UnknownComponentInfo::ExportData(cJSON * jsonNode)
	{
		const char* temp = getCopy();
		*jsonNode = *cJSON_Parse(temp);
	}

	void UnknownComponentInfo::ImportData(cJSON * jsonNode)
	{
		char* temp = cJSON_Print(jsonNode);
		size_t tempSize = strlen(temp);
		mCopyJson = new char[tempSize + 1];
		memcpy(mCopyJson, temp, tempSize + 1);
	}

	void UnknownComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		if (comInfo->m_Type != m_Type)
			return;
		UnknownComponentInfo* ukComInfo = static_cast<UnknownComponentInfo*>(comInfo);

		if(ukComInfo->mCopyJson != nullptr)
			delete[] ukComInfo->mCopyJson;

		if (!mCopyJson)
			return;
		size_t tempSize = strlen(mCopyJson);
		ukComInfo->mCopyJson = new char[tempSize + 1];
		memcpy(ukComInfo->mCopyJson, mCopyJson, tempSize + 1);
	}

	void UnknownComponentInfo::Overlay(ActorComponentInfo* comInfo)
	{
	}

	void UnknownComponentInfo::OverWrite()
	{
	}

	UnknownComponent::UnknownComponent(ActorPtr pActor, ActorCOMMoveableType movable)
		:NodeActorComponent(pActor, movable)
	{
	}

	UnknownComponent::~UnknownComponent()
	{
		
	}

	void UnknownComponent::init(ActorComponentInfo * info)
	{
	}
}
