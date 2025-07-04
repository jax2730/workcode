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
		void update(float deltaTime);

		uint16 getRoadId() const { return mRoadsData.roadID; }
		uint16 getSourceCityId() const;
		uint16 getDestinationCityId() const;
		void setNextRoad(Road* nextRoad) { m_nextRoad = nextRoad; }
		Road* getNextRoad() const { return m_nextRoad; }

		Vehicle* findLeadingVehicle(const Vehicle* vehicle) const;
		float calculateGapToLeadingVehicle(const Vehicle* vehicle) const;



	private:
		PlanetRoadData mRoadsData;

		float mLens = 0.0f;
		std::vector<Vehicle*> mCars;
		std::vector<float> mSeg;

		std::vector<float> m_mainRoadSegmentLengths;
		float m_mainRoadTotalLength = 0.0f;
		bool m_roadDataCached = false;

		Road* m_nextRoad = nullptr;

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

	private:
		ActorPtr mCar;

		// 跟车模型
		std::unique_ptr<ICarFollowingModel> m_carFollowingModel;
		float m_driverFactor = 1.0f;
		float m_driverVariationCoeff = 0.15f;

		// 车辆物理参数
		float m_length = 5.0f;  // 车长
		float m_width = 2.5f;   // 车宽

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

	class Traffic : public ActorLoadListener, public SphericalTerrain::LoadListener
	{
	public:
		Traffic(SceneManager* InSceneManger, WorldManager* InWorldMgr);
		~Traffic();

		void Tick(float dt);



		//回调
		bool OnActorCreateFinish() override;

		void OnCreateFinish() override;
		void OnDestroy() override;

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


	private:

		void createConnectedRoadNetwork(const PlanetRoadGroup& roadGroup);

		void initializeCarFollowingModels();
		std::unique_ptr<ICarFollowingModel> createModelForVehicle(Vehicle::VehicleType type);

		PlanetRoad* m_planetRoad = nullptr;
		std::set<uint16> m_visibleRoadsLastFrame;
		SphericalTerrain* m_targetPlanet = nullptr;
		//std::map<uint16, Road> m_Roads;
		//Road* m_roadManager = nullptr;
		Road* m_roadManager = nullptr;
		std::vector<Vehicle*> m_Vehicles; // 存储车辆指针

		// 道路连接系统
		std::vector<Road*> m_allRoads; // 所有道路
		std::map<uint16, std::vector<Road*>> m_cityConnections; // 城市ID到道路的映射
		//跟车模型配置
		CarFollowingModelFactory::ModelType m_defaultModelType = CarFollowingModelFactory::ModelType::IDM;
		IDMModel::Parameters m_idmParams;
		ACCModel::Parameters m_accParams;
		float m_driverVariationCoeff = 0.15f;//驾驶员变异系数

	};
}
