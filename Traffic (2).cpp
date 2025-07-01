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
#include <ctime>
#include <cstdlib>
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
		for (Road* road : m_allRoads)
		{
			if (road) {
				road->update(dt);
			}
		}
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

			// 创建道路连接网络
			createConnectedRoadNetwork(roadGroup);

			// 使用第一条道路作为起始道路
			if (m_allRoads.size() > 0) {
				m_roadManager = m_allRoads[0]; // 使用road[0]
			}

			if (!roadGroup.roads.empty() && m_roadManager)
			{
				// 
				const float baseSpeed = 50.0f;
				const float laneOffset = 3.0f; // 车道偏移

				// 第一辆正向行驶
				Vehicle* forwardCar = new Vehicle(baseSpeed, Vehicle::LaneDirection::Forward);
				forwardCar->id = 1;
				forwardCar->laneOffset = laneOffset;
				forwardCar->s = 0.0f;
				m_roadManager->addCar(forwardCar);
				m_Vehicles.push_back(forwardCar);

				// 第二辆车反向行驶
				Vehicle* backwardCar = new Vehicle(baseSpeed, Vehicle::LaneDirection::Backward);
				backwardCar->id = 2;
				backwardCar->laneOffset = laneOffset;
				backwardCar->s = 100.0f;
				m_roadManager->addCar(backwardCar);
				m_Vehicles.push_back(backwardCar);

				LogManager::instance()->logMessage("Connected road network created with " + std::to_string(m_allRoads.size()) + " roads.");
			}
		}


	}



	void Traffic::OnDestroy()
	{

	}

	void Traffic::createConnectedRoadNetwork(const PlanetRoadGroup& roadGroup)
	{
		// 1. 创建所有道路对象
		for (const auto& roadData : roadGroup.roads)
		{
			Road* road = new Road(roadData);
			m_allRoads.push_back(road);

			// 记录每个城市连接的道路
			if (roadData.source.type == PlanetRoadData::TerminalType::City)
			{
				uint16 cityId = roadData.source.id;
				m_cityConnections[cityId].push_back(road);
			}
			if (roadData.destination.type == PlanetRoadData::TerminalType::City)
			{
				uint16 cityId = roadData.destination.id;
				m_cityConnections[cityId].push_back(road);
			}
		}

		// 2. 建立道路连接关系
		for (Road* road : m_allRoads)
		{
			uint16 destCityId = road->getDestinationCityId();
			if (destCityId != 0) // 如果有有效的目标城市ID
			{
				// 查找从这个城市出发的道路
				auto it = m_cityConnections.find(destCityId);
				if (it != m_cityConnections.end())
				{
					for (Road* candidateRoad : it->second)
					{
						// 如果找到一条道路从这个城市开始，就连接它
						if (candidateRoad->getSourceCityId() == destCityId && candidateRoad != road)
						{
							road->setNextRoad(candidateRoad);
							LogManager::instance()->logMessage("Connected Road[" + std::to_string(road->getRoadId()) +
								"] -> Road[" + std::to_string(candidateRoad->getRoadId()) + "] via City ID " + std::to_string(destCityId));
							break;
						}
					}
				}
			}
		}
	}
	Road::Road(const PlanetRoadData& roadData) : mRoadsData(roadData)
	{
		for (size_t i = 0; i < roadData.beziers.size() - 1; ++i)
		{
			const auto& startPoint = roadData.beziers[i];
			const auto& endPoint = roadData.beziers[i + 1];

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
		// 收集需要转移的车辆
		std::vector<Vehicle*> vehiclesToTransfer;

		for (Vehicle* car : mCars)
		{
			// 1. 更新车辆已行驶的总路程（反向行驶时减少）
			if (car->laneDirection == Vehicle::LaneDirection::Forward)
			{
				car->update(deltaTime);
			}
			else
			{
				// 反向行驶：减少s值
				car->s -= car->speed * deltaTime;
				while (car->s < 0 && mLens > 0) {
					car->s += mLens; // 循环到道路末端
				}
			}

			// 2. 检查车辆是否到达道路末端需要转移
			if (m_nextRoad && car->laneDirection == Vehicle::LaneDirection::Forward && car->s >= mLens)
			{
				vehiclesToTransfer.push_back(car);
				continue; // 跳过位置计算，因为即将转移
			}

			// 3. 如果路程超过总长，让它循环（仅在没有下一条道路时）
			if (car->s > mLens && mLens > 0) {
				if (!m_nextRoad) {
					car->s = fmod(car->s, mLens);
				}
			}

			// 4. 查找车辆当前在哪一段 (piece_index)
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

			// 5. 计算在当前这一小段上行驶的距离
			float distanceOnSegment = car->s - length_before_piece;

			// 6. 根据路程反求出插值参数 t
			const auto& startPoint = mRoadsData.beziers[piece_index];
			const auto& endPoint = mRoadsData.beziers[piece_index + 1];
			float t = PiecewiseBezierUtil::LengthsTot(startPoint.point, startPoint.destination_control, endPoint.point, endPoint.source_control, distanceOnSegment, 0.f);

			// 7. 根据 t 计算出车辆在道路中心线的位置
			Vector3 centerPos = PiecewiseBezierUtil::CInterpolate(startPoint.point, startPoint.destination_control, endPoint.point, endPoint.source_control, t);

			// 8. 计算贝塞尔曲线在当前点的切线方向
			Vector3 tangent = PiecewiseBezierUtil::CTangent(startPoint.point, startPoint.destination_control, endPoint.point, endPoint.source_control, t);

			tangent.Normalize();
			if (car->laneDirection == Vehicle::LaneDirection::Backward)
			{
				tangent *= -1.f;
			}
			// 9. 计算车道偏移的法向量
			Vector3 roadNormal = startPoint.normal;
			if (roadNormal.length() < 0.1f) {
				roadNormal = Vector3::UNIT_Y;
			}
			roadNormal.Normalize();

			Vector3 right = tangent.crossProduct(roadNormal);
			if (right.length() < 0.1f) {
				Vector3 up = Vector3::UNIT_Y;
				right = tangent.crossProduct(up);
				if (right.length() < 0.1f) {
					up = Vector3::UNIT_Z;
					right = tangent.crossProduct(up);
				}
			}
			right.Normalize();

			Matrix3 mat;
			mat.SetColumn(0, tangent);
			mat.SetColumn(1, roadNormal);
			mat.SetColumn(2, right);
			car->rot.FromRotationMatrix(mat);

			// 10. 计算最终位置和方向
			Vector3 offsetVector = right * car->laneOffset;
			car->pos = centerPos + offsetVector;

			if (car->laneDirection == Vehicle::LaneDirection::Forward) {
				car->dir = tangent;
			}
			else {
				car->dir = -tangent;
			}

			car->updatePos();
		}

		// 执行车辆转移
		for (Vehicle* vehicle : vehiclesToTransfer)
		{
			// 从当前道路移除车辆
			auto it = std::find(mCars.begin(), mCars.end(), vehicle);
			if (it != mCars.end())
			{
				mCars.erase(it);
			}

			// 计算超出的距离，保持移动的连续性
			float overshoot = vehicle->s - mLens;
			vehicle->s = std::max(0.0f, overshoot); // 从新道路的起点加上超出距离开始

			// 添加到下一条道路
			m_nextRoad->addCar(vehicle);

			LogManager::instance()->logMessage("Vehicle " + std::to_string(vehicle->id) +
				" transferred from Road " + std::to_string(getRoadId()) +
				" to Road " + std::to_string(m_nextRoad->getRoadId()) +
				" with overshoot: " + std::to_string(overshoot));
		}

	}



	Vehicle::Vehicle(float initialSpeed, LaneDirection direction)
		: s(0.f), speed(initialSpeed > 0 ? initialSpeed : 10.0f), acc(0.f), pos(Vector3::ZERO), laneDirection(direction)
	{
		// 随机选择不同的车型（如果有多种可用）
		std::vector<std::string> carModels = {
			"actors/car_005.act"
			// 
			// "actors/car_001.act",
			// "actors/car_002.act",
			// "actors/truck_001.act"
		};

		int modelIndex = rand() % carModels.size();
		mCar = Echo::ActorSystem::instance()->getActiveActorManager()->createActor(Name(carModels[modelIndex]), "", false, false);
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
			mCar->setRotation(rot);

		}
	}

	// Road连接相关方法实现
	uint16 Road::getSourceCityId() const
	{
		if (mRoadsData.source.type == PlanetRoadData::TerminalType::City)
		{
			return mRoadsData.source.id;
		}
		return 0; // 无效ID
	}

	uint16 Road::getDestinationCityId() const
	{
		if (mRoadsData.destination.type == PlanetRoadData::TerminalType::City)
		{
			return mRoadsData.destination.id;
		}
		return 0; // 无效ID
	}

}

