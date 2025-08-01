#pragma once
#include "3DUI/Echo3DUIComponent.h"
#include "3DUI/Echo3DUIComponentManager.h"
#include "EchoVector2.h"
#include "EchoVector3.h"
#include "FairyGUI.h"
#include "Actor/EchoActor.h"
#include"EchoPlanetRoad.h"
#include "EchoSphericalTerrainManager.h"
#include"CarFollowingModel.h"
#include <thread>
#include <future>
#include"mutex"
#include"condition_variable"

namespace Echo
{
	class SceneManager;
	class WorldManager;
	class SphericalTerrain;

	//_geometry
	struct JsonRoadNode {
		uint16 id;
		std::string name;
		Vector3 geometry;
	};

	struct JsonRoadLink {
		uint16 road_id;
		std::string road_name;
		uint16 source;
		uint16 target;
		float length;
	};

	struct JsonRoadData {
		std::vector<JsonRoadNode> nodes;
		std::vector<JsonRoadLink> links;
	};
	//_connection

	struct HighwayFeature {
		std::string type = "Feature";
		struct Properties {
			std::string NAME;
			std::vector<std::string> connect_bridge_name;
			std::vector<uint16> connect_bridge_id;
			std::string length;
		} properties;

		struct Geometry {
			std::string type = "LineString";
			std::vector<Vector3> coordinates;
		} geometry;
	};

	struct HighwayConnectData {
		std::string type = "FeatureCollection";
		std::string name;
		std::vector<HighwayFeature> features;
	};

	struct HighwayNode
	{
		Vector3 geometry;
		std::string name;
		uint16 id;
	};

	struct HighwayLink
	{
		float length;
		uint16 road_id;
		std::string road_name;
		UINT16 source;
		uint16 target;
	};
	struct HighwayGraphData
	{
		bool directed = false;
		bool multigraph = false;
		std::vector<HighwayNode> nodes;
		std::vector<HighwayLink> links;
	};

	//持有曲线数据
	class Vehicle;
	class Road {
	public:
		//Road(const JsonRoadLink& linkData, const JsonRoadNode& sourceNode, const JsonRoadNode& targetNode);
		Road(const HighwayLink& linkData, const HighwayNode& sourceNode, const HighwayNode& targetNode, const std::vector<Vector3>& coordinates);
		void addCar(Vehicle* car);
		void update(float deltaTime);
		uint16 getRoadId() const { return mRoadId; }
		float getRoadLength() const { return mLens; }
		uint16 getSourceNodeId() const;
		uint16 getDestinationNodeId() const;
		void setNextRoad(Road* nextRoad);
		Road* getNextRoad() const { return m_nextRoad; }

		// 添加Traffic引用以支持路径导航
		void setTrafficManager(class Traffic* trafficManager) { m_trafficManager = trafficManager; }

		bool isHighwayRoad() const { return m_isHighwayRoad; }


		Vehicle* findLeadingVehicle(const Vehicle* vehicle) const;
		float calculateGapToLeadingVehicle(const Vehicle* vehicle) const;

		// getPostion and  direction
		Vector3 getPositionAtDistance(float distance) const;
		Vector3 getDirectionAtDistance(float distance) const;
		Vector3 getNormalAtDistance(float distance) const;
		void checkVisible();
		void sortVehicles();
		void updateEnvironment();
		void updateLeadIndex(int i);
		void updateLagIndex(int i);
		Vehicle* getVehicleByIndex(int index) const;

		friend class Traffic;
	private:
		// JSON道路相关成员变量
		uint16 mRoadId = 0;
		std::string mRoadName;
		uint16 mSourceNodeId = 0;
		uint16 mTargetNodeId = 0;
		Vector3 mStartPos;
		Vector3 mEndPos;

		float mLens = 0.0f;
		std::vector<Vehicle*> mCars;
		std::vector<float> mSeg;

		std::vector<float> m_mainRoadSegmentLengths;
		float m_mainRoadTotalLength = 0.0f;
		bool m_roadDataCached = false;

