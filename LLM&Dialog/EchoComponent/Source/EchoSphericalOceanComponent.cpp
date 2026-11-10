#include "EchoComponentStableHeaders.h"
#include "EchoSphericalOceanComponent.h"
#include "EchoSphericalOcean.h"
#include "EchoSphericalTerrainManager.h"
#include "EchoTODComponentManager.h"
#include "EchoTODComponentSystem.h"
#include "EchoTODDataUtil.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(SphericalOceanComponent);

	String SphericalOceanComponent::WATERSHALLOWCOLOR		= String("watershallowcolor");
	String SphericalOceanComponent::WATERDEEPCOLOR			= String("waterdeepcolor");
	String SphericalOceanComponent::WATERFOAMCOLOR			= String("waterfoamcolor");
	String SphericalOceanComponent::WATERREFLECTCOLOR		= String("waterreflectcolor");
	String SphericalOceanComponent::UNDERWATERCOLOR			= String("underwatercolor");
	String SphericalOceanComponent::UNDERWATERFOGCOLOR		= String("underwaterfogcolor");
	String SphericalOceanComponent::PRIORITY				= String("priority");
	String SphericalOceanComponent::TRANSFORMTIME			= String("transformtime");

	SphericalOceanComponent::SphericalOceanComponent(ActorPtr pActor, ActorCOMMoveableType movableType)
		: ActorComponent(pActor, movableType)
		, mTMgr(nullptr)
	{
#ifdef ECHO_EDITOR
		// TODO
		// 
		// TODDataUtil::SetTodTemplate(TodDataID::PARAM_SKY_BOTTOM_COLOR, skyBottomColorName_.toString(), 4);
#endif
	}

	SphericalOceanComponent::~SphericalOceanComponent()
	{

	}

	void SphericalOceanComponent::init(ActorComponentInfo* info)
	{
		SphericalOceanComponentInfo* comInfo = static_cast<SphericalOceanComponentInfo*>(info);
		if (comInfo != nullptr)
			mContext = comInfo->mContext;
		TODCOMPONENT_INITMGR(getSphericalOceanContext, mTMgr, false);
	}

	void SphericalOceanComponent::unInit()
	{
		TODCOMPONENT_UNINITMGR(getSphericalOceanContext, mTMgr, T_SphericalOcean);
	}

	void SphericalOceanComponent::use()
	{
		if (!mTMgr) return;

		if (mTMgr->getIsWriteLog())
		{
			std::string logInfo = "";
			logInfo.append("SphericalOcean : use").append(getCombinedName());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}

		SetWaterShallowColor(mContext.mWaterShallowColor);
		SetWaterDeepColor(mContext.mWaterDeepColor);
		SetWaterFoamColor(mContext.mWaterFoamColor);
		SetWaterReflectColor(mContext.mWaterReflectColor);
		SetUnderWaterColor(mContext.mUnderWaterColor);
		SetUnderWaterFogColor(mContext.mUnderWaterFogColor);
	}

	void SphericalOceanComponent::unuse()
	{
		if (mTMgr && mTMgr->getIsWriteLog())
		{
			std::string logInfo = "";
			logInfo.append("SphericalOcean : unuse").append(getCombinedName());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}
	}

	void SphericalOceanComponent::SetWaterShallowColor(const ColorValue& value)
	{
		mContext.mWaterShallowColor = value;
		if (!isCurrentActiveCom()) return;

		SphericalOcean* sOcean = GetCurrentSphericalOcean();
		if (sOcean)
		{
			SphereOceanShadeSettings setting;
			sOcean->getShadeSetting(setting);
			setting.shallowColor = value;
			sOcean->setShadeSetting(setting);
		}
	}

	void SphericalOceanComponent::SetWaterDeepColor(const ColorValue& value)
	{
		mContext.mWaterDeepColor = value;
		if (!isCurrentActiveCom()) return;

		SphericalOcean* sOcean = GetCurrentSphericalOcean();
		if (sOcean)
		{
			SphereOceanShadeSettings setting;
			sOcean->getShadeSetting(setting);
			setting.deepColor = value;
			sOcean->setShadeSetting(setting);
		}
	}

	void SphericalOceanComponent::SetWaterFoamColor(const ColorValue& value)
	{
		mContext.mWaterFoamColor = value;
		if (!isCurrentActiveCom()) return;

		SphericalOcean* sOcean = GetCurrentSphericalOcean();
		if (sOcean)
		{
			SphereOceanShadeSettings setting;
			sOcean->getShadeSetting(setting);
			setting.m_foamColor = value;
			sOcean->setShadeSetting(setting);
		}
	}

	void SphericalOceanComponent::SetWaterReflectColor(const ColorValue& value)
	{
		mContext.mWaterReflectColor = value;
		if (!isCurrentActiveCom()) return;

		SphericalOcean* sOcean = GetCurrentSphericalOcean();
		if (sOcean)
		{
			SphereOceanShadeSettings setting;
			sOcean->getShadeSetting(setting);
			setting.m_reflectColor = value;
			sOcean->setShadeSetting(setting);
		}
	}

	void SphericalOceanComponent::SetUnderWaterColor(const ColorValue& value)
	{
		mContext.mUnderWaterColor = value;
		if (!isCurrentActiveCom()) return;

		SphericalOcean* sOcean = GetCurrentSphericalOcean();
		if (sOcean)
		{
			SphereOceanShadeSettings setting;
			sOcean->getShadeSetting(setting);
			setting.m_underWaterColor = value;
			sOcean->setShadeSetting(setting);
		}
	}

	void SphericalOceanComponent::SetUnderWaterFogColor(const ColorValue& value)
	{
		mContext.mUnderWaterFogColor = value;
		if (!isCurrentActiveCom()) return;

		SphericalOcean* sOcean = GetCurrentSphericalOcean();
		if (sOcean)
		{
			SphereOceanShadeSettings setting;
			sOcean->getShadeSetting(setting);
			// TODO
			sOcean->setShadeSetting(setting);
		}
	}

	void SphericalOceanComponent::setPriority(int priority)
	{
		mContext.mPriority = priority;
		if (mTMgr)
			mTMgr->adjustComponentPriority(mTMgr->getSphericalOceanContext(), this);
	}

	void SphericalOceanComponent::setTransFormTime(float value)
	{
		mContext.mTransFormTime = value;
	}

	void SphericalOceanComponent::setTimelineAttribute(std::vector<float>& data, const Name& name)
	{
		if (!mTMgr) return;

		if (!isCurrentActiveCom()) return;

		mTMgr->setTodComChangeState(T_SphericalOcean);

		if(name == Name(WATERSHALLOWCOLOR))
		{
			SetWaterShallowColor(ColorValue(data[0], data[1], data[2], data[3]));
		}
		else if(name == Name(WATERDEEPCOLOR))
		{
			SetWaterDeepColor(ColorValue(data[0], data[1], data[2], data[3]));
		}
		else if(name == Name(WATERFOAMCOLOR))
		{
			SetWaterFoamColor(ColorValue(data[0], data[1], data[2], data[3]));
		}
		else if(name == Name(WATERREFLECTCOLOR))
		{
			SetWaterReflectColor(ColorValue(data[0], data[1], data[2], data[3]));
		}
		else if(name == Name(UNDERWATERCOLOR))
		{
			SetUnderWaterColor(ColorValue(data[0], data[1], data[2], data[3]));
		}
		else if (name == Name(UNDERWATERFOGCOLOR))
		{
			SetUnderWaterFogColor(ColorValue(data[0], data[1], data[2], data[3]));
		}
	}

	void SphericalOceanComponent::visibleStateChanged(bool isVisible)
	{
		if (!mTMgr) return;

		if (isVisible)
			mTMgr->pushComponent(mTMgr->getSphericalOceanContext(), this);
		else
			mTMgr->popComponent(mTMgr->getSphericalOceanContext(), this);
	}

	SphericalOcean* SphericalOceanComponent::GetCurrentSphericalOcean()
	{
		if (!mTMgr) return nullptr;

		return mTMgr->getCurrentSphericalOcean();
	}

	bool SphericalOceanComponent::isCurrentActiveCom()
	{
		if (!mTMgr)
			return false;

		if (Root::instance()->getActiveSceneManager() != mTMgr->GetSceneManager())
			return false;

		SphericalOcean* sOcean = GetCurrentSphericalOcean();
		if (!sOcean /*|| !sOcean->getIsUseTod()*/) return false;

		auto& Context = mTMgr->getSphericalOceanContext();
		if (Context.mActiveCom != nullptr && Context.mActiveCom == this)
			return true;
		return false;
	}

	//------------------------------------------------------------- Info --------------------------------------------------

	EDITOR_REFLECT_BEGIN(SphericalOceanComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, SphericalOceanComponent::PRIORITY, EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTransFormTime, SphericalOceanComponent::TRANSFORMTIME, EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mWaterShallowColor, SphericalOceanComponent::WATERSHALLOWCOLOR, EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mWaterDeepColor, SphericalOceanComponent::WATERDEEPCOLOR, EditorUI_DRAG, nullptr,reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mWaterFoamColor, SphericalOceanComponent::WATERFOAMCOLOR, EditorUI_DRAG, nullptr,reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mWaterReflectColor, SphericalOceanComponent::WATERREFLECTCOLOR, EditorUI_DRAG, nullptr,reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mUnderWaterColor, SphericalOceanComponent::UNDERWATERCOLOR, EditorUI_DRAG, nullptr,reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mUnderWaterFogColor, SphericalOceanComponent::UNDERWATERFOGCOLOR, EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()

	SphericalOceanComponentInfo::SphericalOceanComponentInfo(const Name& comType, ActorCOMMoveableType type)
		: ActorComponentInfo(comType, type)
	{
	}

	void SphericalOceanComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<SphericalOceanComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void SphericalOceanComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<SphericalOceanComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void SphericalOceanComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		SphericalOceanComponentInfo* targetInfo = static_cast<SphericalOceanComponentInfo*>(comInfo);
		reflect::TypeResolver<SphericalOceanComponentInfo>::get()->copyTo(this, targetInfo);
	}

	void SphericalOceanComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
			reflect::TypeResolver<SphericalOceanComponentInfo>::get()->overWrite(mOriginComInfo, this);
	}
}
