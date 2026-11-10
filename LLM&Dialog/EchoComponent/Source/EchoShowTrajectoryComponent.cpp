// Author: anhaoxiang
#include "EchoComponentStableHeaders.h"
#include "EchoShowTrajectoryComponent.h"
#include "EchoActorProFuncImp.h"
#include "EchoTimelineComponent.h"

namespace Echo {

	int binaryFindResultIdx(std::vector<int>& vec, const int& st, const int &en, const int& k)
	{
		if (st >= en)
			return -1;
		int sz = (int)vec.size();
		if (vec.size() == 1)
			return 0;
		int mid = (st + en) / 2;
		if (vec[mid] == k)
			return mid;
		else if (vec[mid] < k)
		{
			if (mid + 1 == sz || vec[mid + 1] > k)
				return mid;
			else
				return binaryFindResultIdx(vec, mid, en, k);
		}
		else
		{
			return binaryFindResultIdx(vec, st, mid, k);
		}
		return -1;
	}


DECLAREACTORCOMTYPE_CPP(ShowTrajectoryComponent)
}


Echo::ShowTrajectoryComponentInfo::ShowTrajectoryComponentInfo(const Name& name, ActorCOMMoveableType moveable):
	ActorComponentInfo(name,moveable)
{
}

Echo::ShowTrajectoryComponentInfo::~ShowTrajectoryComponentInfo()
{
}

void Echo::ShowTrajectoryComponentInfo::ExportData(cJSON * jsonNode)
{
	cJSON_AddStringToObject(jsonNode, "selectTimelineName", selectTimelineName.data());
	cJSON_AddBoolToObject(jsonNode, "isShow", isShow);
	cJSON_AddNumberToObject(jsonNode, "startCnt", startCnt);
}

void Echo::ShowTrajectoryComponentInfo::ImportData(cJSON * jsonNode)
{
	ReadJSNode(jsonNode, "selectTimelineName", selectTimelineName);
	ReadJSNode(jsonNode, "isShow", isShow);
	ReadJSNode(jsonNode, "startCnt", startCnt);
}

void Echo::ShowTrajectoryComponentInfo::CopyTo(ActorComponentInfo * info)
{
	auto newObjectInfo = dynamic_cast<ShowTrajectoryComponentInfo*>(info);
	newObjectInfo->selectTimelineName = selectTimelineName;
	newObjectInfo->isShow = isShow;
	newObjectInfo->startCnt = startCnt;
}

void Echo::ShowTrajectoryComponentInfo::OverWrite()
{
	if (mOriginComInfo != nullptr)
	{
		ShowTrajectoryComponentInfo* oriInfo = static_cast<ShowTrajectoryComponentInfo*> (mOriginComInfo);
		//reflect::TypeResolver<ShowTrajectoryComponentInfo>::get()->overWrite(oriInfo, this);
		if (!IsCustomParam("selectTimelineName") && !IsRealCustomParam("selectTimelineName"))
		{
			selectTimelineName = oriInfo->selectTimelineName;
		}
		if (!IsCustomParam("isShow") && !IsRealCustomParam("isShow"))
		{
			selectTimelineName = oriInfo->selectTimelineName;
		}
		if (!IsCustomParam("startCnt") && !IsRealCustomParam("startCnt"))
		{
			selectTimelineName = oriInfo->selectTimelineName;
		}
	}
}

Echo::ShowTrajectoryComponent::ShowTrajectoryComponent(ActorPtr  pActor, ActorCOMMoveableType isMoveable):
	ProtocolNodeActorComponent(pActor,isMoveable)
{
	registerTickTimeBegin();
}

Echo::ShowTrajectoryComponent::~ShowTrajectoryComponent()
{
	if (m_pActor == nullptr)
		return;
	if (mHelperObj != nullptr)
	{
		Echo::Root::instance()->getMainSceneManager()->destroyManualObject(mHelperObj);
	}
	selectTimeline = nullptr;
}

void Echo::ShowTrajectoryComponent::init(ActorComponentInfo * info)
{
	TrajectoryInfo = dynamic_cast<ShowTrajectoryComponentInfo*>(info);
	if (mHelperObj)
		Echo::Root::instance()->getMainSceneManager()->destroyManualObject(mHelperObj);
	mHelperObj = Echo::Root::instance()->getMainSceneManager()->createManualObject(Name(getCombinedName() + "||" + "showTrajector"));
	/*if (TrajectoryInfo->selectTimelineName != "")
	{
         getTimelineComponent();
	}
	getTrajectory();
	showTrajectory();*/
}

void Echo::ShowTrajectoryComponent::copyTo(const ActorComponentPtr&  actCom)
{
}

