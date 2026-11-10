#ifndef __EchoRoadManager_H__
#define __EchoRoadManager_H__

#include "EchoComponentPrerequisites.h"
#include "EchoSingleton.h"
#include "EchoWorkQueueBase.h"
#include "EchoPiecewiseBezierComponent.h"

namespace Echo
{
	class CrossRoadComponent;
	class SphericalTerrain;
	struct VegRoadQueryPackage;
	struct PlanetRoadData;

	enum ListenerType : unsigned short
	{
		BezierLink_Listener = 0b1,
		PavementPhyCreate_Listener = 0b10,
		CrossRoadLink_Listener = 0b100,
		All_Listener = BezierLink_Listener | PavementPhyCreate_Listener | CrossRoadLink_Listener
	};

#ifdef ECHO_EDITOR
	class AllComponentData
	{
	public:
		void UpdateBezierComponent();
		void UpdateCrossRoadComponent();

		std::vector<PiecewiseBezierComponent*> m_vecBeziers;
		std::vector<CrossRoadComponent*> m_vecCrossRs;
	};
#endif // ECHO_EDITOR

	struct BezierData
	{
		std::vector<std::vector<Vector3>>	mPoints;
		PiecewiseBezierComponent*			m_pBezier;
		std::vector<float>					mOffsetY;
	};

	struct RoadBezierData
	{
		std::vector<std::array<Vector3, 4>> beziers;
		std::vector<std::array<Vector3, 2>> normals;

		//Quaternion worldquat = Quaternion::IDENTITY;
		float Lwidth = 15.0f;
		float Rwidth = 15.0f;
	};

	struct RoadRangeData
	{
		SphericalTerrain* planet = nullptr;
		std::vector<RoadBezierData> roads;
		std::vector<AxisAlignedBox> boxs;
		bool removeVeg = true;

		friend std::ostream& operator << (std::ostream& o, const RoadRangeData& r) {
			return o;
		}
	};

	struct BuildingRangeData
	{
		struct BuildingData
		{
			AxisAlignedBox box;
			Vector3 worldPos = Vector3::ZERO;
			Quaternion worldQuat = Quaternion::IDENTITY;
		};
		SphericalTerrain* planet = nullptr;
		std::vector<BuildingData> builds;
		bool removeVeg = true;

		friend std::ostream& operator << (std::ostream& o, const BuildingRangeData& r) {
			return o;
		}
	};

	class TreeManager;

	class _EchoComponentExport RoadManager : public Singleton<RoadManager>, public FrameListener
	{
	public:
		RoadManager();
		virtual ~RoadManager();

		virtual bool frameEnded(const FrameEvent& evt) override;

		bool IsAdsorptionOperation();
		void AdsorbedRoad();

		PiecewiseBezierComponent* FindBezierInVec(String pEditorName, String pComName);
		bool SetBezierVec(PiecewiseBezierComponent* p_bezier);
		bool SetPavementVec(PavementComponent* p_pavement);
		bool SetCrossRoadVec(CrossRoadComponent* p_crossroad);

		void ClearUtilKeyDatas(const String& n);

		int CalculateCrossRoadPointAndVec(const MeshPtr& p_mesh, const std::vector<std::vector<Vector3>>& p_vertexData, std::vector<Vector3>& o_vec);

		bool GetPathMeshData(const Name& path, std::shared_ptr<PavementTool::PrivateMeshData>& data, bool needSubData);
		void SetPathMeshData(const Name& path, std::shared_ptr<PavementTool::PrivateMeshData>& data);
		void SetWeightZero(const Name& path);

		std::shared_ptr<PavementTool::PrivateMeshData> prepareMeshData(const Name& meshName);

	protected:

		struct FitGroundMask
		{
			bool needCheck = false;
			bool needUpdate = true;
			//bool mask[200][200] = { };
			std::bitset<100> mask[100];
		};
		struct PlanetRoadFitGroundAreaMap
		{
			//SphericalTerrain*	currentPlanet = nullptr;
			Vector3 posCache = Vector3::ZERO;
			std::vector<WorkQueue::RequestID> mTickIDs;
			std::map<int, std::set<ActorComponentPtr>>	roadCache;
			std::map<int, RoadManager::FitGroundMask>	maskData[2] ;
			unsigned char writeIndex = 0;
			unsigned char readIndex = 1;

			bool needupdate = false;
		};

		struct SceneDataCache
		{
			std::set<SphericalTerrain*> mPlanetCache;
		};

		void	CheckScenePlanetRoadMaskUpdate();

		std::map<SphericalTerrain*, RoadManager::PlanetRoadFitGroundAreaMap*>	mPlanetFitGroundRoadMask;
		std::map<Name, RoadManager::SceneDataCache>	mScenePlanetDataCache;
		//std::map<Name, SphericalTerrain*>	mSceneCurrentPlanet;
		//std::map<Name, std::map<int, std::set<ActorComponentPtr>>>		mPlanetFitGroundRoadCache;
		//std::map<Name, WorkQueue::RequestID>	m_setRoadMaskTickets;
		tthread::recursive_mutex  mLock;
	private:
		class PlanetTerrainTool* planetTerrainTool = nullptr;
	public:
		void	addPlanetFitGroundPavementAreaCache(const ActorComponentPtr& com, SphericalTerrain* planet, const AxisAlignedBox& worldBox);
		void	removePlanetFitGroundPavementAreaCache(const ActorComponentPtr& com);

