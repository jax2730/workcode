#ifndef __EchoDestructibleComponent_H__
#define __EchoDestructibleComponent_H__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoProtocolActorComponent.h"
#include "Actor/EchoActor.h"
#include "EchoPhysicsManager.h"
namespace Echo
{
	class _EchoComponentExport DestructibleComponent : public ProtocolActorComponent, EchoColliderCallBack
	{
		DECLAREACTORCOMTYPE_H(DestructibleComponent)
	public:
		REGISTER_LUA_CLASS_KEY;
		//------------------------COMPONENT BASE VIRTUAL FUNCTION BEGIN-----------------------------------------
		virtual void						init(ActorComponentInfo* info) override;
		virtual void						unInit() override;
		virtual void						bindNode() override;
		virtual void						disconnectCom() override;
		virtual void						connectActor() override;
		virtual void						registerTickTimeBegin() override;
		virtual void						beginFrame(EventID, ProtocolHandle handle) override;

		virtual void						visibleStateChanged(bool visibleState) override;
		virtual void						EditorPlayImp() override;
		virtual void						EditorPauseImp(bool pause) override;
		virtual void						EditorStopImp() override;

		void								changeDestoryPrefabPath(std::string path);
		float								getInitHealth() { return mInitHealth; }
		void								setInitHealth(float health);
		void								clearDestoryPrefab();

		

		virtual void OnCollisionEnter(const EchoCollisionParam& param) override;
		virtual void OnCollisionExit(const EchoCollisionParam& param) override;
		virtual void OnCollisionStay(const EchoCollisionParam& param) override;


		virtual void OnTriggerEnter(const EchoTriggerParam& param) override;
		virtual void OnTriggerExit(const EchoTriggerParam& param) override;
		virtual void OnTriggerStay(const EchoTriggerParam& param) override;


	public:
		enum class DestructionLogic
		{
			USE_CLIENT,
			USE_ENGINE,
		};
		//破坏回调
		void        addDestoryedListener(void* ptr, const std::function<void(uint8)>& func);
		void        removeDestoryedListener(void* ptr);

		//恢复回调
		void        addRecoverListener(void* ptr, const std::function<void(uint8)>& func);
		void        removeRecoverListener(void* ptr);

		//调用这个函数会触发破坏回调
		void        processDestruction();
		//调用这个函数会触发恢复回调
		void        recoverDestruction();


		void        setDestoryedActor(ActorPtr dActor);
		ActorPtr	getDestoryedActor() { return mDestroyedPrefab; }

		void        setHealth(float health) { mHealth = health; }
		float       getHealth() { return mHealth; }
		void        setEnable(bool enable) { mEnable = enable; }
		bool        getEnable() { return mEnable; }
	protected:
		struct UpdateFun
		{
			UpdateFun(void* l, std::function<void(uint8)> f) :listener(l), function(f)
			{}
			void* listener = NULL;
			std::function<void(uint8)> function = NULL;
		};
		std::vector<UpdateFun> m_DestoryedUpdateFunc;
		std::vector<UpdateFun> m_RecoverUpdateFunc;
	protected:
		void applyDamage(float damage);
		void processDestructibleCollision(const EchoCollisionParam& param);

	public:
		Echo::Name mDestoryActorPath;
	private:
		ActorPtr mDestroyedPrefab;	
		float mHealth = 0.0f;
		float mInitHealth = 0.0f;
		bool  mConnectCollision = false;
		bool  mEnable = true;
	};

	class _EchoComponentExport  DestructibleComponentInfo :public ActorComponentInfo
	{
		friend class DestructibleComponent;
	public:
		DestructibleComponentInfo() {}
		DestructibleComponentInfo(const Name& type, ActorCOMMoveableType moveable = ALL_MOVEABLE);
		~DestructibleComponentInfo();
		_DECLCOMINFO_VIRFUN;
	public:
		EDITOR_REFLECT
		std::string   mDestoryActorPath;
		float mHealth = 0.0f;


	};

	class _EchoComponentExport DestructibleComponentFactory :public ActorComFactory
	{
		static_assert(std::is_same<decltype(ALL_MOVEABLE), Echo::ActorCOMMoveableType>::value, "MOVEABLE must be ActorCOMMoveableType type!");
	public:
		DestructibleComponentFactory();
		~DestructibleComponentFactory();
		virtual Echo::EchoSharedPtr<Echo::ActorComponent>		create(const Echo::ActorPtr& pActor)	override { return Echo::EchoSharedPtr<Echo::ActorComponent>(new DestructibleComponent(pActor, ALL_MOVEABLE), Echo::ActorComponentPtrDeleter); }
		virtual Echo::ActorComponentInfo* createComponentInfo()	override { return new DestructibleComponentInfo(mType, ALL_MOVEABLE); }
	};

}

#endif

