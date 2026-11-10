#include "EchoComponentStableHeaders.h"
#include "EchoTodTimelineComponent.h"
#include "EchoTODManager.h"
#include "EchoTODComponentManager.h"
#include "EchoSun.h"
#include "EchoSunComponent.h"
#include "EchoTODComponentSystem.h"
#include "EchoCloud.h"
#include "EchoCloudMeshComponent.h"
#include "EchoCloudSphereComponent.h"
#include "EchoLensFlare.h"

namespace Echo {

	static const float g_dayComFactor = 25200.f; //7点
	static const float g_nightComFactor = 68400.f; //19点

	DECLAREACTORCOMTYPE_CPP(TodTimelineComponent)

	TodTimelineComponentInfo::TodTimelineComponentInfo(const Name & name, ActorCOMMoveableType moveable)
		:TimelineComponentInfo(name,moveable)
	{
	}
	TodTimelineComponentInfo::~TodTimelineComponentInfo()
	{
	}
	
	 TodTimelineComponent::TodTimelineComponent(ActorPtr  pActor, ActorCOMMoveableType isMoveable)
		:TimelineComponent(pActor, isMoveable)
	{
		
	}

	TodTimelineComponent::~TodTimelineComponent()
	{
	}

	TodTimelineComponent * EchoTodTimelineScript::GetTodTimeLineCom(ActorPtr pActor, const std::string & name)
	{
		if(pActor == nullptr)
		   return nullptr;
		return nullptr;
	}
	void TodTimelineComponent::init(ActorComponentInfo * info)
	{
		TimelineComponent::init(info);
		mTimeNodeSpeeds.reserve(24);
		mTimeNodeSpeeds.resize(24, 1.f);
		currentPlayWay = LOOP_PLAY;
		mDeltaTime = 1.f / 60.f;
		endSceneTime = 60.f * 60.f * 24.f;
#ifdef ECHO_EDITOR
		mShowTimeMax = endSceneTime;
#endif
        
		Name levelName_ = g_MainTodComponentName;
		SceneManager* sMgr = getSceneManager();
		if (sMgr)
			levelName_ = sMgr->getName();
		m_tMgr = TODComponentSystem::instance()->getTODComponentManager(levelName_);
	}
	void TodTimelineComponent::unInit()
	{
		TimelineComponent::unInit();
		for (int i = 0; i < AttributeComponents.size(); i++)
			AttributeComponents[i] = nullptr;

		if (m_tMgr)
			m_tMgr->delTodComVec(this);
		m_tMgr = nullptr;
	}
	void TodTimelineComponent::copyTo(const ActorComponentPtr& p_actor_component)
	{
	}
	bool TodTimelineComponent::isCreateFinish()
	{
		return m_isCreateFinish;
	}

	void TodTimelineComponent::resourceArrived(const Name& resName, const Name& resType, bool bFail, const ResourceBasePtr& resourcePtr)
	{
		if (resType == TimeLineInfoManager::instance()->getResourceType())
			TimelineComponent::resourceArrived(resName, resType, bFail, resourcePtr);
		else
		{
			if (resourcePtr.isNull())
			{
				std::string logInfo = "";
				logInfo.append("Texture :").append(resName.c_str()).append(" load false");
				LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
				return;
			}

			if (m_tMgr)
				m_tMgr->addTodTexturePtr(resName.c_str(), static_cast<TexturePtr>(resourcePtr));

			m_bLoadTexture[resName.toString()] = true;

			if (!m_isCreateFinish)
			{
				m_isCreateFinish = true;
				for (const auto& iter : m_bLoadTexture)
					m_isCreateFinish &= iter.second;

				if (m_isCreateFinish)
					m_pActor->notifyComponentFinish(weak_from_this());
			}
		}
	}

	void TodTimelineComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->addTodComVec(this);
		else
			m_tMgr->delTodComVec(this);
	}

	void TodTimelineComponent::setDayTime(const int& dayTime_, const int& minuteTime_)
	{
		mCurrHour = dayTime_;
		mCurrMinutes = minuteTime_;
		remakeDayTime();
	}
	void TodTimelineComponent::updateTime()
	{
		int  allMinutes  = static_cast<int>(currTime / 60.f);
		mCurrSeconds = static_cast<int>(currTime) % 60;
		mCurrHour = allMinutes / 60;
		mCurrMinutes = allMinutes % 60;
	}
	void TodTimelineComponent::remakeDayTime()
	{
		currTime = mCurrHour * 3600.f + mCurrMinutes * 60.f ;
		if (!getIsPlayScene())
			setComponentAttribute();
	}

