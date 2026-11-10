#include "EchoComponentStableHeaders.h"
#include "EchoAtlasSphericalTerrainComponent.h"
#include "EchoSphericalTerrainManager.h"
#include "EchoPlanetManager.h"
#include "Actor/EchoNodeComponent.h"
#include "EchoPlanetEntryControlComponent.h"
namespace
{
	const Echo::Vector3 VEC3_NAN(std::numeric_limits<float>::quiet_NaN());
	const Echo::Name g_CloudInitDiffuseTex = Echo::Name("echo/biome_terrain/Textures/cloud.dds");
}

namespace Echo
{

	DECLAREACTORCOMTYPE_CPP(AtlasSphericalTerrainComponent)
		EDITOR_REFLECT_BEGIN(AtlasSphericalTerrainComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_TerrainPath, "TerrainPath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_EnableLowLODCloud, "EnableLowLODCloud", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CloudUVScale, "CloudUVScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CloudFlowSpeed, "CloudFlowSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CloudHDRFactor, "CloudHDRFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CloudMetallic, "CloudMetallic", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CloudRoughness, "CloudRoughness", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CloudAmbientFactor, "CloudAmbientFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CloudNoiseOffsetScale, "CloudNoiseOffsetScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CloudTexture, "CloudTexture", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		EDITOR_REFLECT_MEMBER_UPDATE(m_TempPlanetPath, "TempPlanetPath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_DissolveTime, "DissolveTime", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_WarFogYawAngle, "WarFogFlowingYawAngle", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_WarFogPitchAngle, "WarFogPitchAngle", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_WarFogFlowingWidth, "WarFogFlowingWidth", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_WarFogFlowingColor, "WarFogFlowingColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_WarFogFlowingSpeed, "WarFogFlowingSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_WarFogFlowingHDRFactor, "WarFogFlowingHDRFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_bUseWarFogFlowingLight, "bUseWarFogFlowingLigh", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_bRegionVisible, "bRegionVisible", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_WarFogTexture, "WarFogTexture", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_WarFogColor, "WarFogColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_WarFogDissolveTillingScale, "WarFogDissolveTillingScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_WarFogDissolveTexture, "WarFogDissolveTexture", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_WarFogDissolveColor, "WarFogDissolveColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_WarFogDissolveColorValue, "WarFogDissolveColorValue", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_WarFogDissolveSoftValue, "WarFogDissolveSoftValue", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_WarFogDissolveWidth, "WarFogDissolveWidth", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
#ifdef ECHO_EDITOR
		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_EditorRegionID, "WarFogEditorRegionID", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_bEffectAllRegion, "bEffectAllRegion", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_DissolveEditorValue, "WarFogEditorDissolveValue", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
#endif
		EDITOR_REFLECT_MEMBER_UPDATE(m_AtlasSphericalData.m_OutLineWidth, "m_OutLineWidth", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN_BASE(AtlasSphericalTerrainComponentInfo)

	AtlasSphericalTerrainComponent::AtlasSphericalTerrainComponent(ActorPtr pActor, ActorCOMMoveableType type)
		: ProtocolActorComponent(pActor, type) {

		registerTickTimeBegin();
	}
	AtlasSphericalTerrainComponent::~AtlasSphericalTerrainComponent() {

	}
	void AtlasSphericalTerrainComponent::init(ActorComponentInfo* info)
	{
		EntityObjectDesc desc;
		desc.pWorldMgr = getWorldManager();
		desc.pSceneMgr = getActor()->GetSceneManager();
		m_pTempPlanet = new EntityObject(desc);

		if (info)
		{
			AtlasSphericalTerrainComponentInfo* pInfo = static_cast<AtlasSphericalTerrainComponentInfo*>(info);
			m_sTerrainPath = pInfo->GetTerrainPath();

			m_bEnableLowLODCloud = pInfo->GetEnableLowLODCloud();
			m_fCloudUVScale = pInfo->GetCloudUVScale();
			m_fCloudFlowSpeed = pInfo->GetCloudFlowSpeed();
			m_fCloudHDRFactor = pInfo->GetCloudHDRFactor();
			m_fCloudMetallic = pInfo->GetCloudMetallic();
			m_fCloudRoughness = pInfo->GetCloudRoughness();
			m_fCloudAmbientFactor = pInfo->GetCloudAmbientFactor();
			m_fCloudNoiseOffsetScale = pInfo->GetCloudNoiseOffsetScale();
			m_strCloudTexture = pInfo->GetCloudTexture();

			mContext = pInfo->m_AtlasSphericalData;

			setDissolveTime(pInfo->GetDissolveTime());
			if (!pInfo->GetTempPlanetPath().empty())
			{
				setTempPlanetMesh(pInfo->GetTempPlanetPath());
			}
		}

		SetNotifiedUpdate(true);
	}

	void AtlasSphericalTerrainComponent::unInit()
	{
		SAFE_DELETE(m_pTempPlanet);
		SetNotifiedUpdate(false);
	}

	void AtlasSphericalTerrainComponent::replay(ActorComponentInfo* info)
	{
		if (info)
		{
			AtlasSphericalTerrainComponentInfo* pInfo = static_cast<AtlasSphericalTerrainComponentInfo*>(info);
			m_sTerrainPath = pInfo->GetTerrainPath();
		}
	}

	void AtlasSphericalTerrainComponent::copyTo(const ActorComponentPtr& actCom)
	{
		if (actCom == nullptr) return;
		AtlasSphericalTerrainComponent* pComp = static_cast<AtlasSphericalTerrainComponent*>(actCom.getPointer());
		pComp->SetTerrainPath(GetTerrainPath());
	}

	void AtlasSphericalTerrainComponent::connectCom()
	{
		if(m_pTempPlanet)
			addChildToNode(m_pTempPlanet->getParentSceneNode());
		UpdateComRootNode(true);
		CreateTerrain();
		UpdateTerrainAtmos();
	}

	void AtlasSphericalTerrainComponent::disconnectCom()
	{
		if (m_pTempPlanet)
			RemoveChildFromNode(m_pTempPlanet->getParentSceneNode());
		UpdateComRootNode(true);
		DestroyTerrain();

		// 如果有关联的星球进出组件，设置对方的Cube星球组件为空
		if (m_AtmosCom != nullptr) {
			if (m_AtmosCom->mAtlasSphericalTerrainComponent == this)
				m_AtmosCom->mAtlasSphericalTerrainComponent = nullptr;
		}
	}

	void AtlasSphericalTerrainComponent::NotifiChildImp(int changeMask)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setWorldTrans(DTransform(getWorldPosition(), getWorldRotation()), getWorldScale());
		}
	}

	void AtlasSphericalTerrainComponent::visibleStateChanged(bool activeState)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setVisible(getRealVisibleState());
		}
	}


	void AtlasSphericalTerrainComponent::OnCreateFinish()
	{
		if (IsCreateFinish())
		{
			m_pSphericalTerrain->setForceLowLod(true);
			m_pSphericalTerrain->setVisible(getRealVisibleState());

            //IMPORTANT:Enable warfog and cloud effect.
            //m_pSphericalTerrain->initLowLODCloud();
            m_pSphericalTerrain->initWarfog();
			//UpdateLowLodCloudParameter();

            //NOTE:Default enable Cloud effect.
            setEnableLowLODCloud(true);

			//地块划分
			m_pSphericalTerrain->initPlotDivision();
			m_pSphericalTerrain->setEnablePlotDivision(m_bEnablePlotDivision);

			//迷雾
			enableRegionVisualize(mContext.m_bRegionVisible);
			setWarfogMaskTexture(mContext.m_WarFogTexture);
			setRegionWarfogColor(mContext.m_WarFogColor);

			//流光
			setWarFogFlowingDirByYawAndPitch(mContext.m_WarFogYawAngle, mContext.m_WarFogPitchAngle);
			setWarFogFlowingWidth(mContext.m_WarFogFlowingWidth);
			setWarFogFlowingSpeed(mContext.m_WarFogFlowingSpeed);
			setWarFogFlowingColor(mContext.m_WarFogFlowingColor);
			setWarFogFlowingHDRFactor(mContext.m_WarFogFlowingHDRFactor);
			setWarFogIsUseFlowingLight(mContext.m_bUseWarFogFlowingLight);

			
			setWarFogDissolveTexture(mContext.m_WarFogDissolveTexture);
			setWarFogDissolveTillingScale(mContext.m_WarFogDissolveTillingScale);
			setWarFogDissolveSoftValue(mContext.m_WarFogDissolveSoftValue);
			setWarFogDissolveWidth(mContext.m_WarFogDissolveWidth);
			setWarFogDissolveColor(mContext.m_WarFogDissolveColor);
			setWarFogDissolveColorValue(mContext.m_WarFogDissolveColorValue);
#ifdef ECHO_EDITOR
			setWarFogEditorDissolveValue(mContext.m_DissolveEditorValue);
#endif

			setOutlineWidth(mContext.m_OutLineWidth);

			//setWarFogIsUseFlowingLight(true);
			/*if (m_pSphericalTerrain->m_terrainData->geometry.topology == ProceduralTerrainTopology::PTS_Sphere)
				setPlotDivisionNums(49, 49);
			else
				setPlotDivisionNums(205, 64);*/

			//testFunc(1.f);
            
            //TODO(yanghang):Test remove.
   //         enableRegionVisualize(true);
 
   //         setDisplayCorseOrFineRegion(false);
   //         setHighlightFineRegionID(26);
   //         setHighlightCorseRegionID(36);
   //         //resetHighlightCorseRegionID();
   //         //resetHighlightFineRegionID();
   //         
   //         setRegionGlowColor(ColorValue(0.9f, 0.9f, 0.9f, 1.0f));
   //         
   //         setRegionHighlightColor(ColorValue::Blue);
   //         setHighlightRegionAlpha(0.5f);
   //         
   //         setRegionHighlightWarfogColor(ColorValue::Blue);            
   //         setRegionWarfogColor(ColorValue::Green);

   //         setOutlineWidth(0.16f);
   //         setOutlineSmoothRange(0.05f);

   //         setCommonRegionAlpha(0.5f);
   //         setRegionFillColor(ColorValue::Green);
   //         setCoarseRegionFillColor(ColorValue::Green);
   //         
			////setRegionWarfogAlpha(0, 0);

   //         setWarfogMaskUVScale(100.0f);
   //         setWarfogMaskScale(0.9f);

   ////         //NOTE:Cloud test.
   ////         //setEnableLowLODCloud(true);
   ////         setCloudUVScale(0.7f);
   ////         setCloudFlowSpeed(0.8f);
   ////         setCloudHDRFactor(1.0f);
   ////         //setCloudMetallic(1.0f);
   ////         //setCloudRoughness(0.0f);
   ////         setCloudAmbientFactor(0.05f);
   ////         setCloudNoiseOffsetScale(0.05f);
   ////         setCloudTexture(("echo/biome_terrain/Textures/cloud2.dds"));

   //         setAdditionOutLineColor(ColorValue::White);
   //         //setAdditionOutLineAlpha(1.0f);
   //         setAdditionOutLineWidth(0.1f);
   //         setAdditionOutLineDistance(0.2f);

			////setWarfogMaskTexture("echo/biome_terrain/Textures/liubian_3.dds");
			////setWarfogMaskTexture("Textures/sky/guangxian.dds");

			////setWarFogSelfEmissiveLightColor(ColorValue::White);
			////setWarFogSelfEmissiveLightPower(100.f);
   //         for(int i = 0;
   //             i < 256;
   //             i++)
   //         {
   //             setCoarseRegionWarfogAlpha(i, 1.f);
   //             setRegionWarfogAlpha(i, 1.f);
			//	setWarFogDissolveValue(i, 0.f);
   //         }

		/*	setCoarseRegionWarfogAlpha(10, 0.f);
			setRegionWarfogAlpha(10, 0.f);
			setCoarseRegionWarfogAlpha(11, 0.f);
			setRegionWarfogAlpha(11, 0.f);
			setCoarseRegionWarfogAlpha(12, 0.f);
			setRegionWarfogAlpha(12, 0.f);
			setCoarseRegionWarfogAlpha(13, 0.f);
			setRegionWarfogAlpha(13, 0.f);
			setCoarseRegionWarfogAlpha(14, 0.f);
			setRegionWarfogAlpha(15, 0.f);
			setCommonRegionAlpha(0.f);*/
            
		}
	}

	/*void AtlasSphericalTerrainComponent::testFunc(float alpha_)
	{
		if (m_pSphericalTerrain->m_terrainData->geometry.topology == ProceduralTerrainTopology::PTS_Sphere)
		{
			ColorValue color;
			for (int i = 0; i < width/2; ++i)
			{
				for (int j = 0; j < height/2; j++)
				{
					if (i % 2 == 0)
					{
						if (j % 2 == 0)
							color = ColorValue::Red;
						else if (j % 2 == 1)
							color = ColorValue::Green;
					}
					else if (i % 2 == 1)
					{
						if (j % 2 == 0)
							color = ColorValue::Blue;
						else if (j % 2 == 1)
							color = ColorValue(1.f, 1.f, 0.f, 1.f);
					}

					if ((i == width || i == 0) || (j == 0 || j == height))
						color = ColorValue::White;
					color.a = alpha_;
					setPlotDivisionColor(0, i, j, color);
					setPlotDivisionColor(1, i, j, color);
					setPlotDivisionColor(2, i, j, color);
					setPlotDivisionColor(3, i, j, color);
					setPlotDivisionColor(4, i, j, color);
					setPlotDivisionColor(5, i, j, color);
				}
			}



			for (int i = width / 2; i < height; ++i)
			{
				for (int j = width / 2; j < height; ++j)
				{
					setPlotDivisionColor(0, i, j, ColorValue::Red);
					setPlotDivisionColor(1, i, j, ColorValue::Green);
					setPlotDivisionColor(2, i, j, ColorValue::Blue);
					setPlotDivisionColor(3, i, j, ColorValue::Black);
					setPlotDivisionColor(4, i, j, ColorValue(1.f,1.f,0.f,1.f));
					setPlotDivisionColor(5, i, j, ColorValue(0.f, 1.f, 1.f, 1.f));
				}
			}
		}
		else if (m_pSphericalTerrain->m_terrainData->geometry.topology == ProceduralTerrainTopology::PTS_Torus)
		{
			for (int i = 0; i < width/2; ++i)
			{
				ColorValue color;

				for (int j = 0; j < height/2; ++j)
				{
					if (i % 2 == 0)
					{
						if (j % 2 == 0)
							color = ColorValue::Red;
						else if (j % 2 == 1)
							color = ColorValue::Green;
					}
					else if (i % 2 == 1)
					{
						if (j % 2 == 0)
							color = ColorValue::Blue;
						else if (j % 2 == 1)
							color = ColorValue(1.f, 1.f, 0.f, 1.f);
					}

					if ((i == width || i == 0) && (j == 0 || j == height))
						color = ColorValue::White;


					color.a = alpha_;

					setPlotDivisionColor(0, i, j, color);
				}
			}

			for (int i = width / 2; i < width ; ++i)
			{
				ColorValue color;

				for (int j = height / 2; j < height; ++j)
				{
					setPlotDivisionColor(0, i, j, ColorValue::Red);
				}
			}
		}
	}*/


	void AtlasSphericalTerrainComponent::OnDestroy()
	{
		m_pSphericalTerrain = nullptr;
	}

	void AtlasSphericalTerrainComponent::registerTickTimeBegin()
	{
		m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}

	void AtlasSphericalTerrainComponent::beginFrame(EventID eventid, ProtocolHandle handle)
	{
		switch (eventid)
		{
		case EID_EveryFrame:
		{
			if (!m_pFactory)
				return;

			AtlasSphericalTerrainComponentFactory* pFactory = static_cast<AtlasSphericalTerrainComponentFactory*>(m_pFactory);

			if (m_pSphericalTerrain && m_pSphericalTerrain->isPlanetTaskFinished())
			{
				if (m_fDissolveTime > 0.0f)
				{
					if (m_fDissolveTimeBegin == 0.0f)
					{
						m_fDissolveTimeBegin = Echo::Root::instance()->getTimeElapsed();
						m_pTempPlanet->setVisible(getRealVisibleState());
					}

					float dissolveTimeCurr = Echo::Root::instance()->getTimeElapsed();
					float dissolveStrength = dissolveTimeCurr - m_fDissolveTimeBegin;
					dissolveStrength /= m_fDissolveTime;
					if (dissolveStrength > 1.0f)
					{
						dissolveStrength = 1.0f;
						m_pTempPlanet->setVisible(false);
					}
					m_pTempPlanet->setStippleTransparency(dissolveStrength);
					m_pSphericalTerrain->setDissolveStrength(1.0f - dissolveStrength);
				}
				else
				{
					m_pTempPlanet->setVisible(false);
				}
			}

			if (pFactory->getToggleComponent() != this)
				return;

			float currSquareDist = (float)getWorldPosition().squaredDistance(getSceneManager()->getMainCamera()->getPosition());
			float toggleSquareDist = getMaxRealRadius() + getToggleDistance();
			toggleSquareDist *= toggleSquareDist;
			if (currSquareDist < toggleSquareDist)
			{
				if (!m_bToggleState)
				{
					setForceLowLod(false);
					m_bToggleState = true;
				}
			}
			else
			{
				setForceLowLod(true);
				m_bToggleState = false;
			}
		}
		default:
			break;
		}
	}

	AxisAlignedBox AtlasSphericalTerrainComponent::getLoadBoundingBox()
	{
		if (m_pSphericalTerrain == nullptr || m_pSphericalTerrain->m_bounds.empty())
		{
			return ActorComponent::getLoadBoundingBox();
		}
		AxisAlignedBox loadBox = m_pSphericalTerrain->m_bounds.front();
		loadBox.transform(m_pSphericalTerrain->m_worldMat);
		loadBox = loadBox.expandBy(m_pSphericalTerrain->getMaxRealRadius() * 64);
		return loadBox;
	}

	void AtlasSphericalTerrainComponent::SetTerrainPath(const String& name)
	{
		if (m_sTerrainPath == name) {
			return;
		}
		m_sTerrainPath = name;
		ChangeTerrain();
	}

	bool AtlasSphericalTerrainComponent::IsCreateFinish() const
	{
		return m_pSphericalTerrain != nullptr && m_pSphericalTerrain->isCreateFinish();
	}

	float AtlasSphericalTerrainComponent::getMaxRealRadius() const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return std::numeric_limits<float>::quiet_NaN();
		}
		return m_pSphericalTerrain->getMaxRealRadius();
	}

	void AtlasSphericalTerrainComponent::setToggleEnable(bool value)
	{
		if (m_pFactory)
		{
			AtlasSphericalTerrainComponentFactory* pFactory = static_cast<AtlasSphericalTerrainComponentFactory*>(m_pFactory);
			pFactory->setComponentToggleEnable(weak_from_this(), value);
		}
	}

	void AtlasSphericalTerrainComponent::setToggleDistance(float dist)
	{
		m_fToggleDistance = dist;
	}

	void AtlasSphericalTerrainComponent::CreateTerrain()
	{
		if (m_pSphericalTerrain != nullptr) return;

		m_pSphericalTerrain = getSceneManager()->getPlanetManager()->createSphericalTerrain(m_sTerrainPath, false, false, true, DTransform(getWorldPosition(), getWorldRotation()), getWorldScale());
		if (m_pSphericalTerrain) {
			if (m_fDissolveTime > 0)
			{
				if(m_pTempPlanet)
					m_pTempPlanet->setStippleTransparency(0.0f);
				m_pSphericalTerrain->setDissolveStrength(1.0f);
			}
			if (m_pSphericalTerrain->isWaitCreateFinish()) {
				m_pSphericalTerrain->addLoadListener((SphericalTerrain::LoadListener*)this);
			}
			else {
				OnCreateFinish();
			}
		}
	}

	void AtlasSphericalTerrainComponent::DestroyTerrain()
	{
		if (m_pSphericalTerrain == nullptr) return;
		m_pSphericalTerrain->removeLoadListener((SphericalTerrain::LoadListener*)this);
		
		{
			//tthread::lock_guard<tthread::recursive_mutex> lock(mSphericalTerrainLock);
			getSceneManager()->getPlanetManager()->destroySphericalTerrain(m_pSphericalTerrain);
			m_pSphericalTerrain = nullptr;
		}
	}

	void AtlasSphericalTerrainComponent::ChangeTerrain()
	{
		DestroyTerrain();
		CreateTerrain();
		UpdateTerrainAtmos();
	}

	void AtlasSphericalTerrainComponent::setForceLowLod(bool value)
	{
		if(m_pSphericalTerrain && m_pSphericalTerrain->isCreateFinish())
			m_pSphericalTerrain->setForceLowLod(value);
	}

	void AtlasSphericalTerrainComponent::loadTempPlanet(const Name& meshPath)
	{
		m_pTempPlanet->clearAllEntity();
		m_pTempPlanet->changeEntity(meshPath, getActor()->isSyncLoadActor());
	}

	//void AtlasSphericalTerrainComponent::UpdateLowLodCloudParameter()
	//{
	//	if (m_pSphericalTerrain)
	//	{
	//		m_pSphericalTerrain->setEnableLowLODCloud(m_bEnableLowLODCloud);
	//		m_pSphericalTerrain->setCloudUVScale(m_fCloudUVScale);
	//		m_pSphericalTerrain->setCloudFlowSpeed(m_fCloudFlowSpeed);
	//		m_pSphericalTerrain->setCloudHDRFactor(m_fCloudHDRFactor);
	//		m_pSphericalTerrain->setCloudMetallic(m_fCloudMetallic);
	//		m_pSphericalTerrain->setCloudRoughness(m_fCloudRoughness);
	//		m_pSphericalTerrain->setCloudAmbientFactor(m_fCloudAmbientFactor);
	//		m_pSphericalTerrain->setCloudNoiseOffsetScale(m_fCloudNoiseOffsetScale);
	//		if (!m_strCloudTexture.empty())
	//			m_pSphericalTerrain->setCloudTexture(m_strCloudTexture);
	//	}
	//}

        //NOTE: selected region will be highlight display.

	void AtlasSphericalTerrainComponent::setDissolveTime(float value)
	{
		m_fDissolveTime = value;
		m_fDissolveTimeBegin = 0.0f;
	}
    
    void AtlasSphericalTerrainComponent::enableRegionVisualize(bool enable)
    {
		mContext.m_bRegionVisible = enable;
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setEnableRegionVisualEffect(enable);
        }   
    }

    bool AtlasSphericalTerrainComponent::setHighlightCorseRegionID(int id)
    {
        bool result =false;
        if(m_pSphericalTerrain)
        {
            result = m_pSphericalTerrain->setHighlightCorseRegionID(id);
        }
        return(result);
    }
        
    void AtlasSphericalTerrainComponent::setDisplayCorseOrFineRegion(bool enableCorse)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setDisplayCorseOrFineRegion(enableCorse);
        }
    }
        
    void AtlasSphericalTerrainComponent::setRegionGlowColor(const ColorValue& color)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setRegionGlowColor(color);
        }
    }
    void AtlasSphericalTerrainComponent::setRegionHighlightColor(const ColorValue& color)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setRegionHighlightColor(color);
        }
    }

	int AtlasSphericalTerrainComponent::getRegionID(const DVector3& worldPos) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check AtlasSphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return -1;
		}
		if (!m_pSphericalTerrain->m_terrainData->sphericalVoronoiRegion.Loaded)
		{
			LogManager::instance()->logMessage("AtlasSphericalTerrainComponent::getRegionID : SphericalVoronoiRegion isn't loaded successfully", LML_CRITICAL);
			return -1;
		}
		return m_pSphericalTerrain->computeRegionInformation(worldPos);
	}

	NaiveWrapper<SphericalVoronoiRegionDefine> AtlasSphericalTerrainComponent::getSphericalVoronoiRegionDefine(int id)
	{

		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check AtlasSphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return NaiveWrapper<SphericalVoronoiRegionDefine>();
		}
		if (!m_pSphericalTerrain->m_terrainData->sphericalVoronoiRegion.Loaded)
		{
			LogManager::instance()->logMessage("AtlasSphericalTerrainComponent::getSphericalVoronoiRegionDefine : SphericalVoronoiRegion isn't loaded successfully", LML_CRITICAL);
			return NaiveWrapper<SphericalVoronoiRegionDefine>();
		}
		if ((id < 0 || id >= m_pSphericalTerrain->m_terrainData->sphericalVoronoiRegion.defineArray.size()))
		{
			LogManager::instance()->logMessage("AtlasSphericalTerrainComponent::getSphericalVoronoiRegionDefine : SphericalVoronoiRegion define ID " + std::to_string(id) + " invalid.", LML_CRITICAL);
			return NaiveWrapper<SphericalVoronoiRegionDefine>();
		}

		return NaiveWrapper<SphericalVoronoiRegionDefine>(m_pSphericalTerrain->m_terrainData->sphericalVoronoiRegion.defineArray[id]);
	}

	std::vector<Vector3> AtlasSphericalTerrainComponent::GetCoarseLayerCenters(bool ToWorldSpace) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check AtlasSphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return std::vector<Vector3>();
		}
		return m_pSphericalTerrain->GetCoarseLayerCenters(ToWorldSpace);
	}

	std::vector<Vector3> AtlasSphericalTerrainComponent::GetFineLayerCenters(bool ToWorldSpace) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check AtlasSphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return std::vector<Vector3>();
		}
		return m_pSphericalTerrain->GetFineLayerCenters(ToWorldSpace);
	}

	std::vector<int> AtlasSphericalTerrainComponent::GetCoarseLayerAdjacentRegions(int id) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check AtlasSphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return std::vector<int>();
		}

		return m_pSphericalTerrain->GetCoarseLayerAdjacentRegions(id);
	}

	std::vector<int> AtlasSphericalTerrainComponent::GetFineLayerAdjacentRegions(int id) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check AtlasSphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return std::vector<int>();
		}

		return m_pSphericalTerrain->GetFineLayerAdjacentRegions(id);
	}

    void AtlasSphericalTerrainComponent::resetHighlightCorseRegionID()
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setHighlightCorseRegionID(-1);
        }
    }

    bool AtlasSphericalTerrainComponent::setHighlightFineRegionID(int id)
    {
        bool result=  false;
        if(m_pSphericalTerrain)
        {
            result = m_pSphericalTerrain->setHighlightFineRegionID(id);
        }
        return (result);
    }
    void AtlasSphericalTerrainComponent::resetHighlightFineRegionID()
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setHighlightFineRegionID(-1);
        }        
    }

	AtlasSphericalTerrainComponentFactory::AtlasSphericalTerrainComponentFactory()
		:ActorComFactory()
	{
		mType = Echo::Name("AtlasSphericalTerrainComponent");
		mMoveableType = UNMOVEABLE;
		bIsEditorCom = true;
		mPathGizmoIcon = Echo::Name("AtlasSphericalTerrainComponent");
	}

	AtlasSphericalTerrainComponentFactory::~AtlasSphericalTerrainComponentFactory()
	{
	}

	void AtlasSphericalTerrainComponentFactory::setToggleEnable(bool value)
	{
		m_bToggleEnable = value;
		if (!m_bToggleEnable && m_pToggleComponent)
		{
			m_pToggleComponent->m_bToggleState = false;
			m_pToggleComponent->setForceLowLod(true);
		}
	}

	void AtlasSphericalTerrainComponentFactory::setComponentToggleEnable(const EchoWeakPtr<AtlasSphericalTerrainComponent>& pComponent, bool value)
	{
		if (!pComponent)
			return;

		if (!m_bToggleEnable)
			return;

		if (value)
		{
			if (m_pToggleComponent && m_pToggleComponent != pComponent)
			{
				m_pToggleComponent->m_bToggleState = false;
				m_pToggleComponent->setForceLowLod(true);
			}
			m_pToggleComponent = pComponent;
		}
		else
		{
			if (m_pToggleComponent == pComponent)
				m_pToggleComponent.Reset();
		}
	}

     void AtlasSphericalTerrainComponent::setOutlineWidth(float width)
    {
		mContext.m_OutLineWidth = width;
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setOutlineWidth(width);
        }   
    }
    
    void AtlasSphericalTerrainComponent::setCommonRegionAlpha(float alpha)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setCommonRegionAlpha(alpha);
        }
    }
    
    void AtlasSphericalTerrainComponent::setHighlightRegionAlpha(float alpha)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setHighlightRegionAlpha(alpha);
        }
    }

    void AtlasSphericalTerrainComponent::setRegionWarfogAlpha(int id, float alpha)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setRegionWarfogAlpha(id, alpha);
        }
    }

    void AtlasSphericalTerrainComponent::setCoarseRegionWarfogAlpha(int id, float alpha)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setCoarseRegionWarfogAlpha(id, alpha);
        }
    }
    
    void AtlasSphericalTerrainComponent::setRegionWarfogColor(const ColorValue& color)
    {
		mContext.m_WarFogColor = color;
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setRegionWarfogColor(color);
        }
    }

	void AtlasSphericalTerrainComponent::setPlotDivisionColor(int idx, int x, int y, const ColorValue& color)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setPlotDivisionColor(idx,x,y,color);
		}
	}

	void AtlasSphericalTerrainComponent::setPlotDivisionNums(int x, int y)
	{
		if (m_pSphericalTerrain)
			m_pSphericalTerrain->setPlotDivisionNums(x, y);
	}

	bool AtlasSphericalTerrainComponent::getPlotDivisionDataByPosition(const Vector3& pos, int& idx, int& x, int& y)
	{
		if (m_pSphericalTerrain)
			return m_pSphericalTerrain->getPlotDivisionDataByPosition(pos, idx, x, y);
		return false;
	}

	bool AtlasSphericalTerrainComponent::getPlotDivisionDataByWorldPosition(const DVector3& pos, int& idx, int& x, int& y)
	{
		if (m_pSphericalTerrain)
			return m_pSphericalTerrain->getPlotDivisionDataByWorldPosition(pos, idx, x, y);
		return false;
	}

	void AtlasSphericalTerrainComponent::UpdateTerrainAtmos() {
		if (m_pSphericalTerrain == nullptr)return;

		if (m_AtmosCom == nullptr) {
			m_pSphericalTerrain->m_atmosProperty = nullptr;
			//m_pSphericalTerrain->m_atmosEnable = false;
		}
		else {
			m_pSphericalTerrain->m_atmosProperty = m_AtmosCom->GetAtmosphereProperty();
			SetAtmosPropertyDirty();
		}
	}

	void AtlasSphericalTerrainComponent::SetAtmosPropertyDirty() {
		if (m_pSphericalTerrain == nullptr)return;
		m_pSphericalTerrain->m_atmosProerptyDirty = true;
	}

	bool AtlasSphericalTerrainComponent::SetPlanetEntryComPtr(PlanetEntryControlComponent* ptr) {
		//解绑操作
		if (ptr == nullptr)
		{
			m_AtmosCom = ptr;
			UpdateTerrainAtmos();
			return false;
		}
		//当前已经与一个星球过渡组件绑定
		if (m_AtmosCom != nullptr) return false;
		m_AtmosCom = ptr;
		UpdateTerrainAtmos();
		return true;
	}

	void AtlasSphericalTerrainComponent::setEnablePlotDivision(bool bEnable)
	{
		m_bEnablePlotDivision = bEnable;
		if (m_pSphericalTerrain)
			m_pSphericalTerrain->setEnablePlotDivision(bEnable);
	}

    void
    AtlasSphericalTerrainComponent::setRegionFillColor(const ColorValue& color)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setRegionFillColor(color);
        }
    }

    void
    AtlasSphericalTerrainComponent::setCoarseRegionFillColor(const ColorValue& color)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setCoarseRegionFillColor(color);
        }
    }

    void
    AtlasSphericalTerrainComponent::setSingleRegionFillColor(int id, const ColorValue& color)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setSingleRegionFillColor(id, color);
        }
    }
    
    void
    AtlasSphericalTerrainComponent::setSingleCoarseRegionFillColor(int id, const ColorValue& color)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setSingleCoarseRegionFillColor(id, color);
        }
    }
    
    void
    AtlasSphericalTerrainComponent::setRegionHighlightWarfogColor(const ColorValue& color)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setRegionHighlightWarfogColor(color);
        }
    }
    void AtlasSphericalTerrainComponent::setWarfogMaskUVScale(const float scale)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setWarfogMaskUVScale(scale);
        }
    }
    void AtlasSphericalTerrainComponent::setWarfogMaskScale(const float scale)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setWarfogMaskScale(scale);
        }
    }

    void AtlasSphericalTerrainComponent::setOutlineSmoothRange(const float range)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setOutlineSmoothRange(range);
        }
    }

    void AtlasSphericalTerrainComponent::setAdditionOutLineColor(const ColorValue& color)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setAdditionOutLineColor(color);
        }
    }
    
    void AtlasSphericalTerrainComponent::setAdditionOutLineAlpha(const float& v)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setAdditionOutLineAlpha(v);
        }
    }
    
    void AtlasSphericalTerrainComponent::setAdditionOutLineWidth(const float& v)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setAdditionOutLineWidth(v);
        }
    }
    
    void AtlasSphericalTerrainComponent::setAdditionOutLineDistance(const float& v)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setAdditionOutLineDistance(v);
        }
    }
    
    void AtlasSphericalTerrainComponent::setWarfogMaskTexture(const String& path)
    {
		mContext.m_WarFogTexture = path;
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setWarfogMaskTexture(path);
        }
    }
	void AtlasSphericalTerrainComponent::setWarFogSelfEmissiveLightColor(const ColorValue& color_)
	{
		if (m_pSphericalTerrain)
			m_pSphericalTerrain->setWarFogSelfEmissiveLightColor(color_);
	}

	void AtlasSphericalTerrainComponent::setWarFogSelfEmissiveLightPower(float power)
	{
		if (m_pSphericalTerrain)
			m_pSphericalTerrain->setWarFogSelfEmissiveLightPower(power);
	}

	void AtlasSphericalTerrainComponent::setWarFogFlowingDirByYawAndPitch(float yaw, float pitch)
	{
		mContext.m_WarFogYawAngle = yaw;
		mContext.m_WarFogPitchAngle = pitch;
		if (m_pSphericalTerrain)
			m_pSphericalTerrain->setWarFogFlowingDirByYawAndPitch(yaw, pitch);
	}

	void AtlasSphericalTerrainComponent::setWarFogFlowingYawAngle(float yaw)
	{
		mContext.m_WarFogYawAngle = yaw;
		if (m_pSphericalTerrain)
			m_pSphericalTerrain->setWarFogFlowingDirByYawAndPitch(mContext.m_WarFogYawAngle, mContext.m_WarFogPitchAngle);
	}

	void AtlasSphericalTerrainComponent::setWarFogFlowingPitchAngle(float pitch)
	{
		mContext.m_WarFogPitchAngle = pitch;
		if (m_pSphericalTerrain)
			m_pSphericalTerrain->setWarFogFlowingDirByYawAndPitch(mContext.m_WarFogYawAngle, mContext.m_WarFogPitchAngle);

	}

	//void AtlasSphericalTerrainComponent::setWarFogFlowingDir(const Vector3& dir)
	//{
	//	if (m_pSphericalTerrain)
	//		m_pSphericalTerrain->setWarFogFlowingDir(dir);
	//}

	void AtlasSphericalTerrainComponent::setWarFogFlowingWidth(float width)
	{
		mContext.m_WarFogFlowingWidth = width;
		if (m_pSphericalTerrain)
			m_pSphericalTerrain->setWarFogFlowingWidth(width);
	}

	void AtlasSphericalTerrainComponent::setWarFogFlowingSpeed(float speed)
	{
		mContext.m_WarFogFlowingSpeed = speed;
		if (m_pSphericalTerrain)
			m_pSphericalTerrain->setWarFogFlowingSpeed(speed);
	}

	void AtlasSphericalTerrainComponent::setWarFogFlowingColor(const ColorValue& color)
	{
		mContext.m_WarFogFlowingColor = color;
		if (m_pSphericalTerrain)
			m_pSphericalTerrain->setWarFogFlowingColor(color);
	}

	void AtlasSphericalTerrainComponent::setWarFogFlowingHDRFactor(float factor)
	{
		mContext.m_WarFogFlowingHDRFactor = factor;
		if (m_pSphericalTerrain)
			m_pSphericalTerrain->setWarFogFlowingHDRFactor(factor);
	}

	void AtlasSphericalTerrainComponent::setWarFogIsUseFlowingLight(bool bUse)
	{
		mContext.m_bUseWarFogFlowingLight = bUse;
		if (m_pSphericalTerrain)
			m_pSphericalTerrain->setWarFogIsUseFlowingLight(bUse);
	}

	void AtlasSphericalTerrainComponent::setWarFogDissolveTexture(const std::string& path)
	{
		mContext.m_WarFogDissolveTexture = path;
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setWarFogDissolveTexture(path);
		}
	}

	void AtlasSphericalTerrainComponent::setWarFogDissolveTillingScale(float scale)
	{
		mContext.m_WarFogDissolveTillingScale = scale;
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setWarFogDissolveTillingScale(scale);
		}
	}

	void AtlasSphericalTerrainComponent::setWarFogDissolveSoftValue(float value)
	{
		mContext.m_WarFogDissolveSoftValue = value;
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setWarFogDissolveSoftValue(value);
		}
	}

	void AtlasSphericalTerrainComponent::setWarFogDissolveWidth(float width)
	{
		mContext.m_WarFogDissolveWidth = width;
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setWarFogDissolveWidth(width);
		}
	}

	void AtlasSphericalTerrainComponent::setWarFogDissolveColor(const Vector3& color)
	{
		mContext.m_WarFogDissolveColor = color;
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setWarFogDissolveColor(color);
		}
	}

	void AtlasSphericalTerrainComponent::setWarFogDissolveColorValue(float value)
	{
		mContext.m_WarFogDissolveColorValue = value;
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setWarFogDissolveColorValue(value);
		}
	}


	void AtlasSphericalTerrainComponent::setTempPlanetMesh(const Name& meshName)
	{
		m_nTempPlanetPath = meshName;
		m_fDissolveTimeBegin = 0.0f;
		loadTempPlanet(meshName);
	}

	void AtlasSphericalTerrainComponent::setTempPlanetPosition(const DBVector3& pos)
	{
		if (m_pTempPlanet)
		{
			m_pTempPlanet->setPosition(pos);
		}
	}

	const DBVector3& AtlasSphericalTerrainComponent::getTempPlanetPosition()
	{
		if (m_pTempPlanet)
		{
			return m_pTempPlanet->getPosition();
		}
		else
		{
			return DBVector3::ZERO;
		}
	}

	void AtlasSphericalTerrainComponent::setTempPlanetOrientation(const Quaternion& qua)
	{
		if (m_pTempPlanet)
		{
			m_pTempPlanet->setOrientation(qua);
		}
	}

	const Quaternion& AtlasSphericalTerrainComponent::getTempPlanetOrientation()
	{
		if (m_pTempPlanet)
		{
			return m_pTempPlanet->getOrientation();
		}
		else
		{
			return Quaternion::IDENTITY;
		}
	}

	void AtlasSphericalTerrainComponent::setTempPlanetScale(const Vector3& scale)
	{
		if (m_pTempPlanet)
		{
			m_pTempPlanet->setScale(scale);
		}
	}

	const Vector3& AtlasSphericalTerrainComponent::getTempPlanetScale()
	{
		if (m_pTempPlanet)
		{
			return m_pTempPlanet->getScale();
		}
		else
		{
			return Vector3::UNIT_SCALE;
		}
	}

	void AtlasSphericalTerrainComponent::setTempPlanetWorldPosition(const DBVector3& pos)
	{
		if (m_pTempPlanet)
		{
			auto tempPos = convertWorldToLocalPosition(pos);
			setTempPlanetPosition(tempPos);
		}
	}

	const DBVector3& AtlasSphericalTerrainComponent::getTempPlanetWorldPosition()
	{
		if (m_pTempPlanet && m_pTempPlanet->getSceneNode())
		{
			return m_pTempPlanet->getSceneNode()->_getDerivedPosition();
		}
		else
		{
			return DBVector3::ZERO;
		}
	}

	void AtlasSphericalTerrainComponent::setTempPlanetWorldOrientation(const Quaternion& qua)
	{
		if (m_pTempPlanet)
		{
			auto tempPos = convertWorldToLocalRotation(qua);
			setTempPlanetOrientation(tempPos);
		}
	}

	const Quaternion& AtlasSphericalTerrainComponent::getTempPlanetWorldOrientation()
	{
		if (m_pTempPlanet && m_pTempPlanet->getSceneNode())
		{
			return m_pTempPlanet->getSceneNode()->_getDerivedOrientation();
		}
		else
		{
			return Quaternion::IDENTITY;
		}
	}

	void AtlasSphericalTerrainComponent::setTempPlanetWorldScale(const Vector3& scale)
	{
		if (m_pTempPlanet)
		{
			auto tempScale = convertWorldToLocalScale(scale);
			setTempPlanetScale(tempScale);
		}
	}

	const Vector3& AtlasSphericalTerrainComponent::getTempPlanetWorldScale()
	{
		if (m_pTempPlanet && m_pTempPlanet->getSceneNode())
		{
			return m_pTempPlanet->getSceneNode()->_getDerivedScale();
		}
		else
		{
			return Vector3::UNIT_SCALE;
		}
	}

	CameraCullMode AtlasSphericalTerrainComponent::getTempPlanetCullMode()
	{
		if (m_pTempPlanet)
		{
			return m_pTempPlanet->getCameraCullMode();
		}
		else
		{
			return CameraCullMode::DefaultCull;
		}
	}

	void AtlasSphericalTerrainComponent::setTempPlanetCullMode(int mode)
	{
		if (m_pTempPlanet)
		{
			m_pTempPlanet->setCameraCullMode(static_cast<CameraCullMode>(mode));
		}
	}

	void AtlasSphericalTerrainComponent::setTempPlanetRenderDistance(float value)
	{
		if (m_pTempPlanet)
		{
			m_pTempPlanet->setRenderingDistance(value);
		}
	}

	float AtlasSphericalTerrainComponent::getTempPlanetRenderDistance()
	{
		if (m_pTempPlanet)
		{
			return m_pTempPlanet->getRenderingDistance();
		}
		else
		{
			return 0.0f;
		}
	}