void Echo::ShowTrajectoryComponent::setActor(const ActorPtr&  pActor)
{
	if (pActor == m_pActor)
		return;
	if (m_pActor == nullptr)
		return;
	if (m_pActor == nullptr)
		return;
	m_pActor = pActor;
}

void Echo::ShowTrajectoryComponent::bindNode()
{
	if (mHelperObj)
	{
		if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
		{
            attachToNode(mHelperObj);
		    UpdateComRootNode();
			getTrajectory();
			showTrajectory();
		}
	}
}

void Echo::ShowTrajectoryComponent::beginFrame(EventID eventID, ProtocolHandle handle)
{
	if (TrajectoryInfo == nullptr)
		return;
	switch (eventID)
	{
	case EID_EveryFrame:
	{
		if (TrajectoryInfo->isShow)
		{
			getTrajectory();
		}
		showTrajectory();
		break;
	}
	default:
		break;
	}
}


void Echo::ShowTrajectoryComponent::registerTickTimeBegin()
{
	m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
	registerTickTimeEnd();
}

void Echo::ShowTrajectoryComponent::getTimelineComponent()
{
	selectTimeline =static_cast<TimelineComponent*>( m_pActor->getComponent(TrajectoryInfo->selectTimelineName).get());
}

void Echo::ShowTrajectoryComponent::getTrajectory()
{
	getTimelineComponent();
	if (selectTimeline == nullptr)
	{
		mHelperObj->clear();
		return;
	}
    
    int temp = binaryFindResultIdx(selectTimeline->AttributeBegin, 0, static_cast<int>(selectTimeline->AttributeBegin.size()), TrajectoryInfo->startCnt);
	if (temp == -1)
		return;
	auto tempCom = selectTimeline->AttributeComponents[temp];
	if (tempCom == nullptr)
		return;
	if (tempCom->getParentNodeCom() == getParentNodeCom())
		isSameSource = true;
	else
	{
		associatedName = tempCom->getComNameToString();
        isSameSource = false;
	}
		
    
	if (getPosition() != Vector3(0.f, 0.f, 0.f))
		setPosition(Vector3(0.f, 0.f, 0.f));

	mHelperObj->clear();
	mHelperObj->begin("Helper_VS.txt", "Helper_PS.txt", Echo::ECHO_LINE_LIST);

	float startTime = selectTimeline->startSceneTime;
	float endTime = selectTimeline->endSceneTime;
	float timeTemp = 0.01f;

	for (float currTime= startTime ; currTime < endTime; currTime += timeTemp)
	{
		Vector3 lPos, rPos;
		float leftTime = currTime;
		float rightTime = currTime + timeTemp;
		if (rightTime > endTime)
			rightTime = endTime;
		float deltaTime = selectTimeline->mDeltaTime;
		int lhs = static_cast<int>(leftTime*deltaTime + 0.5);
		int rhs = static_cast<int>(rightTime*deltaTime + 0.5);
			

		for (int i = TrajectoryInfo->startCnt; i < TrajectoryInfo->startCnt + 3; i++ )
		{
			lPos[i - TrajectoryInfo->startCnt] = selectTimeline->floatInterpolation(i, (float)lhs);
			rPos[i - TrajectoryInfo->startCnt] = selectTimeline->floatInterpolation(i, (float)rhs);
		}

		mHelperObj->position(lPos);
		mHelperObj->colour(1, 0, 0);
		mHelperObj->position(rPos);
		mHelperObj->colour(1, 0, 0);
	}
	mHelperObj->end();
	UpdateComRootNode();
//	mHelperObj->setVisible(true);
}

void Echo::ShowTrajectoryComponent::showTrajectory()
{
	mHelperObj->setVisible(TrajectoryInfo->isShow);
}

bool Echo::ShowTrajectoryComponent::getIsSameSource()
{
	return isSameSource;
}

Echo::ActorPtr   Echo::ShowTrajectoryComponent::getActor()
{
	return m_pActor;
}

std::string Echo::ShowTrajectoryComponent::getAssociatedName()
{
	return associatedName;
}

void Echo::ShowTrajectoryComponent::createComRootNodeImpl()
{
	mComRootNode = Root::instance()->getMainSceneManager()->createSceneNode();
}

Echo::ShowTrajectoryComponent * Echo::EchoShowTrajectoryScript::Script_GetShowTrajectoryComponent(ActorPtr  p_Actor, const std::string name)
{
	//if (p_Actor == nullptr)
	//	return nullptr;
	//auto comHandle = p_Actor->getComponent(Name(name));

	//return  (ShowTrajectoryComponent*)comHandle;
	return nullptr;
}
