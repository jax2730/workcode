#include "CarFollowingModel.h"
#include <algorithm>
#include <cmath>

namespace Echo
{
	//=============================================================================
	// IDM Model Implementation
	//=============================================================================

	IDMModel::IDMModel(const Parameters& params)
		: m_params(params)
		, m_gen(m_rd())
		, m_noise(-0.5f, 0.5f)
	{
	}

	float IDMModel::calculateAcceleration(float gap, float speed, float leadSpeed, float leadAcceleration)
	{
		// 添加随机噪声（仅在间距足够大时）
		float noiseAccel = (gap < m_params.s0) ? 0.0f :
			std::sqrt(m_params.noiseLevel / 0.033f) * m_noise(m_gen); // 假设 dt=0.033s (30fps)



		return noiseAccel + calculateAccelerationDeterministic(gap, speed, leadSpeed);
	}    float IDMModel::calculateAccelerationDeterministic(float gap, float speed, float leadSpeed)
	{
		// 确定有效的期望速度
		float v0eff = m_params.v0 * m_driverFactor;
		v0eff = std::min({ v0eff, m_speedLimit });
		float aeff = m_params.a * m_driverFactor;

		if (v0eff < 0.00001f) return 0.0f;

		// 计算理想的自由流间距（当前速度下的期望间距）
		float idealGap = m_params.s0 + speed * m_params.T;

		// 如果间距非常大（超过理想间距的3倍），使用自由流模式
		if (gap > 3.0f * idealGap && gap > 30.0f) {
			// 完全自由流行驶，不受前车影响
			float result =  (speed < v0eff) ?
				aeff * (1.0f - std::pow(speed / v0eff, 4.0f)) :
				aeff * (1.0f - speed / v0eff);

			return result;
		}

		// 自由流加速度
		float accFree = (speed < v0eff) ?
			aeff * (1.0f - std::pow(speed / v0eff, 4.0f)) :
			aeff * (1.0f - speed / v0eff);

		// 期望间距
		float speedDiff = speed - leadSpeed;
		float sstar = m_params.s0 + std::max(0.0f,
			speed * m_params.T + 0.5f * speed * speedDiff / std::sqrt(aeff * m_params.b));

		// 交互加速度 - 只有在真正需要跟车时才产生
		float accInt = -aeff * std::pow(sstar / std::max(gap, m_params.s0), 2.0f);

		// 如果间距较大，减少交互项的影响
		if (gap > 2.0f * idealGap) {
			float reductionFactor = std::min(1.0f, (3.0f * idealGap - gap) / idealGap);
			accInt *= std::max(0.0f, reductionFactor);
		}
		float result = accFree + accInt;
		// 返回总加速度，限制在最大减速度范围内
		return std::max(-m_params.bmax, result);
	}

	std::unique_ptr<ICarFollowingModel> IDMModel::clone() const
	{
		auto cloned = std::make_unique<IDMModel>(m_params);
		cloned->setDriverVariation(m_driverFactor);
		cloned->setSpeedLimit(m_speedLimit);
		return cloned;
	}

	void IDMModel::setDriverVariation(float driverFactor)
	{
		m_driverFactor = driverFactor;
	}

	void IDMModel::setSpeedLimit(float speedLimit)
	{
		m_speedLimit = speedLimit;
	}

	void IDMModel::setParameters(const Parameters& params)
	{
		m_params = params;
	}

	//=============================================================================
	// ACC Model Implementation
	//=============================================================================

	ACCModel::ACCModel(const Parameters& params)
		: m_params(params)
		, m_gen(m_rd())
		, m_noise(-0.5f, 0.5f)
	{
	}

