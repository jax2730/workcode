#include "traffic.h"
#include <cmath>
#include <algorithm>
#include <functional>

Car::Car(int id) 
    : mData(nullptr), mId(id), mHasDestination(false)
{
    mDestination[0] = mDestination[1] = mDestination[2] = 0.0f;
}

Car::~Car()
{
}

void Car::UpdatePosition(float deltaTime)
{
    if (!mData || !mHasDestination) return;
    
    float toTarget[3] = {
        mDestination[0] - mData->pos[0],
        mDestination[1] - mData->pos[1],
        mDestination[2] - mData->pos[2]
    };
    
    float distance = std::sqrt(toTarget[0] * toTarget[0] + 
                              toTarget[1] * toTarget[1] + 
                              toTarget[2] * toTarget[2]);
    
    if (distance < 0.1f) {
        mHasDestination = false;
        mData->speed = 0.0f;
        return;
    }
    
    toTarget[0] /= distance;
    toTarget[1] /= distance;
    toTarget[2] /= distance;
    
    mData->dir[0] = toTarget[0];
    mData->dir[1] = toTarget[1];
    mData->dir[2] = toTarget[2];
    
    float moveDistance = mData->speed * deltaTime;
    if (moveDistance > distance) {
        mData->pos[0] = mDestination[0];
        mData->pos[1] = mDestination[1];
        mData->pos[2] = mDestination[2];
        mHasDestination = false;
        mData->speed = 0.0f;
    } else {
        mData->pos[0] += toTarget[0] * moveDistance;
        mData->pos[1] += toTarget[1] * moveDistance;
        mData->pos[2] += toTarget[2] * moveDistance;
    }
}

void Car::SetDestination(float x, float y, float z)
{
    mDestination[0] = x;
    mDestination[1] = y;
    mDestination[2] = z;
    mHasDestination = true;
}

void Car::SetSpeed(float speed)
{
    if (mData) {
        mData->speed = std::max(0.0f, speed);
    }
}

CalculationClass::CalculationClass()
    : m_timer(0.0f)                    // 1. float m_timer
    , m_currentValue(0)                // 2. int m_currentValue
    , m_updateFinished(false)          // 3. std::atomic<bool> m_updateFinished
    , m_syncFinished(true)             // 4. std::atomic<bool> m_syncFinished  
    , m_shouldStop(false)              // 5. std::atomic<bool> m_shouldStop
    , mThread(nullptr)                 // 6. std::thread* mThread
{
    m_lastUpdateTime = std::chrono::high_resolution_clock::now();
    mThread = new std::thread(std::bind(&CalculationClass::Update, this));
}

CalculationClass::~CalculationClass()
{
    Stop();
    if (mThread && mThread->joinable()) {
        mThread->join();
    }
    delete mThread;
    mThread = nullptr;
}

void CalculationClass::Update()
{
    while (!m_shouldStop) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - m_lastUpdateTime).count();
        m_lastUpdateTime = currentTime;
        
        // 等待Sync完成
        {
            std::unique_lock<std::mutex> lock(m_syncMutex);
            m_syncCV.wait(lock, [this] { return m_syncFinished.load() || m_shouldStop.load(); });
        }
        
        if (m_shouldStop) break;
        
        // 开始新的计算周期
        m_syncFinished = false;
        m_updateFinished = false;
          m_timer += deltaTime;
        m_currentValue = static_cast<int>(m_timer * 60.0f);
        
        // 更新车辆位置（需要加锁保护）
        {
            std::lock_guard<std::mutex> dataLock(m_dataMutex);
            UpdateCarPositions(deltaTime);
        }
        
        // 准备下一帧数据
        {
            std::lock_guard<std::mutex> dataLock(m_dataMutex);
            m_nextFrameData.clear();
            for (const auto& data : mDatas) {
                m_nextFrameData.push_back(data);
            }
        }
        
        // 标记更新完成
        m_updateFinished = true;
        m_updateCV.notify_one();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void CalculationClass::Sync()
{
    // 等待Update完成当前计算
    std::unique_lock<std::mutex> lock(m_syncMutex);
    m_updateCV.wait(lock, [this] { 
        return m_updateFinished.load() || m_shouldStop.load(); 
    });
    
    if (m_shouldStop) return;
    
    // 交换缓冲区数据
    {
        std::lock_guard<std::mutex> dataLock(m_dataMutex);
        m_currentFrameData = m_nextFrameData;
    }
    
    // 通知Update可以继续下一轮计算
    m_syncFinished = true;
    m_syncCV.notify_one();
}

void CalculationClass::Stop()
{
    m_shouldStop = true;
    m_syncCV.notify_all();
    m_updateCV.notify_all();
}

int CalculationClass::GetCurrentValue() const
{
    return m_currentValue;
}

float CalculationClass::GetTimer() const
{
    return m_timer;
}

void CalculationClass::AddCar(int carId, float x, float y, float z)
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    auto it = std::find_if(mCars.begin(), mCars.end(), 
        [carId](const Car& car) { return car.GetId() == carId; });
    
    if (it != mCars.end()) return;
    
    PrimitiveData data;
    data.carId = carId;
    data.pos[0] = x; data.pos[1] = y; data.pos[2] = z;
    data.speed = 0.0f;
    data.dir[0] = 1.0f; data.dir[1] = 0.0f; data.dir[2] = 0.0f;
    
    mDatas.push_back(data);
    
    Car car(carId);
    car.SetData(&mDatas.back());
    mCars.push_back(car);
}

void CalculationClass::RemoveCar(int carId)
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    // 先移除Car对象
    mCars.erase(std::remove_if(mCars.begin(), mCars.end(),
        [carId](const Car& car) { return car.GetId() == carId; }), 
        mCars.end());
    
    // 再移除对应的数据
    mDatas.erase(std::remove_if(mDatas.begin(), mDatas.end(),
        [carId](const PrimitiveData& data) { return data.carId == carId; }), 
        mDatas.end());
        
    // 重新建立Car和PrimitiveData的关联关系
    // 这里需要更安全的方式来重新关联
    for (auto& car : mCars) {
        car.SetData(nullptr); // 先清空
        for (auto& data : mDatas) {
            if (car.GetId() == data.carId) {
                car.SetData(&data);
                break;
            }
        }
    }
}

void CalculationClass::SetCarDestination(int carId, float x, float y, float z)
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    Car* car = FindCarById(carId);
    if (car) {
        car->SetDestination(x, y, z);
    }
}

void CalculationClass::SetCarSpeed(int carId, float speed)
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    Car* car = FindCarById(carId);
    if (car) {
        car->SetSpeed(speed);
    }
}

std::vector<PrimitiveData> CalculationClass::GetCarDataSafe() const
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_currentFrameData;
}

size_t CalculationClass::GetCarCount() const
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return mCars.size();
}

void CalculationClass::UpdateCarPositions(float deltaTime)
{
    // 注意：这个方法在Update线程中调用，mCars已经在调用处被锁保护
    for (auto& car : mCars) {
        car.UpdatePosition(deltaTime);
    }
}

Car* CalculationClass::FindCarById(int carId)
{
    auto it = std::find_if(mCars.begin(), mCars.end(),
        [carId](const Car& car) { return car.GetId() == carId; });
    
    return (it != mCars.end()) ? &(*it) : nullptr;
}