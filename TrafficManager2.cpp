#include "TrafficManager.h"
#include <functional>
#include "EchoSceneManager.h"
#include "EchoWorldManager.h"
#include "Actor/EchoActorManager.h"
#include "Actor/EchoActor.h"
#include "EchoSphericalTerrainComponent.h"
#include"EchoPlanetRoad.h"
#include"EchoPlanetRoadResource.h"
#include "EchoPiecewiseBezierUtil.h"
#include"chrono"
#include <ctime>
#include <cstdlib>
#include"CarFollowingModel.h"
#include <functional>
#include"EchoRoot.h"
#include"EchoLogManager.h"

namespace Echo
{

	void Traffic::Barrier::wait()
	{
		/*m_promise.get_future().wait();
		std::promise<void> _promise;
		std::swap(m_promise, _promise);*/

		std::unique_lock<std::mutex> lock(m_mutex);
		// wait解锁mutex并等待，直到被唤醒且m_signaled为true
		m_cv.wait(lock, [this] { return m_signaled; });
		// 成功等待后，重置标志以便下次使用
		m_signaled = false;

	}

	void Traffic::Barrier::signal()
	{
		//m_promise.set_value();
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_signaled = true;
		}
		// 通知一个正在等待的线程
		m_cv.notify_one();
	}

	Traffic::Traffic(SceneManager* InSceneManger, WorldManager* InWorldMgr)

	{
		initializeCarFollowingModels();
		srand(static_cast<unsigned int>(time(nullptr)));
		//m_workBarrier.signal();
	}

	Traffic::~Traffic()
	{
		m_bQuite = true;
		m_mainBarrier.signal();

		if (m_thread && m_thread->joinable())
		{
			m_thread->join();
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
		m_nodeConnections.clear();

		// 清理路径数据
		m_allPaths.clear();
	}
	//ve
	void Traffic::_tick(float dt)
	{
		for (Road* road : m_allRoads)
		{
			if (road) {
				road->update(dt);
			}
		}
	}

	void Traffic::UpdatePositon()
	{
		for (Vehicle* vehicle : m_Vehicles)
		{
			if (vehicle)
			{

				vehicle->updatePos();
			}
		}
	}

	bool Traffic::OnActorCreateFinish()
	{


		LogManager::instance()->logMessage("FINISH");
		
		return false;
	}


	void Traffic::OnCreateFinish()
	{
		LogManager::instance()->logMessage("Planet data loading complete. Starting Traffic worker thread now...");

		m_thread = new std::thread(std::bind(&Traffic::onTick, this));
		m_workBarrier.wait();
		LogManager::instance()->logMessage("Data initialization complete. Starting vehicle creation on main thread...");
		initVehicle();
		
		m_isInitialized = true;
		LogManager::instance()->logMessage("All initialization finished.");

		m_mainBarrier.signal();

	}

	void Traffic::onTick()
	{
		using namespace std::chrono;
		static auto lastTikcBegin = steady_clock::now();
		static bool initialized = false;
		if (!initialized)
		{
			initRoads();
			initialized = true;
			m_workBarrier.signal();
			//


		}
		m_mainBarrier.wait();
		//初始化标记

		while (!m_bQuite)
		{

			auto tickBegin = steady_clock::now();
			std::chrono::nanoseconds dt = tickBegin - lastTikcBegin;
			//_tick(dt.count() / 1000.f);
			_tick(std::chrono::duration<float>(dt).count());

			lastTikcBegin = tickBegin;

			//等待 主线程使用数据更新场景

			UpdatePositon();

			m_workBarrier.signal();
			m_mainBarrier.wait();



		}
	}

	//等待工作线程完成当前帧的数据计算，更新数据
	void Traffic::onUpdate()
	{
		if (!m_isInitialized) {
			// 初始化还未完成，跳过此次更新
			return;
		}
		
		m_workBarrier.wait();
	

		m_mainBarrier.signal();
	}

	void Traffic::initVehicle()
	{
		if (m_pathsGenerated && !m_Vehicles.empty())
		{
			// 如果车辆已存在，先清理 (避免重复创建)
			for (auto v : m_Vehicles) delete v;
			m_Vehicles.clear();
		}

		if (m_pathsGenerated)
		{
			addMultipleVehicles(2000);
			assignPathsToAllVehicles();
		}
	}
	
	void Traffic::initRoads()
	{
		std::string sLevelName = "levels/" + mWorldMgr->getLevelName();
		std::string jsonFilePath = sLevelName + "/other/biejing_highway_connect_graph.json";

		JsonRoadData jsonRoadData = parseJsonRoadData(jsonFilePath);
		if (!jsonRoadData.nodes.empty() && !jsonRoadData.links.empty())
		{
			m_jsonRoadData = jsonRoadData;
			createConnectedRoadNetworkFromJson(jsonRoadData);

			
		}
	}


	void Traffic::OnDestroy()
	{
		//移除小车  销毁
	}
	//道路链接
	

	Road::Road(const JsonRoadLink& linkData, const JsonRoadNode& sourceNode, const JsonRoadNode& targetNode)
		: mRoadId(linkData.road_id)
		, mRoadName(linkData.road_name)
		, mSourceNodeId(linkData.source)
		, mTargetNodeId(linkData.target)
		, mStartPos(sourceNode.geometry)
		, mEndPos(targetNode.geometry)
		, mLens(linkData.length)
	{
		// 为JSON道路创建简单的分段 //复杂分段？
		// 这里创建一个简单的直线分段
		mSeg.push_back(mLens);

		LogManager::instance()->logMessage("Created Highway Road[" + std::to_string(mRoadId) +
			"] '" + mRoadName + "' from Node[" + std::to_string(mSourceNodeId) +
			"] to Node[" + std::to_string(mTargetNodeId) + "] length=" + std::to_string(mLens));
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

		updateEnvironment();
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
					/// 使用跟车模型计算自由流加速度（传入大间距模拟无前车）
					const float largeGap = 1000.0f; // 大间距模拟自由流
					const float dummyLeadSpeed = car->speed; // 虚拟前车速度与自己相同

					// 使用跟车模型计算自由流加速度
					car->acc = car->getCarFollowingModel()->calculateAcceleration(
						largeGap, car->speed, dummyLeadSpeed, 0.0f);

					// 限制加速度范围
					car->acc = std::max(-8.0f, std::min(3.0f, car->acc));


					// 更新速度
					car->speed = std::max(0.0f, car->speed + car->acc * deltaTime);
					//
					car->speed = std::min(car->speed, 18.0f);

					// 更新位置

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

			
		
		
			//car->updatePos();
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
		: s(0.f), speed(initialSpeed > 0 ? initialSpeed : 1.0f), acc(0.f), pos(Vector3::ZERO), laneDirection(direction)
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
	void Road::setNextRoad(Road* nextRoad) { m_nextRoad = nextRoad; }
	//默认跟车模型参数设置
	void Traffic::initializeCarFollowingModels()
	{
		// 设置IDM默认参数
		m_idmParams.v0 = 25.0f;       // 期望速度 (m/s)
		m_idmParams.T = 1.0f;         // 期望时间间隔 (s)
		m_idmParams.s0 = 3.0f;        // 最小间距 (m)
		m_idmParams.a = 8.0f;         // 最大加速度 (m/s²)
		m_idmParams.b = 10.5f;         // 舒适减速度 (m/s²)
		m_idmParams.bmax = 20.0f;     // 最大减速度 (m/s²)
		m_idmParams.noiseLevel = 0.1f;

		// 设置ACC默认参数
		m_accParams.v0 = 25.0f;
		m_accParams.T = 1.0f;
		m_accParams.s0 = 3.0f;
		m_accParams.a = 8.0f;
		m_accParams.b = 10.5f;
		m_accParams.bmax = 20.0f;
		m_accParams.cool = 0.3f;
		m_accParams.noiseLevel = 0.1f;
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

		const float baseSpeed = 3.0f;  // 基础速度 (m/s)
		const float laneOffset = 4.5f;  // 车道偏移
		const float minGap = 80.0f;     // 车辆之间的最小间距

		// 计算正向和逆向车辆数量（70%正向，30%逆向）
		int forwardVehicles = static_cast<int>(numVehicles);
		int backwardVehicles = numVehicles - forwardVehicles;

		// 添加正向车辆
		for (int i = 0; i < forwardVehicles; ++i) {

			float vehicleSpeed = baseSpeed + (rand() % 5 - 2) * 0.5;
			vehicleSpeed = std::max(1.0f, vehicleSpeed);

			Vehicle* newVehicle = new Vehicle(vehicleSpeed, Vehicle::LaneDirection::Forward);
			newVehicle->id = m_Vehicles.size() + 1;
			// 正向车辆使用正车道偏移（右侧车道）- 更大的偏移确保隔离
			newVehicle->laneOffset = laneOffset + (rand() % 3 - 1) * 0.3f;
			newVehicle->s = i * minGap;

			auto carFollowingModel = createModelForVehicle(Vehicle::VehicleType::Car);
			newVehicle->setCarFollowingModel(std::move(carFollowingModel));

			//newVehicle->setDriverVariation(m_driverVariationCoeff);
			//float driverVariation = 0.25f + (rand() % 10) * 0.01f; // 0.25-0.7的变异系数
			//newVehicle->setDriverVariation(driverVariation);
			m_roadManager->addCar(newVehicle);
			m_Vehicles.push_back(newVehicle);
		}



		// 添加逆向车辆
		/*for (int i = 0; i < backwardvehicles; ++i) {
			float vehiclespeed = basespeed + (rand() % 5 - 2);
			vehiclespeed = std::max(5.0f, vehiclespeed);

			vehicle* newvehicle = new vehicle(vehiclespeed, vehicle::lanedirection::backward);
			newvehicle->id = m_vehicles.size() + 1;
			 逆向车辆使用负车道偏移（左侧车道）- 更大的偏移确保隔离
			newvehicle->laneoffset = laneoffset + (rand() % 3 - 1) * 0.3f;
			 逆向车辆从道路末端开始
			newvehicle->s = m_roadmanager->mlens - (i * mingap);

			auto carfollowingmodel = createmodelforvehicle(vehicle::vehicletype::car);
			newvehicle->setcarfollowingmodel(std::move(carfollowingmodel));
			 设置驾驶员变异
			newvehicle->setdrivervariation(m_drivervariationcoeff);

			 添加到道路和车辆列表
			m_roadmanager->addcar(newvehicle);
			m_vehicles.push_back(newvehicle);
		}*/

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


	void Road::sortVehicles()
	{
		// 
		// 注意：这个排序适用于所有车辆，前车查找逻辑会根据方向进行调整
		std::sort(mCars.begin(), mCars.end(), [](Vehicle* a, Vehicle* b) {
			return a->s > b->s;
			});
	}

	void Road::updateEnvironment()
	{
		if (mCars.empty()) return;

		// 首先对车辆按位置排序
		sortVehicles();

		// 为每个车辆更新环境索引
		for (int i = 0; i < static_cast<int>(mCars.size()); ++i) {
			updateLeadIndex(i);
			updateLagIndex(i);
		}
	}

	void Road::updateLeadIndex(int i)
	{
		int n = static_cast<int>(mCars.size());
		if (n == 0 || i < 0 || i >= n) return;

		Vehicle* currentVehicle = mCars[i];
		currentVehicle->iLead = -1; // 默认无前车

		// 非环形道路：根据车辆方向查找前车
		if (currentVehicle->laneDirection == Vehicle::LaneDirection::Forward) {
			// 正向车辆：前车是位置更大的同方向车辆（在排序数组中的更前位置）
			for (int j = i - 1; j >= 0; j--) {
				if (mCars[j]->laneDirection == Vehicle::LaneDirection::Forward &&
					mCars[j]->s > currentVehicle->s) {
					currentVehicle->iLead = j;
					break;
				}
			}
		}
		else {
			// 逆向车辆：前车是位置更小的同方向车辆（在排序数组中的更后位置）
			for (int j = i + 1; j < n; j++) {
				if (mCars[j]->laneDirection == Vehicle::LaneDirection::Backward &&
					mCars[j]->s < currentVehicle->s) {
					currentVehicle->iLead = j;
					break;
				}
			}
		}
	}

	void Road::updateLagIndex(int i)
	{
		int n = static_cast<int>(mCars.size());
		if (n == 0 || i < 0 || i >= n) return;

		Vehicle* currentVehicle = mCars[i];
		if (currentVehicle->laneDirection == Vehicle::LaneDirection::Forward) {
			// 正向车辆：后车是位置更小的同方向车辆（在排序数组中的更后位置）
			for (int j = i + 1; j < n; j++) {
				if (mCars[j]->laneDirection == Vehicle::LaneDirection::Forward &&
					mCars[j]->s < currentVehicle->s) {
					currentVehicle->iLag = j;
					break;
				}
			}
		}
		else {
			// 逆向车辆：后车是位置更大的同方向车辆（在排序数组中的更前位置）
			for (int j = i - 1; j >= 0; j--) {
				if (mCars[j]->laneDirection == Vehicle::LaneDirection::Backward &&
					mCars[j]->s > currentVehicle->s) {
					currentVehicle->iLag = j;
					break;
				}
			}
		}
	}

	Vehicle* Road::getVehicleByIndex(int index) const
	{
		if (index >= 0 && index < static_cast<int>(mCars.size())) {
			return mCars[index];
		}
		return nullptr;
	}

	// 使用索引系统的新findLeadingVehicle实现
	Vehicle* Road::findLeadingVehicle(const Vehicle* vehicle) const
	{
		if (!vehicle || vehicle->iLead < 0) return nullptr;

		Vehicle* leadingVehicle = getVehicleByIndex(vehicle->iLead);

		// 验证前车确实在前方且同方向
		if (leadingVehicle && leadingVehicle->laneDirection == vehicle->laneDirection) {
			// 非环形道路：简单位置比较
			bool isAhead = false;
			if (vehicle->laneDirection == Vehicle::LaneDirection::Forward) {
				isAhead = (leadingVehicle->s > vehicle->s);
			}
			else {
				isAhead = (leadingVehicle->s < vehicle->s);
			}

			if (isAhead) {
				return leadingVehicle;
			}
		}

		return nullptr;
	}
	//车辆间隔 - 距离计算
	float Road::calculateGapToLeadingVehicle(const Vehicle* vehicle) const
	{
		if (!vehicle || vehicle->iLead < 0) {
			return 1000.0f; // 无前车时返回大值（自由流）
		}

		Vehicle* leadingVehicle = getVehicleByIndex(vehicle->iLead);
		if (!leadingVehicle) {
			return 1000.0f;
		}

		// 非环形道路：直接计算间距
		float gap = 0.0f;
		if (vehicle->laneDirection == Vehicle::LaneDirection::Forward) {
			// 正向车辆：前车位置 - 前车长度 - 当前车辆位置
			gap = leadingVehicle->s - leadingVehicle->m_length - vehicle->s;
		}
		else {
			// 逆向车辆：当前车辆位置 - 当前车辆长度 - 前车位置
			gap = vehicle->s - vehicle->m_length - leadingVehicle->s;
		}

		return std::max(0.0f, gap); // 确保间距非负
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
		// 限制加速度
		newAcceleration = std::max(-15.0f, std::min(8.0f, newAcceleration));
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

		// 完全动态路径生成 - 基于实际道路网络自动创建路径
		m_allPaths.clear();

		LogManager::instance()->logMessage("开始动态生成路径，共" + std::to_string(m_allRoads.size()) + "条道路");

		// 1. 为每条道路生成单道路路径
		for (Road* road : m_allRoads) {
			if (road) {
				std::vector<uint16> singlePath = { road->getRoadId() };
				m_allPaths.push_back(singlePath);
			}
		}

		// 2. 基于道路连接关系生成多道路路径
		for (Road* startRoad : m_allRoads) {
			if (!startRoad) continue;

			// 使用递归方法生成从当前道路开始的所有路径
			std::vector<uint16> currentPath = { startRoad->getRoadId() };
			std::set<uint16> visited = { startRoad->getRoadId() };
			std::set<std::vector<uint16>> uniquePaths;

			generatePathsRecursive(startRoad, currentPath, visited, uniquePaths, 5);

			// 将生成的路径添加到总路径列表
			for (const auto& path : uniquePaths) {
				m_allPaths.push_back(path);
			}
		}

		// 3. 移除重复路径
		std::set<std::vector<uint16>> uniquePathSet(m_allPaths.begin(), m_allPaths.end());
		m_allPaths.assign(uniquePathSet.begin(), uniquePathSet.end());

		LogManager::instance()->logMessage("动态路径生成完成，共" + std::to_string(m_allPaths.size()) + "条路径");

		// 输出路径信息用于调试
		for (size_t i = 0; i < m_allPaths.size(); ++i) {
			std::string pathStr = "Path[" + std::to_string(i) + "]: ";
			for (size_t j = 0; j < m_allPaths[i].size(); ++j) {
				pathStr += "Road[" + std::to_string(m_allPaths[i][j]) + "]";
				if (j < m_allPaths[i].size() - 1) pathStr += "->";
			}
			LogManager::instance()->logMessage(pathStr);
		}
		if (m_allPaths.size() > 10) {
			LogManager::instance()->logMessage("... 还有 " + std::to_string(m_allPaths.size() - 10) + " 条路径");
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
		
			// 尝试从当前道路的目标节点出发的其他道路
		uint16 destNodeId = currentRoad->getDestinationNodeId();
		if (destNodeId != 0) {
			auto it = m_nodeConnections.find(destNodeId);
			if (it != m_nodeConnections.end()) {
				for (Road* candidateRoad : it->second) {
					if (candidateRoad->getSourceNodeId() == destNodeId &&
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

		float originalPosition = vehicle->s;

		// 验证车辆当前道路与路径起始道路是否匹配
		uint16 currentRoadId = 0;

		Road* currentRoad = nullptr;

		for (Road* road : m_allRoads) {
			// 检查车辆是否在这条道路上
			auto& cars = road->mCars; // 需要访问道路的车辆列表
			if (std::find(cars.begin(), cars.end(), vehicle) != cars.end()) {
				currentRoadId = road->getRoadId();

				currentRoad = road;

				break;
			}
		}

		// 如果当前道路与路径起始道路不匹配，需要重新分配车辆
		if (currentRoadId != path[0]) {
			// 将车辆从当前道路移除
			/*for (Road* road : m_allRoads) {
				auto& cars = road->mCars;*/
			if (currentRoad) {
				auto& cars = currentRoad->mCars;
				auto it = std::find(cars.begin(), cars.end(), vehicle);
				if (it != cars.end()) {
					cars.erase(it);
					//break;
				}
			}

			// 将车辆添加到路径起始道路
			Road* startRoad = getRoadById(path[0]);
			if (startRoad) {
				//vehicle->s = 0.0f; // 重置位置
				vehicle->s = originalPosition;//间距位置
				startRoad->addCar(vehicle);
				LogManager::instance()->logMessage("Vehicle " + std::to_string(vehicle->id) +
					" moved to start road " + std::to_string(path[0]) + " at position " + std::to_string(vehicle->s));
			}
		}

		vehicle->setPath(path);
		LogManager::instance()->logMessage("Vehicle[" + std::to_string(vehicle->id) + "] 分配路径[" + std::to_string(pathIndex) + "] 起始道路:" + std::to_string(path[0]) + " 位置:" + std::to_string(vehicle->s));
	}
	//反向车辆分配方法   7.9间距位置
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
	//路径分配
	

	void Traffic::assignPathsToAllVehicles()
	{
		if (m_allPaths.empty()) {
			LogManager::instance()->logMessage("No paths available for vehicle assignment.");
			return;
		}

		LogManager::instance()->logMessage("Starting intelligent path assignment for " + std::to_string(m_Vehicles.size()) + " vehicles with " + std::to_string(m_allPaths.size()) + " available paths.");

		// 智能分配车辆到不同道路，保持minGap间距
		const float minGap = 80.0f; // 
		// 1. 按道路分组分配车辆
		std::map<uint16, std::vector<Vehicle*>> roadVehicleGroups;

		// 2. 将车辆按照它们当前的相对间距分组分配到不同道路
		int vehiclesPerRoad = std::max(1, static_cast<int>(m_Vehicles.size()) / static_cast<int>(m_allRoads.size()));
		int roadIndex = 0;

		for (int i = 0; i < m_Vehicles.size(); ++i) {
			Vehicle* vehicle = m_Vehicles[i];

			// 确定目标道路
			if (roadIndex >= m_allRoads.size()) {
				roadIndex = 0; // 循环回到第一条道路
			}
			Road* targetRoad = m_allRoads[roadIndex];

			// 为车辆分配合适的路径（从目标道路开始的路径）
			int pathIndex = -1;
			for (int p = 0; p < m_allPaths.size(); ++p) {
				if (!m_allPaths[p].empty() && m_allPaths[p][0] == targetRoad->getRoadId()) {
					pathIndex = p;
					break;
				}
			}

			// 如果没找到合适的路径，使用第一个可用路径
			if (pathIndex == -1 && !m_allPaths.empty()) {
				pathIndex = 0;
			}

			if (pathIndex >= 0) {
				// 计算车辆在目标道路上的理想位置（保持minGap间距）
				int vehicleOrderOnRoad = roadVehicleGroups[targetRoad->getRoadId()].size();
				float idealPosition = vehicleOrderOnRoad * minGap;

				// 保存原始位置，然后设置理想位置
				float originalS = vehicle->s;
				vehicle->s = idealPosition;

				// 分配路径
				if (vehicle->laneDirection == Vehicle::LaneDirection::Forward) {
					assignPathToVehicle(vehicle, pathIndex);
				}
				else {
					assignBackwardPathToVehicle(vehicle, pathIndex);
				}

				// 记录这辆车已分配到这条道路
				roadVehicleGroups[targetRoad->getRoadId()].push_back(vehicle);

				LogManager::instance()->logMessage("Vehicle " + std::to_string(vehicle->id) +
					" assigned to Road " + std::to_string(targetRoad->getRoadId()) +
					" at position " + std::to_string(vehicle->s) +
					" (order: " + std::to_string(vehicleOrderOnRoad) + ")");
			}

			// 每分配几辆车就换到下一条道路
			if ((i + 1) % vehiclesPerRoad == 0) {
				roadIndex++;
			}
		}

		LogManager::instance()->logMessage("Intelligent path assignment completed. Vehicles distributed across " +
			std::to_string(roadVehicleGroups.size()) + " roads with minGap spacing.");
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

	// JSON数据解析方法实现
	JsonRoadData Traffic::parseJsonRoadData(const std::string& jsonFilePath)
	{
		JsonRoadData roadData;

		
		DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(jsonFilePath.c_str(), false, "json"));
		if (pDataStream.isNull() || pDataStream->size() == 0)
		{
			LogManager::instance()->logMessage("Failed to load JSON file: " + jsonFilePath);
			return roadData;
		}

		// 读取文件内容
		size_t nSize = pDataStream->size();
		char* pData = new char[nSize + 1];
		memset(pData, 0, nSize + 1);
		pDataStream->seek(0);
		pDataStream->read(pData, nSize);

		// 解析JSON
		cJSON* pRoot = cJSON_Parse(pData);
		delete[] pData;

		if (pRoot == nullptr)
		{
			LogManager::instance()->logMessage("Failed to parse JSON file: " + jsonFilePath);
			return roadData;
		}

		// 解析nodes数组
		cJSON* pNodes = cJSON_GetObjectItem(pRoot, "nodes");
		if (pNodes && pNodes->type == cJSON_Array)
		{
			int nodeCount = cJSON_GetArraySize(pNodes);
			roadData.nodes.reserve(nodeCount);

			for (int i = 0; i < nodeCount; ++i)
			{
				cJSON* pNode = cJSON_GetArrayItem(pNodes, i);
				if (pNode)
				{
					JsonRoadNode node;

					// 读取节点ID
					cJSON* pId = cJSON_GetObjectItem(pNode, "id");
					if (pId && pId->type == cJSON_Number)
					{
						node.id = (uint16)pId->valueint;
					}

					// 读取节点名称
					cJSON* pName = cJSON_GetObjectItem(pNode, "name");
					if (pName && pName->type == cJSON_String)
					{
						node.name = pName->valuestring;
					}

					// 读取几何位置
					cJSON* pGeometry = cJSON_GetObjectItem(pNode, "geometry");
					if (pGeometry && pGeometry->type == cJSON_Array && cJSON_GetArraySize(pGeometry) >= 3)
					{
						cJSON* pX = cJSON_GetArrayItem(pGeometry, 0);
						cJSON* pY = cJSON_GetArrayItem(pGeometry, 1);
						cJSON* pZ = cJSON_GetArrayItem(pGeometry, 2);

						if (pX && pX->type == cJSON_Number) node.geometry.x = (float)pX->valuedouble;
						if (pY && pY->type == cJSON_Number) node.geometry.y = (float)pY->valuedouble;
						if (pZ && pZ->type == cJSON_Number) node.geometry.z = (float)pZ->valuedouble;
					}

					roadData.nodes.push_back(node);
					m_nodeMap[node.id] = node; // 建立ID到节点的映射
				}
			}
		}

		
		// 解析links数组
		cJSON* pLinks = cJSON_GetObjectItem(pRoot, "links");
		if (pLinks)
		{
			
			
		
		}
		
		cJSON_Delete(pRoot);

		LogManager::instance()->logMessage("Successfully parsed JSON road data: " +
			std::to_string(roadData.nodes.size()) + " nodes, " +
			std::to_string(roadData.links.size()) + " links");

		return roadData;
	}

	// 从JSON数据创建道路网络
	void Traffic::createConnectedRoadNetworkFromJson(const JsonRoadData& roadData)
	{
		// 清理现有道路
		for (Road* road : m_allRoads) {
			delete road;
		}
		m_allRoads.clear();
		m_nodeConnections.clear();

		// 为每个链接创建道路对象
		for (const auto& link : roadData.links)
		{
			// 查找源节点和目标节点
			auto sourceIt = m_nodeMap.find(link.source);
			auto targetIt = m_nodeMap.find(link.target);

			if (sourceIt != m_nodeMap.end() && targetIt != m_nodeMap.end())
			{
				// 创建道路对象
				Road* road = new Road(link, sourceIt->second, targetIt->second);
				road->setTrafficManager(this);
				m_allRoads.push_back(road);

				// 建立节点连接映射
				m_nodeConnections[link.source].push_back(road);
				m_nodeConnections[link.target].push_back(road);
			}
		}

		// 建立道路连接关系
		for (Road* road : m_allRoads)
		{
			uint16 destNodeId = road->getDestinationNodeId();
			if (destNodeId != 0)
			{
				// 查找从目标节点出发的道路
				auto it = m_nodeConnections.find(destNodeId);
				if (it != m_nodeConnections.end())
				{
					for (Road* candidateRoad : it->second)
					{
						if (candidateRoad->getSourceNodeId() == destNodeId && candidateRoad != road)
						{
							road->setNextRoad(candidateRoad);
							LogManager::instance()->logMessage("Connected JSON Road[" + std::to_string(road->getRoadId()) +
								"] -> Road[" + std::to_string(candidateRoad->getRoadId()) + "] via Node ID " + std::to_string(destNodeId));
							break;
						}
					}
				}
			}
		}

		LogManager::instance()->logMessage("Created connected road network from JSON with " +
			std::to_string(m_allRoads.size()) + " roads");
	}

	// Road类的JSON构造函数实现


	// 简化的JSON数据方法实现
	uint16 Road::getSourceNodeId() const
	{
		return mSourceNodeId;
	}

	uint16 Road::getDestinationNodeId() const
	{
		return mTargetNodeId;
	}




}