#ifdef ECHO_EDITOR
	void AtlasSphericalTerrainComponent::setEditorRegionID(int id)
	{
		mContext.m_EditorRegionID = id;
	}

	void AtlasSphericalTerrainComponent::setWarFogEditorDissolveValue(float value)
	{
		mContext.m_DissolveEditorValue = value;
		if (!m_pSphericalTerrain)
			return;
		if (mContext.m_bEffectAllRegion)
		{
			for(int i = 0 ; i < 256 ; ++i)
			{
				m_pSphericalTerrain->setWarFogDissolveValue(i, value);
			}
		}
		else
			m_pSphericalTerrain->setWarFogDissolveValue(mContext.m_EditorRegionID, value);
	}

	void AtlasSphericalTerrainComponent::setEnableEffectAllRegion(bool isEffect)
	{
		mContext.m_bEffectAllRegion = isEffect;
	}
#endif

	void AtlasSphericalTerrainComponent::setWarFogDissolveValue(int i, float value)
	{
		if (m_pSphericalTerrain)
			m_pSphericalTerrain->setWarFogDissolveValue(i, value);
	}



	//NOTE:Low LOD Cloud visual effect control.
	void AtlasSphericalTerrainComponent::setEnableLowLODCloud(bool enalbe)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setEnableLowLODCloud(enalbe);
		}
		m_bEnableLowLODCloud = enalbe;
	}
	void AtlasSphericalTerrainComponent::setCloudUVScale(float v)
	{
		v = std::clamp(v, 0.f, 100.f);
	/*	if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setCloudUVScale(v);
		}*/
		m_fCloudUVScale = v;
	}
	void AtlasSphericalTerrainComponent::setCloudFlowSpeed(float v)
	{
		v = std::clamp(v, 0.f, 10.f);
	/*	if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setCloudFlowSpeed(v);
		}*/
		m_fCloudFlowSpeed = v;
	}
	void AtlasSphericalTerrainComponent::setCloudHDRFactor(float v)
	{
		v = std::clamp(v, 0.f, 10.f);
	/*	if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setCloudHDRFactor(v);
		}*/
		m_fCloudHDRFactor = v;
	}
	void AtlasSphericalTerrainComponent::setCloudMetallic(float v)
	{
		v = std::clamp(v, 0.f, 1.f);
		//if (m_pSphericalTerrain)
		//{
		//	m_pSphericalTerrain->setCloudMetallic(v);
		//}
		m_fCloudMetallic = v;
	}
	void AtlasSphericalTerrainComponent::setCloudRoughness(float v)
	{
		v = std::clamp(v, 0.f, 1.f);
	/*	if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setCloudRoughness(v);
		}*/
		m_fCloudRoughness = v;
	}
	void AtlasSphericalTerrainComponent::setCloudAmbientFactor(float v)
	{
		v = std::clamp(v, 0.f, 1.f);
		/*if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setCloudAmbientFactor(v);
		}*/
		m_fCloudAmbientFactor = v;

	}
	void AtlasSphericalTerrainComponent::setCloudNoiseOffsetScale(float v)
	{
		v = std::clamp(v, 0.f, 1.f);
	/*	if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setCloudNoiseOffsetScale(v);
		}*/
		m_fCloudNoiseOffsetScale = v;
	}

    void AtlasSphericalTerrainComponent::setCloudTexture(const String& path)
    {
	/*	if (m_pSphericalTerrain)
		{
			if (path.empty())
				m_pSphericalTerrain->setCloudTexture(g_CloudInitDiffuseTex.toString());
			else
				m_pSphericalTerrain->setCloudTexture(path);
		}*/
		m_strCloudTexture = path;
    }

	bool AtlasSphericalTerrainComponent::getEnableLowLODCloud()
	{
		return m_bEnableLowLODCloud;
	}
	float AtlasSphericalTerrainComponent::getCloudUVScale()
	{
		return m_fCloudUVScale;
	}
	float AtlasSphericalTerrainComponent::getCloudFlowSpeed()
	{
		return m_fCloudFlowSpeed;
	}
	float AtlasSphericalTerrainComponent::getCloudHDRFactor()
	{
		return m_fCloudHDRFactor;
	}
	float AtlasSphericalTerrainComponent::getCloudMetallic()
	{
		return m_fCloudMetallic;
	}
	float AtlasSphericalTerrainComponent::getCloudRoughness()
	{
		return m_fCloudRoughness;
	}
	float AtlasSphericalTerrainComponent::getCloudAmbientFactor()
	{
		return m_fCloudAmbientFactor;
	}
	float AtlasSphericalTerrainComponent::getCloudNoiseOffsetScale()
	{
		return m_fCloudNoiseOffsetScale;
	}
	const String& AtlasSphericalTerrainComponent::getCloudTexture()
	{
		return m_strCloudTexture;
	}

	DVector3 AtlasSphericalTerrainComponent::getSurfaceWs(const DVector3& worldPos, Vector3* faceNormal) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return VEC3_NAN;
		}
		return m_pSphericalTerrain->getSurfaceWs(worldPos, faceNormal);
	}

	Vector3 AtlasSphericalTerrainComponent::getSurfaceLs(const Vector3& localPos, Vector3* faceNormal) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return VEC3_NAN;
		}
		return m_pSphericalTerrain->getSurfaceLs(localPos.normalisedCopy(), faceNormal);
	}

	DVector3 AtlasSphericalTerrainComponent::getSurfaceFinestWs(const DVector3& worldPos, Vector3* faceNormal) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return VEC3_NAN;
		}
		return m_pSphericalTerrain->getSurfaceFinestWs(worldPos, faceNormal);
	}

	Vector3 AtlasSphericalTerrainComponent::getSurfaceFinestLs(const Vector3& localPos, Vector3* faceNormal) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return VEC3_NAN;
		}
		return m_pSphericalTerrain->getSurfaceFinestLs(localPos.normalisedCopy(), faceNormal);
	}
#ifdef ECHO_EDITOR
	bool AtlasSphericalTerrainComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		if (m_TempPlanetPath.empty()) {
			return false;
		}
		resMap.insert(std::make_pair(ResourceType::Mesh, m_TempPlanetPath.c_str()));
		return true;
	}
#endif // ECHO_EDITOR
}
