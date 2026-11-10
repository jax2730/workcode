#include "EchoComponentStableHeaders.h"
#include "EchoWindPowerComponent.h"
#include "EchoTODComponentManager.h"
#include "EchoTODComponentSystem.h"
#include "EchoSceneManager.h"
#include "EchoTODDataUtil.h"

namespace Echo {
		EDITOR_REFLECT_BEGIN(WindPowerComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mWindPowerHandle.windAngle, "windAngle", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mWindPowerHandle.windPower, "windPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mWindPowerHandle.treeSwayAmp, "treeSwayAmp", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mWindPowerHandle.treeSwayFreq, "treeSwayFreq", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mWindPowerHandle.treeShakeAmp, "treeShakeAmp", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mWindPowerHandle.treeShakeFreq, "treeShakeFreq", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mWindPowerHandle.vegSwayAmp, "vegSwayAmp", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mWindPowerHandle.vegSwayFreq, "vegSwayFreq", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mWindPowerHandle.vegShakeAmp, "vegShakeAmp", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mWindPowerHandle.vegShakeFreq, "vegShakeFreq", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mWindPowerHandle.mPriority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END()

	DECLAREACTORCOMTYPE_CPP(WindPowerComponent)

	WindPowerComponentInfo::WindPowerComponentInfo(const Name& comType, ActorCOMMoveableType type):ActorComponentInfo(comType, type)
	{

	}

	void WindPowerComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<WindPowerComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void WindPowerComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<WindPowerComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void WindPowerComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		reflect::TypeResolver<WindPowerComponentInfo>::get()->copyTo(this, comInfo);
	}

	void WindPowerComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<WindPowerComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

	void WindPowerComponent::init(ActorComponentInfo* info)
	{
		TODCOMPONENT_INITMGR(getWindPowerContext, m_tMgr, false);

		WindPowerComponentInfo* mWindPowerInfo = static_cast<WindPowerComponentInfo*>(info);
		if (mWindPowerInfo)
		{
	        mContext = mWindPowerInfo->mWindPowerHandle;
		}
		use();

	}

	void WindPowerComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
		    m_tMgr->pushComponent(m_tMgr->getWindPowerContext(), this);
		else
			m_tMgr->popComponent(m_tMgr->getWindPowerContext(), this);
	}

	void WindPowerComponent::unInit()
	{
		TODCOMPONENT_UNINITMGR(getWindPowerContext, m_tMgr, T_End);
	}

	Name windPowerWindAngle_ = Name("windAngle");
	Name windPowerWindPower_ = Name("windPower");

	Name windPowerTreeSwayAmp_ = Name("treeSwayAmp");
	Name windPowerTreeSwayFreq_ = Name("treeSwayFreq");
	Name windPowerTreeShakeAmp_ = Name("treeShakeAmp");
	Name windPowerTreeShakeFreq_ = Name("treeShakeFreq");

	Name windPowerVegSwayAmp_ = Name("vegSwayAmp");
	Name windPowerVegSwayFreq_ = Name("vegSwayFreq");
	Name windPowerVegShakeAmp_ = Name("vegShakeAmp");
	Name windPowerVegShakeFreq_ = Name("vegShakeFreq");

	WindPowerComponent::WindPowerComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType) :ActorComponent(pActor, movableType)
	{
#ifdef ECHO_EDITOR
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_WIND_POWER,	windPowerWindPower_.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_WIND_ANGLE,	windPowerWindAngle_.toString(),		1);

		TODDataUtil::SetTodTemplate(TodDataID::PARAM_WIND_TREE_SWAY_AMP,	windPowerTreeSwayAmp_.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_WIND_TREE_SWAY_FREQ,	windPowerTreeSwayFreq_.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_WIND_TREE_SHAKE_AMP,	windPowerTreeShakeAmp_.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_WIND_TREE_SHAKE_FREQ,	windPowerTreeShakeFreq_.toString(),	1);

		TODDataUtil::SetTodTemplate(TodDataID::PARAM_WIND_VEG_SWAY_AMP,		windPowerVegSwayAmp_.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_WIND_VEG_SWAY_FREQ,	windPowerVegSwayFreq_.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_WIND_VEG_SHAKE_AMP,	windPowerVegShakeAmp_.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_WIND_VEG_SHAKE_FREQ,	windPowerVegShakeFreq_.toString(),	1);
