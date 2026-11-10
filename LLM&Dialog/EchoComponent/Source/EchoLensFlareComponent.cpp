#include "EchoComponentStableHeaders.h"
#include "EchoLensFlareComponent.h"
#include "EchoSun.h"
#include "EchoTODComponentManager.h"

namespace Echo {
	
	DECLAREACTORCOMTYPE_CPP(LensFlareComponent)

	EDITOR_REFLECT_BEGIN(LensFlareComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mCameraDirection, "LensFlareCameraDirection", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mCameraPosition, "LensFlareCameraPosition", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mLensFlareVisible, "LensFlareVisible", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mLensFlareScaleRange, "LensFlareScaleRange", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTextureName, "LensFlareTextureName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mLocationMin, "LensFlareLocationMin", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mLocationMax, "LensFlareLocationMax", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSizeMin, "LensFlareSizeMin", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSizeMax, "LensFlareSizeMax", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mOpacityMin, "LensFlareOpacityMin", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mOpacityMax, "LensFlareOpacityMax", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRandomCount, "LensFlareRandomCount", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSunPosition, "LensFlareSunPosition", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSunDirection, "LensFlareSunDirection", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	    EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTextureIdx, "LensFlareTextureIdx", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIsRotate0, "LensFlareIsTotate0", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIsRotate1, "LensFlareIsTotate1", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIsRotate2, "LensFlareIsTotate2", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIsRotate3, "LensFlareIsTotate3", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mColor, "LensFlareColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mLensFlareDefaultPosition, "LensFlareDefaultPosition", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

	EDITOR_REFLECT_END()
    

	LensFlareComponentInfo::LensFlareComponentInfo(const Name& name, ActorCOMMoveableType isMoveable)
		:ActorComponentInfo(name,isMoveable)
	{
	}

	void LensFlareComponentInfo::ExportData(cJSON* jsonNode)
	{
		if (jsonNode == nullptr)
			return;
		size_t sz = mContext.mParamVec.size();
		if (sz == MAX_LENFLARE_COM_CNT && mContext.mColor.size() == MAX_LENFLARE_COM_CNT && mContext.mTextureIdx.size() == MAX_LENFLARE_COM_CNT)
		{
			mContext.mIsRotate0 = mContext.mParamVec[0].rotate;
			mContext.mIsRotate1 = mContext.mParamVec[1].rotate;
			mContext.mIsRotate2 = mContext.mParamVec[2].rotate;
			mContext.mIsRotate3 = mContext.mParamVec[3].rotate;
			for (size_t i = 0; i < sz; ++i)
			{   
				mContext.mColor[i] = mContext.mParamVec[i].color;
				mContext.mTextureIdx[i] = mContext.mParamVec[i].textureIndex;
			}
		}
		 
		reflect::TypeResolver<LensFlareComponentInfo>::get()->ExportJson(this,jsonNode,"");
	}

	void LensFlareComponentInfo::ImportData(cJSON* jsonNode)
	{
		if (jsonNode == nullptr)
			return;
		reflect::TypeResolver<LensFlareComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void LensFlareComponentInfo::CopyTo(ActorComponentInfo* info)
	{
		if (info == nullptr)
			return;
		LensFlareComponentInfo* newObjectInfo = static_cast<LensFlareComponentInfo*>(info);
		reflect::TypeResolver<LensFlareComponentInfo>::get()->copyTo(this, newObjectInfo);
	}

	void LensFlareComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<LensFlareComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

#ifdef ECHO_EDITOR
	bool LensFlareComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		if(mContext.mTextureName.empty())
		   return false;
		resMap.insert(std::make_pair(ResourceType::NeedCopyRes,mContext.mTextureName));
		return true;
	}
#endif

	LensFlareComponent::LensFlareComponent(ActorPtr pActor,ActorCOMMoveableType isMoveAble)
		:ProtocolActorComponent(pActor, isMoveAble)
	{

	}
	LensFlareComponent::~LensFlareComponent()
	{

	}
	void LensFlareComponent::registerTickTimeBegin()
	{
		m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}

	void LensFlareComponent::beginFrame(EventID eventId, ProtocolHandle)
	{
		switch (eventId)
		{
		case EID_EveryFrame:
			_updateCameraLenFlare();
		     break;
		default:
			break;
		}
	}

	void LensFlareComponent::resourceArrived(const Name& resName, const Name& resType, bool bFail, const ResourceBasePtr& resourcePtr)
	{
		if (resourcePtr.isNull())
		{
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, "LensFlare :File not exit: " + resName.toString());
			return;
		}

		if (TODComponentManager::instance() && mLensFlare)
		{
			TODComponentManager::instance()->addTodTexturePtr(resName.toString(), static_cast<TexturePtr>(resourcePtr));
			mLensFlare->setTextureName(resName.toString());
			setComLensFlareVisible(true);
		}
	}

