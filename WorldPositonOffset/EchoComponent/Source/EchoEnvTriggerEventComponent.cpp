#include "EchoComponentStableHeaders.h"
#include "EchoEnvTriggerEventComponent.h"
#include "EchoEnvTriggerEvent.h"
#include "EchoSceneTriggerData.h"
#include "Effect/EchoEffectHolder.h"
#include "EchoTODComponentManager.h"
#include "EchoWeatherManager.h"
#include "EchoComponentSystem.h"
#include "EchoTODComponentSystem.h"
namespace Echo 
{
	DECLAREACTORCOMTYPE_CPP(EnvTriggerEventComponent);

	EDITOR_REFLECT_BEGIN(EnvTriggerEventComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mUpdateMode, "UpdateMode", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, "EnvTriggerPriority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mYrotateAngle, "YRotateAngle", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIsRaining, "IsRaining", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIsSnowing, "IsSnowing", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTriggerEventData, "TriggerEventData", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSecondTriggerEventData, "SecondTriggerEventData", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSecondYrotateAngle, "SecondYRotateAngle", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSecondUpdateMode, "SecondUpdateMode", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
#ifdef ECHO_EDITOR
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTriggerEventName, "TriggerEventName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSecondTriggerEventName, "SecondTriggerEventName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
#endif
	EDITOR_REFLECT_END();

	EnvTriggerEventComponentInfo::EnvTriggerEventComponentInfo(const Name& comType, ActorCOMMoveableType type)
		:ActorComponentInfo(comType,type)
	{
		mContext.mUpdateMode = Mode_All;
	}

	void EnvTriggerEventComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<EnvTriggerEventComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void EnvTriggerEventComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<EnvTriggerEventComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void EnvTriggerEventComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		EnvTriggerEventComponentInfo* triggerInfo = static_cast<EnvTriggerEventComponentInfo*>(comInfo);
		reflect::TypeResolver<EnvTriggerEventComponentInfo>::get()->copyTo(this, triggerInfo);
	}

	void EnvTriggerEventComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr) {
			reflect::TypeResolver<EnvTriggerEventComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

#ifdef ECHO_EDITOR
	bool EnvTriggerEventComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		bool isNeedCopy = false;
		for (const EnvTriggerEventDesc& iter : mContext.mTriggerEventData)
		{
			if (iter.ObjectName != Name::g_cEmptyName)
			{
				isNeedCopy = true;
				resMap.insert(std::make_pair(ResourceType::Particle,iter.ObjectName.c_str()));
			}
		}
		for (const EnvTriggerEventDesc& iter : mContext.mSecondTriggerEventData)
		{
			if (iter.ObjectName != Name::g_cEmptyName)
			{
				isNeedCopy = true;
				resMap.insert(std::make_pair(ResourceType::Particle, iter.ObjectName.c_str()));
			}
		}
		return isNeedCopy;
	}
