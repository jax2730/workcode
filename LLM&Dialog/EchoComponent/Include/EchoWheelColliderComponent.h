#ifndef __EchoWheelColliderComponent_H__
#define __EchoWheelColliderComponent_H__

#include "EchoColliderComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoComInfoDefineMacro.h"
#include "EchoPhysicsManager.h"
#include <functional>

namespace Echo
{
	//class PhysXVehicleNoDrive;
	//
	//struct EchoWheelFriction
	//{	// 车轮摩擦曲线
	//	REGISTER_LUA_CLASS_KEY
	//	float extremumSlip;
	//	float extremumValue;
	//	float asymptoteSlip;
	//	float asymptoteValue;
	//	float stiffness;
	//};
	//
	//struct WheelColliderComponentData : public ColliderComponentData
	//{
	//	float	mass = 20.f;				// 轮胎质量
	//	float	radius = 0.5f;				// 轮胎半径
	//	float	wheelDampingRate = 0.25f;	// 轮胎受到的阻尼率
	//	float	suspensionDistance = 0.3f;  // 悬挂长度
	//	float	forceAppPointDistance = 0.f;// 轮胎的受力点，默认为0 在车轮的底部
	//	//Vector3 Center = Vector3::ZERO;	// 悬挂的偏移 
	//										// 组件位置对应车轮悬挂的起始位置 
	//	float	suspSpring = 35000.f;		// 悬架弹簧强度
	//	float	suspDamper = 4500.f;		// 抑制悬架速度
	//	float	suspTargetPos = 0.5f;		// 车轮不受力时 车轮中心处于悬架的位置 [0~1]
	//	EchoWheelFriction	forwardFriction = { 0.4f,1.f,0.8f,0.5f,1.f };	// 车轮向前滚动时的车轮摩擦曲线
	//	//EchoWheelFriction	sidewaysFriction = { 0.2f,1.f,0.5f,0.75f,1.f };	// 车轮侧向滚动时的车轮摩擦曲线
	//	
	//	WheelColliderComponentData();
	//
	//};
	//
	//class _EchoComponentExport WheelColliderComponent : public ColliderComponent 
	//{
	//public:
	//	DECLAREACTORCOMTYPE_H(WheelColliderComponent)
	//	REGISTER_LUA_CLASS_KEY
	//public:
	//	//virtual void CreatePhysics()	override;
	//	//virtual void ReleasePhysics()	override;
	//	virtual void ChangeShape()							override;
	//	virtual void ChangeAttach()							override;
	//	virtual void AttachShapeEx(RigidBodyComponent* pRb)	override;
	//	virtual void DetachShapeEx()						override;
	//	virtual bool IsCanPhysics()							override;
	//	virtual bool IsPhysicsStaut();
	//	virtual void UpdateComponentInEditor()				override;
	//	virtual void ChangeLocalPose()	override;
	//	//virtual void ChangeIsTrigger()						override;
	//	virtual void ChangeFilterGroup()					override;
	//protected:
	//	EchoGeometry* GenerateGeometry(void* buffer) override;
	//	void	GenerateVBIBCB(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec,std::vector<uint32>& cbVec) override;
	//public:
	//	virtual void SetCompData(ColliderComponentData* pData) override;
	//public:	// 设置/获取轮胎参数 模拟时修改参数可能会使车辆不稳定
	//	void	setMass(float mass);
	//	float	getMass();
	//	void	setRadius(float radius);
	//	float	getRadius();
	//	void	setWheelDampingRate(float dampingRate);
	//	float	getWheelDampingRate();
	//	void	setSuspensionDistance(float distance);
	//	float   getSuspensionDistance();
	//	void    setForceAppPointDistance(float distance);
	//	float	getForceAppPointDistance();
	//	void	setSuspSpring(float spring);
	//	float	getSuspSpring();
	//	void	setSuspDamper(float damper);
	//	float	getSuspDamper();
	//	void	setSuspTargetPos(float pos);
	//	float	getSuspTargetPos();
	//	void    setForwardFriction(const EchoWheelFriction & friction);
	//	EchoWheelFriction getForwardFriction();
	//	//void	 SetSidewaysFriction(const EchoWheelFriction & friction);
	//	//EchoWheelFriction getSidewaysFriction();
	//public:
	//	float	getRealRadius();// 获得考虑了旋转和缩放之后的半径
	//	float	getRealWidth();	// 轮胎物理的宽度
	//	float   getRealSuspensionDistance();	// 获得考虑了旋转和缩放之后的悬架长度
	//	float   getRealForceAppPointDistance();	// 获得考虑了旋转和缩放之后的施力点
	//
	//public: // 获取轮胎运行时状态
	//
	//	void	getWheelCenterWorldPostion(Vector3 & postion, Quaternion & rotate);	// 轮胎位置和旋转的绝对坐标 
	//	bool	isInAir();															// 悬挂无法使车轮接触可行驶的表面 则为true
	//	Vector3 tireContactPoint();													// 轮胎击中可行驶表面的点的坐标
	//	Vector3 tireContactNormal();												// 轮胎集中可行驶表面的点的法线
	//	float	tireFrition();														// 在考虑摩擦力与滑移图,轮胎类型和表面类型的组合	之后，轮胎所经历的摩擦力
	//	float	tirePos();															// 轮胎在其悬挂所处的位置 [0.f~1.f]
	//	Vector3	tireLongitudinalDir();												// 轮胎的前进方向 
	//	Vector3 tireLateralDir();													// 轮胎的横向方向
	//	float	longitudinalSlip();													// 轮胎的纵向滑移
	//	float	lateralSlip();														// 轮胎的横向滑移
	//	//float	steerAngle();														// 轮子关于上方向矢量的转向角
	//	float	rpm();																// 转速
	//	float	rotationAngle();													// 轮胎的围绕其转动轴的转动角度
	//
	//public: // 轮胎控制 需要轮胎参与物理模拟后调用 
	//	void	setBrakeTorque	(float torque);
	//	void	setDriveTorque	(float torque);
	//	void	setSteerAngle	(float	 angle);
	//
	//	float   getBrakeTorque	();
	//	float	getDriveTorque	();
	//	float	getSteerAngle	();
	//
	//public:		
	//	friend class EchoPhysicsSystem;
	//private: // 这些数据仅轮胎参与物理模拟时有效
	//	uint32										m_wheelId;			// 轮胎在其车辆的id
	//	uint32										m_shapeId;			// 轮胎shape在刚体actor中的shapeId 
	//	PhysXVehicleNoDrive*						m_pVehicleNoDrive;  // 指向对应车辆的指针 
	//	PhysXConvexMesh*							m_pPhysXConvexMesh = nullptr;
	//};
	//
	//class _EchoComponentExport WheelColliderComponentInfo : public ColliderComponentInfo
	//{
	//public:
	//	WheelColliderComponentInfo() {}
	//	WheelColliderComponentInfo(const Name& comType, ActorCOMMoveableType MoveType);
	//	~WheelColliderComponentInfo();
	//	virtual void CopyToData(ColliderComponentData* pData) override;
	//	_DECLCOMINFO_VIRFUN
	//public:
	//
	//	_DECLCOMINFO_PARAM_BEGIN(WheelColliderComponent)
	//	_DECLCOMINFO_PARAM_SIM(float, Mass, 20.f, setMass)
	//	_DECLCOMINFO_PARAM_SIM(float, Radius, 0.5f, setRadius)
	//	_DECLCOMINFO_PARAM_SIM(float, WheelDampingRate, 0.25f, setWheelDampingRate)
	//	_DECLCOMINFO_PARAM_SIM(float, SuspensionDistance, 0.3f, setSuspensionDistance)
	//	_DECLCOMINFO_PARAM_SIM(float, ForceAppPointDistance, 0.f, setForceAppPointDistance)
	//	_DECLCOMINFO_PARAM_SIM(float, SuspSpring, 35000.f, setSuspSpring)
	//	_DECLCOMINFO_PARAM_SIM(float, SuspDamper, 4500.f, setSuspDamper)
	//	_DECLCOMINFO_PARAM_SIM(float, SuspTargetPos, 0.5f, setSuspTargetPos)
	//	_DECLCOMINFO_PARAM(EchoWheelFriction, ForwardFriction, (EchoWheelFriction{ 0.4f,1.f,0.8f,0.5f,1.f }), setForwardFriction)
	//	//_DECLCOMINFO_PARAM(EchoWheelFriction, sidewaysFriction, (EchoWheelFriction{ 0.2f,1.f,0.5f,0.75f,1.f }), SetSidewaysFriction)
	//	_DECLCOMINFO_PARAM_END()
	//};
	//DEFINECOMFACTORY(WheelColliderComponent, ALL_MOVEABLE)
	//
	//class EchoWheelColliderScript 
	//{
	//	public:
	//		REGISTER_LUA_CLASS_KEY;
	//		static WheelColliderComponent* Script_GetWheelColliderComponent(ActorPtr pActor, const std::string& name);
	//};
	//
	//Vector3 computeWheelCenterRigidBodyOffset(RigidBodyComponent* pRB, WheelColliderComponent* pWC);


}


#endif // !__EchoWheelColliderComponent_H__
