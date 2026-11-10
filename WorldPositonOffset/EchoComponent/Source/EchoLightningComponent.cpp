#include "EchoComponentStableHeaders.h"
#include "EchoLightningComponent.h"
#include "EchoTODComponentManager.h"
#include "EchoSceneManager.h"
#include "EchoTODComponentSystem.h"

namespace Echo {

		EDITOR_REFLECT_BEGIN(LightningComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mNormalLightningVisible, "NormalLightningVisible", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mLightningVisible, "LightningVisible", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mLightningLife, "LightningLife", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mLightningDelayed, "LightningDelayed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mLightningInterval, "LightningInterval", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mLightningLimitYawAngle, "LightningLimitYawAngle", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mLightningLimitPitchRange, "LightningLimitPitchRange", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mLightningUseBeam, "LightningUseBeam", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mLightningColor, "LightningColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mLightningColorPower, "LightningColorPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mUseSunColor, "UseSunColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mSunColor, "LightningSunColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mUseSunBackColor, "UseSunBackColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mCloudSunBackColour, "LightningSunBackColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mLightningSunBackColourPower, "LightningSunBackColorPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mLightningTexture, "LightningTexture", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mLightningUseParticle, "LightningUseParticle", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mLightningParticleIsLoop, "LightningParticleIsLoop", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mLightningParticleOffsetRange, "ParticleOffsetRange", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mLightningParticleScaleRange, "ParticleScaleRange", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mLightningParticle, "LightningParticle", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mLightningSound, "LightningSound", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.mPriority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		EDITOR_REFLECT_MEMBER_UPDATE(mLightningHandle.m_BellVecs, "BellsData", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END()
		DECLAREACTORCOMTYPE_CPP(LightningComponent)

	LightningComponentInfo::LightningComponentInfo(const Name& comType, ActorCOMMoveableType type):ActorComponentInfo(comType, type)
	{

	}

	void LightningComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<LightningComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void LightningComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<LightningComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void LightningComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		reflect::TypeResolver<LightningComponentInfo>::get()->copyTo(this, comInfo);
	}

	void LightningComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<LightningComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

#ifdef ECHO_EDITOR
	bool LightningComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		bool isNeedCopy = false;
		if (!mLightningHandle.mLightningTexture.empty())
		{
			for(String& texture: mLightningHandle.mLightningTexture)
				resMap.insert(std::make_pair(ResourceType::NeedCopyRes, texture));
			isNeedCopy = true;
		}

		if (!mLightningHandle.mLightningParticle.empty())
		{
			for (String& particle : mLightningHandle.mLightningParticle)
				resMap.insert(std::make_pair(ResourceType::Particle, particle));
			isNeedCopy = true;
		}

		if (!mLightningHandle.mLightningSound.empty())
		{
			for (String& sound : mLightningHandle.mLightningSound)
				resMap.insert(std::make_pair(ResourceType::NeedCopyRes, sound));
			isNeedCopy = true;
		}
		return isNeedCopy;
	}
#endif


