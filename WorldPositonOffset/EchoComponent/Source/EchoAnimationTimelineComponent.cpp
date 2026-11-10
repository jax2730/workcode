#include "EchoComponentStableHeaders.h"
#include "EchoAnimationTimelineComponent.h"
#include "EchoTimelineComponent.h"
#include "Actor/EchoActorHelper.h"
#include "EchoHermite.h"


namespace Echo {

	DECLAREACTORCOMTYPE_CPP(AnimationTimelineComponent)

	EDITOR_REFLECT_BEGIN(AnimationTimelineComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(mFilePath, "FilePath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_TickSpeed, "TickSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()



	std::unordered_map<int, AnimTickFunc>   AnimationTimelineComponent::mTypeToTickFunc = std::unordered_map<int, AnimTickFunc>();

	AnimationTimelineComponentInfo::AnimationTimelineComponentInfo(const Name& name, ActorCOMMoveableType moveable)
		:ActorComponentInfo(name, moveable)
	{
	}

	AnimationTimelineComponentInfo::~AnimationTimelineComponentInfo()
	{
		
	}
	
	void AnimationTimelineComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<AnimationTimelineComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void AnimationTimelineComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<AnimationTimelineComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void AnimationTimelineComponentInfo::CopyTo(ActorComponentInfo* info)
	{
		if (info == nullptr)
			return;
		AnimationTimelineComponentInfo* newObjectInfo = static_cast<AnimationTimelineComponentInfo*>(info);
		if (newObjectInfo == nullptr)
			return;
		reflect::TypeResolver<AnimationTimelineComponentInfo>::get()->copyTo(this, newObjectInfo);
	}

	void AnimationTimelineComponentInfo::Overlay(ActorComponentInfo* srcInfo)
	{
	}

	void AnimationTimelineComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<AnimationTimelineComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}


#ifdef ECHO_EDITOR
	bool AnimationTimelineComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		if (mFilePath.empty())
			return false;
		resMap.insert(std::make_pair(ResourceType::NeedCopyRes, mFilePath));
		_parseCutSceneFile(resMap, mFilePath);
		return true;
	}

	void AnimationTimelineComponentInfo::_parseCutSceneFile(std::unordered_multimap<ResourceType, std::string>& resMap, const std::string& filePath)
	{
		if (filePath.empty())
			return ;

		DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(filePath.c_str()));
		if (pDataStream.isNull())
			return;

		size_t nSize = pDataStream->size();
		char* pData = new char[nSize + 1];
		memset(pData, 0, nSize + 1);
		pDataStream->read(pData, nSize);
		cJSON* pRoot = cJSON_Parse(pData);
		delete[] pData;
		if (pRoot == nullptr)
			return;

		std::string version_;

		ReadJSNode(pRoot, "Ver", version_);
		cJSON* AnimationVec_ = cJSON_GetObjectItem(pRoot, "AnimationVec");
		if (AnimationVec_)
		{
			int AnimationVec_Sz = cJSON_GetArraySize(AnimationVec_);

			for (int i = 0; i < AnimationVec_Sz; ++i)
			{
				cJSON* pAnimationChild = cJSON_GetArrayItem(AnimationVec_, i);
				if (pAnimationChild)
				{
					int AnimationChild_Sz = cJSON_GetArraySize(pAnimationChild);
					for (int j = 0; j < AnimationChild_Sz; ++j)
					{
						cJSON* AnimationChildData = cJSON_GetArrayItem(pAnimationChild, j);
						cJSON* animationKeyItem_ = cJSON_GetObjectItem(AnimationChildData, "AnimaitonKeyItem");

						if (animationKeyItem_)
						{
							int animationKeyItem_Sz = cJSON_GetArraySize(animationKeyItem_);

							for (int k = 0; k < animationKeyItem_Sz; ++k)
							{
								cJSON* keyValue_ = cJSON_GetArrayItem(animationKeyItem_, k);
								if (keyValue_)
								{
									std::string actPath_ = "";
									if (version_ == "1.1")
									{
										ReadJSNode(keyValue_, "Value", actPath_);
									}
									else if (version_ == "1.2")
									{
										ReadJSNode(keyValue_, "stringvalue", actPath_);
									}
									if (!actPath_.empty())
									{
										resMap.insert(std::make_pair(ResourceType::Actor, actPath_));
									}
								}

							}
						}
					}
				}
			}
		}
								
		cJSON_Delete(pRoot);
	}
