#include "EchoComponentStableHeaders.h"
#include "EchoLightFlareComponent.h"
#include "EchoPhysXScene.h"
#include "EchoLightFlareComponentManager.h"
#include "EchoSphericalTerrainManager.h"
#include "EchoSphericalOcean.h"

namespace Echo {

	DECLAREACTORCOMTYPE_CPP(LightFlareComponent)

	
	const float LEN_2 = sqrtf(0.5f) * 0.5f;  // 根号1/2 * 0.5

	float  unit_trans_8[8][2] = { {-0.5f, 0.f}, { -LEN_2 ,LEN_2 }, { 0.f,0.5f } ,{LEN_2, LEN_2}, { 0.5f,0.f }, { LEN_2,-LEN_2 } ,{0.f, -0.5f}, { -LEN_2,-LEN_2 } };
	float  unit_half_trans_4[4][2] = { {-0.5f, 0.f} ,{ -0.25f, 0.433f} ,{ 0.25f, 0.433f} ,{0.5f,0.f} };
	float  unit_trans_4[4][2] = { {-0.5f, 0.f}, { 0.f,0.5f } , { 0.5f,0.f },{ 0.f, -0.5f } };

	EDITOR_REFLECT_BEGIN(LightFlareComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mLightFlareVisible, "LightFlareVisible", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mLightFlareScaleRange, "LightFlareScaleRange", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTextureName, "TextureName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mParamVec, "ParamVec", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mLightFlareType, "LightFlareType", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mLightFlareRealSize, "LightFlareRealSize", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mLightFlareModel, "LightFlareModel", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mAppearDistance, "LightFlareAppearDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mDisAppearDistance, "LightFlareDisAppearDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mMinSizePercentVec, "LightFlareMinSizePercentVec", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFirstLevelDistance, "LightFlareFirstLevelDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSecondLevelDistance,"LightFlareSecondLevelDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mObstructingOpacity, "openObstructingOpacity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

	EDITOR_REFLECT_END()

	LightFlareComponentInfo::LightFlareComponentInfo(const Name& name, ActorCOMMoveableType isMoveable)
		:ActorComponentInfo(name,isMoveable)
	{
	}
	void LightFlareComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<LightFlareComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}
	void LightFlareComponentInfo::ImportData(cJSON* jsonNode)
	{
		if (jsonNode == nullptr)
			return;
		reflect::TypeResolver<LightFlareComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}
	void LightFlareComponentInfo::CopyTo(ActorComponentInfo* info)
	{
		if (info == nullptr)
			return;
		LightFlareComponentInfo* newObjectInfo = static_cast<LightFlareComponentInfo*>(info);
		reflect::TypeResolver<LightFlareComponentInfo>::get()->copyTo(this, newObjectInfo);
	}
	void LightFlareComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<LightFlareComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

#ifdef ECHO_EDITOR
	bool LightFlareComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		if (mContext.mTextureName.empty())
			return false;
		resMap.insert(std::make_pair(ResourceType::NeedCopyRes,mContext.mTextureName));
		return true;
	}
