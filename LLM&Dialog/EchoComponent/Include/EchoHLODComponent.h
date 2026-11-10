//-----------------------------------------------------------------------------
// File:   EchoHLODComponent.h
//
// Author: liaojiaye
//
// Date:   2023-3-29
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef _EchoHLODComponent_H__
#define _EchoHLODComponent_H__


#include "EchoComponentPrerequisites.h" // DLL导出宏需要
#include "Actor/EchoProtocolActorComponent.h" // 基类定义
#include "EchoEntityObject.h"      // CameraCullMode
#include "EchoHelperObj.h"

#include "EchoComDefineMacro.h" // 定义工厂类宏需要 

#define MEMORY_COMPUTE
namespace Echo
{
	enum HLODTextureType
	{
		DIFFUSE = 0,
		NORMAL,
		MAX_TYPE = 2 // 目前支持最多两种类型
	};
	enum HLODPlatform : uint8_t
	{
		PC = 0,
		Mobile
	};
	struct HLODMergeSettings
	{
		HLODPlatform mPlatform = PC;
		bool bCreateTexture = true;
		bool BakeMaterialEnabled = true;
		uint16 BakeOutputSize = 1024;
		String BakeOutputDirectoty = "model/hlod/cache";
		int32 ScreenSize = 3;
		bool bManualTextureSize = true;
		int32 TextureSize = -1;
		int32 NormalTextureSize = -1;
		int32 MetallicTextureSize = -1;
		int32 MaxTriNum = -1;
		Name outputFilePath = Name("");
		std::string DefaultDiffuse = "";// "model/stone/texture/stone_hssm_tiehua1_b.dds";
		struct FColor
		{
			uint8_t B, G, R, A;
			FColor(uint8_t mB = 255, uint8_t mG = 255, uint8_t mR = 255, uint8_t mA = 255)
				: B(mB)
				, G(mG)
				, R(mR)
				, A(mA)
			{}
		};
		FColor DefaultColor = FColor(214, 214, 49, 255);
		
		Name CustomMaterial = Name("model/common/default_hlod.material");
		Name GreyMaterial = Name("model/hlod/material/hlod_hm.material");
		std::set<Name> collectComTypeSet = { Name("EntityComponent"), Name("RoleComponent"),Name("HLODComponent") };
		std::set<Name> exceptChildTypeSet = { Name("InOutsideDivisionComponent"), Name("HLODComponent") };
		std::map<Name, std::string>	ErrorLogMap;
#ifdef MEMORY_COMPUTE
		int32 MemoryLimit=16;
#endif
		// -----------------------------------------------------------
		bool	singleCast = false; // 不合并actor，而是把单个对象转换
	};

	class _EchoComponentExport HLODComActiveListener
	{
	public:
		virtual ~HLODComActiveListener() {}

		virtual void HLODComActiveCallBack(bool bActive) {}
	};
	typedef set<HLODComActiveListener*>::type         HLODComActiveListenerSet;

	class HLODComponentInfo;
	class HLODComponent;

	class BuildingCreateListener : public ActorLoadListener
	{
	public:
		BuildingCreateListener(HLODComponent* host);
		bool OnActorCreateFinish();
	private:
		HLODComponent* m_pHost;
	};

	class _EchoComponentExport HLODComponent : public ProtocolActorComponent
	{
		DECLAREACTORCOMTYPE_H(HLODComponent)
	

	public:
		REGISTER_LUA_CLASS_KEY;
		virtual void						init(ActorComponentInfo* info) override;
		virtual void						unInit() override;
		virtual void						bindNode() override;
		virtual void						disconnectCom() override;
		virtual void						connectActor() override;
		virtual void						visibleStateChanged(bool visibleState) override;

		virtual void						NotifiChildImp(int changeMask) override;
	public:
		virtual void						registerTickTimeBegin() override;
		virtual void						beginFrame(EventID, ProtocolHandle handle) override;

