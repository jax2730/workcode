#include "EchoComponentStableHeaders.h"
#include "EchoTrajectoryComponent.h"
#include "EchoWorldNode.h"
#include "EchoSceneManager.h"
#include "EchoSceneNode.h"
#include "Actor/EchoActor.h"
#include "EchoActorProFuncImp.h"
#include "EchoHermite.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(TrajectoryComponent)
	TrajectoryComponentInfo::TrajectoryComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) : ActorComponentInfo(comType,MoveType),mMaxWeight(0)
	{

	}

	TrajectoryComponentInfo::~TrajectoryComponentInfo()
	{

	}

	void TrajectoryComponentInfo::ImportData(cJSON* jsonNode)
	{
		cJSON* pHermiteArray = cJSON_GetObjectItem(jsonNode, "TrajectoryPosition");
		if (pHermiteArray)
		{
			size_t i = 0;
			for (cJSON* pChild = pHermiteArray->child; nullptr != pChild; pChild = pChild->next)
			{
				EchoFloatHermite hermite;
				for (cJSON* pKey = pChild->child; pKey != nullptr; pKey = pKey->next)
				{
					int weight = 0;
					if (!ReadJSNode(pKey, "weight", weight))
						continue;
					HermiteFloatKey* hfKey = new HermiteFloatKey;
					ReadJSNode(pKey, "value", hfKey->value);
					ReadJSNode(pKey, "leftHandTangent", hfKey->leftHandTangent);
					ReadJSNode(pKey, "rightHandTangent", hfKey->rightHandTangent);
					ReadJSNode(pKey, "interpolateMode", hfKey->interpolateMode);
					hermite.mKeyMap.emplace(weight, hfKey);
				}
				mMaxWeight = std::max(mMaxWeight, hermite.GetMaxLength());
				mPosHermite[i] = hermite;
				++i;
			}
		}
	}

	void TrajectoryComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		if (comInfo->m_Type != m_Type)
			return;
		TrajectoryComponentInfo* tComInfo = static_cast<TrajectoryComponentInfo*>(comInfo);

		for (auto& hermite : tComInfo->mPosHermite)
		{
			for (auto& hKey : hermite.mKeyMap)
			{
				SAFE_DELETE(hKey.second);
			}
			hermite.mKeyMap.clear();
		}

		for (int i = 0; i < 3; ++i)
		{
			for (auto& item : mPosHermite[i].mKeyMap)
			{
				auto originKey = (HermiteFloatKey*)(item.second);
				auto newKey = new HermiteFloatKey;
				newKey->value = originKey->value;
				newKey->leftHandTangent = originKey->leftHandTangent;
				newKey->rightHandTangent = originKey->rightHandTangent;
				newKey->interpolateMode = originKey->interpolateMode;
				tComInfo->mPosHermite[i].mKeyMap.insert(std::make_pair(item.first, newKey));
			}
		}

		tComInfo->mMaxWeight = mMaxWeight;
	}

	void TrajectoryComponentInfo::OverWrite()
	{
	}

	void TrajectoryComponentInfo::ExportData(cJSON* jsonNode)
	{
		cJSON* pHermiteArray = cJSON_CreateArray();
		cJSON_AddItemToObject(jsonNode, "TrajectoryPosition", pHermiteArray);

		for (auto& hermite : mPosHermite)
		{
			cJSON* pHermite = cJSON_CreateArray();
			cJSON_AddItemToArray(pHermiteArray, pHermite);
			for (auto&hKey : hermite.mKeyMap)
			{
				auto hfKey = (HermiteFloatKey*)hKey.second;
				cJSON* pHermiteKey = cJSON_CreateObject();
				cJSON_AddItemToArray(pHermite, pHermiteKey);
				cJSON_AddNumberToObject(pHermiteKey, "weight", hKey.first);
				cJSON_AddNumberToObject(pHermiteKey, "value", hfKey->value);
				cJSON_AddNumberToObject(pHermiteKey, "leftHandTangent", hfKey->leftHandTangent);
				cJSON_AddNumberToObject(pHermiteKey, "rightHandTangent", hfKey->rightHandTangent);
				cJSON_AddNumberToObject(pHermiteKey, "interpolateMode", hfKey->interpolateMode);
			}
		}
	}

	/*TrajectoryComponent::TrajectoryComponent(Actor * pActor)
		: ActorComponent(pActor)
		
		, isShowHelper(false)
		, mHelperObj(NULL)
	{
		
	}*/

	TrajectoryComponent::TrajectoryComponent(ActorPtr actor, ActorCOMMoveableType type)
		: NodeActorComponent(actor, type)
		, isShowHelper(false)
		, mHelperObj(NULL)
	{
		
	}

	TrajectoryComponent::~TrajectoryComponent()
	{
		ReleaseHelper();
	}

	void TrajectoryComponent::CreatePoint(int weight, const Vector3 & pos)
	{
		if (m_pActComInfo == NULL)
			return;
		TrajectoryComponentInfo* info = static_cast<TrajectoryComponentInfo*>(m_pActComInfo);
		for (int i = 0; i < 3; i++)
		{
			info->mPosHermite[i].CreateKey(weight, pos[i]);
			info->mMaxWeight = std::max(info->mMaxWeight, info->mPosHermite[i].GetMaxLength());
		}
		if (isShowHelper)
			UpdateTrajectoryHelper();
	}

	void TrajectoryComponent::RemovePoint(int weight)
	{
		if (m_pActComInfo == NULL)
			return;
		TrajectoryComponentInfo* info = static_cast<TrajectoryComponentInfo*>(m_pActComInfo);
		info->mMaxWeight = 0;
		for (int i = 0; i < 3; i++)
		{
			if(info->mPosHermite[i].mKeyMap.find(weight) != info->mPosHermite[i].mKeyMap.end())
				info->mPosHermite[i].RemoveKey(weight);
			info->mMaxWeight = std::max(info->mMaxWeight, info->mPosHermite[i].GetMaxLength());
		}
		if (isShowHelper)
			UpdateTrajectoryHelper();
	}

	void TrajectoryComponent::RemoveKey(int axis, int weight)
	{
		if (m_pActComInfo == NULL)
			return;
		TrajectoryComponentInfo* info = static_cast<TrajectoryComponentInfo*>(m_pActComInfo);
		info->mMaxWeight = 0;
		info->mPosHermite[axis].RemoveKey(weight);
		for (int i = 0; i < 3; i++)
			info->mMaxWeight = std::max(info->mMaxWeight, info->mPosHermite[i].GetMaxLength());
		if (isShowHelper)
			UpdateTrajectoryHelper();
	}

	void TrajectoryComponent::ShowTrajectoryHelper(bool isShow)
	{
		if(mHelperObj == NULL)
			UpdateTrajectoryHelper();
		mHelperObj->setVisible(isShow);
		isShowHelper = isShow;
	}

	Vector3 TrajectoryComponent::InterpolationPos(float weight)
	{
		if (m_pActComInfo == NULL)
			return Vector3::ZERO;
		TrajectoryComponentInfo* info = static_cast<TrajectoryComponentInfo*>(m_pActComInfo);
		Vector3 pos = Vector3::ZERO;
		for (int i = 0; i < 3; i++)
			pos[i] = info->mPosHermite[i].Interpolate(weight);
		return pos;
	}

	Vector3 TrajectoryComponent::InterpolationWorldPos(float weight)
	{
		if (m_pActor == nullptr)
			return Vector3::ZERO;
		Vector3 localPos = InterpolationPos(weight);
		return m_pActor->convertLocalToWorldPosition(localPos);
	}

	void TrajectoryComponent::UpdateTrajectoryHelper()
	{
		if (m_pActor == nullptr)
			return;
		CreateHelper();
		mHelperObj->clear();
		mHelperObj->begin("Helper_VS.txt", "Helper_PS.txt", Echo::ECHO_LINE_LIST);
		UpdateVertex();
		mHelperObj->end();
		UpdateComRootNode();
	}

	void TrajectoryComponent::setActor(const ActorPtr& pActor)
	{
		if (pActor == m_pActor)
			return;
		if (pActor == nullptr && mHelperObj != NULL)
			detachFromNode(mHelperObj);
		m_pActor = pActor;
		if (m_pActor == nullptr)
			return;
		if (mHelperObj != NULL)
		{
			attachToNode(mHelperObj);
			UpdateComRootNode();
		}
	}

	void TrajectoryComponent::createComRootNodeImpl()
	{
		mComRootNode = Root::instance()->getMainSceneManager()->createSceneNode();
	}

	void TrajectoryComponent::CreateHelper()
	{
		if (mHelperObj != NULL)
			return;
		std::string manualObjName_ = getCombinedName() + "|| TrajectoryHelper";
		char idx = '0';
		while (Echo::Root::instance()->getMainSceneManager()->getManualObject(Name(manualObjName_.data())) != nullptr)
		{
			idx = idx + 1;
			manualObjName_ = manualObjName_.append(&idx, 1);
		}
		mHelperObj = Echo::Root::instance()->getMainSceneManager()->createManualObject(Name(manualObjName_.data()));
		attachToNode(mHelperObj);
		mHelperObj->setVisible(true);
	}

	void TrajectoryComponent::ReleaseHelper()
	{
		if (mHelperObj == NULL)
			return;
		if (m_pActor != nullptr)
			detachFromNode(mHelperObj);
		Echo::Root::instance()->getMainSceneManager()->destroyManualObject(mHelperObj);
		mHelperObj = NULL;
		isShowHelper = false;
	}

	void TrajectoryComponent::UpdateVertex()
	{
		if (m_pActComInfo == NULL)
			return;
		TrajectoryComponentInfo* info = static_cast<TrajectoryComponentInfo*>(m_pActComInfo);
		if (info->mMaxWeight == 0)
			return;
		float dettime = 0.01f;
		int num = (int)ceil((float)info->mMaxWeight / dettime);
		for (int i = 0; i < num; i++)
		{
			Echo::Vector3 lpos, rpos;
			float lefttime = dettime*i;
			float righttime = dettime*(i + 1);
			if (righttime >= (float)info->mMaxWeight)
				righttime = (float)info->mMaxWeight;
			lpos = InterpolationPos(lefttime);
			rpos = InterpolationPos(righttime);
			mHelperObj->position(lpos);
			mHelperObj->colour(0, 1, 0);
			mHelperObj->position(rpos);
			mHelperObj->colour(0, 1, 0);
		}
	}

	float TrajectoryComponent::GetDistance()
	{
		if (m_pActComInfo == NULL)
			return 0.0f;
		TrajectoryComponentInfo* info = static_cast<TrajectoryComponentInfo*>(m_pActComInfo);
		if (info->mMaxWeight == 0)
			return 0.0f;
		float dettime = 0.01f;
		distance = 0.0f;
		int num = (int)ceil((float)info->mMaxWeight / dettime);
		for (int i = 0; i < num; i++)
		{
			Echo::Vector3 lpos, rpos;
			float lefttime = dettime*i;
			float righttime = dettime*(i + 1);	
			if (righttime >= (float)info->mMaxWeight)
				righttime = (float)info->mMaxWeight;
			lpos = InterpolationPos(lefttime);
			rpos = InterpolationPos(righttime);

			distance += lpos.distance(rpos);
		}
		return distance;
	}

	void TrajectoryComponent::GetPosVec(float step, float wide)
	{	
		if (m_pActComInfo == NULL)
			return;
		TrajectoryComponentInfo* info = static_cast<TrajectoryComponentInfo*>(m_pActComInfo);
		float dettime = distance / step;
		int num = (int)ceil(dettime);
		posVec.clear();
		if (distance == 0)
			return;
		dettime = (float)info->mMaxWeight / dettime;
		for (int i = 0; i < num; i++)
		{
			Echo::Vector3 lpos, rpos;
			float lefttime = dettime*i;
			float righttime = dettime*(i + 1);
			lpos = InterpolationPos(lefttime);
			//posVec.push_back(lpos);
			rpos = InterpolationPos(righttime);

			Vector3 line = rpos - lpos;
			Vector3 Y(0.0f, 1.0f, 0.0f);
			Vector3 target = line.crossProduct(Y);
			target.normalise();

			Vector3 p = target * wide;
			Vector3 Pos1 = lpos + p;

			Vector3 Pos2 = lpos - p;

			posVec.push_back(Pos1);
			posVec.push_back(Pos2);
		}
		return;
	}

	Vector3 TrajectoryComponent::GetPos(int i)
	{
		return posVec[i];
	}

	void EchoTrajectoryProFuncImp::initProtocolFunction()
	{
		/*auto p = ProtocolComponentManager::instance();
		REGISTEROMFACTORY(TrajectoryComponent);

		p->registerProtocolFunc(PFID_Trajectory_Show, &EchoTrajectoryProFuncImp::ProCom_ShowTrajectory);*/
	}

	void EchoTrajectoryProFuncImp::uninitProtocolFunction()
	{

	}

	Echo::ProtocolResult EchoTrajectoryProFuncImp::ProCom_ShowTrajectory(void * isShow, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		ActorComponent* com = (ActorComponent*)handle;
		if (com == NULL)
			return PR_OK;
		TrajectoryComponent* trajectoryCom = dynamic_cast<TrajectoryComponent*>(com);
		if(trajectoryCom == NULL)
			return PR_OK;
		trajectoryCom->ShowTrajectoryHelper(*(bool*)isShow);
		return PR_OK;
	}

	bool EchoTrajectoryScript::Script_ShowTrajectory(Echo::ProtocolHandle actHandle, const std::string& comID, bool isShow)
	{
		ProtocolComponentSystem* proComponentMgr = ProtocolComponentSystem::instance();
		Echo::ProtocolHandle comHandle = 0;
		Echo::ActorComTypeDesc desc;
		desc.actHandle = actHandle;
		desc.comType = COMPONENTTYPE(TrajectoryComponent);
		desc.comName = comID;
		Echo::ProtocolHandle descHandle = (Echo::ProtocolHandle)&desc;
		Echo::ProtocolResult result = proComponentMgr->callProtocolFunc(PFID_Actor_GetComponent, &comHandle, descHandle);
		if(result != PR_OK)
			return false;
		result = proComponentMgr->callProtocolFunc(PFID_Trajectory_Show, &isShow, comHandle);
		if (result != PR_OK)
			return false;
		return true;
	}
	TrajectoryComponent * EchoTrajectoryScript::Script_GetTrajectoryComponent(Actor * pActor, const std::string & name)
	{
		ProtocolComponentSystem* pMgr = ProtocolComponentSystem::instance();
		ProtocolHandle comHandle = 0;
		ActorComTypeDesc desc;
		desc.actHandle = (Echo::ProtocolHandle) pActor;
		desc.comType = COMPONENTTYPE(TrajectoryComponent);
		desc.comName = name;
		ProtocolHandle descHandle = (ProtocolHandle)&desc;
		ProtocolResult result = pMgr->callProtocolFunc(PFID_Actor_GetComponent, &comHandle, descHandle);

		if (result != PR_OK)
			return nullptr;
		if (comHandle == 0)
			return nullptr;

		return  (TrajectoryComponent*)comHandle;
	}
}