#ifdef ECHO_EDITOR
	void TodTimelineComponent::mirrorReplicationOneKey(const size_t& idx)
	{
		float midTime = 12 * 3600.f;
		if (currTime == midTime )
			return;
		int oriFrame = static_cast<int>(currTime * mDeltaTime + 0.5f);
		float differenceTime = currTime - midTime;
		float resultTime = midTime - differenceTime;
		int resultFrame = static_cast<int>(resultTime* mDeltaTime + 0.5f);
		float resultValue = floatInterpolation(idx, (float)oriFrame);
		AddKey(idx, resultFrame, resultValue);
	}
	void TodTimelineComponent::mirrorReplicationGroupKey(const size_t& idx)
	{
		float midTime = 12 * 3600.f;
		int midFrame = 12 * 60;
		if (currTime == midTime)
			return;
		auto& hermiteMap = mHermites[idx]->mKeyMap;
		if (hermiteMap.empty())
			return;
		auto resultIter = hermiteMap.lower_bound(midFrame);
		if (resultIter == hermiteMap.end())
			return;
		if (currTime > midTime)
		{
			for (auto mapIter = hermiteMap.begin(); mapIter != resultIter; ++mapIter)
			{
				if (mapIter->second != nullptr)
					delete mapIter->second;
				mapIter->second = nullptr;
			}
            hermiteMap.erase(hermiteMap.begin(), resultIter);
		}
		else
		{
			if (resultIter->first == midFrame)
				++resultIter;
			for (auto mapIter = resultIter; mapIter != hermiteMap.end(); ++mapIter)
			{
				if (mapIter->second != nullptr)
					delete mapIter->second;
				mapIter->second = nullptr;
			}
			hermiteMap.erase(resultIter, hermiteMap.end());
		}
		std::vector<std::pair<int, float>> resultHermiteData;
		for (const auto& mapIter : hermiteMap)
		{
			if (mapIter.first == midFrame)
				continue;
			int resultFrame = midFrame * 2 - mapIter.first;
			HermiteFloatKey* key = static_cast<HermiteFloatKey*>(mapIter.second);
			if (key == nullptr)
				continue;
			float resultValue = key->value;
			resultHermiteData.emplace_back(std::make_pair(resultFrame, resultValue));
		}

		for (const auto& valueIter : resultHermiteData)
		{
			AddKey(idx, valueIter.first, valueIter.second);
		}

	}
	void TodTimelineComponent::mirrorReplicationOneKey()
	{
		for (int i = 0; i < showCurveVec.size(); i++)
		{
			if (showCurveVec[i] == 1)
				mirrorReplicationOneKey(i);
		}
	}
	void TodTimelineComponent::mirrorReplicationGroupKey()
	{
		for (int i = 0; i < showCurveVec.size(); i++)
		{
			if (showCurveVec[i] == 1)
				mirrorReplicationGroupKey(i);
		}
	}
	bool TodTimelineComponent::judgeAttributeIsSave(const std::string& comName_, const Name& attrName_)
	{
		if (m_PTimelineInfo.isNull())
			return false;
		if (!m_PTimelineInfo->mComponentToAttributes.contains(comName_))
			return false;
		for (int i = 0 ; i < m_PTimelineInfo->mComponentToAttributes[comName_].size() ; ++i)
		{
			if (m_PTimelineInfo->mComponentToAttributes[comName_][i].attributeName == attrName_)
				return true;
		}
		return false;
	}
