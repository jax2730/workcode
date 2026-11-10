#include "EchoComponentStableHeaders.h"
#include "EchoHDRComponent.h"
#include "EchoTODComponentManager.h"
#include "EchoLog.h"
#include "EchoLogManager.h"
#include "EchoRenderStrategy.h"
#include "EchoTODComponentSystem.h"
#include "EchoTODDataUtil.h"
namespace Echo
{
	EDITOR_REFLECT_BEGIN(HDRComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(mHDRHandle.EyeAdaption, "EyeAdaption", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mHDRHandle.Bright, "Bright", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mHDRHandle.EyeToneMapping, "EyeToneMapping", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mHDRHandle.EyeToneMappingCurve, "EyeToneMappingCurve", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mHDRHandle.EyeToneMappingColor, "EyeToneMappingColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mHDRHandle.mPriority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()
	DECLAREACTORCOMTYPE_CPP(HDRComponent)

	void HDRHandle::Default()
	{
		// Default JSON Data
		EyeAdaption = Vector4(0.15f, 0.7f, 0.5f, 0.05f);
		Bright.y = 0.5f;
		Bright.z = 1.0f;
		Bright.w = 3.3f;
		EyeToneMapping.y = 0.5f;
		EyeToneMapping.z = 1.0f;
		EyeToneMappingCurve[0] = 3.3f;
		EyeToneMappingCurve[1] = 0.08f;
		EyeToneMappingCurve[2] = 2.43f;
		EyeToneMappingCurve[3] = 0.59f;
		EyeToneMappingColor = ColorValue::White;
	}

	HDRComponentInfo::HDRComponentInfo(const Name& comType, ActorCOMMoveableType type) :ActorComponentInfo(comType, type)
	{

	}
	Name HDREyeAdaption_ = Name("EyeAdaption");
	Name HDRBright_ = Name("Bright");
	Name HDREyeToneMapping_ = Name("EyeToneMapping");
	Name HDREyeToneMappingCurve_ = Name("EyeToneMappingCurve");
	Name HDREyeToneMappingColor_ = Name("EyeToneMappingColor");

	Name HDRBright_X = Name("Bright_0x");
	Name HDREyeToneMapping_X = Name("EyeToneMapping_0x");
	Name HDREyeToneMappingCurve_E = Name("EyeToneMappingCurve_e");

	HDRComponent::HDRComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType):ActorComponent(pActor, movableType)
	{
#ifdef ECHO_EDITOR
		//TODDataUtil::SetTodTemplate(TodDataID::PARAM_POST_PROCESS_HDR_EYEADAPTATION_0X, HDREyeAdaption_X.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_POST_PROCESS_HDR_BRIGHT_0X,		HDRBright_X.toString(),					1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_POST_PROCESS_HDR_TONEMAPPING_0X,	HDREyeToneMapping_X.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_POST_PROCESS_HDR_TONEMAPPING_0W,	HDREyeToneMappingCurve_E.toString(),	1);
#endif // ECHO_EDITOR
	}

	HDRComponent::~HDRComponent()
	{

	}

	void HDRComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<HDRComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void HDRComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<HDRComponentInfo>::get()->ImportJson(this, jsonNode, "");

		mHDRHandle.Default();
	}

