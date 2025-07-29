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
	class PlanetRoad;

	//持有曲线数据
	class Vehicle;

	class Road {
	public:
		Road(const PlanetRoadData& roadData);

		void addCar(Vehicle* car);
		void removeCar(Vehicle* car);
		void update(float deltaTime);
		void updateVehicleState(Vehicle* vehicle, float dt);

		uint16 getRoadId() const { return mRoadsData.roadID; }
		float getRoadLength() const { return mLens; }
		uint16 getSourceCityId() const;
		uint16 getDestinationCityId() const;
		void setNextRoad(Road* nextRoad);
		Road* getNextRoad() const { return m_nextRoad; }

		// 添加Traffic引用以支持路径导航
		void setTrafficManager(class Traffic* trafficManager) { m_trafficManager = trafficManager; }

		Vehicle* findLeadingVehicle(const Vehicle* vehicle) const;
		float calculateGapToLeadingVehicle(const Vehicle* vehicle) const;
		void checkVisible();

		void sortVehicles();
		void updateEnvironment();
		void updateLeadIndex(int i);
		void updateLagIndex(int i);
		Vehicle* getVehicleByIndex(int index) const;

		friend class Traffic;

	private:
		PlanetRoadData mRoadsData;

		float mLens = 0.0f;
		std::vector<Vehicle*> mCars;
		std::vector<float> mSeg;

		std::vector<float> m_mainRoadSegmentLengths;
		float m_mainRoadTotalLength = 0.0f;
		bool m_roadDataCached = false;

		Road* m_nextRoad = nullptr;
		class Traffic* m_trafficManager = nullptr; // 指向Traffic实例的指针

	};

	struct VehicleRenderData
	{

		Vector3 pos = Vector3::ZERO;
		Quaternion rot = Quaternion::IDENTITY;
	};


	class Vehicle
	{
	public:
		enum class LaneDirection
		{
			Forward = 0,  // 正向行驶（沿贝塞尔曲线方向）
			Backward = 1  // 反向行驶（逆贝塞尔曲线方向）
		};

		enum class VehicleType
		{
			Car = 0,
			Truck = 1
		};

		Vehicle(float initialSpeed, LaneDirection direction);
		void assignToRoute(const std::list<uint16>& route, Road* roadManager);
		void setCurrentRoad(Road* road) { m_currentRoad = road; }
		Road* getCurrentRoad() const { return m_currentRoad; }
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
		Road* m_currentRoad = nullptr;
		Vector3 roadNormal = Vector3::UNIT_Y;
		float laneWidth = 3.5f; // 车道宽度
		float laneOffset = 0.0f; // 车道偏移（正值右车道，负值左车道）
		int iLead = -1;    // 前车索引
		int iLag = -1;
		// 车辆物理参数
		float m_length = 5.0f;  // 车长
		float m_width = 2.5f;   // 车宽

		bool Visible = false;

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

	struct CityUI
	{
		std::string mCityName;
		Vector3 mWorldPosition;
		Vector2 mMapPosition;
		echofgui::GObject* mCityUIObject = nullptr;
		echofgui::GObject* mCityNameObject = nullptr;
		echofgui::GRoot* mUIRoot = nullptr;
	};

	struct UIObjectProperty
	{
		echofgui::GObject* mObject = nullptr;
		Vector3 mInitSize;
		Vector3 mCurrentSize;
		Vector3 mInitPos;
		Vector3 mCurrentPos;
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
		//void onUpdate();
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

		SphericalTerrain* getTargetPlanet() const { return m_targetPlanet; }

		struct DoubleBuffer
		{
			std::vector<VehicleRenderData> frontBuffer;
			std::vector<VehicleRenderData> backBuffer;
			std::mutex swapMutex;

			void swapBuffer()
			{
				std::lock_guard<std::mutex> lock(swapMutex);
				frontBuffer.swap(backBuffer);
			}

		};





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
		};

	private:
		void _tick(float dt);
		void UpdatePositon();
		void createConnectedRoadNetwork(const PlanetRoadGroup& roadGroup);
		void generatePathsRecursive(Road* currentRoad, std::vector<uint16>& currentPath,
			std::set<uint16>& visited, std::set<std::vector<uint16>>& uniquePaths, int maxDepth);

		void initializeCarFollowingModels();
		std::unique_ptr<ICarFollowingModel> createModelForVehicle(Vehicle::VehicleType type);

		PlanetRoad* m_planetRoad = nullptr;
		std::set<uint16> m_visibleRoadsLastFrame;
		SphericalTerrain* m_targetPlanet = nullptr;

		std::vector<Vehicle*> m_Vehicles; // 存储车辆指针

		// 道路连接系统
		std::vector<Road*> m_allRoads; // 所有道路
		std::map<uint16, std::vector<Road*>> m_cityConnections; // 城市ID到道路的映射
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

	    std::unique_ptr<VehicleTiker> vehicleTiker;

		std::atomic<bool> m_onTickCompleted{ false };


	};
}
