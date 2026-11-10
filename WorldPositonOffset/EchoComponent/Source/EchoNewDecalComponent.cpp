// --------------------------------------------------------------------------------------
// File:   EchoNewDecalComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2024-6-17
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"

#include "EchoNewDecalComponent.h"

#include "EchoTextureManager.h"
#include "EchoDecalManager.h"

#include "EchoResourceBackgroundQueue.h"
#include "EchoEntity.h"
#include "EchoHelperObj.h"

#include "Actor/EchoActorHelper.h"

namespace Echo
{
	//====================================== NewDecalComponent =======================================
	DECLAREACTORCOMTYPE_CPP(NewDecalComponent)
	NewDecalComponent::NewDecalComponent(ActorPtr  pActor, ActorCOMMoveableType type) :
		NodeActorComponent(pActor, type)
	{
	}

	NewDecalComponent::~NewDecalComponent()
	{

	}

	void NewDecalComponent::init(ActorComponentInfo* info)
	{
		NewDecalComponentInfo* decal_com_info = static_cast<NewDecalComponentInfo*>(info);
		if (decal_com_info)
		{
			m_diffuse_texture_path = decal_com_info->m_diffuse_texture_path;
			m_render_priority = decal_com_info->m_render_priority;

			m_enable_render_distance = decal_com_info->m_enable_render_distance;
			m_max_render_distance = decal_com_info->m_max_render_distance;

			m_enable_stencil_test = decal_com_info->m_enable_stencil_test;
			m_stencil_test_func = static_cast<ECHO_CMP_FUNC>(decal_com_info->m_stencil_test_func);
			m_stencil_test_mask = decal_com_info->m_stencil_test_mask;
			//m_stencil_test_ref = decal_com_info->m_stencil_test_ref;

			m_enable_illum = decal_com_info->m_enable_illum;
			m_enable_weather_cover = decal_com_info->m_enable_weather_cover;
			m_diffuse_texture_u_wrap = decal_com_info->m_diffuse_texture_u_wrap;
			m_diffuse_texture_v_wrap = decal_com_info->m_diffuse_texture_v_wrap;
			m_diffuse_texture_tilling_u = decal_com_info->m_diffuse_texture_tilling_u;
			m_diffuse_texture_tilling_v = decal_com_info->m_diffuse_texture_tilling_v;

			m_normal_texture_path = decal_com_info->m_normal_texture_path;
			m_normal_texture_enable = decal_com_info->m_normal_texture_enable;
			m_normal_texture_u_wrap = decal_com_info->m_normal_texture_u_wrap;
			m_normal_texture_v_wrap = decal_com_info->m_normal_texture_v_wrap;
			m_normal_texture_tilling_u = decal_com_info->m_normal_texture_tilling_u;
			m_normal_texture_tilling_v = decal_com_info->m_normal_texture_tilling_v;


			m_emissive_enable = decal_com_info->m_emissive_enable;;
			m_emissive_color = decal_com_info->m_emissive_color;;
			m_emissive_strength = decal_com_info->m_emissive_strength;;
		}
	}

	void NewDecalComponent::unInit()
	{
		if (m_helper_entity)
		{
			if (m_helper_scene_node)
			{
				m_helper_scene_node->detachObject(m_helper_entity);
			}
			getSceneManager()->destroyEntity(m_helper_entity);
			m_helper_entity = nullptr;
		}

		if (m_helper_scene_node)
		{
			RemoveChildFromNode(m_helper_scene_node);
			getSceneManager()->destroySceneNode(m_helper_scene_node);
			m_helper_scene_node = nullptr;
		}

		if (m_helper_object)
		{
			RemoveChildFromNode(m_helper_object->getSceneNode());
			delete m_helper_object;
			m_helper_object = nullptr;
		}

		if (m_decal != nullptr)
		{
			detachFromNode(m_decal);

			DecalManager* decal_manager = getSceneManager()->getDecalManager();
			if (decal_manager)
			{
				decal_manager->destroyDecal(m_decal);
				m_decal = nullptr;
			}
		}

		UpdateComRootNode(true);
	}