	void HDRComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		reflect::TypeResolver<HDRComponentInfo>::get()->copyTo(this, comInfo);
	}

	void HDRComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<HDRComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

	void HDRComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->pushComponent(m_tMgr->getHDRContext(), this);
		else
			m_tMgr->popComponent(m_tMgr->getHDRContext(), this);
	}

	void HDRComponent::init(ActorComponentInfo* info)
	{

		//getDefaultHDR();

		TODCOMPONENT_INITMGR(getHDRContext, m_tMgr, false);

		HDRComponentInfo* mHDRComponentInfo = static_cast<HDRComponentInfo*>(info);
		if (mHDRComponentInfo == nullptr)
			return;
		mContext = mHDRComponentInfo->mHDRHandle;

	}

	bool HDRComponent::isCurrentActiveCom()
	{
		if (!m_tMgr)
			return false;

		if (Root::instance()->getActiveSceneManager() != m_tMgr->GetSceneManager())
			return false;

		auto& Context = m_tMgr->getHDRContext();
		if (Context.mActiveCom != nullptr && Context.mActiveCom == this)
			return true;
		return false;
	}

	int HDRComponent::getPriority() const 
	{ 
		return mContext.mPriority;
	}

	void HDRComponent::setPriority(const int& val)
	{
		int prior = val;
		if (prior > 255) prior = 255;
		else if (prior < 0) prior = 0;
		mContext.mPriority = prior;
		if(m_tMgr)
			m_tMgr->adjustComponentPriority(m_tMgr->getHDRContext(), this);
	}

	/*void HDRComponent::getDefaultHDR()
	{
		if (pProcessChain == nullptr)
			return;

		if (!pProcessChain->isEnable(PostProcess::Type::HDR))
			return;

		getHDR();
		if (mHDR == nullptr)
			return;

		mDefaultHDR = std::make_shared<HDRHandle>();
		mDefaultHDR->EyeAdaption = mHDR->getEyeAdaptationParams();
		mDefaultHDR->Bright = mHDR->getBrightParams();
		mDefaultHDR->EyeToneMapping = mHDR->getToneMappingParams();
		mDefaultHDR->EyeToneMappingCurve = mHDR->getToneMappingCurve();
		mDefaultHDR->EyeToneMappingColor = mHDR->getColorBalance();
		mDefaultHDR->Default();
	}*/

	void HDRComponent::unInit()
	{
		TODCOMPONENT_UNINITMGR(getHDRContext, m_tMgr, T_End);
	}

	HDR* HDRComponent::getHDR()
	{
		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();

		if (!pRenderSystem)
			return nullptr;

		RenderStrategy* pStrategy = pRenderSystem->getCurRenderStrategy();
		if (!pStrategy)
			return nullptr;
		
		PostProcessChain* pProcessChain = pStrategy->getPostProcessChain();
		if (!pProcessChain)
			return nullptr;

		HDR* hdr = static_cast<HDR*>(pProcessChain->getPostProcess(PostProcess::Type::HDR));
		return hdr;
	}

	void HDRComponent::setEyeAdaption(const Vector4& EyeAdaption)
	{
		mContext.EyeAdaption = EyeAdaption;
		mContext.Default();
		if (!isCurrentActiveCom())
			return;
		HDR* pHDR = this->getHDR();
		if (nullptr != pHDR)
		{
			pHDR->setEyeAdaptationParam0x(mContext.EyeAdaption.x);
			pHDR->setEyeAdaptationParam0y(mContext.EyeAdaption.y);
			pHDR->setEyeAdaptationParam0z(mContext.EyeAdaption.z);
			pHDR->setEyeAdaptationParam0w(mContext.EyeAdaption.w);
		}	
	}

	void HDRComponent::setBright(const Vector4& Bright)
	{
		mContext.Bright = Bright;
		mContext.Default();
		if (!isCurrentActiveCom())
			return;

		HDR* pHDR = this->getHDR();
		if (nullptr != pHDR)
		{
			pHDR->setBrightParam0x(mContext.Bright.x);
			pHDR->setBrightParam0y(mContext.Bright.y);
			pHDR->setBrightParam0z(mContext.Bright.z);
			pHDR->setBrightParam0w(mContext.Bright.w);
		}
	}

	void HDRComponent::setEyeToneMapping(const Vector4& EyeToneMapping)
	{
		mContext.EyeToneMapping = EyeToneMapping;
		mContext.Default();
		if (!isCurrentActiveCom())
			return;

		HDR* pHDR = this->getHDR();
		if (nullptr != pHDR)
		{
			pHDR->setToneMappingParam0x(mContext.EyeToneMapping.x);
			pHDR->setToneMappingParam0y(mContext.EyeToneMapping.y);
			pHDR->setToneMappingParam0z(mContext.EyeToneMapping.z);
		}
	}

	void HDRComponent::setEyeToneMappingCurve(const std::vector<float>& EyeToneMappingCurve)
	{
		mContext.EyeToneMappingCurve = EyeToneMappingCurve;
		mContext.Default();
		if (!isCurrentActiveCom())
			return;

		HDR* pHDR = this->getHDR();
		if (nullptr != pHDR)
		{
			pHDR->setToneMappingCurve(
				mContext.EyeToneMappingCurve[0],
				mContext.EyeToneMappingCurve[1],
				mContext.EyeToneMappingCurve[2],
				mContext.EyeToneMappingCurve[3],
				mContext.EyeToneMappingCurve[4]);
		}
	}

	void HDRComponent::setEyeToneMappingColor(const ColorValue& EyeToneMappingColor)
	{
		mContext.EyeToneMappingColor = EyeToneMappingColor;
		mContext.Default();
		if (!isCurrentActiveCom())
			return;

		HDR* pHDR = this->getHDR();
		if (nullptr != pHDR)
		{
			pHDR->setToneMappingColorBalance(
				mContext.EyeToneMappingColor.r,
				mContext.EyeToneMappingColor.g,
				mContext.EyeToneMappingColor.b);
		}
	}

	void HDRComponent::setIsOpenHDR(bool IsOpen)
	{
		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();

		if (!pRenderSystem)
			return;

		RenderStrategy* pStrategy = pRenderSystem->getCurRenderStrategy();
		if (!pStrategy)
			return;

		PostProcessChain* pProcessChain = pStrategy->getPostProcessChain();
		if (!pProcessChain)
			return;
		
		pProcessChain->setEnable(PostProcess::Type::HDR, IsOpen);

	}

	void HDRComponent::setIsUseCurrent()
	{
		if (!isCurrentActiveCom())
			return;

		HDR* pHDR = this->getHDR();
		
		if (nullptr != pHDR)
		{
			pHDR->setEyeAdaptationParam0x(mContext.EyeAdaption.x);
			pHDR->setEyeAdaptationParam0y(mContext.EyeAdaption.y);
			pHDR->setEyeAdaptationParam0z(mContext.EyeAdaption.z);
			pHDR->setEyeAdaptationParam0w(mContext.EyeAdaption.w);
			pHDR->setBrightParam0x(mContext.Bright.x);
			pHDR->setBrightParam0y(mContext.Bright.y);
			pHDR->setBrightParam0z(mContext.Bright.z);
			pHDR->setBrightParam0w(mContext.Bright.w);

			pHDR->setToneMappingParam0x(mContext.EyeToneMapping.x);
			pHDR->setToneMappingParam0y(mContext.EyeToneMapping.y);
			pHDR->setToneMappingParam0z(mContext.EyeToneMapping.z);

			if (mContext.EyeToneMappingCurve.size() == 5)//防止下标越界
				pHDR->setToneMappingCurve(
					mContext.EyeToneMappingCurve[0],
					mContext.EyeToneMappingCurve[1],
					mContext.EyeToneMappingCurve[2],
					mContext.EyeToneMappingCurve[3],
					mContext.EyeToneMappingCurve[4]);

			pHDR->setToneMappingColorBalance(
				mContext.EyeToneMappingColor.r,
				mContext.EyeToneMappingColor.g,
				mContext.EyeToneMappingColor.b);
		}
	}

	void HDRComponent::use()
	{
		if (!m_tMgr)
			return;
		const TODContext<HDRComponent, HDRHandle>& context = m_tMgr->getHDRContext();
		setIsOpenHDR(context.mIsUseCurrType);
		HDR* pHDR = this->getHDR();

		bool isActive = Root::instance()->getActiveSceneManager() == m_tMgr->GetSceneManager();

		if (nullptr != pHDR && isActive)
		{
			pHDR->setEyeAdaptationParam0x(mContext.EyeAdaption.x);
			pHDR->setEyeAdaptationParam0y(mContext.EyeAdaption.y);
			pHDR->setEyeAdaptationParam0z(mContext.EyeAdaption.z);
			pHDR->setEyeAdaptationParam0w(mContext.EyeAdaption.w);

			pHDR->setBrightParam0x(mContext.Bright.x);
			pHDR->setBrightParam0y(mContext.Bright.y);
			pHDR->setBrightParam0z(mContext.Bright.z);
			pHDR->setBrightParam0w(mContext.Bright.w);

			pHDR->setToneMappingParam0x(mContext.EyeToneMapping.x);
			pHDR->setToneMappingParam0y(mContext.EyeToneMapping.y);
			pHDR->setToneMappingParam0z(mContext.EyeToneMapping.z);

			if(mContext.EyeToneMappingCurve.size() == 5)//防止下标越界
				pHDR->setToneMappingCurve(
					mContext.EyeToneMappingCurve[0],
					mContext.EyeToneMappingCurve[1],
					mContext.EyeToneMappingCurve[2],
					mContext.EyeToneMappingCurve[3],
					mContext.EyeToneMappingCurve[4]);

			pHDR->setToneMappingColorBalance(
				mContext.EyeToneMappingColor.r,
				mContext.EyeToneMappingColor.g,
				mContext.EyeToneMappingColor.b);
		}
	}

	void HDRComponent::unuse()
	{
	}

	void HDRComponent::setTimelineAttribute(std::vector<float>& data,const Name& name_)
	{
		if (m_tMgr)
			m_tMgr->setTodComChangeState(T_HDR);

		// Hdr自适应组件启用后会屏蔽Hdr组件对于此参数的设置，仅使用自适应组件的参数
		//if(name_ == HDREyeAdaption_ && !(m_tMgr && m_tMgr->isUseMaskHdrData()))
		//{
		//	Vector4 EyeAdaption{ data[0],data[1],data[2],data[3] };
		//	setEyeAdaption(EyeAdaption);
		//}
		if(name_ == HDRBright_)
		{
			Vector4 Bright{ data[0],data[1],data[2],data[3] };
			setBright(Bright);
		}
		else if(name_ == HDREyeToneMapping_)
		{
			Vector4 EyeToneMapping{ data[0],data[1],data[2],data[3] };
			setEyeToneMapping(EyeToneMapping);
		}
		else if (name_ == HDREyeToneMappingCurve_)
		{
			std::vector<float> EyeToneMappingCurve{ data[0],data[1],data[2],data[3],data[4] };
			setEyeToneMappingCurve(EyeToneMappingCurve);
		}
		/*else if (name_ == HDREyeToneMappingColor_)
		{
			ColorValue EyeToneMappingColor{ data[0],data[1],data[2],data[3] };
			setEyeToneMappingColor(EyeToneMappingColor);
		}*/
		else if (name_ == HDRBright_X)
		{
			Vector4 Bright{ data[0],0.5f, 1.0f, 3.3f };
			setBright(Bright);
		}
		else if (name_ == HDREyeToneMapping_X)
		{
			Vector4 EyeToneMapping{ data[0],0.5f, 1.0f, 0.0f };
			setEyeToneMapping(EyeToneMapping);
		}
		else if (name_ == HDREyeToneMappingCurve_E)
		{
			std::vector<float> EyeToneMappingCurve{ 3.3f, 0.08f, 2.43f, 0.59f, data[0] };
			setEyeToneMappingCurve(EyeToneMappingCurve);
		}
	}
}