#endif

	LightFlareComponent::LightFlareComponent(ActorPtr pActor, ActorCOMMoveableType isMoveAble)
		:ActorComponent(pActor, isMoveAble)
	{

	}
	LightFlareComponent::~LightFlareComponent()
	{

	}

	void LightFlareComponent::visibleStateChanged(bool isVisible)
	{
		setComLightFlareVisible(isVisible);
	}

	void LightFlareComponent::resourceArrived(const Name& resName, const Name& resType, bool bFail, const ResourceBasePtr& resourcePtr)
	{
		if (resourcePtr.isNull())
		{
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, "LightFlare :File not exit: " + resName.toString());
			return;
		}
	
		if (mLightFlareMgr && mLensFlare)
		{
			mLightFlareMgr->setTextureByName(static_cast<TexturePtr>(resourcePtr), resName.c_str());
			mLensFlare->setTextureName(resName.c_str());
			setComLightFlareVisible(true);
		}
	}

	void LightFlareComponent::init(ActorComponentInfo* info)
	{
		if (info != nullptr)
		{
			LightFlareComponentInfo* lightFlareInfo = static_cast<LightFlareComponentInfo*>(info);
			mContext = lightFlareInfo->mContext;
		}

		LightFlareComponentManager* lMgr = LightFlareComSystem::instance()->getLightFlareComponentManager(m_pActor->getActorManager()->getName());
		if (lMgr)
			lMgr->addLightFlareCom(this);
	}

	void LightFlareComponent::unInit()
	{
		_deleteLightFlare();
		if (mLightFlareMgr)
			mLightFlareMgr->deleteLightDlareCom(this);
	}

	void LightFlareComponent::initRenderObject(ActorComponentInfo* info)
	{
		_createLightFlare();
		_initCamera();
		_resetLightFlareData();
		mDirVec.resize(16);
		mDisVec.resize(16);
	}

	void LightFlareComponent::setLightFlareParamsVec(const std::vector<LensFlareParam>& lightFlarePaream_)
	{
		mContext.mParamVec = lightFlarePaream_;
		if (mLensFlare)
			_resetLightFlareData();
	}

	const std::vector<LensFlareParam>& LightFlareComponent::getLightFlareParamsVec()
	{
		return mContext.mParamVec;
	}

	void LightFlareComponent::setLightFlareTextureName(const std::string& textureName_, bool isSync)
	{
		if (!mLightFlareMgr)
			return;
		mContext.mTextureName = textureName_;
		
		if (textureName_.empty())
			return;
		if (!mLensFlare)
			return;

		if (isSync)
		{
		   mLensFlare->setTextureName(textureName_);
		}
		else
		{
			TexturePtr  texPtr;
			mLightFlareMgr->getTextureByName(texPtr, textureName_);

			//如果该贴图资源未缓存就发送一个异步请求
			if (texPtr.isNull())
			{
				Name texName = Name(textureName_);
				ResourceBasePtr pResource = TextureManager::instance()->getByName(texName);
				if (!pResource.isNull())
				{
					if (pResource->isLoaded())
					{
						setComLightFlareVisible(true);
						mLensFlare->setTextureName(textureName_);
					}
					else if (pResource->isPrepared())
					{
						pResource->load();
						setComLightFlareVisible(true);
						mLensFlare->setTextureName(textureName_);
					}
					else
					{
						setComLightFlareVisible(false);
						ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_LIGHT);
						requestResouce(texName, TextureManager::instance()->getResourceType(), priority);
					}
				}
				else 
				{
					setComLightFlareVisible(false);
					ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_LIGHT);
					requestResouce(texName, TextureManager::instance()->getResourceType(), priority);
				}
			}
			else
			{
				setComLightFlareVisible(true);
				mLensFlare->setTextureName(textureName_);
			}
		}
	}

	void LightFlareComponent::setComLightFlareVisible(bool visible_)
	{
		mContext.mLightFlareVisible = visible_;
		if (mLensFlare)
			mLensFlare->setComLensFlareVisible(visible_);
	}

	void LightFlareComponent::setLightFlareScale(const Vector2& scale_)
	{
		mContext.mLightFlareScaleRange = scale_;
		if (mLensFlare)
			mLensFlare->setDynamicScaleRange(mContext.mLightFlareScaleRange);
	}

	void LightFlareComponent::_computeLightFlareData()
	{
		mContext.mLightPosition = getWorldPosition();
	    mCameraPos = mCamera->getPosition();
		mLightDir = mContext.mLightPosition - mCameraPos;
		mLightDistance = std::sqrtf(mLightDir.x * mLightDir.x + mLightDir.y * mLightDir.y + mLightDir.z * mLightDir.z);
		mLightDir.normalise();
	}

	bool LightFlareComponent::_getLightFlareIsAtConeOfVision()
	{
		if (mGlobalOpacity == 0.f)
			return false;
		//可视性判断
		Vector3 cameraDir = mCamera->getDirection();
		cameraDir.normalise();

		Sphere sunSphere(mContext.mLightPosition, 1.f);
		if (!mCamera->isVisible(sunSphere))
		{
			if(mLensFlare)
			   mLensFlare->setComLensFlareVisible(false);
			return false;
		}
		return true;
	}

	bool LightFlareComponent::_getLightFlareIsVisible()
	{
		//物理射线
		EchoRaycastHit rayHit_;
		ProtocolComponentSystem::instance()->callProtocolFunc(Echo::PFID_SetLightFlareFilter,nullptr,(ProtocolHandle)this);

		bool isshow_ = EchoPhysicsSystem::instance()->GetPhysicsManager(this)->Raycast(mCameraPos, mLightDir, rayHit_, mLightDistance, PhysXGroup::GetMask(Echo::Name("interactionraycast")),mLightFlareFilter);
		if (isshow_)
		{
			if(mLensFlare)
		 	   mLensFlare->setComLensFlareVisible(false);
			return false;
		}
		return true;
	}

	void LightFlareComponent::_computeLightFlareDegreeofOcclusion(int resultSize)
	{
		if (!mLightFlareMgr)
			return;
		if (resultSize == 0)
			return;
		EchoRaycastHit rayHit_;
		_computeLightPointDir(resultSize);

		int cnt = 0;
		for (size_t i = 1; i < resultSize; ++i)
		{
			bool result_ = EchoPhysicsSystem::instance()->GetPhysicsManager(this)->Raycast(mCameraPos, mDirVec[i], rayHit_, mDisVec[i], PhysXGroup::GetMask(Echo::Name("interactionraycast")));
			if (result_)
				mShelterPointSet[i] = 1,++cnt;
			else
				mShelterPointSet[i] = 0;
		}

		if (cnt == 0)
			mIsHalfModel = false;

		if (!mIsHalfModel)
		{
			if (cnt >= 1)
			{
				_setHalfRoundDirection(resultSize);
				mIsHalfModel = true;
			}
			else
				mOcclusionRatio = 1.f;
		}
		else
		{	
			mOcclusionRatio = _getOcclusionPercent(resultSize);
		}

	}

	bool LightFlareComponent::_getLightFlareState()
	{
		if(mLensFlare == nullptr || !mContext.mLightFlareVisible)
		    return false;
		return true;
	}

	void LightFlareComponent::_setLightFlareResultSize()
	{
		LightFlareComponentManager* lMgr = LightFlareComSystem::instance()->getLightFlareComponentManager(m_pActor->getActorManager()->getName());
		if (lMgr == nullptr)
			return;
		if (mLightDistance < mContext.mFirstLevelDistance)
		{
			float difDistance = 0.f;
			if (mContext.mLightFlareModel == M_NearBigFarSmall)
				difDistance = mContext.mFirstLevelDistance - mLightDistance;
			else if (mContext.mLightFlareModel == M_NearSmallBigFar)
				difDistance = mLightDistance;
            mRangeScaleByDistance = difDistance / mContext.mFirstLevelDistance;
		}
		else if (mLightDistance < mContext.mSecondLevelDistance)
		{
			if (mContext.mLightFlareModel == M_NearBigFarSmall)
				mRangeScaleByDistance = minSizeFactor;
			else if (mContext.mLightFlareModel == M_NearSmallBigFar)
				mRangeScaleByDistance = 1.f;
		}
			

		if (mLensFlare)
		{
			int sz_ = mLensFlare->getElementsCount();
			for (int i = 0; i < sz_; ++i)
			{
				LensFlareElement* element_ = mLensFlare->getElement(i);
				if (element_)
				{
					float minSizePercent_ = mContext.mMinSizePercentVec[i] * 0.01f;
					element_->setDynamicScale(mOcclusionRatio * (minSizePercent_ + (1.f - minSizePercent_) * mRangeScaleByDistance));
				}
			}
		}
	}

	void LightFlareComponent::setLightFlareType(LightFlareType type_)
	{
		mContext.mLightFlareType = type_;

		if (mLensFlare == nullptr)
			return;
		unsigned int sz = static_cast<unsigned int>(mContext.mLightFlareType);
		if (mContext.mParamVec.size() != sz)
		{
			mContext.mParamVec.resize(sz);
			mContext.mMinSizePercentVec.resize(sz);
			
			mLensFlare->setElements(mContext.mParamVec);
			for(unsigned int i = 0 ;i < sz ; ++i )
			   mLensFlare->getElement(i)->updateVBIB(sz);
			
			setLightFlareTextureName(mContext.mTextureName);
		}
	}


	void LightFlareComponent::setLightFlareDisAppearDistance(float distance_)
	{
		if (distance_ == mContext.mAppearDistance)
			distance_ = mContext.mAppearDistance - 0.1f;
		mContext.mDisAppearDistance = distance_;
	}


	void LightFlareComponent::setLightFlareFilter(const EchoSceneQueryFilter& flareFilter_)
	{
		mLightFlareFilter = flareFilter_;
	}

	void LightFlareComponent::setLightFlareComMgr(LightFlareComponentManager* lMgr)
	{
		mLightFlareMgr = lMgr;
	}

	void LightFlareComponent::setFirstLevelDistance(float distance_)
	{
		mContext.mFirstLevelDistance = distance_;
	}

	void LightFlareComponent::setSecondLevelDistance(float distance_)
	{
		mContext.mSecondLevelDistance = distance_;
	}

	void LightFlareComponent::setLightFlareMinSizePercent(const std::vector<float>& minSizePercent_)
	{
		mContext.mMinSizePercentVec = minSizePercent_;
	}

	void LightFlareComponent::setLightFlareMinSizePercentByIdx(int i, float percent)
	{
		if (i >= mContext.mMinSizePercentVec.size())
			return;
		mContext.mMinSizePercentVec[i] = percent;
	}

	float LightFlareComponent::getLightFlareMinSizePercentByIdx(int i)
	{
		if (i >= mContext.mMinSizePercentVec.size())
			return 1.f;
		return mContext.mMinSizePercentVec[i];
	}

	void LightFlareComponent::_initCamera()
	{
		SceneManager* p_sMgr = getSceneManager();
		if(p_sMgr)
		   mCamera = p_sMgr->getMainCamera();
	}

	void LightFlareComponent::_createLightFlare()
	{
		SceneManager* p_sMgr = getSceneManager();
		if (!p_sMgr)
			return;
		mLensFlare = new LensFlare();
		if (mLensFlare)
		{
            mLensFlare->_notifyManager(p_sMgr);
			mLensFlare->initLightFlare(Name::g_cEmptyName);
			mLensFlare->setQueryFlags(0);
			mLensFlare->setEnabled(false);
		}
		  
	}

	void LightFlareComponent::_deleteLightFlare()
	{

		SceneManager* p_sMgr = getSceneManager();
		if (!p_sMgr)
			return;
		LensFlare* flare_ = p_sMgr->getLensFlare();
		if (mLensFlare)
		{
			mLensFlare->deinit();
			delete mLensFlare;
		}
		mLensFlare = nullptr;

		p_sMgr->setLensFlare(flare_);
	}

	void LightFlareComponent::_resetLightFlareData()
	{
		if (mLensFlare == nullptr)
			return;
		mLensFlare->setElements(mContext.mParamVec);
		for (unsigned int i = 0; i < mContext.mParamVec.size(); ++i)
		{
			mLensFlare->getElement(i)->updateVBIB(static_cast<int>(mContext.mLightFlareType));
		}

		setLightFlareTextureName(mContext.mTextureName);
		//mLensFlare->setTextureName(mContext.mTextureName);
	}

	void LightFlareComponent::_computeLightPointDir(int resultSize)
	{
		Vector3 unit_x = Vector3::UNIT_Y;
		unit_x = unit_x.crossProduct(mLightDir);
		unit_x.normalise();

		Vector3 unit_y = mLightDir.crossProduct(unit_x);
		unit_y.normalise();

		Vector2 lightSize_;
		const Vector3& worldScale = getWorldScale();
		lightSize_.x = worldScale.x * mContext.mLightFlareRealSize.x * 0.5f;   //* mContext.mParamVec[0].size.x;
		lightSize_.y = worldScale.y * mContext.mLightFlareRealSize.y * 0.5f;   //* mContext.mParamVec[0].size.x;

		if (mIsHalfModel)
		{
			Vector3 tempDir = Vector3(mHalfRoundDirection.x, mHalfRoundDirection.y, 0.f);
			tempDir = tempDir.x * unit_x + tempDir.y * unit_y;
			tempDir.normalise();
			Quaternion q;
			q = Quaternion::FromToRotation(unit_y, tempDir);

			unit_x *= q;
			unit_y *= q;

			unit_x.normalise();
			unit_y.normalise();
			if (resultSize == 16)
				_computeLightPointDir(resultSize, unit_x, unit_y, lightSize_, mLightFlareMgr->unit_half_trans_30);
			else if(resultSize == 8)
				_computeLightPointDir(resultSize, unit_x, unit_y, lightSize_, mLightFlareMgr->unit_trans_16);
			else if(resultSize == 4)
				_computeLightPointDir(resultSize, unit_x, unit_y, lightSize_, unit_half_trans_4);
			else
				_computeLightPointDir(resultSize, unit_x, unit_y, lightSize_, unit_trans_4);
		}
		else
		{
			if(resultSize == 16)
				_computeLightPointDir(resultSize, unit_x, unit_y, lightSize_, mLightFlareMgr->unit_trans_16);
			else if(resultSize == 8)
				_computeLightPointDir(resultSize, unit_x, unit_y, lightSize_, unit_trans_8);
			else
				_computeLightPointDir(resultSize, unit_x, unit_y, lightSize_, unit_trans_4);
		}
			
	}

	void LightFlareComponent::_computeLightPointDir(int resultSize, const Vector3& unit_x, const Vector3& unit_y, const Vector2& lightSize_,const  float transArray[][2])
	{
		//hel.resize(resultSize);
		for (size_t i = 0; i < resultSize; ++i)
		{
			Vector3 temp_x = unit_x * (transArray[i][0] * lightSize_.x);
			Vector3 temp_y = unit_y * (transArray[i][1] * lightSize_.y);

			mDirVec[i] = mContext.mLightPosition + temp_x + temp_y;

			mDirVec[i] = mDirVec[i] - mCameraPos;
			mDisVec[i] = std::sqrtf(mDirVec[i].x * mDirVec[i].x + mDirVec[i].y * mDirVec[i].y + mDirVec[i].z * mDirVec[i].z);
			mDirVec[i].normalise();


			////测试代码
			//if (hel[i] == nullptr)
			//	hel[i] = new HelperObject(Name("123" + std::to_string(i)), HelperObjType_Sphere);
			//if (hel[i])
			//{
			//	hel[i]->setPosition(mCameraPos + mDirVec[i] * mDisVec[i]);
			//	hel[i]->setScale(Vector3(0.01f));
			//	hel[i]->setHelperColorType(static_cast<HelperObjColorType>(i % 16));
			//}
		}
	}

	void LightFlareComponent::_setHalfRoundDirection(int size)
	{
		int result = 0;
		int maxx = 0;
		int maxValue = size / 4 + 1;
		for (int stIdx = 0; stIdx < size; ++stIdx)
		{
			int dirValue_ = 0;
			for (int i = stIdx; i <= stIdx + size / 2; ++i)
			{
				int value = i - stIdx + 1;
				if (value > maxValue)
					value = value - 2 * (value - maxValue);
				dirValue_ += mShelterPointSet[i % size] * value;
			}
			if (dirValue_ >= maxx)
				result = stIdx, maxx = dirValue_;
		}
		if (size == 16)
			mHalfRoundDirection = Vector2(mLightFlareMgr->unit_trans_16[(maxValue - 1 + result) % size][0], mLightFlareMgr->unit_trans_16[(maxValue - 1 + result) % size][1]);
		else if (size == 8)
			mHalfRoundDirection = Vector2(unit_trans_8[(maxValue - 1 + result) % size][0], unit_trans_8[(maxValue - 1 + result) % size][1]);
		else 
			mHalfRoundDirection = Vector2(unit_trans_4[(maxValue - 1 + result) % size][0], unit_trans_4[(maxValue - 1 + result) % size][1]);
	}

	float LightFlareComponent::_getOcclusionPercent(int size)
	{
		int stIdx = 0, enIdx = 0;
		for (int i = 0; i < size; ++i)
		{
			if (mShelterPointSet[i] == 1)
			{
				stIdx = i;
				break;
			}
		}

		for (int i = size; i >= 0; --i)
		{
			if (mShelterPointSet[i] == 1)
			{
				enIdx = i;
				break;
			}
		}
		if (size == 16)
			return  1.f - mLightFlareMgr->mAreaPercentDp_16[stIdx][enIdx];
		else if (size == 8)
			return  1.f - mLightFlareMgr->mAreaPercrntDp_8[stIdx][enIdx];
		else
			return static_cast<float>(enIdx - stIdx) / (size + 1);
		return 1.f;
	}

	void LightFlareComponent::_updateLightFlare()
	{
		if (!mLensFlare)
			return;
		_setLightFlareResultSize();
		mLensFlare->setComLensFlareVisible(true);
		mLensFlare->setMoonRise(false);

		Vector3 cameraDir = mCamera->getDirection();
		cameraDir.normalise();
		Vector3 sunPos = mLightDir * 100;
		Vector3 centerPos = cameraDir * 100;
		Vector3 rayDir = centerPos - sunPos;
		//rayDir.Dot();

		float rayLen = rayDir.length();
		rayDir.normalise();

		const Matrix4& mView = mCamera->getViewMatrix();

		Vector3 vecRight(mView[0][0], mView[0][1], mView[0][2]);
		Vector3 vecUp(mView[1][0], mView[1][1], mView[1][2]);

		float cosVal = rayDir.dotProduct(Vector3(0.0f, 1.0f, 1.0f));
		Radian radian = Math::ACos(cosVal);
		Quaternion qua(radian, cameraDir);

		Vector3 vecRotateRight = qua * vecRight;
		Vector3 vecRotateUp = qua * vecUp;

		vecRight.normalise();
		vecUp.normalise();
		vecRotateRight.normalise();
		vecRotateUp.normalise();


		for (size_t i = 0, count = mLensFlare->getElementsCount(); i < count; ++i)
		{
			LensFlareElement* pElement = mLensFlare->getElement(static_cast<unsigned int>(i));
			if (pElement->isRotate())
			{
				pElement->setCameraParam(vecRotateRight, vecRotateUp, Vector4(rayDir.x, rayDir.y, rayDir.z, rayLen), sunPos + mCameraPos, getParentNodeCom()->getWorldPosition());
			}
			else
			{
				pElement->setCameraParam(vecRight, vecUp, Vector4(rayDir.x, rayDir.y, rayDir.z, rayLen), sunPos + mCameraPos, getParentNodeCom()->getWorldPosition());
			}
		}
	}

	void LightFlareComponent::_updateLightFlareOpacity()
	{
		if (!mLensFlare)
			return;

		float opacity = 1.f;
		if (mContext.mObstructingOpacity)
			opacity = mOcclusionRatio;

		if ( (mContext.mDisAppearDistance > mContext.mAppearDistance && mLightDistance < mContext.mAppearDistance)  ||
			 (mContext.mAppearDistance >= mContext.mDisAppearDistance && mLightDistance > mContext.mAppearDistance) )
		{
			if (mGlobalOpacity * opacity != 1.f)
			{
				mGlobalOpacity = 1.f;
				for (int i = 0; i < mContext.mParamVec.size(); ++i)
					mLensFlare->getElement(i)->setGlobalOpacity(mGlobalOpacity * opacity);
			}
			return;
		}

		float diffDistance = std::fabs(mContext.mAppearDistance - mContext.mDisAppearDistance);
		mGlobalOpacity = 1.f - std::fabs(mLightDistance - mContext.mAppearDistance) / diffDistance;

		double oceanOparity = 1.f;
		SceneManager* sMgr = getSceneManager();
		if (sMgr)
		{
			PlanetManager* pMgr = sMgr->getPlanetManager();
			if (pMgr)
			{
				SphericalTerrain* planet = pMgr->getNearestPlanetToSurface();
				if (planet)
				{
					double depth = 0.0;
					if (planet->isUnderOcean(mCameraPos, &depth))
					{
						SphericalOcean* ocean = planet->getOcean();
						
						if(ocean)
							oceanOparity = exp(-depth / static_cast<double>(ocean->mOceanPSParams[2].w) * 2.0);
					}
				}
				
			}
		}

		mGlobalOpacity = std::min(1.f, std::max(0.f, mGlobalOpacity));
		for (int i = 0; i < mContext.mParamVec.size(); ++i)
			mLensFlare->getElement(i)->setGlobalOpacity((float)oceanOparity * mGlobalOpacity * opacity);
	
	}

	void  WriteJSNode(JSNODE jsonNode, const char* Name, const LensFlareParam& lensFlareParam)
	{
		if (jsonNode == nullptr)
			return;
		cJSON* lensFlareParam_ = cJSON_CreateObject();
		cJSON_AddItemToObject(jsonNode, "lensFlareParam", lensFlareParam_);
		if (lensFlareParam_)
		{
			WriteJSNode(lensFlareParam_, "textureIndex", lensFlareParam.textureIndex);

			WriteJSNode(lensFlareParam_, "location", lensFlareParam.location);
			WriteJSNode(lensFlareParam_, "size", lensFlareParam.size);
			WriteJSNode(lensFlareParam_, "opacity", lensFlareParam.opacity);
			WriteJSNode(lensFlareParam_, "rotate", lensFlareParam.rotate);
			WriteJSNode(lensFlareParam_, "color", lensFlareParam.color);
			WriteJSNode(lensFlareParam_, "luminance", lensFlareParam.luminance);
		}
	}

	bool  ReadJSNode(cJSON* jsonNode, const char* Name, LensFlareParam& lensFlareParam)
	{
		if (jsonNode == nullptr)
			return false;
		cJSON* lensFlareParam_ = cJSON_GetObjectItem(jsonNode, "lensFlareParam");
		if (lensFlareParam_)
		{
			ReadJSNode(lensFlareParam_, "textureIndex", lensFlareParam.textureIndex);
			ReadJSNode(lensFlareParam_, "location", lensFlareParam.location);

			cJSON* size = cJSON_GetObjectItem(lensFlareParam_, "size");
			if (size)
			{
				if (size->type == cJSON_Array)
				{
					ReadJSNode(lensFlareParam_, "size", lensFlareParam.size);
				}
				else if (size->type == cJSON_Number)
				{
					lensFlareParam.size = static_cast<float>(size->valuedouble);
				}
			}
			
			ReadJSNode(lensFlareParam_, "opacity", lensFlareParam.opacity);
			ReadJSNode(lensFlareParam_, "rotate", lensFlareParam.rotate);
			ReadJSNode(lensFlareParam_, "color", lensFlareParam.color);
			ReadJSNode(lensFlareParam_, "luminance", lensFlareParam.luminance);
			return true;
		}
		return false;
	}

	void WriteJSNode(JSNODE jsonNode, const char* Name, LightFlareType type)
	{
		if (jsonNode == nullptr)
			return;
		int type_ = static_cast<int>(type);
		WriteJSNode(jsonNode, Name, type_);
	}
	bool ReadJSNode(cJSON* jsonNode, const char* Name, LightFlareType& type)
	{
		if (jsonNode == nullptr)
			return false;
		int type_;
		bool isRead = ReadJSNode(jsonNode, Name, type_);
		if(isRead)
		   type = static_cast<LightFlareType>(type_);
		return true;
	}

	void _EchoComponentExport WriteJSNode(JSNODE jsonNode, const char* Name, LightFlareModel model)
	{
		if (jsonNode == nullptr)
			return;
		int model_ = static_cast<int>(model);
		WriteJSNode(jsonNode, Name, model_);
	}

	bool _EchoComponentExport ReadJSNode(cJSON* jsonNode, const char* Name, LightFlareModel& model)
	{
		if (jsonNode == nullptr)
			return false;
		int model_;
		bool isRead = ReadJSNode(jsonNode, Name, model_);
		if(isRead)
		   model = static_cast<LightFlareModel>(model_);
		return true;
	}


}
