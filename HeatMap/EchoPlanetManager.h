//-----------------------------------------------------------------------------
// File:   EchoBiomeTerrainManager.h
//
// Author: yanghang
//
// Date:  2024-4-7
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoImage.h"
#if !defined(ECHO_BIOME_TERRAIN_MANAGER_H)
#include "EchoPrerequisites.h"

#include "EchoBiomeTerrainBlock.h"

#include "EchoSphericalTerrainResource.h"
#include "JobSystem/EchoCommonJobManager.h"

namespace Echo
{
	class SphericalOcean;
    void EchoLogToConsole(const String& log);
    
    struct PlanetVertex
    {
        Vector3 pos;
        Vector3 normal;
        Vector3 uv;
    };
    
    struct PlanetProfile
    {
        uint64 meshMemory;
        uint64 texMemory;
        uint64 blockMemory;
        
        uint64 generateMeshTime;
        uint64 generateTexTime;
        uint64 generatePhysxMeshTime;
    };

    struct LookUpMapPair
	{
		TexturePtr GpuData;
		Bitmap CpuData;
	};
    
    //NOTE:Forward decalration
    class SphericalTerrain;

    struct AsyncSphereGenerator:public CommonTaskListener,
                                 public CommonTask
    {
        virtual void CommonTaskFinish(uint64 requestID)override;
        virtual void Execute()override;
        RcBuffer* vertexBuffer = 0;
        
        int gempType = -1;
        std::vector<PlanetVertex> tempVertices;
    };

    struct AsyncTorusGenerator:public CommonTaskListener,
                              public CommonTask
    {
        virtual void CommonTaskFinish(uint64 requestID)override;
        virtual void Execute()override;
        RcBuffer* vertexBuffer = 0;

        float relativeMinorRadius = 0;
        float elevationMax = 0;
        
        std::vector<PlanetVertex> tempVertices;

        struct Listener
        {
            virtual void taskFinish() = 0;
        };

        Listener* listener = 0;
    };

    struct SphereBakeTextureParam
    {
        float lowMaxRadius = 4000.0f;
        float middleMaxRadius = 8000.0f;

        struct Resolution
		{
			uint32 width;
            float hlodRadius;
		};
        
        Resolution lowResolution = {64, 2.0f};
        Resolution middleResolution = {128, 2.0f};
        Resolution highResolution = {256, 2.0f};
    };
    
    struct TorusBakeTextureParam
    {
        float lowMaxRadius = 3000.f;
        float middleMaxRadius = 6000.f;
        
		struct Resolution
		{
			uint32 width;
            float hlodRadius;
		};
        Resolution lowResolution = {256, 2.0f};
        Resolution middleResolution = {512, 2.0f};
        Resolution highResolution = {1024, 2.0f};  
    };

    
    class _EchoExport PlanetManager:
        public WorkQueue::RequestHandler,
        public WorkQueue::ResponseHandler,
		public WorkQueue::WorkQueueOutPutLog
    {
public:
        PlanetManager(SceneManager* mgr);
        ~PlanetManager();

        void displayTerrain(bool visible);        

        void init();
        void uninit();

        void setDisplayMode(BiomeDisplayMode mode);
		void displaySphericalVoronoiRegion(bool mode);

        SphericalTerrain* createSphericalTerrain(const String& terrain, bool bSync = true, bool terrainEditor = false, bool bAtlas = false, const DTransform& trans = DTransform::IDENTITY, const Vector3& scale = Vector3::UNIT_SCALE);
		bool destroySphericalTerrain(SphericalTerrain* pTerrain);
		SphericalTerrain* getNearestSphericalTerrain(const DVector3& worldPosition) const;
        
        //IMPORTANT(yanghang):Common function
        static Bitmap ComputeReverse(const HeightMap& heightMap, float minVal, float maxVal);
        static Bitmap ComputeOcclusion(const HeightMap& heightMap, float amplitude);
		static Bitmap ComputeMaterialIndex(const Bitmap& humidity,
                                           const Bitmap& temperature,
                                           const BiomeComposition& biomeCompo,
                                           const BiomeDistribution& biomeDist);
        
        static Bitmap ComputeBakedAlbedo(const BiomeComposition& biomeCompo,
                                         const Bitmap& biomeMap,
                                         const SphericalOceanData& oceanData,
                                         int width, int height,
                                         bool flipY = false);
		static Bitmap ComputeBakedEmission(const BiomeComposition& biomeCompo,
										   const Bitmap& biomeMap,
										   const SphericalOceanData& oceanData,
										   int width, int height,
                                           bool flipY = false);
		static std::vector<ABGR> CreateCubeImage(Image cubeImages[6], int mips = 1);
        static std::vector<ABGR> CreateImageMip(Image& cubeImages);

        static void useClimateProcess(Bitmap& originMap,
                                      Bitmap& map,
                                      TexturePtr& tex,
                                      ImageProcess& ip);

        //NOTE:Climate  process.
        static Bitmap climateProcess(const Bitmap& bitmap, ImageProcess process);
        
        static bool readHeightMap(HeightMap* out_map, const char* path);
		static bool readBitmap(Bitmap& out_map, const char* path);
		static void writeBitmap(const Bitmap& map, const char* path);
            
        //NOTE: Call when biome changed.
        //TODO(yanghang):Cached this look up map for reuse.
        static TexturePtr generateTextureFromBitmap(const Bitmap& mp);
        
        static LookUpMapPair generateBiomeLookupMap(const BiomeDistribution& biomeData,
                                                    const BiomeComposition& compo,
                                                    bool generateTex, int size = BiomeGPUData::lookupTexWidth);
#ifdef ECHO_EDITOR
		static LookUpMapPair generateRegionLookupMap(const RegionData& regionData, bool generateTex);
#endif
        
        // IMPORTANT(yanghang): 
        void findVisibleObjects(const Camera* pCamera, RenderQueue* pQueue);

        void updateCurrentPlanet(const Camera* pCamera);

        //NOTE(yanghang):Test only.
        void testLoadPlanet(bool load);

        SphericalTerrain* getCurrentPlanet();
		SphericalOcean* getCurrentOcean();

        SphericalTerrain* getNearestPlanet();
		SphericalTerrain* getNearestPlanetToSurface();
		void enableOceanSSR(bool enable);

        //Texture async load.
        virtual bool canHandleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ)	override
		{ return true; }
		virtual bool canHandleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ) override
		{ return true; }
        virtual WorkQueue::Response* handleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ) override;
		virtual void handleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ) override;
		virtual void OutPutPrepareResourceLog(DataStreamPtr& out) override;
        TexturePtr asyncLoadTexture(const Name& path);

        static void getSphereGeometryBuffer(int gempType, std::vector<PlanetVertex>& outVertices);
        static void getTorusGeometry(float relativeMinorRadius,
                                     float elevationMax,
                                     std::vector<PlanetVertex>& torusWarfogVertex);
        static void freeGlobalSphereGeometryResource();
        static void initGlobalSphereGeometryResource();

        static bool isBufferStartedGenerated(int gempType);
        static bool isBufferReady(int gempType);
        static RcBuffer* getBuffer(int gempType);

		static AxisAlignedBox calculateBounds(const Vector3 bezier[4]);

        
        //NOTE:Global planet baking resolution texture param.
        static void setSphereBakeTextureParam(const SphereBakeTextureParam& param);
        static void setTorusBakeTextureParam(const TorusBakeTextureParam& param);
        
        static uint32 getTorusBakeTextureResolution(float radius);
        static uint32 getSphereBakeTextureResolution(float radius);
        static float getSphereHlodRadius(float planetRadius);
        static float getTorusHlodRadius(float radius);
        