#endif

	EnvTriggerEventComponent::EnvTriggerEventComponent(ActorPtr pActor, ActorCOMMoveableType movableType) :
		ActorComponent(pActor, movableType)
	{
	}

	EnvTriggerEventComponent::~EnvTriggerEventComponent()
	{
	}
	void EnvTriggerEventComponent::init(ActorComponentInfo* info)
	{
		TODCOMPONENT_INITMGR(getEnvTriggerContext, m_tMgr, false);

		if (info != nullptr)
		{
			EnvTriggerEventComponentInfo* triggerInfo_ = static_cast<EnvTriggerEventComponentInfo*>(info);
			mContext = triggerInfo_->mContext;
		}
		initEnvTriggerEventData();
		
	}
	void EnvTriggerEventComponent::unInit()
	{
		TODCOMPONENT_UNINITMGR(getEnvTriggerContext, m_tMgr, T_End);
		
		for (auto& iter : mEnvTriggerEventVec)
		{
			if (iter)
				delete iter;
			iter = nullptr;
		}

		for (auto& iter : mSecondEnvTriggerEventVec)
		{
			if (iter)
				delete iter;
			iter = nullptr;
		}
	}
	void EnvTriggerEventComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->pushComponent(m_tMgr->getEnvTriggerContext(), this);
		else
			m_tMgr->popComponent(m_tMgr->getEnvTriggerContext(), this);
	}
	void EnvTriggerEventComponent::use()
	{
		if (m_tMgr)
		{
			WeatherManager* wMgr = m_tMgr->GetWeatherManager();
			if (wMgr)
			{
				wMgr->setRaining(mContext.mIsRaining);
				wMgr->setCoverSnow(mContext.mIsSnowing);
			}
		}

		mIsActiveCom = true;
		_use(mEnvTriggerEventVec, mCurrIdx);
		if (m_tMgr && m_tMgr->getIsUseSecondGroup())
		   _use(mSecondEnvTriggerEventVec,mSecondCurrIdx);
	}

	void EnvTriggerEventComponent::unuse()
	{
		_unuse(mEnvTriggerEventVec);
		_unuse(mSecondEnvTriggerEventVec);
		mIsActiveCom = false;

	}


	void EnvTriggerEventComponent::initEnvTriggerEventData()
	{
		

		_initEnvTriggerEventData(mEnvTriggerEventVec, mContext.mTriggerEventData);


		_initEnvTriggerEventData(mSecondEnvTriggerEventVec, mContext.mSecondTriggerEventData);
		
#ifdef ECHO_EDITOR
		if (mContext.mTriggerEventName.size() < mContext.mTriggerEventData.size())
			mContext.mTriggerEventName.resize(mContext.mTriggerEventData.size());

		if (mContext.mSecondTriggerEventName.size() < mContext.mSecondTriggerEventData.size())
			mContext.mSecondTriggerEventName.resize(mContext.mSecondTriggerEventData.size());
#endif
		
	}

	void EnvTriggerEventComponent::update(float elapsedTime)
	{
		if (!m_tMgr || !m_tMgr->GetSceneManager() )
			return;
		
		//处理位置
		const Vector3& cameraPos = m_tMgr->GetSceneManager()->getMainCamera()->getPosition();
		if (ComponentSystem::instance()->getIsEchoEditor())
			m_tMgr->setRolePosition(cameraPos);
         
		Vector3 updatePos = m_tMgr->getRolePosition();
		if (-5.0f < updatePos.y && updatePos.y < 0.5f)
		{
			updatePos.y = cameraPos.y;
		}
		Camera* mainCamera_ = m_tMgr->GetSceneManager()->getMainCamera();
		if (!mainCamera_)
			return;
		const Quaternion& cameraQua = mainCamera_->getDerivedOrientation();

		int sz = static_cast<int>(mEnvTriggerEventVec.size());
		if (sz != 0)
		{
			_update(elapsedTime, updatePos, cameraPos, cameraQua, mEnvTriggerEventVec, mContext.mUpdateMode, mCurrIdx);
		}

		sz = static_cast<int>(mSecondEnvTriggerEventVec.size());
		if((m_tMgr && m_tMgr->getIsUseSecondGroup()) && sz != 0)
			_update(elapsedTime, updatePos, cameraPos, cameraQua, mSecondEnvTriggerEventVec, mContext.mSecondUpdateMode, mSecondCurrIdx);
	}

	int EnvTriggerEventComponent::_getRandomIdx(const std::vector<EnvTriggerEvent*>& envTriggerEvent_)
	{
		int sz = static_cast<int>(envTriggerEvent_.size());
		if (sz == 0)
			return -1;
		if (sz == 1)
			return envTriggerEvent_[0] == nullptr ? -1 : 0;

		int index = 0;
		index = Math::RangeRandom(0, sz);
        
		for (int i = 0; i < sz; ++i)
		{
			if (envTriggerEvent_[i])
			{
				--index;
				if (index < 0)
				{
					return i;
				}
			}
		}
		return -1;
	}

	void EnvTriggerEventComponent::resetEvent(int idx)
	{
		if (idx < 0  || !m_tMgr)
			return;
		if (!mIsActiveCom)
			return;
		_resetEvent(idx,mEnvTriggerEventVec,mCurrIdx,mContext.mUpdateMode);
	}

	void EnvTriggerEventComponent::resetSecondEvent(int idx)
	{
		if (idx < 0 || !m_tMgr)
			return;
		if (!mIsActiveCom)
			return;
		_resetEvent(idx, mSecondEnvTriggerEventVec, mSecondCurrIdx, mContext.mSecondUpdateMode);
	}
	
	void EnvTriggerEventComponent::addTriggerEvent()
	{
		EnvTriggerEventDesc  eventDesc_;
		mContext.mTriggerEventData.emplace_back(eventDesc_);
		mContext.mYrotateAngle.emplace_back(0.f);
		mEnvTriggerEventVec.emplace_back(nullptr);
#ifdef ECHO_EDITOR
		mContext.mTriggerEventName.emplace_back("");
#endif
	}

	void EnvTriggerEventComponent::addSecondTriggerEvent()
	{
		EnvTriggerEventDesc  eventDesc_;
		mContext.mSecondTriggerEventData.emplace_back(eventDesc_);
		mContext.mSecondYrotateAngle.emplace_back(0.f);
		mSecondEnvTriggerEventVec.emplace_back(nullptr);
#ifdef ECHO_EDITOR
		mContext.mSecondTriggerEventName.emplace_back("");
#endif
	}

	void EnvTriggerEventComponent::deleteTriggerEvent(int idx)
	{
		if (idx < 0 || mContext.mTriggerEventData.size() <= idx)
			return;
		mContext.mTriggerEventData.erase(mContext.mTriggerEventData.begin() + idx);
		if(idx < mContext.mYrotateAngle.size())
		   mContext.mYrotateAngle.erase(mContext.mYrotateAngle.begin() + idx);
#ifdef ECHO_EDITOR
		if(idx < mContext.mTriggerEventName.size())
		   mContext.mTriggerEventName.erase(mContext.mTriggerEventName.begin() + idx);
#endif
		if (mEnvTriggerEventVec[idx] != nullptr)
			delete mEnvTriggerEventVec[idx];
		mEnvTriggerEventVec[idx] = nullptr;
		mEnvTriggerEventVec.erase(mEnvTriggerEventVec.begin() + idx);
	}

	void EnvTriggerEventComponent::deleteSecondTriggerEvent(int idx)
	{
		if (idx < 0 || mContext.mSecondTriggerEventData.size() <= idx)
			return;
		mContext.mSecondTriggerEventData.erase(mContext.mSecondTriggerEventData.begin() + idx);
		if (idx < mContext.mSecondYrotateAngle.size())
			mContext.mSecondYrotateAngle.erase(mContext.mSecondYrotateAngle.begin() + idx);
#ifdef ECHO_EDITOR
		if (idx < mContext.mSecondTriggerEventName.size())
			mContext.mSecondTriggerEventName.erase(mContext.mSecondTriggerEventName.begin() + idx);
#endif
		if (mSecondEnvTriggerEventVec[idx] != nullptr)
			delete mSecondEnvTriggerEventVec[idx];
		mSecondEnvTriggerEventVec[idx] = nullptr;
		mSecondEnvTriggerEventVec.erase(mSecondEnvTriggerEventVec.begin() + idx);
	}

	void EnvTriggerEventComponent::setRainState(bool isRain_)
	{
		mContext.mIsRaining = isRain_;
		if (!m_tMgr)
			return;
		if (!mIsActiveCom)
			return;

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if(wMgr)
			wMgr->setRaining(isRain_);

	}
	void EnvTriggerEventComponent::setSnowState(bool isSnow)
	{
		mContext.mIsSnowing = isSnow;
		if (!m_tMgr)
			return;
		if (!mIsActiveCom)
			return;
		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
			wMgr->setCoverSnow(isSnow);
	}
	void EnvTriggerEventComponent::setTriggerUpdateModel(int mode_)
	{
		mContext.mUpdateMode = mode_;
	}

	void EnvTriggerEventComponent::_setTriggerEventDataByIdx(const EnvTriggerEventDesc& triggerDataDesc_, std::vector<EnvTriggerEvent*>& envTriggerEvent_, std::vector<EnvTriggerEventDesc>& triggerDesc_, int idx)
	{
		if (idx < 0 || idx > triggerDesc_.size())
			return;
		if (triggerDataDesc_.ObjectName != triggerDesc_[idx].ObjectName)
		{
		
			if (idx < envTriggerEvent_.size() && envTriggerEvent_[idx])
			{
				delete envTriggerEvent_[idx];
				envTriggerEvent_[idx] = nullptr;
			}
		}

		triggerDesc_[idx] = triggerDataDesc_;

		
		if (envTriggerEvent_[idx] != nullptr)
		{
			EnvTriggerEventDesc* desc_ = envTriggerEvent_[idx]->getEnvEventDesc();
			if (desc_)
			{
                *desc_ = triggerDataDesc_;
				if (triggerDataDesc_.Objtype == TypeParticle)
				{
					Echo::EffectHolder* pParticle = static_cast<Echo::EffectHolder*>(envTriggerEvent_[idx]->getObjHandle());
					if (pParticle)
					{
						pParticle->setTodAdjustMode(triggerDataDesc_.todAdjustmode);
						pParticle->setTodSwitchMode(triggerDataDesc_.todSwitchMode);
					}
				}
			}
			return;
		}

		if (triggerDataDesc_.Objtype == TypeParticle)
		{
			envTriggerEvent_[idx] = new EnvParticleTriggerEvent(triggerDataDesc_);
			if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
				_createTriggerEventObjectByIdx(envTriggerEvent_,idx);
		}
		else if (triggerDataDesc_.Objtype == TypeRoleAnimation)
		{
			envTriggerEvent_[idx] = new EnvRoleTriggerEvent(triggerDataDesc_);
			if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
				_createTriggerEventObjectByIdx(envTriggerEvent_,idx);
		}
		else if (triggerDataDesc_.Objtype == TypeEntity)
		{
			envTriggerEvent_[idx] = new EnvEntityTriggerEvent(triggerDataDesc_);
			if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
				_createTriggerEventObjectByIdx(envTriggerEvent_,idx);
		}
		else
			return;
		EnvTriggerEventDesc* desc_ = envTriggerEvent_[idx]->getEnvEventDesc();
		if (desc_)
			*desc_ = triggerDataDesc_;
	}

	void EnvTriggerEventComponent::_createTriggerEventObjectByIdx(std::vector<EnvTriggerEvent*>& envTriggerEvent_,int idx)
	{
		if (envTriggerEvent_[idx])
			envTriggerEvent_[idx]->createObjectmanual();
	}

	void EnvTriggerEventComponent::setYRotateAngle(const std::vector<float>& yRotateAngle_)
	{
		mContext.mYrotateAngle = yRotateAngle_;
	}

	void EnvTriggerEventComponent::setPriority(int priority_)
	{
		priority_ = std::max(0, priority_);
		mContext.mPriority = priority_;
		if(m_tMgr)
			m_tMgr->adjustComponentPriority(m_tMgr->getEnvTriggerContext(), this);
	}

	void EnvTriggerEventComponent::_initEnvTriggerEventData(std::vector<EnvTriggerEvent*>& envTriggerEvent_, const std::vector<EnvTriggerEventDesc>& triggerDesc)
	{
		envTriggerEvent_.resize(triggerDesc.size(), nullptr);

		for (int i = 0; i < triggerDesc.size(); ++i)
		{
			if (triggerDesc[i].Objtype == TypeParticle)
			{
				envTriggerEvent_[i] = new EnvParticleTriggerEvent(triggerDesc[i]);
			}
			else if (triggerDesc[i].Objtype == TypeRoleAnimation)
			{
				envTriggerEvent_[i] = new EnvRoleTriggerEvent(triggerDesc[i]);
			}
			else if (triggerDesc[i].Objtype == TypeEntity)
			{
				envTriggerEvent_[i] = new EnvEntityTriggerEvent(triggerDesc[i]);
			}
			if (envTriggerEvent_[i])
				envTriggerEvent_[i]->requestResource();
		}
	}

	void EnvTriggerEventComponent::_use(std::vector<EnvTriggerEvent*>& envTriggerEvent_,int& currIdx_)
	{
		int sz = static_cast<int>(envTriggerEvent_.size());
		if (mContext.mUpdateMode == Mode_All)
		{
			for (size_t i = 0; i < envTriggerEvent_.size(); ++i)
			{
				EnvTriggerEvent* pEvent = envTriggerEvent_[i];
				if (!pEvent)
					continue;
				pEvent->enterEvent();
			}
		}
		else if (mContext.mUpdateMode == Mode_InOrder)
		{
			for (int i = 0; i < (int)envTriggerEvent_.size(); i++)
			{
				EnvTriggerEvent* pEvent = envTriggerEvent_[i];
				if (!pEvent)
					continue;
				currIdx_ = i;
				pEvent->enterEvent();
				break;
			}
		}
		else if (mContext.mUpdateMode == Mode_RandomOrder)
		{
			currIdx_ = _getRandomIdx(envTriggerEvent_);
			if (currIdx_ > -1 && currIdx_ < sz)
			{
				EnvTriggerEvent* pEvent = mEnvTriggerEventVec[currIdx_];
				if (pEvent)
				{
					pEvent->enterEvent();
				}
			}
		}
	}

	void EnvTriggerEventComponent::_unuse(std::vector<EnvTriggerEvent*>& envTriggerEvent_)
	{
		for (const auto& iter : envTriggerEvent_)
		{
			if (!iter)
				continue;
			iter->stopEvent();
			iter->leaveEvent();
		}
	}

	void EnvTriggerEventComponent::_update(float elapsedTime,Vector3 updatePos,Vector3 cameraPos,const Quaternion& cameraQua, std::vector<EnvTriggerEvent*>& envTriggerEvent_, int updateModel_, int& currIdx)
	{
		if (updateModel_ == Mode_All)
		{
			for (size_t i = 0; i < envTriggerEvent_.size(); ++i)
			{
				EnvTriggerEvent* pEvent = envTriggerEvent_[i];
				if (!pEvent)
					continue;

				bool isOverlay = _setParticleOverlayData(pEvent);
				
				if (isOverlay)
				{
					updatePos = m_tMgr->mOverlayPosition;
					cameraPos = m_tMgr->mOverlayPosition;
				}
				pEvent->updateTriggerEvent(updatePos, cameraPos, cameraQua, elapsedTime);
			}
		}
		else
		{
			int sz = static_cast<int>(envTriggerEvent_.size());
			if (sz == 1 && envTriggerEvent_[0])
			{
				envTriggerEvent_[0]->enterEvent();

				bool isOverlay = _setParticleOverlayData(envTriggerEvent_[0]);

				if (isOverlay)
				{
					updatePos = m_tMgr->mOverlayPosition;
					cameraPos = m_tMgr->mOverlayPosition;
				}
				envTriggerEvent_[0]->updateTriggerEvent(updatePos, cameraPos, cameraQua, elapsedTime);
				currIdx = 0;
				return;
			}


			int nextIndex = -1;
			if (currIdx >= 0 && currIdx < sz)
			{
				EnvTriggerEvent* pevent = envTriggerEvent_[currIdx];
				if (pevent)
				{
					if (pevent->isFirstLoopEnd())
					{
						pevent->leaveEvent();
						nextIndex = _getNextIdx(envTriggerEvent_,currIdx,updateModel_);
					}
					else
					{
						bool isOverlay = _setParticleOverlayData(pevent);

						if (isOverlay)
						{
							updatePos = m_tMgr->mOverlayPosition;
							cameraPos = m_tMgr->mOverlayPosition;
						}
						pevent->updateTriggerEvent(updatePos, cameraPos, cameraQua, elapsedTime);
					}
				}
			}
			else
			{
				nextIndex = _getNextIdx(envTriggerEvent_, currIdx, updateModel_);
			}

			if (nextIndex >= 0 && nextIndex < sz)
			{
				EnvTriggerEvent* pevent = envTriggerEvent_[nextIndex];
				if (pevent)
				{
					pevent->enterEvent();
					bool isOverlay = _setParticleOverlayData(pevent);

					if (isOverlay)
					{
						updatePos = m_tMgr->mOverlayPosition;
						cameraPos = m_tMgr->mOverlayPosition;
					}
					pevent->updateTriggerEvent(updatePos, cameraPos, cameraQua, elapsedTime);
				}
				currIdx = nextIndex;
			}
		}
	}

	int EnvTriggerEventComponent::_getNextIdx(const std::vector<EnvTriggerEvent*>& envTriggerEvent_, int currIdx, int updateModel)
	{
		if (updateModel == Mode_InOrder)
		{
			int index = currIdx + 1;

			//寻找下一个可用事件
			int sz = static_cast<int>(envTriggerEvent_.size());
			while (index < sz && envTriggerEvent_[index] == nullptr)
			{
				if (envTriggerEvent_[index])
					return index;
				++index;
				index %= sz;
				if (index == currIdx)
					return currIdx;
			}

		}
		else if (updateModel == Mode_RandomOrder)
		{
			return _getRandomIdx(envTriggerEvent_);
		}

		return -1;
	}
	void EnvTriggerEventComponent::_resetEvent(int idx, std::vector<EnvTriggerEvent*>& envTriggerEvent_, int currIdx, int updateModel)
	{
		if (idx >= envTriggerEvent_.size())
			return;
		EnvTriggerEvent* triggerEvent_ = envTriggerEvent_[idx];
		if (triggerEvent_)
		{
			triggerEvent_->stopEvent();
			triggerEvent_->calResetTime();
			if (updateModel == Mode_All)
				triggerEvent_->enterEvent();
			else
			{
				if (currIdx == idx)
					triggerEvent_->enterEvent();
			}
		}
	}
	
