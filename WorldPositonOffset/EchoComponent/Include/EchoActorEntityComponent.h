#ifndef _EchoActorEntityComponent_H__
#define _EchoActorEntityComponent_H__

#include "EchoComponentPrerequisites.h"
#include "EchoSubEntity.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"
#include "Actor/EchoActorComponent.h"
#include "Actor/EchoNodeComponent.h"
namespace Echo
{
	class _EchoComponentExport EntityComListener
	{
	public:
		virtual ~EntityComListener() {}

		virtual void EntityCreateFinishCallBack() {}
	};
	typedef set<EntityComListener*>::type         EntityComListenerSet;

	class EntityComponentInfo;
	//=============================================================================
	class _EchoComponentExport EntityComponent : public NodeActorComponent
	{
		friend class MaterialComponent;
		DECLAREACTORCOMTYPE_H(EntityComponent)

	public:
		REGISTER_LUA_CLASS_KEY;
		virtual void							init(ActorComponentInfo* info) override;
		virtual void							initRenderObject(ActorComponentInfo* info) override;
		virtual void							unInit() override;
		virtual void							replay(ActorComponentInfo* info) override;
		virtual void							bindNode() override;
		virtual void							connectActor() override;
		virtual void							disconnectCom() override;
		virtual void							disconnectActor() override;
		virtual void							copyTo(const ActorComponentPtr& newComp)override;
		virtual void							visibleStateChanged(bool visibleState)override;
		virtual void							createComRootNodeImpl() override;
		virtual void							DeleteComRootNodeImpl() override;
		virtual void							NotifiChildImp(int changeMask) override;

		virtual void							updateActiveTypeState(ActorComActiveFlags inVal, ActorComActiveFlags inChangeMask) override;
		virtual void							updatePortalCullType(PortalCullType cullType) override;

		void									setActor(const ActorPtr& pActor) override;
		virtual void							setQueryMask(Echo::uint32 flags) override;
#ifdef ECHO_EDITOR
		virtual void							getErrorMessageImpl(String& message) override;
		virtual bool							hasErrorMessageImpl() override;
#endif
		void									updateCombinedName() override;

		CameraCullMode							getCullMode();
		void									setCullMode(int mode = 0);
		float									getRenderDistance();								// 裁切距离
		void									setRenderDistance(float dis);
		float									getTODRenderDis();									// TOD裁切距离
		void									setTODRenderDis(float dis);
		// val 取值为 0.0f ~ 1.0f 0为不消融 1为完全消融
		float									getDissolveStrength();	// 消融的强度								// 消融的强度
		void									setDissolveStrength(float val);

		//投射阴影
		void									setCastShadow(bool isEnable);
		bool									getCastShadow();




		//lod
		void									setLodDisLevel1(float val);
		float									getLodDisLevel1();
		void									setLodDisLevel2(float val);
		float									getLodDisLevel2();

		void									loadEntity(const Name& path, bool synchronous = false);	// 首次加载entity path为.mesh文件的路径
		void									changeEntity(const std::string& path, bool synchronous = false);	// 更换entity
		//void									removeEntity();					// 移除当前加载的entity

		void                                    setEnityFpsState(const int& state_);
		int										getEnityFpsState();

		Entity*                                 getEntity();
		Entity*                                 getCurEntity();

		Entity*                                 getLowestEntity();
		virtual AxisAlignedBox	getLoadBoundingBox() override;

		void setMatNames(const std::map<uint32, st_MaterialInfo>& vtMatNames);
		void setCustomUniform(uint8 uniformIndex, uint8 paramIndex, float paramVal, int lodLevel = -1, int subIndex = -1,EM_UNIFORM_TYPE uniformType = UNIFORM_TYPE_PS);
		void resetCustomUniform(uint8 uniformIndex, uint8 paramIndex, int lodLevel = -1, int subIndex = -1, EM_UNIFORM_TYPE uniformType = UNIFORM_TYPE_PS);
		float getCustomUniform(uint8 uniformIndex, uint8 paramIndex, int lodLevel = -1, int subIndex = -1, EM_UNIFORM_TYPE uniformType = UNIFORM_TYPE_PS);

		void                                    onEntityCreateFinish();

		//添加模型创建完毕的回调
		void addEntityCreateListener(EntityComListener* listener);

		//移除模型创建完毕的回调
		void removeEntityCreateListener(EntityComListener* listener);

		EntityObject* getEntityObject() { return m_pEntityObject; }

		virtual bool isCreateFinish() override;

		void		setClientLogMessage(const Name& logstr);

		void        setAutoLodLevel(bool val);
	public:
		Name									mEntityPath;
		//EntityComponentInfo*					mInfo = nullptr;

