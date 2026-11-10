#include "EchoComponentStableHeaders.h"
#include "EchoSunComponent.h"
#include "EchoSun.h"
#include "EchoSceneManager.h"
#include "EchoTODComponentManager.h"
#include "EchoRenderStrategy.h"
#include "EchoPostProcess.h"
#include "EchoTODManager.h"
#include "EchoTODVariableInfo.h"
#include "EchoLogManager.h"
#include "EchoTODComponentSystem.h"
#include "EchoTODDataUtil.h"

namespace Echo {

	
		EDITOR_REFLECT_BEGIN(SunComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSunColor, "SunColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSunIntensity, "SunIntensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mEmmittingColor, "EmmittingColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mInRadius, "InRadius", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mOutRadius, "OutRadius", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mOpacity, "Opacity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mHDRIntensity, "HDRIntensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTransFormTime, "TransTime", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSunTextureName, "SunTextureName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mMoonTextureName, "MoonTextureName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END()

    DECLAREACTORCOMTYPE_CPP(SunComponent)

	SunComponentInfo::SunComponentInfo(const Name & comType, ActorCOMMoveableType type)
		:ActorComponentInfo(comType,type)
	{
	   
	}
	SunComponentInfo::~SunComponentInfo()
	{
	
	}
	void SunComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<SunComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void SunComponentInfo::ImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<SunComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void SunComponentInfo::CopyTo(ActorComponentInfo * comInfo)
	{
		if (comInfo == nullptr)
			return;
		SunComponentInfo* sunInfo = static_cast<SunComponentInfo*>(comInfo);
		reflect::TypeResolver<SunComponentInfo>::get()->copyTo(this, sunInfo);
	}

	void SunComponentInfo::Overlay(ActorComponentInfo* srcInfo)
	{
	}

	void SunComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<SunComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

#ifdef ECHO_EDITOR
	bool SunComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{

		bool isCopyFile = false;
		if(!mContext.mSunTextureName.empty())
		{
			resMap.insert(std::make_pair(ResourceType::NeedCopyRes,mContext.mSunTextureName));
			isCopyFile = true;
		}
		for (const std::string& texName : mContext.mMoonTextureName)
		{
			if (!texName.empty())
			{
				resMap.insert(std::make_pair(ResourceType::NeedCopyRes, texName));
				isCopyFile = true;
			}
		}
		
		return isCopyFile;
	}
#endif

	Name sunColorName_ = Name("SunColor");
	Name sunIntensityName_ = Name("SunIntensity");
	Name sunEmmitColorName_ = Name("SunEmmitColor");
	Name sunInRadiusName_ = Name("SunInRadius");
	Name sunOutRadiusName_ = Name("SunOutRadius");
	Name sunOpacityName_ = Name("SunOpacity");
	Name sunHDRIntensityName_ = Name("SunHDRIntensity");

	SunComponent::SunComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType) :
		ActorComponent(pActor, movableType)
	{
#ifdef ECHO_EDITOR
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_SUN_COLOR, sunColorName_.toString(), 4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_SUN_COLOR_POWER, sunIntensityName_.toString(), 1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_SUN_EMISSION_COLOR, sunEmmitColorName_.toString(), 4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_SUN_RADIUS, sunOutRadiusName_.toString(), 1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_SUN_RADIUS2, sunInRadiusName_.toString(), 1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_SUN_OPACITY, sunOpacityName_.toString(), 1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_SUN_COLOR_HDR_POWER, sunHDRIntensityName_.toString(), 1);
#endif // ECHO_EDITOR
	}

	SunComponent::~SunComponent()
	{
	}
	void SunComponent::init(ActorComponentInfo * info)
	{
		TODCOMPONENT_INITMGR(getSunContext, m_tMgr, true);
		
		if (info == nullptr)
			return;
		SunComponentInfo* mSunComInfo = static_cast<SunComponentInfo*>(info);
		mContext = mSunComInfo->mContext;
		
	}

	void SunComponent::unInit()
	{
		TODCOMPONENT_UNINITMGR(getSunContext, m_tMgr, T_Sun);
		mSun = nullptr;
	}

	void SunComponent::copyTo(const ActorComponentPtr& actCom)
	{
	}

	void SunComponent::setTimelineAttribute(std::vector<float>& data,const Name& name_)
	{
		if (!m_tMgr)
			return;
		SunComponent* activeCom = m_tMgr->getSunContext().mActiveCom;
		if (activeCom == nullptr || activeCom != this)
			return;
		bool flag = false;

		m_tMgr->setTodComChangeState(T_Sun);
		if (name_ == sunColorName_)
		{
			setSunComColor(Vector4(data[0],data[1],data[2],data[3]));
		}
		else if (name_ == sunIntensityName_)
		{
			setSunIntensity(data[0]);
		}
		else if (name_ == sunEmmitColorName_)
		{
		    setEmmittingColor(Vector4(data[0], data[1], data[2], data[3]));
		}
		else if (name_ == sunInRadiusName_)
		{
			setInRadius(data[0]);
		}
		else if (name_ == sunOutRadiusName_)
		{
			setOutRadius(data[0]);
		}
		else if (name_ == sunOpacityName_)
		{
			setOpacity(data[0]);	
		}
		else if (name_ == sunHDRIntensityName_)
		{
			setHDRIntensity(data[0]);
		}

		SceneManager* sMgr = m_tMgr->GetSceneManager();
		if (sMgr)
		{
			if (flag && sMgr->getSun() == mSun)
			   setSunColor();
		}
			
	}

	void SunComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->pushComponent(m_tMgr->getSunContext(), this);
		else
			m_tMgr->popComponent(m_tMgr->getSunContext(), this);
	}

	void SunComponent::use()
	{
		if (!m_tMgr)
			return;
		mSun = m_tMgr->getSun();
		if (mSun == nullptr)
			return;

		if (m_tMgr->getIsWriteLog())
		{
			std::string logInfo = "";
			logInfo.append("sun : use").append(getCombinedName());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}
	
		setSunColor();
		setEmmittingColor(mContext.mEmmittingColor);
		
	    setInRadius(mContext.mInRadius);
		setOutRadius(mContext.mOutRadius);
		setOpacity(mContext.mOpacity);
		setPriority(mContext.mPriority);
		if(!mContext.mSunTextureName.empty())
		   setSonTexture(mContext.mSunTextureName);
		_setComMoonTexture();
	}

	void SunComponent::unuse()
	{
		mSun = nullptr;
		if (m_tMgr && m_tMgr->getIsWriteLog())
		{
			std::string logInfo = "";
			logInfo.append("Sun : unuse").append(getCombinedName());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}
	}

	void SunComponent::setSunComColor(const Vector4& sunColor_)
	{
		mContext.mSunColor = sunColor_;
		if (!m_tMgr)
			return;
		SunComponent* activeCom = m_tMgr->getSunContext().mActiveCom;
		if (activeCom == nullptr || activeCom != this)
			return;
		setSunColor();
	}

	void SunComponent::setSunIntensity(float intensity_)
	{
		mContext.mSunIntensity = intensity_;
		if (!m_tMgr)
			return;
		SunComponent* activeCom = m_tMgr->getSunContext().mActiveCom;
		if (activeCom == nullptr || activeCom != this)
			return;
		setSunColor();
	}

	void SunComponent::setHDRIntensity(float HdrIntensity_)
	{
		mContext.mHDRIntensity = HdrIntensity_;
		if (!m_tMgr)
			return;

		SunComponent* activeCom = m_tMgr->getSunContext().mActiveCom;
		if (activeCom == nullptr || activeCom != this)
			return;
		setSunColor();
	}

	void SunComponent::setSunColor()
	{
		auto colorTemp = mContext.mSunColor;
		auto color_ = ColorValue(colorTemp.x, colorTemp.y, colorTemp.z, colorTemp.w);
		auto temp = ColorValue::gammaToLinear(color_) * mContext.mSunIntensity;
		bool flag = Root::instance()->getRenderSystem()->getCurRenderStrategy()->getPostProcessChain()->isEnable(PostProcess::HDR);
		if (flag)
		{
			temp *= mContext.mHDRIntensity;
		}
	    
		if (m_tMgr)
		{
			SceneManager* sMgr = m_tMgr->GetSceneManager();
			if(sMgr)
				sMgr->setSunColor(temp);
		}
	}

	Vector4 SunComponent::getSunColor()
	{
		return mContext.mSunColor;
	}


	void SunComponent::setEmmittingColor(const Vector4& color_)
	{
		mContext.mEmmittingColor = color_;
		ColorValue colorTemp = ColorValue(color_.x, color_.y, color_.z, color_.w);
		if(mSun)
		    mSun->setEmissionColour(colorTemp);
		
	}

	Vector4 SunComponent::getEmmittingColor()
	{
		return mContext.mEmmittingColor;
	}

	void SunComponent::setInRadius(const float& inRadius_)
	{
		mContext.mInRadius = inRadius_;
		if(mSun)
		  mSun->setSunInnerRadius(mContext.mInRadius);
	}

	float SunComponent::getInRadius()
	{
		return mContext.mInRadius;
	}

	void SunComponent::setOutRadius(const float& outRadius_)
	{
		mContext.mOutRadius = outRadius_;
		if (mSun == nullptr)
			return;
		mSun->setSunOuterRadius(mContext.mOutRadius);
	}

	float SunComponent::getOutRadius()
	{
		return mContext.mOutRadius;
	}

	void SunComponent::setOpacity(const float& Opacity_)
	{
		mContext.mOpacity = Opacity_;
		if (mSun == nullptr)
			return;
		mSun ->setSunOpacity(mContext.mOpacity);
	}

	float SunComponent::getOpacity()
	{
		return mContext.mOpacity;
	}

	void SunComponent::setPriority(const int & priority_)
	{
		mContext.mPriority = priority_;
		if(m_tMgr)
			m_tMgr->adjustComponentPriority(m_tMgr->getSunContext(), this);
	}

	int SunComponent::getPriority()
	{
		return mContext.mPriority;
	}

	void SunComponent::setDirection(const Vector3 & dir_)
	{
		if (mSun == nullptr)
			return;
		mSun->setSunDirection(dir_);
	}
	void SunComponent::setTransFormTime(float transTime_)
	{
		mContext.mTransFormTime = transTime_;
	}

	float SunComponent::getTransFormTime()
	{
		return mContext.mTransFormTime;
	}

	void SunComponent::setSonTexture(const std::string& sunTextureName_)
	{
		mContext.mSunTextureName = sunTextureName_;
		if (mSun)
			mSun->setSunTextureName(sunTextureName_);
	}

	void SunComponent::setMoonTextures(const std::vector<std::string>& moonTextures)
	{
		mContext.mMoonTextureName = moonTextures;
		if (mSun)
			_setComMoonTexture();
	}


	void SunComponent::_setComMoonTexture()
	{
		if (mSun == nullptr)
			return;
		for (uint32 i = 0; i < mContext.mMoonTextureName.size(); ++i)
		{
			if(!mContext.mMoonTextureName[i].empty())
			   mSun->setMoonTextureName(i, mContext.mMoonTextureName[i]);
		}
	}

};