	float ACCModel::calculateAcceleration(float gap, float speed, float leadSpeed, float leadAcceleration)
	{
		// 添加随机噪声
		float noiseAccel = (gap < m_params.s0) ? 0.0f :
			std::sqrt(m_params.noiseLevel / 0.033f) * m_noise(m_gen);

		// 特殊处理小间距情况
		if (gap < m_params.s0) {
			return std::max(-m_params.bmax,
				-(m_params.b + (m_params.bmax - m_params.b) * (m_params.s0 - gap) / m_params.s0));
		}

		// 确定有效参数
		float v0eff = m_params.v0 * m_driverFactor;
		v0eff = std::min({ v0eff, m_speedLimit });
		float aeff = m_params.a * m_driverFactor;

		if (v0eff < 0.00001f) return 0.0f;

		// IDM部分
		float accIDM = calculateIDMAcceleration(gap, speed, leadSpeed);

		// CAH部分
		float accCAH = calculateCAHAcceleration(gap, speed, leadSpeed, leadAcceleration);

		// 混合策略
		float accMix;
		if (accIDM > accCAH) {
			accMix = accIDM;
		}
		else {
			accMix = accCAH + m_params.b * myTanh((accIDM - accCAH) / m_params.b);
		}

		// 最终ACC加速度
		float accACC = m_params.cool * accMix + (1.0f - m_params.cool) * accIDM;

		return std::max(-m_params.bmax, accACC) + noiseAccel;
	}    float ACCModel::calculateIDMAcceleration(float gap, float speed, float leadSpeed)
	{
		float v0eff = m_params.v0 * m_driverFactor;
		v0eff = std::min({ v0eff, m_speedLimit });
		float aeff = m_params.a * m_driverFactor;

		// 计算理想的自由流间距
		float idealGap = m_params.s0 + speed * m_params.T;

		// 如果间距非常大，使用自由流模式
		if (gap > 3.0f * idealGap && gap > 30.0f) {
			return aeff * (1.0f - std::pow(speed / v0eff, 4.0f));
		}

		// 自由流加速度
		float accFree = aeff * (1.0f - std::pow(speed / v0eff, 4.0f));

		// 期望间距
		float speedDiff = speed - leadSpeed;
		float sstar = m_params.s0 + std::max(0.0f,
			speed * m_params.T + 0.5f * speed * speedDiff / std::sqrt(aeff * m_params.b));

		float accInt = -aeff * std::pow(sstar / std::max(gap, m_params.s0), 2.0f);

		// 如果间距较大，减少交互项的影响
		if (gap > 2.0f * idealGap) {
			float reductionFactor = std::min(1.0f, (3.0f * idealGap - gap) / idealGap);
			accInt *= std::max(0.0f, reductionFactor);
		}

		// IDM+变体
		return std::min(-m_params.bmax, aeff + accInt);
	}

	float ACCModel::calculateCAHAcceleration(float gap, float speed, float leadSpeed, float leadAcceleration)
	{
		float aeff = m_params.a * m_driverFactor;
		float speedDiff = speed - leadSpeed;

		float accCAH;
		if (leadSpeed * speedDiff < -2.0f * gap * leadAcceleration) {
			accCAH = speed * speed * leadAcceleration / (leadSpeed * leadSpeed - 2.0f * gap * leadAcceleration);
		}
		else {
			accCAH = leadAcceleration - std::pow(speedDiff, 2.0f) / (2.0f * std::max(gap, 0.01f)) *
				((speed > leadSpeed) ? 1.0f : 0.0f);
		}

		return std::min(accCAH, aeff);
	}

	float ACCModel::myTanh(float x) const
	{
		if (x > 50.0f) return 1.0f;
		if (x < -50.0f) return -1.0f;
		return (std::exp(2.0f * x) - 1.0f) / (std::exp(2.0f * x) + 1.0f);
	}

	std::unique_ptr<ICarFollowingModel> ACCModel::clone() const
	{
		auto cloned = std::make_unique<ACCModel>(m_params);
		cloned->setDriverVariation(m_driverFactor);
		cloned->setSpeedLimit(m_speedLimit);
		return cloned;
	}

	void ACCModel::setDriverVariation(float driverFactor)
	{
		m_driverFactor = driverFactor;
	}

	void ACCModel::setSpeedLimit(float speedLimit)
	{
		m_speedLimit = speedLimit;
	}

	void ACCModel::setParameters(const Parameters& params)
	{
		m_params = params;
	}

	//=============================================================================
	// Factory Implementation
	//=============================================================================

	std::unique_ptr<ICarFollowingModel> CarFollowingModelFactory::createModel(ModelType type)
	{
		switch (type) {
		case ModelType::IDM:
			return std::make_unique<IDMModel>();
		case ModelType::ACC:
			return std::make_unique<ACCModel>();
		default:
			return std::make_unique<IDMModel>();
		}
	}

	std::unique_ptr<ICarFollowingModel> CarFollowingModelFactory::createIDM(const IDMModel::Parameters& params)
	{
		return std::make_unique<IDMModel>(params);
	}

	std::unique_ptr<ICarFollowingModel> CarFollowingModelFactory::createACC(const ACCModel::Parameters& params)
	{
		return std::make_unique<ACCModel>(params);
	}
}
