#include "EchoComponentStableHeaders.h"
#include "EchoPlanetEntryControlComponent.h"
#include "EchoFogComponent.h"
#include "EchoWorldSystem.h"
#include "EchoWorldManager.h"
#include "EchoAreaManager.h"
#include "EchoSceneArtTemplateManager.h"
#include "EchoSphericalTerrainManager.h"
#include "EchoSphericalTerrainComponent.h"
#include "EchoSceneComponentManager.h"
#include "EchoSphericalTerrainFog.h"
#include "EchoSky.h"
#include "EchoAtlasSphericalTerrainComponent.h"

namespace Echo
{
	EDITOR_REFLECT_BEGIN(PlanetEntryControlComponentInfo)

	//EDITOR_REFLECT_MEMBER_UPDATE(mMinHeight, "MinHetght", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	//EDITOR_REFLECT_MEMBER_UPDATE(mMaxHeight, "MaxHeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

	EDITOR_REFLECT_MEMBER_UPDATE(mFogColor, "FogColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mFogExpScale, "FogExpScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mFogExpTerm, "FogExpTerm", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mHorizonColor, "HorizonColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mFogStart, "FogStart", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mFogEnd, "FogEnd", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mColorChangeGradient, "ColorChangeGradient", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mFogMaxAlpha, "FogMaxAlpha", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mFogSkyParam, "FogSkyParam", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

	EDITOR_REFLECT_MEMBER_UPDATE(mAtmosApproxAlphaPower, "AtmosApproxAlphaPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mAtmosApproxLightPower, "AtmosApproxLightPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mAtmosApproxLightColor, "AtmosApproxLightColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mAtmosApproxAverageDensity, "AtmosApproxAverageDensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

	//EDITOR_REFLECT_MEMBER_UPDATE(mRayleighExpScale, "RExpScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	//EDITOR_REFLECT_MEMBER_UPDATE(mRayleighExpTerm, "RExpTerm", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	//EDITOR_REFLECT_MEMBER_UPDATE(mGroundIrradiance, "GroundIrradiance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	//EDITOR_REFLECT_MEMBER_UPDATE(mMieExpScale, "MExpScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	//EDITOR_REFLECT_MEMBER_UPDATE(mMieExpTerm, "MExpTerm", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

	EDITOR_REFLECT_MEMBER_UPDATE(mAtmosphereRTop, "RTop", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mAtmosphereRTopArchLight, "RTopArchLight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mAtmosphereRBottom, "RBottom", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mAtmosphereBlendDistance, "RBlendDisatnce", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mAtmosphereBlendRange, "RBlendRange", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

	EDITOR_REFLECT_MEMBER_UPDATE(mEnableAtmosphere, "Enable", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mEnableSecondLayer, "EnableSecondLayer", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	//EDITOR_REFLECT_MEMBER_UPDATE(mTODTemplateName, "TODTemplatePath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();
	PlanetEntryControlComponentInfo::PlanetEntryControlComponentInfo(const Name& type, ActorCOMMoveableType moveable)
		: ActorComponentInfo(type, moveable)
	{
	}

	PlanetEntryControlComponentInfo::~PlanetEntryControlComponentInfo()
	{
	}

	void PlanetEntryControlComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<PlanetEntryControlComponentInfo>::get()->ExportJson(this, jsonNode, "");
		WriteJSNode(jsonNode, "Version", "1.1");
	}

	void PlanetEntryControlComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<PlanetEntryControlComponentInfo>::get()->ImportJson(this, jsonNode, "");
		ReadJSNode(jsonNode, "Version", mVersion);
	}

	void PlanetEntryControlComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		PlanetEntryControlComponentInfo* info = static_cast<PlanetEntryControlComponentInfo*>(comInfo);
		reflect::TypeResolver<PlanetEntryControlComponentInfo>::get()->copyTo(this, info);
		info->mVersion = mVersion;
	}

	void PlanetEntryControlComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<PlanetEntryControlComponentInfo>::get()->overWrite(mOriginComInfo, this);
			PlanetEntryControlComponentInfo* info = static_cast<PlanetEntryControlComponentInfo*>(mOriginComInfo);
			info->mVersion = mVersion;
		}
	}

	DECLAREACTORCOMTYPE_CPP(PlanetEntryControlComponent)
	PlanetEntryControlComponent::PlanetEntryControlComponent(ActorPtr pActor, ActorCOMMoveableType movableType)
		: ProtocolNodeActorComponent(pActor, movableType)
	{
		registerTickTimeBegin();
	}

	PlanetEntryControlComponent::~PlanetEntryControlComponent()
	{
	}

	void PlanetEntryControlComponent::init(ActorComponentInfo* info)
	{
		m_pActComInfo = info;
		mMainCamera = getSceneManager()->getMainCamera();
		PlanetEntryControlComponentInfo* mInfo = static_cast<PlanetEntryControlComponentInfo*>(info);
		if (mInfo == nullptr)
			return;
		//mMinHeight = mInfo->mMinHeight;
		//mMaxHeight = mInfo->mMaxHeight;
		mAtmosProperty.hideAtmosphere = !info->GetVisible();
		auto& profile = mAtmosProperty.profile;
		// 星球雾效相关的参数
		profile.lowerLayerColor = mInfo->mFogColor;
		profile.upperLayerColor = mInfo->mHorizonColor;
		profile.absorptionProfile[1].expScale = Echo::Clamp(0.f, 1.f, mInfo->mFogExpScale);
		profile.absorptionProfile[1].expTerm = Echo::Clamp(0.f, 20.f, mInfo->mFogExpTerm);
		profile.blendParameter.x = mInfo->mFogStart;
		profile.blendParameter.y = mInfo->mFogEnd;
		profile.blendParameter.z = mInfo->mColorChangeGradient;
		profile.blendParameter.w = mInfo->mFogMaxAlpha;
		profile.skyParam = mInfo->mFogSkyParam;
		// 大气散射相关的参数
		// 暂时没用上大气散射

		// 模拟大气相关的参数
		auto& approxProfile = mAtmosProperty.approxProfile;
		approxProfile.alphaPower = mInfo->mAtmosApproxAlphaPower;
		approxProfile.lightPower = mInfo->mAtmosApproxLightPower;
		approxProfile.lightColor = mInfo->mAtmosApproxLightColor;
		approxProfile.averageDensity = mInfo->mAtmosApproxAverageDensity;

		// 星球半径相关的参数
		auto& boundary = mAtmosProperty.boundary;
		boundary.rTop = mInfo->mAtmosphereRTop;
		boundary.rBottom = mInfo->mAtmosphereRBottom;
		boundary.rBlendRange = mInfo->mAtmosphereBlendRange;
		boundary.rBlendDistance = mInfo->mAtmosphereBlendDistance;
		if (mInfo->mVersion == "1.1")
			boundary.rTopArchLight = mInfo->mAtmosphereRTopArchLight;
		else
			boundary.rTopArchLight = mInfo->mAtmosphereRTop;

		// 一些开关
		mAtmosProperty.enableAtmosphere = mInfo->mEnableAtmosphere;
		mAtmosProperty.enableSecondLayerEffect = mInfo->mEnableSecondLayer;
	}

	void PlanetEntryControlComponent::unInit()
	{

	}

	void PlanetEntryControlComponent::connectCom() {}

	void PlanetEntryControlComponent::disconnectCom() {}

	void PlanetEntryControlComponent::registerTickTimeBegin()
	{
		m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}
	void PlanetEntryControlComponent::visibleStateChanged(bool visible) 
	{
		mAtmosProperty.hideAtmosphere = m_VisibleState;
		SetFog();
	}

	void PlanetEntryControlComponent::connectActor() {
		ActorPtr fatherActor = getActor();
		while (fatherActor != nullptr)
		{
			ActorComponentPtr childCom = fatherActor->getFirstComByType(COMPONENTTYPE(SphericalTerrainComponent));
			if (childCom != nullptr)
			{
				SphericalTerrainComponent* terrainCom = static_cast<SphericalTerrainComponent*>(childCom.get());
				SetSphericalTerrainComPtr(terrainCom);
				break;
			}
			//if (Root::instance()->getRuntimeMode() == Root::RuntimeMode::eRM_Editor) {
				ActorComponentPtr childCom2 = fatherActor->getFirstComByType(COMPONENTTYPE(AtlasSphericalTerrainComponent));
				if (childCom2 != nullptr)
				{
					AtlasSphericalTerrainComponent* terrainCom = static_cast<AtlasSphericalTerrainComponent*>(childCom2.get());
					SetAtlasSphericalTerrainComPtr(terrainCom);
					break;
				}
				fatherActor = fatherActor->getParentActor();
			//}
		}	
	}

	void PlanetEntryControlComponent::disconnectActor() {
		if (mSphericalTerrainComponent != nullptr)
			mSphericalTerrainComponent->SetPlanetEntryComPtr(nullptr);
		if (mAtlasSphericalTerrainComponent != nullptr)
			mAtlasSphericalTerrainComponent->SetPlanetEntryComPtr(nullptr);
	}

	void PlanetEntryControlComponent::beginFrame(EventID eventid, ProtocolHandle handle)
	{
		switch (eventid)
		{
			case EID_EveryFrame:
			{
				UpdateAtmosphere();
				UpdateTemplate();
				break;
			}
			default:
				break;
		}
	}

	AxisAlignedBox PlanetEntryControlComponent::getLoadBoundingBox() {
		if (mSphericalTerrainComponent == nullptr && mAtlasSphericalTerrainComponent == nullptr) {
			return ActorComponent::getLoadBoundingBox();
		}
		else if (mSphericalTerrainComponent != nullptr) {
			return mSphericalTerrainComponent->getLoadBoundingBox();
		}
		else{
			return mAtlasSphericalTerrainComponent->getLoadBoundingBox();
		}
	}

	void PlanetEntryControlComponent::UpdateAtmosphere()
	{

	}

	void PlanetEntryControlComponent::SetSphericalTerrainComPtr(SphericalTerrainComponent* terrainCom) {
		if (terrainCom->SetPlanetEntryComPtr(this))
			mSphericalTerrainComponent = terrainCom;
	}

	void PlanetEntryControlComponent::SetAtlasSphericalTerrainComPtr(AtlasSphericalTerrainComponent* atlasTerrainCom)
	{
		if (atlasTerrainCom->SetPlanetEntryComPtr(this))
			mAtlasSphericalTerrainComponent = atlasTerrainCom;
	}

	void  PlanetEntryControlComponent::SetRayleighScatter(const Vector3& rs)
	{
		//mAtmosProperty. = rs;
		//SetAtmosphere();
	}
	void  PlanetEntryControlComponent::SetMieScatter(const Vector3& ms)
	{
		//mMieScatter = ms;
		//SetAtmosphere();
	}
	void  PlanetEntryControlComponent::SetMieAbsorption(const Vector3& ma)
	{
		//mMieAbsorption = ma;
		//SetAtmosphere();
	}
	void  PlanetEntryControlComponent::SetOzoneAbsorption(const Vector3& oa)
	{
		//mOzoneAbsorption = oa;
		//SetAtmosphere();
	}
	void PlanetEntryControlComponent::SetGroundIrradiance(const Vector3& groundIrradiance)
	{
		//mGroundIrradiance = groundIrradiance;
		//SetAtmosphere();
	}
	void PlanetEntryControlComponent::SetRayleighExpScale(float rayleighExpScale) {
		//mRayleighExpScale = rayleighExpScale;
		//SetAtmosphere();
	}
	void PlanetEntryControlComponent::SetMieExpScale(float mieExpScale) {
		//mMieExpScale = mieExpScale;
		//SetAtmosphere();
	}
	void PlanetEntryControlComponent::SetRayleighExpTerm(float rayleighExpTerm) {
		//mRayleighExpTerm = rayleighExpTerm;
		//SetAtmosphere();
	}
	void PlanetEntryControlComponent::SetMieExpTerm(float mieExpTerm) {
		//mMieExpTerm = mieExpTerm;
		//SetAtmosphere();
	}

	//void PlanetEntryControlComponent::SetRayleighHeightScale(float rayleightHeightScale)
	//{
	//	mRayleighHeightScale = rayleightHeightScale;
	//	SetAtmosphere();
	//}
	//void PlanetEntryControlComponent::SetMieHeightScale(float mieHeightScale)
	//{
	//	mMieHeightScale = mieHeightScale;
	//	SetAtmosphere();
	//}

	void PlanetEntryControlComponent::UpdateTemplate()
	{

	}

	void PlanetEntryControlComponent::SetFogColor(const ColorValue& val)
	{
		mAtmosProperty.profile.lowerLayerColor = val;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mFogColor = val;
		}