	void LensFlareComponent::init(ActorComponentInfo* info)
	{
		ProtocolActorComponent::init(info);
		if (info == nullptr)
			return;
		LensFlareComponentInfo* lensInfo_ = static_cast<LensFlareComponentInfo*>(info);
		mContext = lensInfo_->mContext;
		_initParamData();
		SceneManager* sMgr = getSceneManager();
		if (sMgr)
		{
			mCamera = sMgr->getMainCamera();
			mLensFlare = sMgr->getLensFlare();
		}

		setComLensFlareVisible(mContext.mLensFlareVisible);
		_resetComLensFlareElement();
	}

	void LensFlareComponent::unInit()
	{
		ProtocolActorComponent::unInit();
		if(mLensFlare)
		   mLensFlare->setComLensFlareVisible(false);
		mLensFlare = nullptr;
		mCamera = nullptr;
	}

     

	void LensFlareComponent::setLensFlareCamera(Camera* pCamera)
	{
		setLensFlareCameraCameraAttribue(pCamera, mContext.mCameraDirection, mContext.mCameraPosition);
	}

	void LensFlareComponent::setLensFlareCameraDirection(const Vector3& direction)
	{
		setLensFlareCameraCameraAttribue(mCamera, direction, mContext.mCameraPosition);
	}

	void LensFlareComponent::setLensFlareCameraPosition(const Vector3& position_)
	{
		setLensFlareCameraCameraAttribue(mCamera, mContext.mCameraDirection, position_);
	}

	void LensFlareComponent::setLensFlareCameraCameraAttribue(Camera* pCamera, const Vector3& direction_, const Vector3& position_)
	{
		if (pCamera == nullptr)
			return;
		mCamera = pCamera;
		mContext.mCameraDirection = direction_;
		mContext.mCameraPosition = position_;
	}

	void LensFlareComponent::setComLensFlareVisible(bool visible_)
	{
		mContext.mLensFlareVisible = visible_;
		if (mLensFlare)
		{
			mLensFlare->setComLensFlareVisible(visible_);
		}
	}

