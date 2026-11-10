#include "EchoComponentStableHeaders.h"
#include "EchoPlanetConverterComponent.h"
#include "EchoCommonStableHeaders.h"
#include "EchoName.h"
#include "Actor/EchoActor.h"
#include "EchoActorProFuncImp.h"
#include "EchoActorEntityComponent.h"
#include "EchoSubMesh.h"
#include "EchoMaterial.h"
#include "EchoMaterialManager.h"
#include "EchoTextureManager.h"
#include "EchoMeshMgr.h"
#include "EchoLogManager.h"
#include "EchoShaderBin.h"
#include "EchoShaderMan.h"

// #define PLC_PERF_TEST

#ifdef PLC_PERF_TEST
#include "EchoProfilerManager.h"
#endif

namespace Echo
{
	EDITOR_REFLECT_BEGIN(PlanetConverterComponentInfo)

		EDITOR_REFLECT_MEMBER_UPDATE(m_UseMesh, "UseMesh", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_UsePlanet, "UsePlanet", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_MeshPath, "MeshPath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		EDITOR_REFLECT_MEMBER_UPDATE(m_TexturePath, "TexturePath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_SaveTexturePath, "SaveTexturePath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_PolarReserve, "PolarReserve", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Color, "Color", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BlurStrength, "BlurStrength", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_FillStrength, "FillStrength", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		//EDITOR_REFLECT_MEMBER_UPDATE(m_HeightMapPath, "HeightMapPath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_ClientDiffPath, "ClientDiffPath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_NormalMapPath, "NormalMapPath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(m_ResolutionL0, "ResolutionL0", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(m_ResolutionL1, "ResolutionL1", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(m_ResolutionL2, "ResolutionL2", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Radius, "Radius", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_MaxHeight, "MaxHeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_XRollSpeed, "XRollSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_YRollSpeed, "YRollSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_ZRollSpeed, "ZRollSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		EDITOR_REFLECT_MEMBER_UPDATE(m_EnableAtmosphere, "EnableAtmosphere", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AtmosphereHeight, "AtmosphereHeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(m_AtmosphereResolutionL0, "AtmosphereResolutionL0", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(m_AtmosphereResolutionL1, "AtmosphereResolutionL1", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(m_AtmosphereResolutionL2, "AtmosphereResolutionL2", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		EDITOR_REFLECT_MEMBER_UPDATE(m_EnableCloud, "EnableCloud", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CloudTexPath, "CloudTexPath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CloudHeight, "CloudHeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(m_CloudResolutionL0, "CloudResolutionL0", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(m_CloudResolutionL1, "CloudResolutionL1", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(m_CloudResolutionL2, "CloudResolutionL2", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		EDITOR_REFLECT_MEMBER_UPDATE(m_EnableWater, "EnableWater", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_WaterNormPath, "WaterNormPath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_WaterHeight, "WaterHeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(m_WaterResolutionL0, "WaterResolutionL0", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(m_WaterResolutionL1, "WaterResolutionL1", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(m_WaterResolutionL2, "WaterResolutionL2", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		EDITOR_REFLECT_MEMBER_UPDATE(m_CullMode, "CullMode", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CullDistance, "CullDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_TodCullDistance, "TodCullDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_DissolveStrength, "DissolveStrength", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_FpsState, "FpsState", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CastShadow, "CastShadow", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_LodDis_Level1, "LodDis_Level1", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_LodDis_Level2, "LodDis_Level2", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		EDITOR_REFLECT_MEMBER_UPDATE(m_matParam, "matParam", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();

	PlanetConverterComponentInfo::PlanetConverterComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) : ActorComponentInfo(comType, MoveType)
	{
	}

	PlanetConverterComponentInfo::~PlanetConverterComponentInfo()
	{
	}

	void PlanetConverterComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<PlanetConverterComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void PlanetConverterComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<PlanetConverterComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void PlanetConverterComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		PlanetConverterComponentInfo* info = static_cast<PlanetConverterComponentInfo*>(comInfo);
		reflect::TypeResolver<PlanetConverterComponentInfo>::get()->copyTo(this, info);
	}

	void PlanetConverterComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr) {
			reflect::TypeResolver<PlanetConverterComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

	//size_t PlanetConverterComponentInfo::GetResolution(const size_t lod)const
	//{
	//	switch (lod)
	//	{
	//		case 0: return m_ResolutionL0;
	//		case 1: return m_ResolutionL1;
	//		case 2: return m_ResolutionL2;
	//		default: return -1;
	//	}
	//}

	//size_t PlanetConverterComponentInfo::GetAtmosphereResolution(const size_t lod)const
	//{
	//	switch (lod)
	//	{
	//		case 0: return m_AtmosphereResolutionL0;
	//		case 1: return m_AtmosphereResolutionL1;
	//		case 2: return m_AtmosphereResolutionL2;
	//		default: return -1;
	//	}
	//}

	//size_t PlanetConverterComponentInfo::GetCloudResolution(const size_t lod)const
	//{
	//	switch (lod)
	//	{
	//		case 0: return m_CloudResolutionL0;
	//		case 1: return m_CloudResolutionL1;
	//		case 2: return m_CloudResolutionL2;
	//		default: return -1;
	//	}
	//}

	//size_t PlanetConverterComponentInfo::GetWaterResolution(const size_t lod)const
	//{
	//	switch (lod)
	//	{
	//		case 0: return m_WaterResolutionL0;
	//		case 1: return m_WaterResolutionL1;
	//		case 2: return m_WaterResolutionL2;
	//		default: return -1;
	//	}
	//}

	//static ColorValue ColorLerp(ColorValue a, ColorValue b, float l, bool clamp = false)
	//{
	//	if (clamp)
	//	{
	//		l = Math::Clamp(l, 0.0f, 1.0f);
	//	}
	//	return a * (1 - l) + b * l;
	//}

	//static ColorValue GetPixelBilinear(Image& img, float u, float v)
	//{
	//	uint32 sizeX = img.getWidth();
	//	uint32 sizeY = img.getHeight();
	//	float fsizeX = static_cast<float>(sizeX);
	//	float fsizeY = static_cast<float>(sizeY);
	//	float posu = fsizeX * u;
	//	float posv = fsizeY * v;
	//	uint32 ldu = static_cast<uint32>(posu);
	//	uint32 ldv = static_cast<uint32>(posv);

	//	uint32 u1 = ldu % sizeX;
	//	uint32 u2 = (ldu + 1) % sizeX;
	//	uint32 v1 = ldv % sizeY;
	//	uint32 v2 = (ldv + 1) % sizeY;

	//	ColorValue ld = img.getColourAt(u1, v1, 0);
	//	ColorValue lu = img.getColourAt(u1, v2, 0);
	//	ColorValue rd = img.getColourAt(u2, v1, 0);
	//	ColorValue ru = img.getColourAt(u2, v2, 0);
	//	float ur = Math::Repeat(posu, 1.0f);
	//	float vr = Math::Repeat(posv, 1.0f);
	//	ColorValue dc = ColorLerp(ld, rd, ur);
	//	ColorValue uc = ColorLerp(lu, ru, ur);
	//	ColorValue res = ColorLerp(dc, uc, vr);
	//	res.a = 1.0f;
	//	return res;
	//}

	static uint32 GetParamOnlyId(uint8 mat, uint8 shaderIndex, uint8 uIndex, uint8 pIndex)
	{
		return (static_cast<uint32>(mat) << 24) | (static_cast<uint32>(shaderIndex) << 16) | (static_cast<uint32>(uIndex) << 8) | (static_cast<uint32>(pIndex));
	}

	static std::vector<std::vector<Vector3>> g_VerticesBaseWithUV;
	static std::vector<std::vector<uint16>> g_TrianglesBaseWithUV;
	static std::vector<std::vector<Vector2>> g_UVsBaseWithUV; // vert tri uv 不随displace变化，重用下
	ECHO_STATIC_MUTEX(g_BaseLock)

	DECLAREACTORCOMTYPE_CPP(PlanetConverterComponent)
	PlanetConverterComponent::PlanetConverterComponent(ActorPtr pActor, ActorCOMMoveableType movableType)
		: ProtocolNodeActorComponent(pActor, movableType)
	{
		registerTickTimeBegin();
	}

	PlanetConverterComponent::~PlanetConverterComponent()
	{
	}

	void PlanetConverterComponent::init(ActorComponentInfo* info)
	{
	}

	void PlanetConverterComponent::unInit()
	{
		if (isActComNotInitMaskEnable(ActComNotInitMask_RenderObject))
			return;

		m_Base.Unload(this);
		m_Atmosphere.Unload(this);
		m_Cloud.Unload(this);
		m_Water.Unload(this);
		if (m_pActComInfo == nullptr)
			return;

		//PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		//if (mPlanetConverterComponentInfo == nullptr)
		//	return;

		//if(!mPlanetConverterComponentInfo->m_UseMesh)
		//	CommonJobManager::instance()->removeListener(this);
	}

	void PlanetConverterComponent::initRenderObject(ActorComponentInfo* info)
	{
		PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(info);
		if (mPlanetConverterComponentInfo == nullptr)
			return;
		// CopySettings(info);
		m_Base.entitySuffix = "Base";
		m_Base.shadername = "IllumPBR";
		m_Base.genmask = "%PBR_SIMPLE%BUMP_MAP";
		m_Base.tex.emplace_back("Diffuse", mPlanetConverterComponentInfo->m_ClientDiffPath);
		m_Base.normal.emplace_back("Normal", mPlanetConverterComponentInfo->m_NormalMapPath);
		m_Atmosphere.entitySuffix = "Atmosphere";
		m_Atmosphere.shadername = "Atmosphere";
		m_Atmosphere.statemask = "%TRANSPARENT";
		m_Cloud.entitySuffix = "Cloud";
		m_Cloud.shadername = "CloudSphere";
		m_Cloud.statemask = "%TRANSPARENT";
		m_Cloud.tex.emplace_back("Customer", mPlanetConverterComponentInfo->m_CloudTexPath);
		m_Water.entitySuffix = "Water";
		m_Water.shadername = "WaterSphere";
		m_Water.tex.emplace_back("Normal", mPlanetConverterComponentInfo->m_WaterNormPath);
		UpdateRollQuaternion();
	}

	void PlanetConverterComponent::initHelperObject(ActorComponentInfo* info)
	{}

	void PlanetConverterComponent::bindActor()
	{
		Generate();
	}
	void PlanetConverterComponent::registerTickTimeBegin()
	{
		m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}

	void PlanetConverterComponent::beginFrame(EventID eventid, ProtocolHandle handle)
	{
		PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		if (mPlanetConverterComponentInfo == nullptr)
			return;


		//if (mPlanetConverterComponentInfo->m_ClientDiffPath == "" || mPlanetConverterComponentInfo->m_HeightMapPath == "")
		//	return;
		switch (eventid)
		{
			case EID_EveryFrame:
			{
				m_CurrentRotation = m_CurrentRotation * m_CurrentRollSpeed;
				this->setRotation(m_CurrentRotation);
				break;
			}
			default:
				break;
		}
	}


	//void PlanetConverterComponent::Execute()
	//{
	//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
	//	if (mPlanetConverterComponentInfo == nullptr)
	//		return;

	//	if (mPlanetConverterComponentInfo->m_UsePlanet)
	//		AssembleVertexBuffer(m_Base);

	//	if (mPlanetConverterComponentInfo->m_EnableAtmosphere)
	//	{
	//		AssembleVertexBuffer(m_Atmosphere);
	//	}
	//	if (mPlanetConverterComponentInfo->m_EnableCloud)
	//	{
	//		AssembleVertexBuffer(m_Cloud);
	//	}
	//	if (mPlanetConverterComponentInfo->m_EnableWater)
	//	{
	//		AssembleVertexBuffer(m_Water);
	//	}
	//}

	//void PlanetConverterComponent::CommonTaskFinish(uint64 requestID)
	//{
	//	if (m_ticket != requestID)
	//		return;

	//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
	//	if (mPlanetConverterComponentInfo == nullptr)
	//		return;

	//	if (mPlanetConverterComponentInfo->m_UsePlanet)
	//		AssembleMeshData(m_Base);

	//	if (mPlanetConverterComponentInfo->m_EnableAtmosphere)
	//	{
	//		AssembleMeshData(m_Atmosphere);
	//	}
	//	if (mPlanetConverterComponentInfo->m_EnableCloud)
	//	{
	//		AssembleMeshData(m_Cloud);
	//	}
	//	if (mPlanetConverterComponentInfo->m_EnableWater)
	//	{
	//		AssembleMeshData(m_Water);
	//	}

	//	m_ticket = 0;
	//}

	void PlanetConverterComponent::Generate()
	{

		if (m_pActComInfo == nullptr)
			return;

		PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		if (mPlanetConverterComponentInfo == nullptr)
			return;

		if (isActComNotInitMaskEnable(ActComNotInitMask_RenderObject))
			return;

		//CommonJobManager::instance()->removeRequest(m_ticket);
		//if (mPlanetConverterComponentInfo->m_ClientDiffPath == "" || mPlanetConverterComponentInfo->m_HeightMapPath == "")
		//	return;

		//if (mPlanetConverterComponentInfo->m_UseMesh)
		//{
		Name meshPath = Name(mPlanetConverterComponentInfo->m_MeshPath);
		m_Base.Unload(this);

		if (mPlanetConverterComponentInfo->m_UsePlanet)
		{
			AssembleMaterial(false, m_Base);
			m_Base.LoadMesh(this, mPlanetConverterComponentInfo->m_Radius, meshPath);
		}

		if (mPlanetConverterComponentInfo->m_EnableAtmosphere)
		{
			m_Atmosphere.Unload(this);
			AssembleMaterial(false, m_Atmosphere);
			m_Atmosphere.LoadMesh(this, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_AtmosphereHeight, meshPath);
		}
		if (mPlanetConverterComponentInfo->m_EnableCloud)
		{
			m_Cloud.Unload(this);
			AssembleMaterial(false, m_Cloud);
			m_Cloud.LoadMesh(this, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_CloudHeight, meshPath);
		}
		if (mPlanetConverterComponentInfo->m_EnableWater)
		{
			m_Water.Unload(this);
			AssembleMaterial(false, m_Water);
			m_Water.LoadMesh(this, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_WaterHeight, meshPath);
		}
		//}
		//else
		//{
		//	GenerateBase();
		//	m_Base.Unload(this);

		//	if (mPlanetConverterComponentInfo->m_UsePlanet)
		//	{
		//		for (size_t i = 0; i < 3; i++)
		//		{
		//			CopyBase(i, mPlanetConverterComponentInfo->GetResolution(i), m_Base);
		//			ModifySize(i, mPlanetConverterComponentInfo->m_Radius, m_Base);
		//			Displace(i, mPlanetConverterComponentInfo->m_HeightMapPath, mPlanetConverterComponentInfo->m_MaxHeight, m_Base);
		//			GenerateNormal(i, m_Base);
		//			GenerateTangent(i, m_Base);
		//		}
		//		AssembleMaterial(false, m_Base);
		//	}
		//	
		//	if (mPlanetConverterComponentInfo->m_EnableAtmosphere)
		//	{
		//		m_Atmosphere.Unload(this);

		//		for (size_t i = 0; i < 3; i++)
		//		{
		//			CopyBase(i, mPlanetConverterComponentInfo->GetAtmosphereResolution(i), m_Atmosphere);
		//			ModifySize(i, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_AtmosphereHeight, m_Atmosphere);
		//			GenerateNormal(i, m_Atmosphere);
		//			GenerateTangent(i, m_Atmosphere);
		//		}
		//		AssembleMaterial(false, m_Atmosphere);
		//	}

		//	if (mPlanetConverterComponentInfo->m_EnableCloud)
		//	{
		//		m_Cloud.Unload(this);

		//		for (size_t i = 0; i < 3; i++)
		//		{
		//			CopyBase(i, mPlanetConverterComponentInfo->GetCloudResolution(i), m_Cloud);
		//			ModifySize(i, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_CloudHeight, m_Cloud);
		//			GenerateNormal(i, m_Cloud);
		//			GenerateTangent(i, m_Cloud);
		//		}
		//		AssembleMaterial(false, m_Cloud);
		//	}

		//	if (mPlanetConverterComponentInfo->m_EnableWater)
		//	{
		//		m_Water.Unload(this);

		//		for (size_t i = 0; i < 3; i++)
		//		{
		//			CopyBase(i, mPlanetConverterComponentInfo->GetWaterResolution(i), m_Water);
		//			ModifySize(i, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_WaterHeight, m_Water);
		//			GenerateNormal(i, m_Water);
		//			GenerateTangent(i, m_Water);
		//		}
		//		AssembleMaterial(false, m_Water);
		//	}
		//	CommonJobManager::instance()->PrepareJob();
		//	m_ticket = CommonJobManager::instance()->addRequestCommonTask(this, this);
		//	//CommonJobManager::instance()->addRequestCommonTask(this, this);
		//}
	}

	void PlanetConverterComponent::UpdatePlanetEnable()
	{
		PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		if (mPlanetConverterComponentInfo == nullptr)
			return;

		m_Base.Unload(this);
		if (m_pActComInfo == nullptr)
			return;

		if (!mPlanetConverterComponentInfo->m_UsePlanet)
		{
			return;
		}
		//if (mPlanetConverterComponentInfo->m_UseMesh)
		//{
		AssembleMaterial(false, m_Base);
		m_Base.LoadMesh(this, mPlanetConverterComponentInfo->m_Radius, Name(mPlanetConverterComponentInfo->m_MeshPath));
		//}
		//else
		//{
		//	if (m_Base.mesh[0].isNull())
		//	{
		//		for (size_t i = 0; i < 3; i++)
		//		{
		//			CopyBase(i, mPlanetConverterComponentInfo->GetResolution(i), m_Base);
		//			ModifySize(i, mPlanetConverterComponentInfo->m_Radius, m_Base);
		//			Displace(i, mPlanetConverterComponentInfo->m_HeightMapPath, mPlanetConverterComponentInfo->m_MaxHeight, m_Base);
		//			GenerateNormal(i, m_Base);
		//			GenerateTangent(i, m_Base);
		//			AssembleMesh(i, m_Base);
		//		}
		//		AssembleLod(m_Base);
		//	}
		//	m_Base.Load(this);
		//}
	}

	void PlanetConverterComponent::UpdateHeight()
	{
		PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		if (mPlanetConverterComponentInfo == nullptr)
			return;
		//if (mPlanetConverterComponentInfo->m_UseMesh)
		//{
		m_Base.Unload(this);

		if (!mPlanetConverterComponentInfo->m_UsePlanet)
			return;

		AssembleMaterial(false, m_Base);
		m_Base.LoadMesh(this, mPlanetConverterComponentInfo->m_Radius, Name(mPlanetConverterComponentInfo->m_MeshPath));
		//}
		//else
		//{
		//	if (m_Base.mesh[0].isNull())
		//	{
		//		return;
		//	}
		//	m_Base.Unload(this);

		//	if (m_pActComInfo == nullptr)
		//		return;

		//	if (!mPlanetConverterComponentInfo->m_UsePlanet)
		//		return;

		//	for (size_t i = 0; i < 3; i++)
		//	{
		//		CopyBase(i, mPlanetConverterComponentInfo->GetResolution(i), m_Base);
		//		ModifySize(i, mPlanetConverterComponentInfo->m_Radius, m_Base);
		//		Displace(i, mPlanetConverterComponentInfo->m_HeightMapPath, mPlanetConverterComponentInfo->m_MaxHeight, m_Base);
		//		GenerateNormal(i, m_Base);
		//		GenerateTangent(i, m_Base);
		//		AssembleMesh(i, m_Base);
		//	}

		//	m_Base.Load(this);
		//}
	}

	//void PlanetConverterComponent::UpdateLod(const size_t lod)
	//{
	//	if (m_Base.mesh[0].isNull())
	//	{
	//		return;
	//	}

	//	m_Base.Unload(this);

	//	if (m_pActComInfo == nullptr)
	//		return;

	//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
	//	if (mPlanetConverterComponentInfo == nullptr)
	//		return;

	//	if (!mPlanetConverterComponentInfo->m_UsePlanet)
	//		return;

	//	GenerateBase();
	//	CopyBase(lod, mPlanetConverterComponentInfo->GetResolution(lod), m_Base);
	//	ModifySize(lod, mPlanetConverterComponentInfo->m_Radius, m_Base);
	//	Displace(lod, mPlanetConverterComponentInfo->m_HeightMapPath, mPlanetConverterComponentInfo->m_MaxHeight, m_Base);
	//	GenerateNormal(lod, m_Base);
	//	GenerateTangent(lod, m_Base);
	//	AssembleMesh(lod, m_Base);
	//	m_Base.Load(this);
	//}

	void PlanetConverterComponent::UpdateMaterial()
	{
		PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		if (mPlanetConverterComponentInfo == nullptr)
			return;
		//if (mPlanetConverterComponentInfo->m_UseMesh)
		//{
		m_Base.Unload(this);
		if (mPlanetConverterComponentInfo->m_UsePlanet)
		{
			AssembleMaterial(true, m_Base);
			m_Base.LoadMesh(this, mPlanetConverterComponentInfo->m_Radius, Name(mPlanetConverterComponentInfo->m_MeshPath));
		}
		//}
		//else
		//{
		//	if (m_Base.mesh[0].isNull())
		//	{
		//		return;
		//	}

		//	m_Base.Unload(this);
		//	if (mPlanetConverterComponentInfo->m_UsePlanet)
		//	{
		//		AssembleMaterial(true, m_Base);
		//		m_Base.Load(this);
		//	}
		//}
	}

	void PlanetConverterComponent::UpdateAtmosphereEnable()
	{
		PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		if (mPlanetConverterComponentInfo == nullptr)
			return;

		m_Atmosphere.Unload(this);
		if (m_pActComInfo == nullptr)
			return;

		if (!mPlanetConverterComponentInfo->m_EnableAtmosphere)
		{
			return;
		}
		//if (mPlanetConverterComponentInfo->m_UseMesh)
		//{
		AssembleMaterial(false, m_Atmosphere);
		m_Atmosphere.LoadMesh(this, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_AtmosphereHeight, Name(mPlanetConverterComponentInfo->m_MeshPath));
		//}
		//else 
		//{
		//	if (m_Atmosphere.mesh[0].isNull())
		//	{
		//		for (size_t i = 0; i < 3; i++)
		//		{
		//			CopyBase(i, mPlanetConverterComponentInfo->GetAtmosphereResolution(i), m_Atmosphere);
		//			ModifySize(i, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_AtmosphereHeight, m_Atmosphere);
		//			GenerateNormal(i, m_Atmosphere);
		//			GenerateTangent(i, m_Atmosphere);
		//			AssembleMesh(i, m_Atmosphere);
		//		}
		//		AssembleLod(m_Atmosphere);
		//	}
		//	m_Atmosphere.Load(this);
		//}
	}

	void PlanetConverterComponent::UpdateAtmosphereHeight()
	{
		PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		if (mPlanetConverterComponentInfo == nullptr)
			return;
		//if (mPlanetConverterComponentInfo->m_EnableAtmosphere && mPlanetConverterComponentInfo->m_UseMesh)
		//{
		m_Atmosphere.Unload(this);
		m_Atmosphere.LoadMesh(this, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_AtmosphereHeight, Name(mPlanetConverterComponentInfo->m_MeshPath));
		//}
		//else
		//{
		//	if (m_Atmosphere.mesh[0].isNull())
		//	{
		//		return;
		//	}

		//	m_Atmosphere.Unload(this);

		//	if (m_pActComInfo == nullptr)
		//		return;


		//	for (size_t i = 0; i < 3; i++)
		//	{
		//		CopyBase(i, mPlanetConverterComponentInfo->GetAtmosphereResolution(i), m_Atmosphere);
		//		ModifySize(i, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_AtmosphereHeight, m_Atmosphere);
		//		GenerateNormal(i, m_Atmosphere);
		//		GenerateTangent(i, m_Atmosphere);
		//		AssembleMesh(i, m_Atmosphere);
		//	}

		//	if (mPlanetConverterComponentInfo->m_EnableAtmosphere)
		//	{
		//		m_Atmosphere.Load(this);
		//	}
		//}
	}

	//void PlanetConverterComponent::UpdateAtmosphereLod(const size_t lod)
	//{
	//	if (m_Atmosphere.mesh[0].isNull())
	//	{
	//		return;
	//	}
	//	
	//	m_Atmosphere.Unload(this);
	//	if (m_pActComInfo == nullptr)
	//		return;

	//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
	//	if (mPlanetConverterComponentInfo == nullptr)
	//		return;
	//	GenerateBase();
	//	CopyBase(lod, mPlanetConverterComponentInfo->GetAtmosphereResolution(lod), m_Atmosphere);
	//	ModifySize(lod, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_AtmosphereHeight, m_Atmosphere);
	//	GenerateNormal(lod, m_Atmosphere);
	//	GenerateTangent(lod, m_Atmosphere);
	//	AssembleMesh(lod, m_Atmosphere);

	//	if (mPlanetConverterComponentInfo->m_EnableAtmosphere)
	//	{
	//		m_Atmosphere.Load(this);
	//	}
	//}

	void PlanetConverterComponent::UpdateCloudEnable()
	{
		PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		if (mPlanetConverterComponentInfo == nullptr)
			return;
		m_Cloud.Unload(this);
		if (m_pActComInfo == nullptr)
			return;

		if (!mPlanetConverterComponentInfo->m_EnableCloud)
		{
			return;
		}
		//if (mPlanetConverterComponentInfo->m_UseMesh)
		//{
		AssembleMaterial(false, m_Cloud);
		m_Cloud.LoadMesh(this, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_CloudHeight, Name(mPlanetConverterComponentInfo->m_MeshPath));
		//}
		//else
		//{
		//	if (m_Cloud.mesh[0].isNull())
		//	{
		//		for (size_t i = 0; i < 3; i++)
		//		{
		//			CopyBase(i, mPlanetConverterComponentInfo->GetCloudResolution(i), m_Cloud);
		//			ModifySize(i, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_CloudHeight, m_Cloud);
		//			GenerateNormal(i, m_Cloud);
		//			GenerateTangent(i, m_Cloud);
		//			AssembleMesh(i, m_Cloud);
		//		}
		//		AssembleLod(m_Cloud);
		//	}
		//	m_Cloud.Load(this);
		//}
	}

	void PlanetConverterComponent::UpdateCloudHeight()
	{
		PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		if (mPlanetConverterComponentInfo == nullptr)
			return;
		//if (mPlanetConverterComponentInfo->m_EnableCloud && mPlanetConverterComponentInfo->m_UseMesh)
		//{
		m_Cloud.Unload(this);
		m_Cloud.LoadMesh(this, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_CloudHeight, Name(mPlanetConverterComponentInfo->m_MeshPath));
		//}
		//else
		//{
		//	if (m_Cloud.mesh[0].isNull())
		//	{
		//		return;
		//	}

		//	m_Cloud.Unload(this);
		//	if (m_pActComInfo == nullptr)
		//		return;
		//	for (size_t i = 0; i < 3; i++)
		//	{
		//		CopyBase(i, mPlanetConverterComponentInfo->GetCloudResolution(i), m_Cloud);
		//		ModifySize(i, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_CloudHeight, m_Cloud);
		//		GenerateNormal(i, m_Cloud);
		//		GenerateTangent(i, m_Cloud);
		//		AssembleMesh(i, m_Cloud);
		//	}

		//	if (mPlanetConverterComponentInfo->m_EnableCloud)
		//	{
		//		m_Cloud.Load(this);
		//	}
		//}
	}

	//void PlanetConverterComponent::UpdateCloudLod(const size_t lod)
	//{
	//	if (m_Cloud.mesh[0].isNull())
	//	{
	//		return;
	//	}
	//	
	//	m_Cloud.Unload(this);
	//	if (m_pActComInfo == nullptr)
	//		return;

	//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
	//	if (mPlanetConverterComponentInfo == nullptr)
	//		return;
	//	GenerateBase();
	//	CopyBase(lod, mPlanetConverterComponentInfo->GetCloudResolution(lod), m_Cloud);
	//	ModifySize(lod, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_CloudHeight, m_Cloud);
	//	GenerateNormal(lod, m_Cloud);
	//	GenerateTangent(lod, m_Cloud);
	//	AssembleMesh(lod, m_Cloud);

	//	if (mPlanetConverterComponentInfo->m_EnableCloud)
	//	{
	//		m_Cloud.Load(this);
	//	}
	//}

	void PlanetConverterComponent::UpdateCloudMaterial()
	{
		PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		if (mPlanetConverterComponentInfo == nullptr)
			return; 
		//if (mPlanetConverterComponentInfo->m_UseMesh)
		//{
		m_Cloud.Unload(this);
		AssembleMaterial(true, m_Cloud);
		m_Cloud.LoadMesh(this, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_CloudHeight, Name(mPlanetConverterComponentInfo->m_MeshPath));
		//}
		//else
		//{
		//	if (m_Cloud.mesh[0].isNull())
		//	{
		//		return;
		//	}

		//	m_Cloud.Unload(this);
		//	AssembleMaterial(true, m_Cloud);
		//	m_Cloud.Load(this);
		//}
	}

	void PlanetConverterComponent::UpdateWaterEnable()
	{
		PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		if (mPlanetConverterComponentInfo == nullptr)
			return;

		m_Water.Unload(this);

		if (m_pActComInfo == nullptr)
			return;

		if (!mPlanetConverterComponentInfo->m_EnableWater)
		{
			return;
		}

		//if (mPlanetConverterComponentInfo->m_UseMesh)
		//{
		AssembleMaterial(false, m_Water);
		m_Water.LoadMesh(this, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_WaterHeight, Name(mPlanetConverterComponentInfo->m_MeshPath));
		//}
		//else
		//{
		//	if (m_Water.mesh[0].isNull())
		//	{
		//		for (size_t i = 0; i < 3; i++)
		//		{
		//			CopyBase(i, mPlanetConverterComponentInfo->GetWaterResolution(i), m_Water);
		//			ModifySize(i, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_WaterHeight, m_Water);
		//			GenerateNormal(i, m_Water);
		//			GenerateTangent(i, m_Water);
		//			AssembleMesh(i, m_Water);
		//		}
		//		AssembleLod(m_Water);
		//	}
		//	m_Water.Load(this);
		//}
	}

	void PlanetConverterComponent::UpdateWaterHeight()
	{
		PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		if (mPlanetConverterComponentInfo == nullptr)
			return;
		//if (mPlanetConverterComponentInfo->m_EnableWater && mPlanetConverterComponentInfo->m_UseMesh)
		//{
		m_Water.Unload(this);
		m_Water.LoadMesh(this, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_WaterHeight, Name(mPlanetConverterComponentInfo->m_MeshPath));
		//}
		//else
		//{
		//	if (m_Water.mesh[0].isNull())
		//	{
		//		return;
		//	}

		//	m_Water.Unload(this);
		//	if (m_pActComInfo == nullptr)
		//		return;


		//	for (size_t i = 0; i < 3; i++)
		//	{
		//		CopyBase(i, mPlanetConverterComponentInfo->GetWaterResolution(i), m_Water);
		//		ModifySize(i, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_WaterHeight, m_Water);
		//		GenerateNormal(i, m_Water);
		//		GenerateTangent(i, m_Water);
		//		AssembleMesh(i, m_Water);
		//	}
		//	if (mPlanetConverterComponentInfo->m_EnableWater)
		//		m_Water.Load(this);
		//}
	}

	//void PlanetConverterComponent::UpdateWaterLod(const size_t lod)
	//{
	//	if (m_Water.mesh[0].isNull())
	//	{
	//		return;
	//	}
	//	
	//	m_Water.Unload(this);

	//	if (m_pActComInfo == nullptr)
	//		return;

	//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
	//	if (mPlanetConverterComponentInfo == nullptr)
	//		return;

	//	GenerateBase();
	//	CopyBase(lod, mPlanetConverterComponentInfo->GetWaterResolution(lod), m_Water);
	//	ModifySize(lod, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_WaterHeight, m_Water);
	//	GenerateNormal(lod, m_Water);
	//	GenerateTangent(lod, m_Water);
	//	AssembleMesh(lod, m_Water);

	//	if (mPlanetConverterComponentInfo->m_EnableWater)
	//	{
	//		m_Water.Load(this);
	//	}
	//}

	void PlanetConverterComponent::UpdateWaterMaterial()
	{
		PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		if (mPlanetConverterComponentInfo == nullptr)
			return;
		//if (mPlanetConverterComponentInfo->m_UseMesh)
		//{
		m_Water.Unload(this);
		AssembleMaterial(true, m_Water);
		m_Water.LoadMesh(this, mPlanetConverterComponentInfo->m_Radius + mPlanetConverterComponentInfo->m_WaterHeight, Name(mPlanetConverterComponentInfo->m_MeshPath));
		//}
		//else
		//{
		//	if (m_Water.mesh[0].isNull())
		//	{
		//		return;
		//	}

		//	m_Water.Unload(this);
		//	AssembleMaterial(true, m_Water);
		//	m_Water.Load(this);
		//}
	}

	void PlanetConverterComponent::UpdateEntityParam()
	{
		m_Base.UpdateEntity(this);
		m_Atmosphere.UpdateEntity(this);
		m_Cloud.UpdateEntity(this);
		m_Water.UpdateEntity(this);
	}

	void PlanetConverterComponent::PlanetSphere::Unload(PlanetConverterComponent *owner)
	{
		if (entity)
		{
			entity->detachFromParent();
			owner->getSceneManager()->destroyEntity(entity);
			entity = nullptr;
		}
		if (m_pSceneNode != nullptr)
		{
			owner->getSceneManager()->destroySceneNode(m_pSceneNode);
		}
		m_pSceneNode = nullptr;
	}
	void PlanetConverterComponent::PlanetSphere::Load(PlanetConverterComponent *owner)
	{
		Name entityName;
		if (Echo::Root::instance()->getRuntimeMode() == Root::eRM_Editor)
			entityName = owner->getCombinedName().append("||").append("Entity").append(entitySuffix);
		entity = owner->getSceneManager()->createEntity(entityName, mesh[0]);
		if (!entity)
			return;
		UpdateEntity(owner);
		owner->attachToNode(entity);
		owner->UpdateComRootNode();
	}

	void PlanetConverterComponent::PlanetSphere::LoadMesh(PlanetConverterComponent* owner, float radius, Name meshPath)
	{
		Name entityName;
		if (Echo::Root::instance()->getRuntimeMode() == Root::eRM_Editor)
			entityName = owner->getCombinedName().append("||").append("Entity").append(entitySuffix);
		entity = owner->getSceneManager()->createEntity(entityName, meshPath);
		if (!entity)
			return;
		entity->setCustomMaterial(material);

		UpdateEntity(owner);

		m_pSceneManager = owner->getSceneManager();
		m_pSceneNode = m_pSceneManager->createSceneNode();
		m_pSceneNode->setUseQuadTree(true);
		m_pSceneNode->setScale(radius, radius, radius);
		m_pSceneNode->attachObject(entity);
		owner->addChildToNode(m_pSceneNode);
		owner->UpdateComRootNode(true);
		m_pSceneNode->setVisible(true);
		entity->setVisible(true);
	}

	void PlanetConverterComponent::PlanetSphere::UpdateEntity(PlanetConverterComponent *owner)
	{
		if (!entity)
			return;
#ifdef ECHO_EDITOR
		entity->setQueryFlags(owner->m_QtFlags);
#endif

		if (owner->m_pActComInfo == nullptr)
			return;

		PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(owner->m_pActComInfo);
		if (mPlanetConverterComponentInfo == nullptr)
			return;

		entity->setCameraCullMode(static_cast<CameraCullMode>(mPlanetConverterComponentInfo->m_CullMode));
		entity->setRenderingDistance(mPlanetConverterComponentInfo->m_CullDistance);
		entity->setRenderingDistanceForTOD(mPlanetConverterComponentInfo->m_TodCullDistance);
		entity->setStippleTransparency(mPlanetConverterComponentInfo->m_DissolveStrength);
		entity->setFPState(static_cast<ENUM_FP_STATE>(mPlanetConverterComponentInfo->m_FpsState));
		entity->setLodDis(LOD_LEVEL_1, mPlanetConverterComponentInfo->m_LodDis_Level1);
		entity->setLodDis(LOD_LEVEL_2, mPlanetConverterComponentInfo->m_LodDis_Level2);
		entity->enableCastShadow(mPlanetConverterComponentInfo->m_CastShadow);
		if(id == 1)
			entity->setRenderPriority(10);
		else if(id == 2)
			entity->setRenderPriority(20);
		else if(id == 3)
			entity->setRenderPriority(30);
	}

	void PlanetConverterComponent::PlanetSphere::InitParam(PlanetConverterComponent *owner)
	{
		if (!materialParam.empty())
		{
			return;
		}

		SShaderFXParams& fxParams = CShaderMan::getSingleton().GetShaderManBin().mfGetFXParams(material->m_pShader);
		// MatInfo& matInfo = material->m_matInfo;

		for (size_t i = 0; i < fxParams.m_FXParams.size(); i++)
		{
			SFXParam& fxParam = fxParams.m_FXParams[i];
			if ((fxParam.m_nFlags & PF_TWEAKABLE_MASK) == 0)
			{
				continue;
			}

			float felement[4] = { 0.0,0.0,0.0,0.0 };
			sscanf(fxParam.m_Values.c_str(), "%f,%f,%f,%f", &felement[0], &felement[1], &felement[2], &felement[3]);
			float max = 1.0f;
			float min = 0.0f;
			float step = 0.1f;

			String element[3];
			int pItem = 0;
			String itemToken = StringUtil::Tokenize(fxParam.m_Annotations.c_str(), ";", pItem);
			while (!itemToken.empty())
			{
				int nElements = 0;
				int pElement = 0;
				String token = StringUtil::Tokenize(itemToken.c_str(), " \t\r\n=", pElement);
				while (!token.empty())
				{
					element[nElements++] = token;
					if (nElements == 2)
					{
						if (pElement < itemToken.size())
						{
							element[nElements] = itemToken.substr(pElement);
						}
						break;
					}
					token = StringUtil::Tokenize(itemToken.c_str(), " \t\r\n=", pElement);
				}

				if (element[1] == "UIMin")
				{
					min = (float)atof(element[2].c_str());
				}
				else if (element[1] == "UIMax")
				{
					max = (float)atof(element[2].c_str());
				}
				else if (element[1] == "UIStep")
				{
					step = (float)atof(element[2].c_str());
				}

				itemToken = Echo::StringUtil::Tokenize(fxParam.m_Annotations.c_str(), ";", pItem);
			}

			for (size_t shaderIdx = 0; shaderIdx < Shader_Type_Cnt; shaderIdx++)
			{
				short uniformIdx = fxParam.m_nUniformIdx[shaderIdx];
				if (uniformIdx == -1)
				{
					continue;
				}

				for (size_t j = 0; j < 3; j++)
				{
					short paramIdx = fxParam.m_nParamIdx[shaderIdx][j];
					if (paramIdx == -1)
					{
						continue;
					}

					MaterialParam p;
					p.min = min;
					p.max = max;
					p.step = step;
					p.type = fxParam.m_eType;
					p.name = fxParam.m_Name;
					// 似乎PF_TWEAKABLE_1标记就是颜色标记，其它所有的都是float单值。参考别的编辑器代码，未考证。
					p.isColor = (fxParam.m_nFlags & PF_TWEAKABLE_1) != 0;
					p.param.uniformIndex = static_cast<uint8>(uniformIdx);
					p.param.paramIndex = static_cast<uint8>(paramIdx);
					p.onlyId = GetParamOnlyId(id, static_cast<uint8>(shaderIdx), p.param.uniformIndex, p.param.paramIndex);
					// auto iter = owner->mPlanetConverterComponentInfo->m_matParam.find(p.onlyId);
					// p.param.paramVal = iter == owner->mPlanetConverterComponentInfo->m_matParam.end()? felement[j] : iter->second;
					// // 材质要的数据
					// SHADER_PARAMS& params = shaderIdx == Shader_Type_VS? matInfo.vsShaderParams : matInfo.psShaderParams;
					// params.push_back(p.param);
					// 自己的值不重要，干脆存个初始吧
					p.param.paramVal = felement[j];
					// 自己存一份数据
					materialParam.push_back(p);
				}
			}
		}
	}

	void PlanetConverterComponent::PlanetSphere::UpdateParam(PlanetConverterComponent *owner)
	{
		// std::set<uint32> set;

		for (auto param = material->m_matInfo.vsShaderParams.begin(); param != material->m_matInfo.vsShaderParams.end(); ++param)
		{
			//auto c = material->m_matInfo.vsShaderParams.end();
			uint32 onlyId = GetParamOnlyId(id, Shader_Type_VS, param->uniformIndex, param->paramIndex);
			if (owner->m_pActComInfo == nullptr)
				return;

			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(owner->m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			auto iter = mPlanetConverterComponentInfo->m_matParam.find(onlyId);
			if (iter != mPlanetConverterComponentInfo->m_matParam.end())
			{
				param->paramVal = iter->second;
				// set.insert(onlyId);
				// ++param;
			}
			else
			{
				// param = material->m_matInfo.vsShaderParams.erase(param);

				for (const MaterialParam& selfData: materialParam)
				{
					if (selfData.onlyId == onlyId)
					{
						param->paramVal = selfData.param.paramVal;
						break;
					}
				}
			}
		}
		for (auto param = material->m_matInfo.psShaderParams.begin(); param != material->m_matInfo.psShaderParams.end(); ++param)
		{
			if (owner->m_pActComInfo == nullptr)
				return;

			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(owner->m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;

			uint32 onlyId = GetParamOnlyId(id, Shader_Type_PS, param->uniformIndex, param->paramIndex);
			auto iter = mPlanetConverterComponentInfo->m_matParam.find(onlyId);
			if (iter != mPlanetConverterComponentInfo->m_matParam.end())
			{
				param->paramVal = iter->second;
				// set.insert(onlyId);
				// ++param;
			}
			else
			{
				// param = material->m_matInfo.psShaderParams.erase(param);

				for (const MaterialParam& selfData: materialParam)
				{
					if (selfData.onlyId == onlyId)
					{
						param->paramVal = selfData.param.paramVal;
						break;
					}
				}
			}
		}
		// for (auto pair: owner->mPlanetConverterComponentInfo->m_matParam)
		// {
		// 	if (set.find(pair.first) != set.end())
		// 	{
		// 		continue;
		// 	}
		// 	uint8 pIndex = static_cast<uint8>(pair.first & static_cast<uint32>(~uint8(0)));
		// 	uint8 uIndex = static_cast<uint8>((pair.first & (static_cast<uint32>(~uint8(0) << 8))) >> 8);
		// 	st_UniformParam param;
		// 	param.paramIndex = pIndex;
		// 	param.uniformIndex = uIndex;
		// 	param.paramVal = pair.second;
		// }
		material->m_matInfo.updateVsShaderParams();
		material->m_matInfo.updatePsShaderParams();
	}

	void PlanetConverterComponent::ModifySize(size_t lod, const float radius, PlanetSphere& sphere)
	{
		for (size_t i = 0; i < m_Vertices[sphere.id][lod].size(); i++)
		{
			m_Vertices[sphere.id][lod][i] = m_Vertices[sphere.id][lod][i] * radius;
		}
	}

	//void PlanetConverterComponent::CopyBase(size_t lod, const size_t level, PlanetSphere &sphere)
	//{
	//	ECHO_LOCK_MUTEX(g_BaseLock)
	//	m_Vertices[sphere.id][lod] = g_VerticesBaseWithUV[level];
	//	m_Triangles[sphere.id][lod] = g_TrianglesBaseWithUV[level];
	//	m_UVs[sphere.id][lod] = g_UVsBaseWithUV[level];
	//}

	//void PlanetConverterComponent::AssembleMesh(const size_t lod, PlanetSphere &sphere)
	//{
	//	AssembleMaterial(false, sphere);
	//	SmartPtr<Mesh> mesh = sphere.mesh[lod];
	//	if (mesh.isNull())
	//	{
	//		Name name;
	//		if (lod != 0)
	//		{
	//			name = MeshMgr::msNameGenerator.generate().append("_lod");
	//		}
	//		mesh = MeshMgr::instance()->createManual(name);
	//		sphere.mesh[lod] = mesh;
	//	}
	//	SubMesh* subMesh = mesh->getSubMesh(0);
	//	if (!subMesh)
	//	{
	//		subMesh = mesh->createSubMesh();
	//	}
	//	subMesh->setMaterial(sphere.material);
	//	 
	//	uint32 elementSize = sphere.material->m_desc.IADesc.StreamArray[0].nStride;
	//	memcpy(&subMesh->m_desc.IADesc, &sphere.material->m_desc.IADesc, sizeof(subMesh->m_desc.IADesc));

	//	const size_t indexSize = m_Triangles[sphere.id][lod].size();
	//	MemoryDataStreamPtr indexData(new MemoryDataStream(indexSize * sizeof(uint16)));
	//	memcpy(indexData->getCurrentPtr(), m_Triangles[sphere.id][lod].data(), indexSize * sizeof(uint16));
	//	subMesh->getIndexData()->clearCpuStreamData();
	//	subMesh->getIndexData()->setCpuStreamData(indexData->getCurrentPtr(), indexData);
	//	subMesh->getIndexData()->m_indexCount = static_cast<uint32>(indexSize);

	//	const size_t vertexSize = m_Vertices[sphere.id][lod].size();
	//	MemoryDataStreamPtr vertexData(new MemoryDataStream(vertexSize * elementSize));

	//	subMesh->getVertexData()->clearCpuStreamData();
	//	subMesh->getVertexData()->addCpuStreamData(vertexData->getCurrentPtr(), vertexData, 0, elementSize);
	//	subMesh->getVertexData()->m_vertexCount = static_cast<uint32>(vertexSize);

	//	float* vertexBufferPtr = reinterpret_cast<float*>(vertexData->getCurrentPtr());
	//	for (size_t i = 0; i < vertexSize; i++)
	//	{
	//		*vertexBufferPtr++ = m_Vertices[sphere.id][lod][i].x;
	//		*vertexBufferPtr++ = m_Vertices[sphere.id][lod][i].y;
	//		*vertexBufferPtr++ = m_Vertices[sphere.id][lod][i].z;
	//		*vertexBufferPtr++ = m_Normals[sphere.id][lod][i].x;
	//		*vertexBufferPtr++ = m_Normals[sphere.id][lod][i].y;
	//		*vertexBufferPtr++ = m_Normals[sphere.id][lod][i].z;
	//		*vertexBufferPtr++ = m_Tangents[sphere.id][lod][i].x;
	//		*vertexBufferPtr++ = m_Tangents[sphere.id][lod][i].y;
	//		*vertexBufferPtr++ = m_Tangents[sphere.id][lod][i].z;
	//		*vertexBufferPtr++ = m_Tangents[sphere.id][lod][i].w;
	//		*vertexBufferPtr++ = m_UVs[sphere.id][lod][i].x;
	//		*vertexBufferPtr++ = m_UVs[sphere.id][lod][i].y;
	//	}
	//	
	//	RenderSystem* renderSystem = Root::instance()->getRenderSystem();
	//	RenderOperation* renderOperation = subMesh->_getRenderOperation();
	//	const size_t indexBufferSize = indexSize * sizeof(uint16);
	//	renderOperation->GetMeshData().Release();
	//	renderOperation->GetMeshData().m_IndexBufferData.m_indexCount = static_cast<uint32>(indexSize);
	//	renderOperation->GetMeshData().m_IndexBufferData.m_indexBuffer = renderSystem->createStaticIB(m_Triangles[sphere.id][lod].data(), static_cast<uint32>(indexBufferSize));
	//	renderOperation->GetMeshData().m_indexBufferSize = static_cast<uint32>(indexBufferSize);
	//	renderOperation->GetMeshData().m_vertexCount = static_cast<uint32>(m_Vertices[sphere.id][lod].size());
	//	renderOperation->GetMeshData().m_vertexBuffSize = elementSize * static_cast<uint32>(m_Vertices[sphere.id][lod].size());
	//	renderOperation->GetMeshData().m_vertexBuffer.push_back(renderSystem->createStaticVB(vertexData->getPtr(), elementSize * static_cast<uint32>(m_Vertices[sphere.id][lod].size())));
	//	
	//	Vector3 boxMin = Vector3::ZERO;
	//	Vector3 boxMax = Vector3::ZERO;
	//	for (size_t i = 0; i < vertexSize; i++)
	//	{
	//		boxMax.makeCeil(m_Vertices[sphere.id][lod][i]);
	//		boxMin.makeFloor(m_Vertices[sphere.id][lod][i]);
	//	}
	//	mesh->setAABB(boxMin, boxMax);
	//	mesh->prepare();
	//	mesh->load();
	//}

	//void PlanetConverterComponent::AssembleVertexBuffer(PlanetSphere& sphere)
	//{
	//	for (int lod = 0; lod < LOD_LEVEL_NUM; ++lod)
	//	{
	//		uint32 elementSize = sphere.material->m_desc.IADesc.StreamArray[0].nStride;

	//		const size_t vertexSize = m_Vertices[sphere.id][lod].size();
	//		MemoryDataStreamPtr vertexData(new MemoryDataStream(vertexSize * elementSize));

	//		float* vertexBufferPtr = reinterpret_cast<float*>(vertexData->getCurrentPtr());
	//		for (size_t i = 0; i < vertexSize; i++)
	//		{
	//			*vertexBufferPtr++ = m_Vertices[sphere.id][lod][i].x;
	//			*vertexBufferPtr++ = m_Vertices[sphere.id][lod][i].y;
	//			*vertexBufferPtr++ = m_Vertices[sphere.id][lod][i].z;
	//			*vertexBufferPtr++ = m_Normals[sphere.id][lod][i].x;
	//			*vertexBufferPtr++ = m_Normals[sphere.id][lod][i].y;
	//			*vertexBufferPtr++ = m_Normals[sphere.id][lod][i].z;
	//			*vertexBufferPtr++ = m_Tangents[sphere.id][lod][i].x;
	//			*vertexBufferPtr++ = m_Tangents[sphere.id][lod][i].y;
	//			*vertexBufferPtr++ = m_Tangents[sphere.id][lod][i].z;
	//			*vertexBufferPtr++ = m_Tangents[sphere.id][lod][i].w;
	//			*vertexBufferPtr++ = m_UVs[sphere.id][lod][i].x;
	//			*vertexBufferPtr++ = m_UVs[sphere.id][lod][i].y;
	//		}
	//		m_VertexDataPtr[sphere.id][lod] = vertexData;
	//	}
	//}
	//void PlanetConverterComponent::AssembleMeshData(PlanetSphere& sphere)
	//{
	//	for (int lod = 0; lod < LOD_LEVEL_NUM; ++lod)
	//	{
	//		SmartPtr<Mesh> mesh = sphere.mesh[lod];
	//		if (mesh.isNull())
	//		{
	//			Name name;
	//			if (lod != 0)
	//			{
	//				name = MeshMgr::msNameGenerator.generate().append("_lod");
	//			}
	//			mesh = MeshMgr::instance()->createManual(name);
	//			sphere.mesh[lod] = mesh;
	//		}
	//		SubMesh* subMesh = mesh->getSubMesh(0);
	//		if (!subMesh)
	//		{
	//			subMesh = mesh->createSubMesh();
	//		}
	//		subMesh->setMaterial(sphere.material);

	//		uint32 elementSize = sphere.material->m_desc.IADesc.StreamArray[0].nStride;
	//		memcpy(&subMesh->m_desc.IADesc, &sphere.material->m_desc.IADesc, sizeof(subMesh->m_desc.IADesc));

	//		const size_t indexSize = m_Triangles[sphere.id][lod].size();
	//		MemoryDataStreamPtr indexData(new MemoryDataStream(indexSize * sizeof(uint16)));
	//		memcpy(indexData->getCurrentPtr(), m_Triangles[sphere.id][lod].data(), indexSize * sizeof(uint16));
	//		subMesh->getIndexData()->clearCpuStreamData();
	//		subMesh->getIndexData()->setCpuStreamData(indexData->getCurrentPtr(), indexData);
	//		subMesh->getIndexData()->m_indexCount = static_cast<uint32>(indexSize);

	//		const size_t vertexSize = m_Vertices[sphere.id][lod].size();
	//		subMesh->getVertexData()->clearCpuStreamData();
	//		subMesh->getVertexData()->addCpuStreamData(m_VertexDataPtr[sphere.id][lod]->getCurrentPtr(), m_VertexDataPtr[sphere.id][lod], 0, elementSize);
	//		subMesh->getVertexData()->m_vertexCount = static_cast<uint32>(vertexSize);

	//		RenderSystem* renderSystem = Root::instance()->getRenderSystem();
	//		RenderOperation* renderOperation = subMesh->_getRenderOperation();

	//		const size_t indexBufferSize = indexSize * sizeof(uint16);
	//		renderOperation->GetMeshData().Release();
	//		renderOperation->GetMeshData().m_IndexBufferData.m_indexCount = static_cast<uint32>(indexSize);
	//		renderOperation->GetMeshData().m_IndexBufferData.m_indexBuffer = renderSystem->createStaticIB(m_Triangles[sphere.id][lod].data(), static_cast<uint32>(indexBufferSize));
	//		renderOperation->GetMeshData().m_indexBufferSize = static_cast<uint32>(indexBufferSize);
	//		renderOperation->GetMeshData().m_vertexCount = static_cast<uint32>(m_Vertices[sphere.id][lod].size());
	//		renderOperation->GetMeshData().m_vertexBuffSize = elementSize * static_cast<uint32>(m_Vertices[sphere.id][lod].size());
	//		renderOperation->GetMeshData().m_vertexBuffer.push_back(renderSystem->createStaticVB(m_VertexDataPtr[sphere.id][lod]->getPtr(), elementSize * static_cast<uint32>(m_Vertices[sphere.id][lod].size())));

	//		Vector3 boxMin = Vector3::ZERO;
	//		Vector3 boxMax = Vector3::ZERO;
	//		for (size_t i = 0; i < vertexSize; i++)
	//		{
	//			boxMax.makeCeil(m_Vertices[sphere.id][lod][i]);
	//			boxMin.makeFloor(m_Vertices[sphere.id][lod][i]);
	//		}
	//		mesh->setAABB(boxMin, boxMax);
	//		mesh->prepare();
	//		mesh->load();
	//	}
	//	AssembleLod(sphere);
	//	sphere.Load(this);
	//}

	//void PlanetConverterComponent::AssembleLod(PlanetSphere &sphere)
	//{
	//	if (sphere.mesh[0].isNull()
	//		|| sphere.mesh[1].isNull()
	//		|| sphere.mesh[2].isNull()
	//	) {
	//		return;
	//	}

	//	if (sphere.mesh[0]->m_bUseLod)
	//	{
	//		return;
	//	}
	//	sphere.mesh[0]->m_bUseLod = true;
	//	sphere.mesh[0]->m_vtLodMesh.push_back(sphere.mesh[1]);
	//	sphere.mesh[0]->m_vtLodMesh.push_back(sphere.mesh[2]);
	//}

	void PlanetConverterComponent::AssembleMaterial(const bool refresh, PlanetSphere &sphere)
	{
		if (sphere.material.isNull())
		{
			sphere.material = MaterialManager::instance()->createManual();
		}
		else if (!refresh)
		{
			return;
		}

		uint32 elementSize = 0;
		sphere.material->m_desc.IADesc.ElementArray[0].nStream = 0;
		sphere.material->m_desc.IADesc.ElementArray[0].eType = ECHO_FLOAT3;
		sphere.material->m_desc.IADesc.ElementArray[0].Semantics = ECHO_POSITION0;
		sphere.material->m_desc.IADesc.ElementArray[0].nOffset = elementSize;
		elementSize += 3 * sizeof(float);
		sphere.material->m_desc.IADesc.ElementArray[1].nStream = 0;
		sphere.material->m_desc.IADesc.ElementArray[1].eType = ECHO_FLOAT3;
		sphere.material->m_desc.IADesc.ElementArray[1].Semantics = ECHO_NORMAL0;
		sphere.material->m_desc.IADesc.ElementArray[1].nOffset = elementSize;
		elementSize += 3 * sizeof(float);
		sphere.material->m_desc.IADesc.ElementArray[2].nStream = 0;
		sphere.material->m_desc.IADesc.ElementArray[2].eType = ECHO_FLOAT4;
		sphere.material->m_desc.IADesc.ElementArray[2].Semantics = ECHO_TANGENT0;
		sphere.material->m_desc.IADesc.ElementArray[2].nOffset = elementSize;
		elementSize += 4 * sizeof(float);
		sphere.material->m_desc.IADesc.ElementArray[3].nStream = 0;
		sphere.material->m_desc.IADesc.ElementArray[3].eType = ECHO_FLOAT2;
		sphere.material->m_desc.IADesc.ElementArray[3].Semantics = ECHO_TEXCOORD0;
		sphere.material->m_desc.IADesc.ElementArray[3].nOffset = elementSize;
		elementSize += 2 * sizeof(float);

		sphere.material->m_desc.IADesc.nElementCount = 4;
		sphere.material->m_desc.IADesc.nStreamCount = 1;
		sphere.material->m_desc.IADesc.StreamArray[0].nStride = elementSize;

		sphere.material->m_matInfo.shadername = sphere.shadername;
		if (sphere.genmask)
		{
			sphere.material->m_matInfo.genmask = sphere.genmask;
		}
		if (sphere.statemask)
		{
			sphere.material->m_matInfo.statemask = sphere.statemask;
		}
		for (auto& pair: sphere.tex)
		{
			sphere.material->m_matInfo.textures[pair.first] = pair.second;
		}
		for (auto& pair : sphere.normal)
		{
			sphere.material->m_matInfo.textures[pair.first] = pair.second;
		}

		sphere.material->updateStateMask(); // 整理写到genmask里的statemask?没用
		sphere.material->updateShaderMaskAndShaderParam();
		sphere.InitParam(this);
		sphere.UpdateParam(this);
		sphere.material->rebuildMaterial();
	}

	//void PlanetConverterComponent::GenerateBase()
	//{
	//	ECHO_LOCK_MUTEX(g_BaseLock)

	//	if (m_pActComInfo == nullptr)
	//			return;

	//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
	//	if (mPlanetConverterComponentInfo == nullptr)
	//		return;

	//	for (size_t i = 0; i < 3; i++)
	//	{
	//		GenerateBase(mPlanetConverterComponentInfo->GetResolution(i));
	//	}
	//	for (size_t i = 0; i < 3; i++)
	//	{
	//		GenerateBase(mPlanetConverterComponentInfo->GetAtmosphereResolution(i));
	//	}
	//	for (size_t i = 0; i < 3; i++)
	//	{
	//		GenerateBase(mPlanetConverterComponentInfo->GetCloudResolution(i));
	//	}
	//	for (size_t i = 0; i < 3; i++)
	//	{
	//		GenerateBase(mPlanetConverterComponentInfo->GetWaterResolution(i));
	//	}
	//}

	//void PlanetConverterComponent::GenerateBase(const size_t level)
	//{

	//	assert(g_VerticesBaseWithUV.size() == g_TrianglesBaseWithUV.size());
	//	for (size_t currentLevel = g_VerticesBaseWithUV.size(); currentLevel <= level; currentLevel++)
	//	{
	//		std::vector<Vector3> vertices;
	//		std::vector<uint16> triangles;
	//		std::vector<Vector2>uvs;
	//		int tessellation = 4*int(std::pow(2.0,currentLevel));
	//		//constexpr float diameter = 20;
	//		if (tessellation < 3 || tessellation % 2 != 0)
	//			throw std::invalid_argument("tessellation parameter must be at least 3, and even.");

	//		size_t verticalSegments = tessellation;
	//		size_t quadSegments = verticalSegments / 2;
	//		size_t horizontalVertices = (quadSegments + 1) * 4;

	//		//constexpr float radius = diameter / 2;
	//		for (size_t i = 0; i <= verticalSegments; i++)
	//		{
	//			// const float v = 1 - float(i) / float(verticalSegments);

	//			const float latitude = (float(i) / verticalSegments - 0.5f) * Math::PI;
	//			float dy, dxz;

	//			//XMScalarSinCos(&dy, &dxz, latitude);
	//			dy = Math::Sin(latitude);
	//			dxz = Math::Cos(latitude);
	//			// Create a single ring of vertices at this latitude.
	//			for (int k = 0; k < 4; ++k)
	//			{
	//				for (size_t j = 0; j <= quadSegments; j++)
	//				{
	//					// const float u = float(j) / float(horizontalSegments);
	//					const float longitude = (float(j) / quadSegments + k) * Math::HALF_PI;
	//					float dx, dz;

	//					//XMScalarSinCos(&dx, &dz, longitude);
	//					dx = Math::Sin(longitude);
	//					dz = Math::Cos(longitude);
	//					dx *= dxz;
	//					dz *= dxz;

	//					const float r = 1.0f - abs(2.0f * i / verticalSegments - 1.f);
	//					float v = r * j / quadSegments;
	//					float u = r - v;
	//					if (latitude < 0)
	//					{
	//						const float uu = 1 - v;
	//						const float vv = 1 - u;
	//						u = uu;
	//						v = vv;
	//					}

	//					if (k == 1)
	//					{
	//						const float uu = -v;
	//						const float vv = u;
	//						u = uu;
	//						v = vv;
	//					}
	//					else if (k == 2)
	//					{
	//						u = -u;
	//						v = -v;
	//					}
	//					else if (k == 3)
	//					{
	//						const float uu = v;
	//						const float vv = -u;
	//						u = uu;
	//						v = vv;
	//					}

	//					u = u * 0.5f + 0.5f;
	//					v = -v * 0.5f + 0.5f; // flip v to map from RH to LH texcoords

	//					//const XMVECTOR normal = XMVectorSet(dx, dy, dz, 0);
	//					//const XMVECTOR textureCoordinate = XMVectorSet(u, v, 0, 0);

	//					vertices.emplace_back(Vector3(dx, dy, dz));
	//					uvs.emplace_back(Vector2(u, v));
	//				}
	//			}
	//		}

	//		//
	//		// Fill the index buffer with triangles joining each pair of latitude rings.
	//		size_t stride = horizontalVertices;

	//		for (size_t i = 0; i < verticalSegments; i++)
	//		{
	//			for (int k = 0; k < 4; ++k)
	//			{
	//				for (size_t j = k * (quadSegments + 1); j < k * (quadSegments + 1) + quadSegments; j++)
	//				{
	//					const size_t nextI = i + 1;
	//					const size_t nextJ = j + 1;

	//					triangles.push_back(uint16(i * stride + j));
	//					triangles.push_back(uint16(i * stride + nextJ));
	//					triangles.push_back(uint16(nextI * stride + j));

	//					triangles.push_back(uint16(i * stride + nextJ));
	//					triangles.push_back(uint16(nextI * stride + nextJ));
	//					triangles.push_back(uint16(nextI * stride + j));
	//				}
	//			}
	//		}

	//		g_VerticesBaseWithUV.emplace_back(std::move(vertices));
	//		g_TrianglesBaseWithUV.emplace_back(std::move(triangles));
	//		g_UVsBaseWithUV.emplace_back(std::move(uvs));
	//	}
	//}

	//void PlanetConverterComponent::GenerateNormal(size_t lod,PlanetSphere &sphere)
	//{
	//	m_Normals[sphere.id][lod].clear();
	//	m_Normals[sphere.id][lod].resize(m_Vertices[sphere.id][lod].size());
	//	for (size_t i = 0; i < m_Triangles[sphere.id][lod].size(); i += 3)
	//	{
	//		size_t i1 = m_Triangles[sphere.id][lod][i + 0];
	//		size_t i2 = m_Triangles[sphere.id][lod][i + 1];
	//		size_t i3 = m_Triangles[sphere.id][lod][i + 2];
	//		Vector3 v1 = m_Vertices[sphere.id][lod][i1];
	//		Vector3 v2 = m_Vertices[sphere.id][lod][i2];
	//		Vector3 v3 = m_Vertices[sphere.id][lod][i3];
	//		Vector3 l1 = v2 - v1;
	//		Vector3 l2 = v3 - v2;
	//		Vector3 faceNormal = Vector3::VCross(l1, l2);
	//		m_Normals[sphere.id][lod][i1] += faceNormal;
	//		m_Normals[sphere.id][lod][i2] += faceNormal;   
	//		m_Normals[sphere.id][lod][i3] += faceNormal;
	//	}
	//	for (size_t i = 0; i < m_Vertices[sphere.id][lod].size(); i++)
	//	{
	//		m_Normals[sphere.id][lod][i].normalise();
	//	}
	//}

	//void PlanetConverterComponent::GenerateTangent(size_t lod, PlanetSphere& sphere)
	//{
	//	m_Tangents[sphere.id][lod].resize(m_Vertices[sphere.id][lod].size());
	//	std::vector<Vector3> tan1, tan2;
	//	tan1.resize(m_Vertices[sphere.id][lod].size());
	//	tan2.resize(m_Vertices[sphere.id][lod].size());

	//	for (size_t i = 0; i < m_Triangles[sphere.id][lod].size(); i += 3) {
	//		size_t i1 = m_Triangles[sphere.id][lod][i + 0];
	//		size_t i2 = m_Triangles[sphere.id][lod][i + 1];
	//		size_t i3 = m_Triangles[sphere.id][lod][i + 2];
	//		Vector3 v1 = m_Vertices[sphere.id][lod][i1];
	//		Vector3 v2 = m_Vertices[sphere.id][lod][i2];
	//		Vector3 v3 = m_Vertices[sphere.id][lod][i3];
	//		Vector2 w1 = m_UVs[sphere.id][lod][i1];
	//		Vector2 w2 = m_UVs[sphere.id][lod][i2];
	//		Vector2 w3 = m_UVs[sphere.id][lod][i3];
	//		Vector3 l1 = v2 - v1;
	//		Vector3 l2 = v3 - v2;
	//		Vector2 a1 = w2 - w1;
	//		Vector2 a2 = w3 - w2;

	//		float r = (a1.x * a2.y - a2.x * a1.y);
	//		if (r < 0.0001f)
	//			r = 2048.0f;
	//		else
	//			r = 1.0f / r;
	//		Vector3 s(a2.y * l1.x - a1.y * l2.x, a2.y * l1.y - a1.y * l2.y, a2.y * l1.z - a1.y * l2.z);
	//		Vector3 t(a1.x * l2.x - a2.x * l1.x, a1.x * l2.y - a2.x * l1.y, a1.x * l2.z - a2.x * l1.z);
	//		tan1[i1] += s * r;
	//		tan1[i2] += s * r;
	//		tan1[i3] += s * r;
	//		tan2[i1] += t * r;
	//		tan2[i2] += t * r;
	//		tan2[i3] += t * r;
	//	}

	//	for (size_t i = 0; i < m_Vertices[sphere.id][lod].size(); i++) {
	//		Vector3 n = m_Normals[sphere.id][lod][i];
	//		Vector3 t = tan1[i];
	//		Vector4 tan((t - n * Vector3::VDot(n, t)).normalisedCopy());
	//		tan.w = (Vector3::VDot(Vector3::VCross(n, t), tan2[i]) < 0.0f) ? -1.0f : 1.0f;
	//		m_Tangents[sphere.id][lod][i] = tan;
	//	}

	//}

	//Vector2 PlanetConverterComponent::GetUV(const Vector3 vec)
	//{
	//	if (m_pActComInfo == nullptr)
	//		return Vector2::ZERO;

	//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
	//	if (mPlanetConverterComponentInfo == nullptr)
	//		return Vector2::ZERO;

	//	Vector2 uv;
	//	uv.x = Math::ATan2(vec.z, vec.x).valueRadians() / (2 * Math::PI) + 0.5f;
	//	uv.y = Math::ASin(vec.y).valueRadians() / Math::PI + 0.5f;
	//	uv.x *= 2.0f;
	//	uv.y = (uv.y - 0.5f) * (1.0f + mPlanetConverterComponentInfo->m_PolarReserve) + 0.5f;
	//	uv.x = Math::Repeat(uv.x, 1.0f);
	//	uv.y = Math::Repeat(uv.y, 1.0f);
	//	return uv;
	//}

	//void PlanetConverterComponent::Displace(size_t lod, const String &heightMapPath, const float maxHeight, PlanetSphere &sphere)
	//{
	//	Image img;
	//	if (!img.load(heightMapPath))
	//	{
	//		LogManager::instance()->logMessage("PlanetConverter: Faild to load heightmap", LML_CRITICAL);
	//		return;
	//	}
	//	for (size_t i = 0; i < m_Vertices[sphere.id][lod].size(); i++)
	//	{
	//		Vector3 vertex = m_Vertices[sphere.id][lod][i];
	//		Vector3 dir = vertex.normalisedCopy();
	//		Vector2 uv = GetUV(dir);
	//		/*Vector2 uv = m_UVs[lod][i];
	//		uv.x = Math::Repeat(uv.x, 1.0f);
	//		uv.y = Math::Repeat(uv.y, 1.0f);*/
	//		float distance = GetPixelBilinear(img, uv.x, uv.y).r;
	//		m_Vertices[sphere.id][lod][i] = vertex + dir * distance * maxHeight;
	//	}
	//}
	
 //	bool PlanetConverterComponent::ModifyTexture()
	//{
	//	if (m_pActComInfo == nullptr)
	//		return false;

	//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
	//	if (mPlanetConverterComponentInfo == nullptr)
	//		return false;

	//	Image diffImg;
	//	if (!diffImg.load(mPlanetConverterComponentInfo->m_TexturePath)) 
	//	{
	//		LogManager::instance()->logMessage("PlanetConverter: Faild to load texture", LML_CRITICAL);
	//		return false;
	//	}
	//	
	//	#ifdef PLC_PERF_TEST
	//		EchoProfilerManager::instance()->ProfilerOpen(true);
	//		EchoProfilerManager::instance()->Release();
	//		EchoProfilerManager::instance()->Begin("Texture");
	//	#endif

	//	Image newTex = diffImg;
	//	const uint32 size = newTex.getWidth();
	//	uint32 reserveSize = static_cast<uint32>(size * mPlanetConverterComponentInfo->m_PolarReserve);
	//	reserveSize -= reserveSize & 1;
	//	const uint32 reserveSingle = reserveSize >> 1;
	//	const uint32 mapSize = size - reserveSize;
	//	const uint32 xExtDiv = 16;
	//	const uint32 yExtDiv = 32;

	//	#ifdef PLC_PERF_TEST
	//		EchoProfilerManager::instance()->Begin("TCopy");
	//	#endif

	//	for (uint32 y = reserveSingle; y < reserveSingle + mapSize; ++y)
	//	{
	//		for (uint32 x = 0; x < size; ++x) {
	//			float u = static_cast<float>(x) / static_cast<float>(size);
	//			float v = static_cast<float>(y - reserveSingle) / static_cast<float>(mapSize);
	//			ColorValue src = GetPixelBilinear(diffImg, u, v);
	//			newTex.setColourAt(src, x, y, 0);
	//		}
	//	}

	//	#ifdef PLC_PERF_TEST
	//		EchoProfilerManager::instance()->End();
	//		EchoProfilerManager::instance()->Begin("TInit");
	//	#endif

	//	std::vector<Vector3> vectorCache;
	//	vectorCache.resize(size * size);
	//	auto AvgColor = [&, size](uint32 x, uint32 y, uint32 xH, uint32 yH, float strength)
	//	{
	//		auto GetVec = [&, size](uint32 xi, uint32 yi)
	//		{
	//			Vector3 cacheData = vectorCache[xi * size + yi];
	//			if (cacheData.x != 0 || cacheData.y != 0 || cacheData.z != 0)
	//			{
	//				return cacheData;
	//			}
	//			const float fx = static_cast<float>(xi) / static_cast<float>(size);
	//			const float fy = static_cast<float>(yi) / static_cast<float>(size);
	//			const float y = Math::Sin(fy - 0.5f * Math::PI);
	//			const float ratio = Math::Tan(fx * Math::PI);
	//			const float sum = 1.0f - y * y;
	//			const float xx = sum / (ratio * ratio + 1.0f);
	//			const float x = Math::Sqrt(xx);
	//			const float z = x * ratio;
	//			const Vector3 res(x, y, z);
	//			vectorCache[xi * size + yi] = res;
	//			return res;
	//		};

	//		float weight = 1.0f;
	//		ColorValue c = newTex.getColourAt(x, y, 0);
	//		const Vector3 cV = GetVec(x, y);

	//		auto AddPixel = [&, cV, GetVec](uint32 cx, uint32 cy)
	//		{
	//			auto GetAngle = [](Vector3 a, Vector3 b)
	//			{
	//				return Math::ACos(Vector3::VDot(a, b)).valueRadians() / Math::PI;
	//			};

	//			Vector3 v = GetVec(cx, cy);
	//			float angle = GetAngle(cV, v);
	//			float w = Math::lerp(1.0f, 0.0f, angle * 200.0f / mPlanetConverterComponentInfo->m_BlurStrength / strength);
	//			if (w > 0.01f)
	//			{
	//				c += w * newTex.getColourAt(cx, cy, 0);
	//				weight += w;
	//			}
	//		};

	//		const int32 xMin = static_cast<int32>(x) - static_cast<int32>(xH);
	//		const int32 xMax = static_cast<int32>(x) + static_cast<int32>(xH);
	//		const int32 yMin = static_cast<int32>(y) - static_cast<int32>(yH);
	//		const int32 yMax = static_cast<int32>(y) + static_cast<int32>(yH);

	//		auto FixCoord = [size](int32 x)
	//		{
	//			const int32 s = static_cast<int32>(size);
	//			while(x >= s) x -= s;
	//			while(x < 0) x += s;
	//			return static_cast<uint32>(x);
	//		};
	//		
	//		for (int32 xP = xMin; xP <= xMax; xP++)
	//		{
	//			for (int32 yP = yMin; yP <= yMax; yP++)
	//			{
	//				const uint32 uxP = FixCoord(xP);
	//				const uint32 uyP = FixCoord(yP);

	//				if (uxP == x && uyP == y)
	//				{
	//					continue;
	//				}

	//				AddPixel(uxP, uyP);
	//			}
	//		}
	//		c /= weight;
	//		c.a = 1.0f;
	//		return c;
	//	};

	//	std::vector<std::vector<ColorValue>> tempColorTop, tempColorBottom;
	//	tempColorTop.resize(size);
	//	tempColorBottom.resize(size);
	//	for (auto& iter: tempColorTop)
	//	{
	//		iter.resize(reserveSingle);
	//	}
	//	for (auto& iter: tempColorBottom)
	//	{
	//		iter.resize(reserveSingle);
	//	}

	//	#ifdef PLC_PERF_TEST
	//		EchoProfilerManager::instance()->End();
	//		EchoProfilerManager::instance()->Begin("TDown");
	//		EchoProfilerManager::instance()->Begin("TFillArea");
	//	#endif
	//	for (uint32 y = 0; y < reserveSingle; ++y)
	//	{
	//		for (uint32 x = 0; x < size; ++x) {
	//			const float ratio = static_cast<float>(y) / static_cast<float>(reserveSingle);
	//			const float fillStrength = mPlanetConverterComponentInfo->m_FillStrength + 1.0f;
	//			const float fillRatio = ratio * fillStrength - fillStrength + 1.0f;
	//			ColorValue loop = newTex.getColourAt(x, reserveSingle * 2 - y - 1, 0);
	//			newTex.setColourAt(ColorLerp(mPlanetConverterComponentInfo->m_Color, loop, fillRatio, true), x, y, 0);
	//		}
	//	}
	//	#ifdef PLC_PERF_TEST
	//		EchoProfilerManager::instance()->End();
	//		EchoProfilerManager::instance()->Begin("THorizontalBlur");
	//	#endif
	//	if (mPlanetConverterComponentInfo->m_BlurStrength != 0)
	//	{
	//		for (uint32 y = 0; y < reserveSingle; ++y)
	//		{
	//			for (uint32 x = 0; x < size; ++x)
	//			{
	//				const float ratio = static_cast<float>(y) / static_cast<float>(reserveSingle);
	//				const float fillStrength = mPlanetConverterComponentInfo->m_FillStrength + 1.0f;
	//				const float fillRatio = ratio * fillStrength - fillStrength + 1.0f;
	//				if (fillRatio < 0)
	//				{
	//					tempColorBottom[x][y] = newTex.getColourAt(x, y, 0);
	//					continue;
	//				}
	//				tempColorBottom[x][y] = AvgColor(x, y, size / xExtDiv, size / yExtDiv, 1.0f - ratio);
	//			}
	//		}
	//	}
	//	#ifdef PLC_PERF_TEST
	//		EchoProfilerManager::instance()->End();
	//		EchoProfilerManager::instance()->End();
	//		EchoProfilerManager::instance()->Begin("TUp");
	//	#endif

	//	for (uint32 y = reserveSingle + mapSize; y < size; ++y)
	//	{
	//		for (uint32 x = 0; x < size; ++x)
	//		{
	//			const float ratio = 1.0f - static_cast<float>(y - reserveSingle - mapSize) / static_cast<float>(reserveSingle);
	//			const float fillStrength = mPlanetConverterComponentInfo->m_FillStrength + 1.0f;
	//			const float fillRatio = ratio * fillStrength - fillStrength + 1.0f;
	//			ColorValue loop = newTex.getColourAt(x, reserveSingle * 2 + mapSize * 2 - y - 1, 0);
	//			newTex.setColourAt(ColorLerp(mPlanetConverterComponentInfo->m_Color, loop, fillRatio, true), x, y, 0);
	//		}
	//	}
	//	if (mPlanetConverterComponentInfo->m_BlurStrength != 0)
	//	{
	//		for (uint32 y = reserveSingle + mapSize; y < size; ++y)
	//		{
	//			for (uint32 x = 0; x < size; ++x)
	//			{
	//				const float ratio = 1.0f - static_cast<float>(y - reserveSingle - mapSize) / static_cast<float>(reserveSingle);
	//				const float fillStrength = mPlanetConverterComponentInfo->m_FillStrength + 1.0f;
	//				const float fillRatio = ratio * fillStrength - fillStrength + 1.0f;
	//				if (fillRatio < 0)
	//				{
	//					tempColorTop[x][y - reserveSingle - mapSize] = newTex.getColourAt(x, y, 0);
	//					continue;
	//				}
	//				tempColorTop[x][y - reserveSingle - mapSize] = AvgColor(x, y, size / xExtDiv, size / yExtDiv, 1.0f - ratio);
	//			}
	//		}
	//	}
	//	#ifdef PLC_PERF_TEST
	//		EchoProfilerManager::instance()->End();
	//		EchoProfilerManager::instance()->Begin("TSet");
	//	#endif
	//	
	//	if (mPlanetConverterComponentInfo->m_BlurStrength != 0)
	//	{
	//		for (uint32 y = 0; y < reserveSingle; ++y)
	//		{
	//			for (uint32 x = 0; x < size; ++x)
	//			{
	//				newTex.setColourAt(tempColorBottom[x][y], x, y, 0);
	//			}
	//		}
	//		for (uint32 y = reserveSingle + mapSize; y < size; ++y)
	//		{
	//			for (uint32 x = 0; x < size; ++x)
	//			{
	//				newTex.setColourAt(tempColorTop[x][y - reserveSingle - mapSize], x, y, 0);
	//			}
	//		}
	//	}

	//	#ifdef PLC_PERF_TEST
	//		EchoProfilerManager::instance()->End();
	//		EchoProfilerManager::instance()->Begin("TSave");
	//	#endif

	//	newTex.setFormat(PixelFormat::PF_A8R8G8B8);
	//	newTex.save(mPlanetConverterComponentInfo->m_SaveTexturePath);
	//	#ifdef PLC_PERF_TEST
	//		EchoProfilerManager::instance()->End();
	//		LogManager::instance()->logMessage(EchoProfilerManager::instance()->ToString(MILLISECONDS), LML_CRITICAL);
	//	#endif
	//	return true;
	//}

	void PlanetConverterComponent::createComRootNodeImpl()
	{
		mComRootNode = getSceneManager()->createSceneNode();
	}

	Echo::AxisAlignedBox PlanetConverterComponent::getLoadBoundingBox()
	{
		Vector3 v3Distance = { 1000.0f, 1000.0f, 1000.0f };
		if (m_pActComInfo != nullptr)
			v3Distance = Vector3(static_cast<PlanetConverterComponentInfo*>(m_pActComInfo)->GetCullDistance());
		const Vector3& minBox = getWorldAABB().getMinimum();
		const Vector3& maxBox = getWorldAABB().getMaximum();
		AxisAlignedBox loadBox(minBox - v3Distance, maxBox + v3Distance);
		return loadBox;
	}
#ifdef ECHO_EDITOR
	bool PlanetConverterComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		bool isNeedCopy = false;
		if (!m_MeshPath.empty())
		{
			resMap.insert(std::make_pair(ResourceType::Mesh, m_MeshPath));
			isNeedCopy = true;
		}
		//if (!m_HeightMapPath.empty())
		//{
		//	resMap.insert(std::make_pair(ResourceType::NeedCopyRes, m_HeightMapPath));
		//	isNeedCopy = true;
		//}

		if (!m_ClientDiffPath.empty())
		{
			resMap.insert(std::make_pair(ResourceType::NeedCopyRes, m_ClientDiffPath));
			isNeedCopy = true;
		}

		if (!m_NormalMapPath.empty())
		{
			resMap.insert(std::make_pair(ResourceType::NeedCopyRes, m_NormalMapPath));
			isNeedCopy = true;
		}

		if (!m_CloudTexPath.empty())
		{
			resMap.insert(std::make_pair(ResourceType::NeedCopyRes, m_CloudTexPath));
			isNeedCopy = true;
		}

		if (!m_WaterNormPath.empty())
		{
			resMap.insert(std::make_pair(ResourceType::NeedCopyRes, m_WaterNormPath));
			isNeedCopy = true;
		}

		return isNeedCopy;
	}
#endif
};
