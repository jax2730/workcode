#include "EchoComponentStableHeaders.h"
#include "EchoSphericalTerrainComponent.h"
#include "EchoSphericalTerrainManager.h"
#include "EchoPlanetManager.h"
#include "EchoPhysicsManager.h"
#include "EchoPhysXSystem.h"
#include "EchoPhysXScene.h"
#include "EchoMeshColliderComponent.h"
#include "EchoPavementComponent.h"
#include "Actor/EchoNodeComponent.h"
#include "EchoPhysXMeshSerializer.h"
#include "EchoPlanetEntryControlComponent.h"
#include "EchoPiecewiseBezierComponent.h"
#include "EchoPlanetRoadResource.h"
#include "EchoPhysXManager.h"
#include "EchoRoadManager.h"
#include"EchoMeshMgr.h"
#include "EchoSubMesh.h"
#include "EchoStringConverter.h"
#include "EchoImage.h"
#include "cJSON.h"
namespace 
{
	const Echo::Vector3 VEC3_NAN(std::numeric_limits<float>::quiet_NaN());
	const Echo::Name g_CloudInitDiffuseTex = Echo::Name("echo/biome_terrain/Textures/cloud.dds");
}

namespace Echo {

	SphericalTerrainComponent::CircleModifier::CircleModifier(const CircleModifier& other) { 
		*this = other; 
	}
	SphericalTerrainComponent::CircleModifier& SphericalTerrainComponent::CircleModifier::operator=(const CircleModifier& other) {
		if (this == &other) return *this; 
		memcpy((void*)this, (void*)&other, sizeof(SphericalTerrainComponent::CircleModifier));
		return *this;
	}
	SphericalTerrainComponent::RectangleModifier::RectangleModifier(const RectangleModifier& other) {
		*this = other;
	}
	SphericalTerrainComponent::RectangleModifier& SphericalTerrainComponent::RectangleModifier::operator=(const RectangleModifier& other) {
		if (this == &other) return *this;
		memcpy((void*)this, (void*)&other, sizeof(SphericalTerrainComponent::RectangleModifier));
		return *this;
	}

}

namespace Echo {

	DECLAREACTORCOMTYPE_CPP(SphericalTerrainComponent)
	EDITOR_REFLECT_BEGIN(SphericalTerrainComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_IsUsePhysics, "IsUsePhysics", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_TerrainPath, "TerrainPath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_MeshFlags, "MeshFlags", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_MaterialId, "MaterialId", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_FilterGroupType, "FilterGroupType", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_CanInteractFilterGroupType, "CanInteractFilterGroupType", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_EnableLowLODCloud, "EnableLowLODCloud", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_CloudUVScale, "CloudUVScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_CloudFlowSpeed, "CloudFlowSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_CloudHDRFactor, "CloudHDRFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_CloudMetallic, "CloudMetallic", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_CloudRoughness, "CloudRoughness", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_CloudAmbientFactor, "CloudAmbientFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_CloudNoiseOffsetScale, "CloudNoiseOffsetScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_CloudTexture, "CloudTexture", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		//////////////////////////////////////
	EDITOR_REFLECT_MEMBER_UPDATE(mCloudProperty.EnableVolumetricCloud, "EnableVolumetricCloud", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mCloudProperty.LayerBottomAltitudeKm, "LayerBottomAltitudeKm", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mCloudProperty.LayerHeightKm, "LayerHeightKm", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mCloudProperty.TracingStartMaxDistance, "TracingStartMaxDistanceKm", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mCloudProperty.TracingMaxDistance, "TracingMaxDistanceKm", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mCloudProperty.CloudBasicNoiseScale, "CloudBasicNoiseScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mCloudProperty.StratusCoverage, "StratusCoverage", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mCloudProperty.StratusContrast, "StratusContrast", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mCloudProperty.MaxDensity, "MaxDensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mCloudProperty.CloudColor, "CloudColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mCloudProperty.WindOffset, "WindOffset", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mCloudProperty.CloudPhaseG, "CloudPhaseG", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mCloudProperty.CloudPhaseG2, "CloudPhaseG2", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mCloudProperty.CloudPhaseMixFactor, "CloudPhaseMixFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mCloudProperty.MsScattFactor, "MsScattFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mCloudProperty.MsExtinFactor, "MsExtinFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mCloudProperty.MsPhaseFactor, "MsPhaseFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

	EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN_BASE(SphericalTerrainComponentInfo)


	SphericalTerrainComponent::SphericalTerrainComponent(ActorPtr pActor, ActorCOMMoveableType type)
		: ActorComponent(pActor, type) {
		
	}
	SphericalTerrainComponent::~SphericalTerrainComponent() {
		
	}

	void SphericalTerrainComponent::init(ActorComponentInfo* info) {
		if (info) {
			SphericalTerrainComponentInfo* pInfo = static_cast<SphericalTerrainComponentInfo*>(info);
			m_sTerrainPath = pInfo->GetTerrainPath();
			m_bIsUsePhysics = pInfo->GetIsUsePhysics();
			m_MeshFlags = pInfo->GetMeshFlags();
			m_MaterialId = pInfo->GetMaterialId();
			m_FilterGroupType = pInfo->GetFilterGroupType();
			m_CanInteractFilterGroupType = pInfo->GetCanInteractFilterGroupType();

			m_bEnableLowLODCloud = pInfo->GetEnableLowLODCloud();
			m_fCloudUVScale = pInfo->GetCloudUVScale();
			m_fCloudFlowSpeed = pInfo->GetCloudFlowSpeed();
			m_fCloudHDRFactor = pInfo->GetCloudHDRFactor();
			m_fCloudMetallic = pInfo->GetCloudMetallic();
			m_fCloudRoughness = pInfo->GetCloudRoughness();
			m_fCloudAmbientFactor = pInfo->GetCloudAmbientFactor();
			m_fCloudNoiseOffsetScale = pInfo->GetCloudNoiseOffsetScale();
			m_strCloudTexture = pInfo->GetCloudTexture();
			mCloudProperty = pInfo->mCloudProperty;
		}
		SetNotifiedUpdate(true);
		RegisterPhysicsTerrain();
	}
	void SphericalTerrainComponent::unInit() {
		ClearPlanetRoadCache();

		SetNotifiedUpdate(false);
		UnregisterPhysicsTerrain();

		for (auto& modifier : m_ModifierMap) {
			SAFE_DELETE(modifier.second);
		}
		m_ModifierMap.clear();

	}
	void SphericalTerrainComponent::replay(ActorComponentInfo* info) {
		if (info) {
			SphericalTerrainComponentInfo* pInfo = static_cast<SphericalTerrainComponentInfo*>(info);
			SetTerrainPath(pInfo->GetTerrainPath());
			SetIsUsePhysics(pInfo->GetIsUsePhysics());
			SetMeshFlags(pInfo->GetMeshFlags());
			SetMaterialId(pInfo->GetMaterialId());
			SetFilterGroupType(pInfo->GetFilterGroupType());
			SetCanInteractFilterGroupType(pInfo->GetCanInteractFilterGroupType());
		}
	}
	void SphericalTerrainComponent::copyTo(const ActorComponentPtr& actCom) {
		if (actCom == nullptr) return;
		SphericalTerrainComponent* pComp = static_cast<SphericalTerrainComponent*>(actCom.getPointer());
		pComp->SetTerrainPath(GetTerrainPath());
		pComp->SetIsUsePhysics(GetIsUsePhysics());
		pComp->SetMeshFlags(GetMeshFlags());
		pComp->SetMaterialId(GetMaterialId());
		pComp->SetFilterGroupType(GetFilterGroupType());
		pComp->SetCanInteractFilterGroupType(GetCanInteractFilterGroupType());
		pComp->SetRenderOffset(GetRenderOffset());
	}
	void SphericalTerrainComponent::connectCom() {
		CreateTerrain();
		UpdateTerrainAtmos();
	}

	void SphericalTerrainComponent::disconnectCom() {
		DestroyTerrain();

		// 如果有关联的星球进出组件，设置对方的星球组件为空
		if (m_AtmosCom != nullptr) {
			if (m_AtmosCom->mSphericalTerrainComponent == this)
				m_AtmosCom->mSphericalTerrainComponent = nullptr;
		}
	}
	void SphericalTerrainComponent::NotifiChildImp(int changeMask) {
		if (m_pSphericalTerrain) {
			m_pSphericalTerrain->setWorldTrans(GetTerrainWorldTrans(), getWorldScale());
			UpdateLoadData();
		}
	}
	void SphericalTerrainComponent::visibleStateChanged(bool activeState) {
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setVisible(getRealVisibleState());
		}
		ChangePhysics();
	}

	class PlanetRoadTool : public PavementComponent::PavementFitPlanetTerrain
	{
	public:
		PlanetRoadTool(SphericalTerrain* planet) : PavementFitPlanetTerrain(nullptr) { mPlanet = planet; }
		~PlanetRoadTool() override = default;

		SphericalTerrain* getTerrainPtr() override { return mPlanet; }

		SphericalTerrain* mPlanet = nullptr;
	};

	void SphericalTerrainComponent::OnCreateFinish() {
		CreatePhysics();
		ApplyModifiers();

		m_pSphericalTerrain->mNewPhysicsObjectFun = std::bind<void*>(&SphericalTerrainComponent::_NewPhysicsObjectFunction, this, std::placeholders::_1);
		m_pSphericalTerrain->mDeletePhysicsObjectFun = std::bind<bool>(&SphericalTerrainComponent::_DeletePhysicsObjectFunction, this, std::placeholders::_1);
		updateVolumetricCloudParams();
        if (m_pSphericalTerrain)
        {
            //TODO(yanghang):Temp Disable this effecot.
            // Enable this when volumetric is ready!
            //m_pSphericalTerrain->initLowLODCloud();
            //NOTE:Default enable Cloud effect.
			//UpdateLowLodCloudParameter();
            setEnableLowLODCloud(true);
            // if (BindRoadDirtyWay)
            {
	            m_FitTerrain = std::make_unique<PlanetRoadTool>(m_pSphericalTerrain);
				m_pSphericalTerrain->BuildPlanetRoad();
				if (m_pSphericalTerrain->getPlanetRoad())
				{
					if (planetroadListener == nullptr)
						planetroadListener = new RoadListener(this);
					m_pSphericalTerrain->getPlanetRoad()->addListener(planetroadListener);
					//getActorManager()->getWorldManager()->addLoadObject(m_pSphericalTerrain->getPlanetRoad()->mTicketID);
				}
				//CreatePlanetRoad();
            }
			//RoadManager::instance()->updatePlanetBuildingMaskCache(m_pSphericalTerrain, true);
        }
            
            /*

            //TODO(yanghang):Test remove.
            m_pSphericalTerrain->initWarfog();
            enableRegionVisualize(true);

            setDisplayCorseOrFineRegion(false);
            setHighlightFineRegionID(21);
            setHighlightCorseRegionID(10);
            //resetHighlightCorseRegionID();
            //resetHighlightFineRegionID();
            
            setRegionGlowColor(ColorValue(0.9f, 0.9f, 0.9f, 1.0f));
            
            setRegionHighlightColor(ColorValue::White);
            setHighlightRegionAlpha(0.5f);
            
            setRegionHighlightWarfogColor(ColorValue(0.5f, 0.5f, 0.5f, 1.0f));
            
            setRegionWarfogColor(ColorValue::Green);

            setOutlineWidth(0.15f);
            setOutlineSmoothRange(0.05f);

            setCommonRegionAlpha(0.5f);
            setRegionFillColor(ColorValue::Green);

            setWarfogMaskUVScale(100.0f);
            setWarfogMaskScale(0.3f);

            setAdditionOutLineColor(ColorValue::White);
            setAdditionOutLineAlpha(1.0f);
            setAdditionOutLineWidth(0.2f);
            setAdditionOutLineDistance(0.1f);
            
            //NOTE:Cloud test.
            m_pSphericalTerrain->initLowLODCloud();
            setEnableLowLODCloud(true);
            setCloudUVScale(0.7f);
            setCloudFlowSpeed(0.8f);
            setCloudHDRFactor(1.0f);
            //setCloudMetallic(1.0f);
            //setCloudRoughness(0.0f);
            setCloudAmbientFactor(0.05f);
            setCloudNoiseOffsetScale(0.05f);
            */
	}

	void SphericalTerrainComponent::OnDestroy()
	{
		ClearPlanetRoadCache();

		SetIsUsePhysics(false);
		m_pSphericalTerrain = nullptr;
		// 如果有关联的星球进出组件，设置对方的星球组件为空
		if (m_AtmosCom != nullptr) {
			if (m_AtmosCom->mSphericalTerrainComponent == this)
				m_AtmosCom->mSphericalTerrainComponent = nullptr;
		}

		// if (BindRoadDirtyWay)
		{
			m_FitTerrain = std::make_unique<PlanetRoadTool>(nullptr);
		}
	}

	AxisAlignedBox SphericalTerrainComponent::getLoadBoundingBox() {
		if (m_pSphericalTerrain == nullptr || m_pSphericalTerrain->m_bounds.empty())
		{
			return ActorComponent::getLoadBoundingBox();
		}
		AxisAlignedBox loadBox = m_pSphericalTerrain->m_bounds.front();
		loadBox.transform(m_pSphericalTerrain->m_worldMat);
		loadBox = loadBox.expandBy(m_pSphericalTerrain->getMaxRealRadius() * 64);
		return loadBox;
	}

	void SphericalTerrainComponent::SetTerrainPath(const String& name) {
		if (m_sTerrainPath == name) {
			return;
		}
		m_sTerrainPath = name;
		ChangeTerrain();
	}
	void SphericalTerrainComponent::SetRenderOffset(const DTransform& trans) {
		m_RenderOffset = trans;
		if (m_pSphericalTerrain) {
			m_pSphericalTerrain->setWorldTrans(GetTerrainWorldTrans(), getWorldScale());
		}
	}

	bool SphericalTerrainComponent::IsCreateFinish() const {
		return m_pSphericalTerrain != nullptr && m_pSphericalTerrain->isCreateFinish();
	}

	float SphericalTerrainComponent::getMaxRealRadius() const {
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return std::numeric_limits<float>::quiet_NaN();
		}
		return m_pSphericalTerrain->getMaxRealRadius();
	}

