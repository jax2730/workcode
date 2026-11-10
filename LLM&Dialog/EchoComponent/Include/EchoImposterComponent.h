//-----------------------------------------------------------------------------
// File:   EchoImposterComponent.h
//
// Author: LangGuangyue
//
// Date:   2023-07-06
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef _EchoImposterComponent_H__
#define _EchoImposterComponent_H__

#include "EchoComponentPrerequisites.h" // DLL导出宏需要
#include "Actor/EchoProtocolActorComponent.h" // 基类定义
#include "EchoActorEntityComponent.h"

#include "EchoEntityObject.h"      // CameraCullMode

#include "EchoComDefineMacro.h" // 定义工厂类宏需要
#include "Actor/EchoActorComponent.h"
#include "EchoImposter.h"


namespace Echo
{
	class ImposterComponentInfo;

	class _EchoComponentExport ImposterComponent : public ProtocolNodeActorComponent, public NodeTransformUpdateListener
	{
		DECLAREACTORCOMTYPE_H(ImposterComponent);
	public:
		REGISTER_LUA_CLASS_KEY;
		virtual void						init(ActorComponentInfo* info) override;
		virtual void						unInit() override;
		virtual void						bindNode() override;
		virtual void						connectActor() override;

	public:
		virtual void						registerTickTimeBegin() override;
		virtual void						beginFrame(EventID, ProtocolHandle handle) override;

	public:
		virtual void						EditorPlayImp() override;
		virtual void						EditorPauseImp(bool pause) override;
		virtual void						EditorStopImp() override;

	public:
		virtual void						OnNodeTransformUpdate() override;
		virtual void						NotifiChildImp(int changeMask) override;

		virtual void						visibleStateChanged(bool visibleState) override;
		virtual AxisAlignedBox				getLoadBoundingBox() override;
	public:
		void								useImposter(bool use);
		void								enableImposter(bool enable);
		bool								isEnable()const;

		const Name&							GetImposterPath() { return mTexturePath; };
		void								SetImposterPath(const Name& tex);//设置imposter路径，这里并不会创建imposter

		float								GetEnableDistance();
		void								SetEnableDistance(float dis);//设置imposter的启用距离
		float								GetCullDistance();
		void								SetCullDistance(float dis) { m_CullDist = dis; };
		void								SetGradientDis(float dis) { m_RealImposterDist = dis; } //imposter渐变显示开始距离，应该小于启用距离
		float								GetGradientDis() { return m_RealImposterDist; }
		float								GetGradientFactor() { return m_gradientFactor; }
		void								SetGradientFactor(float val);

		void								SetEnableAABBData(const Vector3& vMin, const Vector3& vMax);//设置用于计算imposter启用距离的包围盒数据

		float								GetImposterScale() { return m_ImposterScale; };
		void								SetImposterScale(float scale);//设置imposter的缩放
		void								SetImposterScale(const Vector3& vMin, const Vector3& vMax);
		// 0~1 点阵剔除
		void								SetImposterDissolve(float dissolve);
		float								GetImposterDissolve();

		void								SetImposterVisible(bool isVisible);
		bool								GetImposterVisible();

		void								SetImposterFactor(float factor);
		void								SetCullFactor(float factor);

		float								GetImposterFactor() { return m_RenderFactor; }
		float								GetCullFactor() { return m_CullFactor; }

		int									GetCullMode() { return m_CullMode; }
		void								SetCullMode(int i) { m_CullMode = i; m_FrameTick = 0; }

		bool								IsCreateImposter() { return m_bIscreateImposter; };
		void								createImposter(const Name& texPath);//创建imposter
		bool								removeImposter();//销毁当前imposter

		void								updateImposterPos();//更新imposter的位置
		void								updateImposterRot();//更新imposter的旋转
		void								updateImposterScale();//更新imposter的缩放

		float								getFactorCullLength();
		float								AdaptiveScale();
	private:
		bool								checkImposterEnable();
		void								checkGradientScope();

