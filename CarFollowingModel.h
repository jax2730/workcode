#pragma once
#pragma once
#include <memory>
#include <vector>
#include <random>

namespace Echo
{
	// 前向声明
	class Vehicle;

	/**
	 * 跟车模型基类
	 * 提供统一的接口，支持不同的跟车模型实现
	 */
	class ICarFollowingModel
	{
	public:
		virtual ~ICarFollowingModel() = default;

		/**
		 * 计算加速度
		 * @param gap 与前车的距离 [m]
		 * @param speed 当前速度 [m/s]
		 * @param leadSpeed 前车速度 [m/s]
		 * @param leadAcceleration 前车加速度 [m/s²] (可选)
		 * @return 计算得到的加速度 [m/s²]
		 */
		virtual float calculateAcceleration(float gap, float speed, float leadSpeed, float leadAcceleration = 0.0f) = 0;

		/**
		 * 深拷贝模型
		 */
		virtual std::unique_ptr<ICarFollowingModel> clone() const = 0;

		/**
		 * 设置驾驶员变异系数
		 */
		virtual void setDriverVariation(float driverFactor) = 0;

		/**
		 * 设置速度限制
		 */
		virtual void setSpeedLimit(float speedLimit) = 0;
	};

	/**
	 * IDM (Intelligent Driver Model) 跟车模型
	 */
	class IDMModel : public ICarFollowingModel
	{
	public:
		struct Parameters
		{
			float v0 = 15.0f;      // 期望速度 [m/s]
			float T = 1.0f;        // 期望时间间隔 [s]
			float s0 = 2.0f;       // 最小间距 [m]
			float a = 2.0f;        // 最大加速度 [m/s²]
			float b = 1.5f;        // 舒适减速度 [m/s²]
			float bmax = 18.0f;    // 最大减速度 [m/s²]
			float noiseLevel = 0.05f; // 噪声水平
		};

		explicit IDMModel(const Parameters& params = Parameters{});

		float calculateAcceleration(float gap, float speed, float leadSpeed, float leadAcceleration = 0.0f) override;
		std::unique_ptr<ICarFollowingModel> clone() const override;
		void setDriverVariation(float driverFactor) override;
		void setSpeedLimit(float speedLimit) override;

		// IDM特有方法
		void setParameters(const Parameters& params);
		const Parameters& getParameters() const { return m_params; }

	private:
		float calculateAccelerationDeterministic(float gap, float speed, float leadSpeed);

		Parameters m_params;
		float m_driverFactor = 1.0f;
		float m_speedLimit = 1000.0f;
		mutable std::random_device m_rd;
		mutable std::mt19937 m_gen;
		mutable std::uniform_real_distribution<float> m_noise;
	};

	/**
	 * ACC (Adaptive Cruise Control) 跟车模型
	 */
	class ACCModel : public ICarFollowingModel
	{
	public:
		struct Parameters
		{
			float v0 = 15.0f;      // 期望速度 [m/s]
			float T = 1.0f;        // 期望时间间隔 [s]
			float s0 = 2.0f;       // 最小间距 [m]
			float a = 2.0f;        // 最大加速度 [m/s²]
			float b = 1.5f;        // 舒适减速度 [m/s²]
			float bmax = 10.0f;    // 最大减速度 [m/s²]
			float cool = 0.9f;     // 冷却因子
			float noiseLevel = 0.05f; // 噪声水平
		};

		explicit ACCModel(const Parameters& params = Parameters{});

		float calculateAcceleration(float gap, float speed, float leadSpeed, float leadAcceleration = 0.0f) override;
		std::unique_ptr<ICarFollowingModel> clone() const override;
		void setDriverVariation(float driverFactor) override;
		void setSpeedLimit(float speedLimit) override;

		// ACC特有方法
		void setParameters(const Parameters& params);
		const Parameters& getParameters() const { return m_params; }

	private:
		float calculateIDMAcceleration(float gap, float speed, float leadSpeed);
		float calculateCAHAcceleration(float gap, float speed, float leadSpeed, float leadAcceleration);
		float myTanh(float x) const;

		Parameters m_params;
		float m_driverFactor = 1.0f;
		float m_speedLimit = 1000.0f;
		mutable std::random_device m_rd;
		mutable std::mt19937 m_gen;
		mutable std::uniform_real_distribution<float> m_noise;
	};

	/**
	 * 跟车模型工厂
	 */
	class CarFollowingModelFactory
	{
	public:
		enum class ModelType
		{
			IDM,
			ACC
		};

		static std::unique_ptr<ICarFollowingModel> createModel(ModelType type);
		static std::unique_ptr<ICarFollowingModel> createIDM(const IDMModel::Parameters& params = IDMModel::Parameters{});
		static std::unique_ptr<ICarFollowingModel> createACC(const ACCModel::Parameters& params = ACCModel::Parameters{});
	};
}