#endif
		SetFog();
	}
	void PlanetEntryControlComponent::SetFogExpTerm(float val)
	{
		mAtmosProperty.profile.absorptionProfile[1].expTerm = val;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mFogExpTerm = val;
		}
#endif
		SetFog();
	}
	void PlanetEntryControlComponent::SetFogExpScale(float val)
	{
		mAtmosProperty.profile.absorptionProfile[1].expScale = val;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mFogExpScale = val;
		}
#endif
		SetFog();
	}

	void PlanetEntryControlComponent::SetHorizonColor(const ColorValue& val)
	{
		mAtmosProperty.profile.upperLayerColor = val;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mHorizonColor = val;
		}
#endif
		SetFog();
	}
	void PlanetEntryControlComponent::SetFogStart(float val)
	{
		mAtmosProperty.profile.blendParameter.x = val;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mFogStart = val;
		}
#endif
		SetFog();
	}
	void PlanetEntryControlComponent::SetFogEnd(float val)
	{
		mAtmosProperty.profile.blendParameter.y = val;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mFogEnd = val;
		}
#endif
		SetFog();
	}
	void PlanetEntryControlComponent::SetColorChangeGradient(float val)
	{
		mAtmosProperty.profile.blendParameter.z = val;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mColorChangeGradient = val;
		}
