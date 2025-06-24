#pragma once
#include <thread>
#include"vector"
#include"mutex"
#include"atomic"
class PrimitiveData
{
	//route
public:
	float pos[3];
	float dir[3];
	float speed;
	int carId;
	PrimitiveData() : speed(0.0f), carId(-1) {
		pos[0] = pos[1] = pos[2] = 0.0f;
		dir[0] = dir[1] = dir[2] = 0.0f;
	}

};
struct PathPoint //路径？
{


};
class Car {
private:
	PrimitiveData* mData;
	int mId;
	float mDestination[3];
	bool mHasDestination;   //  route

public:
	Car(int id = -1);

	~Car();
	void SetData(PrimitiveData* data) { mData = data; }
	PrimitiveData* GetData() const { return mData; }

	int GetId() const { return mId; } //

	void UpdatePosition(float daltaTime); //
	void SetDestination(float x, float y, float z); //route
	void SetSpeed(float speed);

};

class CalculationClass
{


public:

	CalculationClass();


	~CalculationClass();


	void Update();
	void Sync();
	void Stop();


	int GetCurrentValue() const; //当前计算值
	float GetTimer() const { return m_timer; } //运行时间


	void AddCar(int carId, float x, float y, float z);
	void RemoveCar(int carId);
	void SetCarDestination(int carId, float x, float y, float z);
	void SetCarSpeed(int carId, float speed);

	std::vector<PrimitiveData> GetCarDataSafe() const;
	size_t GetCarCount() const;



private:



	int m_currentValue;
	float m_timer;
	//线程同步；互斥锁；
	std::mutex m_dateMutex;
	std::mutex m_suncMutex;
	std::condition_variable m_updateCV; //线程条件变量
	std::condition_variable m_syncCV; //sync线程条件变量

	//检测是否完成
	std::atomic<bool> m_upateFinished;
	std::atomic<bool> m_syncFinished;
	std::atomic<bool> m_shouldStop;

	//culculate
	std::chrono::high_resolution_clock::time_point m_lastUpdateTime;
	static constexpr float TAGET_FPS = 60.0f;
	static constexpr float FRAME_TIME = 1.0f / TAGET_FPS;

	std::thread* mThread;
	std::vector<Car> mCars;
	std::vector<PrimitiveData> mDatas;

	//数据缓冲
	std::vector<PrimitiveData> m_CurrentFrameData;
	std::vector<PrimitiveData> m_NextFrameDate;

	void updaeCarPositions(float deltaTime);
	Car* FindCarById(int carId);



};
