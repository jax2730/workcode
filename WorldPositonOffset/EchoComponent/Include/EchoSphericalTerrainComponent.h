//-----------------------------------------------------------------------------
// File:   EchoSphericalTerrainComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2024-5-8
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef EchoSphericalTerrainComponent_h__
#define EchoSphericalTerrainComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"

#include "EchoComDefineMacro.h"
#include "EchoSphericalTerrainManager.h"
#include "EchoMeshColliderComponent.h"
#include "EchoPlanetRoad.h"
#include "EchoGeneratedTerrainPhysics.h"
#include "EchoParticleProFunctionImp.h"
#include "EchoParticleSystemComponent.h"
#include "Effect/EchoEffectHolder.h"

namespace Echo
{
	class PlanetRoadNetworkManager;

#define PLANET_ROAD
	struct VolumetricCloudProperty
	{
		void									setEnableVolumetricCloud(const bool& val) { EnableVolumetricCloud = val; }
		bool									getEnableVolumetricCloud()const { return  EnableVolumetricCloud; }

		void									setLayerBottomAltitudeKm(const float& val) { LayerBottomAltitudeKm = val; }
		float									getLayerBottomAltitudeKm()const { return LayerBottomAltitudeKm; }

		void									setLayerHeightKm(const float& val) { LayerHeightKm = val; }
		float									getLayerHeightKm()const { return LayerHeightKm; }

		void									setTracingStartMaxDistance(const float& val) { TracingStartMaxDistance = val; }
		float									getTracingStartMaxDistance()const { return TracingStartMaxDistance; }

		void									setTracingMaxDistance(const float& val) { TracingMaxDistance = val; }
		float									getTracingMaxDistance()const { return TracingMaxDistance; }

		void									setCloudBasicNoiseScale(const float& val) { CloudBasicNoiseScale = val; }
		float									getCloudBasicNoiseScale()const { return CloudBasicNoiseScale; }

		void									setStratusCoverage(const float& val) { StratusCoverage = val; }
		float									getStratusCoverage()const { return StratusCoverage; }

		void									setStratusContrast(const float& val) { StratusContrast = val; }
		float									getStratusContrast()const { return StratusContrast; }

		void									setMaxDensity(const float& val) { MaxDensity = val; }
		float									getMaxDensity()const { return MaxDensity; }

		void									setWindOffset(const Vector3& val) { WindOffset = val; }
		Vector3									getWindOffset()const { return WindOffset; }

		void									setCloudColor(const ColorValue& val) { CloudColor = val; }
		ColorValue								getCloudColor()const { return CloudColor; }

		void									setCloudPhaseG(const float& val) { CloudPhaseG = val; }
		float									getCloudPhaseG()const { return  CloudPhaseG; }
		void									setCloudPhaseG2(const float& val) { CloudPhaseG2 = val; }
		float									getCloudPhaseG2()const { return  CloudPhaseG2; }
		void									setCloudPhaseMixFactor(const float& val) { CloudPhaseMixFactor = val; }
		float									getCloudPhaseMixFactor()const { return  CloudPhaseMixFactor; }
		void									setMsScattFactor(const float& val) { MsScattFactor = val; }
		float									getMsScattFactor()const { return  MsScattFactor; }
		void									setMsExtinFactor(const float& val) { MsExtinFactor = val; }
		float									getMsExtinFactor()const { return  MsExtinFactor; }
		void									setMsPhaseFactor(const float& val) { MsPhaseFactor = val; }
		float									getMsPhaseFactor()const { return  MsPhaseFactor; }

		//CloudLayer
		bool  EnableVolumetricCloud = false;
		float LayerBottomAltitudeKm = 1.50f;				//云高度(距离地表)
		float LayerHeightKm = 1.0f;						//云厚度
		float TracingStartMaxDistance = 30.0f;
		float TracingMaxDistance = 20.0f;

		//Material
		float CloudBasicNoiseScale = 0.03f;
		float StratusCoverage = 0.45f;
		float StratusContrast = 0.5f;
		float MaxDensity = 0.03f;

		float CloudPhaseG = 0.5f;
		float CloudPhaseG2 = -0.5f;
		float CloudPhaseMixFactor = 0.5f;
		float MsScattFactor = 0.419f;
		float MsExtinFactor = 0.1024f;
		float MsPhaseFactor = 0.2453f;

		Vector3 WindOffset = Vector3::ZERO;
		ColorValue CloudColor = ColorValue(1.0f, 1.0f, 1.0f, 1.0f);
	};