		std::vector<Vector3> m_coordinatePath;
		std::vector<float> m_segmentLengths;
		std::vector<float> m_cumulativeLengths;
		bool m_isHighwayRoad = false;

		Road* m_nextRoad = nullptr;
		class Traffic* m_trafficManager = nullptr; // 指向Traffic实例的指针

	};

	class Vehicle
	{
	public:		enum class LaneDirection
	{
		Forward = 0,  // 正向行驶（沿直线方向）
		Backward = 1  // 反向行驶（逆直线方向）
	};

		  enum class VehicleType
		  {
			  Car = 0,
			  Truck = 1
		  };


		  Vehicle(float initialSpeed = 25.0f, LaneDirection direction = LaneDirection::Forward);
		  void assignToRoute(const std::list<uint16>& route, Road* roadManager);
		  void update(float deltaTime);
		  void updatePos();

		  // 跟车模型相关
		  void setCarFollowingModel(std::unique_ptr<ICarFollowingModel> model);
		  ICarFollowingModel* getCarFollowingModel() const { return m_carFollowingModel.get(); }
		  void applyCarFollowing(float deltaTime, const Vehicle* leadingVehicle, float gap);
		  // 设置驾驶员特性
		  void setDriverVariation(float variationCoeff);
		  float getDriverFactor() const { return m_driverFactor; }

		  // 路径相关方法
		  void setPath(const std::vector<uint16>& roadIds);
		  bool moveToNextRoad();
		  bool moveToPreviousRoad(); //逆向车辆
		  uint16 getCurrentRoadId() const;
		  bool hasPath() const; // 检查是否有分配的路径

	public:
		int id = 0; // 车辆ID
		LaneDirection laneDirection = LaneDirection::Forward; // 行驶方向
		float s;//当前走过的路程
		float speed;
		float acc;
		Vector3 pos;
		Vector3 dir;
		Quaternion rot;
		Vector3 roadNormal = Vector3::UNIT_Y;
		float laneWidth = 3.5f; // 车道宽度
		float laneOffset = 0.0f; // 车道偏移（正值右车道，负值左车道）
		int iLead = -1;    // 前车索引
		int iLag = -1;
		// 车辆物理参数
		float m_length = 5.0f;  // 车长
		float m_width = 2.5f;   // 车宽

		bool Visible = false;
		bool m_needReverseDirection = false;

	private:
		ActorPtr mCar;

		// 跟车模型
		std::unique_ptr<ICarFollowingModel> m_carFollowingModel;
		float m_driverFactor = 1.0f;
		float m_driverVariationCoeff = 0.15f;


		// 路径跟踪
		std::vector<uint16> m_pathRoads;
		int m_currentRoadIndex = -1;
		friend class Road;
	};





	class Traffic : public ActorLoadListener, public SphericalTerrain::LoadListener, public FrameListener
	{
	public:
		class VehicleTiker : public Job
		{
		public:
			VehicleTiker(Traffic* traffic);
			virtual ~VehicleTiker();
			virtual void Execute();
		private:
			Traffic* mTraffic = nullptr;

		};


	public:
		Traffic(SceneManager* InSceneManger, WorldManager* InWorldMgr);
		~Traffic();

		void initRoads();
		void onTick();
		void initVehicle();
		void checkVehicle();
		//回调
		bool OnActorCreateFinish() override;

		void OnCreateFinish() override;
		void OnDestroy() override;

		bool frameStarted(const FrameEvent& evt) override;

		// 跟车模型配置 - 简化的模块化接口
		void setDefaultCarFollowingModel(CarFollowingModelFactory::ModelType modelType);
		void updateGlobalModelParameters(const IDMModel::Parameters& idmParams, const ACCModel::Parameters& accParams);


