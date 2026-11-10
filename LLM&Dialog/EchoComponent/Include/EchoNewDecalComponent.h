// --------------------------------------------------------------------------------------
// File:   EchoNewDecalComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2024-6-17
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __EchoNewDecalComponent_h__
#define __EchoNewDecalComponent_h__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"
#include "Actor/EchoActorComponent.h"
#include "Actor/EchoNodeComponent.h"
#include "EchoDecal.h"

namespace Echo
{
	class NewDecalComponentInfo;
	//====================================== NewDecalComponent =======================================
	class _EchoComponentExport NewDecalComponent : public NodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(NewDecalComponent);
		// ActorComponent
	public:
		// 主线程
		void								init(ActorComponentInfo* info) override;
		void								unInit() override;

		// 异步线程
		//void								create(ActorComponentInfo* info) override;
		//void								release() override;
		
		void								initHelperObject(ActorComponentInfo* info) override;
		void								initRenderObject(ActorComponentInfo* info) override;
		void								bindNode() override;

		void								resourceArrived(const Name& resName, const Name& resType, bool bFail, const ResourceBasePtr& resourcePtr) override;

		AxisAlignedBox						getLoadBoundingBox() override;
		void								updateCombinedName() override;

	private:
		void								showEditorHelper(bool isShow) override;
		void								createComRootNodeImpl() override;

	public:
		void								setRenderPriority(const Echo::int32& priority);
		const Echo::int32&					getRenderPriority() const;

		void								setEnableRenderDistance(bool enable);
		bool								getEnableRenderDistance() const;

		void								setMaxRenderDistance(float distance);
		float								getMaxRenderDistance() const;

		void								setDiffuseTexturePath(const std::string& texture_path, bool sync = false);
		const std::string&					getDiffuseTexturePath() const;

		void								setDiffuseTextureUWrap(bool u_wrap);
		bool								getDiffuseTextureUWrap() const;

		void								setDiffuseTextureVWrap(bool v_wrap);
		bool								getDiffuseTextureVWrap() const;

		void								setDiffuseTextureTillingU(float tilling_u);
		float								getDiffuseTextureTillingU() const;

		void								setDiffuseTextureTillingV(float tilling_v);
		float								getDiffuseTextureTillingV() const;


		void								setNormalTexturePath(const std::string& texture_path, bool sync = false);
		const								std::string& getNormalTexturePath() const;

		void								setNormalTextureEnable(bool enable);
		bool								getNormalTextureEnable() const;

		void								setNormalTextureStrength(float value);
		float								getNormalTextureStrength() const;

		void								setNormalTextureUWrap(bool u_wrap);
		bool								getNormalTextureUWrap() const;

		void								setNormalTextureVWrap(bool v_wrap);
		bool								getNormalTextureVWrap() const;

		void								setNormalTextureTillingU(float tilling_u);
		float								getNormalTextureTillingU() const;

		void								setNormalTextureTillingV(float tilling_v);
		float								getNormalTextureTillingV() const;


		void								setEnableStencilTest(bool enable);
		bool								getEnableStencilTest() const;
		
		void								setStencilTestFunc(ECHO_CMP_FUNC func);
		ECHO_CMP_FUNC						getStencilTestFunc() const;

		void								setStencilTestMask(uint8 mask);
		uint8								getStencilTestMask() const;

		void								setEnableIllum(bool enable);
		bool								getEnableIllum() const;

		void								setEnableWeatherCover(bool enable);
		bool								getEnableWeatherCover() const;

		void								setEnableEmissive(bool enable);
		bool								getEnableEmissive() const;
		void								setEmissiveColor(const Vector3& color);
		Vector3								getEmissiveColor();
		void								setEmissiveStrength(float strength);
		float								getEmissiveStrength();

	public:
		bool								getResourceReady() const;
		// 编辑器辅助物体
	private:
		HelperObject*						m_helper_object{ nullptr };
		Entity*								m_helper_entity{ nullptr };
		SceneNode*							m_helper_scene_node{ nullptr };