	//void NewDecalComponent::create(ActorComponentInfo* info)
	//{
	//	ActorComponent::create(info);
	//}

	//void NewDecalComponent::release()
	//{

	//}

	void NewDecalComponent::initHelperObject(ActorComponentInfo* info)
	{
		if (m_helper_entity == nullptr)
		{
			MeshPtr shphere = HelperObjectPreloading::instance()->GetSphereMesh();

			if (shphere.isNull() == false)
			{
				m_helper_entity = getSceneManager()->createEntity(Name(getCombinedName() + "||" + "newdecal_shphere"), shphere, false);
				m_helper_entity->setVisible(false);
				m_helper_entity->setRenderingDistance(1000.0f);
				m_helper_entity->setCameraCullMode(CameraCullMode::DistanceCull);
			}
		}

		if (m_helper_scene_node == nullptr)
		{
			m_helper_scene_node = getSceneManager()->createSceneNode(Name(getCombinedName()));
		}

		if (m_helper_object == nullptr)
		{
			m_helper_object = new HelperObject(Name(getCombinedName()), HelperObjType_Box, false, NULL, getSceneManager());
			if (m_helper_object)
			{
				m_helper_object->setRenderType(ECHO_LINE_STRIP);
				m_helper_object->setHelperColorType(HelperObjColorType_white);
				m_helper_object->setVisible(false);
			}
		}
	}

	void NewDecalComponent::initRenderObject(ActorComponentInfo* info)
	{
		if (m_decal == nullptr)
		{
			DecalManager* decal_manager = getSceneManager()->getDecalManager();
			if (decal_manager)
			{
				m_decal = decal_manager->createDecal();

				if (m_decal)
				{
					setDiffuseTexturePath(m_diffuse_texture_path);
					setNormalTexturePath(m_normal_texture_path);

					m_decal->setRenderPriority(m_render_priority);
					m_decal->setEnableRenderDistance(m_enable_render_distance);
					m_decal->setMaxRenderDistance(m_max_render_distance);

					m_decal->setEnableStencilTest(m_enable_stencil_test);
					m_decal->setStencilTestFunc(m_stencil_test_func);
					m_decal->setStencilTestMask(m_stencil_test_mask);
					//m_decal->setStencilTestRef(m_stencil_test_ref);

					m_decal->setEnableIllum(m_enable_illum);
					m_decal->setEnableWeatherCover(m_enable_weather_cover);

					m_decal->setDiffuseTextureUWrap(m_diffuse_texture_u_wrap);
					m_decal->setDiffuseTextureVWrap(m_diffuse_texture_v_wrap);
					m_decal->setDiffuseTextureTillingU(m_diffuse_texture_tilling_u);
					m_decal->setDiffuseTextureTillingV(m_diffuse_texture_tilling_v);

					m_decal->setNormalTextureEnable(m_normal_texture_enable);
					m_decal->setNormalTextureUWrap(m_normal_texture_u_wrap);
					m_decal->setNormalTextureVWrap(m_normal_texture_v_wrap);
					m_decal->setNormalTextureTillingU(m_normal_texture_tilling_u);
					m_decal->setNormalTextureTillingV(m_normal_texture_tilling_v);

					m_decal->setEnableEmissive(m_emissive_enable);
					m_decal->setEmissiveColor(m_emissive_color);
					m_decal->setEmissiveStrength(m_emissive_strength);
					if (getResourceReady())
					{
						m_decal->setVisible(true);
					}
				}
			}
		}
	}

	void NewDecalComponent::bindNode()
	{
		if (m_decal)
		{
			if (m_helper_object)
			{
				addChildToNode(m_helper_object->getSceneNode());
			}

			if (m_helper_scene_node)
			{
				addChildToNode(m_helper_scene_node);
			}

			if (m_helper_entity)
			{
				m_helper_scene_node->attachObject(m_helper_entity);
				m_helper_scene_node->setScale(Vector3(0.4f, 0.4f, 0.4f));
			}

			attachToNode(m_decal);
			UpdateComRootNode(true);
		}
	}