public:

        bool m_displayTerrain = true;
		bool m_showSphericalVoronoiRegion = false; 

        PlanetProfile m_totalProfile;
        uint64 m_drawTriangleCount = 0;

        std::set<SphericalTerrain*> m_SphericalTerrainSet;

        bool m_printProfileData = false;
        bool m_showDebugBoundingbox = false;
        bool m_showModifierBoundingBox = false;

        bool m_testReadbackBuffer = false;
        bool m_testGenerateBuffer = false;
		bool m_testQuerySurface = false;
		bool m_testOceanQuerySurface = false;
		bool m_testAddModifyTerrain = false;
		bool m_testAddModifyTerrainCircle = false;
		bool m_testRemoveModifyTerrain = false;

		bool m_enableOceanSSR = false;

        //NOTE(yanghang):When camera close to planet. then TOD control visual
        // star work.
        SphericalTerrain* m_currentPlanet;

        //NOTE(yanghang):Record nearest planet in the universe. for fog effect use.
        SphericalTerrain* m_nearestPlanet;
		SphericalTerrain* m_nearestPlanetToSurface;

		SceneManager* m_mgr = nullptr;

        //NOTE:Async load texture for planet.
        WorkQueue *mWorkQueue = 0;
		uint16 mWorkQueueChannel = 0;

		//For Render distant ocean
		class DistantPlanetOceanManager* m_distantOceanMgr;

        static std::unordered_map<int, RcBuffer*> s_sphereVertexBuffers;
        static std::unordered_set<int> s_bufferStartGenerated;

        //NOTE:Global planet baking resolution texture param.
        static SphereBakeTextureParam s_sphereParam;
        static TorusBakeTextureParam s_torusParam;
    };

    void outputProfile(const String& name, PlanetProfile& profile);
    
    struct GenerateRegionSDFResult
    {
        BitmapCube sdfCube;
        BitmapCube coarseSDFCube;
        BitmapCube mergedIDCube;
    };
    
    _EchoExport GenerateRegionSDFResult
    GeneratePlanetRegionSDF(BitmapCube& regionIDCube,
                            const std::vector<SphericalVoronoiRegionDefine>& defines);
    _EchoExport GenerateRegionSDFResult
    GeneratePlanetRegionSDF(BitmapCube& regionIDCube,
                            const std::vector<int>& parentLookupTable);

    _EchoExport BitmapCube
    GenerateCoarseRegion(BitmapCube& regionIDCube,
                         const std::vector<SphericalVoronoiRegionDefine>& defines,
                         int slices);

	struct GenerateRegionSDFResultPlane
	{
		Bitmap fineSdf;
		Bitmap coarseSdf;
		Bitmap mergedId;
	};

	_EchoExport GenerateRegionSDFResultPlane GeneratePlanetRegionSDFWarp(const Bitmap& regionMap,
	                                                                     const std::vector<int>& parentLookupTable);
}

#define ECHO_BIOME_TERRAIN_MANAGER_H
#endif
