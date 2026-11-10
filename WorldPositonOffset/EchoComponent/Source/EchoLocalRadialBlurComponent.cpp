#include "EchoComponentStableHeaders.h"
#include "EchoLocalRadialBlurComponent.h"
#include "EchoRenderStrategy.h"
#include "EchoLocalRadialBlur.h"
#include "EchoTODComponentSystem.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(LocalRadialBlurComponent);

	// ------------------------------------------ Component ----------------------------------------------------------------

	Name LocalRadialBlurComponent::LRB_SPHERECENTER		= Name("SphereCenter");			// 模糊区域中心坐标
	Name LocalRadialBlurComponent::LRB_SPHERERADIUS		= Name("SphereRadius");			// 模糊区域球体半径
	Name LocalRadialBlurComponent::LRB_BLURSTRENGTH		= Name("BlurStrength");			// 模糊总强度
	Name LocalRadialBlurComponent::LRB_JITTERINTENSITY	= Name("JitterIntensity");		// 噪点抖动强度
	Name LocalRadialBlurComponent::LRB_CENTERHOTSPOT	= Name("CenterHotspot");		// 中心聚集度
	Name LocalRadialBlurComponent::LRB_RAYLENGTH		= Name("RayLength");			// 光束长度
	Name LocalRadialBlurComponent::LRB_ABERRATION		= Name("Aberration");			// 色散偏移量
	Name LocalRadialBlurComponent::LRB_HOLLOWRANGE		= Name("HollowRange");			// 控制中心清晰区域的大小
	Name LocalRadialBlurComponent::LRB_OPEN				= Name("Opne");

	LocalRadialBlurComponent::LocalRadialBlurComponent(ActorPtr pActor, ActorCOMMoveableType movableType)
		: NodeActorComponent(pActor, movableType)
	{
	}

	LocalRadialBlurComponent::~LocalRadialBlurComponent()
	{
	}

	void LocalRadialBlurComponent::init(ActorComponentInfo* info)
	{
		if (info == nullptr) return;

		LocalRadialBlurComponentInfo* pInfo = static_cast<LocalRadialBlurComponentInfo*>(info);

		mContext = pInfo->mContext;
		mIsOpen = pInfo->mIsOpen;
	}

	void LocalRadialBlurComponent::unInit()
	{
	}

	void LocalRadialBlurComponent::connectCom()
	{
		_setLocalRadialBlurContextData();
	}

	void LocalRadialBlurComponent::setPosition(const DBVector3& pos, bool updateInstantly, bool redirectParent)
	{
		ComponentNode::setPosition(pos, updateInstantly, redirectParent);

		PostProcessChain* pProcessChain = nullptr;
		LocalRadialBlur* pLRBlur = _getLocalRadiaBlur(pProcessChain);
		if (pLRBlur && pProcessChain)
			pLRBlur->setBlurCenter(getWorldPosition());
	}

	void LocalRadialBlurComponent::use()
	{
		mIsOpen = true;
		_setLocalRadialBlurContextData();
	}

	void LocalRadialBlurComponent::unuse()
	{
		
	}