	void NewDecalComponent::resourceArrived(const Name& resName, const Name& resType, bool bFail, const ResourceBasePtr& resourcePtr) 
	{
		if (m_decal)
		{
			if (resName == Name(m_diffuse_texture_path))
			{
				m_resource_ready_mask |= ResourceMask::DiffuseTexture;

				m_decal->setDiffuseTexture(resourcePtr);
			}
			else if (resName == Name(m_normal_texture_path))
			{
				m_resource_ready_mask |= ResourceMask::NormalTexture;
				m_decal->setNormalTexture(resourcePtr);
			}

			if (getResourceReady())
			{
				m_decal->setVisible(true);
			}
		}
	}

	void NewDecalComponent::setDiffuseTexturePath(const std::string& texture_path, bool sync)
	{
		m_diffuse_texture_path = texture_path;

#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			NewDecalComponentInfo* decal_com_info = static_cast<NewDecalComponentInfo*>(m_pActComInfo);
			decal_com_info->m_diffuse_texture_path = m_diffuse_texture_path;
		}
#endif

		bool need_request = false;

		if (texture_path.empty())
		{
			TexturePtr nullTexturePtr;
			m_decal->setDiffuseTexture(nullTexturePtr);
			return;
		}

		if (m_decal)
		{
			TexturePtr texture_ptr = m_decal->getDiffuseTexture();
			if (texture_ptr.isNull() || texture_ptr->getName() != Name(texture_path))
			{
				need_request = true;
			}
		}

		if (need_request)
		{
			if (sync)
			{
				TexturePtr diffuse_texture = TextureManager::instance()->loadTexture(Name(m_diffuse_texture_path));
				if (diffuse_texture.isNull() == false)
				{
					m_resource_ready_mask |= ResourceMask::DiffuseTexture;
					m_decal->setDiffuseTexture(diffuse_texture);
				}
			}
			else
			{
				requestResouce(Name(m_diffuse_texture_path), TextureManager::instance()->getResourceType(), WorkQueue::RequestPriority::REQUEST_QUEUE_CRITICAL);
			}
		}
	}

	const std::string& NewDecalComponent::getDiffuseTexturePath() const
	{
		return m_diffuse_texture_path;
	}

	void NewDecalComponent::setDiffuseTextureUWrap(bool u_wrap)
	{
		m_diffuse_texture_u_wrap = u_wrap;
		if (m_decal)
		{
			m_decal->setDiffuseTextureUWrap(m_diffuse_texture_u_wrap);
		}
	}

	bool NewDecalComponent::getDiffuseTextureUWrap() const
	{
		return m_diffuse_texture_u_wrap;
	}

	void NewDecalComponent::setDiffuseTextureVWrap(bool v_wrap)
	{
		m_diffuse_texture_v_wrap = v_wrap;
		if (m_decal)
		{
			m_decal->setDiffuseTextureVWrap(m_diffuse_texture_v_wrap);
		}
	}

	bool NewDecalComponent::getDiffuseTextureVWrap() const
	{
		return m_diffuse_texture_v_wrap;
	}

	void NewDecalComponent::setDiffuseTextureTillingU(float tilling_u)
	{
		m_diffuse_texture_tilling_u = tilling_u;
		if (m_decal)
		{
			m_decal->setDiffuseTextureTillingU(m_diffuse_texture_tilling_u);
		}
	}

	float NewDecalComponent::getDiffuseTextureTillingU() const
	{
		return m_diffuse_texture_tilling_u;
	}

	void NewDecalComponent::setDiffuseTextureTillingV(float tilling_v)
	{
		m_diffuse_texture_tilling_v = tilling_v;
		if (m_decal)
		{
			m_decal->setDiffuseTextureTillingV(m_diffuse_texture_tilling_v);
		}
	}

	float NewDecalComponent::getDiffuseTextureTillingV() const
	{
		return m_diffuse_texture_tilling_v;
	}

	void NewDecalComponent::setNormalTexturePath(const std::string& texture_path, bool sync)
	{
		m_normal_texture_path = texture_path;

#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			NewDecalComponentInfo* decal_com_info = static_cast<NewDecalComponentInfo*>(m_pActComInfo);
			decal_com_info->m_normal_texture_path = m_normal_texture_path;
		}