#endif
		SetFog();
	}
	void PlanetEntryControlComponent::SetMaxAlpha(float val) {
		mAtmosProperty.profile.blendParameter.w = val;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mFogMaxAlpha = val;
		}
#endif
		SetFog();
	}
	void PlanetEntryControlComponent::SetSkyParam(float val) {
		mAtmosProperty.profile.skyParam.x = val;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mFogSkyParam = val;
		}
#endif
		SetFog();
	}

	void PlanetEntryControlComponent::SetApproxAlphaPower(float val) {
		mAtmosProperty.approxProfile.alphaPower = val;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mAtmosApproxAlphaPower = val;
		}
#endif
		SetFog();
	}

	void PlanetEntryControlComponent::SetApproxLightPower(float val) {
		mAtmosProperty.approxProfile.lightPower = val;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mAtmosApproxLightPower = val;
		}
#endif
		SetFog();
	}

	void PlanetEntryControlComponent::SetApproxLightColor(const ColorValue& val) {
		mAtmosProperty.approxProfile.lightColor = val;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mAtmosApproxLightColor = val;
		}
#endif
		SetFog();
	}

	void PlanetEntryControlComponent::SetApproxAverageDensity(float val){
		mAtmosProperty.approxProfile.averageDensity = val;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mAtmosApproxAverageDensity = val;
		}