#endif

	void TodTimelineComponent::AddKey(size_t vIdx, int weight,float value, bool flag_)
	{
		TimelineComponent::AddKey(vIdx, weight, value, flag_);
		if (weight == 0)
			TimelineComponent::AddKey(vIdx, 60 * 24, value, flag_);
		else if (weight == 60 * 24)
			TimelineComponent::AddKey(vIdx, 0, value, flag_);

#ifdef ECHO_EDITOR
		mIsSaveFile = false;
#endif

	}
	void TodTimelineComponent::AddKey(size_t vIdx, int frame, const std::string& value)
	{
		TimelineComponent::AddKey(vIdx, frame, value);
		if (frame == 0)
			TimelineComponent::AddKey(vIdx, 60 * 24, value);
		else if (frame == 60 * 24)
			TimelineComponent::AddKey(vIdx, 0, value);
#ifdef ECHO_EDITOR
		mIsSaveFile = false;
#endif
	}
	void TodTimelineComponent::SetKeyValue(size_t vIdx, int keyVal, float value)
	{
		TimelineComponent::SetKeyValue(vIdx, keyVal, value);
		if (keyVal == 0)
			TimelineComponent::SetKeyValue(vIdx, 60 * 24, value);
		else if (keyVal == 60 * 24)
			TimelineComponent::SetKeyValue(vIdx, 0, value);
#ifdef ECHO_EDITOR
		mIsSaveFile = false;
#endif
	}
	void TodTimelineComponent::SetKeyValue(size_t vIdx, int keyVal, const std::string& value)
	{
		TimelineComponent::SetKeyValue(vIdx, keyVal, value);
		if (keyVal == 0)
			TimelineComponent::SetKeyValue(vIdx, 60 * 24, value);
		else if (keyVal == 60 * 24)
			TimelineComponent::SetKeyValue(vIdx, 0, value);
#ifdef ECHO_EDITOR
		mIsSaveFile = false;
#endif
	}
	void TodTimelineComponent::MoveKey(size_t vIdx, int oldT, int newT, float value)
	{
		if ((oldT == 0 || oldT == 1440) && oldT != newT)
		{
			newT = oldT;
		}
		TimelineComponent::MoveKey(vIdx, oldT,newT, value);
		if (newT == 0)
			TimelineComponent::SetKeyValue(vIdx, 60 * 24, value);
		else if (newT == 60 * 24)
			TimelineComponent::SetKeyValue(vIdx, 0, value);
#ifdef ECHO_EDITOR
		mIsSaveFile = false;
#endif
	}
	void TodTimelineComponent::DeleteKey(size_t vIdx, int weight)
	{
		TimelineComponent::DeleteKey(vIdx, weight);
		if (weight == 0)
			TimelineComponent::DeleteKey(vIdx, 60 * 24);
		else if (weight == 60 * 24)
			TimelineComponent::DeleteKey(vIdx, 0);

#ifdef ECHO_EDITOR
		mIsSaveFile = false;
#endif
	}
	void TodTimelineComponent::dealWithOldData()
	{
		initOldData();
		for (size_t i = 0; i < AttributeName.size(); i++)
		{
			size_t idx = AttributeName[i].find(" : ");
			if (idx == std::string::npos)
				return;
			std::string comName = AttributeName[i].substr(0,idx);
			std::string attrName = AttributeName[i].substr(idx + 3, AttributeName.size());
			if (mComponentToAttributes.find(comName) == mComponentToAttributes.end())
			{
				mComponentToAttributes.insert(std::make_pair(comName, std::vector<attributeData>()));
				mComMaps.insert(std::make_pair(comName,AttributeComponents[i]));
				if (AttributeComponents[i] && AttributeComponents[i]->getType() == SunComponent::mType)
				{
					mIsControlSun = true;
				}
			}
			attributeData tempData;
			tempData.st = AttributeBegin[i];
			tempData.en = AttributeEnd[i];
			tempData.attributeName = attrName;
			mComponentToAttributes[comName].emplace_back(tempData);
		}
	}
	void TodTimelineComponent::initActorAndComData()
	{
		for (auto& comIter : mComponentToAttributes)
		{
			ActorComponentPtr com = getActor()->getComponent(comIter.first);
			mComMaps.insert(std::make_pair(comIter.first, com));
			if (com && com->getType() == SunComponent::mType)
			{
				mIsControlSun = true;
			}
		}

		std::set<String> pTexPaths;
		for (auto& comIter : mComponentToAttributes)
		{
			ActorComponentPtr com = getActor()->getComponent(comIter.first);
			if (com)
			{
				if (com->getType() == CloudMeshComponent::mType)
				{
					for (const auto& attrIter : mComponentToAttributes[comIter.first])
					{
						if (CloudMeshComponent::isTextureAttribute(attrIter.attributeName))
							for (const auto& key : mHermites[attrIter.st]->mKeyMap)
								pTexPaths.insert(static_cast<HermiteStringKey*>(key.second)->value);
					}
				}
				else if (com->getType() == CloudSphereComponent::mType)
				{
					for (const auto& attrIter : mComponentToAttributes[comIter.first])
					{
						if (CloudSphereComponent::isTextureAttribute(attrIter.attributeName))
							for (const auto& key : mHermites[attrIter.st]->mKeyMap)
								pTexPaths.insert(static_cast<HermiteStringKey*>(key.second)->value);
					}
				}
			}
		}

		if (!m_isCreateFinish)
		{
			m_isCreateFinish = true;
			for (const String& path : pTexPaths)
			{
				if (!m_tMgr) continue;

				TexturePtr texPtr;
				m_tMgr->getTodTexturePtrByName(path, texPtr);
				if (texPtr.isNull())
				{
					m_isCreateFinish = false;
					ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_SKY);
					requestResouce(Name(path), TextureManager::instance()->getResourceType(), priority);
					m_bLoadTexture[path] = false;
				}
			}
		}
	}
	void TodTimelineComponent::setComponentAttribute()
	{
		bool bMoonRise = true;
		if (g_dayComFactor < currTime && currTime < g_nightComFactor)
			bMoonRise = false;
		else
			bMoonRise = true;

		if (m_tMgr)
		{
			if (mIsControlSun && m_tMgr->getSunContext().mIsUseCurrType)
			{
				Sun* pSun_ = m_tMgr->getSun();
				if (pSun_)
					pSun_->updateMoonPhase(bMoonRise);
			}

			SceneManager* s_Mgr = m_tMgr->GetSceneManager();
			if (s_Mgr)
			{
				Sun* pSun = s_Mgr->getSun();
				//夜晚将太阳纹理换为月亮的
				if (pSun)
					pSun->setMoonRise(bMoonRise);

				Cloud* pCloud = s_Mgr->getCloud();
				//夜晚将片云纹理换为星星的
				if (pCloud)
					pCloud->setMoonRise(eCloudType_Plane, bMoonRise);

				LensFlare* pLensFlare = s_Mgr->getLensFlare();
				//夜晚将不渲染光晕
				if (pLensFlare)
					pLensFlare->setMoonRise(bMoonRise);
			}
		}
		
		for (const auto& comIter : mComponentToAttributes)
		{
			if (mComponentToAttributes.find(comIter.first) == mComponentToAttributes.end())
				continue;
			EchoWeakPtr<ActorComponent>  currCom = nullptr;
			if (mComMaps.find(comIter.first) != mComMaps.end())
			{
				if (!mComMaps[comIter.first].expired())
					currCom = mComMaps[comIter.first];
			}
			if (currCom == nullptr)
				continue;
	
			for (const auto& attrIter : mComponentToAttributes[comIter.first])
			{
				int st = attrIter.st;
				int en = attrIter.en;
				if (st >= mHermites.size())
					continue;   
				const int sz = en - st;

				auto type_ = mHermites[st]->mHermiteType;
				if (type_ == EchoHermite::String_Hermite)
				{
					float frame = getRealTime();
					std::string value_;
					
					if (!attrIter.isAnimation)
					{
						stringInterpolation(st, frame, value_);
						currCom->setTimelineStringAttribute(value_, attrIter.attributeName);
					}
					else
					{
						float data = 0.f;
						HermiteStringKey* pKey = TimeConsuming(attrIter.st, frame, data);
						if (pKey)
						{
							if (attrIter.isTruncation)
								value_ = pKey->value;
							else
								value_ = pKey->path;
						}
						currCom->setTimelineStringAttribute(value_, data, attrIter.attributeName); // ToAnimation
					}
				}
				else
				{
					std::vector<float> data(sz);
					for (int j = st; j < en; j++)
					{
						float frame = getRealTime();
						if (attrIter.isColor == 1)
							data[j - st] = colorInterpolation(j, frame);
						else
							data[j - st] = floatInterpolation(j, frame);
					}
					currCom->setTimelineAttribute(data, attrIter.attributeName);
				}
			}
			currCom = nullptr;
		}
	}
	void TodTimelineComponent::updataTodPlayState(bool isReduce)
	{
	    currTime += Echo::Root::instance()->getTimeSinceLastFrameClient() * mTodPlaySpeed;
		if (currTime >= endSceneTime)
			currTime = endSceneTime;
	}
	void TodTimelineComponent::setTodPlaySpeed(float speed_)
	{
		mTodPlaySpeed = speed_;

		if (m_tMgr)
		{
			TODManager* tMgr = m_tMgr->GetTODManager();
			if (tMgr)
				tMgr->setTimeSpeed(double(speed_));
		}
	}