	LightningComponent::LightningComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType) :ActorComponent(pActor, movableType)
	{
		mGlobalEnvironmentTexName = StringTable::instance()->getString("Light", "DefaultLightningTexture");
	}

	LightningComponent::~LightningComponent()
	{

	}

	bool LightningComponent::isCurrentActiveCom()
	{
		if (!m_tMgr)
			return false;
		auto& Context = m_tMgr->getLightningContext();
		if (Context.mActiveCom != nullptr && Context.mActiveCom == this)
			return true;
		return false;
	}

	void LightningComponent::init(ActorComponentInfo* info)
	{
		TODCOMPONENT_INITMGR(getLightningContext, m_tMgr, false);

		LightningComponentInfo* mLightningComponentInfo = static_cast<LightningComponentInfo*>(info);
		if (mLightningComponentInfo == nullptr)
			return;
		mContext = mLightningComponentInfo->mLightningHandle;
	}

	void LightningComponent::unInit()
	{
		TODCOMPONENT_UNINITMGR(getLightningContext, m_tMgr, T_End);
	}

	int LightningComponent::getPriority() const
	{
		return mContext.mPriority;
	};

	String LightningComponent::getGlobalLightningTextureName()
	{
		return mGlobalEnvironmentTexName;
	}

	void LightningComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->pushComponent(m_tMgr->getLightningContext(), this);
		else
			m_tMgr->popComponent(m_tMgr->getLightningContext(), this);
	}

	void LightningComponent::use()
	{
		if (!m_tMgr)
			return;
		auto& Context = m_tMgr->getLightningContext();
		if (Context.mActiveCom == nullptr || Context.mActiveCom != this)
			return;
		auto& obj = mContext;
		setLightningLifeTime(obj.mLightningLife);
		setLightningDelayed(obj.mLightningDelayed);

		setLightningInterval(obj.mLightningInterval);
		setLightningLimitYawAngle(obj.mLightningLimitYawAngle);

		setLightningLimitPitchRange(obj.mLightningLimitPitchRange);
		setLightningUseBeam(obj.mLightningUseBeam);

		setLightningColor(obj.mLightningColor);
		setLightningColorPower(obj.mLightningColorPower);
		setUseSunColor(obj.mUseSunColor);

		setSunColor(obj.mSunColor);
		setUseSunBackColor(obj.mUseSunBackColor);
		setSunBackColor(obj.mCloudSunBackColour);

		setSunBackColorPower(obj.mLightningSunBackColourPower);
		setGlobalLightningTextureName(obj.mLightningTexture);
		setIsUseParticle(obj.mLightningUseParticle);
		setIsParticleLoop(obj.mLightningParticleIsLoop);

		setParticleOffsetRange(obj.mLightningParticleOffsetRange);
		setParticleScaleRange(obj.mLightningParticleScaleRange);
		setParticlesName(obj.mLightningParticle);
		setLightningSoundName(obj.mLightningSound);
		setIsUseNormalLightning(obj.mNormalLightningVisible);
		setIsUseSpecialLightning(obj.mLightningVisible);
		setPriority(obj.mPriority);
		setLightningBells(obj.m_BellVecs);
	}

	void LightningComponent::unuse()
	{
		if (!m_tMgr)
			return;

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if(wMgr)
		{
			wMgr->setLightningVisible(LT_NORMAL, false);
			wMgr->setLightningVisible(LT_SPECIAL, false);
			//WeatherBellPlayer::_ResetWeatherType(wMgr, wMgr->getWeatherType());
		}
	}

	/*void LightningComponent::resetLightning()
	{
		setLightningLifeTime(mContext.mLightningLife);
		setLightningDelayed(mContext.mLightningDelayed);

		setLightningInterval(mContext.mLightningInterval);
		setLightningLimitYawAngle(mContext.mLightningLimitYawAngle);

		setLightningLimitPitchRange(mContext.mLightningLimitPitchRange);
		setLightningUseBeam(mContext.mLightningUseBeam);

		setLightningColor(mContext.mLightningColor);
		setLightningColorPower(mContext.mLightningColorPower);
		setUseSunColor(mContext.mUseSunColor);

		setSunColor(mContext.mSunColor);
		setUseSunBackColor(mContext.mUseSunBackColor);
		setSunBackColor(mContext.mCloudSunBackColour);

		setSunBackColorPower(mContext.mLightningSunBackColourPower);
		setGlobalLightningTextureName(mContext.mLightningTexture);
		setIsUseParticle(mContext.mLightningUseParticle);
		setIsParticleLoop(mContext.mLightningParticleIsLoop);

		setParticleOffsetRange(mContext.mLightningParticleOffsetRange);
		setParticleScaleRange(mContext.mLightningParticleScaleRange);
		setParticlesName(mContext.mLightningParticle);
		setLightningSoundName(mContext.mLightningSound);
		setPriority(mContext.mPriority);
	}*/

	Name LightningLife_ = Name("LightningLife");
	Name LightningInterval_ = Name("LightningInterval");
	Name LightningLimitYawAngle_ = Name("LightningLimitYawAngle");
	Name LightningLimitPitchRange_ = Name("LightningLimitPitchRange");
	Name LightningColor_ = Name("LightningColor");
	Name LightningColorPower_ = Name("LightningColorPower");
	Name LightningSunColor_ = Name("LightningSunColor");
	Name LightningSunBackColor_ = Name("LightningSunBackColor");
	Name LightningSunBackColorPower_ = Name("LightningSunBackColorPower");
	Name ParticleOffsetRange_ = Name("ParticleOffsetRange");
	Name ParticleScaleRange_ = Name("ParticleScaleRange");
	void LightningComponent::setTimelineAttribute(std::vector<float>& data, const Name& name_)
	{
		if (name_ == LightningLife_)
		{
			setLightningLifeTime(data[0]);
		}
		else if (name_ == LightningInterval_)
		{
			setLightningInterval(Vector2(data[0], data[1]));
		}
		else if (name_ == LightningLimitYawAngle_)
		{
			setLightningLimitYawAngle(data[0]);
		}
		else if (name_ == LightningLimitPitchRange_)
		{
			setLightningLimitPitchRange(Vector2(data[0], data[1]));
		}
		else if (name_ == LightningColor_)
		{
			ColorValue Color(data[0], data[1], data[2], data[3]);
			setLightningColor(Color);
		}
		else if (name_ == LightningColorPower_)
		{
			setLightningColorPower(data[0]);
		}
		else if (name_ == LightningSunColor_)
		{
			ColorValue Color(data[0], data[1], data[2], data[3]);
			setSunColor(Color);
		}
		else if (name_ == LightningSunBackColor_)
		{
			ColorValue Color(data[0], data[1], data[2], data[3]);
			setSunBackColor(Color);
		}
		else if (name_ == LightningSunBackColorPower_)
		{
			setSunBackColorPower(data[0]);
		}
		else if (name_ == ParticleOffsetRange_)
		{
			setParticleOffsetRange(Vector2(data[0], data[1]));
		}
		else if (name_ == ParticleScaleRange_)
		{
			setParticleScaleRange(Vector2(data[0], data[1]));
		}
	}

	void LightningComponent::setPriority(const int& val)
	{
		int prior = val;
		if (prior > 255) prior = 255;
		else if (prior < 0) prior = 0;
		mContext.mPriority = prior;
		if(m_tMgr)
			m_tMgr->adjustComponentPriority(m_tMgr->getLightningContext(), this);
	}

	void LightningComponent::setIsUseNormalLightning(const bool& visible)
	{
		mContext.mNormalLightningVisible = visible;
		if (!isCurrentActiveCom())
			return;

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningVisible(LT_NORMAL, visible);
		}
	}

	void LightningComponent::setIsUseSpecialLightning(const bool& visible)
	{
		mContext.mLightningVisible = visible;
		if (!isCurrentActiveCom())
			return;

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningVisible(LT_SPECIAL, visible);
		}
	}

	void LightningComponent::setLightningLifeTime(const float& LightningLife)
	{
		mContext.mLightningLife = LightningLife;
		if (!isCurrentActiveCom())
			return;

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningLifeTime(type, LightningLife);
		}

	}

	void LightningComponent::setLightningDelayed(const bool& lightningDelayed)
	{
		mContext.mLightningDelayed = lightningDelayed;
		if (!isCurrentActiveCom())
			return;

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningDelayed(type, lightningDelayed);
		}

	}

	void LightningComponent::setLightningInterval(const Vector2& Interval)
	{
		mContext.mLightningInterval = Interval;
		if (!isCurrentActiveCom())
			return;
		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningInterval(type, Interval);
		}
	}

	void LightningComponent::setLightningLimitYawAngle(const float& LightningLimitYawAngle)
	{
		mContext.mLightningLimitYawAngle = LightningLimitYawAngle;
		if (!isCurrentActiveCom())
			return;

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningLimitYawAngle(type, LightningLimitYawAngle);
		}

	}

	void LightningComponent::setLightningLimitPitchRange(const Vector2& Range)
	{
		mContext.mLightningLimitPitchRange = Range;
		if (!isCurrentActiveCom())
			return;

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningLimitPitchRange(type, Range);
		}

	}

	void LightningComponent::setLightningUseBeam(const bool& isUseBeam)
	{
		mContext.mLightningUseBeam = isUseBeam;
		if (!isCurrentActiveCom())
			return;
		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningUseBeam(type, isUseBeam);
		}
	}

	void LightningComponent::setLightningColor(const ColorValue& color)
	{
		mContext.mLightningColor = color;
		if (!isCurrentActiveCom())
			return;

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningColor(type, color);
		}
	}

	void LightningComponent::setLightningColorPower(const float& lightningColorPower)
	{
		mContext.mLightningColorPower = lightningColorPower;
		if (!isCurrentActiveCom())
			return;

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningColorPower(type, lightningColorPower);
		}

	}

	void LightningComponent::setUseSunColor(const bool& isUseSunColor)
	{
		mContext.mUseSunColor = isUseSunColor;

		if (!isCurrentActiveCom())
			return;
		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningUseSunColor(type, isUseSunColor);
		}
	}

	void LightningComponent::setSunColor(const ColorValue& color)
	{
		mContext.mSunColor = color;

		if (!isCurrentActiveCom())
			return;

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			ColorValue temp = ColorValue::gammaToLinear(color);
			wMgr->setLightningSunColor(type, temp);
		}
	}

	void LightningComponent::setUseSunBackColor(const bool& useSunBackColor)
	{
		mContext.mUseSunBackColor = useSunBackColor;

		if (!isCurrentActiveCom())
			return;

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningUseSunBackColor(type, useSunBackColor);
		}
	}

	void LightningComponent::setSunBackColor(const ColorValue& color)
	{
		mContext.mCloudSunBackColour = color;

		if (!isCurrentActiveCom())
			return;

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningSunBackColour(type, color * mContext.mLightningSunBackColourPower);
		}
	}

	void LightningComponent::setSunBackColorPower(const float& power)
	{
		mContext.mLightningSunBackColourPower = power;

		if (!isCurrentActiveCom())
			return;

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningSunBackColour(type, mContext.mCloudSunBackColour * power);
		}

	}

	void LightningComponent::setGlobalLightningTextureName(const std::vector<String>& names)
	{
		mContext.mLightningTexture = names;
		if (!isCurrentActiveCom())
			return;

	/*	if (clearEmptyName(names).empty())
			return;*/

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningTextureName(type, clearEmptyName(names));
		}

	}

	void LightningComponent::setIsUseParticle(const bool& isUse)
	{
		mContext.mLightningUseParticle = isUse;
		if (!isCurrentActiveCom())
			return;

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningUseParticle(type, isUse);
		}
	}

	void LightningComponent::setIsParticleLoop(const bool& isLoop)
	{
		mContext.mLightningParticleIsLoop = isLoop;
		if (!isCurrentActiveCom())
			return;

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningParticleLoop(type, isLoop);
		}

	}

	void LightningComponent::setParticleOffsetRange(const Vector2& range)
	{
		mContext.mLightningParticleOffsetRange = range;
		if (!isCurrentActiveCom())
			return;

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningParticleOffsetRange(type, range);
		}
	}

	void LightningComponent::setParticleScaleRange(const Vector2& ScaleRange)
	{
		mContext.mLightningParticleScaleRange = ScaleRange;

		if (!isCurrentActiveCom())
			return;
		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningParticleScaleRange(type, ScaleRange);
		}
	}

	void LightningComponent::setParticlesName(const std::vector<String>& ParticlesName)
	{
		mContext.mLightningParticle = ParticlesName;

		if (!isCurrentActiveCom())
			return;

		/*if (clearEmptyName(ParticlesName).empty())
			return;*/

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningParticleName(type, clearEmptyName(ParticlesName));
		}
	}

	void LightningComponent::setLightningSoundName(const std::vector<String>& SoundName)
	{
		mContext.mLightningSound = SoundName;

		if (!isCurrentActiveCom())
			return;

		if (clearEmptyName(SoundName).empty())
			return;

		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (wMgr)
		{
			wMgr->setLightningSound(type, clearEmptyName(SoundName));
		}

	}

	void LightningComponent::setLightningBells(const std::vector<WeatherBellData>& bellVec_)
	{
		mContext.m_BellVecs = bellVec_;

		if (!isCurrentActiveCom())
			return;
		std::vector<WeatherBellData> bellData_;
		_getResBellsData(bellData_);
		WeatherManager* wMgr = m_tMgr->GetWeatherManager();
		if (!bellData_.empty() && wMgr)
		{
			wMgr->setLightningBells(type,bellData_);
		}
	}

	std::vector<String> LightningComponent::clearEmptyName(const std::vector<String>& names)
	{
		std::vector<String> temp;
		for (int i = 0; i < names.size(); i++)
			if (names[i].size() > 3)//有效的文件应该至少有后缀，.ogg,.dds,.pef等
			{
				temp.emplace_back(names[i]);
			}
		return temp;
	}
	void LightningComponent::_getResBellsData(std::vector<WeatherBellData>& bellData_)
	{
		for (int i = 0; i < mContext.m_BellVecs.size(); ++i)
		{
			if (!mContext.m_BellVecs[i].bankName.empty() && !mContext.m_BellVecs[i].eventName.empty())
			{
				bellData_.emplace_back(mContext.m_BellVecs[i]);
			}
		}
	}





	void  WriteJSNode(JSNODE jsonNode, const char* Name, const WeatherBellData& bellData)
	{
		if (jsonNode == nullptr)
			return;
		cJSON* triggerEventDesc_ = cJSON_CreateObject();
		cJSON_AddItemToObject(jsonNode, "FMODBellDatas", triggerEventDesc_);
		if (triggerEventDesc_)
		{
			WriteJSNode(triggerEventDesc_, "BankName", bellData.bankName);
			WriteJSNode(triggerEventDesc_, "EventName", bellData.eventName);
		}
	}
	bool  ReadJSNode(cJSON* jsonNode, const char* Name, WeatherBellData& bellData)
	{
		if (jsonNode == nullptr)
			return false;
		cJSON* triggerEventDesc_ = cJSON_GetObjectItem(jsonNode, "FMODBellDatas");
		if (triggerEventDesc_)
		{
			ReadJSNode(triggerEventDesc_, "BankName", bellData.bankName);
			ReadJSNode(triggerEventDesc_, "EventName", bellData.eventName);
			return true;
		}
		return false;
	}
}