	namespace VoxelFoliage
	{
		struct CollectEntranceListener;
	}
	class VoxelChunkValidateListener;

	class PavementFitTerrain;
	//#define STP_USE_RESOURCE 1
	template <typename T>
	class NaiveWrapper
	{
	public:
		NaiveWrapper():hasValue(false){}
		NaiveWrapper(const T& data)
			:hasValue(true)
		{
			member = data;
		}
		bool hasValue;
		T member;
		bool IsValid() const { return hasValue; }
	};
	class PlanetEntryControlComponent;
	class SphericalTerrainComponent;
	typedef EchoWeakPtr<SphericalTerrainComponent> SphericalTerrainComponentPtr;
	class _EchoComponentExport SphericalTerrainComponent :
		public SphericalTerrain::LoadListener,
		public GeneratedTerrainPhysics_Spherical,
		public ProtocolActorComponent
	{
	public:
		struct Modifier {
			enum Enum : uint32 {
				eRECTANGLE = 0,
				eCIRCLE,
				eNUMBER,
			};
			Modifier() {}
			Modifier(Enum _type, const Vector3& _position) :type(_type), position(_position) {}
			Enum type = eNUMBER;
			Vector3 position = Vector3::ZERO;
		};
		struct CircleModifier : public Modifier {
			CircleModifier() : Modifier(eCIRCLE,Vector3::ZERO) {}
			CircleModifier(const Vector3& _position, float _radius) : Modifier(eCIRCLE, _position), radius(_radius) {}
			CircleModifier(const CircleModifier& other);
			CircleModifier& operator=(const CircleModifier& other);
			float radius = 10.0f;
		};
		struct RectangleModifier : public Modifier {
			RectangleModifier() : Modifier(eRECTANGLE, DVector3::ZERO) {}
			RectangleModifier(const Vector3& _position, float _width, float _height, float _yaw) : Modifier(eRECTANGLE, _position), width(_width), height(_height), yaw(_yaw) {}
			RectangleModifier(const RectangleModifier& other);
			RectangleModifier& operator=(const RectangleModifier& other);
			float width = 10.0f;
			float height = 10.0f;
			float yaw = 0.0f;
		};
	public:
		DECLAREACTORCOMTYPE_H(SphericalTerrainComponent)
		REGISTER_LUA_CLASS_KEY
	public:
		virtual void init(ActorComponentInfo* info) override;
		virtual void unInit() override;
		virtual void replay(ActorComponentInfo* info) override;
		virtual void copyTo(const ActorComponentPtr& actCom) override;
		virtual void connectCom() override;
		virtual void disconnectCom() override;
		virtual void NotifiChildImp(int changeMask) override;
		virtual void visibleStateChanged(bool activeState) override;
		virtual void OnCreateFinish() override;

		void registerTickTimeBegin()override;
		void beginFrame(EventID, ProtocolHandle)override;
		void UpdateParticle(bool falseUpdate = false);

		void OnDestroy() override;
		virtual AxisAlignedBox getLoadBoundingBox() override;
	public:
		bool GetIsSyncLoad() const { return m_IsSyncLoad; }
		void SetIsSyncLoad(bool v) { m_IsSyncLoad = v; }

		const String& GetTerrainPath() { return m_sTerrainPath; }
		void SetTerrainPath(const String& name);

		const String& GetStarEffectPath() { return m_sStarEffectPath; }
		void SetStarEffectPath(const String& name);

		const DTransform& GetRenderOffset() { return m_RenderOffset; }
		void SetRenderOffset(const DTransform& trans);

		inline bool IsExistTerrain() { return m_pSphericalTerrain != nullptr; }
		bool IsCreateFinish() const;
		SphericalTerrain* GetSphericalTerrain() { return m_pSphericalTerrain; }
		const SphericalTerrain* GetSphericalTerrain() const { return m_pSphericalTerrain; }

        //NOTE: Visual effect control [Region display on the planet]. 
        //NOTE: selected region will be highlight display.
        void enableRegionVisualize(bool enable);

        //IMPORTANT:Deprecated. instead of setHighlightFineRegionID() |
        // resetHighlightFineRegionID()
        void setSelectedRegion(int regionID);
        void resetSelectedRegion();

        bool setHighlightFineRegionID(int id);
        void resetHighlightFineRegionID();
        
        bool setHighlightCorseRegionID(int id);
        void resetHighlightCorseRegionID();
        
        void setDisplayCorseOrFineRegion(bool enableCorse);
        
        void setRegionGlowColor(const ColorValue& color);
        
        void setRegionFillColor(const ColorValue& color);
        void setCoarseRegionFillColor(const ColorValue& color);
        
        void setSingleRegionFillColor(int id, const ColorValue& color);
        void setSingleCoarseRegionFillColor(int id, const ColorValue& color);
        
        void setRegionHighlightColor(const ColorValue& color);
        
        void setRegionHighlightWarfogColor(const ColorValue& color);
        void setWarfogMaskUVScale(const float scale);
        void setWarfogMaskScale(const float scale);
        void setOutlineSmoothRange(const float range);
        
        void setOutlineWidth(float width);
        void setCommonRegionAlpha(float alpha);
        void setHighlightRegionAlpha(float alpha);

        void setRegionWarfogAlpha(int id, float alpha);
        void setCoarseRegionWarfogAlpha(int id, float alpha);
        
        void setRegionWarfogColor(const ColorValue& color);

        void setWarfogMaskTexture(const String& path);

        void setAdditionOutLineColor(const ColorValue& color);
        void setAdditionOutLineAlpha(const float& v);
        void setAdditionOutLineWidth(const float& v);
        void setAdditionOutLineDistance(const float& v);

		virtual void SetIsUsePhysics(bool value) override;
		virtual void SetMaterialId(int value)override;
		virtual void SetFilterGroupType(PhysXGroup value)override;
		virtual void SetCanInteractFilterGroupType(PhysXGroup value)override;
		virtual void SetMeshFlags(EchoMeshColliderFlags value)override;

        //NOTE:Low LOD Cloud visual effect control.
        void setEnableLowLODCloud(bool enalbe);		
        void setCloudUVScale(float v);				// v range: [0 ~ 100]
        void setCloudFlowSpeed(float v);			// v range: [0 ~ 10]
        void setCloudHDRFactor(float v);			// v range: [0 ~ 10]
        void setCloudMetallic(float v);				// v range: [0 ~ 1]
        void setCloudRoughness(float v);			// v range: [0 ~ 1]
        void setCloudAmbientFactor(float v);		// v range: [0 ~ 1]
        void setCloudNoiseOffsetScale(float v);		// v range: [0 ~ 1]
        void setCloudTexture(const String& path);

		bool  getEnableLowLODCloud();
		float getCloudUVScale();
		float getCloudFlowSpeed();
		float getCloudHDRFactor();
		float getCloudMetallic();
		float getCloudRoughness();
		float getCloudAmbientFactor();
		float getCloudNoiseOffsetScale();
		const String& getCloudTexture();

		/////////////////////////////////
		void									setEnableVolumetricCloud(const bool& val);
		bool									getEnableVolumetricCloud()const { return  mCloudProperty.getEnableVolumetricCloud(); }

		void									setLayerBottomAltitudeKm(const float& val);
		float									getLayerBottomAltitudeKm()const { return mCloudProperty.getLayerBottomAltitudeKm(); }

		void									setLayerHeightKm(const float& val);
		float									getLayerHeightKm()const { return mCloudProperty.getLayerHeightKm(); }

		void									setTracingStartMaxDistance(const float& val);
		float									getTracingStartMaxDistance()const { return mCloudProperty.getTracingStartMaxDistance(); }

		void									setTracingMaxDistance(const float& val);
		float									getTracingMaxDistance()const { return mCloudProperty.getTracingMaxDistance(); }

		void									setCloudBasicNoiseScale(const float& val);
		float									getCloudBasicNoiseScale()const { return mCloudProperty.getCloudBasicNoiseScale(); }
		void									setStratusCoverage(const float& val);
		float									getStratusCoverage()const { return mCloudProperty.getStratusCoverage(); }
		void									setStratusContrast(const float& val);
		float									getStratusContrast()const { return mCloudProperty.getStratusContrast(); }
		void									setMaxDensity(const float& val);
		float									getMaxDensity()const { return mCloudProperty.getMaxDensity(); }
		void									setWindOffset(const Vector3& val);
		Vector3									getWindOffset()const { return mCloudProperty.getWindOffset(); }
		void									setCloudColor(const ColorValue& val);
		ColorValue								getCloudColor()const { return mCloudProperty.getCloudColor(); }

		void									setCloudPhaseG(const float& val);
		float									getCloudPhaseG()const { return  mCloudProperty.getCloudPhaseG(); }
		void									setCloudPhaseG2(const float& val);
		float									getCloudPhaseG2()const { return  mCloudProperty.getCloudPhaseG2(); }
		void									setCloudPhaseMixFactor(const float& val);
		float									getCloudPhaseMixFactor()const { return  mCloudProperty.getCloudPhaseMixFactor(); }
		void									setMsScattFactor(const float& val);
		float									getMsScattFactor()const { return  mCloudProperty.getMsScattFactor(); }
		void									setMsExtinFactor(const float& val);
		float									getMsExtinFactor()const { return  mCloudProperty.getMsExtinFactor(); }
		void									setMsPhaseFactor(const float& val);
		float									getMsPhaseFactor()const { return  mCloudProperty.getMsPhaseFactor(); }

		void									updateVolumetricCloudParams();

		/////////////////////////////////
		void setEnableVoxel(const bool val);
		bool getEnableVoxel() const;

		const std::vector<OrientedBox>& GetUnderGroundRooms() const;
#if ECHO_EDITOR
		void SpawnUnderGroundRooms(int targetCount, const Vector3& roomSize);
		bool CollectCaveEntrancesAsync(VoxelFoliage::CollectEntranceListener* listener);
		bool SaveChunkValidationMask(VoxelChunkValidateListener* listener) const;
#endif

		ActorComponentPtr GetVoxelComponentPtr() const { return m_VoxelComponent; }
	public:
		float getMaxRealRadius() const;
		float getRadius() const;

		int getRegionID(const DVector3& worldPos) const;
		const RegionDefine* getRegionDefine(int id);
		NaiveWrapper<SphericalVoronoiRegionDefineBase> getSphericalVoronoiRegionDefineBase(int id);
		NaiveWrapper<SphericalVoronoiRegionDefine> getSphericalVoronoiRegionDefine(int id);

		std::vector<int> GetCoarseLayerAdjacentRegions(int id) const;
		std::vector<int> GetFineLayerAdjacentRegions(int id) const;
		std::vector<Vector3> GetCoarseLayerCenters(bool ToWorldSpace = false) const;
		std::vector<Vector3> GetFineLayerCenters(bool ToWorldSpace = false) const;

		float getSurfaceArea(int id = -1);
		float getCoarseSurfaceArea(unsigned int id);

		int getSurfaceTypeWs(const DVector3& worldPos) const;
		int getSurfaceTypeLs(const Vector3& localPos) const;
		uint32 getPhySurfaceType(const DVector3& worldPos) const;

		Vector3 getUpVectorWs(const DVector3& worldPos) const;

		Vector3 getGravityWs(const DVector3& worldPos) const;
		Vector3 getGravityLs(const Vector3& localPos) const;
		int getSphereGeometryType() const;

		DVector3 getSurfaceWs(const DVector3& worldPos, Vector3* faceNormal = nullptr) const;
		Vector3 getSurfaceLs(const Vector3& localPos, Vector3* faceNormal = nullptr) const;

		DVector3 getSurfaceFinestWs(const DVector3& worldPos, Vector3* faceNormal = nullptr) const;
		Vector3 getSurfaceFinestLs(const Vector3& localPos, Vector3* faceNormal = nullptr) const;

		int getSurfaceMipWs(const DVector3& worldPos) const;
		int getSurfaceMipLs(const Vector3& localPos) const;

		DVector3 getOceanSurface(const DVector3& worldPos) const;
		DVector3 getOceanWaveSurface(const DVector3& worldPos) const;
		bool isUnderOcean(const DVector3& worldPos, double* depth = nullptr) const;
		bool isUnderOceanWave(const DVector3& worldPos, double* depth = nullptr) const;

		float getBoarderValue(const DVector3& worldSurfacePos) const;

		float getMinNoiseScale() const;
		float getMaxNoiseScale() const;

		bool hasModifier(uint64 id) const;
		const Modifier* getModifier(uint64 id) const;
		void addModifier(uint64 id, const Modifier& modifier);
		void removeModifier(uint64 id);
		bool SetPlanetEntryComPtr(PlanetEntryControlComponent* ptr);
		void SetAtmosPropertyDirty();
		//void EnableAtmosphere(bool enable);
	protected:
		void UpdateTerrainAtmos();
		void CreateTerrain();
		void DestroyTerrain();
		void ChangeTerrain();

		void FindModifierPhysics(uint64 id, std::unordered_map<int, ActorComponentPtr>& phyMap);
		void ApplyModifiers();
		void ApplyModifier(uint64 id, const Modifier* modifier);
		//void UpdateLowLodCloudParameter();
		void* _NewPhysicsObjectFunction(const SphericalTerrain::PhysicsObjectDesc& desc);
		bool _DeletePhysicsObjectFunction(void* ptr);

		inline DTransform GetTerrainWorldTrans();
		static void GenerateVBIBFunction(int index, SphericalTerrainComponent* _thisPtr, std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec);
		static uint32 GetSurfaceTypeFunction(SphericalTerrainComponent* _thisPtr, MeshColliderComponent* _pComp, const DVector3& position, bool bWorld);
	private:
		virtual ActorComponent* getThisCompPtr() override;
		virtual bool IsCanPhysics() override;
		virtual void AddMeshColliderImpl(MeshColliderComponent* pCollider, int index) override;
		virtual void getAllBlockIndex(std::vector<int>&) override;
		virtual void getBlockIndex(const AxisAlignedBox&, std::vector<int>&) override;
		virtual AxisAlignedBox getBlockBound(int) override;
		virtual void UpdateLoadData() override;
		virtual bool getIsNearby(const Vector3& worldPos) override;

		void CreateVoxel();
		void DestroyVoxel();

		bool m_IsSyncLoad = false;
		String m_sTerrainPath;
		float m_NearEarthHeightSqr = 0.0f;
		DTransform m_RenderOffset;

		String m_sStarEffectPath;
		String m_sCurrentPlanetStarEffectPath;
		std::map<uint64, Modifier*> m_ModifierMap;

		PlanetEntryControlComponent* m_AtmosCom = nullptr;

		SphericalTerrain* m_pSphericalTerrain = nullptr;

		std::unique_ptr<EffectHolder> particalHolder;

		unordered_map<Name, PhysXTriangleMeshResourcePtr>::type mPhysicsObjectResourceCache;
		friend class EchoPhysicsManager;
	private:
		bool	m_EnableVoxel				= false;

		bool	m_bEnableLowLODCloud		= false;
		float	m_fCloudUVScale				= 1.0f;		
		float	m_fCloudFlowSpeed			= 1.0f;		
		float	m_fCloudHDRFactor			= 1.0f;	
		float	m_fCloudMetallic			= 0.0f;	
		float	m_fCloudRoughness			= 1.0f;	
		float	m_fCloudAmbientFactor		= .05f;	
		float	m_fCloudNoiseOffsetScale	= .03f;
		String  m_strCloudTexture			= "";

		std::unique_ptr<PavementFitTerrain> m_FitTerrain;

		VolumetricCloudProperty			mCloudProperty;

		ActorComponentPtr m_VoxelComponent;
	public:
		static const String gServerRelativePath;
		static bool ExportForServer(const set<String>::type& terrainPaths, int exportVersion, const String& assetPath, long long timeStamp = 0);
		static bool ExportForServer(const String& terrainPath, int exportVersion, const String& assetPath, long long timeStamp = 0);
		static bool ExportForServer(ProceduralSphere* pTerrain, int exportVersion, const String& assetPath, long long timeStamp, set<String>::type& exportFileSet);
		static String ExportForPlanetGraph(ProceduralSphere* pTerrain);

		static bool ExportPCGForServer(const String& terrainPath);
		static bool ExportRoadSidePCG(const String& terrainPath, const String& folderPath);

		// Heatmap export settings (editor helper)
		enum HeatmapColorMode {
			HEATMAP_RGB = 0,        // 默认RGB（蓝->绿->黄->红）
			HEATMAP_SINGLE_COLOR    // 单色渐变（浅->深）
		};
		struct PCGDiagramDesc {
			float HeatmapDesiredCellLenWs = 200.0f;
			HeatmapColorMode HeatmapColorMode = HeatmapColorMode::HEATMAP_RGB;
			ColorValue HeatmapBaseColor = ColorValue(1.0f, 0.0f, 0.0f, 1.0f); // 默认红色
			bool UseManualThresholds = true;
			float ManualMaxThreshold = 30.0f;
			float ManualMinThreshold = 10.0f;
			float ExportHeatmapThreshold = 30.0f;
		};
		struct PCGDiagramResult {
			uint32 MinThresholdGridNum = 0;
			uint32 MaxThresholdGridNum = 0;
			float MinGridTriNum = 1000.0f;
			float MaxGridTriNum = 0.0f;
			uint32 MinMeshTriNum = UINT_MAX;
			uint32 MaxMeshTriNum = 0;
		};
		static bool ExportPCGObjectsBounds(const String& terrainPath, const PCGDiagramDesc& _desc, PCGDiagramResult& _result);
	public:
		class _EchoComponentExport RoadListener : public PlanetRoadListener
		{
		public:
			RoadListener(SphericalTerrainComponent* planet);
		public:
			virtual void onBuildFinish();
			virtual void onCreate(const PlanetRoadData& road);
			virtual void onDestroy(const PlanetRoadData& road);

			virtual void onCreate(const PlanetCrossRoadData& road);
			virtual void onDestroy(const PlanetCrossRoadData& road);
		public:
			SphericalTerrainComponent* planetCom = nullptr;
		};
		RoadListener* planetroadListener = nullptr;
		std::map<uint16, ActorPtr>	roadCache;
		std::map<uint16, ActorComponentPtr>	crossRoadCache;

		void	BuildPlanetRoad(bool sync = false);
		void	CreatePlanetRoad(bool sync = false);
		void	ClearPlanetRoadCache();
#ifdef ECHO_EDITOR
		void	EditorPlanetRoadSave();
		void	EditorPlanetRoadUpdateByTemplate(bool sync = false);
		void	EditorPlanetRoadReset(uint16 templateID);
		void	EditorPlanetRoadCopyLinkInfoFromFile(const std::string_view& path);
#endif
		PlanetRoadNetworkManager* GetPlanetRoadNetworkManager() { return m_PlanetRoadNetworkManager.get(); }
	private:
		std::unique_ptr<PlanetRoadNetworkManager> m_PlanetRoadNetworkManager = nullptr;
		// All road related logic will be handled by PlanetRoadNetworkManager
		friend class PlanetRoadNetworkManager;
	};