#ifdef ECHO_EDITOR
	bool TodTimelineComponent::setTodAttributeData(const std::string& bindName, const std::string& attrName, float* data, const int& size, float& offect, bool isItemHover)
	{    
		if (data == nullptr)
			return false;
		String combinedID = getActor()->getCombinedID();

		size_t endi = bindName.rfind("&");

		if (combinedID.size() != endi - 1) return false;
		if (bindName.find(combinedID) == String::npos) return false;

		String comName = bindName.substr(endi + 1);

		auto com = getActor()->getComponent(comName);
		if (com == nullptr || com->getActor() != getActor())
			return false;
		if (mComponentToAttributes.find(comName) == mComponentToAttributes.end())
		{
             mComponentToAttributes.insert(std::make_pair(comName, std::vector<attributeData>()));
			 ActorComponentPtr com = getActor()->getComponent(comName);
			 mComMaps.insert(std::make_pair(comName, com));
			 if (com && com->getType() == SunComponent::mType)
			 {
				 mIsControlSun = true;
			 }
		}
	
		attributeData tempData;
		bool flag = false;
		for (const attributeData& attrIter : mComponentToAttributes[comName])
		{
			if (attrIter.attributeName.c_str() == attrName)
			{
				tempData = attrIter;
				flag = true;
				break;
		    }
		}
		int currFrame = getTimeLineFrame();
		
		if (!flag)
		{
			tempData.st = OrderCnt;
			OrderCnt += size;
			tempData.en = OrderCnt;
			tempData.attributeName = attrName;
			mComponentToAttributes[comName].emplace_back(tempData);

			for (int i = tempData.st; i < tempData.en; i++)
			{
				AddProperty(EchoHermite::HermiteType::Float_Hermite);
				AddKey( (size_t)i ,0,  *(data + i - tempData.st));
			}
		}
		else
		{
			if (!isItemHover)
			{
				for (int i = tempData.st; i < tempData.en; i++)
				{
					if (!bPlayScene)
					   AddKey(i, currFrame, *(data + i - tempData.st));
				}
			}
		}

		mSelectAttributeData.attributeName = tempData.attributeName.c_str();
		mSelectAttributeData.comName = comName;
		mSelectAttributeData.st = tempData.st;
		mSelectAttributeData.en = tempData.en;
		if ((showCurveVec[tempData.st] == 1 && !flag) || showCurveVec[tempData.st] == 0)
		{
            remakeCurveState();
			for (int i = tempData.st; i < tempData.en; i++)
				showCurveVec[i] = 1;
			offect = getActorTimeValue(tempData.st);
			mWidthMultiple = 1.f;
			mShowTimeMin = 0.f;
			mShowTimeMax = 3600.f * 24;
			todProportion = 60.f;
			return true;
		}
		return false;
	}
	bool TodTimelineComponent::setTodStringAttributeData(const std::string& bindName, const std::string& attrName, const std::string& value, bool isItemHover)
	{
		String combinedID = getActor()->getCombinedID();

		size_t endi = bindName.rfind("&");
		if (combinedID.size() != endi - 1) return false;
		if (bindName.find(combinedID) == String::npos) return false;

		String comName = bindName.substr(endi + 1);

		auto com = getActor()->getComponent(comName);
		if (com == nullptr || com->getActor() != getActor())
			return false;
		if (mComponentToAttributes.find(comName) == mComponentToAttributes.end())
		{
			mComponentToAttributes.insert(std::make_pair(comName, std::vector<attributeData>()));
			auto com = getActor()->getComponent(comName);
			mComMaps.insert(std::make_pair(comName, com));
		}

		attributeData tempData;
		bool flag = false;
		for (const attributeData& attrIter : mComponentToAttributes[comName])
		{
			if (attrIter.attributeName == attrName.c_str())
			{
				tempData = attrIter;
				flag = true;
				break;
			}
		}
		int currFrame = getTimeLineFrame();

		if (!flag)
		{
			tempData.st = OrderCnt;
			OrderCnt += 1;
			tempData.en = OrderCnt;
			tempData.attributeName = attrName;
			mComponentToAttributes[comName].emplace_back(tempData);

			for (int i = tempData.st; i < tempData.en; i++)
			{
				AddProperty(EchoHermite::HermiteType::String_Hermite);
				AddKey((size_t)i, 0, value);
			}
		}
		else
		{
			if (!isItemHover)
			{
				for (int i = tempData.st; i < tempData.en; i++)
				{
					if (!bPlayScene)
						AddKey(i, currFrame, value);
				}
			}
		}

		mSelectAttributeData.attributeName = tempData.attributeName.c_str();
		mSelectAttributeData.comName = comName;
		mSelectAttributeData.st = tempData.st;
		mSelectAttributeData.en = tempData.en;
		if ((showCurveVec[tempData.st] == 1 && !flag) || showCurveVec[tempData.st] == 0)
		{
			remakeCurveState();
			for (int i = tempData.st; i < tempData.en; i++)
				showCurveVec[i] = 1;
			mWidthMultiple = 1.f;
			mShowTimeMin = 0.f;
			mShowTimeMax = 3600.f * 24;
			todProportion = 60.f;
			return true;
		}
		return false;
	}