		virtual void									EditorPlayImp() override;
		virtual void									EditorPauseImp(bool pause) override;
		virtual void									EditorStopImp() override;
	private:
		void								calCullDistance(float arr[3]);
		void								updateBuildLod2Distence();
	// 这块目前是照抄EntityComponent的
	public:
		void								setHLODVisible(bool isVisible);
		bool								getHLODVisible();
		void								updateCombinedName() override;
		CameraCullMode						getCullMode();
		void								setCullMode(int mode = 0);
		float								getRenderDistance();								// 裁切距离
		void								setRenderDistance(float dis);
		float								getTODRenderDis();									// TOD裁切距离
		void								setTODRenderDis(float dis);

		void								setHLODRenderDis(float dis);
		float								getHLODRenderDis();

		float								getFactorCullLength();

		void								setDissolveRenderDis(float dis);
		float								getDissolveRenderDis();
		// val 取值为 0.0f ~ 1.0f 0为不消融 1为完全消融
		float								getDissolveStrength() { return mDissolveStrength; }	// 消融的强度								// 消融的强度
		void								setDissolveStrength(float val);
		//投射阴影
		void								setCastShadow(bool isEnable);
		bool								getCastShadow();

		//lod
		void								setLodDisLevel1(float val);
		float								getLodDisLevel1();
		void								setLodDisLevel2(float val);
		float								getLodDisLevel2();

		void                                setEnityFpsState(const int& state_);

		bool								mergeHLOD(HLODMergeSettings& settings);

		//添加hlod功能启用的回调
		void addHLODComActiveListener(HLODComActiveListener* listener);

		//移除hlod功能关闭的回调
		void removeHLODComActiveListener(HLODComActiveListener* listener);
	public:
		void								setEntityPath(const Name& path);
		void								useHLOD(bool use);
		void								enableHLOD(bool enable);
		bool								isEnable()const;
		void								loadEntity(const Name& path, bool synchronous = false);	// 参考 EntityComponent 同名函数
		void								createEntity(EM_LOD_LEVEL lodLevel, const MeshPtr& pMesh);
		void								createReplaceHelper();
		void								destroyReplaceHelper();
		void                                showNormalMap(bool);

		void								setHLODFactor(float factor);
		float								getHLODFactor()const { return m_fHLODFactor; }
		void								setDissolveFactor(float factor);
		float								getDissolveFactor()const { return m_fDissolveFactor; }
		void								setCullFactor(float factor);
		float								getCullFactor()const { return m_fCullFactor; }

		void								setReplaceBox(const AxisAlignedBox& box);
		void								hlodMeshExist(bool exist);

		void								setRelateColliderComName(const String&);
		const String&						getRelateColliderComName() const;
		AxisAlignedBox						getBoudingBox();
		void								resetLastHLODLength(class VideoConfig* vd = nullptr);

		virtual void						showEditorHelper(bool isShow) override;

		virtual bool						isCreateFinish() override;

		Entity*								getLowestEntity();
		virtual AxisAlignedBox	getLoadBoundingBox() override;
		EntityObject*						getEntityObject() { return m_pEntityObject; }
		void								entityCreateFinished();
		void								setRelateColliderComponent(const ActorComponentPtr& comPtr);
		const ActorComponentPtr&			getRelateColliderComponent()const;
	public:
		Name                                mEntityPath; // 保存HLOD组件加载的模型的文件路径
		bool                                m_bShowHlodMesh=false;
		float								m_fHLODDist = 300.0f;
		float								m_fHLODFactor = 0.0f;
		//float								m_fHLODDistSq = 0.0f;
		float								m_fDissolveFactor = 0.9f; //溶解距离系数
		float								m_fDissolveDist = 0.0f; //
		float								m_fDissolveDistSq = 0.0f;
		float								m_fCullFactor = 0.0f;
		float								m_fCullDistSq = 0.0f;
		int									m_fCullMode = 1;
	private:
		EntityObject*                       m_pEntityObject = nullptr;
		HelperObject*						m_pReplaceObject = nullptr;
		AxisAlignedBox						m_replaceBox = AxisAlignedBox::BOX_NULL;
		ActorComponentPtr					m_pRelateColliderComponent;

