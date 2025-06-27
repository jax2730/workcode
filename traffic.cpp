#include "Traffic.h"
#include "EchoSceneManager.h"
#include "EchoWorldManager.h"
#include "Actor/EchoActorManager.h"
#include "Actor/EchoActor.h"
#include "EchoSphericalTerrainComponent.h"
#include"EchoPlanetRoad.h"
#include"EchoPlanetRoadResource.h"
#include"TrafficManager.h"
#include "EchoPiecewiseBezierUtil.h"
#include"chrono"
namespace Echo
{

	Traffic::Traffic(SceneManager* InSceneManger, WorldManager* InWorldMgr)

	{

	}

	Traffic::~Traffic()
	{

	}

	void Traffic::Tick(float dt)
	{
		if(m_roadManager)
			m_roadManager->update(dt);
	}

	bool Traffic::OnActorCreateFinish()
	{
		LogManager::instance()->logMessage("123");
		LogManager::instance()->logMessage("1111", LML_CRITICAL);
		if (mActor)
		{
			LogManager::instance()->logMessage("MiniMapManager::OnActorCreateFinish: Actor [" + mActor->getUniqueName() + "] 创建完成，开始初始化道路系统。");

			auto* component = mActor->GetFirstComByType<SphericalTerrainComponent>().get();
			if (component)
			{
				
				m_targetPlanet = component->GetSphericalTerrain();
				m_targetPlanet->addLoadListener(this);
				
			}
			return false;
			}
		


		return false;
	}


	void Traffic::OnCreateFinish()
	{

		if (m_targetPlanet)
		{
			const PlanetRoadGroup& roadGroup = m_targetPlanet->mRoad->mPlanetRoadData->getPlanetRoadGroup();

			m_roadManager = new Road(roadGroup.roads[0]);//std::make_unique<Road>(roadGroup.roads[0]);

			if (!roadGroup.roads.empty())
			{
				Vehicle* car = new Vehicle;

				m_roadManager->addCar(car);

				LogManager::instance()->logMessage("Created " + std::to_string(m_Vehicles.size()) + " vehicles on road system.");
			}
		}


	}
	
	

	void Traffic::OnDestroy()
	{

	}
	Road::Road(const PlanetRoadData& roadData) : mRoadsData(roadData)
	{
		for (size_t i = 0; i < roadData.beziers.size() - 1; ++i)
		{
			const auto& startPoint = roadData.beziers[i];
			const auto& endPoint = roadData.beziers[i+1];

			float currSeg = PiecewiseBezierUtil::LengthFromTime(
				startPoint.point,
				startPoint.destination_control,
				endPoint.point,
				endPoint.source_control,
				1.0f
			);
			mLens += currSeg;
			mSeg.push_back(mLens);
		}

		
		//LogManager::instance()->logMessage("Road Manager created, managing " + std::to_string(m_allRoadsData.size()) + " roads.");

	}

	void Road::addCar(Vehicle* car)
	{
		if (car)
		{
			mCars.push_back(car);
		}
	}

	void Road::update(float deltaTime) 
	{

		for (Vehicle* car : mCars)
		{

			// 2. 更新车辆已行驶的总路程
			car->update(deltaTime);


			// 4. 如果路程超过总长，让它循环
			if (car->s > mLens && mLens > 0) {
				car->s = fmod(car->s, mLens);
			}

			// 5. 查找车辆当前在哪一段 (piece_index)
			// ... (这部分逻辑需要道路的分段累积长度，我们可以在 Road 类中缓存，或者在这里即时计算)
			unsigned int piece_index = 0;
			float length_before_piece = 0;
			for (size_t i = 0; i < mSeg.size(); ++i)
			{
				if (car->s <= mSeg[i])
				{
					piece_index = i;
					length_before_piece = (i == 0) ? 0.0f : mSeg[i - 1];
					break;
				}
			}



			// 6. 计算在当前这一小段上行驶的距离
			float distanceOnSegment = car->s - length_before_piece;

			// 7. 根据路程反求出插值参数 t
			const auto& startPoint = mRoadsData.beziers[piece_index];
			const auto& endPoint = mRoadsData.beziers[piece_index + 1];
			float t = PiecewiseBezierUtil::LengthsTot(startPoint.point, startPoint.destination_control, endPoint.point, endPoint.source_control, distanceOnSegment, 0.f);

			// 8. 根据 t 计算出车辆的当前位置
			car->pos = PiecewiseBezierUtil::CInterpolate(startPoint.point, startPoint.destination_control, endPoint.point, endPoint.source_control, t);
			car->dir = PiecewiseBezierUtil::CInterpolate(startPoint.point, startPoint.destination_control, endPoint.point, endPoint.source_control, t+0.01f);
			car->dir -= car->pos;
			car->dir.Normalize();
			car->updatePos();
		}
		
	}
	


	Vehicle::Vehicle(float initialSpeed)
		: s(0.f), speed(initialSpeed), acc(0.f), pos(Vector3::ZERO)
	{
		mCar = Echo::ActorSystem::instance()->getActiveActorManager()->createActor(Name("actors/car_005.act"), "", false, false);
	}

	

	void Vehicle::update(float deltaTime)
	{
		s += speed * deltaTime;
	}

	void Vehicle::updatePos()
	{
		if (!mCar.Expired())
		{
			mCar->setPosition(pos);
			mCar->setRotation(Quaternion::FromToRotation({ 1.0f, 0.0f, 0.0f }, dir));
		}
	}

}

