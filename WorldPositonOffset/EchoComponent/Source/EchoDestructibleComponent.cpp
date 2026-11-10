#include "EchoComponentStableHeaders.h"
#include "EchoDestructibleComponent.h"
#include "EchoRigidBodyComponent.h"
#include "EchoColliderComponent.h"
#include "EchoMeshColliderComponent.h"
namespace Echo
{

	DECLAREACTORCOMTYPE_CPP(DestructibleComponent);
	EDITOR_REFLECT_BEGIN(DestructibleComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(mDestoryActorPath, "DestoryActorPath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mHealth, "Health", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)		
	EDITOR_REFLECT_END();

	DestructibleComponentInfo::DestructibleComponentInfo(const Name& type, ActorCOMMoveableType moveable) :ActorComponentInfo(type, moveable)
	{
	}


	DestructibleComponentInfo::~DestructibleComponentInfo()
	{

	}
	_DEFINEINFO_VIRFUN_BASE(DestructibleComponentInfo);


	DestructibleComponent::DestructibleComponent(ActorPtr pActor, ActorCOMMoveableType type)
		:ProtocolActorComponent(pActor, type)
	{

	}

	DestructibleComponent::~DestructibleComponent()
	{

	}

	void DestructibleComponent::init(ActorComponentInfo* info)
	{
		ProtocolActorComponent::init(info);
		if (info)
		{
			mDestoryActorPath = static_cast<DestructibleComponentInfo*>(info)->mDestoryActorPath;
			mHealth = static_cast<DestructibleComponentInfo*>(info)->mHealth;
			mInitHealth = mHealth;
			
		}
		
	}

	void DestructibleComponent::unInit()
	{
		ProtocolActorComponent::unInit();
		
	}

	void DestructibleComponent::bindNode()
	{

	}

	void DestructibleComponent::disconnectCom()
	{
		//if (mDestroyedPrefab)
		//{
		//	if (mDestroyedPrefab->getParentActor())
		//	{
		//		mDestroyedPrefab->getParentActor()->removeActor(mDestroyedPrefab);
		//	}
		//	getActor()->getActorManager()->deleteActor(mDestroyedPrefab);
		//	mDestroyedPrefab = nullptr;
		//}

		if (mConnectCollision)
		{
			MeshColliderComponentPtr coliderComponents = getActor()->GetFirstComByType<MeshColliderComponent>();
			MeshColliderUNMOVEABLEComponentPtr unMoveableComponent = getActor()->GetFirstComByType<MeshColliderUNMOVEABLEComponent>();
			if (coliderComponents)
			{
				coliderComponents->RemoveColliderCallBack(this);
				mConnectCollision = false;
			}
			else if (unMoveableComponent)
			{
				unMoveableComponent->RemoveColliderCallBack(this);
				mConnectCollision = false;
			}
		}
	}

	void DestructibleComponent::connectActor()
	{
		if ((!mDestroyedPrefab) && mDestoryActorPath != "")
		{
			mDestroyedPrefab = getActor()->getActorManager()->createActor(mDestoryActorPath, "", false);
			if (mDestroyedPrefab)
			{				
				getActor()->addActor(getActor()->getRootNodeCom(), mDestroyedPrefab);
				mDestroyedPrefab->setRuntimeVisiable(false);
			}
		}	
	}

	void DestructibleComponent::registerTickTimeBegin()
	{
		m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}

	void DestructibleComponent::beginFrame(EventID, ProtocolHandle handle)
	{
		if (!mConnectCollision)
		{
			MeshColliderComponentPtr coliderComponents = getActor()->GetFirstComByType<MeshColliderComponent>();
			MeshColliderUNMOVEABLEComponentPtr unMoveableComponent = getActor()->GetFirstComByType<MeshColliderUNMOVEABLEComponent>();
			if (coliderComponents)
			{
				coliderComponents->AddColliderCallBack(this);
				mConnectCollision = true;
			}
			else if (unMoveableComponent)
			{
				unMoveableComponent->AddColliderCallBack(this);
				mConnectCollision = true;
			}
		}
	}

	void DestructibleComponent::visibleStateChanged(bool visibleState)
	{


	}

	void DestructibleComponent::EditorPlayImp()
	{

	}

	void DestructibleComponent::EditorPauseImp(bool pause)
	{
	}

	void DestructibleComponent::EditorStopImp()
	{
		recoverDestruction();
	}

	void DestructibleComponent::changeDestoryPrefabPath(std::string path)
	{
		mDestoryActorPath = path;
		if (mDestroyedPrefab)
		{
			if (mDestroyedPrefab->getParentActor())
			{
				mDestroyedPrefab->getParentActor()->removeActor(mDestroyedPrefab);
			}
			getActor()->getActorManager()->deleteActor(mDestroyedPrefab);
			mDestroyedPrefab = nullptr;
		}
		if (mDestoryActorPath != "")
		{
			mDestroyedPrefab = getActor()->getActorManager()->createActor(mDestoryActorPath, "", false);
			if (mDestroyedPrefab)
			{
				getActor()->addActor(getActor()->getRootNodeCom(), mDestroyedPrefab);
				mDestroyedPrefab->setRuntimeVisiable(false);

			}
		}
	}

	void DestructibleComponent::setInitHealth(float health)
	{
		mInitHealth = health;
		mHealth = mInitHealth;
	}

	void DestructibleComponent::clearDestoryPrefab()
	{
		mDestoryActorPath = "";
		if (m_pActComInfo)
		{
			DestructibleComponentInfo* minfo = static_cast<DestructibleComponentInfo*>(m_pActComInfo);
			minfo->mDestoryActorPath = "";
		}
		if (!mDestroyedPrefab.expired())
		{
			mDestroyedPrefab = nullptr;
		}
	}

	void DestructibleComponent::OnCollisionEnter(const EchoCollisionParam& param)
	{
		if (mDestroyedPrefab && mEnable)
		{
			processDestructibleCollision(param);
		}
	}

	void DestructibleComponent::OnCollisionExit(const EchoCollisionParam& param)
	{

	}

	void DestructibleComponent::OnCollisionStay(const EchoCollisionParam& param)
	{
		
	}

	void DestructibleComponent::OnTriggerEnter(const EchoTriggerParam& param)
	{
	}

	void DestructibleComponent::OnTriggerExit(const EchoTriggerParam& param)
	{
	}

	void DestructibleComponent::OnTriggerStay(const EchoTriggerParam& param)
	{
	}

	void DestructibleComponent::addDestoryedListener(void* ptr, const std::function<void(uint8)>& func)
	{
		for (auto it = m_DestoryedUpdateFunc.begin(); it != m_DestoryedUpdateFunc.end(); ++it)
		{
			if (it->listener == ptr)
				return;
		}
		m_DestoryedUpdateFunc.push_back(UpdateFun(ptr, func));

	}

	void DestructibleComponent::removeDestoryedListener(void* ptr)
	{
		auto it = m_DestoryedUpdateFunc.begin();
		while (it != m_DestoryedUpdateFunc.end())
		{
			if (it->listener == ptr)
			{
				it = m_DestoryedUpdateFunc.erase(it);
				return;
			}
			else
				++it;
		}
	}

	void DestructibleComponent::addRecoverListener(void* ptr, const std::function<void(uint8)>& func)
	{
		for (auto it = m_RecoverUpdateFunc.begin(); it != m_RecoverUpdateFunc.end(); ++it)
		{
			if (it->listener == ptr)
				return;
		}
		m_RecoverUpdateFunc.push_back(UpdateFun(ptr, func));
	}

	void DestructibleComponent::removeRecoverListener(void* ptr)
	{
		auto it = m_RecoverUpdateFunc.begin();
		while (it != m_RecoverUpdateFunc.end())
		{
			if (it->listener == ptr)
			{
				it = m_RecoverUpdateFunc.erase(it);
				return;
			}
			else
				++it;
		}
	}

	void DestructibleComponent::processDestructibleCollision(const EchoCollisionParam& param)
	{
		float impactDamage = 0;
		RigidBodyComponent* otherRbody = param.rigidbody;
		RigidBodyComponentPtr rigidBodys = getActor()->GetFirstComByType<RigidBodyComponent>();

		if (!rigidBodys.expired())
		{
			if (otherRbody != nullptr)
			{
				float avgMass = (otherRbody->GetMass() + rigidBodys->GetMass()) / 2.0f;
				Vector3 relativeVelocity = otherRbody->GetVelocity() - rigidBodys->GetVelocity();
				impactDamage = param.points[0].normal.Dot(relativeVelocity) * avgMass;
			}
			else
			{
				impactDamage = param.points[0].normal.Dot(rigidBodys->GetVelocity()) * rigidBodys->GetMass();
			}
		}
		impactDamage = std::abs(impactDamage);
		if (impactDamage < 0.1f)
		{
			//设置伤害最小值
			impactDamage = 0.1f;
		}

		if (impactDamage > 0.0f)
		{
			applyDamage(impactDamage);
		}


	}

	void DestructibleComponent::applyDamage(float damage)
	{
		if (mHealth < 0)
		{
			return;
		}
		mHealth -= damage;
		if (mHealth < 0)
		{
			processDestruction();
		}

	}

	void DestructibleComponent::recoverDestruction()
	{
		mHealth = mInitHealth;
		if (mDestroyedPrefab)
		{
			for (auto& childAct : mDestroyedPrefab->getChildActors())
			{
				if (childAct.second.expired())
				{
					continue;
				}
				childAct.second->setPosition(DBVector3(0, 0, 0));
				childAct.second->setRotation(Echo::Quaternion::IDENTITY);
			}
			mDestroyedPrefab->setWorldRotation(getActor()->getWorldRotation());
			mDestroyedPrefab->setWorldPosition(getActor()->getWorldPosition());
			mDestroyedPrefab->setWorldScale(getActor()->getWorldScale());
			mDestroyedPrefab->setRuntimeVisiable(false);
		}
		
		if (!m_RecoverUpdateFunc.empty())
		{
			for (auto it = m_RecoverUpdateFunc.begin(); it != m_RecoverUpdateFunc.end(); ++it)
			{
				(it->function)(1);
			}
		}		
	}


	void DestructibleComponent::setDestoryedActor(ActorPtr dActor)
	{
		mDestroyedPrefab = dActor;
	}

	void DestructibleComponent::processDestruction()
	{
		if (mDestroyedPrefab.expired())
		{
			return;
		}
		mHealth = -1.0f;
		if (!mDestroyedPrefab->getVisibleState())
		{
			mDestroyedPrefab->setVisiable(true);
		}
		mDestroyedPrefab->setRuntimeVisiable(true);

		RigidBodyComponentPtr rigidBodys = getActor()->GetFirstComByType<RigidBodyComponent>();	
		for (auto& childAct : mDestroyedPrefab->getChildActors())
		{
			if (childAct.second.expired())
			{
				continue;
			}
			RigidBodyComponentPtr childRigidBodys = childAct.second->GetFirstComByType<RigidBodyComponent>();
			if ( (!rigidBodys.expired()) && (!childRigidBodys.expired()) )
			{
				if (!rigidBodys->GetIsKinematic())
				{
					childRigidBodys->SetAngularVelocity(rigidBodys->GetAngularVelocity());
					childRigidBodys->SetVelocity(rigidBodys->GetVelocity());
				}
			}
			childAct.second->setPosition(DBVector3(0, 0, 0));
			childAct.second->setRotation(Echo::Quaternion::IDENTITY);
		}
		mDestroyedPrefab->setWorldRotation(getActor()->getWorldRotation());
		mDestroyedPrefab->setWorldPosition(getActor()->getWorldPosition());
		mDestroyedPrefab->setWorldScale(getActor()->getWorldScale());


		if (!m_DestoryedUpdateFunc.empty())
		{
			for (auto it = m_DestoryedUpdateFunc.begin(); it != m_DestoryedUpdateFunc.end(); ++it)
			{
				(it->function)(1);
			}
		}

	}

	

	DestructibleComponentFactory::DestructibleComponentFactory()
	{
		mType = Echo::Name("DestructibleComponent");

	}

	DestructibleComponentFactory::~DestructibleComponentFactory()
	{


	}


}