	float SphericalTerrainComponent::getRadius() const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return std::numeric_limits<float>::quiet_NaN();
		}
		return m_pSphericalTerrain->getRadius();
	}

	int SphericalTerrainComponent::getRegionID(const DVector3& worldPos) const {
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return -1;
		}
		if (!m_pSphericalTerrain->m_terrainData->sphericalVoronoiRegion.Loaded)
		{
			LogManager::instance()->logMessage("SphericalTerrainComponent::getRegionID : SphericalVoronoiRegion isn't loaded successfully", LML_CRITICAL);
			return -1;
		}
		return m_pSphericalTerrain->computeRegionInformation(worldPos);
	}
	const RegionDefine* SphericalTerrainComponent::getRegionDefine(int id) {
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return nullptr;
		}
		if ((id < 0 || id >= m_pSphericalTerrain->m_terrainData->region.biomeArray.size()))
		{
			LogManager::instance()->logMessage("SphericalTerrainComponent::getRegionDefine : Region define ID " + std::to_string(id) + " invalid.", LML_CRITICAL);
			return nullptr;
		}
		return &(m_pSphericalTerrain->m_terrainData->region.biomeArray[id]);

	}

	NaiveWrapper<SphericalVoronoiRegionDefineBase> SphericalTerrainComponent::getSphericalVoronoiRegionDefineBase(int id)
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return NaiveWrapper<SphericalVoronoiRegionDefineBase>();
		}
		if (!m_pSphericalTerrain->m_terrainData->sphericalVoronoiRegion.Loaded)
		{
			LogManager::instance()->logMessage("SphericalTerrainComponent::getSphericalVoronoiRegionDefineBase : SphericalVoronoiRegion isn't loaded successfully", LML_CRITICAL);
			return NaiveWrapper<SphericalVoronoiRegionDefineBase>();
		}
		if ((id < 0 || id >= m_pSphericalTerrain->m_terrainData->sphericalVoronoiRegion.defineBaseArray.size()))
		{
			LogManager::instance()->logMessage("SphericalTerrainComponent::getSphericalVoronoiRegionDefineBase : SphericalVoronoiCoarseRegion define ID " + std::to_string(id) + " invalid.", LML_CRITICAL);
			return NaiveWrapper<SphericalVoronoiRegionDefineBase>();
		}
		return NaiveWrapper<SphericalVoronoiRegionDefineBase>(m_pSphericalTerrain->m_terrainData->sphericalVoronoiRegion.defineBaseArray[id]);

	}

	NaiveWrapper<SphericalVoronoiRegionDefine> SphericalTerrainComponent::getSphericalVoronoiRegionDefine(int id)
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return NaiveWrapper<SphericalVoronoiRegionDefine>();
		}
		if (!m_pSphericalTerrain->m_terrainData->sphericalVoronoiRegion.Loaded)
		{
			LogManager::instance()->logMessage("SphericalTerrainComponent::getSphericalVoronoiRegionDefine : SphericalVoronoiRegion isn't loaded successfully", LML_CRITICAL);
			return NaiveWrapper<SphericalVoronoiRegionDefine>();
		}
		if ((id < 0 || id >= m_pSphericalTerrain->m_terrainData->sphericalVoronoiRegion.defineArray.size()))
		{
			LogManager::instance()->logMessage("SphericalTerrainComponent::getSphericalVoronoiRegionDefine : SphericalVoronoiRegion define ID " + std::to_string(id) + " invalid.", LML_CRITICAL);
			return NaiveWrapper<SphericalVoronoiRegionDefine>();
		}

		return NaiveWrapper<SphericalVoronoiRegionDefine>(m_pSphericalTerrain->m_terrainData->sphericalVoronoiRegion.defineArray[id]);
	}

	std::vector<int> SphericalTerrainComponent::GetCoarseLayerAdjacentRegions(int id) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return std::vector<int>();
		}

		return m_pSphericalTerrain->GetCoarseLayerAdjacentRegions(id);
	}

	std::vector<int> SphericalTerrainComponent::GetFineLayerAdjacentRegions(int id) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return std::vector<int>();
		}

		return m_pSphericalTerrain->GetFineLayerAdjacentRegions(id);
	}

	std::vector<Vector3> SphericalTerrainComponent::GetCoarseLayerCenters(bool ToWorldSpace) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return std::vector<Vector3>();
		}
		return m_pSphericalTerrain->GetCoarseLayerCenters(ToWorldSpace);
	}

	std::vector<Vector3> SphericalTerrainComponent::GetFineLayerCenters(bool ToWorldSpace) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return std::vector<Vector3>();
		}
		return m_pSphericalTerrain->GetFineLayerCenters(ToWorldSpace);
	}

	float SphericalTerrainComponent::getSurfaceArea(int id)
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return std::numeric_limits<float>::quiet_NaN();
		}
		return m_pSphericalTerrain->getSurfaceArea(id);
	}

	float SphericalTerrainComponent::getCoarseSurfaceArea(unsigned int id)
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return std::numeric_limits<float>::quiet_NaN();
		}
		return m_pSphericalTerrain->getCoarseSurfaceArea(id);
	}


	int SphericalTerrainComponent::getSurfaceTypeWs(const DVector3& worldPos) const {
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return -1;
		}
		int type = m_pSphericalTerrain->getSurfaceTypeWs(worldPos);
		if (type != -1)
		{
			if (m_pSphericalTerrain->m_terrainData->composition.usedBiomeTemplateList.size() <= static_cast<size_t>(type))
			{
				LogManager::instance()->logMessage("-error-\t" + m_pSphericalTerrain->m_terrainData->composition.filePath + " doesn't match matIndexMap.", LML_CRITICAL);
				return -1;
			}
			type = static_cast<int>(m_pSphericalTerrain->m_terrainData->composition.usedBiomeTemplateList[type].biomeID);
		}
		return type;
	}
	int SphericalTerrainComponent::getSurfaceTypeLs(const Vector3& localPos) const {
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return -1;
		}
		int type = m_pSphericalTerrain->getSurfaceTypeLs(localPos.normalisedCopy());
		if (type != -1)
		{
			if (m_pSphericalTerrain->m_terrainData->composition.usedBiomeTemplateList.size() <= static_cast<size_t>(type))
			{
				LogManager::instance()->logMessage("-error-\t" + m_pSphericalTerrain->m_terrainData->composition.filePath + " doesn't match matIndexMap.", LML_CRITICAL);
				return -1;
			}
			type = static_cast<int>(m_pSphericalTerrain->m_terrainData->composition.usedBiomeTemplateList[type].biomeID);
		}
		return type;
	}
	uint32 SphericalTerrainComponent::getPhySurfaceType(const DVector3& worldPos) const {
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return -1;
		}
		return m_pSphericalTerrain->getPhySurfaceType(worldPos);
	}
	Vector3 SphericalTerrainComponent::getUpVectorWs(const DVector3& worldPos) const {
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return VEC3_NAN;
		}
		return m_pSphericalTerrain->getUpVectorWs(worldPos);
	}
	Vector3 SphericalTerrainComponent::getGravityWs(const DVector3& worldPos) const {
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return VEC3_NAN;
		}
		return m_pSphericalTerrain->getGravityWs(worldPos);
	}
	Vector3 SphericalTerrainComponent::getGravityLs(const Vector3& localPos) const {
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return VEC3_NAN;
		}
		return m_pSphericalTerrain->getGravityLs(localPos);
	}
	int SphericalTerrainComponent::getSphereGeometryType() const {
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return -1;
		}
		if (m_pSphericalTerrain->m_pTerrainGenerator == nullptr) {
			return -1;
		}
		else {
			return (int)m_pSphericalTerrain->m_pTerrainGenerator->getType();
		}
	}
	DVector3 SphericalTerrainComponent::getSurfaceWs(const DVector3& worldPos, Vector3* faceNormal) const {
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return VEC3_NAN;
		}
		return m_pSphericalTerrain->getSurfaceWs(worldPos, faceNormal);
	}
	Vector3 SphericalTerrainComponent::getSurfaceLs(const Vector3& localPos, Vector3* faceNormal) const {
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return VEC3_NAN;
		}
		return m_pSphericalTerrain->getSurfaceLs(localPos.normalisedCopy(), faceNormal);
	}

	DVector3 SphericalTerrainComponent::getSurfaceFinestWs(const DVector3& worldPos, Vector3* faceNormal) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return VEC3_NAN;
		}
		return m_pSphericalTerrain->getSurfaceFinestWs(worldPos, faceNormal);
	}

	Vector3 SphericalTerrainComponent::getSurfaceFinestLs(const Vector3& localPos, Vector3* faceNormal) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return VEC3_NAN;
		}
		return m_pSphericalTerrain->getSurfaceFinestLs(localPos.normalisedCopy(), faceNormal);
	}

	int SphericalTerrainComponent::getSurfaceMipWs(const DVector3& worldPos) const {
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return -1;
		}
		return m_pSphericalTerrain->getSurfaceMipWs(worldPos);
	}
	int SphericalTerrainComponent::getSurfaceMipLs(const Vector3& localPos) const {
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return -1;
		}
		return m_pSphericalTerrain->getSurfaceMipLs(localPos.normalisedCopy());
	}

	DVector3 SphericalTerrainComponent::getOceanSurface(const DVector3& worldPos) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return VEC3_NAN;
		}
		return m_pSphericalTerrain->getOceanSurface(worldPos);
	}

	DVector3 SphericalTerrainComponent::getOceanWaveSurface(const DVector3& worldPos) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return VEC3_NAN;
		}
		return m_pSphericalTerrain->getOceanWaveSurface(worldPos);
	}

	bool SphericalTerrainComponent::isUnderOcean(const DVector3& worldPos, double* depth) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return false;
		}
		return m_pSphericalTerrain->isUnderOcean(worldPos, depth);
	}

	bool SphericalTerrainComponent::isUnderOceanWave(const DVector3& worldPos, double* depth) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return false;
		}
		return m_pSphericalTerrain->isUnderOceanWave(worldPos, depth);
	}

	float SphericalTerrainComponent::getBoarderValue(const DVector3& worldSurfacePos) const
	{
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return false;
		}
		return m_pSphericalTerrain->getBoarderValue(worldSurfacePos);
	}

	float SphericalTerrainComponent::getMinNoiseScale() const {
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return 1.0f;
		}
		return m_pSphericalTerrain->m_noiseElevationMin + 1.0f;
	}
	float SphericalTerrainComponent::getMaxNoiseScale() const {
		if (!IsCreateFinish())
		{
			LogManager::instance()->logMessage("-error-   " + m_sTerrainPath + " creation isn't finished. Check SphericalTerrainComponent::IsCreateFinish().", LML_CRITICAL);
			return 1.0f;
		}
		return m_pSphericalTerrain->m_noiseElevationMax + 1.0f;
	}
	bool SphericalTerrainComponent::hasModifier(uint64 id) const {
		return m_ModifierMap.contains(id);
	}
	const SphericalTerrainComponent::Modifier* SphericalTerrainComponent::getModifier(uint64 id) const {
		auto modifierIter = m_ModifierMap.find(id);
		if (modifierIter == m_ModifierMap.end()) return nullptr;
		return modifierIter->second;
	}
	void SphericalTerrainComponent::addModifier(uint64 id, const Modifier& modifier) {
		removeModifier(id);
		Modifier* pNewModifier = nullptr;
		switch (modifier.type) {
			case Modifier::eRECTANGLE: {
				const RectangleModifier* pModifier = static_cast<const RectangleModifier*>(&modifier);
				pNewModifier = new RectangleModifier(*pModifier);
				break;
			}
			case Modifier::eCIRCLE: {
				const CircleModifier* pModifier = static_cast<const CircleModifier*>(&modifier);
				pNewModifier = new CircleModifier(*pModifier);
				break;
			}
			default: {
				break;
			}
		}
		if (pNewModifier == nullptr) return;
		m_ModifierMap[id] = pNewModifier;
		if (!IsCreateFinish()) return;
		ApplyModifier(id, pNewModifier);
		std::unordered_map<int, ActorComponentPtr> phyMap;
		FindModifierPhysics(id, phyMap);
		for (auto& iter : phyMap) {
			MeshColliderComponent* pCollider = (MeshColliderComponent*)iter.second.getPointer();
			pCollider->SetMeshGenFun(std::bind<void>(&SphericalTerrainComponent::GenerateVBIBFunction, iter.first, this, std::placeholders::_1, std::placeholders::_2));
		}
	}
	void SphericalTerrainComponent::removeModifier(uint64 id) {
		auto modifierIter = m_ModifierMap.find(id);
		if (modifierIter == m_ModifierMap.end()) return;
		SAFE_DELETE(modifierIter->second);
		m_ModifierMap.erase(modifierIter);
		if (!IsCreateFinish()) return;
		std::unordered_map<int, ActorComponentPtr> phyMap;
		FindModifierPhysics(id, phyMap);
		m_pSphericalTerrain->removeModifier(id);
		for (auto& iter : phyMap) {
			MeshColliderComponent* pCollider = (MeshColliderComponent*)iter.second.getPointer();
			pCollider->SetMeshGenFun(std::bind<void>(&SphericalTerrainComponent::GenerateVBIBFunction, iter.first, this, std::placeholders::_1, std::placeholders::_2));
		}
	}
	void SphericalTerrainComponent::FindModifierPhysics(uint64 id, std::unordered_map<int, ActorComponentPtr>& phyMap) {
		if (m_ColliderRootPtr == nullptr) return;
		for (auto& iter : m_SyncColliderMap) {
			if (m_pSphericalTerrain->testModifierIntersect(id, iter.first)) {
				phyMap.insert(std::make_pair(iter.first, iter.second));
			}
		}
		for (auto& iter : m_AsyncColliderMap) {
			if (m_pSphericalTerrain->testModifierIntersect(id, iter.first)) {
				phyMap.insert(std::make_pair(iter.first, iter.second));
			}
		}
	}

	void SphericalTerrainComponent::ApplyModifiers() {
		for (auto& modifierIter : m_ModifierMap) {
			ApplyModifier(modifierIter.first, modifierIter.second);
		}
	}
	void SphericalTerrainComponent::ApplyModifier(uint64 id, const Modifier* modifier) {
		DVector3 worldPosition = convertLocalToWorldPosition(modifier->position);
		switch (modifier->type) {
		case Modifier::eRECTANGLE: {
			const RectangleModifier* pModifier = static_cast<const RectangleModifier*>(modifier);
			m_pSphericalTerrain->addModifier(id, pModifier->width, pModifier->height, worldPosition, pModifier->yaw / 180.0f * ECHO_PI);
			break;
		}
		case Modifier::eCIRCLE: {
			const CircleModifier* pModifier = static_cast<const CircleModifier*>(modifier);
			m_pSphericalTerrain->addModifier(id, pModifier->radius, worldPosition);
			break;
		}
		default: {
			break;
		}
		}
	}
	//void SphericalTerrainComponent::UpdateLowLodCloudParameter()
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
	DTransform SphericalTerrainComponent::GetTerrainWorldTrans() {
		return DTransform(convertLocalToWorldPosition(m_RenderOffset.translation), convertLocalToWorldRotation(m_RenderOffset.rotation));
	}

	void SphericalTerrainComponent::CreateTerrain() {
		if (m_pSphericalTerrain != nullptr) return;
		m_pSphericalTerrain = getSceneManager()->getPlanetManager()->createSphericalTerrain(m_sTerrainPath, false, false, false, GetTerrainWorldTrans(), getWorldScale());
		if (m_pSphericalTerrain) {
			if (m_pSphericalTerrain->isWaitCreateFinish()) {
				m_pSphericalTerrain->addLoadListener((SphericalTerrain::LoadListener*)this);
			}
			else {
				OnCreateFinish();
			}
		}
	}
	void SphericalTerrainComponent::DestroyTerrain() {
		if (m_pSphericalTerrain == nullptr) return;
		ClearPlanetRoadCache();
		m_pSphericalTerrain->removeLoadListener((SphericalTerrain::LoadListener*)this);
		
		DestroyPhysics();
		{
			getSceneManager()->getPlanetManager()->destroySphericalTerrain(m_pSphericalTerrain);
			m_pSphericalTerrain = nullptr;
		}
	}
	void SphericalTerrainComponent::ChangeTerrain() {
		DestroyTerrain();
		CreateTerrain();
		UpdateTerrainAtmos();
	}

	ActorComponent* SphericalTerrainComponent::getThisCompPtr() {
		return this;
	}
	bool SphericalTerrainComponent::getIsNearby(const Vector3& worldPos) {
		return getWorldPosition().squaredDistance(worldPos) < m_NearEarthHeightSqr;
	}
	bool SphericalTerrainComponent::IsCanPhysics() {
		return m_pSphericalTerrain != nullptr && m_pSphericalTerrain->isCreateFinish();
	}
	void SphericalTerrainComponent::AddMeshColliderImpl(MeshColliderComponent* pCollider, int index) {
		pCollider->m_pGetSurfaceTypeFun = std::bind<uint32>(&SphericalTerrainComponent::GetSurfaceTypeFunction, this, pCollider, std::placeholders::_1, std::placeholders::_2);
		pCollider->SetMeshGenFun(std::bind<void>(&SphericalTerrainComponent::GenerateVBIBFunction, index, this, std::placeholders::_1, std::placeholders::_2));
		pCollider->setPosition(m_pSphericalTerrain->getNodeOriginLocal(index) * m_pSphericalTerrain->m_radius);
	}
	void SphericalTerrainComponent::getAllBlockIndex(std::vector<int>& leavesIndexs) {
		leavesIndexs = m_pSphericalTerrain->getLeavesIndex();
	}
	void SphericalTerrainComponent::getBlockIndex(const AxisAlignedBox& localBound, std::vector<int>& indexs) {
		AxisAlignedBox bound = localBound;
		Vector3 scale(1.0f / m_pSphericalTerrain->m_radius);
		bound.scale(scale);
		indexs = m_pSphericalTerrain->getLeavesInLs(bound);
	}
	AxisAlignedBox SphericalTerrainComponent::getBlockBound(int index) {
		AxisAlignedBox bound = m_pSphericalTerrain->getIndexBound(index);
		Vector3 scale(m_pSphericalTerrain->m_radius);
		bound.scale(scale);
		return bound;
	}

	void SphericalTerrainComponent::UpdateLoadData() {
		GeneratedTerrainPhysics_Spherical::UpdateLoadData();
		float nearEarthHeight = 10000.0f;
		if (m_pSphericalTerrain) {
			nearEarthHeight = m_pSphericalTerrain->getMaxRealRadius() + 600.0f;
		}
		m_NearEarthHeightSqr = Math::Floor(nearEarthHeight * nearEarthHeight);
	}

	void* SphericalTerrainComponent::_NewPhysicsObjectFunction(const SphericalTerrain::PhysicsObjectDesc& desc) {
		EchoPhysicsManager* pPhysicsManager = EchoPhysicsSystem::instance()->GetPhysicsManager(this);
		if (pPhysicsManager == nullptr) return nullptr;
		PhysXWorldManager* pPhysXWorldManager = pPhysicsManager->GetPhysXWorldManager();
		if (pPhysXWorldManager == nullptr) return nullptr;
		PhysXTriangleMeshResourcePtr meshPtr;
		auto iter = mPhysicsObjectResourceCache.find(desc.meshphy);
		if (iter == mPhysicsObjectResourceCache.end()) {
			meshPtr = PhysXTriangleMeshResourceManager::instance()->load(desc.meshphy);
			mPhysicsObjectResourceCache[desc.meshphy] = meshPtr;
		}
		else {
			meshPtr = iter->second;
		}
		return pPhysXWorldManager->AddStatic_TriangleMesh(desc.transforms, meshPtr);
	}
	bool SphericalTerrainComponent::_DeletePhysicsObjectFunction(void* ptr) {
		EchoPhysicsManager* pPhysicsManager = EchoPhysicsSystem::instance()->GetPhysicsManager(this);
		if (pPhysicsManager == nullptr) return false;
		PhysXWorldManager* pPhysXWorldManager = pPhysicsManager->GetPhysXWorldManager();
		if (pPhysXWorldManager == nullptr) return false;
		return pPhysXWorldManager->RemoveStatic(ptr);
	}

	void SphericalTerrainComponent::GenerateVBIBFunction(int index, SphericalTerrainComponent* _thisPtr, std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec) {
		if (_thisPtr != nullptr)
		{
			if (_thisPtr->m_pSphericalTerrain == nullptr) return;
			vbVec = _thisPtr->m_pSphericalTerrain->generateVertexBufferPhysX(index);
			ibVec = _thisPtr->m_pSphericalTerrain->generateIndexBufferPhysX();
			float radius = _thisPtr->m_pSphericalTerrain->m_radius;
			for (Vector3& vb : vbVec) {
				vb *= radius;
			}
		}
	}

	uint32 SphericalTerrainComponent::GetSurfaceTypeFunction(SphericalTerrainComponent* _thisPtr, MeshColliderComponent* _pComp, const DVector3& position, bool bWorld) {
		if (_thisPtr == nullptr || _thisPtr->m_pSphericalTerrain == nullptr) return 0;
		if (bWorld) {
			return (uint32)_thisPtr->m_pSphericalTerrain->getPhySurfaceType(position);
		}
		else {
			return (uint32)_thisPtr->m_pSphericalTerrain->getPhySurfaceType(_pComp->convertLocalToWorldPosition(position));
		}
	}

	const String SphericalTerrainComponent::gServerRelativePath = "biome_terrain/";
	bool SphericalTerrainComponent::ExportForServer(const set<String>::type& terrainPaths, int exportVersion, const String& assetPath, long long timeStamp) {
		bool res = true;
		set<String>::type exportFileSet;
		for (const auto& terrainPath : terrainPaths) {
			ProceduralSphere* sphere = new ProceduralSphere();
			if (sphere->init(terrainPath)) {
				res = ExportForServer(sphere, exportVersion, assetPath, timeStamp, exportFileSet) && res;
			}
			else {
				res = false;
			}
			SAFE_DELETE(sphere);
		}
		return res;
	}
	bool SphericalTerrainComponent::ExportForServer(const String& terrainPath, int exportVersion, const String& assetPath, long long timeStamp) {
		set<String>::type exportFileSet;
		ProceduralSphere* sphere = new ProceduralSphere();
		bool res = false;
		if (sphere->init(terrainPath)) {
			res = ExportForServer(sphere, exportVersion, assetPath, timeStamp, exportFileSet);
		}
		SAFE_DELETE(sphere);
		return res;
	}

	void DealFilePath(String& filePath) {
		auto sufPox = filePath.find_last_of(".");
		if (sufPox != filePath.npos) {
			filePath = filePath.substr(0, sufPox);
		}
		std::replace(filePath.begin(), filePath.end(), '\\', '/');
		std::replace(filePath.begin(), filePath.end(), '/', '_');
	};

	bool SphericalTerrainComponent::ExportForServer(ProceduralSphere* pTerrain, int exportVersion, const String& assetPath, long long timeStamp, set<String>::type& exportFileSet) {
		if (pTerrain->m_terrainData.isNull()) return false;

		bool bNewRegion                         = true;
		const std::array<Bitmap, 6>* regionMaps = nullptr;
		std::vector<Vector3> centers;
		if (pTerrain->m_terrainData->sphericalVoronoiRegion.Loaded
			&& !pTerrain->m_terrainData->sphericalVoronoiRegion.mData.isNull()
			&& pTerrain->m_terrainData->sphericalVoronoiRegion.mData->mData.mbDataMaps
			&& pTerrain->m_terrainData->sphericalVoronoiRegion.mData->mData.mbPoints)
		{
			regionMaps = &pTerrain->m_terrainData->sphericalVoronoiRegion.mData->mData.mDataMaps;
			centers    = pTerrain->m_terrainData->sphericalVoronoiRegion.mData->mData.mPoints;
		}
		else if (pTerrain->m_terrainData->region.Loaded)
		{
			regionMaps = &(pTerrain->m_terrainData->region.RegionMaps);
			bNewRegion = false;
		}
		int width = 256, height = 256;
		if (regionMaps)
		{
			width  = (*regionMaps)[0].width;
			height = (*regionMaps)[0].height;
			switch (pTerrain->m_topology)
			{
			case PTS_Sphere:
			{
				for (int face = 0; face < 6; ++face)
				{
					if ((*regionMaps)[face].height != width || (*regionMaps)[face].width != width || (*regionMaps)[face].pitch != width || (*regionMaps)[face].bytesPerPixel != 1)
					{
						regionMaps = nullptr;
						centers.clear();
						break;
					}
				}
			}
			break;
			case PTS_Torus:
			{
				if ((*regionMaps)[0].bytesPerPixel != 1)
				{
					regionMaps = nullptr;
					centers.clear();
				}
			}
			break;
			}
		}

		String terrainPath = pTerrain->m_terrainData->getName().toString();

		String terrainName = terrainPath;
		DealFilePath(terrainName);
		String terrainfile = gServerRelativePath + terrainName + ".terrain";

		String offlinemapName = terrainName;
		if (pTerrain->m_terrainData->sphericalVoronoiRegion.Loaded)
		{
			offlinemapName = pTerrain->m_terrainData->sphericalVoronoiRegion.prefix;
			DealFilePath(offlinemapName);
		}
		String offlinemapfile = gServerRelativePath + offlinemapName + ".offlinemap";
		String centersfile    = gServerRelativePath + offlinemapName + ".centers";
		if (!regionMaps)
		{
			offlinemapfile = "";
			centersfile    = "";
		}

		String matIdxMapFile;
		if (!pTerrain->m_terrainData->matIndexMaps.empty())
		{
			matIdxMapFile = pTerrain->m_terrainData->matIndexMaps[0];
			DealFilePath(matIdxMapFile);
		}
		matIdxMapFile = gServerRelativePath + matIdxMapFile + ".bin";

		cJSON* root = cJSON_CreateObject();

		WriteJSNode(root, "version", 1);
		WriteJSNode(root, "timeStamp", (uint32)(timeStamp));
		WriteJSNode(root, "name", terrainPath);
		WriteJSNode(root, "scale", pTerrain->m_radius);

		if (!pTerrain->m_terrainData->geometry.bounds.empty())
		{
			AxisAlignedBox bound = pTerrain->m_terrainData->geometry.bounds.front();
			Vector3 scale(pTerrain->m_radius);
			bound.scale(scale);
			WriteJSNode(root, "bound", bound);
		}

		int sliceSizeInTile = 0;
		int slices[2] = { 0, 0 };
		// geometry node
		{
			const auto& geometry = pTerrain->m_terrainData->geometry;

			cJSON* geometryNode = cJSON_CreateObject();

			cJSON* noiseNode = cJSON_CreateObject();
			cJSON_AddItemToObject(geometryNode, "3d noise param", noiseNode);
			SphericalTerrainResource::save3DNoiseParameters(noiseNode, const_cast<Noise3DData&>(geometry.noise));

			WriteJSNode(geometryNode, "topology", static_cast<int>(geometry.topology));
			const int maxDepth = geometry.quadtreeMaxDepth;
			sliceSizeInTile = 1 << maxDepth;
			switch (geometry.topology)
			{
			case PTS_Sphere:
			{
				const auto& [radius, elevationRatio, gempType] = geometry.sphere;
				WriteJSNode(geometryNode, "radius", radius);
				WriteJSNode(geometryNode, "relativeMinorRadius", 0.0f);
				WriteJSNode(geometryNode, "maxSegmentsX", sliceSizeInTile);
				WriteJSNode(geometryNode, "maxSegmentsY", sliceSizeInTile);
				WriteJSNode(geometryNode, "elevationRatio", elevationRatio);
				WriteJSNode(geometryNode, "gempType", gempType);
			}
			break;
			case PTS_Torus:
			{
				const auto& [radius, elevationRatio, relativeMinorRadius, maxSegments] = geometry.torus;
				WriteJSNode(geometryNode, "radius", radius);
				WriteJSNode(geometryNode, "relativeMinorRadius", relativeMinorRadius);
				slices[0] = maxSegments[0];
				slices[1] = maxSegments[1];
				WriteJSNode(geometryNode, "maxSegmentsX", maxSegments[0] * sliceSizeInTile);
				WriteJSNode(geometryNode, "maxSegmentsY", maxSegments[1] * sliceSizeInTile);
				WriteJSNode(geometryNode, "elevationRatio", elevationRatio);
			}
			break;
			}

			WriteJSNode(geometryNode, "minRadius", pTerrain->getRadius() * (pTerrain->m_terrainData->geometry.elevationMin + 1.f));
			WriteJSNode(geometryNode, "maxRadius", pTerrain->getRadius() * (pTerrain->m_terrainData->geometry.elevationMax + 1.f));

			auto instances = geometry.StampTerrainInstances;
			for (auto& instance : instances)
			{
				auto path     = instance.templateID;
				HeightMapDataPtr heightmap{};
				try
				{
					heightmap = StampTerrain3D::HeightMapDataManager::instance()->prepare(Name(path));
				}
				catch (...)
				{
					heightmap.setNull();
				}
				if (heightmap.isNull()) continue;

				const auto& hm = heightmap->mHeightMap;
				if (!hm.pixels) continue;

				DealFilePath(path);
				path += ".raw";
				path                = String(gServerRelativePath).append(path);
				instance.templateID = path;
				if (!exportFileSet.contains(path))
				{
					if (DataStreamPtr stream(Root::instance()->CreateDataStream((assetPath + path).c_str())); !stream.isNull())
					{
						stream->write(hm.pixels, static_cast<size_t>(hm.m_x) * hm.m_y * sizeof(float));
						stream->close();
						exportFileSet.insert(path);
					}
				}
			}
			HeightMapStamp3D::Save(geometryNode, instances);

			cJSON_AddItemToObject(root, "terrain geometry", geometryNode);
		}

		if (!pTerrain->m_terrainData->modifiers.filePath.empty())
		{
			std::unordered_map<int, std::vector<uint32>> culledLookup;
			std::unordered_map<int, int> tileToFinestTile;
			const auto& modifiers = pTerrain->m_terrainData->modifiers.instances;
			const auto& tileToMod = pTerrain->m_terrainData->modifiers.nodeToInstances;
			int maxDepth          = pTerrain->m_maxDepth;
			auto toFinestTileId   = [&](const ProceduralSphere::NodeInfo info)
			{
				if (pTerrain->isSphere())
				{
					return info.slice * sliceSizeInTile * sliceSizeInTile +
						info.y * sliceSizeInTile + info.x;
				}
				if (pTerrain->isTorus())
				{
					int sliceX = info.slice % slices[0];
					int sliceY = info.slice / slices[0];
					return (sliceY * sliceSizeInTile + info.y) * sliceSizeInTile * slices[0] +
						(sliceX * sliceSizeInTile + info.x);
				}
				return -1;
			};
			for (const auto& [tileId, mods] : tileToMod)
			{
				auto info = ProceduralSphere::getNodeInfo(tileId, maxDepth);
				if (info.depth != maxDepth) continue;
				auto finestTileId = toFinestTileId(info);
				if (tileToFinestTile.contains(tileId) || culledLookup.contains(finestTileId))
				{
					ECHO_EXCEPT(Exception::ERR_INVALID_STATE, "Same ID referenced twice in mod map.", "SphericalTerrainComponent::ExportForServer")
				}
				tileToFinestTile[tileId]   = finestTileId;
				culledLookup[finestTileId] = mods;
			}
			StaticModifiers finestTileMod;
			finestTileMod.filePath        = "dummy";
			finestTileMod.instances       = modifiers;
			finestTileMod.nodeToInstances = culledLookup;

			auto* modsJsonNode = SphericalTerrainResource::serializeModifiers(finestTileMod);
			cJSON_AddItemToObject(root, "static modifier", modsJsonNode);
		}

		if (const auto& ocean = pTerrain->m_terrainData->sphericalOcean; ocean.haveOcean)
		{
			WriteJSNode(root, "ocean radius", ocean.geomSettings.radius);
			WriteJSNode(root, "ocean minor radius", ocean.geomSettings.relativeMinorRadius);
		}

		WriteJSNode(root, "width", width);
		WriteJSNode(root, "height", height);
		WriteJSNode(root, "offlinemap", offlinemapfile);
		WriteJSNode(root, "centers", centersfile);
		WriteJSNode(root, "matIdxMapSize", pTerrain->m_terrainData->m_matIndexMaps[0].byteSize);
		WriteJSNode(root, "matIdxMap", matIdxMapFile);

		WriteJSNode(root, "region offset", pTerrain->m_terrainData->sphericalVoronoiRegion.offset);
		cJSON* biomeVec     = cJSON_CreateArray();
		cJSON* biomeBaseVec = cJSON_CreateArray();
		if (bNewRegion)
		{
			const auto& biomeArray = pTerrain->m_terrainData->sphericalVoronoiRegion.defineArray;
			for (int id = 0; id < biomeArray.size(); ++id)
			{
				cJSON* biomeNode = cJSON_CreateObject();
				WriteJSNode(biomeNode, "Parent", biomeArray[id].Parent);
				WriteJSNode(biomeNode, "ExtraStr", biomeArray[id].StrEx);
				WriteJSNode(biomeNode, "ExtraInt", biomeArray[id].IntEx);
				cJSON_AddItemToArray(biomeVec, biomeNode);
			}
			const auto& biomeBaseArray = pTerrain->m_terrainData->sphericalVoronoiRegion.defineBaseArray;
			for (int id = 0; id < biomeBaseArray.size(); ++id)
			{
				cJSON* biomeNode = cJSON_CreateObject();
				// WriteJSNode(biomeNode, "Parent", biomeBaseArray[id].Parent);
				WriteJSNode(biomeNode, "ExtraStr", biomeBaseArray[id].StrEx);
				WriteJSNode(biomeNode, "ExtraInt", biomeBaseArray[id].IntEx);
				cJSON_AddItemToArray(biomeBaseVec, biomeNode);
			}
		}
		else
		{
			const auto& biomeArray = pTerrain->m_terrainData->region.biomeArray;
			for (int id = 0; id < biomeArray.size(); ++id)
			{
				cJSON* biomeNode = cJSON_CreateObject();
				WriteJSNode(biomeNode, "Parent", -1);
				WriteJSNode(biomeNode, "ExtraStr", biomeArray[id].StrEx);
				WriteJSNode(biomeNode, "ExtraInt", biomeArray[id].IntEx);
				cJSON_AddItemToArray(biomeVec, biomeNode);
			}
		}
		cJSON_AddItemToObject(root, "regions", biomeVec);
		cJSON_AddItemToObject(root, "coarse regions", biomeBaseVec);


		const auto& biomes = pTerrain->m_terrainData->composition.usedBiomeTemplateList;
		std::vector<int> globalLut;
		globalLut.reserve(biomes.size());
		for (const auto& biome : biomes)
		{
			globalLut.emplace_back(biome.biomeID);
		}
		auto biomeIdArray = cJSON_CreateIntArray(globalLut.data(), static_cast<int>(globalLut.size()));
		cJSON_AddItemToObject(root, "biomeID", biomeIdArray);

		char* out = cJSON_Print(root);
		cJSON_Delete(root);

		if (out != nullptr)
		{
			String absPath = assetPath + terrainfile;
			std::replace(absPath.begin(), absPath.end(), '\\', '/');
			if (DataStreamPtr stream(Root::instance()->CreateDataStream(absPath.c_str())); !stream.isNull())
			{
				stream->writeData(out, strlen(out));
				stream->close();
				free(out);
			}
		}

		auto fileIter = exportFileSet.find(matIdxMapFile);
		if (fileIter == exportFileSet.end())
		{
			String absPath = assetPath + matIdxMapFile;
			if (DataStreamPtr stream(Root::instance()->CreateDataStream(absPath.c_str())); !stream.isNull())
			{
				for (const auto& indexMap : pTerrain->m_terrainData->m_matIndexMaps)
				{
					if (indexMap.byteSize == 0) continue;

					std::vector<uint8_t> map;
					const auto pixelCount = static_cast<size_t>(indexMap.byteSize);
					map.reserve(pixelCount);
					for (size_t i = 0; i < pixelCount; ++i)
					{
						uint8_t pix  = static_cast<uint8_t*>(indexMap.pixels)[i];
						uint8_t type = pix & SphericalTerrain::MaterialTypeMask;
						pix          = type < globalLut.size() ? type : 0u;
						map.emplace_back(pix);
					}
					stream->write(map.data(), pixelCount * sizeof(uint8_t));
				}
				stream->close();
				exportFileSet.insert(matIdxMapFile);
			}
		}

		if (regionMaps)
		{
			fileIter = exportFileSet.find(offlinemapfile);
			if (fileIter == exportFileSet.end())
			{
				String absPath = assetPath + offlinemapfile;
				if (DataStreamPtr stream(Root::instance()->CreateDataStream(absPath.c_str())); !stream.isNull())
				{
					const int z = (int)pTerrain->m_terrainData->sphericalVoronoiRegion.mData->mData.Params.z;
					for (int face = 0; face < z; ++face)
					{
						stream->write((*regionMaps)[face].pixels, (*regionMaps)[face].width * (*regionMaps)[face].height);
					}
					stream->close();
					exportFileSet.insert(offlinemapfile);
				}
			}

			fileIter = exportFileSet.find(centersfile);
			if (fileIter == exportFileSet.end())
			{
				String absPath = assetPath + centersfile;
				if (DataStreamPtr stream(Root::instance()->CreateDataStream(absPath.c_str())); !stream.isNull())
				{
					if (pTerrain->isSphere())
					{
						stream->write(centers.data(), centers.size() * sizeof(Vector3));
					}
					else if (pTerrain->isTorus())
					{
						std::vector<Vector2> tmp(centers.size());
						for (int i = 0; i < (int)centers.size(); ++i)
						{
							tmp[i] = Vector2(centers[i].x, centers[i].y);
							assert(centers[i].z == 0.0f);
						}
						stream->write(tmp.data(), centers.size() * sizeof(Vector2));
					}
					stream->close();
					exportFileSet.insert(centersfile);
				}
			}
		}
		return true;
	}

	String SphericalTerrainComponent::ExportForPlanetGraph(ProceduralSphere* pTerrain)
	{
		if (pTerrain->m_terrainData.isNull()) return {};

		String terrainPath = pTerrain->m_terrainData->getName().toString();

		cJSON* root = cJSON_CreateObject();

		WriteJSNode(root, "name", terrainPath);
		WriteJSNode(root, "scale", pTerrain->m_radius);

		int sliceSizeInTile = 0;
		int slices[2]       = { 0, 0 };
		// geometry node
		{
			const auto& geometry = pTerrain->m_terrainData->geometry;

			cJSON* geometryNode = cJSON_CreateObject();

			cJSON* noiseNode = cJSON_CreateObject();
			cJSON_AddItemToObject(geometryNode, "3d noise param", noiseNode);
			SphericalTerrainResource::save3DNoiseParameters(noiseNode, const_cast<Noise3DData&>(geometry.noise));

			WriteJSNode(geometryNode, "topology", static_cast<int>(geometry.topology));
			const int maxDepth = geometry.quadtreeMaxDepth;
			sliceSizeInTile    = 1 << maxDepth;
			switch (geometry.topology)
			{
			case PTS_Sphere:
			{
				const auto& [radius, elevationRatio, gempType] = geometry.sphere;
				WriteJSNode(geometryNode, "radius", radius);
				WriteJSNode(geometryNode, "relativeMinorRadius", 0.0f);
				WriteJSNode(geometryNode, "maxSegmentsX", sliceSizeInTile);
				WriteJSNode(geometryNode, "maxSegmentsY", sliceSizeInTile);
				WriteJSNode(geometryNode, "elevationRatio", elevationRatio);
				WriteJSNode(geometryNode, "gempType", gempType);
			}
			break;
			case PTS_Torus:
			{
				const auto& [radius, elevationRatio, relativeMinorRadius, maxSegments] = geometry.torus;
				WriteJSNode(geometryNode, "radius", radius);
				WriteJSNode(geometryNode, "relativeMinorRadius", relativeMinorRadius);
				slices[0] = maxSegments[0];
				slices[1] = maxSegments[1];
				WriteJSNode(geometryNode, "maxSegmentsX", maxSegments[0] * sliceSizeInTile);
				WriteJSNode(geometryNode, "maxSegmentsY", maxSegments[1] * sliceSizeInTile);
				WriteJSNode(geometryNode, "elevationRatio", elevationRatio);
			}
			break;
			}

			auto instances = geometry.StampTerrainInstances;
			for (auto& instance : instances)
			{
				auto path = instance.templateID;
				HeightMapDataPtr heightmap{};
				try
				{
					heightmap = StampTerrain3D::HeightMapDataManager::instance()->prepare(Name(path));
				}
				catch (...)
				{
					heightmap.setNull();
				}
				if (heightmap.isNull()) continue;

				const auto& hm = heightmap->mHeightMap;
				if (!hm.pixels) continue;
				instance.templateID = path;
			}
			HeightMapStamp3D::Save(geometryNode, instances);

			cJSON_AddItemToObject(root, "terrain geometry", geometryNode);
		}

		if (pTerrain->m_terrainData->modifiers.instances && !pTerrain->m_terrainData->modifiers.instances->empty())
		{
			std::unordered_map<int, std::vector<uint32>> culledLookup;
			std::unordered_map<int, int> tileToFinestTile;
			const auto& modifiers = pTerrain->m_terrainData->modifiers.instances;
			const auto& tileToMod = pTerrain->m_terrainData->modifiers.nodeToInstances;
			int maxDepth          = pTerrain->m_maxDepth;
			auto toFinestTileId   = [&](const ProceduralSphere::NodeInfo info)
			{
				if (pTerrain->isSphere())
				{
					return info.slice * sliceSizeInTile * sliceSizeInTile +
						info.y * sliceSizeInTile + info.x;
				}
				if (pTerrain->isTorus())
				{
					int sliceX = info.slice % slices[0];
					int sliceY = info.slice / slices[0];
					return (sliceY * sliceSizeInTile + info.y) * sliceSizeInTile * slices[0] +
						(sliceX * sliceSizeInTile + info.x);
				}
				return -1;
			};
			for (const auto& [tileId, mods] : tileToMod)
			{
				auto info = ProceduralSphere::getNodeInfo(tileId, maxDepth);
				if (info.depth != maxDepth) continue;
				auto finestTileId = toFinestTileId(info);
				if (tileToFinestTile.contains(tileId) || culledLookup.contains(finestTileId))
				{
					ECHO_EXCEPT(Exception::ERR_INVALID_STATE, "Same ID referenced twice in mod map.", "SphericalTerrainComponent::ExportForServer")
				}
				tileToFinestTile[tileId]   = finestTileId;
				culledLookup[finestTileId] = mods;
			}
			StaticModifiers finestTileMod;
			finestTileMod.filePath        = "dummy";
			finestTileMod.instances       = modifiers;
			finestTileMod.nodeToInstances = culledLookup;

			auto* modsJsonNode = SphericalTerrainResource::serializeModifiers(finestTileMod);
			cJSON_AddItemToObject(root, "static modifier", modsJsonNode);
		}

		if (const auto& ocean = pTerrain->m_terrainData->sphericalOcean; ocean.haveOcean)
		{
			WriteJSNode(root, "ocean radius", ocean.geomSettings.radius);
			WriteJSNode(root, "ocean minor radius", ocean.geomSettings.relativeMinorRadius);
		}

		std::unique_ptr<char> str(cJSON_Print(root));
		return str.get();
	}

	bool SphericalTerrainComponent::ExportPCGForServer(const String& terrainPath) {
		static DTransform worldPos = DTransform::IDENTITY;
		PlanetManager* pPlanetManager = Root::instance()->getMainSceneManager()->getPlanetManager();
		if (pPlanetManager == nullptr) return false;
		SphericalTerrain* pSphericalTerrain = pPlanetManager->createSphericalTerrain(terrainPath, true, false, false, worldPos);
		pSphericalTerrain->BuildPlanetRoad(true);
		bool res = pSphericalTerrain->exportPlanetGenObjects();
		pPlanetManager->destroySphericalTerrain(pSphericalTerrain);
		return res;
	}

	bool SphericalTerrainComponent::ExportPCGObjectsBounds(const String& terrainPath)
	{
		PlanetManager* pPlanetManager = Root::instance()->getMainSceneManager()->getPlanetManager();
		if (!pPlanetManager) return false;

		DTransform worldPos = DTransform::IDENTITY;
		SphericalTerrain* pSphericalTerrain = pPlanetManager->createSphericalTerrain(terrainPath, true, false, false, worldPos);
		if (!pSphericalTerrain)
		{
			LogManager::instance()->logMessage("--error-- ExportPCGObjectsBounds createSphericalTerrain failed: " + terrainPath, LML_CRITICAL);
			return false;
		}

		pSphericalTerrain->BuildPlanetRoad(true);

		PCG::PlanetGenObjectScheduler* pScheduler = pSphericalTerrain->getPlanetGenObjectScheduler();
		if (pScheduler) {
			pSphericalTerrain->BuildPlanetRoad(true);
			std::map<Name, std::vector<std::tuple<Vector3, Quaternion, Vector3, uint64>>> allObjects;
			/*
					Name : .mesh路径
					std::tuple<Vector3, Quaternion, Vector3, uint64> : 位置/旋转/缩放/ID

			*/
			pScheduler->ExportGenObjects(allObjects);

			// 采用“面→网格→像素”的统计：先把一个面划分为若干网格单元(cell)，
			// 每个 cell 累加落入该单元的对象三角面数量；最终再把 cell 映射到贴图像素。
            // 网格与贴图分辨率根据星球半径与期望的“单格实际长度”动态生成。
            const float desiredCellLenWs = std::max(1.0f, SphericalTerrainComponent::GetHeatmapDesiredCellLen());   // 期望的每格在世界空间的边长（米/单位），可由UI设置
			const float faceAngSpan = ECHO_PI * 0.5f; // 每个面张角约 90°
			const float planetRadius = pSphericalTerrain->getRadius();

            int gridW = 64;
            int gridH = 64;
            if (pSphericalTerrain->isSphere())
            {
                // 面宽对应的弧长 = R * 90°
                const float faceArcLen = planetRadius * faceAngSpan; // 单位：世界长度
                // 根据期望长度得到格子数量（允许细粒度变化，不强制较大的最小值）
                const float cellLen = std::max(1.0f, desiredCellLenWs);
                const float gridF   = faceArcLen / cellLen;    // 可能为小数
                gridW = Math::Clamp((int)std::round(gridF), 2, 8192);
                gridH = gridW; // 维持方形格子
            }
			// 贴图大小：默认 1 像素 = 1 格，保证纹理与网格一一对应（也可改为每格多像素）
			const int faceW = gridW;
			const int faceH = gridH;
			const float TriUnit = 1000.f;            // 三角面缩放系数，避免数值过大

			// 6 面“网格热量”与每面的最大值（用于归一化）
			std::array<std::vector<float>, 6> gridFaces{
				std::vector<float>(gridW * gridH, 0.f),
				std::vector<float>(gridW * gridH, 0.f),
				std::vector<float>(gridW * gridH, 0.f),
				std::vector<float>(gridW * gridH, 0.f),
				std::vector<float>(gridW * gridH, 0.f),
				std::vector<float>(gridW * gridH, 0.f)
			};
			std::array<float, 6> faceMax{};

			// 遍历所有导出的对象
			int validMeshCount = 0;
			uint64 totalTriangles = 0;

			for (auto& kv : allObjects)
			{
				const Name& meshName = kv.first; // .mesh路径
				try {
					MeshPtr meshptr = MeshMgr::instance()->load(meshName);

					if (meshptr.isNull()) continue;

					// 特殊处理：若为虚拟网格（VirMesh），切换到真实网格再取包围盒与三角面数
					MeshPtr sourceMesh = meshptr;
					if (sourceMesh->m_bVirMesh)
					{
						if (sourceMesh->mRealMesh.isNull())
							continue;
						sourceMesh = sourceMesh->mRealMesh;
					}

					const AxisAlignedBox& meshBound = sourceMesh->getAABB();//模型局部包围盒
					uint32 triangleCount = sourceMesh->getTriangleCount();//模型三角面

					// 调试辅助：确保可命中并观察 triangleCount 的实际值
					volatile uint32 __dbgTriangleCount = triangleCount; // 可在此行打断点
					if (triangleCount == 0u)
					{
						LogManager::instance()->logMessage("--warning-- triangleCount==0 for " + meshName.toString());
					}

					if (meshBound.isNull() || meshBound.isInfinite()) continue;

					// 遍历该mesh的所有实例
					for (const auto& inst : kv.second)
					{
						const Vector3& pos = std::get<0>(inst);      // 位置
						const Quaternion& rot = std::get<1>(inst);   // 旋转
						const Vector3& scale = std::get<2>(inst);    // 缩放
						const uint64 id = std::get<3>(inst);         // ID

						// 计算世界包围盒（应用缩放和旋转）
						Matrix4 mtx;
						mtx.makeTransform(pos, scale, rot);
						AxisAlignedBox worldAabb = meshBound;
						worldAabb.transform(mtx);

						++validMeshCount;
						totalTriangles += triangleCount;

						// 1) 世界→局部→单位球
						Vector3 centerWs = (worldAabb.getMaximum() + worldAabb.getMinimum()) * 0.5f;
						Vector3 centerLs = pSphericalTerrain->ToLocalSpace((DVector3)centerWs);
						centerLs.normalise();
						// 2) 球→cube面及局部坐标
						SphericalTerrainQuadTreeNode::Face face;
						Vector2 xy;
						SphericalTerrainQuadTreeNode::MapToCube(centerLs, face, xy); // xy in [-1,1]
						Vector2 uvLocal = xy * 0.5f + 0.5f; // [0,1]
						// 3) 选择网格 cell
						int gcx = Math::Clamp(int(uvLocal.x * (gridW - 1) + 0.5f), 0, gridW - 1);
						int gcy = Math::Clamp(int(uvLocal.y * (gridH - 1) + 0.5f), 0, gridH - 1);

						// 4) 估算覆盖半径（按角半径换算到 cell 单位），并把三角面均匀摊入覆盖到的 cell 中
						Vector3 half = (worldAabb.getMaximum() - worldAabb.getMinimum()) * 0.5f;
						float linRadius = half.length();
						float angRadius = Math::Clamp(linRadius / std::max(planetRadius, 1e-3f), 0.f, faceAngSpan);
						int crx = std::max(0, int(angRadius * (float)gridW / faceAngSpan + 0.5f));
						int cry = std::max(0, int(angRadius * (float)gridH / faceAngSpan + 0.5f));
						int gxStart = Math::Clamp(gcx - crx, 0, gridW - 1);
						int gxEnd = Math::Clamp(gcx + crx, 0, gridW - 1);
						int gyStart = Math::Clamp(gcy - cry, 0, gridH - 1);
						int gyEnd = Math::Clamp(gcy + cry, 0, gridH - 1);
						const int coverW = gxEnd - gxStart + 1;
						const int coverH = gyEnd - gyStart + 1;
						const int coverN = std::max(1, coverW * coverH);
						const float addShare = (float(triangleCount) / TriUnit) / float(coverN);
						for (int gy = gyStart; gy <= gyEnd; ++gy)
						{
							for (int gx = gxStart; gx <= gxEnd; ++gx)
							{
								int gidx = gy * gridW + gx;
								gridFaces[face][gidx] += addShare;
								faceMax[face] = std::max(faceMax[face], gridFaces[face][gidx]);
							}
						}

						// 这里可以保存或使用 id, meshName, worldAabb, triangleCount
					}
				}
				catch (...) {
					LogManager::instance()->logMessage("--error-- Mesh::load " + meshName.toString());
				}
			}

			// 输出 6 张面贴图（将网格热量映射到像素）
			{
				std::string selPath = terrainPath.c_str();
				std::replace(selPath.begin(), selPath.end(), '\\', '/');
				bool isAbsolute = (selPath.size() > 1 && (selPath[1] == ':' || selPath[0] == '/'));
				if (!isAbsolute)
				{
					std::string root = Root::instance()->getRootResourcePath();
					std::replace(root.begin(), root.end(), '\\', '/');
					if (!root.empty() && root.back() == '/') selPath = root + selPath; else selPath = root + '/' + selPath;
				}
				size_t slash = selPath.find_last_of('/');
				size_t dot = selPath.find_last_of('.');
				std::string dir = (slash != std::string::npos) ? selPath.substr(0, slash) : std::string(".");
				std::string base = selPath;
				if (slash != std::string::npos) base = selPath.substr(slash + 1);
				if (dot != std::string::npos && dot > slash) base = selPath.substr(slash + 1, dot - slash - 1);

                for (int face = 0; face < 6; ++face)
				{
					Image img;
					img.setHeight(faceH);
					img.setWidth(faceW);
					img.setFormat(PixelFormat::PF_R8G8B8);
					img.allocMemory();
					ColorValue color;
					float maxV = std::max(faceMax[face], 1e-6f);
					for (int y = 0; y < faceH; ++y)
					{
						for (int x = 0; x < faceW; ++x)
						{
							// 取该像素对应的网格 cell 值
							int gx = (int)((float)x / (float)faceW * (float)gridW);
							int gy = (int)((float)y / (float)faceH * (float)gridH);
							gx = Math::Clamp(gx, 0, gridW - 1);
							gy = Math::Clamp(gy, 0, gridH - 1);
							float v = gridFaces[face][gy * gridW + gx];
							color = CalculateHeatmapColor(v, maxV);
							img.setColourAt(color, x, y, 0);
						}
					}
					static const char* faceName[6] = { "posx","negx","posy","negy","posz","negz" };
					std::string outFile = dir + "/" + base + ".heatmap_" + faceName[face] + ".png";
					img.save(outFile);
					img.freeMemory();

                    // 生成高分辨率版本，便于肉眼检查与更平滑的贴图显示
                    const int upscale = 8; // 每格扩展到 upscale×upscale 像素
                    const int hiW = faceW * upscale;
                    const int hiH = faceH * upscale;
                    Image hi;
                    hi.setHeight(hiH);
                    hi.setWidth(hiW);
                    hi.setFormat(PixelFormat::PF_R8G8B8);
                    hi.allocMemory();
                    float maxV2 = std::max(faceMax[face], 1e-6f);
                    for (int gy = 0; gy < gridH; ++gy)
                    {
                        for (int gx = 0; gx < gridW; ++gx)
                        {
                            float v = gridFaces[face][gy * gridW + gx];
                            ColorValue c = CalculateHeatmapColor(v, maxV2);
                            int x0 = gx * upscale, x1 = x0 + upscale - 1;
                            int y0 = gy * upscale, y1 = y0 + upscale - 1;
                            for (int py = y0; py <= y1; ++py)
                                for (int px = x0; px <= x1; ++px)
                                    hi.setColourAt(c, px, py, 0);
                        }
                    }
                    std::string outFileHi = dir + "/" + base + ".heatmap_hi_" + faceName[face] + ".png";
                    hi.save(outFileHi);
                    hi.freeMemory();
				}
			}
		}
		pPlanetManager->destroySphericalTerrain(pSphericalTerrain);
		return true;
	}

    // -------------------- Editor helper (heatmap settings) --------------------
    float SphericalTerrainComponent::s_HeatmapDesiredCellLenWs = 500.0f;
    SphericalTerrainComponent::HeatmapColorMode SphericalTerrainComponent::s_HeatmapColorMode = SphericalTerrainComponent::HeatmapColorMode::HEATMAP_RGB;
    ColorValue SphericalTerrainComponent::s_HeatmapBaseColor = ColorValue(1.0f, 0.0f, 0.0f, 1.0f); // 默认红色
    bool SphericalTerrainComponent::s_UseManualThresholds = false;
    float SphericalTerrainComponent::s_ManualMaxThreshold = 12000.0f;
    float SphericalTerrainComponent::s_ManualMinThreshold = 200.0f;

    void SphericalTerrainComponent::SetHeatmapDesiredCellLen(float lenWs)
    {
        // Clamp to a sensible range to avoid pathological allocations
        float clamped = Math::Clamp(lenWs, 1.0f, 100000.0f);
        SphericalTerrainComponent::s_HeatmapDesiredCellLenWs = clamped;
    }

    float SphericalTerrainComponent::GetHeatmapDesiredCellLen()
    {
        return SphericalTerrainComponent::s_HeatmapDesiredCellLenWs;
    }

    void SphericalTerrainComponent::SetHeatmapColorMode(HeatmapColorMode mode)
    {
        SphericalTerrainComponent::s_HeatmapColorMode = mode;
    }

    SphericalTerrainComponent::HeatmapColorMode SphericalTerrainComponent::GetHeatmapColorMode()
    {
        return SphericalTerrainComponent::s_HeatmapColorMode;
    }

    void SphericalTerrainComponent::SetHeatmapBaseColor(const ColorValue& color)
    {
        SphericalTerrainComponent::s_HeatmapBaseColor = color;
    }

    ColorValue SphericalTerrainComponent::GetHeatmapBaseColor()
    {
        return SphericalTerrainComponent::s_HeatmapBaseColor;
    }

    void SphericalTerrainComponent::SetUseManualThresholds(bool enable)
    {
        SphericalTerrainComponent::s_UseManualThresholds = enable;
    }

    bool SphericalTerrainComponent::GetUseManualThresholds()
    {
        return SphericalTerrainComponent::s_UseManualThresholds;
    }

    void SphericalTerrainComponent::SetManualMaxThreshold(float maxValue)
    {
        SphericalTerrainComponent::s_ManualMaxThreshold = Math::Clamp(maxValue, 1.0f, 1000000.0f);
    }

    float SphericalTerrainComponent::GetManualMaxThreshold()
    {
        return SphericalTerrainComponent::s_ManualMaxThreshold;
    }

    void SphericalTerrainComponent::SetManualMinThreshold(float minValue)
    {
        SphericalTerrainComponent::s_ManualMinThreshold = Math::Clamp(minValue, 0.0f, 100000.0f);
    }

    float SphericalTerrainComponent::GetManualMinThreshold()
    {
        return SphericalTerrainComponent::s_ManualMinThreshold;
    }

    ColorValue SphericalTerrainComponent::CalculateHeatmapColor(float rawValue, float autoMaxValue)
    {
        // 根据设置决定使用手动阈值还是自动阈值
        float maxThreshold, minThreshold;
        if (s_UseManualThresholds)
        {
            maxThreshold = s_ManualMaxThreshold;
            minThreshold = s_ManualMinThreshold;
        }
        else
        {
            maxThreshold = std::max(autoMaxValue, 1e-6f);
            minThreshold = 0.0f;
        }
        
        // 确保阈值合理
        if (maxThreshold <= minThreshold)
        {
            maxThreshold = minThreshold + 1.0f;
        }
        
        // 计算标准化的强度值
        float intensity;
        if (rawValue <= minThreshold)
        {
            intensity = 0.0f; // 低于下限，显示为最低强度（可能全黑）
        }
        else if (rawValue >= maxThreshold)
        {
            intensity = 1.0f; // 超过上限，显示为最高强度
        }
        else
        {
            intensity = (rawValue - minThreshold) / (maxThreshold - minThreshold);
        }
        
        // Gamma校正，使变化更平滑
        float t = Math::Pow(Math::Clamp(intensity, 0.f, 1.f), 0.6f);
        
        if (s_HeatmapColorMode == HeatmapColorMode::HEATMAP_RGB)
        {
            // RGB模式：蓝->绿->黄->红的彩虹渐变
            if (t < 0.25f) {
                float k = t / 0.25f;
                return ColorValue(0.f, 0.5f * k, 1.f, 1.f);
            }
            else if (t < 0.5f) {
                float k = (t - 0.25f) / 0.25f;
                return ColorValue(0.f, 0.5f + 0.5f * k, 1.f - k, 1.f);
            }
            else if (t < 0.75f) {
                float k = (t - 0.5f) / 0.25f;
                return ColorValue(k, 1.f, 0.f, 1.f);
            }
            else {
                float k = (t - 0.75f) / 0.25f;
                return ColorValue(1.f, 1.f - k, 0.f, 1.f);
            }
        }
        else // HeatmapColorMode::HEATMAP_SINGLE_COLOR
        {
            // 单色模式：基础颜色按强度变化
            if (intensity <= 0.0f)
            {
                return ColorValue(0.f, 0.f, 0.f, 1.f); // 全黑
            }
            
            ColorValue base = s_HeatmapBaseColor;
            return ColorValue(base.r * t, base.g * t, base.b * t, 1.f);
        }
    }

	bool SphericalTerrainComponent::test(const String& test)
	{
		return false;
	}



	//NOTE: selected region will be highlight display.
    void SphericalTerrainComponent::setSelectedRegion(int regionID)
    {
        
    }
    
    void SphericalTerrainComponent::resetSelectedRegion()
    {
        
    }
    
    void SphericalTerrainComponent::enableRegionVisualize(bool enable)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setEnableRegionVisualEffect(enable);
        }   
    }

    bool SphericalTerrainComponent::setHighlightCorseRegionID(int id)
    {
        bool result = false;
        if(m_pSphericalTerrain)
        {
            result = m_pSphericalTerrain->setHighlightCorseRegionID(id);
        }
        return(result);
    }
        
    void SphericalTerrainComponent::setDisplayCorseOrFineRegion(bool enableCorse)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setDisplayCorseOrFineRegion(enableCorse);
        }
    }
        
    void SphericalTerrainComponent::setRegionGlowColor(const ColorValue& color)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setRegionGlowColor(color);
        }
    }
    void SphericalTerrainComponent::setRegionHighlightColor(const ColorValue& color)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setRegionHighlightColor(color);
        }
    }

    void SphericalTerrainComponent::resetHighlightCorseRegionID()
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setHighlightCorseRegionID(-1);
        }
    }

    bool SphericalTerrainComponent::setHighlightFineRegionID(int id)
    {
        bool result = false;
        if(m_pSphericalTerrain)
        {
            result = m_pSphericalTerrain->setHighlightFineRegionID(id);
        }
        return(result);
    }
    void SphericalTerrainComponent::resetHighlightFineRegionID()
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setHighlightFineRegionID(-1);
        }   
    }

    void SphericalTerrainComponent::setOutlineWidth(float width)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setOutlineWidth(width);
        }   
    }
    
    void SphericalTerrainComponent::setCommonRegionAlpha(float alpha)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setCommonRegionAlpha(alpha);
        }
    }
    
    void SphericalTerrainComponent::setHighlightRegionAlpha(float alpha)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setHighlightRegionAlpha(alpha);
        }
    }

    void SphericalTerrainComponent::setRegionWarfogAlpha(int id, float alpha)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setRegionWarfogAlpha(id, alpha);
        }
    }

    void SphericalTerrainComponent::setCoarseRegionWarfogAlpha(int id, float alpha)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setCoarseRegionWarfogAlpha(id, alpha);
        }
    }
    
    void SphericalTerrainComponent::setRegionWarfogColor(const ColorValue& color)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setRegionWarfogColor(color);
        }
    }
    
	bool SphericalTerrainComponent::SetPlanetEntryComPtr(PlanetEntryControlComponent* ptr)
	{
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

	void SphericalTerrainComponent::UpdateTerrainAtmos() {
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

	void SphericalTerrainComponent::SetAtmosPropertyDirty() {
		if (m_pSphericalTerrain == nullptr)return;
		m_pSphericalTerrain->m_atmosProerptyDirty = true;
	}

	//void SphericalTerrainComponent::EnableAtmosphere(bool enable) {
	//	if (m_pSphericalTerrain == nullptr)return;
	//	m_pSphericalTerrain->m_atmosEnable = enable;
	//}

    void
    SphericalTerrainComponent::setRegionFillColor(const ColorValue& color)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setRegionFillColor(color);
        }
    }

    void
    SphericalTerrainComponent::setCoarseRegionFillColor(const ColorValue& color)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setCoarseRegionFillColor(color);
        }
    }

    void
    SphericalTerrainComponent::setSingleRegionFillColor(int id, const ColorValue& color)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setSingleRegionFillColor(id, color);
        }
    }
    
    void
    SphericalTerrainComponent::setSingleCoarseRegionFillColor(int id, const ColorValue& color)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setSingleCoarseRegionFillColor(id, color);
        }
    }

    void
    SphericalTerrainComponent::setRegionHighlightWarfogColor(const ColorValue& color)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setRegionHighlightWarfogColor(color);
        }
    }
    void SphericalTerrainComponent::setWarfogMaskUVScale(const float scale)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setWarfogMaskUVScale(scale);
        }
    }

    void SphericalTerrainComponent::setAdditionOutLineColor(const ColorValue& color)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setAdditionOutLineColor(color);
        }
    }
    
    void SphericalTerrainComponent::setAdditionOutLineAlpha(const float& v)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setAdditionOutLineAlpha(v);
        }
    }
    
    void SphericalTerrainComponent::setAdditionOutLineWidth(const float& v)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setAdditionOutLineWidth(v);
        }
    }
    
    void SphericalTerrainComponent::setAdditionOutLineDistance(const float& v)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setAdditionOutLineDistance(v);
        }
    }
    
    void SphericalTerrainComponent::setWarfogMaskScale(const float scale)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setWarfogMaskScale(scale);
        }
    }

    void SphericalTerrainComponent::setOutlineSmoothRange(const float range)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setOutlineSmoothRange(range);
        }
    }

    void SphericalTerrainComponent::setWarfogMaskTexture(const String& path)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setWarfogMaskTexture(path);
        }
    }

    //NOTE:Low LOD Cloud visual effect control.
    void SphericalTerrainComponent::setEnableLowLODCloud(bool enalbe)
    {
        if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setEnableLowLODCloud(enalbe);
        }
		m_bEnableLowLODCloud = enalbe;
    }
    void SphericalTerrainComponent::setCloudUVScale(float v)
    {
		v = std::clamp(v, 0.f, 100.f);
      /*  if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setCloudUVScale(v);
        }*/
		m_fCloudUVScale = v;
    }
    void SphericalTerrainComponent::setCloudFlowSpeed(float v)
    {
		v = std::clamp(v,0.f,10.f);
        /*if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setCloudFlowSpeed(v);
        }*/
		m_fCloudFlowSpeed = v;
    }
    void SphericalTerrainComponent::setCloudHDRFactor(float v)
    {
		v = std::clamp(v, 0.f, 10.f);
    /*    if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setCloudHDRFactor(v);
        }*/
		m_fCloudHDRFactor = v;
    }
    void SphericalTerrainComponent::setCloudMetallic(float v)
    {
		v = std::clamp(v, 0.f, 1.f);
    /*    if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setCloudMetallic(v);
        }*/
		m_fCloudMetallic = v;
    }
    void SphericalTerrainComponent::setCloudRoughness(float v)
    {
		v = std::clamp(v, 0.f, 1.f);
        //if(m_pSphericalTerrain)
        //{
        //    m_pSphericalTerrain->setCloudRoughness(v);
        //}
		m_fCloudRoughness = v;
    }
    void SphericalTerrainComponent::setCloudAmbientFactor(float v)
    {
		v = std::clamp(v, 0.f, 1.f);
    /*    if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setCloudAmbientFactor(v);
        }*/
		m_fCloudAmbientFactor = v;

    }
    void SphericalTerrainComponent::setCloudNoiseOffsetScale(float v)
    {
		v = std::clamp(v, 0.f, 1.f);
     /*   if(m_pSphericalTerrain)
        {
            m_pSphericalTerrain->setCloudNoiseOffsetScale(v);
        }*/
		m_fCloudNoiseOffsetScale = v;
	}
    
    void SphericalTerrainComponent::setCloudTexture(const String& path)
    {
    /*    if(m_pSphericalTerrain)
        {
			if (path.empty())
				m_pSphericalTerrain->setCloudTexture(g_CloudInitDiffuseTex.toString());
			else
				m_pSphericalTerrain->setCloudTexture(path);
        } */       
		m_strCloudTexture = path;
    }

	bool SphericalTerrainComponent::getEnableLowLODCloud()
	{
		return m_bEnableLowLODCloud;
	}
	float SphericalTerrainComponent::getCloudUVScale()
	{
		return m_fCloudUVScale;
	}
	float SphericalTerrainComponent::getCloudFlowSpeed()
	{
		return m_fCloudFlowSpeed;
	}
	float SphericalTerrainComponent::getCloudHDRFactor()
	{
		return m_fCloudHDRFactor;
	}
	float SphericalTerrainComponent::getCloudMetallic()
	{
		return m_fCloudMetallic;
	}
	float SphericalTerrainComponent::getCloudRoughness()
	{
		return m_fCloudRoughness;
	}
	float SphericalTerrainComponent::getCloudAmbientFactor()
	{
		return m_fCloudAmbientFactor;
	}
	float SphericalTerrainComponent::getCloudNoiseOffsetScale()
	{
		return m_fCloudNoiseOffsetScale;
	}
	const String& SphericalTerrainComponent::getCloudTexture()
	{
		return m_strCloudTexture;
	}

	void SphericalTerrainComponent::setEnableVolumetricCloud(const bool& val)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setEnableVolumetricCloud(val);
		}
		mCloudProperty.setEnableVolumetricCloud(val);
	}

	void SphericalTerrainComponent::setLayerBottomAltitudeKm(const float& val)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setLayerBottomAltitudeKm(val);
		}
		mCloudProperty.setLayerBottomAltitudeKm(val);
	}
	void SphericalTerrainComponent::setLayerHeightKm(const float& val)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setLayerHeightKm(val);
		}
		mCloudProperty.setLayerHeightKm(val);
	}
	void SphericalTerrainComponent::setTracingStartMaxDistance(const float& val)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setTracingStartMaxDistance(val);
		}
		mCloudProperty.setTracingStartMaxDistance(val);
	}
	void SphericalTerrainComponent::setTracingMaxDistance(const float& val)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setTracingMaxDistance(val);
		}
		mCloudProperty.setTracingMaxDistance(val);
	}

	void SphericalTerrainComponent::setCloudBasicNoiseScale(const float& val)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setCloudBasicNoiseScale(val);
		}
		mCloudProperty.setCloudBasicNoiseScale(val);
	}
	void SphericalTerrainComponent::setStratusCoverage(const float& val)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setStratusCoverage(val);
		}
		mCloudProperty.setStratusCoverage(val);
	}
	void SphericalTerrainComponent::setStratusContrast(const float& val)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setStratusContrast(val);
		}
		mCloudProperty.setStratusContrast(val);
	}
	void SphericalTerrainComponent::setMaxDensity(const float& val)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setMaxDensity(val);
		}
		mCloudProperty.setMaxDensity(val);
	}
	void SphericalTerrainComponent::setWindOffset(const Vector3& val)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setWindOffset(val);
		}
		mCloudProperty.setWindOffset(val);
	}

	void SphericalTerrainComponent::setCloudColor(const ColorValue& val)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setCloudColor(val);
		}
		mCloudProperty.setCloudColor(val);
	}

	void SphericalTerrainComponent::setCloudPhaseG(const float& val)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setCloudPhaseG(val);
		}
		mCloudProperty.setCloudPhaseG(val);
	}
	void SphericalTerrainComponent::setCloudPhaseG2(const float& val)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setCloudPhaseG2(val);
		}
		mCloudProperty.setCloudPhaseG2(val);
	}
	void SphericalTerrainComponent::setCloudPhaseMixFactor(const float& val)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setCloudPhaseMixFactor(val);
		}
		mCloudProperty.setCloudPhaseMixFactor(val);
	}
	void SphericalTerrainComponent::setMsScattFactor(const float& val)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setMsScattFactor(val);
		}
		mCloudProperty.setMsScattFactor(val);
	}
	void SphericalTerrainComponent::setMsExtinFactor(const float& val)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setMsExtinFactor(val);
		}
		mCloudProperty.setMsExtinFactor(val);
	}
	void SphericalTerrainComponent::setMsPhaseFactor(const float& val)
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setMsPhaseFactor(val);
		}
		mCloudProperty.setMsPhaseFactor(val);
	}

	void SphericalTerrainComponent::updateVolumetricCloudParams()
	{
		if (m_pSphericalTerrain)
		{
			m_pSphericalTerrain->setEnableVolumetricCloud(mCloudProperty.getEnableVolumetricCloud());
			m_pSphericalTerrain->setLayerBottomAltitudeKm(mCloudProperty.getLayerBottomAltitudeKm());
			m_pSphericalTerrain->setLayerHeightKm(mCloudProperty.getLayerHeightKm());
			m_pSphericalTerrain->setTracingStartMaxDistance(mCloudProperty.getTracingStartMaxDistance());
			m_pSphericalTerrain->setTracingMaxDistance(mCloudProperty.getTracingMaxDistance());
			m_pSphericalTerrain->setCloudBasicNoiseScale(mCloudProperty.getCloudBasicNoiseScale());
			m_pSphericalTerrain->setStratusCoverage(mCloudProperty.getStratusCoverage());
			m_pSphericalTerrain->setStratusContrast(mCloudProperty.getStratusContrast());
			m_pSphericalTerrain->setMaxDensity(mCloudProperty.getMaxDensity());
			m_pSphericalTerrain->setWindOffset(mCloudProperty.getWindOffset());
			m_pSphericalTerrain->setCloudColor(mCloudProperty.getCloudColor());
			m_pSphericalTerrain->setCloudPhaseG(mCloudProperty.getCloudPhaseG());
			m_pSphericalTerrain->setCloudPhaseG2(mCloudProperty.getCloudPhaseG2());
			m_pSphericalTerrain->setCloudPhaseMixFactor(mCloudProperty.getCloudPhaseMixFactor());
			m_pSphericalTerrain->setMsScattFactor(mCloudProperty.getMsScattFactor());
			m_pSphericalTerrain->setMsExtinFactor(mCloudProperty.getMsExtinFactor());
			m_pSphericalTerrain->setMsPhaseFactor(mCloudProperty.getMsPhaseFactor());
		}
	}

	void SphericalTerrainComponent::BuildPlanetRoad(bool sync)
	{
		if (!m_pSphericalTerrain)
		{
			LogManager::instance()->logMessage("SphericalTerrainComponent::BuildPlanetRoad() planet is null");
			return;
		}
		m_pSphericalTerrain->BuildPlanetRoad(sync);
		if(m_pSphericalTerrain->getPlanetRoad())
		{
			if (planetroadListener == nullptr)
				planetroadListener = new RoadListener(this);
			m_pSphericalTerrain->getPlanetRoad()->addListener(planetroadListener);
			if (sync)
				RoadManager::instance()->updatePlanetBuildingMaskCache(m_pSphericalTerrain, true);
		}
	}

	void SphericalTerrainComponent::CreatePlanetRoad(bool sync)
	{
		if (GetTerrainPath().empty() || !GetSphericalTerrain())
		{
			LogManager::instance()->logMessage("SphericalTerrainComponent::CreatePlanetRoad() planet is null");
			return;
		}
		if (!GetSphericalTerrain()->isCreateFinish())
		{
			LogManager::instance()->logMessage("SphericalTerrainComponent::CreatePlanetRoad() planet is not create finished");
			return;
		}
		if (!m_pSphericalTerrain->mRoad || m_pSphericalTerrain->mRoad->mPlanetRoadData.isNull())
		{
			LogManager::instance()->logMessage("SphericalTerrainComponent::CreatePlanetRoad() there is no road in planet");
			return;
		}
		const PlanetRoadGroup& roadGroup = m_pSphericalTerrain->mRoad->mPlanetRoadData->getPlanetRoadGroup();
		if (roadGroup.roads.empty())
		{
			LogManager::instance()->logMessage("SphericalTerrainComponent::CreatePlanetRoad() planet roads empty");
			return;
		}
		if (!m_FitTerrain.get())
			m_FitTerrain = std::make_unique<PlanetRoadTool>(m_pSphericalTerrain);

		for (auto& road : roadCache)
		{
			if (!road.second.expired())
			{
				PavementComponent* pavementCom = static_cast<PavementComponent*>(road.second->getComponent("Road").get());
				if (pavementCom)
				{
					pavementCom->setWaitDelete(true);
				}
			}
			getActor()->removeActor(road.second);
			getActorManager()->deleteActor(road.second);
		}
		roadCache.clear();

		ActorComFactory* bezierFactory = ActorComFactoryManager::getSingleton().GetActorComFactory(COMPONENTTYPE(PiecewiseBezierComponent));

		ActorPtr planetAct = getActor();
		NameGenerator roadName("PlanetRoad");
		for (const PlanetRoadData& road : roadGroup.roads)
		{
			if (road.beziers.empty() || road.datas.empty())
				continue;

			ActorPtr  roadAct = getActorManager()->createActor();
			if (roadAct.expired())
				continue;

			std::string roadStr = roadName.generate();
			roadAct->setActorID(roadStr);
			roadAct->setActorInfoID(roadStr);

			planetAct->addActor(planetAct->getRootNodeCom(), roadAct, false);
			ActorComponentPtr rootNode = roadAct->getRootNodeCom();

			roadAct->setPosition(road.position);
			roadAct->setRotation(road.quat);

			PiecewiseBezierComponentInfo* bezier_info = static_cast<PiecewiseBezierComponentInfo*>(bezierFactory->createComponentInfo());

			for (const PlanetRoadBezierPoint& bezierPoint : road.beziers)
			{
				PiecewiseBezierComponentInfo::BezierInfoPoint point;
				point.point = bezierPoint.point;
				point.destination_control = bezierPoint.destination_control;
				point.source_control = bezierPoint.source_control;
				point.normal = bezierPoint.normal;
				point.FitGorundLevel = bezierPoint.FitGorundLevel;

				bezier_info->mPoints.push_back(point);
			}
			ActorComponentPtr bezierCom = roadAct->addComponent(COMPONENTTYPE(PiecewiseBezierComponent), "Bezier", rootNode, bezier_info/*, 0, false*/);

#ifdef ECHO_EDITOR
			PavementComponent* pavementCom = static_cast<PavementComponent*>(roadAct->addComponent(COMPONENTTYPE(PavementComponent), "Road", rootNode, nullptr, 0, false).get());
			PavementComponentInfo* pavement_info = static_cast<PavementComponentInfo*>(pavementCom->m_pActComInfo);
			pavement_info->mBindComName = "Bezier";
			pavement_info->isFitGround = road.isFitGround;
			pavement_info->mVegetationCollection = road.VegetationCollection;
#else
			PavementComponent* pavementCom = static_cast<PavementComponent*>(roadAct->addComponent(COMPONENTTYPE(PavementComponent), "Road", rootNode/*, nullptr, 0, false*/).get());
#endif // ECHO_EDITOR

			pavementCom->setBezierCom(bezierCom);
			pavementCom->setPavementFitTerrain(m_FitTerrain.get());
			pavementCom->setFitGroundModel(road.isFitGround);
			pavementCom->setVegetationCollection(road.VegetationCollection);
			NameGenerator pavementName("Pavement");
			for (const PlanetRoadDesc& roadData : road.datas)
			{
				PavementMeshBaseData pavementData;
				pavementData.groupName = pavementName.generate();
				pavementData.meshName = roadData.meshName;
				pavementData.offset = roadData.offset;
				pavementData.quat = roadData.quat;
				pavementData.scale = roadData.scale;
				pavementData.partLength = roadData.partLength;
				pavementData.m_SpacingStep = roadData.m_SpacingStep;
				pavementData.m_RenderDistance = roadData.m_RenderDistance;
				pavementData.m_Lod1Distance = roadData.m_Lod1Distance;
				pavementData.m_Lod2Distance = roadData.m_Lod2Distance;
				pavementData.m_surfaceType = roadData.m_surfaceType;
#ifdef ECHO_EDITOR
				pavement_info->mPavementData.push_back(pavementData);
#else
				pavementCom->createPavement(pavementData, PavementComponent::UpdateAll, sync);
#endif
			}
#ifdef ECHO_EDITOR
			for (auto& data : pavement_info->mPavementData)
			{
				pavementCom->createPavement(data, PavementComponent::UpdateAll, sync);
			}
#endif

			roadCache.emplace(road.roadID, roadAct);
		}
	}
	void SphericalTerrainComponent::ClearPlanetRoadCache()
	{
		if (m_pSphericalTerrain && m_pSphericalTerrain->getPlanetRoad())
		{
			m_pSphericalTerrain->getPlanetRoad()->removeListener(planetroadListener);
			SAFE_DELETE(planetroadListener);
			for (auto& road : roadCache)
			{
				if (!road.second.expired())
				{
					PavementComponent* pavementCom = static_cast<PavementComponent*>(road.second->getComponent("Road").get());
					if (pavementCom)
					{
						pavementCom->setWaitDelete(true);
					}
				}
				getActor()->removeActor(road.second);
				getActorManager()->deleteActor(road.second);
			}
			roadCache.clear();
		}
	}
	void SphericalTerrainComponent::RoadListener::onBuildFinish()
	{
		if (!planetCom || !planetCom->GetSphericalTerrain() || !planetCom->GetSphericalTerrain()->mRoad ||
			!planetCom->GetSphericalTerrain()->mRoad->isBuild() || planetCom->GetSphericalTerrain()->mRoad->mPlanetRoadData.isNull())
			return;
		RoadManager::instance()->updatePlanetBuildingMaskCache(planetCom->GetSphericalTerrain(), true);
	}
	void SphericalTerrainComponent::RoadListener::onCreate(const PlanetRoadData& road)
	{
		if (!planetCom)
			return;
		if (planetCom->roadCache.find(road.roadID) != planetCom->roadCache.end())
			return;
		if (road.beziers.empty() || road.datas.empty())
			return;

		ActorPtr  planetAct = planetCom->getActor();
		ActorPtr  roadAct = planetCom->getActorManager()->createActor();
		if (roadAct.expired())
			return;

		std::ostringstream roadName;
		roadName << "PlanetRoad" << road.roadID;

		std::string roadStr = roadName.str();
		roadAct->setActorID(roadStr);
		roadAct->setActorInfoID(roadStr);

		planetAct->addActor(planetAct->getRootNodeCom(), roadAct, false);
		ActorComponentPtr rootNode = roadAct->getRootNodeCom();

		roadAct->setPosition(road.position);
		roadAct->setRotation(road.quat);

		ActorComFactory* bezierFactory = ActorComFactoryManager::getSingleton().GetActorComFactory(COMPONENTTYPE(PiecewiseBezierComponent));
		PiecewiseBezierComponentInfo* bezier_info = static_cast<PiecewiseBezierComponentInfo*>(bezierFactory->createComponentInfo());

		for (const PlanetRoadBezierPoint& bezierPoint : road.beziers)
		{
			PiecewiseBezierComponentInfo::BezierInfoPoint point;
			point.point = bezierPoint.point;
			point.destination_control = bezierPoint.destination_control;
			point.source_control = bezierPoint.source_control;
			point.normal = bezierPoint.normal;
			point.FitGorundLevel = bezierPoint.FitGorundLevel;

			bezier_info->mPoints.push_back(point);
		}
		ActorComponentPtr bezierCom = roadAct->addComponent(COMPONENTTYPE(PiecewiseBezierComponent), "Bezier", rootNode, bezier_info/*, 0, false*/);

#ifdef ECHO_EDITOR
		PavementComponent* pavementCom = static_cast<PavementComponent*>(roadAct->addComponent(COMPONENTTYPE(PavementComponent), "Road", rootNode, nullptr, 0, false).get());
		PavementComponentInfo* pavement_info = static_cast<PavementComponentInfo*>(pavementCom->m_pActComInfo);
		pavement_info->mBindComName = "Bezier";
		pavement_info->isFitGround = road.isFitGround;
		pavement_info->mVegetationCollection = road.VegetationCollection;
#else
		PavementComponent* pavementCom = static_cast<PavementComponent*>(roadAct->addComponent(COMPONENTTYPE(PavementComponent), "Road", rootNode/*, nullptr, 0, false*/).get());
#endif // ECHO_EDITOR
		pavementCom->setBezierCom(bezierCom);
		pavementCom->setPavementFitTerrain(planetCom->m_FitTerrain.get());
		pavementCom->setFitGroundModel(road.isFitGround);
		pavementCom->setVegetationCollection(road.VegetationCollection);
		NameGenerator pavementName("Pavement");
		for (const PlanetRoadDesc& roadData : road.datas)
		{
			PavementMeshBaseData pavementData;
			pavementData.groupName = pavementName.generate();
			pavementData.meshName = roadData.meshName;
			pavementData.offset = roadData.offset;
			pavementData.quat = roadData.quat;
			pavementData.scale = roadData.scale;
			pavementData.partLength = roadData.partLength;
			pavementData.m_SpacingStep = roadData.m_SpacingStep;
			pavementData.m_RenderDistance = roadData.m_RenderDistance;
			pavementData.m_Lod1Distance = roadData.m_Lod1Distance;
			pavementData.m_Lod2Distance = roadData.m_Lod2Distance;
			pavementData.m_surfaceType = roadData.m_surfaceType;
#ifdef ECHO_EDITOR
			pavement_info->mPavementData.push_back(pavementData);
#else
			pavementCom->createPavement(pavementData, PavementComponent::UpdateAll, false);
#endif
		}
#ifdef ECHO_EDITOR
		for (auto& data : pavement_info->mPavementData)
		{
			pavementCom->createPavement(data, PavementComponent::UpdateAll, false);
		}
#endif
		planetCom->roadCache.emplace(road.roadID, roadAct);

		RoadManager::instance()->updatePlanetRoadMaskCache(planetCom->GetSphericalTerrain(), road, true, false);
	}

	void SphericalTerrainComponent::RoadListener::onDestroy(const PlanetRoadData& road)
	{
		if (!planetCom)
			return;
		if (planetCom->roadCache.find(road.roadID) == planetCom->roadCache.end())
			return;
		ActorPtr roadAct = planetCom->roadCache[road.roadID];
		if (roadAct.expired())
			return;
		PavementComponent* pavementCom = static_cast<PavementComponent*>(roadAct->getComponent("Road").get());
		if (pavementCom)
		{
			pavementCom->setWaitDelete(true);
		}
		planetCom->getActor()->delActor(roadAct);
		planetCom->roadCache.erase(road.roadID);
		RoadManager::instance()->updatePlanetRoadMaskCache(planetCom->GetSphericalTerrain(), road, false, false);
	}
#ifdef ECHO_EDITOR
	bool SphericalTerrainComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) {
		return true;
	}
#endif

}