#ifdef ECHO_EDITOR
	void EnvTriggerEventComponent::setTriggerEventName(const std::vector<std::string>& triggerEventName_)
	{
		mContext.mTriggerEventName = triggerEventName_;
	}
	
#endif
	void EnvTriggerEventComponent::setTriggerEventAllData(const std::vector<EnvTriggerEventDesc>& triggerData_)
	{
		mContext.mTriggerEventData = triggerData_;
	}

	void EnvTriggerEventComponent::setTriggerEventAllData(const std::vector<EnvTriggerEventDesc>& triggerData_, int i)
	{
		setTriggerEventData(std::make_pair(i, triggerData_[i]));
		resetEvent(i);
		mContext.mTriggerEventData = triggerData_;
	}

	void EnvTriggerEventComponent::setTriggerEventAllData(const std::vector<EnvTriggerEventDesc>& triggerData_, bool isAdd, int i)
	{
		if (isAdd)
			addTriggerEvent();
		else
			deleteTriggerEvent(i);
		//mContext.mTriggerEventData = triggerData_;
	}


	void EnvTriggerEventComponent::setSecondTriggerEventAllData(const std::vector<EnvTriggerEventDesc>& triggerData_)
	{ 
		mContext.mSecondTriggerEventData = triggerData_; 
	}
	void EnvTriggerEventComponent::setSecondTriggerEventAllData(const std::vector<EnvTriggerEventDesc>& triggerData_, int i)
	{
		setSecondTriggerEventData(std::make_pair(i, triggerData_[i]));
		resetSecondEvent(i);
		mContext.mSecondTriggerEventData = triggerData_;
	}

	void EnvTriggerEventComponent::setSecondTriggerEventAllData(const std::vector<EnvTriggerEventDesc>& triggerData_, bool isAdd, int i)
	{
		if (isAdd)
			addSecondTriggerEvent();
		else
			deleteSecondTriggerEvent(i);
		//mContext.mSecondTriggerEventData = triggerData_;
	}

	void EnvTriggerEventComponent::setTriggerEventData(const std::pair<int, EnvTriggerEventDesc>& triggerDataDesc_)
	{
		_setTriggerEventDataByIdx(triggerDataDesc_.second, mEnvTriggerEventVec,mContext.mTriggerEventData, triggerDataDesc_.first);
	}

	void EnvTriggerEventComponent::setSecondTriggerEventData(const std::pair<int, EnvTriggerEventDesc>& triggerDataDesc_)
	{
		_setTriggerEventDataByIdx(triggerDataDesc_.second, mSecondEnvTriggerEventVec, mContext.mSecondTriggerEventData, triggerDataDesc_.first);
	}
	
	void EnvTriggerEventComponent::_setIsUseSecondTriggerGroup(bool isUse_)
	{
		if (isUse_)
			_use(mSecondEnvTriggerEventVec, mSecondCurrIdx);
		else
			_unuse(mSecondEnvTriggerEventVec);
	}

	bool EnvTriggerEventComponent::_setParticleOverlayData(EnvTriggerEvent* pEvent)
	{
		if (!pEvent)
			return false;
		EnvTriggerEventDesc* desc_ = pEvent->getEnvEventDesc();
		if (desc_)
		{
			if (desc_->Objtype == TypeParticle)
			{
				Echo::EffectHolder* pParticle = static_cast<Echo::EffectHolder*>(pEvent->getObjHandle());
				if (pParticle)
				{
					EffectHolder::ErrorCode result = pParticle->setAllElementRingEmitterInnerRadius(m_tMgr->mOverlayInRadiu);
					pParticle->setAllElementRingEmitterRadius(m_tMgr->mOverlayOutRadiu);

					if (result == EffectHolder::ErrorCode::NoError)
					{
						if (!m_tMgr->mIsUseOverlay)
							return false;
						return true;
					}
				}
			}
		}
		return false;
	}

	bool    EnvTriggerEventComponent::getIsUseSecondTriggerGroup()
	{
		if (!m_tMgr)
			return false;
		return m_tMgr->getIsUseSecondGroup();
	}


	void  WriteJSNode(JSNODE jsonNode, const char* Name, const EnvTriggerEventDesc& triggerEventDesc)
	{
		if (jsonNode == nullptr)
			return;
		cJSON* triggerEventDesc_ = cJSON_CreateObject();
		cJSON_AddItemToObject(jsonNode, "TriggerEventDesc", triggerEventDesc_);
		if(triggerEventDesc_)
		{
			WriteJSNode(triggerEventDesc_, "ObjectName", triggerEventDesc.ObjectName);

			int objType = static_cast<int>(triggerEventDesc.Objtype);
			WriteJSNode(triggerEventDesc_, "ObjectType", objType);
			WriteJSNode(triggerEventDesc_, "ObjectScale", triggerEventDesc.ObjScale);
			WriteJSNode(triggerEventDesc_, "YRotate", triggerEventDesc.YRotate);
			WriteJSNode(triggerEventDesc_, "FlushTime", triggerEventDesc.FlushTime);
			WriteJSNode(triggerEventDesc_, "PlayTime", triggerEventDesc.playTime);

			WriteJSNode(triggerEventDesc_, "AnimationName", triggerEventDesc.AnimationName);
			int flushMode_ = static_cast<int>(triggerEventDesc.Flushmode);
			WriteJSNode(triggerEventDesc_, "FlushMode", flushMode_);
			WriteJSNode(triggerEventDesc_, "bCameraRotate", triggerEventDesc.bCameraRotate);
			
			WriteJSNode(triggerEventDesc_, "OffsetPos", triggerEventDesc.OffsetPos);
			WriteJSNode(triggerEventDesc_, "bSnapToTerrain", triggerEventDesc.bSnapToTerrain);

			int adjustMode_ = static_cast<int>(triggerEventDesc.adjustModel);
			WriteJSNode(triggerEventDesc_, "adjustModel", adjustMode_);
			int todAdjustMode_ = static_cast<int>(triggerEventDesc.todAdjustmode);
			WriteJSNode(triggerEventDesc_, "TodAdjustModel", todAdjustMode_);
			int switchMode_ = static_cast<int>(triggerEventDesc.todSwitchMode);
			WriteJSNode(triggerEventDesc_, "TodSwitchMode", switchMode_);

			WriteJSNode(triggerEventDesc_, "bIsPriority", triggerEventDesc.bIsPrior);
			WriteJSNode(triggerEventDesc_, "bFadeOut", triggerEventDesc.bFadeout);
			WriteJSNode(triggerEventDesc_, "FollowRotate", triggerEventDesc.followRotate);

			WriteJSNode(triggerEventDesc_, "autoRotation", triggerEventDesc.autoRotation);
			WriteJSNode(triggerEventDesc_, "fixedYOffset", triggerEventDesc.fixedYOffset);
			WriteJSNode(triggerEventDesc_, "ignoreYOffset", triggerEventDesc.ignoreYOffset);
			WriteJSNode(triggerEventDesc_, "culldistance", triggerEventDesc.culldistance);
		}
	}
	bool  ReadJSNode(cJSON* jsonNode, const char* Name, EnvTriggerEventDesc& triggerEventDesc)
	{
		if (jsonNode == nullptr)
			return false;
		cJSON* triggerEventDesc_ = cJSON_GetObjectItem(jsonNode, "TriggerEventDesc");
		if (triggerEventDesc_)
		{
			ReadJSNode(triggerEventDesc_, "ObjectName", triggerEventDesc.ObjectName);
			int objType;
			ReadJSNode(triggerEventDesc_, "ObjectType", objType);
			triggerEventDesc.Objtype = static_cast<ObjectType>(objType);
			ReadJSNode(triggerEventDesc_, "ObjectScale", triggerEventDesc.ObjScale);
			ReadJSNode(triggerEventDesc_, "YRotate", triggerEventDesc.YRotate);
			ReadJSNode(triggerEventDesc_, "FlushTime", triggerEventDesc.FlushTime);
			ReadJSNode(triggerEventDesc_, "PlayTime", triggerEventDesc.playTime);

			ReadJSNode(triggerEventDesc_, "AnimationName", triggerEventDesc.AnimationName);
			int flushMode_;
			ReadJSNode(triggerEventDesc_, "FlushMode", flushMode_);
			triggerEventDesc.Flushmode = static_cast<FlushMode>(flushMode_);

			ReadJSNode(triggerEventDesc_, "bCameraRotate", triggerEventDesc.bCameraRotate);
			ReadJSNode(triggerEventDesc_, "OffsetPos", triggerEventDesc.OffsetPos);
			ReadJSNode(triggerEventDesc_, "bSnapToTerrain", triggerEventDesc.bSnapToTerrain);

			int adjustMode_;
			ReadJSNode(triggerEventDesc_, "adjustModel", adjustMode_);
			triggerEventDesc.adjustModel = static_cast<EffectAdjustMode>(adjustMode_);

			int todAdjustMode_;
			ReadJSNode(triggerEventDesc_, "TodAdjustModel", todAdjustMode_);
			triggerEventDesc.todAdjustmode = static_cast<TODAdjustMode>(todAdjustMode_);

			int switchMode_; 
			ReadJSNode(triggerEventDesc_, "TodSwitchMode", switchMode_);
			triggerEventDesc.todSwitchMode = static_cast<TODSwitchMode>(switchMode_);

			ReadJSNode(triggerEventDesc_, "bIsPriority", triggerEventDesc.bIsPrior);
			ReadJSNode(triggerEventDesc_, "bFadeOut", triggerEventDesc.bFadeout);
			ReadJSNode(triggerEventDesc_, "FollowRotate", triggerEventDesc.followRotate);

			ReadJSNode(triggerEventDesc_, "autoRotation", triggerEventDesc.autoRotation);
			ReadJSNode(triggerEventDesc_, "fixedYOffset", triggerEventDesc.fixedYOffset);
			ReadJSNode(triggerEventDesc_, "ignoreYOffset", triggerEventDesc.ignoreYOffset);
			ReadJSNode(triggerEventDesc_, "culldistance", triggerEventDesc.culldistance);
			return true;
		}
		return false;
	}

	void  WriteJSNode(JSNODE jsonNode, const char* Name, UpdateMode updateMode)
	{
		if (jsonNode == nullptr)
			return;
		int mode_ = static_cast<int>(updateMode);
		WriteJSNode(jsonNode, Name, mode_);
	}
	bool  ReadJSNode(cJSON* jsonNode, const char* Name, UpdateMode& updateMode)
	{
		if (jsonNode == nullptr)
			return false;
		int mode_;
		ReadJSNode(jsonNode, Name, mode_);
		updateMode = static_cast<UpdateMode>(updateMode);
		return true;
	}
}
