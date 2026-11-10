#pragma once

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoProtocolActorComponent.h"
#include "EchoEntityObjectBatch.h"
#include "Actor/EchoActor.h"

namespace Echo
{
	class _EchoComponentExport EntityBatchComponent :public ProtocolActorComponent, public NodeTransformUpdateListener
	{
		DECLAREACTORCOMTYPE_H(EntityBatchComponent)
	public:
		REGISTER_LUA_CLASS_KEY;

		//ActorComponent function begin

		virtual void						init(ActorComponentInfo* info) override;
		virtual void						unInit() override;
		virtual void						bindNode() override;
		virtual void						disconnectCom() override;
		virtual void						connectActor() override;
		virtual void						visibleStateChanged(bool visibleState) override;
		virtual void						registerTickTimeBegin() override;
		virtual void						beginFrame(EventID, ProtocolHandle handle) override;
		virtual void						EditorPlayImp() override;
		virtual void						EditorPauseImp(bool pause) override;
		virtual void						EditorStopImp() override;
		//ActorComponent function end

		virtual void						OnNodeTransformUpdate() override;

		//设置相机裁剪包围盒大小
		void								setRenderBoundingBox(const AxisAlignedBox& box);
		CameraCullMode						getCullMode();
		void								setCullMode(int mode = 0);
		//float								getRenderDistance();								// 裁切距离
		//void								setRenderDistance(float dis);
		float								getTODRenderDis();									// TOD裁切距离
		void								setTODRenderDis(float dis);

		//投射阴影
		void								setCastShadow(bool isEnable);
		bool								getCastShadow();

		//lod
		void								setLodDisLevel1(float val);
		float								getLodDisLevel1();
		void								setLodDisLevel2(float val);
		float								getLodDisLevel2();

		// val 取值为 0.0f ~ 1.0f 0为不消融 1为完全消融
		float								getDissolveStrength();	// 消融的强度	
		void								setDissolveStrength(float val);

		void								setBatchRenderDis(float dis); 
		float								getBatchRenderDis(); // 裁切距离

		bool								getBatchVisible() const { return m_bBatchVisible; }
		void								setBatchEnableDis(float dis);
		float								getBatchEnableDis(); //启用距离 在这个距离之外启用Batch
		void								setBatchEnableFactor(float factor);
		float								getBatchEnableFactor(); //启用距离系数
		void								setShadowRenderObject(EntityObject* obj, const ActorComponentPtr& pCom = nullptr);
		EntityObject*						getShadowRenderObject();
		void								addComToBatch(ActorComponent* pCom);
		void								removeComFromBatch(ActorComponent* pCom);
#ifdef ECHO_EDITOR
		//合并Actor下所有的EntityComponent
		void								batchEntitys();
#endif

		void								useEntityBatch(bool use);
		void								enableEntityBatch(bool enable);
		bool								isEnable()const;

		void								setEntityBatchFactor(float factor);
		float								getEntityBatchFactor()const { return m_fEntityBatchFactor; }


	public:
		float								m_fEntityBatchDist = 300.0f; //渲染距离
		float								m_fEntityBatchFactor = 0.0f; 
		float								m_fEnableDis = 300.0f; //启用距离 在这个距离之外启用Batch
		float								m_fEnableDisFactor = 0.0f;
	private:
		EntityObjectBatch*					m_pEntityObjectBatch = nullptr;
		//float								mDissolveStrength = 0.0f;
		bool								m_bEnable = true;
		inline static bool					m_bGlobalEnable = true;
		bool								m_bBatchVisible = true;
		bool								m_bUpdateFinish = false;
		//bool								mbBatchFinish = false;
		//WorkQueue::RequestID				mRequestID = 0;
	};

	class _EchoComponentExport EntityBatchComponentInfo :public ActorComponentInfo
	{
		friend class EntityBatchComponent;
	public:
		EntityBatchComponentInfo() {}
		EntityBatchComponentInfo(const Name& type, ActorCOMMoveableType moveable = ALL_MOVEABLE);
		~EntityBatchComponentInfo();

		_DECLCOMINFO_VIRFUN;
	public:

		float				GetTodCullDistance() const { return m_TodCullDistance; }
		void				SetTodCullDistance(float val) { m_TodCullDistance = val; }

		float				GetCullDistance() const { return m_CullDistance; }
		void				SetCullDistance(float val) { m_CullDistance = val; }

		int					GetCullMode() const { return m_CullMode; }
		void				SetCullMode(int val) { m_CullMode = val; }
		
		void                setDist(float size) { m_EntityBatchDist = size; }
		float               getDist() const { return m_EntityBatchDist; }
		void				setEntityBatchFactor(float factor) { m_EntityBatchFactor = factor; }
		float				getEntityBatchFactor() const  {return m_EntityBatchFactor;}
		void				setEnableDis(float dis) { m_EnableDis = dis; }
		float				getEnableDis() const { return m_EnableDis; }
		void				setEnableDisFactor(float factor) { m_EnableDisFactor = factor; }
		float				getEnableDisFactor() const { return m_EnableDisFactor; }

	public:

		int							m_CullMode = 1;
		float						m_CullDistance = 1000.f;
		float						m_TodCullDistance = 1000.f;

		float						m_EntityBatchDist = 300.0f; // Change
		float						m_EntityBatchFactor = 0.0f;
		float						m_EnableDis = 300.0f;
		float						m_EnableDisFactor = 0.0f;
		float						m_CullFactor = 0.0f;
		float						m_DissolveStrength = 0.0f;
		bool						m_CastShadow = false;
		float						m_LodDis_Level1 = 100.0f;
		float						m_LodDis_Level2 = 200.0f;
		AxisAlignedBox				m_BoundBox = AxisAlignedBox::BOX_NULL;

		EDITOR_REFLECT
	};

	class _EchoComponentExport EntityBatchComponentFactory :public ActorComFactory
	{
		static_assert(std::is_same<decltype(ALL_MOVEABLE), Echo::ActorCOMMoveableType>::value, "MOVEABLE must be ActorCOMMoveableType type!");
	public:
		EntityBatchComponentFactory();
		~EntityBatchComponentFactory();
		static Echo::ProtocolResult ProCom_EnableBatchEntityObj(void* enable, Echo::ProtocolHandle handle);
		virtual Echo::EchoSharedPtr<Echo::ActorComponent>		create(const Echo::ActorPtr& pActor)	override { return Echo::EchoSharedPtr<Echo::ActorComponent>(new EntityBatchComponent(pActor, ALL_MOVEABLE), Echo::ActorComponentPtrDeleter); }
		virtual Echo::ActorComponentInfo* createComponentInfo()	override { return new EntityBatchComponentInfo(mType, ALL_MOVEABLE); }
	};
}
