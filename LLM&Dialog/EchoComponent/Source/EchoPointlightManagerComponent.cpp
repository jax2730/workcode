#include "EchoComponentStableHeaders.h"
#include "EchoPointlightManagerComponent.h"
#include "EchoTODComponentManager.h"
#include "EchoSceneManager.h"
#include "EchoTODComponentSystem.h"
#include "EchoTODDataUtil.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(PointlightManagerComponent);

	EDITOR_REFLECT_BEGIN(PointlightManagerComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mLightPower, "LightPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRolePointLightColor, "RoleLightColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRolePointLightPower, "RoleLightPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIsUseRolePointLight, "IsUseRolePointLight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSelfEmissiveLightPower, "SelfEmissiveLightPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	Name lightPowerName_  = Name("LightPower");
	Name roleLightColorName_ = Name("RoleLightColor");
	Name roleLightPowerName_ = Name("RoleLightPower");
	Name selfEmissiveLightPowerName_ = Name("SelfEmissiveLightPower");

	//info
	PointlightManagerComponentInfo::PointlightManagerComponentInfo(const Name& comType, ActorCOMMoveableType type)
		:ActorComponentInfo(comType,type)
	{
	}
	PointlightManagerComponentInfo::~PointlightManagerComponentInfo()
	{
	}
	void PointlightManagerComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<PointlightManagerComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}
	void PointlightManagerComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<PointlightManagerComponentInfo>::get()->ImportJson(this, jsonNode, ""); 
	}
	void PointlightManagerComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		reflect::TypeResolver<PointlightManagerComponentInfo>::get()->copyTo(this, comInfo);
	}
	void PointlightManagerComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr) 
		{
			reflect::TypeResolver<PointlightManagerComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}


	//component
	PointlightManagerComponent::PointlightManagerComponent(ActorPtr pActor, ActorCOMMoveableType movableType)
		: ActorComponent(pActor, movableType)
	{
#ifdef ECHO_EDITOR
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_GLOBAL_POINT_LIGHT_POWER,	lightPowerName_.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_ROLE_POINT_LIGHT_COLOR,	roleLightColorName_.toString(),		4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_ROLE_POINT_LIGHT_POWER,	roleLightPowerName_.toString(),		1);
#endif // ECHO_EDITOR
	}

	PointlightManagerComponent::~PointlightManagerComponent()
	{
	}

	void PointlightManagerComponent::init(ActorComponentInfo* info)
	{
		TODCOMPONENT_INITSCENEMGR(getPointlightMgrContext, mTodComMgr, false, mSceneMgr);

		if (info == nullptr)
			return;
		PointlightManagerComponentInfo* mPLightManagerInfo = static_cast<PointlightManagerComponentInfo*>(info);
		mContext = mPLightManagerInfo->mContext;
		mContext.mRolePointLightColorLinear = ColorValue::gammaToLinear(mContext.mRolePointLightColor);
	}
	void PointlightManagerComponent::unInit()
	{
		TODCOMPONENT_UNINITMGR(getPointlightMgrContext, mTodComMgr, T_End);
		mSceneMgr = nullptr;
	}
	void PointlightManagerComponent::copyTo(const ActorComponentPtr& newObj)
	{
	}
	void PointlightManagerComponent::setTimelineAttribute(std::vector<float>& data, const Name& name)
	{
		if (mTodComMgr == nullptr)
			return;
		const auto& pLightMgrContext_ = mTodComMgr->getPointlightMgrContext();
		mTodComMgr->setTodComChangeState(T_PointLight);
		if (pLightMgrContext_.mActiveCom != this)
			return;

		if (name == lightPowerName_)
			setGlobalPointLightPower(data[0]);
		else if (name == roleLightColorName_)
			setRolePointLightColor(ColorValue(data[0], data[1], data[2], data[3]));
		else if (name == roleLightPowerName_)
			setRolePointLightPower(data[0]);
		else if (name == selfEmissiveLightPowerName_)
			setSelfEmissiveLightPower(data[0]);
	}
	void PointlightManagerComponent::visibleStateChanged(bool isVisible)
	{
		if (!mTodComMgr)
			return;
		if (isVisible)
			mTodComMgr->pushComponent(mTodComMgr->getPointlightMgrContext(), this);
		else
			mTodComMgr->popComponent(mTodComMgr->getPointlightMgrContext(), this);
	}
	void PointlightManagerComponent::use()
	{
		if (mTodComMgr == nullptr || mTodComMgr->getPointlightMgrContext().mIsUseCurrType == false)
			return;
		if (mSceneMgr == nullptr)
			return;
		mSceneMgr->setLightPower(mContext.mLightPower);
		mSceneMgr->setRolePointLightColor(mContext.mRolePointLightColor);
		mSceneMgr->setRolePointLightPower(mContext.mRolePointLightPower);
		mSceneMgr->setRolePointLightVisible(mContext.mIsUseRolePointLight);
		mSceneMgr->setSelfEmissiveLightPower(mContext.mSelfEmissiveLightPower);
	}
	void PointlightManagerComponent::unuse()
	{

	}
	void PointlightManagerComponent::setGlobalPointLightPower(float value_)
	{
		mContext.mLightPower = value_;
		if (mTodComMgr == nullptr)
			return;

		const auto& pLightMgrContext_ = mTodComMgr->getPointlightMgrContext();

		if (mTodComMgr && mSceneMgr && pLightMgrContext_.mActiveCom == this)
			mSceneMgr->setLightPower(value_);
	}
	void PointlightManagerComponent::setSelfEmissiveLightPower(float power_)
	{
		mContext.mSelfEmissiveLightPower = power_;

		if (!mTodComMgr)
			return;
		const auto& pLightMgrContext_ = mTodComMgr->getPointlightMgrContext();

		if (mTodComMgr && mSceneMgr && pLightMgrContext_.mActiveCom == this)
			mSceneMgr->setSelfEmissiveLightPower(power_);

	}
	void PointlightManagerComponent::setIsUseRolePointLight(bool isUse)
	{
		mContext.mIsUseRolePointLight = isUse;
		if (mTodComMgr == nullptr)
			return;
		const auto& pLightMgrContext_ = mTodComMgr->getPointlightMgrContext();
		if (mSceneMgr && pLightMgrContext_.mActiveCom == this)
			mSceneMgr->setRolePointLightVisible(isUse);

	}
	void PointlightManagerComponent::setRolePointLightColor(const ColorValue& color_)
	{
		mContext.mRolePointLightColor = color_;
		mContext.mRolePointLightColorLinear = ColorValue::gammaToLinear(mContext.mRolePointLightColor);
		
		if (mTodComMgr == nullptr)
			return;
		const auto& pLightMgrContext_ = mTodComMgr->getPointlightMgrContext();
		if (mSceneMgr && pLightMgrContext_.mActiveCom == this)
			mSceneMgr->setRolePointLightColor(color_);
	}
	void PointlightManagerComponent::setRolePointLightPower(float power_)
	{
		mContext.mRolePointLightPower = power_;
		if (mTodComMgr == nullptr)
			return;
		const auto& pLightMgrContext_ = mTodComMgr->getPointlightMgrContext();
		if (mSceneMgr && pLightMgrContext_.mActiveCom == this)
			mSceneMgr->setRolePointLightPower(power_);
	}
	void PointlightManagerComponent::setPriority(int priority_)
	{
		mContext.mPriority = priority_;
		if(mTodComMgr)
			mTodComMgr->adjustComponentPriority(mTodComMgr->getPointlightMgrContext(), this);
	}
}

