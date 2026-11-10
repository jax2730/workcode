//-----------------------------------------------------------------------------
// File:  EchoParticleSystemComponent.cpp
//
// Author: LangGuangyue
//
// Date:   2022-8-24
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoActorProFuncImp.h"
#include "EchoParticleSystemComponent.h"
#include "EchoResourceFolderManager.h"
#include "EchoAnimationTimelineComponent.h"
#include "EchoActorRoleComponent.h"
namespace Echo {

	DECLAREACTORCOMTYPE_CPP(ParticleSystemComponent)

	// ---------------------------------------- 粒子系统组件初始化类 ---------------------------------------
	EDITOR_REFLECT_BEGIN(ParticleSystemComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_path, "path", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_color, "color", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_playspeed, "playspeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_lifetime, "lifetime", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_is_forever, "is_forever", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_is_loop, "is_loop", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	ParticleSystemComponentInfo::ParticleSystemComponentInfo(const Name& type, ActorCOMMoveableType movable) 
		:ActorComponentInfo(type, movable)
	{
	}

	ParticleSystemComponentInfo::~ParticleSystemComponentInfo()
	{
	}

	void ParticleSystemComponentInfo::ExportData(cJSON * jsonNode)
	{
		cJSON* particle_emitter = cJSON_CreateObject();
		cJSON_AddItemToObject(jsonNode, "Particle Emitters", particle_emitter);
		reflect::TypeResolver<ParticleSystemComponentInfo>::get()->ExportJson(this, particle_emitter, "");
	}

	void ParticleSystemComponentInfo::ImportData(cJSON * jsonNode)
	{
		cJSON* particle_emitter = cJSON_GetObjectItem(jsonNode, "Particle Emitters");
		if (particle_emitter)
			reflect::TypeResolver<ParticleSystemComponentInfo>::get()->ImportJson(this, particle_emitter, "");
	}

	void ParticleSystemComponentInfo::CopyTo(ActorComponentInfo * p_actor_com_info)
	{
		ParticleSystemComponentInfo* particleComInfo = static_cast<ParticleSystemComponentInfo*>(p_actor_com_info);
		reflect::TypeResolver<ParticleSystemComponentInfo>::get()->copyTo(this, particleComInfo);
	}

	void ParticleSystemComponentInfo::Overlay(ActorComponentInfo * comInfo)
	{
	}

	void ParticleSystemComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			ParticleSystemComponentInfo* oriInfo = static_cast<ParticleSystemComponentInfo*> (mOriginComInfo);
			reflect::TypeResolver<ParticleSystemComponentInfo>::get()->overWrite(oriInfo, this);
		}
	}

	ParticleSystemComponent::ParticleSystemComponent(ActorPtr  pActor, ActorCOMMoveableType movableType) 
		: NodeActorComponent(pActor, movableType)
	{
	}


	ParticleSystemComponent::~ParticleSystemComponent()
	{
	}

	void ParticleSystemComponent::init(ActorComponentInfo * info)
	{
		NodeActorComponent::init(info);
	}

	void ParticleSystemComponent::unInit()
	{
		NodeActorComponent::unInit();
		if (mParticleHolder == nullptr)
			return;
		
		delete mParticleHolder;
		mParticleHolder = nullptr;
	}

	void ParticleSystemComponent::bindNode()
	{
		if (m_pParentNodeCom == nullptr)
			return;
		if (mParticleHolder != NULL && mParticleHolder->getSceneNode()->getParent() == nullptr)
		{
			addChildToNode(mParticleHolder->getSceneNode());
			UpdateComRootNode();
		}
	}

	void ParticleSystemComponent::setActor(const ActorPtr&  pActor)
	{
		if (pActor == m_pActor)
			return;
		m_pActor = pActor;
		if (m_pActor == nullptr)
			return;
	}

	void ParticleSystemComponent::connectCom()
	{
		if (m_pActComInfo == nullptr)
			return;
	}

	void ParticleSystemComponent::initRenderObject(ActorComponentInfo* info)
	{
		ParticleSystemComponentInfo* mInfo = static_cast<ParticleSystemComponentInfo*>(m_pActComInfo);

		if (mInfo != nullptr && !mInfo->m_path.empty())
		{
			LoadParticle(mInfo->m_path, getActor()->isSyncLoadActor(), getSceneManager(), -9999.0f, false, false, true);
			UseInfoParams();
		}

		//过场动画相关
		AnimationTimelineComponent::registerTickComByActor(getActor(), this->weak_from_this());
	}

	Name particleSystemColorName_ = Name("ParticleSystemColor");
	Name particleSystemLifeTimeName_ = Name("ParticleSystemLifeTime");
	Name particleEmitterName_ = Name("ParticleEmitter");

	void ParticleSystemComponent::setTimelineAttribute(std::vector<float>& data, const Name& name)
	{
		NodeActorComponent::setTimelineAttribute(data, name);

		if (name == particleSystemColorName_)
		{
			SetColor(Vector4(data[0],data[1],data[2],data[3]));
		}
		else if (name == particleSystemLifeTimeName_)
		{
			SetLifeTime(data[0]);
		}
		else if (name == particleEmitterName_)
		{
			if (data[0] == -1)
				StopEmitterParticle();
			else
				StartEmitterParticle();
		}
		if(mParticleHolder)
		   mParticleHolder->setPlaySpeed(mPlaySpeed * getTLPlaySpeed());
	}

	void ParticleSystemComponent::visibleStateChanged(bool visibleState)
	{
		if (mParticleHolder)
		{
			if (ActorSystem::instance()->isHideRenderObject())
			{
				mParticleHolder->setVisible(false);
				return;
			}
			mParticleHolder->setVisible(visibleState);
		}
	}

	void ParticleSystemComponent::updatePlaySpeed()
	{
		if (mParticleHolder)
			mParticleHolder->setPlaySpeed(mPlaySpeed * getTLPlaySpeed());
	}