	private:
		EntityObject*							m_pEntityObject = nullptr;
		bool									m_bAttachNode = false;
		bool                                    m_bCreateFinish = false;
		EntityComListenerSet					m_setEntityComListeners;
	};

	class _EchoComponentExport EntityComponentInfo : public ActorComponentInfo
	{
	public:
		EntityComponentInfo() {}
		EntityComponentInfo(const Name& comType, ActorCOMMoveableType moveType);
		virtual ~EntityComponentInfo();

		_DECLCOMINFO_VIRFUN
		//virtual void									ExportData(cJSON* jsonNode)override;
		//virtual void									ImportData(cJSON* jsonNode)override;
		//virtual void									CopyTo(ActorComponentInfo* comInfo)override;
		//virtual void									Overlay(ActorComponentInfo* srcInfo)override;
		//virtual void                                    OverWrite() override;
#ifdef ECHO_EDITOR
		virtual bool getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif
		std::string GetMeshName() const { return m_MeshName; }
		void SetMeshName(const std::string& val) { m_MeshName = val; }

		float GetDissolveStrength() const { return m_DissolveStrength; }
		void SetDissolveStrength(float val) { m_DissolveStrength = val; }

		float GetTodCullDistance() const { return m_TodCullDistance; }
		void SetTodCullDistance(float val) { m_TodCullDistance = val; }

		float GetCullDistance() const { return m_CullDistance; }
		void SetCullDistance(float val) { m_CullDistance = val; }

		int GetCullMode() const { return m_CullMode; }
		void SetCullMode(int val) { m_CullMode = val; }

		int GetFpsState() const { return m_FpsState; }
		void SetFpsState(int val) { m_FpsState = val; }
	public:
		std::string   m_MeshName;
		int           m_CullMode = 1;
		float         m_CullDistance = 1000.f;
		float         m_TodCullDistance = 1000.f;
		float         m_DissolveStrength = 0.0f;
		int           m_FpsState = 0;
		bool		  m_CastShadow = true;
		float		  m_LodDis_Level1 = 100.0f;
		float		  m_LodDis_Level2 = 200.0f;

		//NodeComponentInfo mNodeComponet;

		EDITOR_REFLECT


		//DECLARECOMINFO_NAME(EntityComponent, MeshName,Name(""), changeEntity);
		//DECLARECOMINFO_INT(EntityComponent, CullMode, 0, setCullMode);// 裁切模式
		//DECLARECOMINFO_FLOAT(EntityComponent, CullDistance, 1000.f, setRenderDistance);
		//DECLARECOMINFO_FLOAT(EntityComponent, TodCullDistance, 1000.f, setTODRenderDis);
		//DECLARECOMINFO_FLOAT(EntityComponent, DissolveStrength,0.0f, setDissolveStrength);
		//DECLARECOMINFO_INT(EntityComponent, FpsState, 0, setEnityFpsState);
	};

	class _EchoComponentExport EntityComponentFactory : public Echo::ActorComFactory
	{
		static_assert(std::is_same<decltype(ALL_MOVEABLE), Echo::ActorCOMMoveableType>::value, "MOVEABLE must be ActorCOMMoveableType type!");
	public:
		EntityComponentFactory();
		virtual ~EntityComponentFactory();
		virtual Echo::EchoSharedPtr<Echo::ActorComponent>		create(const Echo::ActorPtr& pActor)	override { return Echo::EchoSharedPtr<Echo::ActorComponent>(new EntityComponent(pActor, ALL_MOVEABLE), Echo::ActorComponentPtrDeleter); }
		virtual Echo::ActorComponentInfo* createComponentInfo()	override { return new EntityComponentInfo(mType, ALL_MOVEABLE); }
		static Echo::ProtocolResult										ProCom_OutputLodInfo(void*, Echo::ProtocolHandle);
	protected:
		static void outputMeshLodInfo(DataStreamPtr& dataStream, const Name& meshName, std::set<Name>& meshFindSet);
		static void outputMeshLodInfoImpl(DataStreamPtr& dataStream, const Name& meshName, std::set<Name>& meshFindSet, uint32& maxTriNum);
	};

	// 脚本类
	class EchoEntityScript : public EchoActorComponentScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;
	public:
		EchoEntityScript() = default;
		EchoEntityScript(const EchoActorScript& pActor, const std::string& name) :
			EchoActorComponentScript(pActor, name) {}

	public:
		static EchoEntityScript			Script_GetEntityComponent(const EchoActorScript& pActor, const std::string& name);
	};
}// namespace Echo

#endif
