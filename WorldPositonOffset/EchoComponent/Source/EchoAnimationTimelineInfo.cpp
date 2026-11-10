#include "EchoComponentStableHeaders.h"
#include "EchoAnimationTimelineInfo.h"
//#include "EchoTimeLineInfo.h"
#include "EchoResourceManagerFactory.h"
#include "EchoLogManager.h"
#include "EchoRoot.h"
#include "EchoHermiteCurve.h"
#include "EchoHermite.h"
#include "EchoTimeLineInfo.h"


namespace Echo {


	void  AnimationTimelineInfoHandle::clearAnimationVec()
	{
		for (int i = 0; i < mAnimationVec.size(); ++i)
		{
			if (!mAnimationVec[i])
				continue;
			for (auto& iter : mAnimationVec[i]->mKeyMap)
			{
				if (iter.second)
					delete iter.second;
				iter.second = nullptr;
			}
			mAnimationVec[i]->mKeyMap.clear();

			delete mAnimationVec[i];
			mAnimationVec[i] = nullptr;
		}
	}
	void AnimationTimelineInfoHandle::copyAnimationVec(const AnimationTimelineInfoHandle& rhs)
	{
		mAnimationVec.resize(rhs.mAnimationVec.size());

		mClipType = rhs.mClipType;

		for (int i = 0; i < rhs.mAnimationVec.size(); ++i)
		{
			if (!rhs.mAnimationVec[i])
				continue;
			if (rhs.mAnimationVec[i]->mHermiteType == EchoHermite::String_Hermite)
			{
				EchoHermiteFactory* factory_ = EchoHermiteFactortManager::getSingleton().getHermiteFactory(rhs.mAnimationVec[i]->getType());
				if (!factory_)
				{
					assert( !rhs.mAnimationVec[i]->getType().c_str() );
					continue;
				}
				mAnimationVec[i] =  static_cast<EchoStringHermite*>(factory_->createHermite());
				for (const auto& iter : rhs.mAnimationVec[i]->mKeyMap)
				{
					HermiteStringKey* key_ = static_cast<HermiteStringKey*>(mAnimationVec[i]->CreateKey());
					HermiteStringKey* oriKey_ = static_cast<HermiteStringKey*>(iter.second);
					oriKey_->Copy(key_);
					mAnimationVec[i]->mKeyMap[iter.first] = key_;
					key_ = nullptr;
				}
			}
		}

		if (mClipType.size() < mAnimationVec.size())
			mClipType.resize(mAnimationVec.size(), std::pair<int, std::string>(0, ""));
	}

	AnimationTimelineInfoHandle::AnimationTimelineInfoHandle(const AnimationTimelineInfoHandle& rhs)
	{
		mStartTime = rhs.mStartTime;
		mEndTime = rhs.mEndTime;
		mDeltaTime = rhs.mDeltaTime;
#ifdef ECHO_EDITOR
		mClipName = rhs.mClipName;
#endif
		clearAnimationVec();
		copyAnimationVec(rhs);
	}

	AnimationTimelineInfoHandle& AnimationTimelineInfoHandle::operator=(const AnimationTimelineInfoHandle& rhs)
	{
		if (this == &rhs)
			return *this;
		mStartTime = rhs.mStartTime;
		mEndTime = rhs.mEndTime;
		mDeltaTime = rhs.mDeltaTime;
		
#ifdef ECHO_EDITOR
		mClipName = rhs.mClipName;
#endif
		clearAnimationVec();
		copyAnimationVec(rhs);
		return *this;
	}

	AnimationTimelineInfoHandle::~AnimationTimelineInfoHandle()
	{
		clearAnimationVec();
	}





