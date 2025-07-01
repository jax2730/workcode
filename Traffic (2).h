#pragma once
#include "3DUI/Echo3DUIComponent.h"
#include "3DUI/Echo3DUIComponentManager.h"
#include "EchoVector2.h"
#include "EchoVector3.h"
#include "FairyGUI.h"
#include "Actor/EchoActor.h"
#include"EchoPlanetRoad.h"
#include "EchoSphericalTerrainManager.h"
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

		Vehicle(float initialSpeed = 25.0f, LaneDirection direction = LaneDirection::Forward);
		void assignToRoute(const std::list<uint16>& route, Road* roadManager);
		void update(float deltaTime);
		void updatePos();

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




	private:

		void createConnectedRoadNetwork(const PlanetRoadGroup& roadGroup);

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


	};
}