	class _EchoComponentExport SphericalTerrainComponentInfo : public ActorComponentInfo{
	public:
		SphericalTerrainComponentInfo() {}
		SphericalTerrainComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) :ActorComponentInfo(comType, MoveType) {};
		virtual ~SphericalTerrainComponentInfo() {};
#ifdef ECHO_EDITOR
		virtual bool getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(SphericalTerrainComponent)
		_DECLCOMINFO_PARAM_SIM(bool, IsUsePhysics, true, SetIsUsePhysics)
		_DECLCOMINFO_PARAM(String, TerrainPath, String(""), SetTerrainPath)
		_DECLCOMINFO_PARAM(String, StarEffectPath, String(""), SetStarEffectPath)
		_DECLCOMINFO_PARAM_SIM(EchoMeshColliderFlags, MeshFlags, EchoMeshColliderFlag::eNONE, SetMeshFlags)
		_DECLCOMINFO_PARAM_SIM(int, MaterialId, -1, SetMaterialId)
		_DECLCOMINFO_PARAM_SIM(PhysXGroup, FilterGroupType, ePhysXGroup_Mesh, SetFilterGroupType)
		_DECLCOMINFO_PARAM_SIM(PhysXGroup, CanInteractFilterGroupType, ePhysXGroup_All, SetCanInteractFilterGroupType)

