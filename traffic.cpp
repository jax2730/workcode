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
#include"CarFollowingModel.h"
#include "StaticPaths.h"
namespace Echo
{

	Traffic::Traffic(SceneManager* InSceneManger, WorldManager* InWorldMgr)

	{
		initializeCarFollowingModels();
		srand(static_cast<unsigned int>(time(nullptr)));
		
	}

	Traffic::~Traffic()
	{
		//addLoadListener销毁
			// 移除 SphericalTerrain 的监听器，防止析构时的异常
		if (m_targetPlanet) {
			m_targetPlanet->removeLoadListener(this);
			m_targetPlanet = nullptr;
		}

		// 清理所有车辆
		for (Vehicle* vehicle : m_Vehicles) {
			if (vehicle) {
				delete vehicle;
			}
		}
		m_Vehicles.clear();

		// 清理所有道路
		for (Road* road : m_allRoads) {
			if (road) {
				delete road;
			}
		}
		m_allRoads.clear();

		// 清理城市连接映射
		m_cityConnections.clear();

		// 清理路径数据
		m_allPaths.clear();
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
				//const float baseSpeed = 50.0f;
				//const float laneOffset = 3.0f; // 车道偏移
				//
				// 第一辆正向行驶
				//Vehicle* forwardCar = new Vehicle(baseSpeed, Vehicle::LaneDirection::Forward);
				//forwardCar->id = 1;
				//forwardCar->laneOffset = laneOffset;
				//forwardCar->s = 0.0f;
				//m_roadManager->addCar(forwardCar);
				//m_Vehicles.push_back(forwardCar);

				// 第二辆车反向行驶
				//Vehicle* backwardCar = new Vehicle(baseSpeed, Vehicle::LaneDirection::Backward);
				//backwardCar->id = 2;
				//backwardCar->laneOffset = laneOffset;
				//backwardCar->s = 100.0f;
				//m_roadManager->addCar(backwardCar);
				//m_Vehicles.push_back(backwardCar);
				generateAllPaths();


				addMultipleVehicles(80);
				assignPathsToAllVehicles();
				LogManager::instance()->logMessage("Connected road network created with " + std::to_string(m_allRoads.size()) + " roads.");
			}
		}


	}



	void Traffic::OnDestroy()
	{
		//移除小车  销毁
	}
	//道路链接
	void Traffic::createConnectedRoadNetwork(const PlanetRoadGroup& roadGroup)
	{
		// 1. 创建所有道路对象
		for (const auto& roadData : roadGroup.roads)
		{
			Road* road = new Road(roadData);
			road->setTrafficManager(this); // 设置Traffic引用
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
			Vehicle* leadingVehicle = findLeadingVehicle(car);
			if (leadingVehicle && car->getCarFollowingModel())
			{
				float gap = calculateGapToLeadingVehicle(car);
				car->applyCarFollowing(deltaTime, leadingVehicle, gap);
			}
			else
			{
				// 没有前车时使用自由流行驶
				if (car->getCarFollowingModel()) {
					// 有跟车模型但没有前车，使用期望速度进行自由流行驶
					float desiredSpeed = 12.0f; // 默认期望速度

					// 尝试从跟车模型获取期望速度
					if (auto* idmModel = dynamic_cast<IDMModel*>(car->getCarFollowingModel())) {
						desiredSpeed = idmModel->getParameters().v0;
					}
					else if (auto* accModel = dynamic_cast<ACCModel*>(car->getCarFollowingModel())) {
						desiredSpeed = accModel->getParameters().v0;
					}

					float speedDiff = desiredSpeed - car->speed;
					if (speedDiff > 0.1f) {
						car->acc = 1.0f; // 温和加速到期望速度
					}
					else if (speedDiff < -0.1f) {
						car->acc = -0.5f; // 温和减速到期望速度
					}
					else {
						car->acc = 0.0f; // 保持当前速度
					}

					// 更新速度和位置
					car->speed = std::max(0.0f, car->speed + car->acc * deltaTime);
					if (car->laneDirection == Vehicle::LaneDirection::Forward) {
						car->s += car->speed * deltaTime;
					}
					else {
						car->s -= car->speed * deltaTime;
						// 反向车辆边界检查
						while (car->s < 0 && mLens > 0) {
							car->s += mLens; // 循环到道路末端
						}
					}
				}
				else {
					// 没有跟车模型的简单车辆
					const float freeFlowAcceleration = 1.0f; // m/s²
					car->acc = std::max(-3.0f, std::min(freeFlowAcceleration, car->acc));
					car->speed = std::max(0.0f, car->speed + car->acc * deltaTime);

					// 更新位置
					if (car->laneDirection == Vehicle::LaneDirection::Forward) {
						car->s += car->speed * deltaTime;
					}
					else {
						car->s -= car->speed * deltaTime;
						// 反向车辆边界检查
						while (car->s < 0 && mLens > 0) {
							car->s += mLens; // 循环到道路末端
						}
					}
				}
			}

			// 2. 检查车辆是否到达道路末端需要转移
			if (car->laneDirection == Vehicle::LaneDirection::Forward && car->s >= mLens)
			{
				vehiclesToTransfer.push_back(car);
				continue; // 跳过位置计算，因为即将转移
			}
			// 检查逆向车辆是否到达道路起点需要转移
			else if (car->laneDirection == Vehicle::LaneDirection::Backward && car->s <= 0.0f)
			{
				vehiclesToTransfer.push_back(car);
				continue; // 跳过位置计算，因为即将转移
			}

			// 3. 边界处理：如果车辆超出道路范围，进行循环或转移处理
			if (car->laneDirection == Vehicle::LaneDirection::Forward && car->s > mLens && mLens > 0) {
				bool hasNextDestination = (m_trafficManager && car->hasPath()) || m_nextRoad;
				if (!hasNextDestination) {
					car->s = fmod(car->s, mLens); // 正向车辆从头开始循环
				}
			}
			else if (car->laneDirection == Vehicle::LaneDirection::Backward && car->s < 0.0f && mLens > 0) {
				bool hasNextDestination = (m_trafficManager && car->hasPath()) || m_nextRoad;
				if (!hasNextDestination) {
					car->s = mLens + fmod(car->s, mLens); // 逆向车辆从末端开始循环
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


			// 根据车辆方向计算超出距离和新位置
			float overshoot = 0.0f;
			if (vehicle->laneDirection == Vehicle::LaneDirection::Forward) {
				overshoot = vehicle->s - mLens;
				vehicle->s = std::max(0.0f, overshoot); // 从新道路起点开始
			}
			else {
				// 逆向车辆从道路起点移动到末端
				overshoot = -vehicle->s; // s为负值，overshoot为正值
				vehicle->s = mLens - std::max(0.0f, overshoot); // 从新道路末端减去超出距离
			}

			// 根据车辆路径决定下一条道路
			Road* nextRoad = nullptr;
			if (m_trafficManager && vehicle->hasPath()) {
				// 车辆有分配的路径
				if (vehicle->laneDirection == Vehicle::LaneDirection::Forward) {
					// 正向车辆：移动到路径中的下一条道路
					if (vehicle->moveToNextRoad()) {
						uint16 nextRoadId = vehicle->getCurrentRoadId();
						nextRoad = m_trafficManager->getRoadById(nextRoadId);
						LogManager::instance()->logMessage("Forward Vehicle " + std::to_string(vehicle->id) +
							" following path to Road " + std::to_string(nextRoadId));
					}
				}
				else {
					// 逆向车辆：移动到路径中的上一条道路
					if (vehicle->moveToPreviousRoad()) {
						uint16 nextRoadId = vehicle->getCurrentRoadId();
						nextRoad = m_trafficManager->getRoadById(nextRoadId);
						// 设置逆向车辆在新道路的位置（从末端开始）
						if (nextRoad) {
							vehicle->s = nextRoad->mLens - std::max(0.0f, overshoot);
						}
						LogManager::instance()->logMessage("Backward Vehicle " + std::to_string(vehicle->id) +
							" following reverse path to Road " + std::to_string(nextRoadId));
					}
				}

				if (!nextRoad) {
					LogManager::instance()->logMessage("Vehicle " + std::to_string(vehicle->id) +
						" reached end of path - using default next road");
					nextRoad = m_nextRoad; // 路径结束，使用默认下一条道路
				}
			}
			else {
				// 没有路径或Traffic引用，使用默认下一条道路
				nextRoad = m_nextRoad;
			}

			// 添加到目标道路
			if (nextRoad) {
				nextRoad->addCar(vehicle);
				LogManager::instance()->logMessage("Vehicle " + std::to_string(vehicle->id) +
					" transferred from Road " + std::to_string(getRoadId()) +
					" to Road " + std::to_string(nextRoad->getRoadId()) +
					" with overshoot: " + std::to_string(overshoot));
			}
			else {
				LogManager::instance()->logMessage("Vehicle " + std::to_string(vehicle->id) +
					" has no next road - removing from simulation");
			}
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
	//默认跟车模型参数设置
	void Traffic::initializeCarFollowingModels()
	{
		// 设置IDM默认参数
		m_idmParams.v0 = 50.0f;       // 期望速度 (m/s)
		m_idmParams.T = 1.0f;         // 期望时间间隔 (s)
		m_idmParams.s0 = 2.0f;        // 最小间距 (m)
		m_idmParams.a = 500.0f;         // 最大加速度 (m/s²)
		m_idmParams.b = 100.5f;         // 舒适减速度 (m/s²)
		m_idmParams.bmax = 800.0f;     // 最大减速度 (m/s²)
		m_idmParams.noiseLevel = 0.05f;

		// 设置ACC默认参数
		m_accParams.v0 = 50.0f;
		m_accParams.T = 1.0f;
		m_accParams.s0 = 2.0f;
		m_accParams.a = 500.0f;
		m_accParams.b = 10.5f;
		m_accParams.bmax = 800.0f;
		m_accParams.cool = 0.9f;
		m_accParams.noiseLevel = 0.05f;
	}

	std::unique_ptr<ICarFollowingModel> Traffic::createModelForVehicle(Vehicle::VehicleType type)
	{
		switch (m_defaultModelType) {
		case CarFollowingModelFactory::ModelType::IDM:
			return CarFollowingModelFactory::createIDM(m_idmParams);
		case CarFollowingModelFactory::ModelType::ACC:
			return CarFollowingModelFactory::createACC(m_accParams);
		default:
			return CarFollowingModelFactory::createIDM(m_idmParams);
		}
	}

	void Traffic::setDefaultCarFollowingModel(CarFollowingModelFactory::ModelType modelType)
	{
		m_defaultModelType = modelType;
	}

	void Traffic::updateGlobalModelParameters(const IDMModel::Parameters& idmParams, const ACCModel::Parameters& accParams)
	{
		m_idmParams = idmParams;
		m_accParams = accParams;

		// 更新所有现有车辆的模型参数
		for (Vehicle* vehicle : m_Vehicles) {
			if (vehicle->getCarFollowingModel()) {
				// 重新创建跟车模型以应用新参数
				auto newModel = createModelForVehicle(Vehicle::VehicleType::Car);
				newModel->setDriverVariation(vehicle->getDriverFactor());
				vehicle->setCarFollowingModel(std::move(newModel));
			}
		}
	}

	// 驾驶模式配置方法 =

	void Traffic::useConservativeDriving()
	{
		// 保守驾驶：更低速度，更大间距，更温和的加减速
		m_idmParams.v0 = 12.0f;
		m_idmParams.T = 1.8f;
		m_idmParams.s0 = 3.0f;
		m_idmParams.a = 1.5f;
		m_idmParams.b = 2.0f;
		m_idmParams.bmax = 15.0f;
		m_idmParams.noiseLevel = 0.02f;

		m_accParams.v0 = 12.0f;
		m_accParams.T = 1.8f;
		m_accParams.s0 = 3.0f;
		m_accParams.a = 1.5f;
		m_accParams.b = 2.0f;
		m_accParams.bmax = 8.0f;
		m_accParams.cool = 0.95f;
		m_accParams.noiseLevel = 0.02f;

		m_driverVariationCoeff = 0.08f; // 较小的变异系数

		// 更新所有现有车辆
		updateGlobalModelParameters(m_idmParams, m_accParams);

		LogManager::instance()->logMessage("Traffic: Conservative driving mode activated");
	}

	void Traffic::useAggressiveDriving()
	{
		// 激进驾驶：更高速度，更小间距，更强烈的加减速
		m_idmParams.v0 = 20.0f;
		m_idmParams.T = 0.8f;
		m_idmParams.s0 = 1.5f;
		m_idmParams.a = 2.8f;
		m_idmParams.b = 1.2f;
		m_idmParams.bmax = 20.0f;
		m_idmParams.noiseLevel = 0.08f;

		m_accParams.v0 = 20.0f;
		m_accParams.T = 0.8f;
		m_accParams.s0 = 1.5f;
		m_accParams.a = 2.8f;
		m_accParams.b = 1.2f;
		m_accParams.bmax = 12.0f;
		m_accParams.cool = 0.85f;
		m_accParams.noiseLevel = 0.08f;

		m_driverVariationCoeff = 0.25f; // 较大的变异系数

		// 更新所有现有车辆
		updateGlobalModelParameters(m_idmParams, m_accParams);

		LogManager::instance()->logMessage("Traffic: Aggressive driving mode activated");
	}

	void Traffic::useNormalDriving()
	{
		// 正常驾驶：使用默认参数
		initializeCarFollowingModels();
		m_driverVariationCoeff = 0.15f;

		// 更新所有现有车辆
		updateGlobalModelParameters(m_idmParams, m_accParams);

		LogManager::instance()->logMessage("Traffic: Normal driving mode activated");
	}

	void Traffic::setDriverVariationLevel(float level)
	{
		m_driverVariationCoeff = std::max(0.0f, std::min(1.0f, level));

		// 更新所有现有车辆的驾驶员变异
		for (Vehicle* vehicle : m_Vehicles) {
			vehicle->setDriverVariation(m_driverVariationCoeff);
		}

		LogManager::instance()->logMessage("Traffic: Driver variation level set to " + std::to_string(m_driverVariationCoeff));
	}

	// 车辆管理方法

	void Traffic::addMultipleVehicles(int numVehicles)
	{
		if (!m_roadManager || numVehicles <= 0) return;

		const float baseSpeed = 20.0f;  // 基础速度 (m/s)
		const float laneOffset = 4.5f;  // 车道偏移
		const float minGap = 300.0f;     // 车辆之间的最小间距

		// 计算正向和逆向车辆数量（70%正向，30%逆向）
		int forwardVehicles = static_cast<int>(numVehicles * 0.7f);
		int backwardVehicles = numVehicles - forwardVehicles;

		// 添加正向车辆
		for (int i = 0; i < forwardVehicles; ++i) {
			float vehicleSpeed = baseSpeed + (rand() % 5 - 2);
			vehicleSpeed = std::max(5.0f, vehicleSpeed);

			Vehicle* newVehicle = new Vehicle(vehicleSpeed, Vehicle::LaneDirection::Forward);
			newVehicle->id = m_Vehicles.size() + 1;
			// 正向车辆使用正车道偏移（右侧车道）- 更大的偏移确保隔离
			newVehicle->laneOffset = laneOffset + (rand() % 3 - 1) * 0.3f;
			newVehicle->s = i * minGap;

			auto carFollowingModel = createModelForVehicle(Vehicle::VehicleType::Car);
			newVehicle->setCarFollowingModel(std::move(carFollowingModel));
			newVehicle->setDriverVariation(m_driverVariationCoeff);

			m_roadManager->addCar(newVehicle);
			m_Vehicles.push_back(newVehicle);
		}

		// 添加逆向车辆
		for (int i = 0; i < backwardVehicles; ++i) {
			float vehicleSpeed = baseSpeed + (rand() % 5 - 2);
			vehicleSpeed = std::max(5.0f, vehicleSpeed);

			Vehicle* newVehicle = new Vehicle(vehicleSpeed, Vehicle::LaneDirection::Backward);
			newVehicle->id = m_Vehicles.size() + 1;
			// 逆向车辆使用负车道偏移（左侧车道）- 更大的偏移确保隔离
			newVehicle->laneOffset = laneOffset + (rand() % 3 - 1) * 0.3f;
			// 逆向车辆从道路末端开始
			newVehicle->s = m_roadManager->mLens - (i * minGap);

			auto carFollowingModel = createModelForVehicle(Vehicle::VehicleType::Car);
			newVehicle->setCarFollowingModel(std::move(carFollowingModel));
			// 设置驾驶员变异
			newVehicle->setDriverVariation(m_driverVariationCoeff);

			// 添加到道路和车辆列表
			m_roadManager->addCar(newVehicle);
			m_Vehicles.push_back(newVehicle);
		}

		LogManager::instance()->logMessage("Traffic: Added " + std::to_string(numVehicles) + " vehicles. Total vehicles: " + std::to_string(m_Vehicles.size()));
	}
	//车辆密度
	void Traffic::setVehicleDensity(float vehiclesPerKm)
	{
		if (!m_roadManager || vehiclesPerKm <= 0) return;

		// 计算道路总长度（假设）
		float totalRoadLength = 1000.0f; // 1km，可以根据实际道路长度调整
		int targetVehicleCount = static_cast<int>(vehiclesPerKm * totalRoadLength / 1000.0f);

		// 清理现有车辆
		for (Vehicle* vehicle : m_Vehicles) {
			delete vehicle;
		}
		m_Vehicles.clear();

		// 重新创建车辆
		addMultipleVehicles(targetVehicleCount);

		LogManager::instance()->logMessage("Traffic: Set vehicle density to " + std::to_string(vehiclesPerKm) + " vehicles/km, created " + std::to_string(targetVehicleCount) + " vehicles");
	}

	// Road前方车辆

	Vehicle* Road::findLeadingVehicle(const Vehicle* vehicle) const
	{
		if (!vehicle) return nullptr;

		Vehicle* leadingVehicle = nullptr;
		float minDistance = std::numeric_limits<float>::max();

		for (Vehicle* otherVehicle : mCars) {
			if (otherVehicle == vehicle) continue;

			// 只考虑同方向行驶的车辆
			if (otherVehicle->laneDirection != vehicle->laneDirection) continue;

			float distance;
			if (vehicle->laneDirection == Vehicle::LaneDirection::Forward) {
				// 正向行驶：寻找前方车辆
				if (otherVehicle->s > vehicle->s) {
					distance = otherVehicle->s - vehicle->s;
				}
				else {
					continue; // 跳过后方车辆
				}
			}
			else {
				// 反向行驶：寻找前方车辆（s值较小的）
				if (otherVehicle->s < vehicle->s) {
					distance = vehicle->s - otherVehicle->s;
				}
				else {
					continue; // 跳过后方车辆
				}
			}

			if (distance < minDistance) {
				minDistance = distance;
				leadingVehicle = otherVehicle;
			}
		}

		return leadingVehicle;
	}
	//车辆间隔
	float Road::calculateGapToLeadingVehicle(const Vehicle* vehicle) const
	{
		Vehicle* leadingVehicle = findLeadingVehicle(vehicle);
		if (!leadingVehicle) {
			return 1000.0f; // 返回一个很大的值表示没有前车
		}

		float gap;
		if (vehicle->laneDirection == Vehicle::LaneDirection::Forward) {
			gap = leadingVehicle->s - vehicle->s - 5.0f; // 假设车长5米
		}
		else {
			gap = vehicle->s - leadingVehicle->s - 5.0f; // 假设车长5米
		}

		return std::max(2.0f, gap); // 确保间距不为负或太小
	}

	//  跟车模型

	void Vehicle::setCarFollowingModel(std::unique_ptr<ICarFollowingModel> model)
	{
		m_carFollowingModel = std::move(model);
		if (m_carFollowingModel) {
			m_carFollowingModel->setDriverVariation(m_driverFactor);
		}
	}

	void Vehicle::setDriverVariation(float variationCoeff)
	{
		m_driverVariationCoeff = variationCoeff;
		// 计算驾驶员因子：使用均匀分布，变异系数转换为标准差
		static std::random_device rd;
		static std::mt19937 gen(rd());
		std::uniform_real_distribution<float> dist(-0.5f, 0.5f);

		m_driverFactor = 1.0f + std::sqrt(12.0f) * variationCoeff * dist(gen);
		m_driverFactor = std::max(0.1f, std::min(2.0f, m_driverFactor)); // 限制在合理范围内

		if (m_carFollowingModel) {
			m_carFollowingModel->setDriverVariation(m_driverFactor);
		}
	}

	void Vehicle::applyCarFollowing(float deltaTime, const Vehicle* leadingVehicle, float gap)
	{
		if (!m_carFollowingModel || !leadingVehicle) {
			return;
		}

		// 使用跟车模型计算加速度
		float newAcceleration = m_carFollowingModel->calculateAcceleration(
			gap, speed, leadingVehicle->speed, leadingVehicle->acc);
		// 限制加速度以避免过度激进的行为
		newAcceleration = std::max(-6.0f, std::min(3.0f, newAcceleration));
		// 更新加速度
		acc = newAcceleration;
		//// 如果速度过低且间距足够，给一个最小速度以避免完全停止
		//if (speed < 0.5f && gap > 10.0f) {
		//	speed = 0.5f; // 最小移动速度
		//}
		// 更新速度（确保不为负）
		speed = std::max(0.0f, speed + acc * deltaTime);

		// 更新位置
		if (laneDirection == LaneDirection::Forward) {
			s += speed * deltaTime;
		}
		else {
			s -= speed * deltaTime;
		}
	}
	//城市路径生成
	void Traffic::generateAllPaths()
	{
		if (m_pathsGenerated) return;

		// 切换模式：true=使用静态路径，false=动态生成路径
		bool useStaticPaths = true; // 首次运行设置为false，生成路径后改为true

		if (useStaticPaths) {
			// 静态模式：直接加载预定义路径
            
			m_allPaths = STATIC_PATHS;
			LogManager::instance()->logMessage("使用静态路径，共" + std::to_string(m_allPaths.size()) + "条路径");
		}
		else {
			// 动态模式：计算所有路径
			m_allPaths.clear();
			std::set<std::vector<uint16>> uniquePaths;

			LogManager::instance()->logMessage("开始动态生成路径，共" + std::to_string(m_allRoads.size()) + "条道路");

			// 1. 单条道路路径
			for (Road* road : m_allRoads) {
				std::vector<uint16> singlePath = { road->getRoadId() };
				uniquePaths.insert(singlePath);
			}

			// 2. 多条道路路径（递归生成，最大深度5）
			for (Road* startRoad : m_allRoads) {
				std::vector<uint16> currentPath = { startRoad->getRoadId() };
				std::set<uint16> visited = { startRoad->getRoadId() };
				generatePathsRecursive(startRoad, currentPath, visited, uniquePaths, 5);
			}

			// 转换为vector
			for (const auto& path : uniquePaths) {
				m_allPaths.push_back(path);
			}

			LogManager::instance()->logMessage("动态路径生成完成，共" + std::to_string(m_allPaths.size()) + "条路径");

			// 打印所有路径供复制到StaticPaths.h
			LogManager::instance()->logMessage("=== 复制以下路径到StaticPaths.h ===");
			for (size_t i = 0; i < m_allPaths.size(); ++i) {
				std::string pathLine = "        {";
				for (size_t j = 0; j < m_allPaths[i].size(); ++j) {
					pathLine += std::to_string(m_allPaths[i][j]);
					if (j < m_allPaths[i].size() - 1) pathLine += ", ";
				}
				pathLine += "},";

				// 添加注释
				pathLine += "  // Path[" + std::to_string(i) + "]: ";
				for (size_t j = 0; j < m_allPaths[i].size(); ++j) {
					pathLine += "Road[" + std::to_string(m_allPaths[i][j]) + "]";
					if (j < m_allPaths[i].size() - 1) pathLine += "->";
				}

				LogManager::instance()->logMessage(pathLine);
			}
			LogManager::instance()->logMessage("=== 复制完成，然后设置useStaticPaths=true ===");
		}

         		m_pathsGenerated = true;
	}

	void Traffic::generatePathsRecursive(Road* currentRoad, std::vector<uint16>& currentPath,
		std::set<uint16>& visited, std::set<std::vector<uint16>>& uniquePaths, int maxDepth)
	{
		if (maxDepth <= 0) return;

		Road* nextRoad = currentRoad->getNextRoad();
		if (nextRoad && visited.find(nextRoad->getRoadId()) == visited.end()) {
			// 添加到当前路径
			currentPath.push_back(nextRoad->getRoadId());
			visited.insert(nextRoad->getRoadId());
			uniquePaths.insert(currentPath);

			// 递归继续
			generatePathsRecursive(nextRoad, currentPath, visited, uniquePaths, maxDepth - 1);

			// 回溯
			currentPath.pop_back();
			visited.erase(nextRoad->getRoadId());
		}

		// 尝试从当前道路的目标城市出发的其他道路
		uint16 destCityId = currentRoad->getDestinationCityId();
		if (destCityId != 0) {
			auto it = m_cityConnections.find(destCityId);
			if (it != m_cityConnections.end()) {
				for (Road* candidateRoad : it->second) {
					if (candidateRoad->getSourceCityId() == destCityId &&
						visited.find(candidateRoad->getRoadId()) == visited.end()) {

						currentPath.push_back(candidateRoad->getRoadId());
						visited.insert(candidateRoad->getRoadId());
						uniquePaths.insert(currentPath);

						generatePathsRecursive(candidateRoad, currentPath, visited, uniquePaths, maxDepth - 1);

						currentPath.pop_back();
						visited.erase(candidateRoad->getRoadId());
					}
				}
			}
		}
	}

	void Traffic::assignPathToVehicle(Vehicle* vehicle, int pathIndex)
	{
		if (!vehicle || pathIndex < 0 || pathIndex >= m_allPaths.size()) return;

		const auto& path = m_allPaths[pathIndex];
		if (path.empty()) return;

		// 验证车辆当前道路与路径起始道路是否匹配
		uint16 currentRoadId = 0;
		for (Road* road : m_allRoads) {
			// 检查车辆是否在这条道路上
			auto& cars = road->mCars; // 需要访问道路的车辆列表
			if (std::find(cars.begin(), cars.end(), vehicle) != cars.end()) {
				currentRoadId = road->getRoadId();
				break;
			}
		}

		// 如果当前道路与路径起始道路不匹配，需要重新分配车辆
		if (currentRoadId != path[0]) {
			// 将车辆从当前道路移除
			for (Road* road : m_allRoads) {
				auto& cars = road->mCars;
				auto it = std::find(cars.begin(), cars.end(), vehicle);
				if (it != cars.end()) {
					cars.erase(it);
					break;
				}
			}

			// 将车辆添加到路径起始道路
			Road* startRoad = getRoadById(path[0]);
			if (startRoad) {
				vehicle->s = 0.0f; // 重置位置
				startRoad->addCar(vehicle);
				LogManager::instance()->logMessage("Vehicle " + std::to_string(vehicle->id) +
					" moved to start road " + std::to_string(path[0]) + " for path assignment");
			}
		}

		vehicle->setPath(path);
		LogManager::instance()->logMessage("Vehicle[" + std::to_string(vehicle->id) + "] 分配路径[" + std::to_string(pathIndex) + "] 起始道路:" + std::to_string(path[0]));
	}
	//反向车辆分配方法
	void Traffic::assignBackwardPathToVehicle(Vehicle* vehicle, int pathIndex)
	{
		if (!vehicle || pathIndex < 0 || pathIndex >= m_allPaths.size()) return;

		const auto& forwardPath = m_allPaths[pathIndex];
		if (forwardPath.empty()) return;

		// 创建反向路径（颠倒原路径）
		std::vector<uint16> backwardPath;
		for (auto it = forwardPath.rbegin(); it != forwardPath.rend(); ++it) {
			backwardPath.push_back(*it);
		}

		// 验证车辆当前道路与反向路径起始道路是否匹配
		uint16 currentRoadId = 0;
		for (Road* road : m_allRoads) {
			auto& cars = road->mCars;
			if (std::find(cars.begin(), cars.end(), vehicle) != cars.end()) {
				currentRoadId = road->getRoadId();
				break;
			}
		}

		// 如果当前道路与反向路径起始道路不匹配，需要重新分配车辆
		if (currentRoadId != backwardPath[0]) {
			// 将车辆从当前道路移除
			for (Road* road : m_allRoads) {
				auto& cars = road->mCars;
				auto it = std::find(cars.begin(), cars.end(), vehicle);
				if (it != cars.end()) {
					cars.erase(it);
					break;
				}
			}

			// 将车辆添加到反向路径起始道路（原路径的末端道路）
			Road* startRoad = getRoadById(backwardPath[0]);
			if (startRoad) {
				vehicle->s = startRoad->getRoadLength(); // 逆向车辆从道路末端开始
				startRoad->addCar(vehicle);
				LogManager::instance()->logMessage("Backward Vehicle " + std::to_string(vehicle->id) +
					" moved to start road " + std::to_string(backwardPath[0]) + " for backward path assignment");
			}
		}

		vehicle->setPath(backwardPath);
		LogManager::instance()->logMessage("Backward Vehicle[" + std::to_string(vehicle->id) + "] 分配反向路径[" + std::to_string(pathIndex) + "] 起始道路:" + std::to_string(backwardPath[0]));

	}

	void Traffic::assignPathsToAllVehicles()
	{
		if (m_allPaths.empty()) {
			LogManager::instance()->logMessage("No paths available for vehicle assignment.");
			return;
		}

		LogManager::instance()->logMessage("Starting flexible path assignment for " + std::to_string(m_Vehicles.size()) + " vehicles with " + std::to_string(m_allPaths.size()) + " available paths.");

		// 配置参数：可以调整这些值来控制分配策略
		const int maxSpecificPaths = std::min(static_cast<int>(m_allPaths.size()), static_cast<int>(m_Vehicles.size())); // 最多分配特定路径的车辆数量
		const bool enableRandomAssignment = true; // 是否为未分配特定路径的车辆随机分配路径

		int assignedSpecificPaths = 0;
		int assignedRandomPaths = 0;

		// 第一阶段：为前N辆车分配特定路径（按车辆和路径索引匹配）
		for (int i = 0; i < std::min(maxSpecificPaths, static_cast<int>(m_Vehicles.size())); ++i) {
			if (i < m_allPaths.size()) {
				Vehicle* vehicle = m_Vehicles[i];

				if (vehicle->laneDirection == Vehicle::LaneDirection::Forward) {
					assignPathToVehicle(vehicle, i);
					assignedSpecificPaths++;
					LogManager::instance()->logMessage("Forward Vehicle " + std::to_string(vehicle->id) + " assigned specific path " + std::to_string(i));
				}
				else if (vehicle->laneDirection == Vehicle::LaneDirection::Backward) {
					assignBackwardPathToVehicle(vehicle, i);
					assignedSpecificPaths++;
					LogManager::instance()->logMessage("Backward Vehicle " + std::to_string(vehicle->id) + " assigned specific backward path " + std::to_string(i));
				}
			}
		}

		// 第二阶段：为剩余车辆随机分配路径（如果启用）
		if (enableRandomAssignment) {
			for (int i = maxSpecificPaths; i < m_Vehicles.size(); ++i) {
				Vehicle* vehicle = m_Vehicles[i];

				// 随机选择一个路径索引
				int randomPathIndex = rand() % m_allPaths.size();

				if (vehicle->laneDirection == Vehicle::LaneDirection::Forward) {
					assignPathToVehicle(vehicle, randomPathIndex);
					assignedRandomPaths++;
					LogManager::instance()->logMessage("Forward Vehicle " + std::to_string(vehicle->id) + " assigned random path " + std::to_string(randomPathIndex));
				}
				else if (vehicle->laneDirection == Vehicle::LaneDirection::Backward) {
					assignBackwardPathToVehicle(vehicle, randomPathIndex);
					assignedRandomPaths++;
					LogManager::instance()->logMessage("Backward Vehicle " + std::to_string(vehicle->id) + " assigned random backward path " + std::to_string(randomPathIndex));
				}
			}
		}
	}

	// Vehicle路径跟踪方法

	void Vehicle::setPath(const std::vector<uint16>& roadIds)
	{
		m_pathRoads = roadIds;
		if (!roadIds.empty()) {
			m_currentRoadIndex = 0;  // 确保从路径的第一个道路开始
		}
		else {
			m_currentRoadIndex = -1; // 无效状态
		}
	}

	bool Vehicle::moveToNextRoad()
	{
		if (m_currentRoadIndex + 1 < m_pathRoads.size()) {
			m_currentRoadIndex++;
			s = 0.0f; // 重置在新道路上的位置
			return true;
		}
		else if (!m_pathRoads.empty()) {
			// 路径循环：回到路径起点
			m_currentRoadIndex = 0;
			s = 0.0f; // 重置在新道路上的位置
			return true;
		}
		return false; // 空路径时返回false
	}
	

	bool Vehicle::moveToPreviousRoad()
	{
		if (m_currentRoadIndex > 0)
		{
			m_currentRoadIndex--;
			s = 0.0f;
			return true;
		}
		return false;
	}

	uint16 Vehicle::getCurrentRoadId() const
	{
		if (m_currentRoadIndex >= 0 && m_currentRoadIndex < m_pathRoads.size()) {
			return m_pathRoads[m_currentRoadIndex];
		}
		return 0; // 无效ID
	}
	bool Vehicle::hasPath() const
	{
		return !m_pathRoads.empty() && m_currentRoadIndex >= 0 && m_currentRoadIndex < m_pathRoads.size();
	}

	//路径管理

	
	Road* Traffic::getRoadById(uint16 roadId)
	{
		for (Road* road : m_allRoads) {
			if (road && road->getRoadId() == roadId) {
				return road;
			}
		}
		return nullptr;
	}



	

}