		static Echo::ProtocolResult			addBiomeVegetationData(void* p, Echo::ProtocolHandle handle);
		static Echo::ProtocolResult			delBiomeVegetationData(void* p, Echo::ProtocolHandle handle);
		void	registerScenePlanet(SceneManager* scene, SphericalTerrain* planet);
		void	unregisterScenePlanet(SceneManager* scene, SphericalTerrain* planet);

		void	UpdatScenePlanetMaskData(SphericalTerrain* planet, const AxisAlignedBox& worldBox);

		static Echo::ProtocolResult			ProCom_CheckPlanetPosition_FitGorundRoad(void* planet, Echo::ProtocolHandle handle);
#ifdef ECHO_EDITOR
		void	CalculateAllPlanetRoadMask(SphericalTerrain* planet, bool sync = false);
#endif
		void	CheckPlanetPosition_FitGorundRoad(VegRoadQueryPackage* data);

		void	updatePlanetPavementMaskCache(SphericalTerrain* mPlanet, const std::set<ActorComponentPtr>& pavementVec, bool sync = false);

		void	updatePlanetRoadMaskCache(SphericalTerrain* mPlanet, const PlanetRoadData& road, bool isRemoveVeg, bool sync = false);

		void	updateMaskCache(SphericalTerrain* mPlanet, const AxisAlignedBox& localBox, const Vector3& worldPos, const Quaternion& worldQuat, bool isRemoveVeg);
		void	updatePlanetBuildingMaskCache(SphericalTerrain* mPlanet, bool isRemoveVeg);

		void	CalculateRoadRange(const RoadRangeData& data, std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec);

		std::vector<Vector3> CalculatePlanetMask(SphericalTerrain* mPlanet, const std::vector<Vector3>& worldPositions, const std::vector<uint16>& indexVec);
		void	operationCompleted_PlanetMask(WorkQueue::RequestID tickID, SphericalTerrain* mPlanet, const std::vector<Vector3>& data, const std::vector<AxisAlignedBox>& boxVec, bool isRemove);

		bool	usePlanetRoadMask = true;


#ifdef ECHO_EDITOR
		void	UpdateAllPavementPhysics(bool sync = false);
#endif
		PiecewiseBezierComponentPtr m_pBezierEditor;
		//bool m_bUpdateBezier = false;

		std::vector<PavementComponent*> m_vecPavements;

		float m_pavementFitGroundLevelHight = 40.0f;

		int m_pavementDrawCount = 0;
		int m_pavementDrawCountLOD[LOD_LEVEL_NUM];
		int m_pavementTriangleCount = 0;
		int m_pavementTriangleCountLOD[LOD_LEVEL_NUM];

		bool m_BeginListener = false;
		int ListenerPick = 0;

		std::map<Name, size_t> m_mapWeight;
		std::map<Name, std::shared_ptr<PavementTool::PrivateMeshData>> m_mapPathMesh;
		std::unordered_map<String, std::vector<std::shared_ptr<PavementTool::PavementUtilMesh>>*> m_mapUtilKeyData;

#ifdef ECHO_EDITOR
		bool GetPointPickEntity(MovableObject* moveable, const Ray& ray, Vector3& pos, float& dis);

		std::map<Name, TreeManager*> m_mapMeshBVH;
		std::map<String, std::shared_ptr<BezierData>> mBezierPoints;
#endif // ECHO_EDITOR
	private:
		//class RoadWorkQueue* m_WorkQueue = nullptr;
#ifdef ECHO_EDITOR
		AllComponentData* m_pComs = nullptr;
#endif
	};

#ifdef ECHO_EDITOR
	class TreeManager
	{
	public:
		TreeManager(MeshPtr& mesh);
		~TreeManager();

		bool getPickHeight(const Echo::Ray& ray, Vector3& hit_pos, float& hit_dis, const Echo::Matrix4& mt4World);

		class BVH_Node
		{
		public:
			struct Triangle
			{
				union {
					int index[3];
					struct {
						int a, b, c;
					};
					int _index[3];
				};

				Triangle(int x,int y,int z)
				{ index[0] = x; index[1] = y; index[2] = z; }

				Triangle(uint16 x, uint16 y, uint16 z, uint16 w)
				{
					index[0] = CombineData(w, x);
					index[1] = CombineData(w, y);
					index[2] = CombineData(w, z);
				}

				static int CombineData(uint16 sub_num, uint16 data)
				{
					int ndata = (int)sub_num;
					ndata <<= 16;
					ndata += data;
					return ndata;
				}

				static void DataUnbinding(int ndata, uint16& sub_num, uint16& data)
				{
					sub_num = uint16(ndata >> 16);
					data = (uint16)ndata;
				}
			};

			BVH_Node(std::vector<Triangle>& pTriangles, TreeManager* pThis, float mDivisionValue);
			~BVH_Node();

			bool getPickHeight(const Echo::Ray& ray, float& hit_distance);

			BVH_Node* m_pChildNode[8] = { nullptr };
			TreeManager* p_Datas;

			Vector3 m_Center = Vector3::ZERO;
			AxisAlignedBox m_Box;

			std::vector<Triangle> m_vTriangles;
		};

		std::vector<std::vector<Vector3>> m_vDatas;
		BVH_Node* m_pRoot;
	};
#endif // ECHO_EDITOR

}
#endif // !__EchoRoadManager_H__


