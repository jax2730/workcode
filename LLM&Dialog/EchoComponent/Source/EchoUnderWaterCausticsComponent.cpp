#include "EchoComponentStableHeaders.h"
#include "EchoUnderWaterCausticsComponent.h"
#include "EchoTODComponentManager.h"
#include "EchoTODComponentSystem.h"
#include "EchoOcean.h"
#include "EchoVideoConfigSystem.h"
#include "EchoWorldManager.h"
#include "EchoTODDataUtil.h"
#include "EchoStringTable.h"
namespace Echo {
		EDITOR_REFLECT_BEGIN(UnderWaterCausticsComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mCausticsHandle.isForceUnderWater, "isForceUnderWater", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mCausticsHandle.isEnable, "UnderWaterEnable", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mCausticsHandle.StartDistance, "StartDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mCausticsHandle.EndDistance, "EndDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mCausticsHandle.Density, "Density", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mCausticsHandle.DisturbStrength, "DisturbStrength", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mCausticsHandle.Color, "Color", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mCausticsHandle.strength, "strength", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mCausticsHandle.mPriority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END()
	DECLAREACTORCOMTYPE_CPP(UnderWaterCausticsComponent)

	UnderWaterCausticsComponentInfo::UnderWaterCausticsComponentInfo(const Name& comType, ActorCOMMoveableType type) :ActorComponentInfo(comType, type)
	{

	}

	void UnderWaterCausticsComponentInfo::ExportData(cJSON* jsonNode)
	{
			reflect::TypeResolver<UnderWaterCausticsComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void UnderWaterCausticsComponentInfo::ImportData(cJSON* jsonNode)
	{
			reflect::TypeResolver<UnderWaterCausticsComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void UnderWaterCausticsComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
			if (comInfo == nullptr)
				return;
			reflect::TypeResolver<UnderWaterCausticsComponentInfo>::get()->copyTo(this, comInfo);
	}

	void UnderWaterCausticsComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<UnderWaterCausticsComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

	Name underWaterCausticsStartDistance_ = Name("StartDistance");
	Name underWaterCausticsEndDistance_ = Name("EndDistance");
	Name underWaterCausticsDensity_ = Name("Density");
	Name underWaterCausticsDisturbStrength_ = Name("DisturbStrength");
	Name underWaterCausticsColor_ = Name("Color");
	Name underWaterCausticsStrength_ = Name("ColorStrength");
	Name underWaterCausticsIsForceUnderWater_ = Name("isForceUnderWater");
	Name underWaterCausticsUnderWaterEnable_ = Name("UnderWaterEnable");

	UnderWaterCausticsComponent::UnderWaterCausticsComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType) :ActorComponent(pActor, movableType)
	{
#ifdef ECHO_EDITOR
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_UNDERWATER_CAUSTIC_COLOR,			underWaterCausticsColor_.toString(),	4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_UNDERWATER_CAUSTIC_COLOR_POWER,	underWaterCausticsStrength_.toString(),	1);
#endif // ECHO_EDITOR
		mCausticTextureName = StringTable::instance()->getString("Ocean", "DefaultUnderWaterCausticTexture");
		mTunnelTextureName = StringTable::instance()->getString("Ocean", "DefaultUnderWaterTunnelTexture");
	}

	UnderWaterCausticsComponent::~UnderWaterCausticsComponent()
	{

	}

	void UnderWaterCausticsComponent::init(ActorComponentInfo* info)
	{
		TODCOMPONENT_INITSCENEMGR(getUnderWaterCausticsContext, m_tMgr, false, m_sMgr);

		UnderWaterCausticsComponentInfo* mCausticsInfo = static_cast<UnderWaterCausticsComponentInfo*>(info);
		if (mCausticsInfo == nullptr)
			return;
		mContext = mCausticsInfo->mCausticsHandle;
		//setCausticTexture();
	}

	void UnderWaterCausticsComponent::unInit()
	{
		if(m_sMgr)
		    m_sMgr->setIsForceUnderWater(false);
		TODCOMPONENT_UNINITMGR(getUnderWaterCausticsContext, m_tMgr, T_End);
		m_sMgr = nullptr;
	}

	void UnderWaterCausticsComponent::use()
	{
		if(m_sMgr)
		{
			m_sMgr->setCausticParams(Vector4(mContext.StartDistance, mContext.EndDistance, mContext.Density, mContext.DisturbStrength));
			m_sMgr->setCausticColour(mContext.Color);
		}
		
		if (getWorldManager()) {
			getWorldManager()->setUnderWaterEnable(mContext.isEnable);
		}
		else {
			VideoConfigSystem::instance()->getActiveVideoConfig()->setUnderWaterEnable(mContext.isEnable);
		}

		setIsUseUnderWater(true);
		setIsForceUnderWater(IsForceUnderWater());
	}