		AxisAlignedBox						getBoudingBox();
		void								resetImposterFactor(class VideoConfig* vd = nullptr);
		Name							getPlatformTexName(const Name& texName);
	public:
		//ImposterComponentInfo*				mInfo = nullptr;

		bool								m_bPreview = true;
		bool								m_bAdaptiveScaling = true;
	private:
		bool								m_bIscreateImposter = false;
		uint32								m_ImposterID = 0;
		Name                                mTexturePath; // 保存Imposter组件加载的Texture纹理的文件路径
		bool								m_bEnable = true;
		float								m_ImposterDist = 500.0f;
		float								m_RealImposterDist = 500.0f;
		float								m_dissolve = 0.0f;
		bool								m_dissolveState = false;
		float								m_gradientFactor = 0.9f;
		float								m_RenderFactor = 0.f;
		float								m_CullDist = 1000.f;
		float								m_CullFactor = 0.f;
		//	imposter data
		Vector3								m_ImposterPos	= Vector3::ZERO;
		Vector3								m_ImposterFront = Vector3(0, 0, 1);
		Vector3								m_ImposterUp	= Vector3(0, 1, 0);
		float								m_ImposterScale = 2.f;
		Name								m_ImposterTex;
		//  DefaultCull 
		float								m_ImposterFactor[2] = { 0.f, 0.f };
		float								m_ImposterLengthSq[2] = { 0.f, 0.f };
		float								m_ImposterDissolveLengthSq = 0.f;
		float								m_ImposterCurLengthSq = 0.f;
		int									m_CullMode = 1;
		int									m_FrameTick = 0;

		bool                                m_bShowImposterMesh = false;

		bool								isRegisterNodeListener = false;

		AxisAlignedBox						m_EnableAABB = AxisAlignedBox::BOX_NULL;
	};

	class _EchoComponentExport ImposterComponentInfo : public ActorComponentInfo
	{
		friend class ImposterComponent;
	public:
		ImposterComponentInfo() {}
		ImposterComponentInfo(const Name& type, ActorCOMMoveableType movable = ALL_MOVEABLE);
		~ImposterComponentInfo();
#ifdef ECHO_EDITOR
		virtual bool getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif
		void						ExportData(cJSON* jsonNode) override;
		void						ImportData(cJSON* jsonNode) override;
		void						CopyTo(ActorComponentInfo* p_actor_com_info) override;
		virtual void				Overlay(ActorComponentInfo* comInfo) override;
		virtual void				OverWrite()override;

	public:

		Name                        mTexturePath; // 保存Imposter组件加载的Texture纹理的文件路径
		float						m_ImposterDist = 500; // Change
		float						m_ImposterScale = 2.0f; 
		float						m_ImposterFactor = 0.f;
		float						m_CullDist = 1000.f;
		float						m_CullFactor = 0.f;
		int							m_CullMode = 1;
		EDITOR_REFLECT
	};
	
	class _EchoComponentExport ImposterComponentFactory : public Echo::ActorComFactory
	{
		static_assert(std::is_same<decltype(ALL_MOVEABLE), Echo::ActorCOMMoveableType>::value, "MOVEABLE must be ActorCOMMoveableType type!");
	public:
		ImposterComponentFactory();
		virtual ~ImposterComponentFactory();
		virtual Echo::EchoSharedPtr<Echo::ActorComponent>		create(const Echo::ActorPtr& pActor)	override { return Echo::EchoSharedPtr<Echo::ActorComponent>(new ImposterComponent(pActor, ALL_MOVEABLE), Echo::ActorComponentPtrDeleter); }
		virtual Echo::ActorComponentInfo* createComponentInfo()	override { return new ImposterComponentInfo(mType, ALL_MOVEABLE); }
		static Echo::ProtocolResult										ProCom_EnableImposter(void* pos, Echo::ProtocolHandle handle);
	};
}

#endif