	private:
		Decal*								m_decal{ nullptr };

		using ResourceMaskFlags = Echo::int8;
		enum ResourceMask : ResourceMaskFlags
		{
			DiffuseTexture = 1,
			NormalTexture = 2,
		};

		ResourceMaskFlags					m_resource_ready_mask{ 0x00 }; // 资源全部准备就绪后显示Decal
		ResourceMaskFlags					m_resourceReady{ 0x01 };

		Echo::int32							m_render_priority{ 0 };

		bool								m_enable_render_distance{ false };
		float								m_max_render_distance{ 0.0f };

		bool								m_enable_stencil_test{ false };
		ECHO_CMP_FUNC						m_stencil_test_func{ ECHO_CMP_FUNC::ECHO_CMP_LESS };
		uint8								m_stencil_test_mask{ 0 };
		//uint8								m_stencil_test_ref{ 0 };

		bool								m_enable_illum{ true };
		bool                                m_enable_weather_cover{ false };

		std::string							m_diffuse_texture_path;
		bool								m_diffuse_texture_u_wrap{ true };
		bool								m_diffuse_texture_v_wrap{ true };
		float								m_diffuse_texture_tilling_u{ 1.0f };
		float								m_diffuse_texture_tilling_v{ 1.0f };

		std::string							m_normal_texture_path;
		bool								m_normal_texture_enable{ false };
		float								m_normal_strength{ 1.0f };
		bool								m_normal_texture_u_wrap{ true };
		bool								m_normal_texture_v_wrap{ true };
		float								m_normal_texture_tilling_u{ 1.0f };
		float								m_normal_texture_tilling_v{ 1.0f };

		bool								m_emissive_enable{ false };
		Vector3								m_emissive_color{ 1.0f };
		float								m_emissive_strength{ 0 };
		

	};


	//====================================== NewDecalComponentInfo =======================================
	class _EchoComponentExport NewDecalComponentInfo : public ActorComponentInfo
	{
	public:
		_DECLCOMINFO_VIRFUN;
		EDITOR_REFLECT;

		NewDecalComponentInfo() = default;
		NewDecalComponentInfo(const Name& comType, ActorCOMMoveableType moveType);

	public:
#ifdef ECHO_EDITOR
		bool								getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif

	public:
		Echo::int32							m_render_priority{ 0 };

		bool								m_enable_render_distance{ false };
		float								m_max_render_distance{ 0.0f };

		bool								m_enable_stencil_test{ false };
		uint32								m_stencil_test_func{ ECHO_CMP_FUNC::ECHO_CMP_LESS };
		uint8								m_stencil_test_mask{ 0 };
		//uint8								m_stencil_test_ref{ 0 };
		
		bool								m_enable_illum{ true };
		bool                                m_enable_weather_cover{ false };

		std::string							m_diffuse_texture_path;
		bool								m_diffuse_texture_u_wrap{ true };
		bool								m_diffuse_texture_v_wrap{ true };
		float								m_diffuse_texture_tilling_u{ 1.0f };
		float								m_diffuse_texture_tilling_v{ 1.0f };

		std::string							m_normal_texture_path;
		bool								m_normal_texture_enable;
		float								m_normal_strength{ 1.0f };
		bool								m_normal_texture_u_wrap{ true };
		bool								m_normal_texture_v_wrap{ true };
		float								m_normal_texture_tilling_u{ 1.0f };
		float								m_normal_texture_tilling_v{ 1.0f };

		bool								m_emissive_enable{ false };
		Vector3								m_emissive_color{ 1.0f };
		float								m_emissive_strength{ 0 };

	};


	//====================================== NewDecalComponentInfo =======================================
	DEFINECOMFACTORY(NewDecalComponent, ALL_MOVEABLE)
}

#endif // __EchoNewDecalComponent_h__