#endif

		if (texture_path.empty())
		{
			TexturePtr normal_texture;
			m_decal->setNormalTexture(normal_texture);
		}
		else
		{
			bool need_request = false;
			if (m_decal)
			{
				TexturePtr texture_ptr = m_decal->getNormalTexture();
				if (texture_ptr.isNull() || texture_ptr->getName() != Name(texture_path))
				{
					need_request = true;
				}
			}

			if (need_request)
			{
				if (sync)
				{
					TexturePtr normal_texture = TextureManager::instance()->loadTexture(Name(m_normal_texture_path));
					if (normal_texture.isNull() == false)
					{
						m_resource_ready_mask |= ResourceMask::NormalTexture;
						m_decal->setNormalTexture(normal_texture);
					}
				}
				else
				{
					requestResouce(Name(m_normal_texture_path), TextureManager::instance()->getResourceType(), WorkQueue::RequestPriority::REQUEST_QUEUE_CRITICAL);
				}
			}
		}
	}



	const std::string& NewDecalComponent::getNormalTexturePath() const
	{
		// TODO: 在此处插入 return 语句
		return m_normal_texture_path;
	}

	void NewDecalComponent::setNormalTextureEnable(bool enable)
	{
		m_normal_texture_enable = enable;
		if (m_decal)
		{
			m_decal->setNormalTextureEnable(m_normal_texture_enable);
		}
	}

	bool NewDecalComponent::getNormalTextureEnable() const
	{
		return m_normal_texture_enable;
	}

	void NewDecalComponent::setNormalTextureStrength(float value)
	{
		m_normal_strength = value;
		if (m_decal)
		{
			m_decal->setNormalTextureStrength(m_normal_strength);
		}
	}

	float NewDecalComponent::getNormalTextureStrength() const
	{
		return m_normal_strength;
	}

	void  NewDecalComponent::setNormalTextureUWrap(bool u_wrap)
	{
		m_normal_texture_u_wrap = u_wrap;
		if (m_decal)
		{
			m_decal->setNormalTextureUWrap(m_normal_texture_u_wrap);
		}
	}

	bool  NewDecalComponent::getNormalTextureUWrap() const
	{
		return m_normal_texture_u_wrap;
	}

	void  NewDecalComponent::setNormalTextureVWrap(bool v_wrap)
	{
		m_normal_texture_v_wrap = v_wrap;
		if (m_decal)
		{
			m_decal->setNormalTextureVWrap(m_normal_texture_v_wrap);
		}
	}

	bool  NewDecalComponent::getNormalTextureVWrap() const
	{
		return m_normal_texture_v_wrap;
	}

	void  NewDecalComponent::setNormalTextureTillingU(float tilling_u)
	{
		m_normal_texture_tilling_u = tilling_u;
		if (m_decal)
		{
			m_decal->setNormalTextureTillingU(m_normal_texture_tilling_u);
		}
	}
	float NewDecalComponent::getNormalTextureTillingU() const
	{
		return m_diffuse_texture_tilling_u;
	}

	void NewDecalComponent::setNormalTextureTillingV(float tilling_v)
	{
		m_normal_texture_tilling_v = tilling_v;
		if (m_decal)
		{
			m_decal->setNormalTextureTillingV(m_normal_texture_tilling_v);
		}
	}

	float NewDecalComponent::getNormalTextureTillingV() const
	{
		return m_normal_texture_tilling_v;
	}


	void NewDecalComponent::setEnableStencilTest(bool enable)
	{
		m_enable_stencil_test = enable;
		if (m_decal)
		{
			m_decal->setEnableStencilTest(m_enable_stencil_test);
		}
	}

	bool NewDecalComponent::getEnableStencilTest() const
	{
		return m_enable_stencil_test;
	}

	void NewDecalComponent::setStencilTestFunc(ECHO_CMP_FUNC func)
	{
		m_stencil_test_func = func;
		if (m_decal)
		{
			m_decal->setStencilTestFunc(m_stencil_test_func);
		}
	}

	ECHO_CMP_FUNC NewDecalComponent::getStencilTestFunc() const
	{
		return m_stencil_test_func;
	}

	void NewDecalComponent::setStencilTestMask(uint8 mask)
	{
		m_stencil_test_mask = mask;
		if (m_decal)
		{
			m_decal->setStencilTestMask(m_stencil_test_mask);
		}
	}

	uint8 NewDecalComponent::getStencilTestMask() const
	{
		return m_stencil_test_mask;
	}
		  
	void NewDecalComponent::setEnableIllum(bool enable)
	{
		m_enable_illum = enable;
		if (m_decal)
		{
			m_decal->setEnableIllum(m_enable_illum);
		}
	}

	bool NewDecalComponent::getEnableIllum() const
	{
		return m_enable_illum;
	}

	void NewDecalComponent::setEnableWeatherCover(bool enable)
	{
		m_enable_weather_cover = enable;
		if (m_decal)
		{
			m_decal->setEnableWeatherCover(m_enable_weather_cover);
		}
	}

	bool NewDecalComponent::getEnableWeatherCover() const
	{
		return m_enable_weather_cover;
	}

	void NewDecalComponent::setEnableEmissive(bool enable)
	{
		m_emissive_enable = enable;
		if (m_decal)
		{
			m_decal->setEnableEmissive(m_emissive_enable);
		}
	}

	bool NewDecalComponent::getEnableEmissive() const
	{
		return m_emissive_enable;
	}

	void NewDecalComponent::setEmissiveColor(const Vector3& color)
	{
		m_emissive_color = color;
		if (m_decal)
		{
			m_decal->setEmissiveColor(m_emissive_color);
		}
	}

	Vector3 NewDecalComponent::getEmissiveColor()
	{
		return m_emissive_color;
	}

	void NewDecalComponent::setEmissiveStrength(float strength)
	{
		m_emissive_strength = strength;
		if (m_decal)
		{
			m_decal->setEmissiveStrength(m_emissive_strength);
		}
	}

	float NewDecalComponent::getEmissiveStrength()
	{
		return m_emissive_strength;
	}


	bool NewDecalComponent::getResourceReady() const
	{
		return m_resource_ready_mask == m_resourceReady;
	}

	AxisAlignedBox NewDecalComponent::getLoadBoundingBox()
	{
		Vector3 v3Distance = { 1000.0f, 1000.0f, 1000.0f };
		if (m_pActComInfo != nullptr)
		{
			NewDecalComponentInfo* com_info = static_cast<NewDecalComponentInfo*>(m_pActComInfo);
			if (com_info->m_enable_render_distance)
			{
				v3Distance = com_info->m_max_render_distance;
			}
		}
			
		const Vector3& minBox = getWorldAABB().getMinimum();
		const Vector3& maxBox = getWorldAABB().getMaximum();
		AxisAlignedBox loadBox(minBox - v3Distance, maxBox + v3Distance);
		return loadBox;
	}

	void NewDecalComponent::updateCombinedName()
	{
		if (m_helper_entity)
		{
			m_helper_entity->setName(Name(getCombinedName() + "||" + "newdecal_shphere")); 
		}
	}

	void NewDecalComponent::showEditorHelper(bool isShow)
	{
		if (m_helper_object)
		{
			m_helper_object->setVisible(isShow);
		}

		if (m_helper_entity)
		{
			m_helper_entity->setVisible(isShow);
		}
	}

	void NewDecalComponent::setRenderPriority(const Echo::int32& priority)
	{
		m_render_priority = priority;
		if (m_decal)
		{
			m_decal->setRenderPriority(m_render_priority);
		}
	}

	const Echo::int32& NewDecalComponent::getRenderPriority() const
	{
		return m_render_priority;
	}


	void NewDecalComponent::setEnableRenderDistance(bool enable)
	{
		m_enable_render_distance = enable;
		if (m_decal)
		{
			m_decal->setEnableRenderDistance(m_enable_render_distance);
		}
	}

	bool NewDecalComponent::getEnableRenderDistance() const
	{
		return m_enable_render_distance;
	}

	void NewDecalComponent::setMaxRenderDistance(float distance)
	{
		m_max_render_distance = distance;
		if (m_decal)
		{
			m_decal->setMaxRenderDistance(m_max_render_distance);
		}
	}

	float NewDecalComponent::getMaxRenderDistance() const
	{
		return m_max_render_distance;
	}

	void NewDecalComponent::createComRootNodeImpl()
	{
		SceneManager* sMgr = getSceneManager();
		if (sMgr)
		{
			mComRootNode = sMgr->createSceneNode();
		}
	}

	//====================================== NewDecalComponentInfo =======================================
	NewDecalComponentInfo::NewDecalComponentInfo(const Name& comType, ActorCOMMoveableType moveType) :
		ActorComponentInfo(comType, moveType)
	{
	
	}