		String								m_strRelateColliderComName = "";
		//HelperObject*						m_pHLODObject = nullptr;
		//HelperObject*						m_pDissolveObject = nullptr;
		//HelperObject*						m_pCullObject = nullptr;
		float								mDissolveStrength = 0.0f;
		float								mLastHLODFactor[2] = { 0.f,0.f };
		float								mLastHLODLength[2] = { 0.f,0.f };
		float								mLastHlodDissolveLength = 0.f;
		int									m_FrameTick = 0;
		//bool                                bNormal = false;
		bool								m_bEnable = true;
		bool								m_bCreateFinish = false;
		bool								m_bHLODMeshExist = true;
		HLODComActiveListenerSet		m_ActiveListenerSet;
		friend class BuildingCreateListener;
		BuildingCreateListener*			m_pBuildingCreateListener = nullptr;
		ActorComponentPtr				m_BuildingComp;
		float							m_BuildingCullSq = 0.0f;
	};

	class _EchoComponentExport HLODComponentInfo : public ActorComponentInfo
	{
		friend class HLODComponent;
	public:
		HLODComponentInfo(){}
		HLODComponentInfo(const Name& type, ActorCOMMoveableType movable = ALL_MOVEABLE);
		~HLODComponentInfo();

		_DECLCOMINFO_VIRFUN

	public:
#ifdef ECHO_EDITOR
		virtual bool										getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif
		Name GetMeshName() const { return m_MeshName; }
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
		void                        setDist(float size) { m_HLODDist = size; }
		float                       getDist() { return m_HLODDist; }

	public:
		// 存放减面后模型的路径
		Name						m_MeshName;

		String						m_RelateColliderComName = "";
		int							m_CullMode = 1;
		float						m_CullDistance = 1000.f;
		float						m_TodCullDistance = 1000.f;
		float						m_DissolveStrength = 0.0f;
		int							m_FpsState = 0;
		bool						m_CastShadow = true;
		float						m_LodDis_Level1 = 100.0f;
		float						m_LodDis_Level2 = 200.0f;

		float						m_HLODDist = 300; // Change
		float						m_HLODFactor = 0.0f;
		float						m_DissolveFactor = 0.0f;
		float						m_CullFactor = 0.0f;

		EDITOR_REFLECT
	};

	class _EchoComponentExport HLODComponentFactory : public Echo::ActorComFactory
	{
		static_assert(std::is_same<decltype(ALL_MOVEABLE), Echo::ActorCOMMoveableType>::value, "MOVEABLE must be ActorCOMMoveableType type!");
	public:
		HLODComponentFactory();
		virtual ~HLODComponentFactory();
		virtual Echo::EchoSharedPtr<Echo::ActorComponent>		create(const Echo::ActorPtr& pActor)	override { return Echo::EchoSharedPtr<Echo::ActorComponent>(new HLODComponent(pActor, ALL_MOVEABLE), Echo::ActorComponentPtrDeleter); }
		virtual Echo::ActorComponentInfo* createComponentInfo()	override { return new HLODComponentInfo(mType, ALL_MOVEABLE); }
		static Echo::ProtocolResult										ProCom_EnableHLOD(void* pos, Echo::ProtocolHandle handle);
		static Echo::ProtocolResult										ProCom_DisplayMissHLODLog(void* pos, Echo::ProtocolHandle handle);
		static Echo::ProtocolResult										ProCom_DisplayAllHLODAABBAndDisLog(void* pos, Echo::ProtocolHandle handle);
	};
}
#endif
