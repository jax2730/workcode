//-----------------------------------------------------------------------------
// File:   EchoSphericalTerrainManager.h
//
// Author: lizizhen
//
// Date:  2024-4-7
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#if !defined(ECHO_SPHERICAL_TERRAIN_MANAGER_H)

#include <span>

#include "EchoComputable.h"
#include "EchoPrerequisites.h"

#include "JobSystem/EchoCommonJobManager.h"
#include "EchoSphericalTerrianQuadTreeNode.h"
#include "EchoSphericalTerrainAtmosphere.h"
#include "EchoPlanetGenObjectScheduler.h"

namespace Echo
{
    class WarfogRenderable;
    class PlanetLowLODCloud;
	class SphericalTerrain;
	class PlanetManager;
	struct RegionCoordinate;
	struct AtmosphereProperty;
	class SphericalTerrainAtmosphereRenderable;
	class SphereSnowQuadTreeMgr;
	class UberNoiseFBM3D;
	class TerrainModify3D;
	class TerrainStaticModify3D;
	class PlanetPlotDivision;
	class PlanetRoad;
	class EchoVolumetricCloud;
	namespace PCG {
		class PlanetGenObjectScheduler;
	};

	// Base class for generating mesh in physic export tool. DO NOT create any rendering related object.
	class _EchoExport ProceduralSphere
	{
	public:
		inline static float unitSphereArea = 12.566370f;
		using Node = SphericalTerrainQuadTreeNode;
		using Face = Node::Face;
		using Edge = Node::Edge;
		using Quad = Node::Quad;

		struct NodeInfo
		{
			int slice;
			int depth;
			int x;
			int y;
		};

		ProceduralSphere();
		virtual ~ProceduralSphere();
		bool init(const String& filePath);

		static NodeInfo getNodeInfo(int index, int maxDepth);
		static NodeInfo getNodeInfo(const Node& node);
		int getNodeIndex(const Node& node) const;
		int getNodeIndex(const NodeInfo& info) const;
        static int getNodeIndex(const NodeInfo& info, int maxDepth);

		float getBlockSize() const { return static_cast<float>(Node::getLengthOnSphere(m_maxDepth)) * m_radius; }
		int getNodesCount() const;

		static std::vector<int> getLeavesIndex(int maxDepth, int maxSlice);
		[[nodiscard]] std::vector<int> getLeavesIndex() const;

		[[nodiscard]] Vector3 getNodeOriginSphere(const NodeInfo& info, const TerrainGenerator* pGen = nullptr) const;
		[[nodiscard]] Vector3 getNodeOriginTorus(const NodeInfo& info) const;
		[[nodiscard]] Vector3 getNodeOriginLocal(int index) const;

		static std::vector<uint16> generateIndexBufferPhysX();

		void generateElevations(const Vector3* vertices, float* elevations, size_t size) const;
		[[nodiscard]] float generateElevation(const Vector3& vertex) const;

		void computeStaticModify(int nodeIndex, Vector3* vertices, const Vector3* baseVertices, float* elevations, size_t count) const;

		void applyElevations(Vector3* outVertices, const Vector3* inVertices, const float* elevations, size_t count) const;

		virtual std::vector<Vector3> generateVertexBufferPhysX(int index);
		bool getIndexCubeSurfaceBound(const int index, Vector3& position, Quaternion& rotation, Vector3& halfSize);

		using Fp16 = std::uint16_t;
		std::array<std::vector<Fp16>, 6> generateDetailedHeightmap() const;
		void outputDetailedHeightmap() const;

		static inline std::array<std::vector<Node::IndexType>, 16> s_indexData {};

		SphericalTerrainResourcePtr m_terrainData {};

		float getRadius() const { return m_radius; }

		bool isSphere() const { return m_topology == PTS_Sphere; }
		bool isTorus() const { return m_topology == PTS_Torus; }
		bool hasShapeGenerator() const { return m_pTerrainGenerator != nullptr; }

		int m_numSlice[2] { 6, 1 };
		int m_maxDepth = 0;

		float m_radius                       = 0.0f;
		float m_relativeMinorRadius          = 0.05f; // for torus
		float m_elevationRatio               = 0.0f;
		ProceduralTerrainTopology m_topology = PTS_Sphere;

		//NOTE:Before use this noise, use our seed to set it's state.
		std::unique_ptr<UberNoiseFBM3D> m_noise;
		std::unique_ptr<HeightMapStamp3D> m_stampterrain3D;
		std::unique_ptr<TerrainStaticModify3D> m_staticModifier;

		std::vector<OrientedBox> getFlatAreas() const;

		TerrainGenerator* m_pTerrainGenerator = nullptr;
		float m_terrainMinBorder = -1.0f;
		float m_terrainBorderSize = 0.0f;
		float m_oneOverTerrainBorderSize = 0.0f;
		float m_terrainBorderHeight = 0.0f;