	AnimationTimeLineInfo::AnimationTimeLineInfo(ResourceManagerBase* creator, const Name& actFileName, bool isManual, ManualResourceLoader* loader)
		: ResourceBase(creator, actFileName, isManual, loader)
	{
	}
	AnimationTimeLineInfo::~AnimationTimeLineInfo()
	{
		unload();
	}
	void AnimationTimeLineInfo::loadFile(const std::string& filePath)
	{
		if (filePath == "")
			return;
		DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(filePath.c_str()));
		if (pDataStream.isNull())
		{
			if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
				LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, "AnimationTimeline : File not exit: " + filePath);
			return;
		}

		size_t nSize = pDataStream->size();
		char* pData = new char[nSize + 1];
		memset(pData, 0, nSize + 1);
		pDataStream->read(pData, nSize);
		cJSON* pRoot = cJSON_Parse(pData);
		delete[] pData;
		if (pRoot == nullptr)
			return;
		

		ReadJSNode(pRoot, "Ver", mVer);

		_loadFile(pRoot);
		
		cJSON_Delete(pRoot);

	}
	void AnimationTimeLineInfo::saveFile(const std::string& filePath)
	{
		if (filePath == "")
			return;
		cJSON* pRoot = cJSON_CreateObject();
		mVer = "1.2";
		WriteJSNode(pRoot, "Ver", mVer);
		
		_saveFile(pRoot);

		char* out = cJSON_Print(pRoot);
		cJSON_Delete(pRoot);

		DataStreamPtr pDataStream(Echo::Root::instance()->CreateDataStream(filePath.data()));
		if (pDataStream.isNull())
		{
			free(out);
			return;
		}


		pDataStream->writeData(out, strlen(out));
		pDataStream->close();
		free(out);
		return;
	}
	void AnimationTimeLineInfo::_loadFile(cJSON* pRoot)
	{
		if (!pRoot)
			return;
		
		ReadJSNode(pRoot, "StartTime", mDataHandle.mStartTime);
		ReadJSNode(pRoot, "EndTime", mDataHandle.mEndTime);

		cJSON* AnimationVec_ =cJSON_GetObjectItem(pRoot, "AnimationVec");
		if(AnimationVec_)
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
						std::string type_;

						ReadJSNode(AnimationChildData, "HermiteType", type_);
						EchoStringHermite* hermite_ = nullptr;
						if (mVer == "1.l")
							hermite_ = new EchoStringHermite();
						else if (mVer == "1.2")
						{
							EchoHermiteFactory* fac_ = EchoHermiteFactortManager::getSingleton().getHermiteFactory(Name(type_));
							if (fac_)
								hermite_ = static_cast<EchoStringHermite*>(fac_->createHermite());
						}

						if (hermite_ == nullptr)
						{
							assert(!"Create Hermite False");
							continue;
						}

						std::pair<int, std::string> tempTypePair;
						ReadJSNode(AnimationChildData, "TYPE", tempTypePair.first);
						ReadJSNode(AnimationChildData, "COMNAME", tempTypePair.second);
						mDataHandle.mClipType.emplace_back(tempTypePair);

#ifdef ECHO_EDITOR
						std::string clipName_ = "";
						ReadJSNode(AnimationChildData, "ClipName", clipName_);
						mDataHandle.mClipName.emplace_back(clipName_);
#endif

						cJSON* animationKeyItem_ = cJSON_GetObjectItem(AnimationChildData, "AnimaitonKeyItem");

						if (animationKeyItem_)
						{
							int animationKeyItem_Sz = cJSON_GetArraySize(animationKeyItem_);

							for (int k = 0; k < animationKeyItem_Sz; ++k)
							{
								cJSON* keyValue_ = cJSON_GetArrayItem(animationKeyItem_, k);
								if (keyValue_)
								{
									int frame_;
									ReadJSNode(keyValue_, "Frame", frame_);

									if (mVer == "1.l")
									{
										HermiteStringKey* key_ = new HermiteStringKey();
										ReadJSNode(keyValue_, "Value", key_->value);
										ReadJSNode(keyValue_, "StartTime", key_->mStartTime);
										ReadJSNode(keyValue_, "Length", key_->mLength);
										hermite_->mKeyMap[frame_] = key_;
									}
									else if (mVer == "1.2")
									{
										HermiteKey* key_ = hermite_->CreateKey();
										key_->readJson(keyValue_);
										hermite_->mKeyMap[frame_] = key_;
									}
								}
							}
							mDataHandle.mAnimationVec.emplace_back(hermite_);
						}
					}
				}
			}
			
		}

		if (mDataHandle.mClipType.size() < mDataHandle.mAnimationVec.size())
			mDataHandle.mClipType.resize(mDataHandle.mAnimationVec.size(),std::pair<int,std::string>(0,""));
	}
	void AnimationTimeLineInfo::_saveFile(cJSON* pRoot)
	{
		if (!pRoot)
			return;

		if (mDataHandle.mClipType.size() < mDataHandle.mAnimationVec.size())
			mDataHandle.mClipType.resize(mDataHandle.mAnimationVec.size(), std::pair<int, std::string>(0, ""));

		WriteJSNode(pRoot,"StartTime", mDataHandle.mStartTime);
		WriteJSNode(pRoot,"EndTime", mDataHandle.mEndTime);

		cJSON* AnimationVec_ = cJSON_CreateArray();
		cJSON_AddItemToObject(pRoot, "AnimationVec", AnimationVec_);

		for (int i = 0; i < mDataHandle.mAnimationVec.size(); ++i)
		{
			cJSON* AnimaitonItem_ = cJSON_CreateArray();
			cJSON_AddItemToObject(AnimationVec_, "AnimaitonItem", AnimaitonItem_);

			cJSON* Animation_ = cJSON_CreateObject();
			cJSON_AddItemToArray(AnimaitonItem_, Animation_);


			WriteJSNode(Animation_,"IDX",i);
			WriteJSNode(Animation_, "TYPE", mDataHandle.mClipType[i].first);
			WriteJSNode(Animation_, "COMNAME", mDataHandle.mClipType[i].second);
#ifdef ECHO_EDITOR
			if(i < mDataHandle.mClipName.size())
			   WriteJSNode(Animation_, "ClipName", mDataHandle.mClipName[i]);
#endif
			/*if (mDataHandle.mAnimationVec[i]->mHermiteType == EchoHermite::String_Hermite)
			{
				std::string type = "String";
				WriteJSNode(Animation_, "HermiteType", type);
			}*/
			std::string type = mDataHandle.mAnimationVec[i]->getType().c_str();
			WriteJSNode(Animation_, "HermiteType", type);

			cJSON* AnimaitonKeyItem_ = cJSON_CreateArray();

			cJSON_AddItemToObject(Animation_, "AnimaitonKeyItem", AnimaitonKeyItem_);
			for (const auto& iter : mDataHandle.mAnimationVec[i]->mKeyMap)
			{
				cJSON* AnimationKey_ = cJSON_CreateObject();
				cJSON_AddItemToArray(AnimaitonKeyItem_, AnimationKey_);
				
				
				int frame = iter.first;
				WriteJSNode(AnimationKey_, "Frame" , frame);

				if (mDataHandle.mAnimationVec[i]->mHermiteType == EchoHermite::String_Hermite)
				{
					HermiteStringKey* key_ = static_cast<HermiteStringKey*>(iter.second);
					if (key_)
					{
						key_->writeJson(AnimationKey_);
						/*WriteJSNode(AnimationKey_, "Value", key_->value );
						WriteJSNode(AnimationKey_, "StartTime", key_->mStartTime);
						WriteJSNode(AnimationKey_, "Length", key_->mLength);*/
					}
				}
			}
		}
	}
	void AnimationTimeLineInfo::prepareImpl(void)
	{
		loadFile(mName.c_str());
	}

	size_t AnimationTimeLineInfo::calcMemUsage(void) const
	{
		return 0u;
	}


	AnimationTimeLineInfoManager::AnimationTimeLineInfoManager()
		:ResourceManagerBase()
	{
		mResourceType = "AnimationTimeLineInfo";
		ResourceManagerFactory::instance()->registerResourceManager(mResourceType, this);
	}

	AnimationTimeLineInfoManager::~AnimationTimeLineInfoManager()
	{
		ResourceManagerFactory::instance()->unregisterResourceManager(mResourceType);
	}

	AnimationTimelineInfoPtr AnimationTimeLineInfoManager::createManual(const Name& name, ManualResourceLoader* loader)
	{
		AnimationTimelineInfoPtr newTimelineInfo = createResource(name, true, loader);
		return newTimelineInfo;
	}

	ResourceBase* AnimationTimeLineInfoManager::createImpl(const Name& name, bool isManual, ManualResourceLoader* loader, const NameValuePairList* createParams)
	{
		AnimationTimeLineInfo* timelineInfo = new AnimationTimeLineInfo(this, name, isManual, loader);
		return timelineInfo;
	}

}