#ifdef ECHO_EDITOR
	bool NewDecalComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		bool check = false;

		if (this->m_diffuse_texture_path.empty() == false)
		{
			resMap.insert(std::make_pair(ResourceType::NeedCopyRes, this->m_diffuse_texture_path));
			check = true;
		}

		if (this->m_normal_texture_path.empty() == false)
		{
			resMap.insert(std::make_pair(ResourceType::NeedCopyRes, this->m_normal_texture_path));
			check = true;
		}
		return check;
	}
#endif

	EDITOR_REFLECT_BEGIN(NewDecalComponentInfo)	
	EDITOR_REFLECT_MEMBER_UPDATE(m_render_priority, "RenderPriority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_enable_render_distance, "EnableRenderDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_max_render_distance, "MaxRenderDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

	EDITOR_REFLECT_MEMBER_UPDATE(m_enable_stencil_test, "EnableStencilTest", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_stencil_test_func, "StencilTestFunc", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_stencil_test_mask, "StencilTestMask", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	//EDITOR_REFLECT_MEMBER_UPDATE(m_stencil_test_ref, "StencilTestRef", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

	EDITOR_REFLECT_MEMBER_UPDATE(m_enable_illum, "EnableIllum", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_enable_weather_cover, "EnableWeatherCover", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

	EDITOR_REFLECT_MEMBER_UPDATE(m_diffuse_texture_path, "DiffuseTexturePath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_diffuse_texture_u_wrap, "DiffuseTextureUWrap", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_diffuse_texture_v_wrap, "DiffuseTextureVWrap", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_diffuse_texture_tilling_u, "DiffuseTextureTillingU", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_diffuse_texture_tilling_v, "DiffuseTextureTillingV", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

	EDITOR_REFLECT_MEMBER_UPDATE(m_normal_texture_path, "NormalTexturePath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_normal_texture_enable, "NormalTextureEnable", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_normal_texture_u_wrap, "NormalTextureUWrap", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_normal_texture_v_wrap, "NormalTextureVWrap", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_normal_texture_tilling_u, "NormalTextureTillingU", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_normal_texture_tilling_v, "NormalTextureTillingV", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

	EDITOR_REFLECT_MEMBER_UPDATE(m_emissive_enable, "EnableEmissive", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_emissive_color, "EmissiveColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_emissive_strength, "EmissiveStrength", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

	EDITOR_REFLECT_END()

	_DEFINEINFO_VIRFUN_BASE(NewDecalComponentInfo)
}