#endif

	AnimationTimelineComponent::AnimationTimelineComponent(EchoWeakPtr<Actor> pActor, ActorCOMMoveableType isMoveable)
		:ProtocolActorComponent(pActor, isMoveable)
	{
		if (!pActor.expired())
			pActor->m_IsAnimationTimelineActor = true;
		pActor->setChildActorIsAnimationTimeline();
	}

	AnimationTimelineComponent::~AnimationTimelineComponent()
	{
	}


	void AnimationTimelineComponent::init(ActorComponentInfo* info)
	{
		setPriority(20000);
		ProtocolActorComponent::init(info);

		if (info)
		{
			mFilePath = static_cast<AnimationTimelineComponentInfo*>(info)->mFilePath;
			m_TickSpeed = static_cast<AnimationTimelineComponentInfo*>(info)->m_TickSpeed;
		}

	}

	void AnimationTimelineComponent::connectCom()
	{
		if (!mFilePath.empty())
		{
			bool loadState = false;
			if (m_pActor)
				loadState = m_pActor->isSyncLoadActor();
			loadFile(mFilePath.data(), loadState);
		}
	}

	void AnimationTimelineComponent::unInit()
	{
#ifdef ECHO_EDITOR
		CallProtocolFunc(PFID_DeleteAniTimelineCom, this, ProtocolHandle(0));
#endif // 
		_clearOldData();
	}

	void AnimationTimelineComponent::registerTickTimeBegin()
	{
		m_eventSet[TT_PrePhysicsUpdate].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}

	void AnimationTimelineComponent::prePhysicsUpdate(EventID eventId, ProtocolHandle)
	{

		_updateLoadPercent();

		if (!m_LoadObjectSet.empty() && m_bUseLoadState)
		{
			m_LoadObjectTickSecond += Echo::Root::instance()->getTimeSinceLastFrameClient();
			//超时机制
			if (m_LoadObjectTickSecond >= m_LoadTimeoutData)
			{
				_loadFinish();
			}
		}

		switch (eventId)
		{
			case EID_EveryFrame:
			{
				if (!mLoadState && m_bUseLoadState)
					break;
				if (mLoadState && !mFilmListenerVec.empty())
				{
					_clearFilmListener();
				}

				if(mIsPlaying)
				{
					mCurrTime += Root::instance()->getTimeSinceLastFrameClient() * m_TickSpeed;
					if (mCurrTime > mContext.mEndTime)
						mCurrTime = mContext.mEndTime;

					updateAnimationData();

					if (mCurrTime >= mContext.mEndTime)
						setIsPlay(false);
#ifdef ECHO_EDITOR
					setIsEditorTick(false);
#endif
				}


				break;
			}
			default: 
				break;
		}
	}

	void AnimationTimelineComponent::resourceArrived(const Name& resName, const Name& resType, bool bFail, const ResourceBasePtr& resourcePtr)
	{
		if (resourcePtr.isNull())
		{
			if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
			{
				LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, "clip  Timeline :File not exit: " + resName.toString());
			}
			return;
		}
		mAnimationPtr = static_cast<AnimationTimelineInfoPtr>(resourcePtr);
		mFilePath = resName.toString();
		resetAnimationData(mAnimationPtr);
		_initAnimationData();

	}

	void AnimationTimelineComponent::resetAnimationData(AnimationTimelineInfoPtr aniPtr_)
	{
		if (aniPtr_.isNull())
			return;
		mAnimationPtr = aniPtr_;
		mFilePath = aniPtr_->getName().c_str();
		mContext = aniPtr_->mDataHandle;

		mIdxToPtr.resize(mContext.mClipType.size());
		mIdxToComPtr.resize(mContext.mClipType.size(),nullptr);
#ifdef ECHO_EDITOR
		if (mContext.mClipName.size() != mContext.mAnimationVec.size())
		{
			mContext.mClipName.resize(mContext.mAnimationVec.size(), "Clip Track");
		}
#endif

	}

	void AnimationTimelineComponent::resetAnimationInfoData(AnimationTimelineInfoPtr aniPtr_)
	{
		if (aniPtr_.isNull())
			return;
		for (int i = 0; i < mIdxToComPtr.size(); ++i)
		{
			if (!mIdxToComPtr[i].expired() && i < mContext.mClipType.size())
				mContext.mClipType[i].second = mIdxToComPtr[i]->getComNameToString();
		}
		aniPtr_->mDataHandle = mContext;
	}

	void AnimationTimelineComponent::loadFile(const std::string& filePath_, bool synchronous)
	{
		_clearOldData();

		if (synchronous)
		{
			try
			{
				mAnimationPtr = AnimationTimeLineInfoManager::instance()->load(Name(filePath_));
			}
			catch (...)
			{
				mAnimationPtr = AnimationTimeLineInfoManager::instance()->createManual(Name(filePath_));
				if (mAnimationPtr.isNull())
					LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, "clip Timeline :File not exit: " + filePath_);
				else
				{
					mAnimationPtr->load();
				}
			}
		
			mFilePath = filePath_;
			resetAnimationData(mAnimationPtr);
			_initAnimationData();
		}
		else
		{
			auto pResource = AnimationTimeLineInfoManager::instance()->getByName(Name(filePath_));
			if (!pResource.isNull())
			{
				if (pResource->isLoaded())
				{
					mAnimationPtr = static_cast<AnimationTimelineInfoPtr>(pResource);
					mFilePath = filePath_;
					resetAnimationData(mAnimationPtr);
					_initAnimationData();
				}
				else if (pResource->isPrepared())
				{
					pResource->load();
					mAnimationPtr = static_cast<AnimationTimelineInfoPtr>(pResource);
					mFilePath = filePath_;
					resetAnimationData(mAnimationPtr);
					_initAnimationData();
				}
				else
				{
					ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_ENTITY);
					requestResouce(Name(filePath_), AnimationTimeLineInfoManager::instance()->getResourceType(), priority);
				}
			}
			else
			{
				ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_ENTITY);
				requestResouce(Name(filePath_), AnimationTimeLineInfoManager::instance()->getResourceType(), priority);
			}
		}
	}

	void AnimationTimelineComponent::saveFile(const std::string& filePath_)
	{
		if (filePath_.empty())
			return;

		ResourceBasePtr tempInfo = AnimationTimeLineInfoManager::instance()->load(Name(filePath_));
		if (tempInfo.isNull())
			mAnimationPtr = AnimationTimeLineInfoManager::instance()->createManual(Echo::Name(filePath_));
		else
			mAnimationPtr = tempInfo;

		resetAnimationInfoData(mAnimationPtr);
		mAnimationPtr->saveFile(filePath_);
	}

	void AnimationTimelineComponent::initFilmActorData(ActorPtr actorPtr_, int idx, int frame)
	{
		if (actorPtr_.expired())
			return;

		float maxLength_ = 0.f;
		const std::vector<ActorComponentPtr>& comVec_  =  actorPtr_->getComponentsByType(TimelineComponent::mType);
	    
		for (int i = 0; i < comVec_.size(); ++i)
		{
			mActorToCom[actorPtr_].insert(comVec_[i]);
			if (!comVec_[i].expired())
				maxLength_ = std::max(static_cast<TimelineComponent*>(comVec_[i].getPointer())->getTimelineTimeLength(), maxLength_);
		}

		std::vector<ActorPtr> actorVec_;

		actorPtr_->getAllChildActors(actorVec_);

		for (int i = 0; i < actorVec_.size(); ++i)
		{
			const std::vector<ActorComponentPtr>& childComVec_  = actorVec_[i]->getComponentsByType(TimelineComponent::mType);
			for (int j = 0; j < childComVec_.size(); ++j)
			{
				mActorToCom[actorPtr_].insert(childComVec_[j]);
				if (!childComVec_[j].expired())
					maxLength_ = std::max(static_cast<TimelineComponent*>(childComVec_[j].getPointer())->getTimelineTimeLength(), maxLength_);
			}		
		}


		setKeyValue(idx, frame, maxLength_);
#ifdef ECHO_EDITOR
		mActorToClipData[actorPtr_].mRealLength = maxLength_;
		mActorToClipData[actorPtr_].mSize = static_cast<int>(mActorToCom[actorPtr_].size());

#endif
	}

	void AnimationTimelineComponent::setFilmListenerState(filmActorListener* filmListener_, bool state_)
	{
		if (mFilmListenerVec.count(filmListener_) != 0)
			mFilmListenerVec[filmListener_] = state_;

		/*bool loadState_ = true;
		for (const auto& iter : mFilmListenerVec)
			loadState_ &= iter.second;

		mLoadState = loadState_;*/
	}

	void AnimationTimelineComponent::setIsPlay(bool isPlay)
	{
		mIsPlaying = isPlay;
		if (mCurrTime == mContext.mEndTime || mCurrTime < mContext.mStartTime)
			mCurrTime = mContext.mStartTime;

		if (!isPlay)
		{
			for (clipLoadListener* listener_ : mLoadListenerVec)
			{
				if (listener_)
					listener_->onClipPlayEnd();
			}

			for (std::pair<const ActorPtr, std::set<ActorComponentPtr>>& actcoms : mActorToCom)
				for (auto& coms : actcoms.second)
					if (!coms.expired() && coms->getVisibleState())
						coms.getStaticCastWeakPtr<TimelineComponent>()->playEnd();
		}
	}

	void AnimationTimelineComponent::updateAnimationData()
	{
		for ( int i = 0 ; i < mContext.mAnimationVec.size() ; ++ i )
		{
			if (!mContext.mAnimationVec[i])
				continue;
			if (mContext.mAnimationVec[i]->mHermiteType == EchoHermite::String_Hermite)
			{
				for (const auto& keyIter : mContext.mAnimationVec[i]->mKeyMap)
				{
					if (!keyIter.second)
						continue;
					HermiteStringKey* key_ = static_cast<HermiteStringKey*>(keyIter.second);
					_updateTimelineGroupData(i ,key_->value, keyIter.first, key_->mLength, key_->mStartTime);
				}
			}

		}
	}

	void AnimationTimelineComponent::setKeyValue(int idx, int frame, const std::string& value_, float startTime, float length, bool isAsync)
	{
		if (idx >= mContext.mAnimationVec.size() || !mContext.mAnimationVec[idx])
			return;
		
		if (!mContext.mAnimationVec[idx]->mKeyMap.contains(frame))
			return;

		HermiteStringKey* key_ = static_cast<HermiteStringKey*>(mContext.mAnimationVec[idx]->mKeyMap[frame]);

		if (!key_)
			return;

		bool isLoad = key_->value != value_;
		key_->value = value_;
		key_->mStartTime = startTime;
		key_->mLength = length;

		if(isLoad && _judgeIsActorClipKey(idx))
	 	   _loadActorData(idx , frame, value_, isAsync);
	}

	void AnimationTimelineComponent::setKeyValue(int idx, int frame,  float realLength)
	{
		if (idx >= mContext.mAnimationVec.size() || !mContext.mAnimationVec[idx])
			return;

		if (!mContext.mAnimationVec[idx]->mKeyMap.contains(frame))
			return;

		HermiteStringKey* key_ = static_cast<HermiteStringKey*>(mContext.mAnimationVec[idx]->mKeyMap[frame]);

		if (!key_)
			return;
	
		key_->mRealLength = realLength;
	}


	void    AnimationTimelineComponent::setKeyValue(int idx, int frame, float startTime, float length)
	{
		if (idx >= mContext.mAnimationVec.size() || !mContext.mAnimationVec[idx])
			return;

		if (!mContext.mAnimationVec[idx]->mKeyMap.contains(frame))
			return;

		HermiteStringKey* key_ = static_cast<HermiteStringKey*>(mContext.mAnimationVec[idx]->mKeyMap[frame]);

		if (!key_)
			return;

		key_->mLength = length;
		key_->mStartTime = startTime;
	}

	void AnimationTimelineComponent::setKeyLength(int idx, int frame, float length)
	{
		if (idx >= mContext.mAnimationVec.size() || !mContext.mAnimationVec[idx])
			return;

		std::map<int, Echo::HermiteKey*>::iterator  iter = mContext.mAnimationVec[idx]->mKeyMap.find(frame);

		if (iter == mContext.mAnimationVec[idx]->mKeyMap.end())
			return;

		++iter;

		if (iter == mContext.mAnimationVec[idx]->mKeyMap.end())
		{
			HermiteStringKey* key_ = static_cast<HermiteStringKey*>(mContext.mAnimationVec[idx]->mKeyMap[frame]);
			if (!key_)
				return;
			key_->mLength = length;
		}
		else
		{
			HermiteStringKey* key_ = static_cast<HermiteStringKey*>(mContext.mAnimationVec[idx]->mKeyMap[frame]);
			int nextFrame_ = iter->first;
			int frameLength_ = getFrameByTime(length);

			if (frame + frameLength_ > nextFrame_)
			{
				frameLength_ = nextFrame_ - frame;
				length = getTimeByFrame(frameLength_);
			}
			key_->mLength = length;
		}
	}

	void AnimationTimelineComponent::copyKeyRealLengthToLength(int idx, int frame)
	{
		if (idx >= mContext.mAnimationVec.size() || !mContext.mAnimationVec[idx])
			return;

		if (!mContext.mAnimationVec[idx]->mKeyMap.contains(frame))
			return;

		HermiteStringKey* key_ = static_cast<HermiteStringKey*>(mContext.mAnimationVec[idx]->mKeyMap[frame]);

		if (!key_)
			return;

		key_->mLength = key_->mRealLength;
	}

	void AnimationTimelineComponent::createKey(int idx, int frame)
	{
		if (idx >= mContext.mAnimationVec.size() || !mContext.mAnimationVec[idx])
			return;

		if (mContext.mAnimationVec[idx]->mKeyMap.contains(frame))
			return;

		if (mContext.mAnimationVec[idx]->mHermiteType == EchoHermite::String_Hermite)
		{
			HermiteStringKey* key_ = static_cast<HermiteStringKey*>(mContext.mAnimationVec[idx]->CreateKey());
			mContext.mAnimationVec[idx]->mKeyMap[frame] = key_;
		}

	}

	void AnimationTimelineComponent::getKey(int idx, int frame, std::string& value, float& startTime, float& length)
	{
		if (idx >= mContext.mAnimationVec.size() || !mContext.mAnimationVec[idx])
			return;

		if (!mContext.mAnimationVec[idx]->mKeyMap.contains(frame))
			return;

		if (mContext.mAnimationVec[idx]->mHermiteType == EchoHermite::String_Hermite)
		{
			HermiteStringKey* key_ = static_cast<HermiteStringKey*>(mContext.mAnimationVec[idx]->mKeyMap[frame]);
			value = key_->value;
			startTime = key_->mStartTime;
			length = key_->mLength;
		}
	}

	int AnimationTimelineComponent::copyKey(int oriIdx, int oriFrame, int resultIdx, int resultFrame)
	{
		if (oriIdx >= mContext.mAnimationVec.size() || !mContext.mAnimationVec[oriIdx])
			return resultFrame;
		if (resultIdx >= mContext.mAnimationVec.size() || !mContext.mAnimationVec[resultIdx])
			return resultFrame;

		if (!mContext.mAnimationVec[oriIdx]->mKeyMap.contains(oriFrame))
			return resultFrame;

		std::map<int, Echo::HermiteKey*>::iterator nextIter_ = mContext.mAnimationVec[resultIdx]->mKeyMap.lower_bound(resultFrame);

		if (nextIter_ != mContext.mAnimationVec[resultIdx]->mKeyMap.begin())
			--nextIter_;

		HermiteStringKey* preKey =  static_cast<HermiteStringKey*>(nextIter_->second);
		if (preKey)
		{
			int preFrame_ = nextIter_->first;

			int frameLength_ = getFrameByTime(preKey->mLength);

			HermiteStringKey* oriKey = static_cast<HermiteStringKey*>(mContext.mAnimationVec[oriIdx]->mKeyMap[oriFrame]);

			if(preFrame_ + frameLength_ > resultFrame )
			{
				const auto& endIter_ = mContext.mAnimationVec[resultIdx]->mKeyMap.rbegin();
				
				if (!endIter_->second)
					return resultFrame;

				int endFrame = endIter_->first;
				
				HermiteStringKey* endKey_ = static_cast<HermiteStringKey*>(endIter_->second);

				int endLength = getFrameByTime(endKey_->mLength);

				resultFrame = endFrame + endLength;
			}
			_addKey(oriKey, EchoHermite::String_Hermite, resultIdx, resultFrame);

			if(_judgeIsActorClipKey(resultIdx) && !oriKey->value.empty())
			   _loadActorData(resultIdx,resultFrame, oriKey->value);
		}
		
		return resultFrame;
	}

	void AnimationTimelineComponent::deleteKey(int idx, int frame)
	{
		if (idx >= mContext.mAnimationVec.size() || !mContext.mAnimationVec[idx])
			return;

		if (!mContext.mAnimationVec[idx]->mKeyMap.contains(frame))
			return;

		HermiteStringKey* key_ = static_cast<HermiteStringKey*>(mContext.mAnimationVec[idx]->mKeyMap[frame]);

		if (key_)
			delete key_;
		key_ = nullptr;

		mContext.mAnimationVec[idx]->mKeyMap.erase(frame);
		if (_judgeIsActorClipKey(idx))
		   _deleteIdxToActorKey(idx,frame);
	}

	bool AnimationTimelineComponent::forcedMoveKey(int oriIdx, int oriFrame, int resultIdx, int resultFrame)
	{
		EchoHermite* hermite_ = mContext.mAnimationVec[resultIdx];
		EchoHermite* oriHermite_ = mContext.mAnimationVec[oriIdx];

		HermiteStringKey* oriKey = static_cast<HermiteStringKey*>(oriHermite_->mKeyMap[oriFrame]);

		HermiteStringKey* newKey_ =  static_cast<HermiteStringKey*>(hermite_->CreateKey());
		oriKey->Copy(newKey_);

		delete oriKey;
		oriKey = nullptr;

		oriHermite_->mKeyMap.erase(oriFrame);
		hermite_->mKeyMap[resultFrame] = newKey_;

		if (_judgeIsActorClipKey(oriIdx))
			_moveIdxToActorKey(oriIdx, oriFrame, resultIdx, resultFrame);
		return true;
	}

	// oriFrame : 被点选的起始帧数		resultFrame : 移动结束后的帧数
	// oriIdx	: 被点选处于第几轨道		resultIdx	: 移动结束后的处于第几轨道
	bool AnimationTimelineComponent::moveKey(int oriIdx, int oriFrame, int resultIdx, int resultFrame)
	{
		if (oriIdx == resultIdx && oriFrame == resultFrame)
			return false;

		if (resultIdx >= mContext.mAnimationVec.size())
			return false;

		if (resultIdx < mContext.mClipType.size() && oriIdx < mContext.mClipType.size() && mContext.mClipType[oriIdx].first != mContext.mClipType[resultIdx].first)
			return false;

		EchoHermite* hermite_ =  mContext.mAnimationVec[resultIdx];

		EchoHermite* oriHermite_ = mContext.mAnimationVec[oriIdx];
		if (!hermite_ || !oriHermite_)
			return false;


		if (!oriHermite_->mKeyMap.contains(oriFrame))
			return false;

		if (oriHermite_->mHermiteType == EchoHermite::String_Hermite && hermite_->mHermiteType == EchoHermite::String_Hermite)
		{
			bool isLeftOK = false;		// 判断是否触发左移动操作
			bool isRightOk = false;		// 判断是否触发右移动操作

			std::map<int, Echo::HermiteKey*>::iterator nextIter_ = hermite_->mKeyMap.lower_bound(resultFrame);

			int nextFrame = INT32_MAX;
			if (nextIter_ != hermite_->mKeyMap.end())
				nextFrame = nextIter_->first;

			if (nextFrame == oriFrame && oriIdx == resultIdx)
				isRightOk = true;
			
			HermiteStringKey* oriKey = static_cast<HermiteStringKey*>(oriHermite_->mKeyMap[oriFrame]);
			if (!isRightOk)
			{
				int currLength = static_cast<int>(oriKey->mLength * mContext.mDeltaTime + 0.5f);
				if (resultFrame + currLength <= nextFrame)
					isRightOk = true;
			}


			if (nextIter_ != hermite_->mKeyMap.begin())
				--nextIter_;
			else
				isLeftOK = true;

			if (!isLeftOK)
			{
				int preframe = nextIter_->first;
				HermiteStringKey* preKey_ = static_cast<HermiteStringKey*>(nextIter_->second);
				int preLength = static_cast<int>(preKey_->mLength * mContext.mDeltaTime + 0.5f);
				if (preframe == oriFrame && oriIdx == resultIdx)
					isLeftOK = true;
				else
				{
					//上一个key的片段尾
					preframe = preframe + preLength;
					if (resultFrame >= preframe)
						isLeftOK = true;
				}
			}

			if (!isRightOk || !isLeftOK)
				return false;

			return forcedMoveKey(oriIdx, oriFrame, resultIdx, resultFrame);
		}  

		return false;
	}

	bool AnimationTimelineComponent::calculateProximityKey(float frameLen, int oriIdx, int oriFrame, int resultIdx, int& resultFrame, int& direction, std::set<int>& noJudged, bool noChange)
	{
		if (oriIdx == resultIdx && oriFrame == resultFrame)
			return true;

		if (resultIdx >= mContext.mAnimationVec.size())
			return false;

		if (resultIdx < mContext.mClipType.size() && oriIdx < mContext.mClipType.size() && mContext.mClipType[oriIdx].first != mContext.mClipType[resultIdx].first)
			return false;

		EchoHermite* hermite_ = mContext.mAnimationVec[resultIdx];

		EchoHermite* oriHermite_ = mContext.mAnimationVec[oriIdx];
		if (!hermite_ || !oriHermite_)
			return false;

		if (!oriHermite_->mKeyMap.contains(oriFrame))
			return false;

		if (hermite_->mKeyMap.empty())
			return true;

		if (oriHermite_->mHermiteType == EchoHermite::String_Hermite && hermite_->mHermiteType == EchoHermite::String_Hermite)
		{
			auto discriminant = [&](std::map<int, Echo::HermiteKey*>::iterator& Iter) 
			{
				if (noJudged.empty())
					return false;

				if (noJudged.find(Iter->first) != noJudged.end())
					return true;

				return false;
			};

			HermiteStringKey* pStr = reinterpret_cast<HermiteStringKey*>(oriHermite_->mKeyMap[oriFrame]);
			int oriLength = static_cast<int>(pStr->mLength * mContext.mDeltaTime + 0.5f);

			int beginFrame = resultFrame;
			int endFrame = resultFrame + oriLength;

			std::map<int, Echo::HermiteKey*>::iterator iter = hermite_->mKeyMap.lower_bound(resultFrame);

			if (iter != hermite_->mKeyMap.end())
				while (discriminant(iter))
				{
					++iter;
					if (iter == hermite_->mKeyMap.end())
						break;
				}

			int end = 0;
			int offset = static_cast<int>(5.f / frameLen + 0.5f);
			bool isLeft = true;

			bool isContains = false;
			if (iter != hermite_->mKeyMap.end())
			{
				end = iter->first;

				if (endFrame > end)
					isContains = true;
				else
				{
					if (end - endFrame < offset)
					{
						offset = end - endFrame;
						isLeft = false;
					}
				}
			}

			bool needDetermine = true;

			if (iter != hermite_->mKeyMap.begin())
				--iter;
			else if (discriminant(iter) || iter->first > resultFrame)
				needDetermine = false;

			while (discriminant(iter))
			{
				if (iter == hermite_->mKeyMap.begin())
				{
					needDetermine = false;
					break;
				}
				else
					--iter;
			}

			if (needDetermine)
			{
				pStr = reinterpret_cast<HermiteStringKey*>(iter->second);

				end = iter->first + static_cast<int>(pStr->mLength * mContext.mDeltaTime + 0.5f);

				if (end > beginFrame)
					isContains = true;
				else
					offset = offset > beginFrame - end ? beginFrame - end : offset;
			}

			if (!isContains && offset < static_cast<int>(5.f / frameLen + 0.5f))
			{
				if(!noChange) resultFrame += (isLeft ? -offset : offset);
				direction = isLeft ? -1 : 1;
			}

			if (isContains)
				return false;
		}
		return true;
	}

	void AnimationTimelineComponent::clipKey(int idx, int keyFrame, int currFrame)
	{
		if (currFrame < keyFrame)
			return;

		if (idx >= mContext.mAnimationVec.size() || !mContext.mAnimationVec[idx])
			return;

		if (!mContext.mAnimationVec[idx]->mKeyMap.contains(keyFrame))
			return;

		HermiteStringKey* key_ = static_cast<HermiteStringKey*>(mContext.mAnimationVec[idx]->mKeyMap[keyFrame]);

		if (!key_)
			return;

		float diffLength = (currFrame - keyFrame) / mContext.mDeltaTime;

		if (diffLength >= key_->mLength || diffLength <= 0.f)
			return;

		float newKeyLength = key_->mLength - diffLength;

		key_->mLength = diffLength;

		HermiteStringKey* newKey = static_cast<HermiteStringKey*>(mContext.mAnimationVec[idx]->CreateKey());
		key_->Copy(newKey);

		newKey->mLength = newKeyLength;
		newKey->mStartTime = diffLength;
		mContext.mAnimationVec[idx]->mKeyMap[currFrame] = newKey;
		
		if(_judgeIsActorClipKey(idx))
		   _loadActorData(idx, currFrame, newKey->value);

	}

	void AnimationTimelineComponent::createClip(int type_)
	{
		EchoHermite::HermiteType hermiteType_ = static_cast<EchoHermite::HermiteType>(type_);
		if (hermiteType_ == EchoHermite::String_Hermite)
		{
			mContext.mAnimationVec.emplace_back(new EchoStringHermite());
			mContext.mClipType.emplace_back(0, "");
			
			//处理指针相关容器
			mIdxToPtr.emplace_back(std::map<int, ActorPtr>());
			mIdxToComPtr.emplace_back(nullptr);
		}

#ifdef ECHO_EDITOR
		mContext.mClipName.emplace_back("Clip Track");
#endif
	}

	void AnimationTimelineComponent::createClip(int clipType, const ActorComponentPtr& comPtr, const std::string& clipName, const Name& hermiteType)
	{
		EchoHermiteFactory* factory_ = EchoHermiteFactortManager::getSingleton().getHermiteFactory(hermiteType);
		if (!factory_)
		{
			assert(!hermiteType.c_str());
			return;
		}
		mContext.mAnimationVec.emplace_back(factory_->createHermite());
		mContext.mClipType.emplace_back(clipType,comPtr->getComNameToString());

		//处理指针相关容器
		mIdxToPtr.emplace_back(std::map<int, ActorPtr>());
		mIdxToComPtr.emplace_back( comPtr);

#ifdef ECHO_EDITOR
		mContext.mClipName.emplace_back(clipName);
#endif
	}

	void AnimationTimelineComponent::deleteClip(int idx)
	{
		if (idx >= mContext.mAnimationVec.size())
			return;

		if(idx < mContext.mClipType.size())
	    	mContext.mClipType.erase(mContext.mClipType.begin() + idx);

		if (!mContext.mAnimationVec[idx])
		{
			mContext.mAnimationVec.erase(mContext.mAnimationVec.begin() + idx);
			return;
		}

		for (auto& iter : mContext.mAnimationVec[idx]->mKeyMap)
		{
			if (iter.second)
				delete  iter.second;
			iter.second = nullptr;
		}
		mContext.mAnimationVec[idx]->mKeyMap.clear();
		delete mContext.mAnimationVec[idx];
		mContext.mAnimationVec[idx] = nullptr;
		mContext.mAnimationVec.erase(mContext.mAnimationVec.begin() + idx);
       
		if (idx < mIdxToComPtr.size())
		{
			m_pActor->delComponent(mIdxToComPtr[idx]);
			mIdxToComPtr.erase(mIdxToComPtr.begin() + idx);
		}

		_deleteIdxToActorRow(idx);


#ifdef ECHO_EDITOR
       if(idx < mContext.mClipName.size())
		  mContext.mClipName.erase(mContext.mClipName.begin() + idx);
#endif

	}

	void AnimationTimelineComponent::resetCreateClipIdx(int idx)
	{
		if (idx >= mIdxToPtr.size()) return;

		auto resetIdx = [idx](auto& vec)
		{
			auto iter = vec.begin();
			if (idx != 0)
				iter = std::next(vec.begin() + idx - 1);
			std::reverse(iter, vec.end());
			std::reverse(std::next(iter), vec.end());
		};

		resetIdx(mContext.mAnimationVec);
		resetIdx(mContext.mClipType);
		if (!mIdxToPtr[idx].empty() || !mIdxToPtr.rbegin()->empty())
			resetIdx(mIdxToPtr);
		if (mIdxToComPtr[idx] || *mIdxToComPtr.rbegin())
			resetIdx(mIdxToComPtr);
#ifdef ECHO_EDITOR
		resetIdx(mContext.mClipName);
#endif
	}

	int AnimationTimelineComponent::getFrameByTime(float time_)
	{
		return static_cast<int>(time_ * mContext.mDeltaTime + 0.5f);
	}

	float AnimationTimelineComponent::getTimeByFrame(int frame_)
	{
		return static_cast<float>(frame_) / mContext.mDeltaTime;
	}

	void AnimationTimelineComponent::setCurrentTimeByFrame(int frame)
	{
		mCurrTime = static_cast<float>(frame) / mContext.mDeltaTime;
	}

	void AnimationTimelineComponent::setStartTimeByFrame(int frame)
	{
		mContext.mStartTime  = static_cast<float>(frame) / mContext.mDeltaTime;
	}

	void AnimationTimelineComponent::setEndTimeByFrame(int frame)
	{
		mContext.mEndTime = static_cast<float>(frame) / mContext.mDeltaTime;
		mContext.mEndTime = std::max(mContext.mEndTime,mContext.mStartTime + 1.f/ mContext.mDeltaTime);
	}

	bool AnimationTimelineComponent::getHermiteKeyData(int idx, int frame, float& startTime, float& length)
	{
		if (idx >= mContext.mAnimationVec.size())
			return false;
		if (!mContext.mAnimationVec[idx])
		{
			assert(!"AnimationTimelineComponent::getHermiteKeyData idx Error");
			return false;
		}
		if (mContext.mAnimationVec[idx]->mKeyMap.contains(frame))
		{
			HermiteStringKey* key = static_cast<HermiteStringKey*> (mContext.mAnimationVec[idx]->mKeyMap[frame]);
			startTime = key->mStartTime;
			length = key->mLength;
			return true;
		}
		return false;
			
	}

	bool AnimationTimelineComponent::setHermiteKeyStartTime(int idx, int frame, float startTime)
	{
		if (idx >= mContext.mAnimationVec.size())
			return false;
		if (!mContext.mAnimationVec[idx])
		{
			assert(!"AnimationTimelineComponent::setHermiteKeyStartTime idx Error");
			return false;
		}
		if (mContext.mAnimationVec[idx]->mKeyMap.contains(frame))
		{
			HermiteStringKey* key = static_cast<HermiteStringKey*> (mContext.mAnimationVec[idx]->mKeyMap[frame]);
			key->mStartTime = startTime;
			return true;
		}
		return false;
	}

	HermiteKey* const AnimationTimelineComponent::getHermiteKey(int idx, int frame)
	{
		if (idx >= mContext.mAnimationVec.size() || !mContext.mAnimationVec[idx] || !mContext.mAnimationVec[idx]->mKeyMap.contains(frame))
		   return nullptr;
		return mContext.mAnimationVec[idx]->mKeyMap[frame];
	}

	void AnimationTimelineComponent::refreshTimelineComData()
	{

		for (auto& actIter_ : mActorToCom)
		{
			for (auto& comIter : actIter_.second)
			{
				if (comIter.expired())
					_initTimelineComList(actIter_.first);
			}
		}

		for (const auto& actorIter : mActorToCom)
		{
			if (actorIter.first.expired())
				continue;

			for (const ActorComponentPtr& comIter : actorIter.second)
			{
				if (comIter.expired())
					continue;
				TimelineComponent* timCom = static_cast<TimelineComponent*>(comIter.get());
				timCom->refreshExitAttributeActor();
				timCom->refreshExitAttributeCom();
			}
		}

		for (int i = 0 ; i < mIdxToComPtr.size() ; ++ i)
		{
			if (mIdxToComPtr[i].expired() && i < mContext.mClipType.size() && mContext.mClipType[i].first !=0)
			{
				mIdxToComPtr[i] = m_pActor->getComponent(mContext.mClipType[i].second);
			}
		}

	}

	void AnimationTimelineComponent::addClipLoadListener(clipLoadListener* listener_)
	{
		if (listener_)
		{
			std::vector<clipLoadListener*>::const_iterator iter = std::find(mLoadListenerVec.begin(), mLoadListenerVec.end(), listener_);

			if (iter == mLoadListenerVec.end())
			{
				mLoadListenerVec.push_back(listener_);
			}
		}
	}

	void AnimationTimelineComponent::deleteClioLoadListener(clipLoadListener* listener_)
	{
		if (listener_)
		{
			std::vector<clipLoadListener*>::const_iterator iter = std::find(mLoadListenerVec.begin(), mLoadListenerVec.end(), listener_);

			if (iter != mLoadListenerVec.end())
			{
				mLoadListenerVec.erase(iter);
			}
		}
	}

	void AnimationTimelineComponent::deleteClipLoadListener(clipLoadListener* listener_)
	{
		if (listener_)
		{
			std::vector<clipLoadListener*>::const_iterator iter = std::find(mLoadListenerVec.begin(), mLoadListenerVec.end(), listener_);

			if (iter != mLoadListenerVec.end())
			{
				mLoadListenerVec.erase(iter);
			}
		}
	}

	void AnimationTimelineComponent::registerTickFunc(int type, const AnimTickFunc& func)
	{
		if (!mTypeToTickFunc.contains(type))
			mTypeToTickFunc.insert(std::make_pair(type, func));
	}

	void AnimationTimelineComponent::unRegisterTickFunc(int type)
	{
		if (mTypeToTickFunc.contains(type))
			mTypeToTickFunc.erase(type);
	}

	void AnimationTimelineComponent::registerTickCom(const ActorComponentPtr& p_Com_)
	{
		if (p_Com_.expired())
			return;
		std::vector<ActorComponentPtr>::const_iterator  iter = std::find(m_TickVec.begin(), m_TickVec.end(), p_Com_);
		if (iter == m_TickVec.end())
		{
			m_TickVec.emplace_back(p_Com_);
		}
	}

	void AnimationTimelineComponent::unRegisterTickCom(const ActorComponentPtr& p_Com_)
	{
		if (p_Com_.expired())
			return;
		std::vector<ActorComponentPtr>::const_iterator  iter = std::find(m_TickVec.begin(), m_TickVec.end(), p_Com_);
		if (iter != m_TickVec.end())
		{
			m_TickVec.erase(iter);
		}
	}

	void AnimationTimelineComponent::registerTickComByActor(const ActorPtr& actorPtr_, const ActorComponentPtr& p_Com_)
	{
		if (actorPtr_.expired() || p_Com_.expired())
			return;
		const ActorComponentPtr& actCom_ = actorPtr_->getCutSceneCom();
		if (actCom_.expired())
			return;
		AnimationTimelineComponent* cutSceneCom_ = static_cast<AnimationTimelineComponent*>(actCom_.get());
		cutSceneCom_->registerTickCom(p_Com_);
		cutSceneCom_->_updateTickComSpeed(p_Com_);
	}

	void AnimationTimelineComponent::unRegisterTickComByActor(const ActorPtr& actorPtr_, const ActorComponentPtr& p_Com_)
	{
		if (actorPtr_.expired() || !p_Com_)
			return;
		const ActorComponentPtr& actCom_ = actorPtr_->getCutSceneCom();
		if (actCom_.expired())
			return;
		AnimationTimelineComponent* cutSceneCom_ = static_cast<AnimationTimelineComponent*>(actCom_.get());
		cutSceneCom_->unRegisterTickCom(p_Com_);
	}

	void AnimationTimelineComponent::updateTickComSpeed()
	{
		std::vector<ActorComponentPtr>::const_iterator iter = m_TickVec.begin();

		while (iter != m_TickVec.end())
		{
			if (iter->expired())
				iter = m_TickVec.erase(iter);
			else
			{
				_updateTickComSpeed(*iter);
				++iter;
			}
		}
	}

	int AnimationTimelineComponent::getClipTypeByIdx(int idx)
	{
		if(idx >= mContext.mClipType.size())
		   return 0;

		return mContext.mClipType[idx].first;
	}

	void AnimationTimelineComponent::setSpeed(float speed)
	{
		m_TickSpeed = speed;
		updateTickComSpeed();
	}

	int AnimationTimelineComponent::getLoadPercent()
	{
		int res = 0;

		if (m_CollectResourceCount > 0)
		{
			size_t count = m_CollectResourceCount - m_LoadObjectSet.size();
			res = int(1.0f * count / m_CollectResourceCount * 100.0f);
		}
		else
			res = 100;
		return res;
	}

	int AnimationTimelineComponent::getLoadPercent(LoadLevel level)
	{
		if (m_LoadObjectSet.size() == 0 || m_CollectResourceCount == 0)
			return 100;
		if (level == LoadLevel::Clip_Lv0)
		{
			return getLoadPercent();
		}
		else if (level == LoadLevel::Clip_Lv1)
		{
			double res_per =static_cast<double>(getLoadPercent());
			double single_per = 100.0 / m_CollectResourceCount;
			//debug
			//stringstream ss;

			for (const std::map<ProtocolHandle, LoadPercentData>::value_type& iter : m_LoadObjectSet)
			{
				res_per += iter.second.percent * single_per * 0.01;

				//ss << iter.first << " " << iter.second.count << " " << iter.second.percent << " " << iter.second.succCount << " " << iter.second.waitComs.size() << std::endl;
			}

		/*	ss << "unitPer" << single_per << std::endl;
			ss << "res_per" << res_per << std::endl;
			LogManager::instance()->logMessage(ss.str());*/

			return static_cast<int>(res_per);
		}
		return 0;
	}

	void AnimationTimelineComponent::resourceLoadFinish(ProtocolHandle handle)
	{
		/*stringstream ss;
		ss << "ClipActorLoadFinish" << handle;
		LogManager::instance()->logMessage(ss.str());*/

		m_LoadObjectSet.erase(handle);
		if (m_LoadObjectSet.empty())
			_loadFinish();
	}

	void AnimationTimelineComponent::addToTimelineList(const ActorComponentPtr& timelineCom_)
	{
		if (timelineCom_.expired())
			return;
		const ActorPtr& timelineActor_ = timelineCom_->getActor();
		if (timelineActor_.expired())
			return;

		bool isDeleteUntreatedActor = false;
		for (const ActorPtr& actor_ : m_UntreatedActor)
		{
			if (actor_.Expired() || timelineActor_ != actor_)
				continue;
			
			if (m_TimelineLoadList.count(timelineActor_) == 0)
			{
				const std::vector<ActorComponentPtr>& timelineVec_=   timelineActor_->getComponentsByType(TimelineComponent::mType);
				for (int i = 0; i < timelineVec_.size(); ++i)
					m_TimelineLoadList[timelineActor_].insert(std::make_pair(timelineVec_[i],false));
			}
			isDeleteUntreatedActor = true;
			break;
		}

		if(isDeleteUntreatedActor)
			m_UntreatedActor.erase(timelineActor_);

		addToLoadActorPercent(timelineActor_,timelineCom_);
	}

	void AnimationTimelineComponent::addToLoadActorPercent(const ActorPtr& actPtr_, const ActorComponentPtr& timelineCom_)
	{
		if (m_TimelineLoadList.count(actPtr_) == 0 || m_TimelineLoadList[actPtr_].count(timelineCom_) == 0)
			return;
	
		m_TimelineLoadList[actPtr_][timelineCom_] = true;

		bool isLoadFinish = true;
		for (const TimelineMap::value_type& iter : m_TimelineLoadList[actPtr_])
			isLoadFinish &= iter.second;
		if (!isLoadFinish)
			return;

		for (const TimelineMap::value_type& iter : m_TimelineLoadList[actPtr_])
		{
			if (iter.first.expired())
				continue;
			int count = static_cast<TimelineComponent*>(iter.first.get())->preLoadResource(ProtocolHandle(actPtr_.get()));

			EchoTSet<ActorComponentPtr>  waitComs;
			ActorHelper::getWaitFinishComs(actPtr_,waitComs,true);

			ProtocolHandle actHandle = ProtocolHandle(actPtr_.get());
			if (m_LoadObjectSet.count(actHandle) != 0)
			{
				m_LoadObjectSet[actHandle].count += count + static_cast<int>(waitComs.size());
				m_LoadObjectSet[actHandle].waitComs = waitComs;
			}
		}
		
	}

	void AnimationTimelineComponent::notifyTimelineLoadPercent(const ActorComponentPtr& timeCom_)
	{
		ProtocolHandle handle = 0;
		for (const std::map<ActorPtr, TimelineMap>::value_type& iter : m_TimelineLoadList)
		{
			if (iter.first.expired())
				continue;
			if (iter.second.count(timeCom_) != 0)
			{
				handle = ProtocolHandle(iter.first.get());
				break;
			}
		}

		if (m_LoadObjectSet.count(handle) == 0)
			return;
		m_LoadObjectSet[handle].succCount++;
		if (m_LoadObjectSet[handle].count == 0)
			m_LoadObjectSet[handle].percent = 99.0;
		else
			m_LoadObjectSet[handle].percent = static_cast<double>(m_LoadObjectSet[handle].succCount) / m_LoadObjectSet[handle].count;
		//该actor未从收集列表中移除，表示该actor未加载完成
		m_LoadObjectSet[handle].percent = std::min(99.0, m_LoadObjectSet[handle].percent);
	}

	void AnimationTimelineComponent::setIsUseLoadState(bool state_)
	{
		m_bUseLoadState = state_;
	}

	void AnimationTimelineComponent::setLoadTimeOutTime(float time_)
	{
		m_LoadTimeoutData = time_;
	}

