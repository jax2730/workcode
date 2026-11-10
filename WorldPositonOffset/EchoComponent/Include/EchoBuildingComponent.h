#ifndef _EchoBuildingComponent_H__
#define _EchoBuildingComponent_H__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"
#include "Actor/EchoProtocolActorComponent.h"
#include "EchoActorEntityComponent.h"
#include "Actor/EchoActorBuilding.h"

namespace Echo
{
	class DistantBuildingComponent;
	class _EchoComponentExport BuildingComponent : public ProtocolActorComponent, public ActorLoadListener
	{
		DECLAREACTORCOMTYPE_H(BuildingComponent)
	public:
		REGISTER_LUA_CLASS_KEY;
		virtual void						init(ActorComponentInfo* info) override;
		virtual void						unInit() override;
		virtual void						connectActor() override;

		virtual void						connectCom() override;
		virtual void						disconnectCom() override;

		virtual void						NotifiChildImp(int changeMask) override;
		virtual void						visibleStateChanged(bool visibleState) override;
		virtual void						registerTickTimeBegin() override;
		virtual void						beginFrame(EventID, ProtocolHandle handle) override;

		virtual bool						OnActorCreateFinish() override;
		virtual AxisAlignedBox				getLoadBoundingBox() override;

		void								addEntityCom(const EchoWeakPtr<EntityComponent>& pCom);
		void								removeEntityCom(const EchoWeakPtr<EntityComponent>& pCom, bool forceRemove = false);
		void								updateEntityComTransform(const EchoWeakPtr<EntityComponent>& pCom);
		bool								hasEntityComponent(const EchoWeakPtr<EntityComponent>& pCom);

		void								useBuildingRendering(bool use);
		bool								isUsingBuildingRendering()const { return m_bUsingBuildRender; }

		void								setBuildingSimpleMesh(const String& meshName);
		const String&						getBuildingSimpleMeshName();

		void								setBuildingLodDistanceLevel1(float val);
		float								getBuildingLodDistanceLevel1();
		void								setBuildingLodDistanceLevel2(float val);
		float								getBuildingLodDistanceLevel2();

		void								setBuildingRenderDistance(float distance);
		float								getBuildingRenderDistance() const { return m_fBuildingRenderDistance; }

		void								setBuildingUpdateLoadByBlock(bool val);
		bool								getBuildingUpdateLoadByBlock() const;

		float								getFactorCullLength();

		void								setBuildingLodFactorLevel1(float val);
		void								setBuildingLodFactorLevel2(float val);
		void								setBuildingRenderFactor(float val);

		float								getBuildLodFactorLevel1() { return m_fBuildingFactor[0]; }
		float								getBuildLodFactorLevel2() { return m_fBuildingFactor[1]; }
		float								getBuildRenderFactor() { return m_fBuildingFactor[2]; }
		
		void								setBuildingFactor(int id, float val);

		void								setDistantBuildingComName(const String& com);

		int									getBuildingCullMode() { return m_bBuildingCullMode; }
		void								setBuildingCullMode(int mode);
		bool								isBuildingControlling() const { return m_bBuildingControlling; }
		bool								getIsUpdateByBlock();

		void								updateCombinedName() override;
	protected:
#ifdef ECHO_EDITOR
		void								batchEntities();
#endif
		void								removeAllEntityCom();
		float								resetLastLodFactorDistance(int i);
		void								updateEntityComTransformImpl(const EchoWeakPtr<EntityComponent>& pCom);
		void								findDistantBuildingCom();
	private:
		String									m_nMeshName = "";
		bool									m_bUsingBuildRender = false;
		float									m_fBuildingLodDistanceLevel1 = 50.0f;
		float									m_fBuildingLodDistanceLevel2 = 100.0f;
		float									m_fBuildingRenderDistance = 300.0f;

		float									m_fBuildingFactor[3] = { 0.f,0.f,0.f };
		int										m_bBuildingCullMode = 1;

		// 辅助计算裁剪系数中间值
		float									m_fLastBuildingLodFactor[3] = { 0.f,0.f,0.f };
		float									m_fLastBuildingLodLength[3] = { 0.f,0.f,0.f };
		int										m_FrameTick = 0;

		bool									m_bBuildingControlling = false;
		ActorBuilding*					m_pActorBuilding = nullptr;
		int										m_nTransformCnt = 0;
		bool									m_bNeedUpdateTrans = false;

		EchoAgent<DistantBuildingComponent>		m_disBuildingCom;

		std::map<EchoWeakPtr<EntityComponent>, int> m_mapEntityComs;
	};

	class _EchoComponentExport BuildingComponentInfo :public ActorComponentInfo
	{
		friend class EntityBatchComponent;
	public:
		BuildingComponentInfo() {}
		BuildingComponentInfo(const Name& type, ActorCOMMoveableType moveable = UNMOVEABLE);
		~BuildingComponentInfo();

#ifdef ECHO_EDITOR
		virtual bool										getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif

		_DECLCOMINFO_VIRFUN;
	public:
		String				m_BuildingSimpleMeshName = "";
		String				m_DistantBuildingComName = "";
		float				m_BuildingLodDistanceLevel1 = 50.0f;
		float				m_BuildingLodDistanceLevel2 = 100.0f;
		float				m_BuildingRenderDistance = 300.0f;
		float				m_BuildingLodFactorLevel1 = 0.f;
		float				m_BuildingLodFactorLevel2 = 0.f;
		float				m_BuildingRenderFactor = 0.f;
		int					m_BuildingCullMode = 1;
		bool				m_BuildingUpdateLodByBlock = false;
		EDITOR_REFLECT
	};

	DEFINECOMFACTORY(BuildingComponent, UNMOVEABLE)
}

#endif