		_DECLCOMINFO_PARAM_SIM(bool,  EnableLowLODCloud,		false, setEnableLowLODCloud)
		_DECLCOMINFO_PARAM_SIM(float, CloudUVScale,				1.0f , setCloudUVScale)
		_DECLCOMINFO_PARAM_SIM(float, CloudFlowSpeed,			1.0f , setCloudFlowSpeed)
		_DECLCOMINFO_PARAM_SIM(float, CloudHDRFactor,			1.0f , setCloudHDRFactor)
		_DECLCOMINFO_PARAM_SIM(float, CloudMetallic,			0.0f , setCloudMetallic)
		_DECLCOMINFO_PARAM_SIM(float, CloudRoughness,			1.0f , setCloudRoughness)
		_DECLCOMINFO_PARAM_SIM(float, CloudAmbientFactor,		.05f , setCloudAmbientFactor)
		_DECLCOMINFO_PARAM_SIM(float, CloudNoiseOffsetScale,	.03f , setCloudNoiseOffsetScale)
		_DECLCOMINFO_PARAM	  (String,CloudTexture,				""	 , setCloudTexture)

		_DECLCOMINFO_PARAM_SIM(bool, EnableVoxel,				false, setEnableVoxel)
		_DECLCOMINFO_PARAM_END()
	public:
		VolumetricCloudProperty			mCloudProperty;
	};

	DEFINECOMFACTORY(SphericalTerrainComponent,ALL_MOVEABLE)

};

#endif // EchoSphericalTerrainComponent_h__
