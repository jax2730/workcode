#include "EchoComponentStableHeaders.h"
#include "EchoImposterComponent.h"

#include "EchoImage.h"
#include "EchoEntity.h"
#include "EchoSubMesh.h"
#include "EchoMeshMgr.h"
#include "EchoActorEntityComponent.h"
#include "EchoImposterManager.h"
#include "EchoDynamicVideoParamManager.h"
#include "EchoWorldManager.h"
#include "EchoVideoConfigSystem.h"

namespace Echo
{
	EDITOR_REFLECT_BEGIN(ImposterComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(mTexturePath, "TexturePath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_ImposterScale, "ImposterScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_ImposterDist, "ImposterDist", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_ImposterFactor, "ImposterFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CullMode, "ImposterCullMode", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CullDist, "ImposterCullDist", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CullFactor, "ImposterCullFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	ImposterComponentInfo::ImposterComponentInfo(const Name& type, ActorCOMMoveableType movable)
		:ActorComponentInfo(type, movable)
	{
	}

	ImposterComponentInfo::~ImposterComponentInfo()
	{
	}

#ifdef ECHO_EDITOR
	bool ImposterComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		if (mTexturePath.empty())
		{
			return false;
		}
		resMap.insert(std::make_pair(ResourceType::NeedCopyRes, mTexturePath.c_str()));
		return true;
	}
#endif

	void ImposterComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<ImposterComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}
	void ImposterComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<ImposterComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}
	void ImposterComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		ImposterComponentInfo* pInfo = static_cast<ImposterComponentInfo*>(comInfo);
		reflect::TypeResolver<ImposterComponentInfo>::get()->copyTo(this, pInfo);
	}
	void ImposterComponentInfo::Overlay(ActorComponentInfo* comInfo)
	{
	}

	void ImposterComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			ImposterComponentInfo* oriInfo = static_cast<ImposterComponentInfo*> (mOriginComInfo);
			mTexturePath = oriInfo->mTexturePath;
			m_ImposterDist = oriInfo->m_ImposterDist;
		}
	}


	DECLAREACTORCOMTYPE_CPP(ImposterComponent)

	ImposterComponent::ImposterComponent(ActorPtr pActor, ActorCOMMoveableType type)
		:ProtocolNodeActorComponent(pActor, type)
	{
		registerTickTimeBegin();
	}

	ImposterComponent::~ImposterComponent()
	{
	}

	void ImposterComponent::init(ActorComponentInfo* info)
	{
		SetNotifiedUpdate(true);
		ProtocolNodeActorComponent::init(info);
		m_pActComInfo = info;
		ImposterComponentInfo* mInfo = static_cast<ImposterComponentInfo*>(m_pActComInfo);
		if (mInfo == nullptr)
		{
			return;
		}
		if (!mInfo->mTexturePath.empty())
			createImposter(mInfo->mTexturePath);
		m_ImposterDist = mInfo->m_ImposterDist;
		m_gradientFactor = 0.9f;
		m_RealImposterDist = m_ImposterDist * m_gradientFactor;
		SetImposterScale(mInfo->m_ImposterScale);
		SetCullMode(mInfo->m_CullMode);
		SetImposterFactor(mInfo->m_ImposterFactor);
		SetCullDistance(mInfo->m_CullDist);
		SetCullFactor(mInfo->m_CullFactor);
		if(!isRegisterNodeListener)
		{
			auto it = std::bind<void>(&ImposterComponent::OnNodeTransformUpdate, this);

			auto&& rootNodePtr = getActor()->getRootNodeCom();
			if (false == rootNodePtr.expired())
				rootNodePtr->getSceneNode()->addTransformUpdateFunc(this, it);

			isRegisterNodeListener = true;
		}
	}

	void ImposterComponent::unInit()
	{
		auto&& rootNodePtr = getActor()->getRootNodeCom();
		if (false == rootNodePtr.expired())
			rootNodePtr->getSceneNode()->removeTransformUpdateFunc(this);

		SetNotifiedUpdate(false);
		ProtocolNodeActorComponent::unInit();
		removeImposter();
	}

	void ImposterComponent::bindNode()
	{
		if (m_pParentNodeCom.expired())
			return;
		m_pParentNodeCom->UpdateComRootNode(true);
	}

	void ImposterComponent::connectActor()
	{
		ImposterComponentInfo* mInfo = static_cast<ImposterComponentInfo*>(m_pActComInfo);
		if (mInfo == nullptr)
			return;

		SetCullMode(mInfo->m_CullMode);
		SetImposterFactor(mInfo->m_ImposterFactor);
		SetCullFactor(mInfo->m_CullFactor);
		if (m_bIscreateImposter)
		{
			bool checkEnable = checkImposterEnable();
			SetImposterVisible(getActor()->getActorManager()->IsEditorPlay() ? checkEnable : m_bPreview);
			updateImposterRot();
		}

		{
			if (getActor()->getParentActor() && getActor()->getParentActor()->getIsDistanceBuilding())
			{
				getActor()->setIsDistanceBuilding(true);
			}
		}
	}

	void ImposterComponent::registerTickTimeBegin()
	{
		m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}

	bool ImposterComponent::checkImposterEnable()
	{
		bool distanceCheck = true;
		AxisAlignedBox aabb = getBoudingBox();

		VideoConfig* pVideoConfig;
		if (getWorldManager())
			pVideoConfig = getWorldManager()->getVideoConfig();
		else
			pVideoConfig = VideoConfigSystem::instance()->getActiveVideoConfig();
		float fLodDisScale = DynamicVideoParamManager::instance()->getLodDisScale();
		if (pVideoConfig)
			fLodDisScale *= pVideoConfig->getSceneLodDisScale();
		float curImposterDist = m_RealImposterDist * fLodDisScale;
		float minSqureDis = FLT_MAX;
		float gradientDis = m_ImposterDist * fLodDisScale;
		bool dissolveState = false;

		float curImposterDistSq = curImposterDist * curImposterDist;
		float gradientDisSq = gradientDis * gradientDis;
		
		if (m_CullMode == CameraCullMode::DefaultCull)
		{
			if(m_FrameTick == 0)
				m_FrameTick = Root::instance()->getNextFrameNumber();
			const unsigned long runFrame = Root::instance()->getNextFrameNumber() - m_FrameTick;
			if (runFrame >= 30)
			{
				m_FrameTick = Root::instance()->getNextFrameNumber();
				resetImposterFactor();
			}
			//gradientDisSq = m_ImposterDissolveLengthSq;
			gradientDisSq = m_ImposterLengthSq[0];
			curImposterDistSq = m_ImposterLengthSq[0];
		}

		for (auto& pageManager : getActor()->m_pPagedManager)
		{
			for (size_t i = 0; i < Target_Component_Cnt; ++i)
			{
				const Vector3 currPos = pageManager ? pageManager->getTargetComponentPosition(Target_Component_Type(i)) : getSceneManager()->getMainCamera()->getPosition();
				float curSqureDis = aabb.SquareDistPointToAABB(currPos);
				minSqureDis = std::min(minSqureDis, curSqureDis);
				if (curSqureDis < curImposterDistSq)
				{
					distanceCheck = false;
					dissolveState = false;
				}
				else if (curSqureDis <= gradientDisSq)
				{
					float sqdis = curImposterDistSq;
					float val = (curSqureDis - sqdis) / (gradientDisSq - sqdis);
					SetImposterDissolve(1.0f - val);
					dissolveState = true;
				}
				else if (curSqureDis > gradientDisSq)
				{
					SetImposterDissolve(0.0f);
					dissolveState = false;
				}
			}
		}
		m_dissolveState = dissolveState;
		return distanceCheck;
	}

	void ImposterComponent::checkGradientScope()
	{

	}

	AxisAlignedBox ImposterComponent::getBoudingBox()
	{
		AxisAlignedBox ab;
		if (m_EnableAABB == AxisAlignedBox::BOX_NULL)
		{
			ab = getActor()->getExactWorldLoadBounds();
			if (!ab.isFinite())
				ab = getActor()->getWorldBounds();
		}
		else
			ab = AxisAlignedBox(m_EnableAABB.getMinimum() + getWorldPosition(), m_EnableAABB.getMaximum() + getWorldPosition());
		return ab;
	}

	void ImposterComponent::resetImposterFactor(VideoConfig* vd)
	{
		VideoConfig* pVideoConfig = vd;
		if (!pVideoConfig)
		{
			if (getWorldManager())
				pVideoConfig = getWorldManager()->getVideoConfig();
			if (!pVideoConfig)
				pVideoConfig = VideoConfigSystem::instance()->getActiveVideoConfig();
		}

		if (!pVideoConfig) return;

		bool changeLength[2] = { false, false };
		float imposterFactor = 0.f;
		float dissolveFactor = 0.f;
		if (m_RenderFactor <= std::numeric_limits<float>::epsilon())
		{
			if (pVideoConfig)
				imposterFactor = pVideoConfig->getCurrentVideoConfig()->renderImposterfactor;
			dissolveFactor = imposterFactor * 0.9f;

			if (m_ImposterFactor[0] != imposterFactor)
			{
				m_ImposterFactor[0] = imposterFactor;
				changeLength[0] = true;
			}
		}

		if (m_CullFactor <= std::numeric_limits<float>::epsilon())
		{
			if (pVideoConfig)
				imposterFactor = pVideoConfig->getCurrentVideoConfig()->cullImposterfactor;
			if (m_ImposterFactor[1] != imposterFactor)
			{
				m_ImposterFactor[1] = imposterFactor;
				changeLength[1] = true;
			}
		}

		if (m_ImposterLengthSq[0] <= std::numeric_limits<float>::epsilon())
			changeLength[0] = true;
		if (m_ImposterLengthSq[1] <= std::numeric_limits<float>::epsilon())
			changeLength[1] = true;

		if (changeLength[0] || changeLength[1])
		{
			AxisAlignedBox aabb = getBoudingBox();
			if (changeLength[0])
			{
				m_ImposterLengthSq[0] = aabb.getHalfSize().squaredLength() / m_ImposterFactor[0];
				m_ImposterDissolveLengthSq = aabb.getHalfSize().squaredLength() / dissolveFactor;
				m_ImposterCurLengthSq = m_ImposterLengthSq[0] * m_gradientFactor * m_gradientFactor;
			}
			if (changeLength[1])
				m_ImposterLengthSq[1] = aabb.getHalfSize().squaredLength() / m_ImposterFactor[1];
		}
	}

	Name ImposterComponent::getPlatformTexName(const Name& texName)
	{
		RES_PLATFORM_TYPE tempType = (RES_PLATFORM_TYPE)Root::instance()->m_LodMeshType;

		if (PLATFORM_TYPE_DEFAULT == tempType || PLATFORM_TYPE_PC == tempType)
		{
			return texName;
		}
		else
		{
			std::string filename = texName.c_str();
			size_t dot_pos = filename.find_last_of('.');
			if (dot_pos == std::string::npos)
			{
				return texName;
			}

			std::string new_name = filename.substr(0, dot_pos) +
				"_md" +
				filename.substr(dot_pos);
			return Name(new_name);
		}
	}


	void ImposterComponent::beginFrame(EventID eventid, ProtocolHandle handle)
	{
		switch (eventid)
		{
		case EID_EveryFrame:
		{
			if (!m_bEnable || !m_bIscreateImposter)
				return;

			bool checkEnable = checkImposterEnable();


			if (checkEnable || (ActorSystem::instance()->getWaitActorBuildingFinish() && !m_pActor->isChildCreateFinish()))
			{
				useImposter(true);
			}
			else
			{
				useImposter(false);
			}
		}
		default:
			break;
		}


	}

	void ImposterComponent::EditorPlayImp()
	{
		enableImposter(true);
	}

	void ImposterComponent::EditorPauseImp(bool pause)
	{

	}

	void ImposterComponent::EditorStopImp()
	{
		enableImposter(false);
	}

	void ImposterComponent::OnNodeTransformUpdate()
	{
		if (!m_bIscreateImposter)
			return;
		updateImposterPos();
		updateImposterRot();
	}

	void ImposterComponent::NotifiChildImp(int changeMask)
	{
		if (!m_bIscreateImposter)
			return;
		updateImposterPos();
		updateImposterRot();
		AdaptiveScale();
		if (m_CullMode == DefaultCull)
		{
			m_ImposterLengthSq[0] = 0.f;
			m_ImposterLengthSq[1] = 0.f;
			m_ImposterDissolveLengthSq = 0.f;
			// 立即更新
			m_FrameTick = Root::instance()->getNextFrameNumber() - 30;
		}
	}

	void ImposterComponent::visibleStateChanged(bool visibleState)
	{
		if (visibleState && m_bEnable && ActorSystem::instance()->getMainActorManager()->IsEditorPlay() && !m_bShowImposterMesh)
			return;
		SetImposterVisible(visibleState);
	}

	AxisAlignedBox ImposterComponent::getLoadBoundingBox()
	{
		Vector3 v3Distance = { 1000.0f, 1000.0f, 1000.0f };
		ImposterComponentInfo* pInfo = nullptr;
		if (m_pActComInfo != nullptr)
			pInfo = static_cast<ImposterComponentInfo*>(m_pActComInfo);
		if (pInfo != nullptr)
		{
			if (m_CullMode == CameraCullMode::DefaultCull)
			{
				if (pInfo->m_CullFactor == m_ImposterFactor[1] && m_ImposterLengthSq[1] != 0.f)
					v3Distance = Vector3(std::sqrt(m_ImposterLengthSq[1]));
				else
				{
					float cullFactor = pInfo->m_CullFactor;
					if (cullFactor <= std::numeric_limits<float>::epsilon())
					{
						VideoConfig* pVideoConfig = nullptr;
						if (getWorldManager())
							pVideoConfig = getWorldManager()->getVideoConfig();
						if (!pVideoConfig)
							pVideoConfig = VideoConfigSystem::instance()->getActiveVideoConfig();
						if (pVideoConfig)
							cullFactor = pVideoConfig->getCurrentVideoConfig()->cullImposterfactor;
					}
					float cullLength = getBoudingBox().getHalfSize().squaredLength() / cullFactor;
					v3Distance = Vector3(std::sqrt(cullLength));
				}
			}
			else
				v3Distance = Vector3(pInfo->m_CullDist);
		}

		AxisAlignedBox tempWorldBox;
		if(getWorldAABB().isNull())
		{
			tempWorldBox = getActor()->getWorldBounds();
		}
		else
		{
			tempWorldBox = getWorldAABB();
		}
		const Vector3& minBox = tempWorldBox.getMinimum();
		const Vector3& maxBox = tempWorldBox.getMaximum();

		AxisAlignedBox loadBox(minBox - v3Distance, maxBox + v3Distance);
		return loadBox;
	}

	// =============================================== FEATURE =======================================================

	void ImposterComponent::useImposter(bool use)
	{
		if (m_bShowImposterMesh == use)
		{
			return;
		}
		m_bShowImposterMesh = use;

		if (!getActor().expired() && m_bIscreateImposter && !m_dissolveState)
		{
			for (auto& childActor : getActor()->getChildActors())
			{
				auto&& rootCom = childActor.second->getRootNodeCom();
				if (rootCom.expired())
					continue;
				rootCom->setRuntimeVisible(!use);
			}
		}
		SetImposterVisible(m_bEnable ? m_bShowImposterMesh : m_bPreview);
		updateImposterRot();
	}

	void ImposterComponent::enableImposter(bool enable)
	{
		m_bEnable = enable;
		useImposter(m_bEnable);
	}

	bool ImposterComponent::isEnable() const
	{
		return m_bEnable;
	}

	void ImposterComponent::SetImposterPath(const Name& tex)
	{
		mTexturePath = getPlatformTexName(tex);
		m_bIscreateImposter = true;
	};

	float ImposterComponent::GetEnableDistance()
	{
		/*if (m_CullMode == CameraCullMode::DefaultCull)
			return std::sqrt(m_ImposterLengthSq[0]);*/
		return m_ImposterDist;
	}

	void ImposterComponent::createImposter(const Name& texPath)
	{
		Name platformTexPath = getPlatformTexName(texPath);
		if (platformTexPath == mTexturePath && m_bIscreateImposter)
			return;
		if (!platformTexPath.empty())
		{
			if (m_bIscreateImposter && !mTexturePath.empty() && mTexturePath != platformTexPath)
			{
				if (removeImposter())
				{
					mTexturePath = platformTexPath;

					m_ImposterPos = getWorldPosition();
					m_ImposterFront = getSceneNode()->GetForward();
					m_ImposterUp = getSceneNode()->GetUp();
					//m_ImposterData.position = getActor()->getWorldBounds().getCenter();
					m_ImposterTex = mTexturePath;

					ImposterMgr* mgr = ImposterSystem::instance()->getMgr(getActor()->GetSceneManager());
					if (mgr)
						m_bIscreateImposter = mgr->addImposter(m_ImposterID, m_ImposterTex, m_ImposterScale, m_ImposterPos, m_ImposterFront, m_ImposterUp);
					//SetImposterScale(aabb.isNull() ? 2.0f : (getActor()->getWorldBounds().getMinimum()-getActor()->getWorldBounds().getMaximum()));
				}
			}
			else if(!m_bIscreateImposter && mTexturePath.empty())
			{
				mTexturePath = platformTexPath;

				m_ImposterPos = getWorldPosition();
				m_ImposterFront = getSceneNode()->GetForward();
				m_ImposterUp = getSceneNode()->GetUp();
				//m_ImposterData.position = getActor()->getWorldBounds().getCenter();
				//ImposterComponentInfo* mInfo = static_cast<ImposterComponentInfo*>(m_pActComInfo);
				//m_ImposterData.scale = mInfo ? mInfo->m_ImposterScale : 2.0f;
				m_ImposterTex = mTexturePath;

				ImposterMgr* mgr = ImposterSystem::instance()->getMgr(getActor()->GetSceneManager());
				if (mgr)
					m_bIscreateImposter = mgr->addImposter(m_ImposterID, m_ImposterTex, m_ImposterScale, m_ImposterPos, m_ImposterFront, m_ImposterUp);
				//SetImposterScale(getActor()->getWorldBounds().getMinimum(), getActor()->getWorldBounds().getMaximum());
			}
		}
	}

	bool ImposterComponent::removeImposter()
	{
		if (mTexturePath.empty())
			return false;

		m_bIscreateImposter = false;
		ImposterMgr* mgr = ImposterSystem::instance()->getMgr(getActor()->GetSceneManager());
		if (mgr)
			mgr->removeImposter(mTexturePath, m_ImposterID);
		return true;
	}

	void ImposterComponent::SetEnableDistance(float dis)
	{
		m_ImposterDist = dis;
		m_RealImposterDist = m_ImposterDist * m_gradientFactor;
	}

	float ImposterComponent::GetCullDistance()
	{
		/*if (m_CullMode == CameraCullMode::DefaultCull)
			return std::sqrt(m_ImposterLengthSq[1]);*/
		return m_CullDist;
	};

	void ImposterComponent::SetGradientFactor(float val)
	{
		m_gradientFactor = val;
		m_RealImposterDist = m_ImposterDist * m_gradientFactor;
	}

	void ImposterComponent::SetImposterVisible(bool isVisible)
	{
		if (!m_bIscreateImposter)
			return;

		ImposterMgr* mgr = ImposterSystem::instance()->getMgr(getActor()->GetSceneManager());
		if (ActorSystem::instance()->isHideRenderObject())
		{
			if (mgr)
				mgr->setImposterHide(mTexturePath, m_ImposterID, true);
			return;
		}
		if (isVisible && getRealVisibleState())
		{
			if (mgr)
				mgr->setImposterHide(mTexturePath, m_ImposterID, false);
			updateImposterScale();
			AdaptiveScale();
		}
		else
		{
			if (mgr)
				mgr->setImposterHide(mTexturePath, m_ImposterID, true);
		}
		updateImposterPos();
	}

	bool ImposterComponent::GetImposterVisible()
	{
		if (!m_bIscreateImposter)
			return false;

		bool isHide = false;
		ImposterMgr* mgr = ImposterSystem::instance()->getMgr(getActor()->GetSceneManager());
		if (mgr)
			mgr->getImposterHide(mTexturePath, m_ImposterID, isHide);
		return !isHide;
	}

	void ImposterComponent::SetImposterFactor(float factor)
	{
		if (factor < std::numeric_limits<float>::epsilon())
			factor = 0.f;
		m_RenderFactor = factor;
		if (m_ImposterFactor[0] != factor || m_ImposterLengthSq[0] == 0.f)
		{
			m_ImposterFactor[0] = factor;
			m_ImposterLengthSq[0] = 0.f;
			m_ImposterDissolveLengthSq = 0.f;
			resetImposterFactor();
		}
	}

	void ImposterComponent::SetCullFactor(float factor)
	{
		if (factor < std::numeric_limits<float>::epsilon())
			factor = 0.f;
		m_CullFactor = factor;
		if (m_ImposterFactor[1] != factor || m_ImposterLengthSq[1] == 0.f)
		{
			m_ImposterFactor[1] = factor;
			m_ImposterLengthSq[1] = 0.f;
			resetImposterFactor();
		}
	}

	void ImposterComponent::SetEnableAABBData(const Vector3& vMin, const Vector3& vMax)
	{
		m_EnableAABB.setExtents(vMin, vMax);
	}

	void ImposterComponent::SetImposterScale(float scale)
	{
		m_ImposterScale = scale;
		updateImposterScale();
		AdaptiveScale();
	}

	void ImposterComponent::SetImposterScale(const Vector3& vMin, const Vector3& vMax)
	{
		float scaleX = vMax.x - vMin.x;
		float scaleY = vMax.y - vMin.y;
		float scale = Math::Max(scaleX, scaleY);
		//float scaleZ = vMax.z - vMin.z;
		//SetImposterScale(Math::Max(Math::Max(scaleX, scaleY), scaleZ));
		SetImposterScale(scale < 1.0f ? 1.0f : scale);
	}

	void ImposterComponent::SetImposterDissolve(float dissolve)
	{
		if (!m_bIscreateImposter)
			return;
		if (dissolve > 1.0f)
		{
			dissolve = 1.0f;
		}
		if (dissolve < 0.0f)
		{
			dissolve = 0.0f;
		}

		if (Math::FloatEqual(dissolve, m_dissolve))
			return;

		m_dissolve = dissolve;

		ImposterMgr* mgr = ImposterSystem::instance()->getMgr(getActor()->GetSceneManager());
		if (mgr)
			mgr->setImposterDissolve(mTexturePath, m_ImposterID, dissolve);
	}

	float ImposterComponent::GetImposterDissolve()
	{
		if (!m_bIscreateImposter)
			return 0.0f;

		float result = .0f;
		ImposterMgr* mgr = ImposterSystem::instance()->getMgr(getActor()->GetSceneManager());
		if (mgr)
			mgr->getImposterDissolve(mTexturePath, m_ImposterID, result);
		return result;
	}

	void ImposterComponent::updateImposterPos()
	{
		if (!m_bIscreateImposter)
			return;

		Vector3 position;

		if (m_EnableAABB.isNull())
		{
			AxisAlignedBox aabb = getActor()->getExactWorldLoadBounds();
			if (aabb.isNull())
				position = getWorldPosition();
			else
				position = aabb.GetCenter();
		}
		else
		{
			position = m_EnableAABB.GetCenter();
			position = getActor()->getWorldMatrix() * position;
		}

		if (m_ImposterPos != position)
		{
			m_ImposterPos = position;

			ImposterMgr* mgr = ImposterSystem::instance()->getMgr(getActor()->GetSceneManager());
			if (mgr)
				mgr->setImposterPos(mTexturePath, m_ImposterID, position);
		}
	}

	void ImposterComponent::updateImposterRot()
	{
		if (!m_bIscreateImposter)
			return;
		m_ImposterFront = getSceneNode()->GetForward();
		m_ImposterUp = getSceneNode()->GetUp();

		ImposterMgr* mgr = ImposterSystem::instance()->getMgr(getActor()->GetSceneManager());
		if (mgr)
			mgr->setImposterDir(mTexturePath, m_ImposterID, m_ImposterFront, m_ImposterUp);
		updateImposterPos();
	}

	void ImposterComponent::updateImposterScale()
	{
		if (!m_bIscreateImposter)
			return;

		ImposterMgr* mgr = ImposterSystem::instance()->getMgr(getActor()->GetSceneManager());
		if (mgr)
			mgr->setImposterScale(mTexturePath, m_ImposterID, m_ImposterScale);
	}

	float ImposterComponent::getFactorCullLength()
	{
		if (m_ImposterLengthSq[1] == 0.f)
			resetImposterFactor();
		return std::sqrt(m_ImposterLengthSq[1]);
	}

	float ImposterComponent::AdaptiveScale()
	{
		float scale = m_ImposterScale;
		if (m_bAdaptiveScaling)
		{
			if (!m_bIscreateImposter)
				return scale;
			scale = m_ImposterScale * getActor()->getWorldScale().x;
			ImposterMgr* mgr = ImposterSystem::instance()->getMgr(getActor()->GetSceneManager());
			if (mgr)
				mgr->setImposterScale(mTexturePath, m_ImposterID, scale);
		}
		return scale;
	}

	ImposterComponentFactory::ImposterComponentFactory()
		:ActorComFactory()
	{
		mType = Echo::Name("ImposterComponent");
		mMoveableType = ALL_MOVEABLE;
		bIsEditorCom = true;
		mPathGizmoIcon = Echo::Name("ImposterComponent");
		auto&& pcm = ProtocolComponentSystem::instance();
		if (pcm)
			pcm->registerProtocolExtFunc(Name("EnableImposterCom"), ProCom_EnableImposter);
	}

	ImposterComponentFactory::~ImposterComponentFactory()
	{
		ProtocolComponentSystem::instance()->unregisterProtocolExtFunc(Name("EnableImposterCom"));
	}

	Echo::ProtocolResult ImposterComponentFactory::ProCom_EnableImposter(void* enable, Echo::ProtocolHandle handle)
	{
		auto&& pFactory = ActorComFactoryManager::getSingleton().GetActorComFactory(Name("ImposterComponent"));
		if (pFactory == nullptr)
			return PR_Falied;
		for (auto& pComponent : pFactory->GetComponentSet())
		{
			if (pComponent == nullptr)
				continue;
			auto&& divCom = static_cast<ImposterComponent*>(pComponent.get());
			bool enb = *(bool*)enable;
			divCom->enableImposter(enb);
		}
		return PR_OK;
	}
}