#endif // ECHO_EDITOR
	}

	WindPowerComponent::~WindPowerComponent()
	{
		
	}

	void WindPowerComponent::use()
	{
		if (!m_tMgr)
			return;
		Echo::SceneManager* mainScene = m_tMgr->GetSceneManager();
		if (mainScene == nullptr)
			return;
		Vector4 windPara,treePara,vegPara;
		float fRadian = (mContext.windAngle / 180.f) * Math::PI;
		mContext.windDirection.x = Math::Cos(fRadian);
		mContext.windDirection.y = 0;
		mContext.windDirection.z = Math::Sin(fRadian);
		windPara.x =mContext.windDirection.x;
		windPara.y = mContext.windDirection.y;
		windPara.z = mContext.windDirection.z;
		windPara.w = mContext.windPower;

		treePara.x = mContext.treeSwayAmp;
		treePara.y = mContext.treeSwayFreq;
		treePara.z = mContext.treeShakeAmp;
		treePara.w = mContext.treeShakeFreq;

		vegPara.x = mContext.vegSwayAmp;
		vegPara.y = mContext.vegSwayFreq;
		vegPara.z = mContext.vegShakeAmp;
		vegPara.w = mContext.vegShakeFreq;
		
		mainScene->setWindParams(windPara);
		mainScene->setGlobalTreeAnimationParams(treePara);
		mainScene->setGlobalVegetationAnimationParams(vegPara);
	}

	void WindPowerComponent::unuse()
	{
		
	}

	bool WindPowerComponent::isCurrentActiveCom()
	{
		if (!m_tMgr)
			return false;
		auto& Context = m_tMgr->getWindPowerContext();
		if (Context.mActiveCom != nullptr && Context.mActiveCom == this)
			return true;
		return false;
	}

	void WindPowerComponent::useWindParas()
	{
		if (!m_tMgr)
			return;
		SceneManager* sMgr = m_tMgr->GetSceneManager();
		if(sMgr)
		   sMgr->setWindParams(Vector4(mContext.windDirection.x, mContext.windDirection.y, mContext.windDirection.z, mContext.windPower));
	}

	void WindPowerComponent::useTreeParas()
	{
		if (!m_tMgr)
			return;
		SceneManager* sMgr = m_tMgr->GetSceneManager();
		if (sMgr)
			sMgr->setGlobalTreeAnimationParams(Vector4(mContext.treeSwayAmp, mContext.treeSwayFreq, mContext.treeShakeAmp, mContext.treeShakeFreq));
	}

	void WindPowerComponent::useVegParas()
	{
		if (!m_tMgr)
			return;
		SceneManager* sMgr = m_tMgr->GetSceneManager();
		if (sMgr)
		    sMgr->setGlobalVegetationAnimationParams(Vector4(mContext.vegSwayAmp, mContext.vegSwayFreq, mContext.vegShakeAmp, mContext.vegShakeFreq));
	}

	void WindPowerComponent::setPriority(const int& val)
	{
		int prior = val;
		if (prior > 255) prior = 255;
		else if (prior < 0) prior = 0;
		mContext.mPriority = prior;
		if(m_tMgr)
			m_tMgr->adjustComponentPriority(m_tMgr->getWindPowerContext(), this);
	}

	int	WindPowerComponent::getPriority() const
	{
		return mContext.mPriority;
	}

	void WindPowerComponent::setGlobalWinderPower(const float& wp) 
	{
		mContext.windPower = wp;
		if (!isCurrentActiveCom())
			return;
		useWindParas();
	}

	void WindPowerComponent::setGlobalWinderAngle(const float& Angle)
	{
		float fRadian = (Angle / 180.f) * Math::PI;
		mContext.windDirection.x = Math::Cos(fRadian);
		mContext.windDirection.y = 0;
		mContext.windDirection.z = Math::Sin(fRadian);
		mContext.windAngle = Angle;
		if (!isCurrentActiveCom())
			return;
		useWindParas();
	}

	void WindPowerComponent::setGlobalTreeSwayAmp(const float& value)
	{
		mContext.treeSwayAmp = value;
		if (!isCurrentActiveCom())
			return;
		useTreeParas();
	}

	void WindPowerComponent::setGlobalTreeSwayFreq(const float& value)
	{
		mContext.treeSwayFreq = value;
		if (!isCurrentActiveCom())
			return;
		useTreeParas();
	}

	void WindPowerComponent::setGlobalTreeShakeAmp(const float& value)
	{
		mContext.treeShakeAmp = value;
		if (!isCurrentActiveCom())
			return;
		useTreeParas();
	}

	void WindPowerComponent::setGlobalTreeShakeFreq(const float& value)
	{
		mContext.treeShakeFreq = value;
		if (!isCurrentActiveCom())
			return;
		useTreeParas();
	}

	void WindPowerComponent::setGlobalVegSwayAmp(const float& value)
	{
		mContext.vegSwayAmp = value;
		if (!isCurrentActiveCom())
			return;
		useVegParas();
	}

	void WindPowerComponent::setGlobalVegSwayFreq(const float& value)
	{
		mContext.vegSwayFreq = value;
		if (!isCurrentActiveCom())
			return;
		useVegParas();
	}

	void WindPowerComponent::setGlobalVegShakeAmp(const float& value)
	{
		mContext.vegShakeAmp = value;
		if (!isCurrentActiveCom())
			return;
		useVegParas();
	}

	void WindPowerComponent::setGlobalVegShakeFreq(const float& value)
	{
		mContext.vegShakeFreq = value;
		if (!isCurrentActiveCom())
			return;
		useVegParas();
	}

	void WindPowerComponent::setTimelineAttribute(std::vector<float>& data,const Name& name_)
	{
		if (m_tMgr)
			m_tMgr->setTodComChangeState(T_WindPower);
		if (name_ == windPowerWindAngle_)
		{
			setGlobalWinderAngle(data[0]);
		}
		else if (name_ == windPowerWindPower_)
		{
			setGlobalWinderPower(data[0]);
		}
		else if (name_ == windPowerTreeSwayAmp_)
		{
			setGlobalTreeSwayAmp(data[0]);
		}
		else if (name_ == windPowerTreeSwayFreq_)
		{
			setGlobalTreeSwayFreq(data[0]);
		}
		else if (name_ == windPowerTreeShakeAmp_)
		{
			setGlobalTreeShakeAmp(data[0]);
		}
		else if (name_ == windPowerTreeShakeFreq_)
		{
			setGlobalTreeShakeFreq(data[0]);
		}
		else if (name_ == windPowerVegSwayAmp_)
		{
			setGlobalVegSwayAmp(data[0]);
		}
		else if (name_ == windPowerVegSwayFreq_)
		{
			setGlobalVegSwayFreq(data[0]);
		}
		else if (name_ == windPowerVegShakeAmp_)
		{
			setGlobalVegShakeAmp(data[0]);
		}
		else if (name_ == windPowerVegShakeFreq_)
		{
			setGlobalVegShakeFreq(data[0]);
		}
	}
}
