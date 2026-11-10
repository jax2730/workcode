#include "EchoComponentStableHeaders.h"
#include "EchoEmitterComponent.h"
#include "Actor/EchoReflect.h"

#define ReturnObjNULL if(!m_pEmitterObj) return

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(EmitterComponent)

	//----------------------------------------------------------------------------------------------------

	EDITOR_REFLECT_BEGIN(EmitterComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_actorFile, "ActorFile", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(emitterData, "EmitterData", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	EmitterComponentInfo::EmitterComponentInfo(const Echo::Name& comType, ActorCOMMoveableType moveType)
		: ActorComponentInfo(comType, moveType)
	{
	}
	EmitterComponentInfo::~EmitterComponentInfo()
	{
	}
#ifdef ECHO_EDITOR
	bool EmitterComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		if (m_actorFile.empty()) {
			return false;
		}
		resMap.insert(std::make_pair(ResourceType::Actor, m_actorFile.toString()));
		return true;
	}
#endif
	_DEFINEINFO_VIRFUN_BASE(EmitterComponentInfo)

	//----------------------------------------------------------------------------------------------------

	EmitterComponent::EmitterComponent(ActorPtr  pActor, ActorCOMMoveableType movableType)
		: ProtocolNodeActorComponent(pActor, movableType)
	{
	}

	EmitterComponent::~EmitterComponent()
	{
	}
	void EmitterComponent::init(ActorComponentInfo* info)
	{
		SetNotifiedUpdate(true);
		ProtocolNodeActorComponent::init(info);
		m_pActComInfo = info;
		EmitterComponentInfo* mInfo = static_cast<EmitterComponentInfo*>(m_pActComInfo);
		if (mInfo)
		{
			m_EmitterData = mInfo->emitterData;
			m_actorFile = mInfo->m_actorFile;
		}
		m_pEmitterObj = new EffectEmitterObject(this, &m_EmitterData);
		if (ActorSystem::instance()->getMainActorManager()->IsEditorPlay())
		{
			m_pEmitterObj->active();
		}
		/*oldpos = newpos;
		newpos = getWorldPosition();
		scale = getSceneNode()->getScale();*/

	}
	void EmitterComponent::unInit()
	{
		SetNotifiedUpdate(false);
		ProtocolNodeActorComponent::unInit();
		SAFE_DELETE(m_pEmitterObj);
	}
	void EmitterComponent::bindNode()
	{
	}
	void EmitterComponent::replay(ActorComponentInfo* info)
	{
	}
	void EmitterComponent::registerTickTimeBegin()
	{
		m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}
	void EmitterComponent::beginFrame(EventID eventId, ProtocolHandle)
	{
		switch (eventId)
		{
		case EID_EveryFrame:
		{
			tick();
			break;
		}
		default:
			break;
		}
	}
	void EmitterComponent::NotifiChildImp(int changeMask)
	{
		oldpos = newpos;
		newpos = getSceneNode()->_getDerivedPosition();
		scale = getSceneNode()->getScale();
	}
	const DVector3& EmitterComponent::getWorldPosition()
	{
		return newpos;
	}
	const DVector3& EmitterComponent::getLastWorldPosition()
	{
		return oldpos;
	}
	const Vector3& EmitterComponent::getScale()
	{
		return scale;
	}
	void EmitterComponent::setScale(const Vector3& _scale)
	{
		NodeActorComponent::setScale(_scale);
		scale = _scale;
	}
	const Matrix4& EmitterComponent::getLocalMatrix()
	{
		localMatrix.makeTransform(getPosition(), getScale(), getRotation());
		return localMatrix;
	}
	const DMatrix4& EmitterComponent::getWorldMatrix()
	{
		worldMatrix.makeTransform(getWorldPosition(), getWorldScale(), getWorldRotation());
		return worldMatrix;
	}
	void EmitterComponent::EditorPlayImp()
	{
		ReturnObjNULL;

		m_pEmitterObj->active();
	}
	void EmitterComponent::EditorPauseImp(bool pause)
	{
		ReturnObjNULL;

		if (pause)
			m_pEmitterObj->freeze();
		else
			m_pEmitterObj->active();
		
	}
	void EmitterComponent::EditorStopImp()
	{
		if (m_pEmitterObj)
			m_pEmitterObj->reset();
	}
	void EmitterComponent::tick()
	{
		ReturnObjNULL;

		if (!m_pEmitterObj->isActive() || m_actorFile.empty())
			return;
		float _dt = Root::instance()->getTimeSinceLastFrameClient();
		m_pEmitterObj->Tick(_dt);

		m_pEmitterObj->TickWorldTransform(getWorldMatrix());

		unsigned int count = 0;
		const std::vector<EffectParticle*>& actorsActiveVec = m_pEmitterObj->getParticlesActive();
		count = (uint16)actorsActiveVec.size();

		for (unsigned int i = 0; i < count; i++)
		{
			EffectParticle* pact = actorsActiveVec[i];
			ActorEmitterVisualData* pVisualData = static_cast<ActorEmitterVisualData*>(pact->mpVisualData);
			if (pVisualData == nullptr)
			{
				pVisualData = new ActorEmitterVisualData(this);
				if (!m_actorFile.empty())
				{
					pVisualData->init(m_actorFile);
				}
				pact->mpVisualData = pVisualData;
			}
			ActorPtr act = pVisualData->m_actor;
			if (act.expired() || !act->isCreateFinish())
				continue;
			//act->setPosition(pact->mParticleData.mPosition);
			act->setWorldPosition(pact->mParticleData.mSynchroData.mWorldPosition);
			act->setWorldRotation(pact->mParticleData.mSynchroData.mParentWorldQuaternion * pact->mParticleData.mSynchroData.mTickThreadLocalRotation);
			act->setWorldScale(pact->mParticleData.mSynchroData.mSize * pact->mParticleData.mSynchroData.mParentWorldScale);
			act->setVisiable(true);
		}
	}
	uint16 EmitterComponent::getEmittedActorNum()
	{
		ReturnObjNULL 0;

		return m_pEmitterObj->GetParticleQuota();
	}
	void EmitterComponent::setEmittedActorNum(uint16 num)
	{
		ReturnObjNULL;

		m_pEmitterObj->SetParticleQuota(num);
		m_EmitterData.m_ParticleNum = num;
	}
	EffectEmitterType EmitterComponent::getEmitterType()
	{
		ReturnObjNULL EffectEmitterType::EMT_BASIC;

		return m_pEmitterObj->m_pEmitterData->mEmitterType;
	}
	void EmitterComponent::setEmitterType(EffectEmitterType type)
	{
		ReturnObjNULL;

		if (m_EmitterData.mEmitterType == type || type == EffectEmitterType::EMT_BASIC || type == EffectEmitterType::EMT_MAX)
			return;
		m_EmitterData.mEmitterType = type;
		m_pEmitterObj->SetEmitterData(&m_EmitterData);
	}
	float EmitterComponent::getEmitterDelayTime()
	{
		ReturnObjNULL 0.0f;

		return m_pEmitterObj->GetDelayTime();
	}
	void EmitterComponent::setEmitterDelayTime(float delayTime)
	{
		ReturnObjNULL;

		if (delayTime <= 0)
			return;
		m_pEmitterObj->SetDelayTime(delayTime);
		m_EmitterData.m_delayTime = delayTime;
	}
	float EmitterComponent::getEmitterLifeTime()
	{
		ReturnObjNULL 0.0f;

		return m_pEmitterObj->GetLifeTime();
	}
	void EmitterComponent::setEmitterLifeTime(float lifeTime)
	{
		ReturnObjNULL;

		if (lifeTime <= 0)
			return;
		m_EmitterData.m_LifeTime = lifeTime;
		m_pEmitterObj->SetLifeTime(lifeTime);
		m_pEmitterObj->EnterReset();
	}


	float EmitterComponent::getEmitterEmitTime()
	{
		ReturnObjNULL 0.0f;

		return m_pEmitterObj->GetEmitLife();
	}

	void EmitterComponent::setEmitterEmitTime(float emitTime)
	{
		ReturnObjNULL;

		if (emitTime <= 0)
			return;
		m_EmitterData.m_EmitLife = emitTime;
		m_pEmitterObj->SetEmitLife(emitTime);
	}

	float EmitterComponent::getEmitterPreSecond()
	{
		ReturnObjNULL 0.0f;

		return m_EmitterData.mEmissionPreSecond;
	}
	void EmitterComponent::setEmitterPreSecond(float speed)
	{
		ReturnObjNULL;

		m_EmitterData.mEmissionPreSecond = speed;
	}
	bool EmitterComponent::getEmitterIsWorldSpace()
	{
		ReturnObjNULL false;

		return m_EmitterData.mbWorldSpace;
	}
	void EmitterComponent::setEmitterIsWorldSpace(bool isWorldSpace)
	{
		ReturnObjNULL;

		m_EmitterData.mbWorldSpace = isWorldSpace;
	}
	bool EmitterComponent::getEmitterPreHeat()
	{
		ReturnObjNULL false;

		return m_pEmitterObj->GetEnablePreHeat();
	}
	void EmitterComponent::setEmitterPreHeat(bool isPreHeat)
	{
		ReturnObjNULL;

		m_EmitterData.m_bEnablePreHeat = isPreHeat;
		m_pEmitterObj->SetEnablePreHeat(isPreHeat);
	}
	float EmitterComponent::getEmitterPreHeatTime()
	{
		ReturnObjNULL(0.0f);

		return m_pEmitterObj->GetPreHeatSeconds();
	}
	void EmitterComponent::setEmitterPreHeatTime(float preheatTime)
	{
		ReturnObjNULL;

		m_EmitterData.m_preHeatSeconds = preheatTime;
		m_pEmitterObj->SetPreHeatSeconds(preheatTime);
	}

	void EmitterComponent::setLiveForeverOrLoop(const int& val)
	{
		ReturnObjNULL;

		bool bForeverFlag = false;
		bool bLoopFlag = false;
		bool pauseFrame = false; //在某一帧暂停
		switch (val)
		{
		case 0:
			//All false
			break;
		case 1:
			bForeverFlag = true;
			bLoopFlag = false;
			pauseFrame = false;
			break;
		case 2:
			bForeverFlag = false;
			bLoopFlag = true;
			pauseFrame = false;
			break;
		case 3:
			bForeverFlag = false;
			bLoopFlag = false;
			pauseFrame = true;
			break;
		default:
			break;
		}
		m_EmitterData.m_LiveForever = bForeverFlag;
		m_pEmitterObj->SetLiveForever(bForeverFlag);

		m_EmitterData.m_Loop = bLoopFlag;
		m_pEmitterObj->SetLoop(bLoopFlag);

		m_EmitterData.m_LifeMode = val;
		m_pEmitterObj->SetLifeMode(val);
		
	}

	int EmitterComponent::getLiveForeverOrLoop()
	{
		ReturnObjNULL 0;
		return m_EmitterData.m_LifeMode;
	}



	bool EmitterComponent::getIsActorRandomDirection()
	{
		ReturnObjNULL false;

		return m_EmitterData.mbRandomDirection;
	}
	void EmitterComponent::setIsActorRandomDirection(bool isRandom)
	{
		ReturnObjNULL;

		m_EmitterData.mbRandomDirection = isRandom;
	}
	bool EmitterComponent::getIsFixedLifeTime()
	{
		ReturnObjNULL false;

		return m_EmitterData.mbFixedTTL;
	}
	void EmitterComponent::setIsFixedLifeTime(bool isFixed)
	{
		ReturnObjNULL;

		m_EmitterData.mbFixedTTL = isFixed;
	}
	float EmitterComponent::getActorLifeTimeMin()
	{
		ReturnObjNULL 0.0f;

		return m_EmitterData.mTTLMin;
	}
	void EmitterComponent::setActorLifeTimeMin(float lifeTime)
	{
		ReturnObjNULL;

		m_EmitterData.mTTLMin = lifeTime;
	}
	float EmitterComponent::getActorLifeTimeMax()
	{
		ReturnObjNULL 0.0f;

		return m_EmitterData.mTTLMax;
	}
	void EmitterComponent::setActorLifeTimeMax(float lifeTime)
	{
		ReturnObjNULL;

		m_EmitterData.mTTLMax = lifeTime;
	}
	bool EmitterComponent::getIsFixedSpeed()
	{
		ReturnObjNULL false;

		return m_EmitterData.mbFixedSpeed;
	}
	void EmitterComponent::setIsFixedSpeed(bool isFixed)
	{
		ReturnObjNULL;

		m_EmitterData.mbFixedSpeed = isFixed;
	}
	float EmitterComponent::getActorSpeedMin()
	{
		ReturnObjNULL 0.0f;

		return m_EmitterData.mParticleSpeedMin;
	}
	void EmitterComponent::setActorSpeedMin(float speed)
	{
		ReturnObjNULL;

		m_EmitterData.mParticleSpeedMin = speed;
	}
	float EmitterComponent::getActorSpeedMax()
	{
		ReturnObjNULL 0.0f;

		return m_EmitterData.mParticleSpeedMax;
	}
	void EmitterComponent::setActorSpeedMax(float speed)
	{
		ReturnObjNULL;

		m_EmitterData.mParticleSpeedMax = speed;
	}
	bool EmitterComponent::getIsFixedQuat()
	{
		ReturnObjNULL false;

		return m_EmitterData.mbFixedInitialRotation;
	}
	void EmitterComponent::setIsFixedQuat(bool isFixed)
	{
		ReturnObjNULL;

		m_EmitterData.mbFixedInitialRotation = isFixed;
	}
	Vector3 EmitterComponent::getActorQuatMin()
	{
		ReturnObjNULL Vector3::ZERO;

		return m_EmitterData.mInitialRotation;
	}
	void EmitterComponent::setActorQuatMin(Vector3 Quat)
	{
		ReturnObjNULL;

		m_EmitterData.mInitialRotation = Quat;
	}
	Vector3 EmitterComponent::getActorQuatMax()
	{
		ReturnObjNULL Vector3::ZERO;

		return m_EmitterData.mInitialRotation2;
	}
	void EmitterComponent::setActorQuatMax(Vector3 Quat)
	{
		ReturnObjNULL;

		m_EmitterData.mInitialRotation2 = Quat;
	}
	bool EmitterComponent::getIsFixedScale()
	{
		ReturnObjNULL false;

		return m_EmitterData.mbFixedSize;
	}
	void EmitterComponent::setIsFixedScale(bool isFixed)
	{
		ReturnObjNULL;

		m_EmitterData.mbFixedSize = isFixed;
	}
	Vector3 EmitterComponent::getActorScaleMin()
	{
		ReturnObjNULL Vector3::ZERO;

		return m_EmitterData.mSizeMin;
	}
	void EmitterComponent::setActorScaleMin(Vector3 Scale)
	{
		ReturnObjNULL;

		m_EmitterData.mSizeMin = Scale;
	}
	Vector3 EmitterComponent::getActorScaleMax()
	{
		ReturnObjNULL Vector3::ZERO;

		return m_EmitterData.mSizeMax;
	}
	void EmitterComponent::setActorScaleMax(Vector3 Scale)
	{
		ReturnObjNULL;

		m_EmitterData.mSizeMax = Scale;
	}
	void EmitterComponent::UpdateActorEmitterFile(const Echo::Name& actFile, bool isSync /*= false*/)
	{
		if (m_actorFile == actFile)
			return;
		m_actorFile = actFile;
		if (m_pEmitterObj)
		{
			unsigned int count = m_pEmitterObj->m_PoolSize;
			EffectParticle*& actorsVec = m_pEmitterObj->m_ParticlesPool[0];

			for (unsigned int i = 0; i < count; i++)
			{
				EffectParticle* pact = &actorsVec[i];
				ActorEmitterVisualData* pVisualData = static_cast<ActorEmitterVisualData*>(pact->mpVisualData);
				if (pVisualData != nullptr)
				{
					pVisualData->init(m_actorFile);
				}
			}
		}
	}
	void _EchoComponentExport WriteJSNode(cJSON* jsonNode, const char* Name, const EffectEmitterData& data)
	{
		if (!jsonNode)
			return;

		cJSON* EmitterData = cJSON_CreateObject();
		cJSON_AddItemToObject(jsonNode, Name, EmitterData);
		//cJSON* EmitterData = cJSON_CreateObject();
		//cJSON_AddItemToArray(emitter, EmitterData);

		WriteJSNode(EmitterData, "EffectEmitterType", (int)data.mEmitterType);
		WriteJSNode(EmitterData, "LifeTime", data.m_LifeTime);
		WriteJSNode(EmitterData, "EmitLife", data.m_EmitLife);
		WriteJSNode(EmitterData, "delayTime", data.m_delayTime);
		WriteJSNode(EmitterData, "LiveForever", data.m_LiveForever);
		WriteJSNode(EmitterData, "ActorLiveForever", data.m_ParticleLiveForever);
		WriteJSNode(EmitterData, "ActorNum", data.m_ParticleNum);
		WriteJSNode(EmitterData, "EmissionPreSecond", data.mEmissionPreSecond);
		WriteJSNode(EmitterData, "PreHeat", data.m_bEnablePreHeat);
		WriteJSNode(EmitterData, "PreHeatSeconds", data.m_preHeatSeconds);
		WriteJSNode(EmitterData, "WorldSpace", data.mbWorldSpace);
		WriteJSNode(EmitterData, "RandomDirection", data.mbRandomDirection);

		WriteJSNode(EmitterData, "FixedActorSpeed", data.mbFixedSpeed);
		WriteJSNode(EmitterData, "ActorSpeedMin", data.mParticleSpeedMin);
		WriteJSNode(EmitterData, "ActorSpeedMax", data.mParticleSpeedMax);
		
		WriteJSNode(EmitterData, "FixedActorLifeTime", data.mbFixedTTL);
		WriteJSNode(EmitterData, "ActorLifeTimeMin", data.mTTLMin);
		WriteJSNode(EmitterData, "ActorLifeTimeMax", data.mTTLMax);
		
		WriteJSNode(EmitterData, "FixedActorRotation", data.mbFixedInitialRotation);
		WriteJSNode(EmitterData, "ActorRotation1", data.mInitialRotation);
		WriteJSNode(EmitterData, "ActorRotation2", data.mInitialRotation2);
		
		WriteJSNode(EmitterData, "FixedActorScale", data.mbFixedSize);
		WriteJSNode(EmitterData, "ActorScale1", data.mSizeMin);
		WriteJSNode(EmitterData, "ActorScale2", data.mSizeMax);
	}
	bool _EchoComponentExport ReadJSNode(cJSON* jsonNode, const char* Name, EffectEmitterData& data)
	{
		if (!jsonNode)
			return false;

		cJSON* EmitterData = cJSON_GetObjectItem(jsonNode, Name);
		if (!EmitterData)
			return false;
		int type;
		ReadJSNode(EmitterData, "EffectEmitterType", type);
		data.mEmitterType = (EffectEmitterType)type;
		ReadJSNode(EmitterData, "LifeTime", data.m_LifeTime);
		ReadJSNode(EmitterData, "EmitLife", data.m_EmitLife);
		ReadJSNode(EmitterData, "delayTime", data.m_delayTime);
		ReadJSNode(EmitterData, "LiveForever", data.m_LiveForever);
		ReadJSNode(EmitterData, "ActorLiveForever", data.m_ParticleLiveForever);
		ReadJSNode(EmitterData, "ActorNum", data.m_ParticleNum);
		ReadJSNode(EmitterData, "EmissionPreSecond", data.mEmissionPreSecond);
		ReadJSNode(EmitterData, "PreHeat", data.m_bEnablePreHeat);
		ReadJSNode(EmitterData, "PreHeatSeconds", data.m_preHeatSeconds);
		ReadJSNode(EmitterData, "WorldSpace", data.mbWorldSpace);
		ReadJSNode(EmitterData, "RandomDirection", data.mbRandomDirection);

		ReadJSNode(EmitterData, "FixedActorSpeed", data.mbFixedSpeed);
		ReadJSNode(EmitterData, "ActorSpeedMin", data.mParticleSpeedMin);
		ReadJSNode(EmitterData, "ActorSpeedMax", data.mParticleSpeedMax);

		ReadJSNode(EmitterData, "FixedActorLifeTime", data.mbFixedTTL);
		ReadJSNode(EmitterData, "ActorLifeTimeMin", data.mTTLMin);
		ReadJSNode(EmitterData, "ActorLifeTimeMax", data.mTTLMax);

		ReadJSNode(EmitterData, "FixedActorRotation", data.mbFixedInitialRotation);
		ReadJSNode(EmitterData, "ActorRotation1", data.mInitialRotation);
		ReadJSNode(EmitterData, "ActorRotation2", data.mInitialRotation2);

		ReadJSNode(EmitterData, "FixedActorScale", data.mbFixedSize);
		ReadJSNode(EmitterData, "ActorScale1", data.mSizeMin);
		ReadJSNode(EmitterData, "ActorScale2", data.mSizeMax);
		return true;
	}
	ActorEmitterVisualData::ActorEmitterVisualData(EmitterComponent* com)
	{
		if (com)
			m_EmitterCom = com->weak_from_this();
	}
	ActorEmitterVisualData::~ActorEmitterVisualData()
	{
		ActorPtr mainActor = ActorSystem::instance()->getMainActorManager()->mMainActor;
			if (!mainActor.expired())
				mainActor->removeActorLoadListener(this);
			if (m_actor.expired())
				return;
		ActorSystem::instance()->getMainActorManager()->deleteActor(m_actor);
		ActorSystem::instance()->getMainActorManager()->deleteActorInfo(m_actor->getActorInfo());
		}
	void ActorEmitterVisualData::reset()
	{
		if (m_actor)
			m_actor->setVisiable(false);
	}
	bool ActorEmitterVisualData::OnActorCreateFinish()
	{
		if (!m_actor)
		{
			m_actorPath.clear();
			return false;
		}

		ActorPtr mainActor = ActorSystem::instance()->getMainActorManager()->mMainActor;
		if (mainActor == nullptr)
		{
			ActorSystem::instance()->getMainActorManager()->createMainActor();
			mainActor = ActorSystem::instance()->getMainActorManager()->mMainActor;
		}
		mainActor->addActor(mainActor->getRootNodeCom(), m_actor, false);

		m_actor->setVisiable(false);
		m_actor->getRootNodeCom()->m_pActComInfo->SetVisible(false);
		return true;
	}
	void ActorEmitterVisualData::init(const Echo::Name& actfile)
	{
		if (m_actorPath == actfile && m_EmitterCom.expired())
			return;
		
		m_actorPath = actfile;
		if (m_actor != nullptr)
		{
			ActorSystem::instance()->getMainActorManager()->deleteActor(m_actor);
			ActorSystem::instance()->getMainActorManager()->deleteActorInfo(m_actor->getActorInfo());
			m_actor = nullptr;
		}
		if (actfile.empty())
			return;
		bool isSync = m_EmitterCom->getActor()->isSyncLoadActor();
		m_actor = ActorSystem::instance()->getMainActorManager()->createActor(m_actorPath, "", isSync, false);

		if(isSync)
			{
				if (!m_actor)
				{
					m_actorPath.clear();
					return;
				}
			ActorPtr mainActor = ActorSystem::instance()->getMainActorManager()->mMainActor;
				if (mainActor == nullptr)
				{
				ActorSystem::instance()->getMainActorManager()->createMainActor();
				mainActor = ActorSystem::instance()->getMainActorManager()->mMainActor;
				}
				mainActor->addActor(mainActor->getRootNodeCom(), m_actor, false);

			m_actor->setVisiable(false); 
				m_actor->getRootNodeCom()->m_pActComInfo->SetVisible(false);
			}
			else
			{
				m_actor->addActorLoadListener(this);
			}
		}
}