#ifdef ECHO_EDITOR
	bool ParticleSystemComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		if (m_path.empty() == false)
		{
			resMap.insert(std::make_pair(ResourceType::Particle, m_path));
			return true;
		}

		return false;
	}
#endif

	void ParticleSystemComponent::LoadParticle(const std::string& path, bool synchronous, SceneManager* Creator, float priority, bool UseCache, bool bIsPrior,\
		bool bCreateByScene, EffectVanishListen* inParticleVanishListen,bool bCreateByAnimation, float lifeTime, uint8 particleType)
	{
		EffectHolder::CreateInfo create_info;
		//create_info.set_client_log_message(desc.clientLogMessage);
		create_info.set_create_priority(priority);
		create_info.set_is_create_by_animation(bCreateByAnimation);
		create_info.set_is_enable_create_priority(bIsPrior == false && bCreateByScene == false);
		create_info.set_is_synch(synchronous);
		create_info.set_is_use_cache(UseCache);
		create_info.set_life_time(lifeTime);
		//create_info.set_particle_name(desc.particleName);
		create_info.set_particle_vanish_listen(inParticleVanishListen);
		create_info.set_resource_name(Name(path));
		create_info.set_particletype(particleType);
		if (Creator == nullptr)
		{
			Creator = getSceneManager();
		}
		create_info.set_scene_manager(Creator);
		LoadParticle(create_info);
	}

	void ParticleSystemComponent::LoadParticle(const ParticleHolderCreatorDesc& desc)
	{
		EffectHolder::CreateInfo create_info;
		create_info.set_client_log_message(desc.clientLogMessage);
		create_info.set_create_priority(desc.priority);
		create_info.set_is_create_by_animation(desc.bCreateByAnimation);
		create_info.set_is_enable_create_priority(desc.bIsPrior == false && desc.bCreateByScene == false);
		create_info.set_is_synch(desc.synch);
		create_info.set_is_use_cache(desc.UseCache);
		create_info.set_life_time(desc.lifeTime);
		create_info.set_particle_name(desc.particleName);
		create_info.set_particle_vanish_listen(desc.inParticleVanishListen);
		create_info.set_resource_name(desc.resName);
		create_info.set_particletype(desc.particleType);
		if (desc.Creator == nullptr)
		{
			const_cast<ParticleHolderCreatorDesc&>(desc).Creator = getSceneManager();
		}
		create_info.set_scene_manager(desc.Creator);
		create_info.set_particle_init_async(desc.is_init_async);
		LoadParticle(create_info);
	}

	void ParticleSystemComponent::LoadParticle(const EffectHolder::CreateInfo& create_info)
	{
		if (mParticleHolder != nullptr)
			DelParticle();

		const_cast<EffectHolder::CreateInfo&>(create_info).set_particle_delete_listen(this);
		mParticleHolder = new EffectHolder(create_info);

		assert(mParticleHolder != nullptr);

		mParticleHolder->setCameraCullMode(DistanceCull);
		if (ActorSystem::instance()->isHideRenderObject())
		{
			mParticleHolder->setVisible(false);
		}
		else
		{
			mParticleHolder->setVisible(getRealVisibleState());
		}
		mParticleHolder->setParticlePlayTime(mParticleHolder->getParticleTotalLifeTime());

		if (m_pParentNodeCom == nullptr)
			return;
		if (mParticleHolder != NULL)
		{
			addChildToNode(mParticleHolder->getSceneNode());
			UpdateComRootNode(true, true);
		}
		PlayParticle(false);
	}

	Name ParticleSystemComponent::GetParticleName()
	{
		if (!mParticleHolder)
			return Name();
		else
			return mParticleHolder->getResName();
	}

	Name ParticleSystemComponent::GetResourceName() const
	{
		if (!mParticleHolder)
			return Name();
		else
			return mParticleHolder->getResName();
	}

	bool ParticleSystemComponent::GetPause() const
	{
		if (mParticleHolder == NULL)
			return false;
		return mParticleHolder->getPause();
	}

	void ParticleSystemComponent::SetPause(bool is_pause)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->setPause(is_pause);
	}

	void ParticleSystemComponent::PlayParticle(bool ispause)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->setPause(ispause);
	}

	void ParticleSystemComponent::ResetParticle()
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->Reset();
	}

	float ParticleSystemComponent::GetLifeTime()
	{
		if (mParticleHolder == NULL)
			return 0.0f;
		return mParticleHolder->GetLifeTime();
	}

	void ParticleSystemComponent::SetLifeTime(float val)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->SetLifeTime(val);
	}

	float ParticleSystemComponent::GetPlaySpeed()
	{
		if (mParticleHolder == NULL)
			return 0.0f;
		return mParticleHolder->getPlaySpeed();
	}

	void ParticleSystemComponent::SetPalySpeed(float speed)
	{
		mPlaySpeed = speed;
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->setPlaySpeed(speed);
	}

	void ParticleSystemComponent::SetPalyTime(float playtime)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->setParticleStartTime(playtime);
	}

	float ParticleSystemComponent::GetPlayTime()
	{
		if (mParticleHolder == NULL)
			return 0.0f;
		return mParticleHolder->getParticlePresentTime();
	}

	bool ParticleSystemComponent::GetLoop()
	{
		if (mParticleHolder == NULL)
			return false;
		return mParticleHolder->getLoop();
	}

	void ParticleSystemComponent::SetLoop(bool isLoop)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->setLoop(isLoop);
	}

	void ParticleSystemComponent::SetLiveForever(bool inLiveForever)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->SetLiveForever(inLiveForever);
	}

	bool ParticleSystemComponent::GetLiveForever() const
	{
		if (mParticleHolder == NULL)
			return false;
		return mParticleHolder->GetLiveForever();
	}

	void ParticleSystemComponent::SetChainBeginEnd(const DVector3& begin, const DVector3& end)
	{
		if (mParticleHolder)
		{
			mParticleHolder->SetChainBeginEnd(begin, end);
		}
	}

	void ParticleSystemComponent::SetHermitCurveTrail(std::vector<DVector3>* vertices)
	{
		if (mParticleHolder)
		{
			mParticleHolder->SetHermitCurveTrail(vertices);
		}
	}

	void ParticleSystemComponent::SetInputRoleObject(RoleObject* mRoleObject)
	{
		if (mParticleHolder)
		{
			mParticleHolder->setInputRoleObject(mRoleObject);
		}
	}

	void ParticleSystemComponent::UpdateInputRoleWithParent()
	{
		if (getActor())
		{
			std::vector<ActorComponentPtr>	roleComponents = getActor()->getComponentsByType<RoleComponent>();
			for (int i = 0; i < (int)roleComponents.size(); i++)
			{
				if (!roleComponents[i].expired())
				{
					RoleComponent* roleComponent = static_cast<RoleComponent*>(roleComponents[i].getPointer());
					this->SetInputRoleObject(roleComponent->GetRole());
				}
			}
		}
	}

	void ParticleSystemComponent::UseInfoParams()
	{
		ParticleSystemComponentInfo* particleComInfo = static_cast<ParticleSystemComponentInfo*>(this->m_pActComInfo);
		if (particleComInfo)
		{
			if (mParticleHolder != nullptr)
			{
				mParticleHolder->SetLifeTime(particleComInfo->m_lifetime);
				mParticleHolder->SetLiveForever(particleComInfo->m_is_forever);
				mParticleHolder->SetColor(ColorValue(particleComInfo->m_color.x, particleComInfo->m_color.y, particleComInfo->m_color.z, particleComInfo->m_color.w));
				mParticleHolder->setLoop(particleComInfo->m_is_loop);
				mParticleHolder->setPlaySpeed(particleComInfo->m_playspeed);
			}
		}
	}

	void ParticleSystemComponent::UsePEFDataUpdateInfoParams(ParticleSystemComponentInfo* ParticleInfo)
	{
		ParticleSystemComponentInfo* particleComInfo = static_cast<ParticleSystemComponentInfo*>(ParticleInfo);
		if (particleComInfo)
		{
			if (mParticleHolder != nullptr)
			{
				particleComInfo->m_lifetime = mParticleHolder->GetLifeTime();
				particleComInfo->m_is_forever = mParticleHolder->GetLiveForever();
				const ColorValue& cl = mParticleHolder->GetColor();
				particleComInfo->m_color = { cl.r, cl.g, cl.b, cl.a };
				particleComInfo->m_is_loop = mParticleHolder->getLoop();
				particleComInfo->m_playspeed = mParticleHolder->getPlaySpeed();
			}
		}
	}

	void ParticleSystemComponent::LoopParticle(bool isLoop)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->setLoop(isLoop);
	}

	void ParticleSystemComponent::DelParticle()
	{
		if (mParticleHolder == nullptr)
			return;

		delete mParticleHolder;
		mParticleHolder = nullptr;
	}

	bool ParticleSystemComponent::IsVanished()
	{
		if (!mParticleHolder)
			return false;
		return mParticleHolder->IsVanished();
	}

	bool ParticleSystemComponent::isEffectCreateFinish()
	{
		if (mParticleHolder == NULL)
			return false;
		return mParticleHolder->isCreateFinish();
	}

	void ParticleSystemComponent::SetCameraCullMode(uint8_t cullMode)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->setCameraCullMode(CameraCullMode(cullMode));
	}

	void ParticleSystemComponent::SetRenderingDistance(float renderDis)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->setRenderingDistance(renderDis);
	}

	void ParticleSystemComponent::SetVanishParticle()
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->VanishParticle();
	}

	void ParticleSystemComponent::SetParticlePriority(float val)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->SetParticlePriority(val);
	}

	void ParticleSystemComponent::ChangeDiffuseTexture(const char * path, const char * name)
	{		
		if (mParticleHolder == NULL)
			return;
		std::string strPath(path);
		std::string strName(name);
		mParticleHolder->ChangeDiffuseTexture(strPath.c_str(), strName.c_str());
	}

	void ParticleSystemComponent::PlayEmitterParticle(bool _isStop)
	{
		if (!_isStop)
			StartEmitterParticle();
		else
			StopEmitterParticle();
		//isStop = _isStop;
	}

	void ParticleSystemComponent::StopEmitterParticle()
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->StopEmitterParticle();
	}

	void ParticleSystemComponent::StartEmitterParticle()
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->StartEmitterParticle();
	}

	//bool ParticleSystemComponent::GetEmitterSetting()
	//{
	//	if (mParticleHolder == NULL)
	//		return false;
	//	return isStop;
	//}

	void ParticleSystemComponent::EnableUIRender(bool val)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->enableUIRender(val);
	}

	void ParticleSystemComponent::SetFollowPostion(bool isfollow)
	{
		if (mParticleHolder == NULL)
			return;
		if (getSceneNode())
		{
			getSceneNode()->setInheritPosition(isfollow);
			return;
		}
		mParticleHolder->setFollowPostion(isfollow);
	}

	void ParticleSystemComponent::SetFollowOrientation(bool isfollow)
	{
		if (mParticleHolder == NULL)
			return;
		if (getSceneNode())
		{
			getSceneNode()->setInheritOrientation(isfollow);
			return;
		}
		mParticleHolder->setFollowOrientation(isfollow);
	}

	void ParticleSystemComponent::SetFollowScale(bool isfollow)
	{
		if (mParticleHolder == NULL)
			return;
		if (getSceneNode())
		{
			getSceneNode()->setInheritScale(isfollow);
			return;
		}
		mParticleHolder->setFollowScale(isfollow);
	}

	void ParticleSystemComponent::SetManualPosition(bool isManual)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->setManualPosition(isManual);
	}

	void ParticleSystemComponent::SetManualOrientation(bool isManual)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->setManualOrientation(isManual);
	}
	void ParticleSystemComponent::SetManualScale(bool isManual)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->setManualScale(isManual);
	}

	Vector4 ParticleSystemComponent::GetColor()
	{
		if (mParticleHolder == NULL)
			return Vector4(0.0f,0.0f,0.0f,1.0f);

		Vector4 _color = Vector4(0, 0, 0, 1);
		ColorValue p_color = mParticleHolder->GetColor();
		_color.x = p_color.r;
		_color.y = p_color.g;
		_color.z = p_color.b;
		_color.w = p_color.a;

		return _color;
	}

	void ParticleSystemComponent::SetColor(const Vector4 & _color)
	{
		if (mParticleHolder == NULL)
			return;
		ColorValue mColor;
		mColor.r = _color.x;
		mColor.g = _color.y;
		mColor.b = _color.z;
		mColor.a = _color.w;
		mParticleHolder->SetColor(mColor);
	}

	void ParticleSystemComponent::SetColor_RGB(float R, float G, float B, float A)
	{
		if (mParticleHolder == NULL)
			return;
		ColorValue mColor;
		mColor.r = R;
		mColor.g = G;
		mColor.b = B;
		mColor.a = A;
		mParticleHolder->SetColor(mColor);
	}

	void ParticleSystemComponent::SetIllusionAlpha(float val)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->SetIllusionAlpha(val);
	}

	void ParticleSystemComponent::SetBillboardSpinRot(float val)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->setBillboardSpinRot(val);
	}

	bool ParticleSystemComponent::IsVanished() const
	{
		if (mParticleHolder == NULL)
			return false;
		return mParticleHolder->IsVanished();
	}

	void ParticleSystemComponent::VanishParticle()
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->VanishParticle();
	}

	void ParticleSystemComponent::SetChainBeginEnd(const Vector3 & _vBegin, const Vector3 & _vEnd)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->SetChainBeginEnd(_vBegin, _vEnd);
	}


	void ParticleSystemComponent::addCollisionParam(int index, const Vector3& mColliderPosition, float mColliderRadius, const Vector3& mColliderVelocity)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->addCollisionParam(index, mColliderPosition, mColliderRadius, mColliderVelocity);
	}

	void ParticleSystemComponent::updateCollisionParam(int index, const Vector3& mColliderPosition, float mColliderRadius, const Vector3& mColliderVelocity)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->updateCollisionParam(index, mColliderPosition, mColliderRadius, mColliderVelocity);
	}

	void ParticleSystemComponent::removeCollisionParam(int index)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->removeCollisionParam(index);
	}


	bool ParticleSystemComponent::idPrepared()
	{
		if (mParticleHolder == NULL)
			return false;
		return mParticleHolder->isPrepared();
	}

	void ParticleSystemComponent::StartPrepareRes()
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->startPrepareRes();
	}

	void ParticleSystemComponent::AddRenderableToRenderQueueGroup(SceneManager* pScene, RenderQueueGroupID id, int32 priority)
	{
		if (mParticleHolder == NULL)
			return;
		RenderQueue* pRenderQueue = pScene->getRenderQueue();
		RenderQueueGroup* pRenderGroup = pRenderQueue->getRenderQeueuGroup(id);
		mParticleHolder->addRenderableToRenderQueueGroup(pRenderGroup, priority);
	}

	void ParticleSystemComponent::SetHermitCurveTrail(std::vector<Vector3>* vertices)
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->SetHermitCurveTrail(vertices);
	}

	void ParticleSystemComponent::NotifyCurrentCamera(SceneManager * pScene)
	{
		if (mParticleHolder == NULL)
			return;
		Camera* pCamera = pScene->getUICamera();
		mParticleHolder->_notifyCurrentCamera(pCamera);
	}

	void ParticleSystemComponent::UpdateVBIB()
	{
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->UpdateVBIB();
	}

	bool ParticleSystemComponent::HaveParticle()
	{
		if (mParticleHolder == NULL)
			return false;
		return true;
	}

	float ParticleSystemComponent::GetParticleDelayDeleteTime() const noexcept
	{
		if (mParticleHolder == nullptr)
		{
			return 0.0f;
		}
		else
		{
			EffectObject* effect_object = mParticleHolder->getParticle();
			return effect_object ? effect_object->GetWaitDestoryTime() : 0.0f;
		}
	}

	bool ParticleSystemComponent::SetEffectObjectIsEnableWaitDestoryTime(bool enable) noexcept
	{
		if (mParticleHolder)
		{
			return mParticleHolder->setParticleObjectIsEnableWaitDestoryTime(enable);
		}

		return false;
	}

	bool ParticleSystemComponent::GetEffectObjectIsEnableWaitDestoryTime() const noexcept
	{
		if (mParticleHolder)
		{
			return mParticleHolder->getParticleObjectIsEnableWaitDestoryTime();
		}

		return false;
	}

	void ParticleSystemComponent::setFPState(Echo::ENUM_FP_STATE val)
	{
		if (mParticleHolder)
		{
			mParticleHolder->setFPState(val);
		}
	}

	std::pair<ParticleSystemComponent::ErrorCode, Vector3> ParticleSystemComponent::getElementWorldPositionByIndex(unsigned index) 
	{
		if (mParticleHolder)
		{
			std::pair<EffectHolder::ErrorCode, Vector3> res = mParticleHolder->getElementWorldPositionByIndex(index);
			switch (res.first)
			{
			case EffectHolder::ErrorCode::NoError:
				return std::pair<ErrorCode, Vector3>(ErrorCode::NoError, res.second);
				break;
			case EffectHolder::ErrorCode::TypeError:
				return std::pair<ErrorCode, Vector3>(ErrorCode::TypeError, Vector3::ZERO);
				break;
			case EffectHolder::ErrorCode::EffectObjectIsNull:
				return std::pair<ErrorCode, Vector3>(ErrorCode::EffectObjectIsNull, Vector3::ZERO);
				break;
			case EffectHolder::ErrorCode::ElementObjectIsNull:
				return std::pair<ErrorCode, Vector3>(ErrorCode::ElementObjectIsNull, Vector3::ZERO);
				break;
			case EffectHolder::ErrorCode::OtherError:
				return std::pair<ErrorCode, Vector3>(ErrorCode::OtherError, Vector3::ZERO);
				break;
			default:
				break;
			}
		}
		return std::pair<ErrorCode, Vector3>(ErrorCode::ParticleHolderIsNull, Vector3::ZERO);
	}

	std::pair<ParticleSystemComponent::ErrorCode, Vector3> ParticleSystemComponent::getElementWorldPositionByName(const std::string& name) 
	{
		if (mParticleHolder)
		{
			std::pair<EffectHolder::ErrorCode, Vector3> res = mParticleHolder->getElementWorldPositionByName(name);
			switch (res.first)
			{
			case EffectHolder::ErrorCode::NoError:
				return std::pair<ErrorCode, Vector3>(ErrorCode::NoError, res.second);
				break;
			case EffectHolder::ErrorCode::TypeError:
				return std::pair<ErrorCode, Vector3>(ErrorCode::TypeError, Vector3::ZERO);
				break;
			case EffectHolder::ErrorCode::EffectObjectIsNull:
				return std::pair<ErrorCode, Vector3>(ErrorCode::EffectObjectIsNull, Vector3::ZERO);
				break;
			case EffectHolder::ErrorCode::ElementObjectIsNull:
				return std::pair<ErrorCode, Vector3>(ErrorCode::ElementObjectIsNull, Vector3::ZERO);
				break;
			case EffectHolder::ErrorCode::OtherError:
				return std::pair<ErrorCode, Vector3>(ErrorCode::OtherError, Vector3::ZERO);
				break;
			default:
				break;
			}
		}
		return std::pair<ErrorCode, Vector3>(ErrorCode::ParticleHolderIsNull, Vector3::ZERO);
	}

	ParticleSystemComponent::ErrorCode ParticleSystemComponent::setRingEmitterRadiusByElementIndex(unsigned index, float val)
	{
		if (mParticleHolder)
		{
			EffectHolder::ErrorCode res = mParticleHolder->setRingEmitterRadiusByElementIndex(index, val);
			switch (res)
			{
			case EffectHolder::ErrorCode::NoError:
				return ErrorCode::NoError;
				break;
			case EffectHolder::ErrorCode::TypeError:
				return ErrorCode::TypeError;
				break;
			case EffectHolder::ErrorCode::EffectObjectIsNull:
				return ErrorCode::EffectObjectIsNull;
				break;
			case EffectHolder::ErrorCode::ElementObjectIsNull:
				return ErrorCode::ElementObjectIsNull;
				break;
			case EffectHolder::ErrorCode::OtherError:
				return ErrorCode::OtherError;
				break;
			default:
				break;
			}
		}
		return ErrorCode::ParticleHolderIsNull;
	}

	ParticleSystemComponent::ErrorCode ParticleSystemComponent::setRingEmitterRadiusByElementName(const std::string& name, float val)
	{
		if (mParticleHolder)
		{
			EffectHolder::ErrorCode res = mParticleHolder->setRingEmitterRadiusByElementName(name, val);
			switch (res)
			{
			case EffectHolder::ErrorCode::NoError:
				return ErrorCode::NoError;
				break;
			case EffectHolder::ErrorCode::TypeError:
				return ErrorCode::TypeError;
				break;
			case EffectHolder::ErrorCode::EffectObjectIsNull:
				return ErrorCode::EffectObjectIsNull;
				break;
			case EffectHolder::ErrorCode::ElementObjectIsNull:
				return ErrorCode::ElementObjectIsNull;
				break;
			case EffectHolder::ErrorCode::OtherError:
				return ErrorCode::OtherError;
				break;
			default:
				break;
			}
		}
		return ErrorCode::ParticleHolderIsNull;
	}

	ParticleSystemComponent::ErrorCode ParticleSystemComponent::setRingEmitterInnerRadiusByElementIndex(unsigned index, float val)
	{
		if (mParticleHolder)
		{
			EffectHolder::ErrorCode res = mParticleHolder->setRingEmitterInnerRadiusByElementIndex(index, val);
			switch (res)
			{
			case EffectHolder::ErrorCode::NoError:
				return ErrorCode::NoError;
				break;
			case EffectHolder::ErrorCode::TypeError:
				return ErrorCode::TypeError;
				break;
			case EffectHolder::ErrorCode::EffectObjectIsNull:
				return ErrorCode::EffectObjectIsNull;
				break;
			case EffectHolder::ErrorCode::ElementObjectIsNull:
				return ErrorCode::ElementObjectIsNull;
				break;
			case EffectHolder::ErrorCode::OtherError:
				return ErrorCode::OtherError;
				break;
			default:
				break;
			}
		}
		return ErrorCode::ParticleHolderIsNull;
	}

	ParticleSystemComponent::ErrorCode ParticleSystemComponent::setRingEmitterInnerRadiusByElementName(const std::string& name, float val)
	{
		if (mParticleHolder)
		{
			EffectHolder::ErrorCode res = mParticleHolder->setRingEmitterInnerRadiusByElementName(name, val);
			switch (res)
			{
			case EffectHolder::ErrorCode::NoError:
				return ErrorCode::NoError;
				break;
			case EffectHolder::ErrorCode::TypeError:
				return ErrorCode::TypeError;
				break;
			case EffectHolder::ErrorCode::EffectObjectIsNull:
				return ErrorCode::EffectObjectIsNull;
				break;
			case EffectHolder::ErrorCode::ElementObjectIsNull:
				return ErrorCode::ElementObjectIsNull;
				break;
			case EffectHolder::ErrorCode::OtherError:
				return ErrorCode::OtherError;
				break;
			default:
				break;
			}
		}
		return ErrorCode::ParticleHolderIsNull;
	}

	ParticleSystemComponent::ErrorCode ParticleSystemComponent::setAllElementRingEmitterRadius(float val)
	{
		if (mParticleHolder)
		{
			EffectHolder::ErrorCode res = mParticleHolder->setAllElementRingEmitterRadius(val);
			switch (res)
			{
			case EffectHolder::ErrorCode::NoError:
				return ErrorCode::NoError;
				break;
			case EffectHolder::ErrorCode::TypeError:
				return ErrorCode::TypeError;
				break;
			case EffectHolder::ErrorCode::EffectObjectIsNull:
				return ErrorCode::EffectObjectIsNull;
				break;
			case EffectHolder::ErrorCode::ElementObjectIsNull:
				return ErrorCode::ElementObjectIsNull;
				break;
			case EffectHolder::ErrorCode::OtherError:
				return ErrorCode::OtherError;
				break;
			default:
				break;
			}
		}
		return ErrorCode::ParticleHolderIsNull;
	}

	ParticleSystemComponent::ErrorCode ParticleSystemComponent::setAllElementRingEmitterInnerRadius(float val)
	{
		if (mParticleHolder)
		{
			EffectHolder::ErrorCode res = mParticleHolder->setAllElementRingEmitterInnerRadius(val);
			switch (res)
			{
			case EffectHolder::ErrorCode::NoError:
				return ErrorCode::NoError;
				break;
			case EffectHolder::ErrorCode::TypeError:
				return ErrorCode::TypeError;
				break;
			case EffectHolder::ErrorCode::EffectObjectIsNull:
				return ErrorCode::EffectObjectIsNull;
				break;
			case EffectHolder::ErrorCode::ElementObjectIsNull:
				return ErrorCode::ElementObjectIsNull;
				break;
			case EffectHolder::ErrorCode::OtherError:
				return ErrorCode::OtherError;
				break;
			default:
				break;
			}
		}
		return ErrorCode::ParticleHolderIsNull;
	}

	void ParticleSystemComponent::UpdateParamValue(float v)
	{
		if (mParticleHolder)
		{
			mParticleHolder->UpdateParamValue(v);
		}
	}

	void ParticleSystemComponent::EditorPlayImp()
	{
		SetPalyTime(0.0f);
		PlayParticle(false);
		PlayEmitterParticle(false);
	}

	void ParticleSystemComponent::onParticleDelete()
	{
		DelParticle();
	}

	void ParticleSystemComponent::EditorPauseImp(bool pause)
	{
		PlayParticle(pause);
	}

	void ParticleSystemComponent::EditorStopImp()
	{
		SetPalyTime(0.0f);
		PlayParticle(true);
		PlayEmitterParticle(true);
	}
}