		void useConservativeDriving();    // 保守驾驶模式
		void useAggressiveDriving();      // 激进驾驶模式
		void useNormalDriving();          // 正常驾驶模式
		void setDriverVariationLevel(float level);  // 设置驾驶员变异水平 (0.0-1.0)
		// 车辆管理
		void addMultipleVehicles(int numVehicles);  // 添加多个车辆
		void setVehicleDensity(float vehiclesPerKm);  // 设置车辆密度
		// 路径管理
		void generateAllPaths();
		void assignPathToVehicle(Vehicle* vehicle, int pathIndex);
		void assignBackwardPathToVehicle(Vehicle* vehicle, int pathIndex); // 为逆向车辆分配反向路径
		void assignPathsToAllVehicles();
		Road* getRoadById(uint16 roadId);  // 根据道路ID查找道路对象



	private:
		class Barrier
		{
		public:
			void wait();
			void signal();
		private:
			std::promise<void> m_promise;
			std::mutex m_mutex;
			std::condition_variable m_cv;
			bool m_signaled = false;
		};	private:
			void _tick(float dt);
			void UpdatePositon();
			void generatePathsRecursive(Road* currentRoad, std::vector<uint16>& currentPath,
				std::set<uint16>& visited, std::set<std::vector<uint16>>& uniquePaths, int maxDepth);


			// JSON相关方法声明
				/*JsonRoadData parseJsonRoadData(const std::string& jsonFilePath);
				void createConnectedRoadNetworkFromJson(const JsonRoadData& roadData);*/
				// Highway Connect JSON相关方法声明
			HighwayConnectData parseHighwayConnectData(const std::string& jsonFilePath);
			HighwayGraphData parseHighwayGraphData(const std::string& jsonFilePath);
			void createRoadNetworkFromHighwayData(const HighwayConnectData& connectData, const HighwayGraphData& graphData);
			Road* createRoadFromHighwayLink(const HighwayLink& link, const HighwayNode& sourceNode, const HighwayNode& targetNode, const std::vector<Vector3>& coordinates);

			//路径Bfs

			void generateNodePathsBFS(uint16 startNodeId,
				const std::map<uint16, std::vector<uint16>>& nodeAdjacencyMap,
				std::set<std::vector<uint16>>& allNodePaths, int maxPathLength);
			void initializeCarFollowingModels();
			std::unique_ptr<ICarFollowingModel> createModelForVehicle(Vehicle::VehicleType type);

			std::set<uint16> m_visibleRoadsLastFrame;

			//std::map<uint16, Road> m_Roads;
			//Road* m_roadManager = nullptr;
			Road* m_roadManager = nullptr;
			std::vector<Vehicle*> m_Vehicles; // 存储车辆指针
			// 道路连接系统
			std::vector<Road*> m_allRoads; // 所有道路
			std::map<uint16, std::vector<Road*>> m_nodeConnections; // 节点ID到道路的映射

			// JSON相关成员变量
			JsonRoadData m_jsonRoadData;
			std::map<uint16, JsonRoadNode> m_nodeMap; // 节点ID到节点的映射

			HighwayGraphData m_highwayGraphData;
			std::vector<HighwayNode> m_highwayNodes;


			//跟车模型配置
			CarFollowingModelFactory::ModelType m_defaultModelType = CarFollowingModelFactory::ModelType::IDM;
			IDMModel::Parameters m_idmParams;
			ACCModel::Parameters m_accParams;
			float m_driverVariationCoeff = 0.15f;//驾驶员变异系数

			// 路径管理
			std::vector<std::vector<uint16>> m_allPaths;
			bool m_pathsGenerated = false;
			std::thread* m_thread = nullptr;
			bool m_bContinue = false;
			bool m_bQuite = false;
			bool m_isInitialized = false;
			Barrier m_workBarrier;
			Barrier m_mainBarrier;
			Barrier m_initBarrier;
			//Barrier m_finalInitBarrier;

			// 基础管理器引用
			SceneManager* mSceneMgr = nullptr;
			WorldManager* mWorldMgr = nullptr;

			std::string m_safeLevelName = "karst2";

			std::atomic<bool> m_onTickCompleted{ false };
			std::unique_ptr<VehicleTiker> vehicleTiker;
	};
}
