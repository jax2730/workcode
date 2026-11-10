#include "EchoComponentStableHeaders.h"
#include "EchoSphereFogComponent.h"
#include "EchoTODComponentManager.h"
#include "EchoTODComponentSystem.h"
#include "EchoSphericalTerrainManager.h"
#include "EchoSphericalTerrainComponent.h"
#include "EchoPlanetConverterComponent.h"
#include "EchoSphericalTerrainFog.h"

namespace Echo{

	DECLAREACTORCOMTYPE_CPP(SphereFogComponent)


	EDITOR_REFLECT_BEGIN(SphereFogComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFogColor, "FogColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFogExpTerm, "FogExpTerm", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFogExpScale, "FogExpScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFogMaxAlpha, "FogMaxAlpha", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mHorizonColor, "HorizonColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mColorChangeGradient, "ColorChangeGradient", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFogStart, "FogStart", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFogEnd, "FogEnd", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFogSkyParam, "FogSkyParam", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mEnableAtmosphere, "EnableAtmosphere", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTransFormTime, "TransFormTime", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();


	SphereFogComponentInfo::SphereFogComponentInfo(const Name& type, ActorCOMMoveableType moveable)
		:ActorComponentInfo(type,moveable)
	{
	}
	SphereFogComponentInfo::~SphereFogComponentInfo()
	{
	}
	void SphereFogComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<SphereFogComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}
	void SphereFogComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<SphereFogComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}
	void SphereFogComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		SphereFogComponentInfo* info = static_cast<SphereFogComponentInfo*>(comInfo);
		reflect::TypeResolver<SphereFogComponentInfo>::get()->copyTo(this, info);
	}
	void SphereFogComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<SphereFogComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

	SphereFogComponent::SphereFogComponent(ActorPtr pActor, ActorCOMMoveableType movableType)
		: ActorComponent(pActor, movableType)
	{
	}

	SphereFogComponent::~SphereFogComponent()
	{
	}

	void SphereFogComponent::use()
	{
		if (!m_tMgr)
			return;
		mIsActiveCom = true;
		SetFog();
	}
	void SphereFogComponent::unuse()
	{
		mIsActiveCom = false;
	}
	void SphereFogComponent::setTimelineAttribute(std::vector<float>& data, const Name& name)
	{
		if (!m_tMgr || !mIsActiveCom)
			return;

		m_tMgr->setTodComChangeState(T_SphereFog);
		if (name == fogColorName_)
		{
			SetFogColor(ColorValue(data[0], data[1], data[2], data[3]));
		}
		else if (name == fogExpScaleName_)
		{
			SetFogExpScale(data[0]);
		}
		else if (name == fogExpTermName_)
		{
			SetFogExpTerm(data[0]);
		}
		else if (name == fogMaxAlphaName_)
		{
			SetMaxAlpha(data[0]);
		}
		else if (name == fogHorizonColorName_)
		{
			SetHorizonColor(ColorValue(data[0], data[1], data[2], data[3]));
		}
		else if (name == fogColorChangeGradientName_)
		{
			SetColorChangeGradient(data[0]);
		}
		else if (name == fogStartName_)
		{
			SetFogStart(data[0]);
		}
		else if (name == fogEndName_)
		{
			SetFogEnd(data[0]);
		}
		else if (name == fogSkyParamName_)
		{
			SetSkyParam(data[0]);
		}

	}
	void SphereFogComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->pushComponent(m_tMgr->getSphereFogContext(), this);
		else
			m_tMgr->popComponent(m_tMgr->getSphereFogContext(), this);
	}
	void SphereFogComponent::init(ActorComponentInfo* info)
	{
		m_pActComInfo = info;
		SphereFogComponentInfo* mInfo = static_cast<SphereFogComponentInfo*>(info);
		if (mInfo)
		{
			mContext = mInfo->mContext;
			mContext.mFogExpScale = Echo::Clamp(0.f, 1.f, mContext.mFogExpScale);
			mContext.mFogExpTerm = Echo::Clamp(0.f, 20.f, mContext.mFogExpTerm);
		}
		TODCOMPONENT_INITMGR(getSphereFogContext, m_tMgr, true);
	}
	void SphereFogComponent::unInit()
	{
		TODCOMPONENT_UNINITMGR(getSphereFogContext, m_tMgr, T_SphereFog);
	}
	void SphereFogComponent::SetFogColor(const ColorValue& val)
	{
		mContext.mFogColor = val;
		SetFog();
	}
	void SphereFogComponent::SetFogExpScale(float val)
	{
		mContext.mFogExpScale = val;
		SetFog();
	}
	void SphereFogComponent::SetFogExpTerm(float val)
	{
		mContext.mFogExpTerm = val;
		SetFog();
	}
	void SphereFogComponent::SetHorizonColor(const ColorValue& val)
	{
		mContext.mHorizonColor = val;
		SetFog();
	}
	void SphereFogComponent::SetFogStart(float val)
	{
		mContext.mFogStart = val;
		SetFog();
	}
	void SphereFogComponent::SetFogEnd(float val)
	{
		mContext.mFogEnd = val;
		SetFog();
	}
	void SphereFogComponent::SetColorChangeGradient(float value)
	{
		mContext.mColorChangeGradient = value;
		SetFog();
	}
	void SphereFogComponent::SetMaxAlpha(float val)
	{
		mContext.mFogMaxAlpha = val;
		SetFog();
	}
	void SphereFogComponent::SetSkyParam(float val)
	{
		mContext.mFogSkyParam.x = val;
		SetFog();
	}
	void SphereFogComponent::SetEnable(bool val)
	{
		mContext.mEnableAtmosphere = val;
		SetFog();
	}
	
	void SphereFogComponent::SetFog()
	{
		if (!mIsActiveCom)
			return;
		RenderSystem* mRenderSystem = Root::instance()->getRenderSystem();

		if (!mRenderSystem)
			return;
		RenderStrategy* pStrategy = mRenderSystem->getCurRenderStrategy();
		if (!pStrategy)
			return;
		PostProcessChain* pProcessChain = pStrategy->getPostProcessChain();
		if (!pProcessChain)
			return;

		

		SphericalTerrainFog*  mFog = static_cast<SphericalTerrainFog*> (pProcessChain->getPostProcess(PostProcess::Type::SphericalTerrainFog));
		if (!mFog)
			return;

		//伪代码 这下面应该变成底层的新接口
		mFog->setTodFogParams(mContext.mHorizonColor, mContext.mFogColor, mContext.mFogExpTerm, mContext.mFogExpScale,
			mContext.mFogStart,mContext.mFogEnd, mContext.mColorChangeGradient, mContext.mFogMaxAlpha, mContext.mFogSkyParam);
		//mFog->enableFog(mContext.mEnableAtmosphere);

	}
	void SphereFogComponent::setPriority(int priority)
	{
		mContext.mPriority = priority;
		if (m_tMgr)
			m_tMgr->adjustComponentPriority(m_tMgr->getSphereFogContext(), this);
	}
}