#endif
	void TodTimelineComponent::preUpdateScene(EventID eventId, ProtocolHandle handle)
	{
		if (bPlayScene == false)
			return;
		switch (eventId)
		{
		      case EID_EveryFrame:
			  {
				  runLoopVideo();
				  setComponentAttribute();
				  break;
			  }
			  default:
				  break;
		}
	}
	void TodTimelineComponent::setDayNode(const DayTimeNode& node_)
	{
		mTimeNode = node_;
    	switch(node_){
			case SUNRISE:
			{
				mCurrHour = 0;
				break;
			}	
			case MORNING:
			{
				mCurrHour = 6;
				break;
			}
			case MID_MORNING:
			{
				mCurrHour = 8;
				break;
			}
			case NOON:
			{
				mCurrHour = 12;
				break;
			}
			case AFTERNOON:
			{
				mCurrHour = 17;
				break;
			}
			case Banwan:
			{
				mCurrHour = 18;
				break;
			}
			case EVENING:
			{
				mCurrHour = 20;
				break;
			}
			case SUNSET:
			{
				mCurrHour = 24;
				break;
			}
			default:
				break;
		}
		mCurrMinutes = 0;
		remakeDayTime();
	}

	void TodTimelineComponent::runLoopVideo()
	{
		if (currTime < endSceneTime)
		{
			currTime += Root::instance()->getTimeSinceLastFrameClient() * mTodPlaySpeed * mTimeNodeSpeeds[mCurrHour];
			if (currTime >= endSceneTime)
				currTime = startSceneTime;
			updateTime();
		}
	}
	void TodTimelineComponent::setIsPlay(bool isplay_)
	{
		bPlayScene = isplay_;
		if (isplay_)
		{
			std::string tempStr;
			float tempFloat;
			double tempDouble;

			if (m_tMgr)
			{
				TODManager* tMgr = m_tMgr->GetTODManager();
				if (tMgr)
				{
					tMgr->getTodStatus(tempStr, tempStr, tempDouble, tempFloat, isTodEditorPlay);
					getTimeNodeSpeed();
					if (isTodEditorPlay)
					{
						mTodPlaySpeed = (float)tMgr->getTimeSpeed();
						currTime = (float)tMgr->getCurrentTime() * 60 * 60 * 24;
					}
				}
			}
			updateTime();
		}
	}
	void TodTimelineComponent::getTimeNodeSpeed()
	{
		if (m_tMgr == nullptr)
			return;

		TODManager* tMgr = m_tMgr->GetTODManager();
		if (tMgr == nullptr)
			return;

		auto speedSlots = tMgr->getTODTimeSpeedSlots();
		for (auto item : speedSlots)
		{
			int st = static_cast<int>(item.mStart * 24);
			int en = static_cast<int>(item.mEnd * 24);
			for (int i = st ; i < en; i++)
				mTimeNodeSpeeds[i] = static_cast<float>(item.mSpeed);
		}
	}
};
