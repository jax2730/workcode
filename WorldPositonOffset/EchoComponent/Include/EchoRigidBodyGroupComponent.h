
#ifndef __EchoRigidBodyGroupComponent_H__
#define __EchoRigidBodyGroupComponent_H__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "EchoPhysicsManager.h"
#include "EchoComDefineMacro.h"

//namespace Echo
//{
//	class RigidBodyComponent;
//	class RigidBodyComponentUpdateListener;
//	class PhysXAggregate;
//	class PhysicsActor;
//	class RigidBodyComponentUpdateListener;
//}
//
//namespace Echo
//{
//	struct _EchoComponentExport RigidBodyGroupComponentInfo : public ActorComponentInfo
//	{
//	public:
//		RigidBodyGroupComponentInfo() {}
//		RigidBodyGroupComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) :ActorComponentInfo(comType, MoveType) {}
//		~RigidBodyGroupComponentInfo() {}
//		_DECLCOMINFO_VIRFUN
//	public:
//		std::vector<Name>		rigidBodys;					// 组合名
//		bool					disableCollision = true;	// 关闭碰撞
//		EDITOR_REFLECT
//		
//	};
//
//	class _EchoComponentExport RigidBodyGroupComponent: public PhysicsBaseComponent, public EchoRigidBodyCallBack
//	{
//	public:
//		DECLAREACTORCOMTYPE_H(RigidBodyGroupComponent)
//		REGISTER_LUA_CLASS_KEY;
//	public:
//		//重载于ActorComponent 
//		virtual void								init(ActorComponentInfo* pAInfo)	override;
//		virtual void								replay(ActorComponentInfo* pAInfo)  override;
//		//virtual void								copyTo(const ActorComponentPtr& pComp)override;	// 无法被复制
//		virtual void								connectCom()					override;
//		virtual void								dealComponentDelete(const ActorComponentPtr&  actCom) override;
//	public:
//		//重载于PhysicsBaseComponent
//		virtual void CreatePhysics()			override;
//		virtual void ReleasePhysics()			override;
//		virtual bool IsCanPhysics()				override;
//		virtual bool IsExistPhysics()			override;
//		virtual void UpdateComponentInEditor()	override;
//
//	public:
//		void							SynchronousPhysics();
//		
//		std::vector<ActorComponentPtr>	getAllRigidBodyComponent();		// 返回组员的指针 调用时无法通过该位置的组员名字找到component时返回nullptr
//		std::vector<Name>							getAllRigidBodyCompName();		// 返回组员的comName
//		size_t										getRigidBodyCount();			// 返回组员的数量
//		bool										isDisableCollision();			// 是否关闭组内碰撞	
//
//		bool								addRigidBody(const Name& comName);						// 接受com名,刚体存在且未加入任意刚体组时该名字才会被接受并返回true 
//		uint32								resetRigidBodys(const std::vector<Name>& comNames);		// 返回被接受的组员的数量
//		void								delRigidBody(const Name& comName);						// 
//		void								disableCollision(bool disable);							// 设置禁用组内碰撞，默认禁用
//
//		static bool							rigidBodyInGroup(ActorComponentPtr pAC);		// 查询该com名字的rigidbody是否在某一个组里 刚体不能重复入组
//		PhysXAggregate*						GetPhysics();
//	private:
//		void								_addRigidBody(ActorComponentPtr rigidbody);
//		void								_delRigidBody(ActorComponentPtr rigidbody);
//		void								_delAllRigidBody();
//		void								_resetAllRigidBody();
//	private:
//		bool								checkAndAllocAggregateSize(uint32 quantity);
//		virtual void						OnRigidBodyStateChanged(uint32 mask) override;
//	private:
//		//std::list<Name>						mRigidBodyComNames;
//		std::list<ActorComponentPtr>			mTeamMembers;
//		bool									mDisableCollision = true;
//		
//		PhysXAggregate*							mRigidBodyAggregate = nullptr;
//		//RigidBodyComponentUpdateListener*		RigidBodyCreatePhysicsListener;
//	private:
//		static std::unordered_set<ActorComponent*>	RigidBodyMap;
//		friend class EchoPhysicsSystem;
//	};
//	DEFINECOMFACTORY(RigidBodyGroupComponent, UNMOVEABLE)
//
//}

#endif // !__EchoRigidBodyGroupComponent_H__