		float m_noiseElevationMax = std::numeric_limits<float>::max();
		float m_noiseElevationMin = -1.0f;

		float applyTerrainBorder(const Vector3& vertice, float elevation) const;

		void applyNoiseState();
		void applyStaticModifiers();
	public:
		//Planets create internal physical objects.
		struct PhysicsObjectDesc {
			Name meshphy;
			std::vector<std::tuple<DVector3, Quaternion, Vector3, uint32>> transforms;
		};
		std::function<void*(const PhysicsObjectDesc& desc)> mNewPhysicsObjectFun = nullptr;
		std::function<bool(void*)> mDeletePhysicsObjectFun = nullptr;
	};

    struct CubeMapView
    {
		const Bitmap* faces[6] { nullptr };

		int size() const
		{
			return faces[0] ? faces[0]->size() * 6 : 0;
		}

		void reset()
		{
			for (auto& face : faces)
			{
				face = nullptr;
			}
		}

    	template <typename T>
		T getVal(const Vector3& sph) const
		{
			SphericalTerrainQuadTreeNode::Face face;
			Vector2 xy;
			SphericalTerrainQuadTreeNode::MapToCube(sph, face, xy);
			xy = xy * 0.5f + 0.5f;
			return getVal<T>(xy, face);
		}

    	template <typename T>
		T getVal(const Vector2& uv, const int face) const
		{
			assert(face >= 0 && face < 6);
			if (!faces[face] || !faces[face]->pixels) return T();
			return faces[face]->getVal<T>(uv);
		}
	};

