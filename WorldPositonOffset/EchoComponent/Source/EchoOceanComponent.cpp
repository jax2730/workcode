#include "EchoComponentStableHeaders.h"
#include "EchoOceanComponent.h"
#include "EchoPrerequisites.h"

#include "EchoFFTOcean.h"
#include "EchoTODComponentManager.h"
#include "EchoLogManager.h"
#include "EchoTODComponentSystem.h"
#include "EchoTODDataUtil.h"
#include "EchoStringTable.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(OceanComponent)
		EDITOR_REFLECT_BEGIN(OceanComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mOceanScale, "OceanScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mMeshFile, "MeshFile", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mNormalTexName, "NormalTexName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFoamTexName, "FoamTexName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFresnelBias, "FresnelBias", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFresnelIntensity, "FresnelIntensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFresnelRatio, "FresnelRatio", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTextureScale, "TextureScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mReflectionAmount, "ReflectionAmount", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mWaveAmount, "WaveAmount", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mLayer, "Layer", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mVisible, "OceanVisible", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mOceanHeight, "OceanHeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mWaveFrequency, "WaveFrequency", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mWaveAmplitude, "WaveAmplitude", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mBumpScale, "BumpScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRefractAmount, "RefractAmount", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mBorderSoft, "BorderSoft", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mReflectTextureName, "ReflectTextureName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSpecularPower, "SpecularPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSpecularStrength, "SpecularStrength", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFoamBias, "FoamBias", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFoamSoft, "FoamSoft", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFoamTilling, "FoamTilling", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mUnderWaterFogDensity, "UnderWaterFogDensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mUnderWaterFogStart, "UnderWaterFogStart", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mUnderWaterFogEnd, "UnderWaterFogEnd", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mWaterShallowColor, "WaterShallowColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mWaterDeepColor, "WaterDeepColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mWaterFoamColor, "WaterFoamColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mWaterReflectColor, "WaterReflectColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mUnderWaterColor, "UnderWaterColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mUnderWaterFogColor, "UnderWaterFogColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()

	OceanComponentInfo::OceanComponentInfo()
	{
	}

	OceanComponentInfo::OceanComponentInfo(const Name& comType, ActorCOMMoveableType type)
		: ActorComponentInfo(comType, type)
	{
	}

	OceanComponentInfo::~OceanComponentInfo()
	{

	}

	void OceanComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<OceanComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void OceanComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<OceanComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void OceanComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		OceanComponentInfo* info = static_cast<OceanComponentInfo*>(comInfo);
		reflect::TypeResolver<OceanComponentInfo>::get()->copyTo(this, info);
	}

	void OceanComponentInfo::Overlay(ActorComponentInfo* srcInfo)
	{
	}

	void OceanComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<OceanComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

	Name waterShallowColorName	= Name("WaterShallowColor");
	Name waterDeepColorName		= Name("WaterDeepColor");
	Name waterFoamColorName		= Name("WaterFoamColor");
	Name waterReflectColorName	= Name("WaterReflectColor");
	Name underWaterColorName	= Name("UnderWaterColor");
	Name underWaterFogColorName = Name("UnderWaterFogColor");

	OceanComponent::OceanComponent(ActorPtr  pActor, ActorCOMMoveableType movableType)
		:ActorComponent(pActor, movableType)
	{
#ifdef ECHO_EDITOR
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_OCEAN_SHALLOW_COLOR,			waterShallowColorName.toString(),	4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_OCEAN_DEEP_COLOR,				waterDeepColorName.toString(),		4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_OCEAN_FOAM_COLOR,				waterFoamColorName.toString(),		4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_OCEAN_REFLECT_COLOR,			waterReflectColorName.toString(),	4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_OCEAN_UNDERWATER_OCEAN_COLOR,	underWaterColorName.toString(),		4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_OCEAN_UNDERWATER_FOG_COLOR,	underWaterFogColorName.toString(),	4);
#endif // ECHO_EDITOR
	}
	OceanComponent::~OceanComponent()
	{
	}
	void OceanComponent::init(ActorComponentInfo* info)
	{
		TODCOMPONENT_INITSCENEMGR(getOceanContext, m_tMgr, false, mSceneManager);

		OceanComponentInfo* mOceanComponentInfo = static_cast<OceanComponentInfo*>(info);

		if (mOceanComponentInfo == nullptr)
			return;
		mContext = mOceanComponentInfo->mContext;
	
	}
	void OceanComponent::unInit()
	{
		if (!m_tMgr || !mSceneManager)
			return;
		TODCOMPONENT_UNINITMGR(getOceanContext, m_tMgr, T_End);
		Ocean* tempOcean = mSceneManager->getOcean();
		if (mOceanHandle)
			delete mOceanHandle;
		mOceanHandle = nullptr;
		if(tempOcean)
			mSceneManager->setOcean(tempOcean);

		mSceneManager = nullptr;
	}
	void OceanComponent::use()
	{
		if (!m_tMgr)
			return;

		mOceanHandle = m_tMgr->getOcean();
		if (mOceanHandle == nullptr)
			return;

		if (m_tMgr->getIsWriteLog())
		{
			std::string logInfo = "";
			logInfo.append("Ocean : use").append(getActor()->getCombinedID()).append(" : ").append(getComNameToString());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}
		//mSceneManager = Root::instance()->getMainSceneManager();
		//mOceanHandle->init();

		if (m_pActComInfo == nullptr)
			return;
		OceanComponentInfo* mOceanComponentInfo = static_cast<OceanComponentInfo*>(m_pActComInfo);

		if (mOceanComponentInfo == nullptr)
			return;

		if (mContext.mLayer)
		{
			unsigned int layer = mContext.mLayer;
			mOceanHandle->setVisibilityFlags(1 << layer);
		}

		setVisibleImpl(mContext.mVisible);

		setOceanScale(mContext.mOceanScale);
		setMeshFile(mContext.mMeshFile);
		setNormalTexName(mContext.mNormalTexName);
		setFoamTexName(mContext.mFoamTexName);

		setFresnelBias(mContext.mFresnelBias);
		setFresnelIntensity(mContext.mFresnelIntensity);
		setFresnelRatio(mContext.mFresnelRatio);
		setTextureScale(mContext.mTextureScale);
		setWaveAmount(mContext.mWaveAmount);
		setReflectionAmount(mContext.mReflectionAmount);
		mOceanHandle->setCausticTexture();

		setOceanHeight(mContext.mOceanHeight);
		setWaveFrequency(mContext.mWaveFrequency);
		setWaveAmplitude(mContext.mWaveAmplitude);
		setBumpScale(mContext.mBumpScale);
		setRefractAmount(mContext.mRefractAmount);
		setBorderSoft(mContext.mBorderSoft);
		setReflectTextureName(mContext.mReflectTextureName);

		setSpecularPower(mContext.mSpecularPower);
		setSpecularStrength(mContext.mSpecularStrength);

		setFoamBias(mContext.mFoamBias);
		setFoamSoft(mContext.mFoamSoft);
		setFoamTilling(mContext.mFoamTilling);

		setUnderWaterFogDensity(mContext.mUnderWaterFogDensity);
		setUnderWaterFogStart(mContext.mUnderWaterFogStart);
		setUnderWaterFogEnd(mContext.mUnderWaterFogEnd);

		setWaterShallowColor(mContext.mWaterShallowColor);
		setWaterDeepColor(mContext.mWaterDeepColor);
		setWaterFoamColor(mContext.mWaterFoamColor);
		setWaterReflectColor(mContext.mWaterReflectColor);
		setUnderWaterColor(mContext.mUnderWaterColor);
		setUnderWaterFogColor(mContext.mUnderWaterFogColor);
		setPriority(mContext.mPriority);
	}

	void OceanComponent::unuse()
	{
		mOceanHandle = nullptr;
		if (m_tMgr && m_tMgr->getIsWriteLog())
		{
			std::string logInfo = "";
			logInfo.append("Ocean : unuse").append(getActor()->getCombinedID()).append(" : ").append(getComNameToString());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}
	}

	void OceanComponent::setTimelineAttribute(std::vector<float>& data,const Name& name_)
	{
		if (!m_tMgr)
			return;

		m_tMgr->setTodComChangeState(T_Ocean);
		OceanComponent* activeCom = m_tMgr->getOceanContext().mActiveCom;
		if (activeCom == nullptr || activeCom != this)
			return;
		if (m_pActComInfo == nullptr)
			return;
		OceanComponentInfo* pComInfo = static_cast<OceanComponentInfo*>(this->m_pActComInfo);

		if (name_ == waterShallowColorName) {
			pComInfo->setWaterShallowColor(ColorValue(data[0], data[1], data[2], data[3]));
			this->setWaterShallowColor(pComInfo->getWaterShallowColor());
		}
		else if (name_ == waterDeepColorName) {
			pComInfo->setWaterDeepColor(ColorValue(data[0], data[1], data[2], data[3]));
			this->setWaterDeepColor(pComInfo->getWaterDeepColor());
		}
		else if (name_ == waterFoamColorName) {
			pComInfo->setWaterFoamColor(ColorValue(data[0], data[1], data[2], data[3]));
			this->setWaterFoamColor(pComInfo->getWaterFoamColor());
		}
		else if (name_ == waterReflectColorName) {
			pComInfo->setWaterReflectColor(ColorValue(data[0], data[1], data[2], data[3]));
			this->setWaterReflectColor(pComInfo->getWaterReflectColor());
		}
		else if (name_ == underWaterColorName) {
			pComInfo->setUnderWaterColor(ColorValue(data[0], data[1], data[2], data[3]));
			this->setUnderWaterColor(pComInfo->getUnderWaterColor());
		}

		else if (name_ == underWaterFogColorName) {
			pComInfo->setUnderWaterFogColor(ColorValue(data[0], data[1], data[2], data[3]));
			this->setUnderWaterFogColor(pComInfo->getUnderWaterFogColor());
		}
	}

	void OceanComponent::copyTo(const ActorComponentPtr& pcomp)
	{
	}

	void OceanComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->pushComponent(m_tMgr->getOceanContext(), this);
		else
			m_tMgr->popComponent(m_tMgr->getOceanContext(), this);
	}

	void OceanComponent::setVisibleImpl(bool& value)
	{
		if (mOceanHandle)
			mOceanHandle->setVisible(value);
		mContext.setVisible(value);
	}

	void OceanComponent::setOceanScale(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setScale(value);
		mContext.setOcenScale(value);
	}

	void OceanComponent::setMeshFile(Echo::String& value)
	{
		if (mOceanHandle)
			mOceanHandle->setMeshFile(value);
		mContext.setMeshFile(value);
	}

	void OceanComponent::setNormalTexName(Echo::String& value)
	{
		if (mOceanHandle)
			mOceanHandle->setNormalTexture(value);
		mContext.setNormalTexName(value);
	}

	void OceanComponent::setFoamTexName(Echo::String& value)
	{
		if (mOceanHandle)
			mOceanHandle->setFoamTexture(value);
		mContext.setFoamTexName(value);
	}

	void OceanComponent::setFresnelBias(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setFresnelBias(value);
		mContext.setFresnelBias(value);
	}

	void OceanComponent::setFresnelIntensity(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setFresnelIntensity(value);
		mContext.setFresnelIntensity(value);
	}

	void OceanComponent::setFresnelRatio(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setFresnelRatio(value);
		mContext.setFresnelRatio(value);
	}

	void OceanComponent::setTextureScale(Echo::Vector2& value)
	{
		if (mOceanHandle)
			mOceanHandle->setTextureScale(value);
		mContext.setTextureScale(value);
	}

	void OceanComponent::setReflectionAmount(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setReflectionAmount(value);
		mContext.setReflectionAmount(value);
	}

	void OceanComponent::setWaveAmount(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setWaveAmount(value);
		mContext.setWaveAmount(value);
	}

	void OceanComponent::setLayer(int& value)
	{
		unsigned int layer = value;
		if (mOceanHandle)
			mOceanHandle->setVisibilityFlags(1 << layer);
		mContext.setLayer(value);
	}

	void OceanComponent::setPriority(int priority)
	{
		mContext.setPriority(priority);
		if(m_tMgr)
		   m_tMgr->adjustComponentPriority(m_tMgr->getOceanContext(), this);
	}

	void OceanComponent::setOceanHeight(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setOceanHeight(value);
		mContext.setOceanHeight(value);
	}
	void OceanComponent::setWaveFrequency(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setWaveFrequency(value);
		mContext.setWaveFrequency(value);
	}
	void OceanComponent::setWaveAmplitude(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setWaveAmplitude(value);
		mContext.setWaveAmplitude(value);
	}
	void OceanComponent::setBumpScale(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setBumpScale(value);
		mContext.setBumpScale(value);
	}
	void OceanComponent::setRefractAmount(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setRefractAmount(value);
		mContext.setRefractAmount(value);
	}
	void OceanComponent::setBorderSoft(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setBorderSoft(value);
		mContext.setBorderSoft(value);
	}
	void OceanComponent::setReflectTextureName(const String& value)
	{
		if (mOceanHandle)
			mOceanHandle->setReflectTexture(value);
		mContext.setReflectTextureName(value);
	}

	void OceanComponent::setSpecularPower(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setSpecularPower(value);
		mContext.setSpecularPower(value);
	}
	void OceanComponent::setSpecularStrength(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setSpecularStrength(value);
		mContext.setSpecularStrength(value);
	}

	void OceanComponent::setFoamBias(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setFoamBias(value);
		mContext.setFoamBias(value);
	}
	void OceanComponent::setFoamSoft(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setFoamSoft(value);
		mContext.setFoamSoft(value);
	}
	void OceanComponent::setFoamTilling(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setFoamTilling(value);
		mContext.setFoamTilling(value);
	}

	void OceanComponent::setUnderWaterFogDensity(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setUnderWaterFogDensity(value);
		mContext.setUnderWaterFogDensity(value);
	}
	void OceanComponent::setUnderWaterFogStart(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setUnderWaterFogStart(value);
		mContext.setUnderWaterFogStart(value);
	}
	void OceanComponent::setUnderWaterFogEnd(float& value)
	{
		if (mOceanHandle)
			mOceanHandle->setUnderWaterFogEnd(value);
		mContext.setUnderWaterFogEnd(value);
	}

	//TOD属性
	void OceanComponent::setWaterShallowColor(const ColorValue& val)
	{
		if (mSceneManager)
			mSceneManager->setWaterShallowColor(val);
		mContext.setWaterShallowColor(val);
	}
	void OceanComponent::setWaterDeepColor(const ColorValue& val)
	{
		if (mSceneManager)
			mSceneManager->setWaterDeepColor(val);
		mContext.setWaterDeepColor(val);
	}
	void OceanComponent::setWaterFoamColor(const ColorValue& val) 
	{
		if (mSceneManager)
			mSceneManager->setWaterFoamColor(val);
		mContext.setWaterFoamColor(val);
	}
	void OceanComponent::setWaterReflectColor(const ColorValue& val)
	{
		if (mSceneManager)
			mSceneManager->setWaterReflectColor(val);
		mContext.setWaterReflectColor(val);
	}
	void OceanComponent::setUnderWaterColor(const ColorValue& val)
	{
		if (mSceneManager)
			mSceneManager->setUnderWaterColor(val);
		mContext.setUnderWaterColor(val);
	}
	void OceanComponent::setUnderWaterFogColor(const ColorValue& val)
	{
		if (mOceanHandle)
			mOceanHandle->setUnderWaterFogColor(val);
		mContext.setUnderWaterFogColor(val);
	}

	void OceanComponent::setbFFTOcean(bool& value)
	{
		Root::instance()->m_bFFTOcean = value;
	}

