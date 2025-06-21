// 交通计算模块 - 基于TrafficManager的完整实现
// 专门提供给AsyncApp等外部模块调用的计算逻辑
#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

// 数据结构 - 与TrafficManager完全一致
class PrimitiveData
{
public:
    float pos[3];    // 位置信息 x, y, z
    float dir[3];    // 方向信息 x, y, z
    float speed;     // 速度
    int carId;       // 小车ID
    
    PrimitiveData() : speed(0.0f), carId(-1) {
        pos[0] = pos[1] = pos[2] = 0.0f;
        dir[0] = dir[1] = dir[2] = 0.0f;
    }
};

// 车辆类 - 与TrafficManager接口兼容
class Car {
public:
    Car(int id = -1);
    ~Car();
    
    void SetData(PrimitiveData* data) { mData = data; }
    PrimitiveData* GetData() const { return mData; }
    
    void UpdatePosition(float deltaTime);
    void SetDestination(float x, float y, float z);
    void SetSpeed(float speed);
    
    int GetId() const { return mId; }

private:
    PrimitiveData* mData;
    int mId;
    float mDestination[3];  // 目标位置
    bool mHasDestination;   // 是否有目标点
};

// 计算类 - 完全兼容TrafficManager的CalculationClass接口
class CalculationClass
{
public:
    CalculationClass();
    ~CalculationClass();

    // 核心线程接口
    void Update();        // 更新线程函数
    void Sync();          // 同步函数，在主线程调用
    void Stop();          // 停止计算线程
    
    int GetCurrentValue() const;
    float GetTimer() const { return m_timer; }
    
    // 车辆管理接口 - 与AsyncApp调用的接口完全匹配
    void AddCar(int carId, float x, float y, float z);
    void RemoveCar(int carId);
    void SetCarDestination(int carId, float x, float y, float z);
    void SetCarSpeed(int carId, float speed);
    
    // 数据获取接口 - 线程安全
    std::vector<PrimitiveData> GetCarDataSafe() const;
    size_t GetCarCount() const;

private:
    float m_timer;
    int m_currentValue;
    
    // 线程同步相关
    std::mutex m_dataMutex;              // 数据保护锁
    std::mutex m_syncMutex;              // 同步锁
    std::condition_variable m_updateCV;   // Update线程条件变量
    std::condition_variable m_syncCV;     // Sync线程条件变量
    
    std::atomic<bool> m_updateFinished;  // Update是否完成标志
    std::atomic<bool> m_syncFinished;    // Sync是否完成标志
    std::atomic<bool> m_shouldStop;      // 停止标志
    
    // 计算相关
    std::chrono::high_resolution_clock::time_point m_lastUpdateTime;
    static constexpr float TARGET_FPS = 60.0f;
    static constexpr float FRAME_TIME = 1.0f / TARGET_FPS;

    std::thread* mThread;
    std::vector<Car> mCars;
    std::vector<PrimitiveData> mDatas;
      // 双缓冲数据
    std::vector<PrimitiveData> m_currentFrameData;  // 当前帧数据
    std::vector<PrimitiveData> m_nextFrameData;     // 下一帧数据
    
    void UpdateCarPositions(float deltaTime);     // 更新所有小车位置
    Car* FindCarById(int carId);                  // 根据ID查找小车
};