	void LensFlareComponent::setLensFlareTexture(const std::string& textureName_, bool sync)
	{
		mContext.mTextureName = textureName_;

		if (!mLensFlare)
			return;

		if (sync)
		{
			mLensFlare->setTextureName(textureName_);
		}
		else if (TODComponentManager::instance())
		{
			TexturePtr texPtr;
			TODComponentManager::instance()->getTodTexturePtrByName(textureName_, texPtr);
			if (!texPtr.isNull())
			{
				mLensFlare->setTextureName(textureName_);
			}
			else
			{
				setComLensFlareVisible(false);
				ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_SKY);
				requestResouce(Name(textureName_), TextureManager::instance()->getResourceType(), priority);
			}
		}
	}

	void LensFlareComponent::setLensFlareParamsVec(const std::vector<LensFlareParam>& lensFlarePaream_)
	{
		mContext.mParamVec = lensFlarePaream_;
		if (mLensFlare)
		{
			_resetComLensFlareElement();
		}
	}

	const std::vector<LensFlareParam>& LensFlareComponent::getLensFlareParamsVec()
	{
		return mContext.mParamVec;
	}

	void  LensFlareComponent::setLensFlareLocationMin(const std::vector<float>& locationMin_)
	{
		mContext.mLocationMin = locationMin_;
		if (mLensFlare)
		{
			_resetComLensFlareElement();
		}

	}
	void  LensFlareComponent::setLensFlareLocationMax(const std::vector<float>& locationMax_)
	{
		mContext.mLocationMax = locationMax_;
		if (mLensFlare)
		{
			_resetComLensFlareElement();
		}
	}
	void   LensFlareComponent::setLensFlareSizeMin(const std::vector<float>& sizeMin_)
	{
		mContext.mSizeMin = sizeMin_;
		if (mLensFlare)
		{
			_resetComLensFlareElement();
		}
	}
	void   LensFlareComponent::setLensFlareSizeMax(const std::vector<float>& sizeMax_)
	{
		mContext.mSizeMax = sizeMax_;
		if (mLensFlare)
		{
			_resetComLensFlareElement();
		}
	}
	void   LensFlareComponent::setLensFlareOpacityMin(const std::vector<float>& opacityMin)
	{
		mContext.mOpacityMin = opacityMin;
		if (mLensFlare)
		{
			_resetComLensFlareElement();
		}
	}
	void  LensFlareComponent::setLensFlareOpacityMax(const std::vector<float>& opacityMax)
	{
		mContext.mOpacityMax = opacityMax;
		if (mLensFlare)
		{
			_resetComLensFlareElement();
		}
	}
	void LensFlareComponent::setLensFlareScale(const Vector2& scale_)
	{
		mContext.mLensFlareScaleRange = scale_;
		if (mLensFlare)
		{
			mLensFlare->setDynamicScaleRange(mContext.mLensFlareScaleRange);
		}
	}

	void LensFlareComponent::_initParamData()
	{
		size_t sz = mContext.mParamVec.size();
		if (sz == MAX_LENFLARE_COM_CNT && mContext.mColor.size() == MAX_LENFLARE_COM_CNT && mContext.mTextureIdx.size() == MAX_LENFLARE_COM_CNT)
		{
			mContext.mParamVec[0].rotate = mContext.mIsRotate0;
			mContext.mParamVec[1].rotate = mContext.mIsRotate1;
			mContext.mParamVec[2].rotate = mContext.mIsRotate2;
			mContext.mParamVec[3].rotate = mContext.mIsRotate3;
			for (size_t i = 0; i < sz; ++i)
			{
				mContext.mParamVec[i].color = mContext.mColor[i];
				mContext.mParamVec[i].textureIndex = mContext.mTextureIdx[i];
			}
		}

	}

	bool LensFlareComponent::calculateDirection(const Vector3& position, Vector3& direction)
	{
		if (!mCamera)
		{
			LogManager::instance()->logMessage(LML_NORMAL, "[LensFlareComponent::_calculateDirection] Missing Data Calculation Direction Failed!");
			return false;
		}

		direction = position - mCamera->getPosition();
		direction.normalise();

		return true;
	}

	void  LensFlareComponent::setLensFlareRandomCount(const std::vector<int>& randomCount_)
	{
		mContext.mRandomCount = randomCount_;
		if (mLensFlare)
		{
			_resetComLensFlareElement();
		}
	}
	void  LensFlareComponent::setLensFlareTextureIdx(int idx, uint32 lensFlareIdx_)
	{
		if (idx >= mContext.mParamVec.size())
			return;
		mContext.mParamVec[idx].textureIndex = lensFlareIdx_;
		if (mLensFlare)
		{
			_resetComLensFlareElement();
		}
	}
	
	void LensFlareComponent::setLensFlareIsRotate(int idx, bool rotate_)
	{
		if (idx >= mContext.mParamVec.size())
			return;
		mContext.mParamVec[idx].rotate = rotate_;
		if (mLensFlare)
		{
			_resetComLensFlareElement();
		}
	}
	void LensFlareComponent::setLensFlareColor(int idx, const ColorValue& color_)
	{
		if (idx >= mContext.mParamVec.size())
			return;
		mContext.mParamVec[idx].color = color_;
		if (mLensFlare)
		{
			_resetComLensFlareElement();
		}
	}

	void LensFlareComponent::_resetComLensFlareElement()
	{
		if (mLensFlare == nullptr)
			return;
		std::vector<LensFlareParam> paramVec;
		for (int i = 0; i < MAX_LENFLARE_COM_CNT; i++)
		{
			int random = mContext.mRandomCount[i];
			for (int j = 0; j < random; j++)
			{
				LensFlareParam param;

				param.textureIndex = mContext.mParamVec[i].textureIndex;
				param.color = mContext.mParamVec[i].color;
				param.rotate = mContext.mParamVec[i].rotate;

				Vector2 locationRange;
				locationRange.x = mContext.mLocationMin[i];
				locationRange.y = mContext.mLocationMax[i];
				if (locationRange.x > locationRange.y)
				{
					locationRange.y = locationRange.x;
				}
				param.location = Echo::Math::RangeRandom(locationRange.x, locationRange.y);

				Echo::Vector2 sizeRange;
				sizeRange.x = mContext.mSizeMin[i];
				sizeRange.y = mContext.mSizeMax[i];
				if (sizeRange.x > sizeRange.y)
				{
					sizeRange.y = sizeRange.x;
				}
				param.size = Echo::Math::RangeRandom(sizeRange.x, sizeRange.y);

				Echo::Vector2 opacityRange;
				opacityRange.x = mContext.mOpacityMin[i];
				opacityRange.y = mContext.mOpacityMax[i];
				if (opacityRange.x > opacityRange.y)
				{
					opacityRange.y = opacityRange.x;
				}
				param.opacity = Echo::Math::RangeRandom(opacityRange.x, opacityRange.y);

				paramVec.push_back(param);
			}
		}
		mLensFlare->setElements(paramVec);
		setLensFlareTexture(mContext.mTextureName);
	}

	void LensFlareComponent::_updateCameraLenFlare()
	{
		if (mLensFlare == nullptr || !mContext.mLensFlareVisible || mCamera == nullptr)
			return;

		Vector3 cameraPos = mCamera->getPosition();

		Vector3 lensFlarePosition = mContext.mSunPosition;
		Vector3 lensFlareDirection = mContext.mSunDirection;

		if (mContext.mLensFlareDefaultPosition && mLensFlare->_getManager())
		{
			Sun* pSun = mLensFlare->_getManager()->getSun();

			if(!pSun)
			{
				LogManager::instance()->logMessage(LML_NORMAL, "[LensFlareComponent::_updateCameraLenFlare] Missing Data Calculation Position Failed!");
				return ;
			}

			const Vector3& lightSrcDir = pSun->getSunDirection();
			float farClip = mCamera->getFarClipDistance();
			lensFlarePosition = -lightSrcDir * (farClip - 100.0f) + cameraPos;
		}

		calculateDirection(lensFlarePosition, lensFlareDirection);
		
		if (lensFlareDirection.y < 0.001f)
		{
			mLensFlare->setComLensFlareVisible(false);
            return;
		}
			
		mLensFlare->setMoonRise(false);

		Vector3 sunDir = lensFlareDirection;
		Vector3 cameraDir = mCamera->getDirection();
		cameraDir.normalise();
		Sphere sunSphere(lensFlarePosition, 500);
		if (!mCamera->isVisible(sunSphere))
		{
			mLensFlare->setComLensFlareVisible(false);
			return;
		}
		mLensFlare->setComLensFlareVisible(true);

		Vector3 sunPos = sunDir * 100;
		Vector3 centerPos = cameraDir * 100;
		Vector3 rayDir = centerPos - sunPos;

		float rayLen = rayDir.length();
		rayDir.normalise();

		float cosQ = rayDir.dotProduct(cameraDir);
		float degressQ = Math::ACos(cosQ).valueDegrees();
		float dynamicScale = (degressQ - 75.f) / 15.0f;
		dynamicScale = Math::Clamp(dynamicScale,mContext.mLensFlareScaleRange.x,mContext.mLensFlareScaleRange.y);

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
			LensFlareElement* pElement =  mLensFlare->getElement(static_cast<unsigned int>(i));
			pElement->setDynamicScale(dynamicScale);
			if (pElement->isRotate())
			{
				pElement->setCameraParam(vecRotateRight, vecRotateUp, Vector4(rayDir.x, rayDir.y, rayDir.z, rayLen), sunPos + cameraPos, lensFlarePosition);
			}
			else
			{
				pElement->setCameraParam(vecRight, vecUp, Vector4(rayDir.x, rayDir.y, rayDir.z, rayLen), sunPos + cameraPos, lensFlarePosition);
			}
		}

	}
}