	class _EchoExport SphericalTerrain :
		public ProceduralSphere,
		public ResourceBackgroundQueue::Listener
	{
	public:
		/// LvConfig
		static void setDetailDistance(float detailDistance);
		static float getDetailDistance();
		static void setLodRatio(float lodRatio);
		static float getLodRatio();

		void setForceLowLod(bool forceLod);
		bool isPlanetTaskFinished();
		void setVisible(bool visible);
		bool getVisible() const;

		void setHideTiles(bool hide);
		bool getHideTiles() const;

        //IMPORTANT:Must called after planet had completly initialized.
        void initLowLODCloud();
        void initWarfog();
		void initPlotDivision();


        
        //NOTE:Region warfog display
        void setEnableRegionVisualEffect(bool enable);
        void setDisplayCorseOrFineRegion(bool enableCorse);
        bool setHighlightCorseRegionID(int id);
        bool setHighlightFineRegionID(int id);
        void setRegionGlowColor(const ColorValue& color);
        
        void setRegionFillColor(const ColorValue& color);
        void setCoarseRegionFillColor(const ColorValue& color);

        void setSingleRegionFillColor(int id, const ColorValue& color);
        void setSingleCoarseRegionFillColor(int id, const ColorValue& color);
        
        void setRegionHighlightColor(const ColorValue& color);
        void setRegionHighlightWarfogColor(const ColorValue& color);
        void setWarfogMaskUVScale(const float scale);
        void setWarfogMaskScale(const float scale);
        void setOutlineSmoothRange(const float scale);

        void setAdditionOutLineColor(const ColorValue& color);
        void setAdditionOutLineAlpha(const float& v);
        void setAdditionOutLineWidth(const float& v);
        void setAdditionOutLineDistance(const float& v);
        

        void setOutlineWidth(float width);
        void setCommonRegionAlpha(float alpha);
        void setHighlightRegionAlpha(float alpha);

        void setRegionWarfogAlpha(int id, float alpha);
        void setCoarseRegionWarfogAlpha(int id, float alpha);
        
        void setRegionWarfogColor(const ColorValue& color);

        void setWarfogMaskTexture(const String& path);
		void setWarFogSelfEmissiveLightColor(const ColorValue& color_);
		void setWarFogSelfEmissiveLightPower(float power);

		void setWarFogIsUseFlowingLight(bool bUse);
		void setWarFogFlowingDirByYawAndPitch(float yaw, float pitch);
		void setWarFogFlowingDir(const Vector3& dir);
		void setWarFogFlowingWidth(float width);
		void setWarFogFlowingSpeed(float speed);
		void setWarFogFlowingColor(const ColorValue& color);
		void setWarFogFlowingHDRFactor(float factory_);

		void setWarFogDissolveTexture(const std::string& path);
		void setWarFogDissolveTillingScale(float u);
		void setWarFogDissolveSoftValue(float value);
		void setWarFogDissolveWidth(float width);
		void setWarFogDissolveColor(const Vector3& color);
		void setWarFogDissolveColorValue(float value);
		void setWarFogDissolveValue(int id ,float value);
        //NOTE:Low-lod Cloud
        void setEnableLowLODCloud(bool enalbe);
		bool getEnableLowLODCloud() { return m_enableLowLODCloud; }
        void setCloudUVScale(float v);
        void setCloudFlowSpeed(float v);
        void setCloudHDRFactor(float v);
        void setCloudMetallic(float v);
        void setCloudRoughness(float v);
        void setCloudAmbientFactor(float v);
        void setCloudNoiseOffsetScale(float v);
        void setCloudTexture(const String& path);
		void setCloudDirByPitchAndYaw(float pitch,float yaw);

		//NoTE:PlotDivision
		void setEnablePlotDivision(bool bEnable);
		bool getEnablePlotDivision() { return m_bEnablePlotDivision; }

		//PTS_Torus类型的星球 idx传0，x(0 - 1023)，y(0-255)|| PTS_Sphere idx 0 - 前 ，1 - 后， 2 - 左 ，3 - 右 ，4 - 上 ，5 -下 ，x(0-15),y(0-15)
		void setPlotDivisionColor(int idx, int x, int y, const ColorValue& color);
		void setPlotDivisionNums(int x,int y);

		bool getPlotDivisionDataByPosition(const Vector3& pos,int& idx,int& x,int& y);
		bool getPlotDivisionDataByWorldPosition(const DVector3& pos,int& idx,int& x,int& y);

		//VolumetricCloud
		void									setEnableVolumetricCloud(const bool& val);
		bool									getEnableVolumetricCloud()const { return  m_bEnableVolumetricCloud; }
		void									setLayerBottomAltitudeKm(const float& val);
		float									getLayerBottomAltitudeKm()const { return  m_fLayerBottomAltitudeKm; }
		void									setLayerHeightKm(const float& val);
		float									getLayerHeightKm()const { return  m_fLayerHeightKm; }
		void									setTracingStartMaxDistance(const float& val);
		float									getTracingStartMaxDistance()const { return  m_fTracingStartMaxDistance; }
		void									setTracingMaxDistance(const float& val);
		float									getTracingMaxDistance()const { return  m_fTracingMaxDistance; }

		void									setCloudBasicNoiseScale(const float& val);
		float									getCloudBasicNoiseScale()const { return  m_fCloudBasicNoiseScale; }
		void									setStratusCoverage(const float& val);
		float									getStratusCoverage()const { return  m_fStratusCoverage; }
		void									setStratusContrast(const float& val);
		float									getStratusContrast()const { return  m_fStratusContrast; }
		void									setMaxDensity(const float& val);
		float									getMaxDensity()const { return  m_fMaxDensity; }

		void									setWindOffset(const Vector3& val);
		Vector3									getWindOffset()const { return m_vWindOffset; }
		void									setCloudColor(const ColorValue& val);
		ColorValue								getCloudColor()const { return  m_cCloudColor; }

		float getSnowHeight(uint32_t matid);

		void									setCloudPhaseG(const float& val);
		float									getCloudPhaseG()const { return  m_fCloudPhaseG; }
		void									setCloudPhaseG2(const float& val);
		float									getCloudPhaseG2()const { return  m_fCloudPhaseG2; }
		void									setCloudPhaseMixFactor(const float& val);
		float									getCloudPhaseMixFactor()const { return  m_fCloudPhaseMixFactor; }
		void									setMsScattFactor(const float& val);
		float									getMsScattFactor()const { return  m_fMsScattFactor; }
		void									setMsExtinFactor(const float& val);
		float									getMsExtinFactor()const { return  m_fMsExtinFactor; }
		void									setMsPhaseFactor(const float& val);
		float									getMsPhaseFactor()const { return  m_fMsPhaseFactor; }
	protected:
		void applyToVolumetricCloud(const std::function<void(EchoVolumetricCloud*)>& func);
		void UpdateVolumetricCloudParam(const Camera* pCamera);

	protected:
		inline static float s_detailDistance = 1024.0f;
		inline static float s_lodRatio       = 1.0f;

	public:
		friend class SphericalTerrainFog;
		friend class PlanetManager;
        friend class SphericalTerrainQuadTreeNode;
		explicit SphericalTerrain(PlanetManager* mgr);
		~SphericalTerrain() override;

		struct GenerateVertexListener : CommonTaskListener
		{
			// shouldn't be a stack object
			void CommonTaskFinish(uint64 requestId) override;
			Node* node = nullptr;
		};

		struct GenerateVertexTask : CommonTask
		{
			// shouldn't be a stack object
			void Execute() override;
			Node* node = nullptr;
		};

		//NOTE(yanghang):Editor function.
		void biomeDefineChanged(const String& newBiomePath);
		// reload biome from memory, resource with the exact same name may already exist in global cache, need a flag for reloading
		void biomeDefineChanged(const BiomeComposition& newBiome,
                                const BiomeDistribution& newBiomeDist,
                                bool reload = false);

		// Run time set or unset Ocean
		void sphericalOceanChanged(bool setOcean);

		void reload3DNoiseChanged(Noise3DData& newData);
		Noise3DData get3DNoiseData();

		void loadHumidityClimateProcess(const ImageProcess& newIP, const AoParameter& newAP);
		void loadTemperatureClimateProcess(const ImageProcess& newIP);

		bool createImpl(const SphericalTerrainResourcePtr& resource);
		bool init(const String& filePath, bool bSync = true, bool terrainEditor = false, bool bAtlas = false);
		void shutDown();

		void createRoots();
		void generateMesh();
		void generateBiome();

		void findVisibleObjects(const Camera* pCamera, RenderQueue* pQueue);
		void updateGeometry(const Vector3& camL, float minRenderDistSqrL);
		void printLog(const Camera* pCamera);
		bool findPlanetVisible(const Camera* pCamera, RenderQueue* pQueue, int lod);

		void setDisplayMode(BiomeDisplayMode mode);
		
		void setWorldPos(const DBVector3& pos);
		void setWorldRot(const Quaternion& rot);
		void setWorldTrans(const DTransform& trans, const Vector3& scale);

		void setRadius(float radius);
		void setRadius(float major, float minor);
		void setTerrainGeneratorType(uint32 type);

		void onWorldTransformChanged();
		float getMaxRealRadius() const;
		float getMinRealRadius() const;

		void setElevation(float elevation);
		void setMaxDepth(int maxDepth);
		void setSliceCount(int x, int y);

		std::vector<Vector3> generateVertexBufferPhysX(int index) override;
		std::vector<Vector3> generatePositionWithCache(const NodeInfo& info, int index);

		void generateVertexBufferBenchmark();
		void surfaceQueryBenchmark() const;

		void createMaterial();
		void destroyMaterial();
		void applyGeometry();
		void bindTextures();

        //IMPORTANT(yanghang):Editor use only for export material map.
        void forceAllComputePipelineExecute();
		
		MaterialWrapper m_material;
		MaterialWrapper m_material_low_LOD;

		static void createIndexBuffer();
		static void destroyIndexBuffer();

		// Index buffer, will be used in findVisible object
		// each terrain tile should be split into two meshes according to vertex normal direction
		// Top, Bottom, Left, Right sides degenerated or not equals 16 types
		static inline std::array<RcBuffer*, 16> s_tileIndexBuffers = { nullptr };
		static inline int s_tileIndexCount[16] = {};

		static inline int s_count = 0;

		SphericalOcean* getOcean(){return m_ocean;}
		// m_haveOcean control: findvisable and setposition
		bool m_haveOcean = false;
		SphericalOcean* m_ocean = 0;

		// updated by component connection
		//AtmosphereApproxProfile* m_atmosApproxProfile	= nullptr;
		//AtmosphereBoundary* m_atmosBoundary				= nullptr;
		//bool m_atmosEnable								= false;

		void disableAtmosphere();
		AtmosphereProperty* m_atmosProperty = nullptr;
		bool m_atmosProerptyDirty = true;
		bool atmosInitSuccess = false;
		std::unique_ptr<SphericalTerrainAtmosphereRenderable> m_atmos;

		class DeformSnow* m_deformSnow = nullptr;
		std::vector<std::unique_ptr<Node>> m_slices {};
#if ECHO_EDITOR
		mutable std::shared_mutex m_heightQueryMutex;	// Only in editor roots will be recreated.
#endif
		// for far distance rendering
		std::unique_ptr<Node> m_planet = nullptr;
		static inline std::array<RcBuffer*, 4> s_planetIndexBuffers = { nullptr };
		static inline int s_planetIndexCount[4] {};

		std::map<int, Node*> m_leaves;
		mutable std::shared_mutex m_leafMutex;
		std::vector<AxisAlignedBox> m_vegetationRecheckRange;

		using VegetationQueryFunc = std::function<std::vector<bool>(std::span<Vector3>)>;
		bool isVegetationCheckValid() const;
		void setVegetationCheckFunction(VegetationQueryFunc&& func);
		VegetationQueryFunc m_vegetationCheckFunction;
		mutable std::shared_mutex m_vegetationCheckMutex;

		std::vector<bool> getOnSurface(std::span<Vector3> worldPositions) const;

		RcSampler* getBiomeIdTexture(int slice);

		CubeMapView m_materialIndexMap {};
#ifdef ECHO_EDITOR
		mutable std::shared_mutex m_biomeQueryMutex;	// Only in editor biome ID will be regenerated.
		std::array<Bitmap, 6> m_generatedMaterialMap;
#endif

		//AtmosphereProperty m_atmosphereProperty;

		static constexpr int BiomeTextureResolution = 512;
		static constexpr int BiomeCylinderTextureWidth = BiomeTextureResolution * 4;
		// Torus Region Parameters
		static constexpr int BiomeTorusTextureHeight = 640;
		static constexpr int BiomeTorusRegionMaxH = 9; // BiomeTorusRegionMaxH * (BiomeTorusRegionRatio * BiomeTorusRegionMaxH) <= 256
		static constexpr int BiomeTorusRegionRatio = 3;

		static constexpr uint8_t MaxMaterialTypes  = 16u;
		static constexpr uint8_t MaterialTypeMask  = 0b00001111u;
		static constexpr uint8_t MaterialOceanMask = 0b10000000u;

		//ToDo:这里的实现不太对，后续应改成存TexturePtr成员变量，异步加载
		//用于打乱不同材质边界的贴图，全局只需要一张。先暂时放在这里
		static inline TexturePtr s_fbmNoiseTexture;
		static inline Name s_fbmPathStr = Name("echo/biome_terrain/Textures/fbm.dds");

		static TexturePtr loadFbmTexture();
		static void freeFbmTexture();

		//static inline TexturePtr s_voronoiTexture;
		//static inline Name s_voronoiPathStr = Name("echo/biome_terrain/Textures/T_Voronoi_Perturbed.dds");

		//static TexturePtr loadVoronoiTexture();
		//static void freeVoronoiTexture();

#ifdef ECHO_EDITOR
		void createComputePipeline();
		void destroyComputePipeline();
		bool isComputePipelineReady() const;

		void createComputeTarget();
		void destroyComputeTarget();

		RcComputeTarget** getTemperatureComputeTargets();
		RcComputeTarget** getHumidityComputeTargets();
		RcComputeTarget** getMaterialIndexComputeTargets();

		void getWorldMaps(Bitmap* elevation = nullptr, Bitmap* standardDeviation = nullptr);

		void computeElevationGPU();
		void computeTemperatureGPU();
		void computeHumidityGPU();
		void computeMaterialIndexGPU();

		std::array<TexturePtr, 6> m_ManualSphericalVoronoiTex;
		std::array<TexturePtr, 6> m_sphericalVoronoiTexturesWC;
		std::array<TexturePtr, 6> m_sphericalVoronoiCoarseTexturesWC;

		Vector4 m_MLSphericalVoronoiParams{ Vector4::ZERO };
		Vector4 m_ManualParams{ Vector4::ZERO };
		
		std::vector<int> m_adjacent_coarse_regions;
		std::vector<int> m_adjacent_fine_regions;
		void markSphericalVoronoiRegionId(int id);
		void updateSphericalVoronoiRegionInternal();
		

		bool bDisplayCenter = false;
		std::string currentFineMap;
		std::string currentCoarseMap;
		void markSphericalVoronoiRegionCenter(bool IsEnable);
		bool bDisplayCoarseLayer = false;
		std::vector<int> GetHierarchicalSphericalVoronoiParents() const;

		
		void computeVertexGpu(RcBuffer* vb, const Node& node);

private:
		struct PcgPipeline;
		std::unique_ptr<PcgPipeline> m_pcg;
#endif
public:
		struct SurfaceAreaInfo
		{
			
			static constexpr int TotalSphere = BiomeTextureResolution * BiomeTextureResolution * 2 + BiomeTextureResolution * (BiomeTextureResolution - 2) * 2 + (BiomeTextureResolution - 2) * (BiomeTextureResolution - 2) * 2;
			static constexpr int TotalTorus = BiomeCylinderTextureWidth * BiomeTorusTextureHeight;
			bool mPrepared = false;
			float mArea = 0.0f;
			std::string mPrefix = "";
			std::vector<float> mSurfaceArea;

			inline bool match(const float Area, const std::string& prefix) const
			{
				return mArea == Area && prefix == mPrefix;
			}

			void prepare(const float Area, const std::string& prefix);
		};
		SurfaceAreaInfo mSaInfo;
		float getSurfaceAreaRatio(int id);
		float getSurfaceArea() const;
		float getSurfaceArea(int id);
		float getCoarseSurfaceArea(unsigned int id);
		
		std::vector<int> GetCoarseLayerAdjacentRegions(int id) const;
		std::vector<int> GetFineLayerAdjacentRegions(int id) const;
		std::vector<Vector3> GetCoarseLayerCenters(bool ToWorldSpace) const;
		std::vector<Vector3> GetFineLayerCenters(bool ToWorldSpace) const;
		
		// Useful for rendering
		std::array<TexturePtr, 6> m_LoadedSphericalVoronoiTex;
		// Geometry heatmap textures (one per cube face), loaded on demand
		mutable std::array<TexturePtr, 6> m_GeometryHeatmapTex;

        bool m_enableRegionVisualEffect = false;

		TexturePtr SelectPlanetRegionRef(const int slice) const;
		TexturePtr SelectGeometryHeatmapRef(const int slice) const;
		Vector4 getPlanetRegionParameters() const;
		static bool CreatePlanetRegionTexture(const std::array<Bitmap, 6>& InMaps, std::array<TexturePtr, 6>& OutTextures);
		void displaySphericalVoronoiRegion(bool mode);

		
		static int computeSphericalRegionParentId(const Vector3& localcoord, const std::array<Bitmap, 6>& relateds);

		static bool PointToCube(const Vector3& InPoint, int& face, Vector2& uv);
		bool PointToPlane(const Vector3& InPoint, int& face, Vector2& uv, bool isL) const;
		Vector3 PointToLocalSpace(const Vector3& InPoint) const;
		Vector3 PointToWorldSpace(const Vector3& InPoint) const;
		int computeSphericalRegionId(const Vector3& worldcoord) const;
		int computePlanetRegionId(const Vector3& worldpos) const;
		int computeRegionInformation(const Vector3& worldcoord) const;

		[[nodiscard]] static RcBuffer* createRawVb();
		[[nodiscard]] static RcBuffer* createDynamicVb(const std::vector<TerrainVertex>& vertices);

		void computeVertexCpu(Node& node);
		static std::vector<Vector3> computeNormal(size_t width, size_t height, const std::vector<Vector3>& positionWithRing);
		[[nodiscard]] static std::vector<TerrainVertex> assembleVertices(const std::vector<Vector3>& pos, const std::vector<Vector3>& nor, const std::vector<Vector2>& uv);

		//vertex data is needed to split mesh
		[[nodiscard]] static std::vector<TerrainVertex> copyVertexBufferFromGpu(RcBuffer* vb);

		std::unique_ptr<TerrainModify3D> m_modifier;
		void addModifier(uint64_t id, float width, float height, const DVector3& position, float yaw);
		void addModifier(uint64_t id, float radius, const DVector3& position);
		void removeModifier(uint64_t id);
		void addModifierToNodes(uint64_t id);
		bool testModifierIntersect(uint64_t id, int node);
		bool containsModifier(uint64_t id) const;

		void generateVertexBuffer(Node& node);
		void cancelGenerateVertexTask(const Node& node);
		void cancelGenerateVertexTask();

		void subdivideNode(Node& node);
		bool canSubdivideNode(const Node& node);
		void unifyNode(Node& node);
		bool canUnifyNode(const Node& node);

		using VertexGenTriplet = std::tuple<GenerateVertexListener, GenerateVertexTask, uint64_t>;
        std::unordered_map<const Node*, VertexGenTriplet> m_genVertexRequests;
        int m_currFrameRequests = 0;

		static constexpr bool m_gpuMode = true;

		bool m_vbGenAsync                 = false;
		bool m_biomeDirty                 = true;
		bool m_biomeDefineDirty           = true;
		bool m_bStampTerrain              = false;
		bool m_useObb                     = false;
		bool m_editingTerrain             = false;
		bool m_showSphericalVoronoiRegion = false;
		bool m_useSphericalVoronoiRegion  = true;
		bool m_lowLod                     = false;
		bool m_useLowLODMaterial          = false;
		bool m_forceLowLOD                = false;
		bool m_bDisplay                   = false;
		bool m_visible                    = true;
		bool m_bAtlas					  = false;
		bool m_hideTiles				  = false;

		bool updateStampTerrain();
		bool updateStampTerrain_impl();

		int getSurfaceTypeWs(const DVector3& worldPos, bool* underwater = nullptr) const;
        int getSurfaceTypeLs(const Vector3& localPos, bool* underwater = nullptr) const;
		int getSurfaceType(const Vector2& localPos, int slice, bool* underwater = nullptr) const;
		ColorValue getSurfaceColor(uint8_t surfaceType) const;
        uint32 getPhySurfaceType(const DVector3& worldPos) const;

		bool getIsNearbyLs(const Vector3& localPos) const;
		bool getIsNearbyWs(const Vector3& worldPos) const;

		Vector3 getGravityLs(const Vector3& localPos) const;
		Vector3 getGravityWs(const Vector3& worldPos) const;

		Quaternion getCardinalOrientation(const Vector3& localPos) const;
		Quaternion getCardinalOrientation(const DVector3& worldPos) const;

		Vector3 getOriginLs(const Vector3& localPos) const;

		double getElevation(const DVector3& worldPos) const;
		float getElevation(const Vector3& localPos) const;

		Vector3 getUpVectorLs(const Vector3& localPos, Vector3* basePos = nullptr) const;
		Vector3 getUpVectorLs(const Vector2& xy, int slice, Vector3* basePos = nullptr) const;
		Vector3 getUpVectorWs(const DVector3& worldPos) const;

		//demo
		//Vector3 getGroundPos(const DVector3& worldPos);

		DVector3 getSurfaceWs(const DVector3& worldPos, Vector3* faceNormal = nullptr) const;
		DVector3 getSurfaceWs(const Vector2& xy, int slice, Vector3* faceNormal = nullptr) const;
		Vector3 getSurfaceLs(const Vector3& localPos, Vector3* faceNormal = nullptr, int* depth = nullptr) const;
		std::array<Vector3, 3> getSurfaceTriangleLs(const Vector2& xy, int slice, int* depth = nullptr) const;

		DVector3 getSurfaceFinestWs(const DVector3& worldPos, Vector3* faceNormal = nullptr) const;
		Vector3 getSurfaceFinestLs(const Vector3& localPos, Vector3* faceNormal = nullptr) const;

		float getDistanceToSurface(const Vector3& planetPos) const;

		int getSurfaceMipWs(const DVector3& worldPos) const;
		int getSurfaceMipLs(const Vector3& localPos) const;

		void getSurfaceTriangleFinestLs(const Vector3& localPos, Vector3& p0, Vector3& p1, Vector3& p2) const;
		void getSurfaceTriangleFinestWs(const DVector3& worldPos, DVector3& p0, DVector3& p1, DVector3& p2) const;

		DVector3 getOceanSurface(const DVector3& worldPos) const;
		DVector3 getOceanWaveSurface(const DVector3& worldPos) const;
		bool isUnderOcean(const DVector3& worldPos, double* depth = nullptr) const;
		bool isUnderOceanWave(const DVector3& worldPos, double* depth = nullptr) const;

		float getBoarderValue(const Vector3& localPos) const;
		float getBoarderValue(const DVector3& worldPos) const;

		void vegetationRecheck(const AxisAlignedBox& box);

#ifdef ECHO_EDITOR
		void showElevation(bool show);
		void showTemperature(bool show);
		void showHumidity(bool show);
		void showGlobalViews(RenderQueue* pQueue);
		void updateStampTerrainDisplay(bool bDisplay, const Matrix4& mtx);

		void computeBoundCpu();
		void copyMaterialIndexFromGpuMemory();
#endif

		float m_elevationMax = std::numeric_limits<float>::max();
		float m_elevationMin = -1.0f;

        //NOTE: Editor helper purpose.
        //TODO: Remove from game release.
        BiomeDisplayMode m_displayMode = BiomeDisplayMode::Full;

		//NOTE: World transform.
		Quaternion m_rot        = Quaternion::IDENTITY;
		DBVector3 m_pos         = DVector3::ZERO;
		Vector3 m_scale         = Vector3::UNIT_SCALE;
		Vector3 m_realScale		= Vector3::UNIT_SCALE;
		Vector3 m_realScaleInv  = Vector3::UNIT_SCALE;
		DBMatrix4 m_worldMat    = DMatrix4::IDENTITY;
		DBMatrix4 m_worldMatInv = DMatrix4::IDENTITY;

		int m_biomeTextureWidth  = 0;
		int m_biomeTextureHeight = 0;

		DVector3 ToWorldSpace(const Vector3& localPos) const { return m_worldMat * static_cast<DVector3>(localPos); }
		Vector3 ToLocalSpace(const DVector3& worldPos) const { return m_worldMatInv * worldPos; }

		// size : 1 + number of slice * (1 + 4 + 16 + 64 + 256 + ...)
		std::vector<AxisAlignedBox> m_bounds;
		bool isBoundAccurate() const { return !m_bounds.empty(); } // using accurate precomputed bounds
		std::vector<int> getLeavesInWs(const AxisAlignedBox& box) const;
		std::vector<int> getLeavesInLs(const AxisAlignedBox& box) const;
		std::vector<int> getTreeNodeInLs(const AxisAlignedBox& box, int _depth) const;
		const AxisAlignedBox& getIndexBound(int index) const;

		std::unordered_map<int, std::vector<uint32>> calculateNodeIntersect(const std::vector<AxisAlignedBox>& boxes) const;

        //单套材质相关系数
		float m_tillingParam = 20.f;
		float m_normalStrength = 2.f;

		//NOTE by hejiwei: 为了使三平面投影的范围减小，这两个参数改为固定的5与0.57
		//三平面投影计算权重需要的系数 每个方向的权重为 pow(abs(Normal) - delta, m)
		float m_weightParamM = 5.f;
		float m_weightParamDelta = 0.57f;

		Vector3 getGravityDirection(const Vector3& currentPos) { return currentPos - m_pos; }
		float getCurrentHeight(const Vector3& currentPos) { return (currentPos - m_pos).length() - m_radius; }

		void setDissolveStrength(float value);
		float getDissolveStrength() { return m_fDissolveStrength; }

        // TODO(yanghang): Seemed that no need light source.
        inline static LightList	s_lightList {};

        Matrix4 m_StampTerrainMtx{ Matrix4::IDENTITY };

        PlanetProfile m_profile = {};

		class BiomeVegetation* m_vegHandle = nullptr;
		class BiomeVegGroup* m_vegGroupHandle = nullptr;
	public:
		class LoadListener {
		public:
			virtual ~LoadListener() = default;
			virtual void OnCreateFinish() {};
			virtual void OnDestroy() {};
		};
		typedef vector<LoadListener*>::type LoadListenerVec;

		void addLoadListener(LoadListener* listener);
		void removeLoadListener(LoadListener* listener);
		bool isCreateFinish() { return m_bCreateFinish; }
		bool isWaitCreateFinish() { return m_bWaitCreateFinish; }

        PlanetManager* getMgr(){return m_mgr;}

        WarfogRenderable* getWarfog(){return m_warfog;}


		//远景云相关
		int32_t getPlanetCloudId() { return m_CloudTemplateId; }
		void    setPlanetCloudId(int32_t id);
	protected:
		void onCreateFinish();
		void onDestroy();
		void operationCompleted(BackgroundProcessTicket ticket, const ResourceBackgroundQueue::ResourceRequest& request, const ResourceBasePtr& resourcePtr) override;
		void requestResource(const Name& name);
		void clearRequest();
	private:
		LoadListenerVec m_LoadListeners;
		bool m_bCreateFinish = false;
		bool m_bWaitCreateFinish = false;
		BackgroundProcessTicket mLoadTicket = 0;

        PlanetManager* m_mgr = nullptr;

        bool m_isRequestedBiomeTexture = false;
        bool m_bindGPUBiomeTexture = false;

        WarfogRenderable* m_warfog = 0;
        PlanetLowLODCloud* m_cloud = 0;
        bool m_enableLowLODCloud = false;

		int32_t  m_CloudTemplateId = -1;

		PlanetPlotDivision*  m_PlotDivision = nullptr;
		bool                 m_bEnablePlotDivision = false;

		float m_fDissolveStrength = 0.0f;

		////VolumetricCloudParam
		float m_fLayerBottomAltitudeKm = 1.50f;				//云高度(距离地表)
		float  m_fLayerHeightKm = 1.0f;						//云厚度
		float  m_fTracingStartMaxDistance = 30.0f;
		float  m_fTracingMaxDistance = 20.0f;

		//Material
		float  m_fCloudBasicNoiseScale = 0.03f;
		float  m_fStratusCoverage = 0.45f;
		float  m_fStratusContrast = 0.5f;
		float  m_fMaxDensity = 0.03f;
		Vector3 m_vWindOffset = Vector3::ZERO;
		ColorValue m_cCloudColor = ColorValue(1.0f, 1.0f, 1.0f, 1.0f);

		//VolumetricAdvanced
		float m_fCloudPhaseG = 0.5f;
		float m_fCloudPhaseG2 = -0.5f;
		float m_fCloudPhaseMixFactor = 0.5f;
		float m_fMsScattFactor = 1.419f;
		float m_fMsExtinFactor = 0.1024f;
		float m_fMsPhaseFactor = 0.2453f;

		bool m_bVolumetricCloudProerptyDirty = true;

		bool m_bEnableVolumetricCloud = false;

	public:
		void	BuildPlanetRoad(bool sync = false, bool reBuild = false);//reBuild:如果已经构建数据，是否重新构建
		PlanetRoad* getPlanetRoad() { return mRoad; };

		PlanetRoad* mRoad = nullptr;
#ifdef ECHO_EDITOR
	public:
		void	updateBiomePercentage();
		std::vector<float> biomePercentage;

		void	initBiomePercentageCache();
		void	updateBiomePercentageCache(int index);
		std::vector<std::array<float, 2>> biomePercentageCache;

		void	updateSeaSurfaceTemperature();
		mutable float m_SeaSurfaceTemperature = 0.0f;
#endif
	public:
		bool exportPlanetGenObjects(const std::string& folderPath);
		bool exportPlanetGenObjects();
		void resetPlanetGenObject();
		bool existPlanetGenObject() const { return m_pPlanetGenObjectScheduler != nullptr; }
		bool addDeletedGenObject(uint64 id);
		bool removeDeletedGenObject(uint64 id);
		bool getAllDeletedGenObject(std::set<uint64>& ids);
		bool hideGenObjects(const std::set<uint64>& ids);
		bool showGenObjects(const std::set<uint64>& ids);
		bool queryGenObjects(const DVector3& worldPos, const Quaternion& worldRot, const Vector3& halfSize, std::set<uint64>& objIds, bool checkVisible = false);
		bool queryGenObjects(const DVector3& worldPos, float radius, std::set<uint64>& objIds, bool checkVisible = false);
		bool getGenObjectInfo(uint64 id, GenObjectInfo& info);
		PCG::PlanetGenObjectScheduler* getPlanetGenObjectScheduler() { return m_pPlanetGenObjectScheduler; }
	private:
		PCG::PlanetGenObjectScheduler* m_pPlanetGenObjectScheduler = nullptr;
		static std::function<PCG::PlanetGenObjectScheduler* (SphericalTerrain*)> gNewPlanetGenObjectScheduler;
		friend class PCGSystem;
	};
}

#define ECHO_SPHERICAL_TERRAIN_MANAGER_H 
#endif
