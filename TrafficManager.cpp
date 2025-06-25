#include"TrafficManager.h"
#include <functional>
#include"EchoPlanetRoad.h"
#include"algorithm"

CalculationClass::CalculationClass()
	:m_currentValue(0)
	, m_timer(0.0f)
	, m_upateFinished(false)
	, m_syncFinished(true)
	, m_shouldStop(false)
	, mThread(nullptr)


{
	m_lastUpdateTime = std::chrono::high_resolution_clock::now();//记录启动时间
	mThread = new std::thread(std::bind(&CalculationClass::Update, this));//线程执行Update
}
CalculationClass::~CalculationClass()
{

}

void CalculationClass::Update()
{
	//pos = 
	//pos = seep * dt



}





void CalculationClass::Sync()
{

}
void CalculationClass::Stop()
{
}
void CalculationClass::AddCar(int carId, float x, float y, float z)
{
}
void CalculationClass::RemoveCar(int carId)
{
}
void CalculationClass::SetCarDestination(int carId, float x, float y, float z)
{
}
void CalculationClass::SetCarSpeed(int carId, float speed)
{
}
std::vector<PrimitiveData> CalculationClass::GetCarDataSafe() const
{
	return std::vector<PrimitiveData>();
}
size_t CalculationClass::GetCarCount() const
{
	return size_t();
}
void CalculationClass::updaeCarPositions(float deltaTime)
{
}
Car* CalculationClass::FindCarById(int carId)
{
	return nullptr;
}
int CalculationClass::GetCurrentValue() const
{
	return m_currentValue;
}

Car::Car(int id)
	: mData(nullptr), mId(id), mDestination(), mHasDestination(false)
{
	mDestination[0] = mDestination[1] = mDestination[2] = 0.0f;
}

Car::~Car()
{
}

void Car::UpdatePosition(float daltaTime)
{
	if (!mData || !mHasDestination) return;

	float toTarget[3] =
	{
		mDestination[0] - mData->pos[0],
		mDestination[1] - mData->pos[1],
		mDestination[2] - mData->pos[2]
	};

	float distance = std::sqrt(toTarget[0] * toTarget[0] + toTarget[1] * toTarget[1] + toTarget[2] * toTarget[2]);

	if (distance < 0.1f)
	{
		mHasDestination = false;
		mData->speed = 0.0f;
		return;
	}
	//标准化方向向量
	toTarget[0] /= distance;
	toTarget[1] /= distance;
	toTarget[2] /= distance;
	//更新方向

	mData->dir[0] = toTarget[0];
	mData->dir[1] = toTarget[1];
	mData->dir[2] = toTarget[2];

	//帧移动的距离
	float moveDistance = mData->speed * daltaTime;



	//移动小车

	if (moveDistance > distance)
	{
		//overmove
		mData->pos[0] = mDestination[0];
		mData->pos[1] = mDestination[1];
		mData->pos[2] = mDestination[2];
		mHasDestination = false;
		mData->speed = 0.0f;

	}
	else
	{
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
}

void Car::SetSpeed(float speed)
{
}
