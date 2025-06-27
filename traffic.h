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


	private:
	    PlanetRoadData mRoadsData;

		float mLens = 0.0f;
		std::vector<Vehicle*> mCars;
		std::vector<float> mSeg;

		std::vector<float> m_mainRoadSegmentLengths;
		float m_mainRoadTotalLength = 0.0f;
		bool m_roadDataCached = false;
	};

	class Vehicle
	{
	public:
		Vehicle(float initialSpeed = 25.0f);
		void assignToRoad(const PlanetRoadData* roadData);
		void update(float deltaTime);
		void updatePos();

	public:
		float s;//当前走过的路程
		float speed;
		float acc;
		Vector3 pos;
		Vector3 dir;

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
		
		PlanetRoad* m_planetRoad = nullptr;
		std::set<uint16> m_visibleRoadsLastFrame;
		SphericalTerrain* m_targetPlanet = nullptr;
		//std::map<uint16, Road> m_Roads;
		//Road* m_roadManager = nullptr;
		Road* m_roadManager = nullptr;
		std::vector<std::unique_ptr<Vehicle>> m_Vehicles;

		
	};
}