#endif
		SetFog();
	}

	SphericalTerrainAtmosphereApproxProfile* PlanetEntryControlComponent::GetApproxProfile() {
		return &mAtmosProperty.approxProfile;
	}

	SphericalTerrainAtmosphereBoundary* PlanetEntryControlComponent::GetBoundary() {
		return &mAtmosProperty.boundary;
	}

	SphericalTerrainAtmosphereProperty* PlanetEntryControlComponent::GetAtmosphereProperty() {
		return &mAtmosProperty;
	}

	void PlanetEntryControlComponent::SetAtmosphereRTop(float rTop) {
		mAtmosProperty.boundary.rTop = rTop;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mAtmosphereRTop = rTop;
		}
#endif
		SetFog();
	}
	void PlanetEntryControlComponent::SetAtmosphereRTopArchLight(float rTopArchLight) {
		mAtmosProperty.boundary.rTopArchLight = rTopArchLight;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mAtmosphereRTopArchLight = rTopArchLight;
		}
#endif
		SetFog();
	}

	void PlanetEntryControlComponent::SetAtmosphereRBottom(float rBottom) {
		mAtmosProperty.boundary.rBottom = rBottom;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mAtmosphereRBottom = rBottom;
		}
#endif
		SetFog();
	}

	void PlanetEntryControlComponent::SetAtmosBlendRange(float val) {
		mAtmosProperty.boundary.rBlendRange = val;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mAtmosphereBlendRange = val;
		}
#endif
		SetFog();
	}

	void PlanetEntryControlComponent::SetAtmosBlendDistance(float val) {
		mAtmosProperty.boundary.rBlendDistance = val;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mAtmosphereBlendDistance = val;
		}
#endif
		SetFog();
	}

	void PlanetEntryControlComponent::SetEnable(bool val) {
		mAtmosProperty.enableAtmosphere = val;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mEnableAtmosphere = val;
		}
#endif
		SetFog();
	}

	void PlanetEntryControlComponent::SetEnableSecondLayerEffect(bool val) {
		mAtmosProperty.enableSecondLayerEffect = val;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			PlanetEntryControlComponentInfo* peccInfo = static_cast<PlanetEntryControlComponentInfo*>(m_pActComInfo);
			peccInfo->mEnableSecondLayer = val;
		}
#endif
		SetFog();
	}

	void PlanetEntryControlComponent::SetAtmosApprox() {}

	void PlanetEntryControlComponent::SetAtmosphereRange() {}

	void PlanetEntryControlComponent::SetAtmosphere(){}

	void PlanetEntryControlComponent::SetFog()
	{
		if (mSphericalTerrainComponent != nullptr)
		{
			mSphericalTerrainComponent->SetAtmosPropertyDirty();
		}
		if (mAtlasSphericalTerrainComponent != nullptr)
		{
			mAtlasSphericalTerrainComponent->SetAtmosPropertyDirty();
		}
	}

	void PlanetEntryControlComponent::EnableFog() {}

	void PlanetEntryControlComponent::EnableSecondLayerAtmosphere() {}

#ifdef ECHO_EDITOR
	bool PlanetEntryControlComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		bool isNeedCopy = false;

		return isNeedCopy;
	}
#endif
}