#define IF_LOCALRADIABLUR(name, pc)							\
	PostProcessChain* pc = nullptr;							\
	LocalRadialBlur* name = _getLocalRadiaBlur(pc);			\
	if (pc && name)

	void LocalRadialBlurComponent::setBlurCenter(const Vector3& value)
	{
		//mContext.mSphereCenter = value;
		if (getPosition() != value)
			setPosition(value, false);

		if (_isValidComponent())
		{
			IF_LOCALRADIABLUR(pLRBlur, pProcessChain)
				pLRBlur->setBlurCenter(getWorldPosition());
		}
	}

	void LocalRadialBlurComponent::setBlurRadius(float value)
	{
		mContext.mSphereRadius = value;
		if (_isValidComponent())
		{
			IF_LOCALRADIABLUR(pLRBlur, pProcessChain)
				pLRBlur->setBlurRadius(value);
		}
	}

	void LocalRadialBlurComponent::setBlurStrength(float value)
	{
		mContext.mBlurStrength = value;
		if (_isValidComponent())
		{
			IF_LOCALRADIABLUR(pLRBlur, pProcessChain)
				pLRBlur->setBlurStrength(value);
		}
	}

	void LocalRadialBlurComponent::setJitterIntensity(float value)
	{
		mContext.mJitterIntensity = value;
		if (_isValidComponent())
		{
			IF_LOCALRADIABLUR(pLRBlur, pProcessChain)
				pLRBlur->setJitterIntensity(value);
		}
	}

	void LocalRadialBlurComponent::setCenterHotspot(float value)
	{
		mContext.mCenterHotspot = value;
		if (_isValidComponent())
		{
			IF_LOCALRADIABLUR(pLRBlur, pProcessChain)
				pLRBlur->setCenterHotspot(value);
		}
	}

	void LocalRadialBlurComponent::setRayLength(float value)
	{
		mContext.mRayLength = value;
		if (_isValidComponent())
		{
			IF_LOCALRADIABLUR(pLRBlur, pProcessChain)
				pLRBlur->setRayLength(value);
		}
	}

	void LocalRadialBlurComponent::setAberration(float value)
	{
		mContext.mAberration = value;
		if (_isValidComponent())
		{
			IF_LOCALRADIABLUR(pLRBlur, pProcessChain)
				pLRBlur->setAberration(value);
		}
	}

	void LocalRadialBlurComponent::setHollowRange(float value)
	{
		mContext.mHollowRange = value;
		if (_isValidComponent())
		{
			IF_LOCALRADIABLUR(pLRBlur, pProcessChain)
				pLRBlur->setHollowRange(value);
		}
	}

	void LocalRadialBlurComponent::setOpen(bool open)
	{
		if (_isValidComponent())
		{
			mIsOpen = open;
			IF_LOCALRADIABLUR(pLRBlur, pProcessChain)
				if (pProcessChain->isEnable(PostProcess::LocalRadialBlur) != open)
					pProcessChain->setEnable(PostProcess::LocalRadialBlur, open);
		}
	}

	void LocalRadialBlurComponent::setTimelineAttribute(std::vector<float>& data, const Name& name)
	{
		if (name == LRB_SPHERECENTER)
		{
			setBlurCenter(Vector3(data[0], data[1], data[2]));
		}
		else if (name == LRB_SPHERERADIUS)
		{
			setBlurRadius(data[0]);
		}
		else if (name == LRB_BLURSTRENGTH)
		{
			setBlurStrength(data[0]);
		}
		else if (name == LRB_JITTERINTENSITY)
		{
			setJitterIntensity(data[0]);
		}
		else if (name == LRB_CENTERHOTSPOT)
		{
			setCenterHotspot(data[0]);
		}
		else if (name == LRB_RAYLENGTH)
		{
			setRayLength(data[0]);
		}
		else if (name == LRB_ABERRATION)
		{
			setAberration(data[0]);
		}
		else if (name == LRB_HOLLOWRANGE)
		{
			setHollowRange(data[0]);
		}
		else if (name == LRB_OPEN)
		{
			setOpen(data[0] > 0.f ? true : false);
		}
	}

	void LocalRadialBlurComponent::visibleStateChanged(bool isVisible)
	{
		if (!isVisible)
			setOpen(false);
		else
			setOpen(true);
	}

	LocalRadialBlur* LocalRadialBlurComponent::_getLocalRadiaBlur(PostProcessChain*& pProcessChain)
	{
		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		if (!pRenderSystem) return nullptr;

		RenderStrategy* pStrategy = pRenderSystem->getCurRenderStrategy();
		if (!pStrategy) return nullptr;

		pProcessChain = pStrategy->getPostProcessChain();
		if (!pProcessChain) return nullptr;

		return dynamic_cast<LocalRadialBlur*>(pProcessChain->getPostProcess(PostProcess::LocalRadialBlur));
	}

	bool LocalRadialBlurComponent::_isValidComponent()
	{
		bool isValid = true;
		if (Root::instance()->getActiveSceneManager() != getSceneManager())
			isValid = false;
		return isValid;
	}

	void LocalRadialBlurComponent::_setLocalRadialBlurContextData()
	{
		setOpen(mIsOpen);
		setBlurCenter(getPosition());
		setBlurRadius(mContext.mSphereRadius);
		setBlurStrength(mContext.mBlurStrength);
		setJitterIntensity(mContext.mJitterIntensity);
		setCenterHotspot(mContext.mCenterHotspot);
		setRayLength(mContext.mRayLength);
		setAberration(mContext.mAberration);
		setHollowRange(mContext.mHollowRange);
	}

#undef IF_LOCALRADIABLUR

	// ----------------------------------------- info --------------------------------------------------------

	using lrbCom = LocalRadialBlurComponent;

	EDITOR_REFLECT_BEGIN(LocalRadialBlurComponentInfo)
	//EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSphereCenter, lrbCom::LRB_SPHERECENTER.toString(), EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSphereRadius, lrbCom::LRB_SPHERERADIUS.toString(), EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mBlurStrength, lrbCom::LRB_BLURSTRENGTH.toString(), EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mJitterIntensity, lrbCom::LRB_JITTERINTENSITY.toString(), EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mCenterHotspot, lrbCom::LRB_CENTERHOTSPOT.toString(), EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRayLength, lrbCom::LRB_RAYLENGTH.toString(), EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mAberration, lrbCom::LRB_ABERRATION.toString(), EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mHollowRange, lrbCom::LRB_HOLLOWRANGE.toString(), EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mIsOpen, lrbCom::LRB_OPEN.toString(), EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	LocalRadialBlurComponentInfo::LocalRadialBlurComponentInfo(const Name& comType, ActorCOMMoveableType type)
		: ActorComponentInfo(comType, type)
	{
	}

	void LocalRadialBlurComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<LocalRadialBlurComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void LocalRadialBlurComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<LocalRadialBlurComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void LocalRadialBlurComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		LocalRadialBlurComponentInfo* targetInfo = static_cast<LocalRadialBlurComponentInfo*>(comInfo);
		reflect::TypeResolver<LocalRadialBlurComponentInfo>::get()->copyTo(this, targetInfo);
	}

	void LocalRadialBlurComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
			reflect::TypeResolver<LocalRadialBlurComponentInfo>::get()->overWrite(mOriginComInfo, this);
	}
}