#ifdef ECHO_EDITOR
	void AnimationTimelineComponent::getClipComByIdx(int idx, ActorComponentPtr& ptr_)
	{
		if (idx >= mIdxToComPtr.size())
			return;
		ptr_ = mIdxToComPtr[idx];
	}
	void AnimationTimelineComponent::getClipTypeNameByIdx(int idx, std::string& name)
	{
		if (idx >= mContext.mClipType.size())
			return ;

		name =  mContext.mClipType[idx].second;
	}

	void AnimationTimelineComponent::setHermiteNameByidx(int idx, const std::string& name_)
	{
		assert(idx < mContext.mClipName.size() && "clip animation : error index !!!");

		if (idx >= mContext.mClipName.size())
			return;

		mContext.mClipName[idx] = name_;

	}
	void AnimationTimelineComponent::setIsEditorTick(bool isEditor)
	{
		m_TickData.m_IsEditorOperation = isEditor;
	}

	void AnimationTimelineComponent::LoadActorData(int idx, int frame, const String& actorPath)
	{
		if (_judgeIsActorClipKey(idx) && !actorPath.empty())
			_loadActorData(idx, frame, actorPath);
	}

#endif

	void AnimationTimelineComponent::_addKey(HermiteKey* key_, int type_, int idx, int frame)
	{
		EchoHermite::HermiteType hermiteType_ = static_cast<EchoHermite::HermiteType>(type_);
		if (!key_ || idx >= mContext.mAnimationVec.size() || !mContext.mAnimationVec[idx])
			return;

		if (hermiteType_ == EchoHermite::String_Hermite)
		{
			HermiteStringKey* newKey_ = static_cast<HermiteStringKey*>(mContext.mAnimationVec[idx]->CreateKey());
			static_cast<HermiteStringKey*>(key_)->Copy(newKey_);
			mContext.mAnimationVec[idx]->mKeyMap.insert(std::make_pair(frame,newKey_));
		}
	}

	void AnimationTimelineComponent::_initAnimationData()
	{
		for (int i = 0; i < mContext.mAnimationVec.size(); ++i)
		{
			if (!mContext.mAnimationVec[i])
				continue;

			if (i >= mContext.mClipType.size())
			{
				assert(!" out of clipType size ");
				continue;
			}


			if (mContext.mClipType[i].first == 0)
			{
				for (const auto& iter : mContext.mAnimationVec[i]->mKeyMap)
				{
					if (!iter.second)
						continue;
					HermiteStringKey* key_ = static_cast<HermiteStringKey*>(iter.second);
					if (!key_)
						continue;
				    _loadActorData(i, iter.first, key_->value);
				}
			}
			else
			{
				_loadComData(i);
			}
		}
	}

	void AnimationTimelineComponent::_clearFilmListener()
	{
		for (auto& iter : mFilmListenerVec)
		{
			if (!iter.first)
				continue;
			ActorPtr actorPtr_ = iter.first->getActor();
			if(!actorPtr_.expired())
			   actorPtr_->removeActorLoadListener(iter.first);
			if (iter.first)
				delete iter.first;
		}
		mFilmListenerVec.clear();

        //加载完成的回调
		for (clipLoadListener* listener_ : mLoadListenerVec)
		{
			if (listener_)
				listener_->OnClipCreateFinish();
		}
	}

	void AnimationTimelineComponent::_updateTimelineGroupData(int idx,const std::string& keyValue_, int startFrame, float length, float startTime)
	{

		float startFrameTime = static_cast<float>(startFrame) / mContext.mDeltaTime;
		float diffTime = mCurrTime - startFrameTime;

        if(idx >= mContext.mClipType.size())
		{
			assert(!"the idx is out of range");
			return;
		}

		if (mContext.mClipType[idx].first == 0)
			_updateActorClipData(idx, keyValue_, startFrame, length, startTime, diffTime);
		else
			_updateComClipData(idx, keyValue_, startFrame, length, startTime, diffTime);
	}

	void AnimationTimelineComponent::_updateActorClipData(int idx, const std::string& keyValue_, int startFrame, float length, float startTime, float diffTime)
	{
		if (mIdxToPtr.size() <= idx )
		{
			assert(! "IdxToPtr  out of size");
			return;
		}

		if (!mIdxToPtr[idx].contains(startFrame))
			return;

		ActorPtr  actorPtr_ = mIdxToPtr[idx][startFrame];
		if (actorPtr_.expired())
			return;

		if (diffTime < 0.f || diffTime > length)
		{
			if (actorPtr_->getRealVisibleState())
				actorPtr_->setRuntimeVisible(false);
			return;
		}

		if (!actorPtr_->getRealVisibleState())
			actorPtr_->setRuntimeVisible(true);

		for (auto& iter : mActorToCom[actorPtr_])
		{
			if (iter.expired())
				continue;
			TimelineComponent* timeCom_ = static_cast<TimelineComponent*>(iter.get());

			//源时间轴的开始时间 + 片段的开始偏移时间 + 当前播放片段的偏差时间 
			float currTime_ = timeCom_->startSceneTime + startTime + diffTime;
			if (currTime_ <= timeCom_->endSceneTime)
			{
#ifdef ECHO_EDITOR
				if (!mIsPlaying && m_TickData.m_IsEditorOperation)
					timeCom_->setTLPlaySpeed(1.f);	// 属于编辑器拖拽操作，则不传递播放速度
				else
					timeCom_->setTLPlaySpeed(getSpeed());		// 处于播放状态时设置Timeline总轨播放速度
#else
				timeCom_->setTLPlaySpeed(getSpeed());
#endif
				timeCom_->timelineUpdate(currTime_);
				timeCom_->setComponentAttribute();
			}
		}

		if (diffTime >= length - std::numeric_limits<float>::epsilon() ||   mCurrTime >= mContext.mEndTime - std::numeric_limits<float>::epsilon())
		{
			if (actorPtr_->getRealVisibleState())
				actorPtr_->setRuntimeVisible(false);
		}
	}

	void AnimationTimelineComponent::_updateComClipData(int idx, const std::string& keyValue_,int startFrame, float length, float startTime, float diffTime)
	{
		if (idx >= mContext.mClipType.size())
		{
			assert(!"the idx is out of range");
			return;
		}

		if (idx >= mIdxToComPtr.size() )
		{
			assert(!"IdxToComPtr  out of size");
			return;
		}

		ActorComponentPtr  comPtr_ = mIdxToComPtr[idx];
		if (comPtr_.expired())
			return;

		if (diffTime < 0.f )
			return;
#ifdef ECHO_EDITOR
		if (!mIsPlaying && m_TickData.m_IsEditorOperation)
			comPtr_->setTLPlaySpeed(1.f);		// 属于编辑器拖拽操作，则不传递播放速度
		else
			comPtr_->setTLPlaySpeed(getSpeed());		// 处于播放状态时设置Timeline总轨播放速度
#else
		comPtr_->setTLPlaySpeed(getSpeed());
#endif

		float lastElapsedTime_ = Root::instance()->getTimeSinceLastFrameClient() * m_TickSpeed;
		if (diffTime > length)
		{
			if (diffTime - lastElapsedTime_ <= length)
			{
				m_TickData.m_tickState = state_end;
				m_TickData.m_Key = mContext.mAnimationVec[idx]->mKeyMap[startFrame];
				AnimationTimelineComponent::mTypeToTickFunc[mContext.mClipType[idx].first](comPtr_.getPointer(), keyValue_, m_TickData);
			}
			return;
		}
		if (!comPtr_->getRealVisibleState())
			comPtr_->setRuntimeVisible(true);

		if (mTypeToTickFunc.contains(mContext.mClipType[idx].first))
		{
			if (diffTime - lastElapsedTime_ < 0.f || (mCurrTime >= lastElapsedTime_ - std::numeric_limits<float>::epsilon() && mCurrTime <= lastElapsedTime_ + std::numeric_limits<float>::epsilon()))
				m_TickData.m_tickState = state_begin;
			else if (mCurrTime >= mContext.mEndTime - std::numeric_limits<float>::epsilon())
				m_TickData.m_tickState = state_end;
			else
				m_TickData.m_tickState = state_running;
			m_TickData.m_Time = startTime + diffTime;
			m_TickData.m_Key = mContext.mAnimationVec[idx]->mKeyMap[startFrame];
			AnimationTimelineComponent::mTypeToTickFunc[mContext.mClipType[idx].first](comPtr_.getPointer(), keyValue_, m_TickData);
		}

	}

	void AnimationTimelineComponent::_loadActorData(int idx, int frame_, const std::string& actorPath_, bool isAsync)
	{
		ActorManager* actorMgr = getActorManager();
		if (!actorMgr)
			return;

		if (mIdxToPtr.size() <= idx || !mIdxToPtr[idx].contains(frame_))
		{

			struct st_ActorCreateInfo  createInfo;
			createInfo.m_CutSceneCom_ = this->weak_from_this();

			ActorPtr actor_ = actorMgr->createActor(Name(actorPath_), "", isAsync,false,nullptr, &createInfo);

			actor_->setRuntimeVisible(false);
			m_pActor->addActor(m_pActor->getRootNodeCom(), actor_);

			//未处理子百分比的容器收集actor
			m_UntreatedActor.insert(actor_);

#ifdef ECHO_EDITOR
			ActorHelper::setActorType(actor_, ActorType_Clip);
#endif
			if (!isAsync)
			{
				filmActorListener* filmListener_ = new filmActorListener(actor_, this, idx, frame_);
				actor_->addActorLoadListener(filmListener_);
	            mLoadState = false;
                actor_->setWholeLoad(true);
				mFilmListenerVec.insert(std::make_pair(filmListener_, false));
				m_LoadObjectSet.emplace(std::make_pair(ProtocolHandle(actor_.get()), LoadPercentData()));
				++m_CollectResourceCount;

				/*stringstream ss;
				ss << "Clip Actor start Load : " <<  ProtocolHandle(actor_.get()) << std::endl;
				LogManager::instance()->logMessage(ss.str());*/

			    filmListener_ = nullptr;
			}
			else
			{
				initFilmActorData(actor_,idx, frame_);
			}
			if (mIdxToPtr.size() <= idx)
				mIdxToPtr.resize(idx + 1);
			mIdxToPtr[idx][frame_] = actor_;
		}
		
	}

	void AnimationTimelineComponent::_loadComData(int idx)
	{
		mIdxToComPtr.resize(mContext.mClipType.size(),nullptr);

		if (idx >= mContext.mClipType.size())
		{
			assert(false && "idx out of clipType size");
			return;
		}

		mIdxToComPtr[idx] = m_pActor->getComponent(mContext.mClipType[idx].second);

		if (idx >= mContext.mAnimationVec.size())
		{
			assert(false && "idx out of size");
			return;
		}
		bool isCollect_ = false;
		if (mContext.mAnimationVec[idx] && !mIdxToComPtr[idx].expired())
		{
			mIdxToComPtr[idx]->preLoadTimelineResource(&mContext.mAnimationVec[idx]->mKeyMap, TimelineResourceType::TL_Type_Key, isCollect_);
		}
		if (isCollect_)
		{
			m_LoadObjectSet.insert(std::make_pair(ProtocolHandle(mIdxToComPtr[idx].get()),LoadPercentData()));
			++m_CollectResourceCount;
		}
	}

	void AnimationTimelineComponent::_clearOldData()
	{
		_clearFilmListener();

		for (const auto& iter : mIdxToPtr)
		{
			for (const auto& actorIter : iter)
			{
				ActorManager* actMgr_ = getActorManager();
				actMgr_->deleteActor(actorIter.second);
			}
		}
		mIdxToComPtr.clear();
		mIdxToPtr.clear();
		mActorToCom.clear();

#ifdef ECHO_EDITOR
		mActorToClipData.clear();
#endif 

	}

	void AnimationTimelineComponent::_moveIdxToActorKey(int oldIdx, int oldFrame, int resultIdx, int resultFrame)
	{
		if (mIdxToPtr.size() <= oldIdx || !mIdxToPtr[oldIdx].contains(oldFrame))
			return;

		ActorPtr oldPtr = mIdxToPtr[oldIdx][oldFrame];
		if (resultIdx >= mIdxToPtr.size())
		{
			mIdxToPtr.resize(resultIdx + 1);
		}

		mIdxToPtr[resultIdx][resultFrame] = oldPtr;

		mIdxToPtr[oldIdx].erase(oldFrame);

		if (mIdxToPtr[oldIdx].empty())
			mIdxToPtr.erase(mIdxToPtr.begin() + oldIdx);
	}

	void AnimationTimelineComponent::_deleteIdxToActorKey(int Idx, int Frame)
	{
		if (mIdxToPtr.size() <= Idx || !mIdxToPtr[Idx].contains(Frame))
			return;

		ActorPtr ptr_ = mIdxToPtr[Idx][Frame];
		mActorToCom.erase(ptr_);
		mIdxToPtr[Idx].erase(Frame);
		if (mIdxToPtr[Idx].empty())
			mIdxToPtr.erase(mIdxToPtr.begin() + Idx);

#ifdef ECHO_EDITOR
		mActorToClipData.erase(ptr_);
#endif 

		ActorManager* pActorMgr = getActorManager();
		if (pActorMgr)
			pActorMgr->deleteActor(ptr_);

	}

	void AnimationTimelineComponent::_deleteIdxToActorRow(int idx)
	{
		if (mIdxToPtr.size() <= idx )
			return;

		std::unordered_set<ActorPtr> ptrSet;
		for (auto& iter : mIdxToPtr[idx])
		{
			mActorToCom.erase(iter.second);
			if(!iter.second.expired())
			   ptrSet.insert(iter.second);
		}
		mIdxToPtr[idx].clear();
		mIdxToPtr.erase(mIdxToPtr.begin() + idx);

#ifdef ECHO_EDITOR
		for(const ActorPtr tempPtr : ptrSet)
		   mActorToClipData.erase(tempPtr);
#endif 
		ActorManager* pActorMgr = getActorManager();
		if (pActorMgr)
		{
			for(const ActorPtr& ptr_ : ptrSet)
			{
				pActorMgr->deleteActor(ptr_);
			}
		}
		ptrSet.clear();
	}

	void AnimationTimelineComponent::_initTimelineComList(const ActorPtr& actPtr_)
	{
		if (actPtr_.expired())
			return;

		const std::vector<ActorComponentPtr>& comVec_ = actPtr_->getComponentsByType(TimelineComponent::mType);

		for (int i = 0; i < comVec_.size(); ++i)
		{
			mActorToCom[actPtr_].insert(comVec_[i]);
		}

		std::vector<ActorPtr> actorVec_;

		actPtr_->getAllChildActors(actorVec_);

		for (int i = 0; i < actorVec_.size(); ++i)
		{
			const std::vector<ActorComponentPtr>& childComVec_ = actorVec_[i]->getComponentsByType(TimelineComponent::mType);
			for (int j = 0; j < childComVec_.size(); ++j)
				mActorToCom[actPtr_].insert(childComVec_[j]);
		}
	}

	bool AnimationTimelineComponent::_judgeIsActorClipKey(int idx)
	{
		if (idx < mContext.mClipType.size() && mContext.mClipType[idx].first != 0)
			return false;
		return true;
	}

	void AnimationTimelineComponent::_updateTickComSpeed(const ActorComponentPtr& com_)
	{
		if (com_.expired())
			return;
		com_->setTLPlaySpeed(getSpeed());
		com_->updatePlaySpeed();
	}

	void AnimationTimelineComponent::_loadFinish()
	{
		m_CollectResourceCount = 0;
		m_LoadObjectTickSecond = 0.f;
		mLoadState = true;
	}

	void AnimationTimelineComponent::_updateLoadPercent()
	{
		for (std::map<ProtocolHandle, LoadPercentData>::value_type& loadIter : m_LoadObjectSet)
		{
			if (loadIter.second.waitComs.empty())
				continue;
			EchoTSet<ActorComponentPtr>::const_iterator comIter_ = loadIter.second.waitComs.begin();
			while (comIter_ != loadIter.second.waitComs.end())
			{
				if ((*comIter_).expired() || (*comIter_)->isCreateFinish())
				{
					++loadIter.second.succCount;
					comIter_ = loadIter.second.waitComs.erase(comIter_);

					if (loadIter.second.count == 0)
						loadIter.second.percent = 99.0;
					else
						loadIter.second.percent = static_cast<double>(loadIter.second.succCount) / loadIter.second.count;
					loadIter.second.percent = std::min(99.0, loadIter.second.percent);
					continue;
				}
				++comIter_;
	 		}
		}
	}

	bool filmActorListener::OnActorCreateFinish()
	{
		mAnimationCom->initFilmActorData(mActor, mIdx, mFrame);
		mAnimationCom->resourceLoadFinish(ProtocolHandle(mActor.get()));
		mAnimationCom->setFilmListenerState(this, true);

		/*stringstream ss;
		ss << "Clip Actor Finish Load : " << ProtocolHandle(mActor.get()) << std::endl;
		LogManager::instance()->logMessage(ss.str());*/

#ifdef ECHO_EDITOR
		ActorHelper::setActorType(mActor, ActorType_Clip);
#endif
		return true;
	}
}