#ifdef ECHO_EDITOR
	bool OceanComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		bool isNeedCopy = false;
		if (!mContext.mMeshFile.empty())
		{
			resMap.insert(std::make_pair(ResourceType::Mesh, mContext.mMeshFile));
			isNeedCopy = true;
		}

		if (!mContext.mNormalTexName.empty())
		{
			resMap.insert(std::make_pair(ResourceType::NeedCopyRes, mContext.mNormalTexName));
			isNeedCopy = true;
		}

		if (!mContext.mFoamTexName.empty())
		{
			resMap.insert(std::make_pair(ResourceType::NeedCopyRes, mContext.mFoamTexName));
			isNeedCopy = true;
		}

		if (!mContext.mReflectTextureName.empty())
		{
			resMap.insert(std::make_pair(ResourceType::NeedCopyRes, mContext.mReflectTextureName));
			isNeedCopy = true;
		}

		return isNeedCopy;
	}
#endif
	OceanComponentContext::OceanComponentContext()
	{
		if (!StringTable::Alive()) return;

		mMeshFile = StringTable::instance()->getString("Ocean", "DefaultMesh");
		mNormalTexName = StringTable::instance()->getString("Ocean", "DefaultNormalTexture");
		mFoamTexName = StringTable::instance()->getString("Ocean", "DefaultFoamTexture");
		mReflectTextureName = StringTable::instance()->getString("Ocean", "DefaultReflectTexture");
	}
}