	void UnderWaterCausticsComponent::unuse()
	{
		if(m_sMgr)
		   m_sMgr->setIsForceUnderWater(false);

		if (getWorldManager()) {
			getWorldManager()->setUnderWaterEnable(false);
		}
		else {
			VideoConfigSystem::instance()->getActiveVideoConfig()->setUnderWaterEnable(false);
		}
	}

	int UnderWaterCausticsComponent::getPriority() const
	{
		return mContext.mPriority;
	}

	void UnderWaterCausticsComponent::setPriority(const int& val)
	{
		int prior = val;
		if (prior > 255) prior = 255;
		else if (prior < 0) prior = 0;
		mContext.mPriority = prior;
		if(m_tMgr)
		   m_tMgr->adjustComponentPriority(m_tMgr->getUnderWaterCausticsContext(), this);
	}

	bool UnderWaterCausticsComponent::IsForceUnderWater()
	{ 
		return mContext.isForceUnderWater;
	}

	bool UnderWaterCausticsComponent::getUnderWaterEnable() const 
	{ 
		return mContext.isEnable;
	}

	void UnderWaterCausticsComponent::setGlobalStartDistance(const float& val)
	{
		mContext.StartDistance = val;
		if (!isCurrentActiveCom())
			return;

		if (m_sMgr) {
			Vector4 temp(mContext.StartDistance, mContext.EndDistance, mContext.Density, mContext.DisturbStrength);
			m_sMgr->setCausticParams(temp);
		}
	}

	void UnderWaterCausticsComponent::setGlobalEndDistance(const float& val)
	{
		mContext.EndDistance = val;

		if (!isCurrentActiveCom())
			return;
		if (m_sMgr)
		{
            Vector4 temp(mContext.StartDistance, mContext.EndDistance, mContext.Density, mContext.DisturbStrength);
		    m_sMgr->setCausticParams(temp);
		}
	}

	void UnderWaterCausticsComponent::setGlobalDensity(const float& val)
	{
		mContext.Density = val;

		if (!isCurrentActiveCom())
			return;

		if (m_sMgr)
		{
			Vector4 temp(mContext.StartDistance, mContext.EndDistance, mContext.Density, mContext.DisturbStrength);
			m_sMgr->setCausticParams(temp);
		}
	}

	void UnderWaterCausticsComponent::setGlobalDisturbStrength(const float& val)
	{
		mContext.DisturbStrength = val;
		if (!isCurrentActiveCom())
			return;
		if (m_sMgr)
		{
			Vector4 temp(mContext.StartDistance, mContext.EndDistance, mContext.Density, mContext.DisturbStrength);
			m_sMgr->setCausticParams(temp);
		}
	}

	void UnderWaterCausticsComponent::setCausticsClolor(const ColorValue& Color)
	{
		mContext.Color.r = Color.r;
		mContext.Color.g = Color.g;
		mContext.Color.b = Color.b;
		if (!isCurrentActiveCom())
			return;
		if(m_sMgr)
		   m_sMgr->setCausticColour(mContext.Color);
	}

	void UnderWaterCausticsComponent::setColorStrength(const float& val)
	{
		mContext.Color.a = val;
		mContext.strength = val;
		if (!isCurrentActiveCom())
			return;
		if(m_sMgr)
		   m_sMgr->setCausticColour(mContext.Color);
	}

	void UnderWaterCausticsComponent::useCaustics()
	{
		if (!isCurrentActiveCom())
			return;
		if (m_sMgr)
		{
           Vector4 causticsParas = Vector4(mContext.StartDistance, mContext.EndDistance, mContext.Density, mContext.DisturbStrength);
		   m_sMgr->setCausticParams(causticsParas);
		}
		
	}

	void UnderWaterCausticsComponent::useColor()
	{
		if (!isCurrentActiveCom())
			return;
		if(m_sMgr)
		   m_sMgr->setCausticColour(mContext.Color);
	}

	void UnderWaterCausticsComponent::setIsForceUnderWater(const bool& isForceUnderWater)
	{
		mContext.isForceUnderWater = isForceUnderWater;
		if (!isCurrentActiveCom())
			return;
		setIsUseUnderWater(true);

		if (!m_sMgr)
			return;

		m_sMgr->setIsForceUnderWater(isForceUnderWater);
		Ocean* ocean = m_sMgr->getOcean();
		if (ocean)
			ocean->setForceUnderWater(isForceUnderWater);

	}
	
	bool UnderWaterCausticsComponent::isCurrentActiveCom()
	{
		if (!m_tMgr)
			return false;
		const auto& Context = m_tMgr->getUnderWaterCausticsContext();
		if (Context.mActiveCom != nullptr && Context.mActiveCom == this)
			return true;
		return false;
	}

	void UnderWaterCausticsComponent::setUnderWaterEnable(const bool& UnderWaterEnable)
	{
		mContext.isEnable = UnderWaterEnable;
		if (!isCurrentActiveCom())
			return;
		if (getWorldManager()) {
			getWorldManager()->setUnderWaterEnable(UnderWaterEnable);
		}
		else {
			VideoConfigSystem::instance()->getActiveVideoConfig()->setUnderWaterEnable(UnderWaterEnable);
		}
		setIsUseUnderWater(UnderWaterEnable);
	}

	void UnderWaterCausticsComponent::setIsUseUnderWater(const bool& isUse)
	{
		if (!m_sMgr)
			return;
		Echo::Camera* camera = m_sMgr->getMainCamera();
		const Echo::Vector3& cameraPos = camera->getDerivedPosition();
		bool IsUnderOcean = false;
		float height = -99999.0;
		float waterLine = 0.0f;
		Echo::Ocean* pOcean = m_sMgr->getOcean();
		if (pOcean && pOcean->isVisible())
		{
			if (pOcean->getOceanWaveHeight() > cameraPos.y)
			{
				height = pOcean->getOceanWaveHeight();
				IsUnderOcean = true;
			}
		}
		if (IsForceUnderWater())
		{
			IsUnderOcean = true;
			height = 99999.9f;
		}
		if (isUse) {
			if (getWorldManager()) {
				getWorldManager()->applyUnderWaterProcess(height, IsUnderOcean);
			}
			else {
				if (IsUnderOcean)
				{
					const Vector3* cameraCorner = camera->getWorldSpaceCorners();
					waterLine = (cameraCorner[0].y - height) / (cameraCorner[0].y - cameraCorner[2].y);
				}
				VideoConfigSystem::instance()->getActiveVideoConfig()->applyUnderWaterProcess(waterLine, IsUnderOcean);
			}
		}
	}

	void UnderWaterCausticsComponent::setCausticTexture()
	{
		if (!m_sMgr)
			return;
		//set UnderWater Caustic Texture 
		TexturePtr m_pCausticTex = TextureManager::instance()->loadTexture(Name(mCausticTextureName));
		TexturePtr m_pTunnelTex = TextureManager::instance()->loadTexture(Name(mTunnelTextureName));
		m_sMgr->setGlobalCausticTexture(m_pCausticTex);

		RenderSystem* pRenderSys = Root::instance()->getRenderSystem();

		for (uint32 i = 0; i < RENDER_STRATEGY_CNT; i++)
		{
			RenderStrategy* pRenderStrategy = pRenderSys->getRenderStrategy(i);
			if (pRenderStrategy == nullptr)
				continue;

			PostProcessChain* pPostProcessChain = pRenderStrategy->getPostProcessChain();
			if (pPostProcessChain == nullptr)
				continue;

			UnderWater* pUnderWater = (UnderWater*)pPostProcessChain->getPostProcess(PostProcess::Type::UnderWater);

			if (pUnderWater)
			{
				pUnderWater->setCausticTexture(m_pCausticTex.getPointer());
			}

			//Tunnel* pTunnel = (Tunnel*)pPostProcessChain->getPostProcess(PostProcess::Type::Tunnel);

			//if (pTunnel)
			//{
			//	pTunnel->setTunnelTexture(m_pTunnelTex.getPointer());
			//}
		}
	}

	void UnderWaterCausticsComponent::setTimelineAttribute(std::vector<float>& data,const Name& name_)
	{
		if (!m_tMgr)
			return;
		const auto& Context = m_tMgr->getUnderWaterCausticsContext();
		if (Context.mActiveCom == nullptr || Context.mActiveCom != this)
			return;

		if (Context.mIsUseCurrType == false)
			return;
		m_tMgr->setTodComChangeState(T_UnderWaterCaustics);
		if (name_ == underWaterCausticsStartDistance_)
		{
			mContext.StartDistance = data[0];
			setGlobalStartDistance(data[0]);
		}
		else if (name_ == underWaterCausticsEndDistance_)
		{
			mContext.EndDistance = data[0];
			setGlobalEndDistance(data[0]);
		}
		else if (name_ == underWaterCausticsDensity_)
		{
			mContext.Density = data[0];
			setGlobalDensity(data[0]);
		}
		else if (name_ == underWaterCausticsDisturbStrength_)
		{
			mContext.DisturbStrength = data[0];
			setGlobalDisturbStrength(data[0]);
		}
		else if (name_ == underWaterCausticsColor_)
		{
			ColorValue color = ColorValue(data[0], data[1], data[2], data[3]);
			mContext.Color = color;
			setCausticsClolor(color);
		}
		else if (name_ == underWaterCausticsStrength_)
		{
			mContext.strength = data[0];
			setColorStrength(data[0]);
		}
		else if (name_ == underWaterCausticsIsForceUnderWater_)
		{
			mContext.isForceUnderWater = data[0];
			setIsForceUnderWater(data[0]);
		}
		else if (name_ == underWaterCausticsUnderWaterEnable_)
		{
			mContext.isEnable = data[0];
			setUnderWaterEnable(data[0]);
		}
	}

	void UnderWaterCausticsComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->pushComponent(m_tMgr->getUnderWaterCausticsContext(), this);
		else
			m_tMgr->popComponent(m_tMgr->getUnderWaterCausticsContext(), this);
	}
}
