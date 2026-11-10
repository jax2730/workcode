#include "EchoComponentStableHeaders.h"
#include "EchoNpcDialogComponent.h"
#include "EchoUI3DTypeDefine.h"
#include "UI/UIPackage.h"
#include "UI/GObject.h"
#include "UI/GRoot.h"
#include "UI/GComponent.h"
#include "Event/UIEventType.h"
#include "Event/EventContext.h"

using namespace echofgui;
namespace Echo
{
	// --------------------------------- Private class --------------------------------- //
	class NpcDialogComponent::Private
	{
	public:
		std::vector<echofgui::Handle> m_Objects;  // Multiple GObjects
		echofgui::Handle m_pRoot;                  // Single GRoot for all items
	};

	// Extract package name from file path
	static std::string GetDialogPackageName(const std::string& filePath)
	{
		int start = filePath.find_last_of('/');
		if (start == std::string::npos) return "";

		int end = filePath.find_last_of('_');
		if (end == std::string::npos) return "";

		return filePath.substr(start + 1, end - start - 1);
	}

	// --------------------------------- NpcDialogComponentInfo --------------------------------- //
	DECLAREACTORCOMTYPE_CPP(NpcDialogComponent)

		EDITOR_REFLECT_BEGIN(NpcDialogComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_type, "DialogType", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_FGUIFilePath, "FGUIFilePath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_ActiveItemIndex, "ActiveItemIndex", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END()

		NpcDialogComponentInfo::NpcDialogComponentInfo(const Name& comType, ActorCOMMoveableType type) : ActorComponentInfo(comType, type)
	{
	}

	std::string NpcDialogComponentInfo::GetFGUIFilePath() const
	{
		return m_FGUIFilePath;
	}

	void NpcDialogComponentInfo::SetFGUIFilePath(const std::string& path)
	{
		COMPONENT_INFO_PARAM_CHANGE(NpcDialogComponentInfo, this, m_FGUIFilePath, path)
	}

	void NpcDialogComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<NpcDialogComponentInfo>::get()->ExportJson(this, jsonNode, "");

		// Export items array
		cJSON* itemsArray = cJSON_CreateArray();
		for (const auto& item : m_Items)
		{
			cJSON* itemNode = cJSON_CreateObject();
			cJSON_AddStringToObject(itemNode, "ItemName", item.m_ItemName.c_str());
			cJSON_AddStringToObject(itemNode, "CloseButtonName", item.m_CloseButtonName.c_str());
			cJSON_AddNumberToObject(itemNode, "PosX", item.m_PosX);
			cJSON_AddNumberToObject(itemNode, "PosY", item.m_PosY);
			cJSON_AddNumberToObject(itemNode, "ScaleX", item.m_ScaleX);
			cJSON_AddNumberToObject(itemNode, "ScaleY", item.m_ScaleY);
			cJSON_AddNumberToObject(itemNode, "Rotation", item.m_Rotation);
			cJSON_AddBoolToObject(itemNode, "Visible", item.m_Visible);
			cJSON_AddBoolToObject(itemNode, "UniformScale", item.m_UniformScale);
			cJSON_AddItemToArray(itemsArray, itemNode);
		}
		cJSON_AddItemToObject(jsonNode, "Items", itemsArray);
	}

	void NpcDialogComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<NpcDialogComponentInfo>::get()->ImportJson(this, jsonNode, "");

		// Import items array
		m_Items.clear();
		cJSON* itemsArray = cJSON_GetObjectItem(jsonNode, "Items");
		if (itemsArray && itemsArray->type == cJSON_Array)
		{
			int arraySize = cJSON_GetArraySize(itemsArray);
			for (int i = 0; i < arraySize; ++i)
			{
				cJSON* itemNode = cJSON_GetArrayItem(itemsArray, i);
				if (!itemNode) continue;

				DialogItemConfig item;
				cJSON* nameNode = cJSON_GetObjectItem(itemNode, "ItemName");
				if (nameNode && nameNode->valuestring) item.m_ItemName = nameNode->valuestring;

				cJSON* closeNameNode = cJSON_GetObjectItem(itemNode, "CloseButtonName");
				if (closeNameNode && closeNameNode->valuestring) item.m_CloseButtonName = closeNameNode->valuestring;

				cJSON* posXNode = cJSON_GetObjectItem(itemNode, "PosX");
				if (posXNode) item.m_PosX = static_cast<float>(posXNode->valuedouble);

				cJSON* posYNode = cJSON_GetObjectItem(itemNode, "PosY");
				if (posYNode) item.m_PosY = static_cast<float>(posYNode->valuedouble);

				cJSON* scaleXNode = cJSON_GetObjectItem(itemNode, "ScaleX");
				if (scaleXNode) item.m_ScaleX = static_cast<float>(scaleXNode->valuedouble);

				cJSON* scaleYNode = cJSON_GetObjectItem(itemNode, "ScaleY");
				if (scaleYNode) item.m_ScaleY = static_cast<float>(scaleYNode->valuedouble);

				cJSON* rotNode = cJSON_GetObjectItem(itemNode, "Rotation");
				if (rotNode) item.m_Rotation = static_cast<float>(rotNode->valuedouble);

				cJSON* visNode = cJSON_GetObjectItem(itemNode, "Visible");
				if (visNode) item.m_Visible = (visNode->type == cJSON_True);

				cJSON* uniformNode = cJSON_GetObjectItem(itemNode, "UniformScale");
				if (uniformNode) item.m_UniformScale = (uniformNode->type == cJSON_True);

				m_Items.push_back(item);
			}
		}
	}

	void NpcDialogComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr) return;

		const auto info = dynamic_cast<NpcDialogComponentInfo*>(comInfo);
		reflect::TypeResolver<NpcDialogComponentInfo>::get()->copyTo(this, info);

		// Copy items
		info->m_Items = m_Items;
	}

	void NpcDialogComponentInfo::Overlay(ActorComponentInfo* srcInfo)
	{
	}

	void NpcDialogComponentInfo::OverWrite()
	{
		if (mOriginComInfo == nullptr) return;

		reflect::TypeResolver<NpcDialogComponentInfo>::get()->overWrite(mOriginComInfo, this);
	}

#ifdef ECHO_EDITOR
	bool NpcDialogComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		if (!m_FGUIFilePath.empty())
		{
			resMap.emplace(ResourceType::NeedCopyRes, m_FGUIFilePath);
		}
		return true;
	}
#endif

	// --------------------------------- NpcDialogComponent --------------------------------- //
	NpcDialogComponent::NpcDialogComponent(ActorPtr pActor, const ActorCOMMoveableType movableType) :
		ActorComponent(std::move(pActor), movableType)
	{
		m_Private = new Private();
	}

	NpcDialogComponent::~NpcDialogComponent()
	{
		SAFE_DELETE(m_Private);
	}

	void NpcDialogComponent::init(ActorComponentInfo* info)
	{
		if (info == nullptr) return;
		m_info = dynamic_cast<NpcDialogComponentInfo*>(info);
		if (!m_info) return;

		// Load all FGUI items
		Reload();

		// Show UI
		if (getGRoot())
		{
			getGRoot()->setVisible(true);
		}
	}

	void NpcDialogComponent::unInit()
	{
		// Release all GObjects
		for (auto& objHandle : m_Private->m_Objects)
		{
			GObject* pObject = objHandle.get<GObject>();
			if (pObject)
			{
				pObject->removeFromParent();
				pObject->release();
			}
		}
		m_Private->m_Objects.clear();

		// Release GRoot
		GRoot* pRoot = m_Private->m_pRoot.get<GRoot>();
		if (pRoot)
		{
			pRoot->removeFromParent();
			pRoot->release();
			m_Private->m_pRoot = nullptr;
		}
	}

	void NpcDialogComponent::Reload()
	{
		if (!m_info) return;

		// Release existing resources
		unInit();

		// Check if we have items to load
		if (m_info->m_Items.empty() || m_info->m_FGUIFilePath.empty())
		{
			return;
		}

		// Get package name from path
		std::string packageName = GetDialogPackageName(m_info->m_FGUIFilePath);
		if (packageName.empty())
		{
			return;
		}

		// Add package
		std::string packagePath = m_info->m_FGUIFilePath.substr(0, m_info->m_FGUIFilePath.find(packageName)) + packageName;
		UIPackage::addPackage(packagePath);

		// Create GRoot
		GRoot* defaultRoot = GRoot::defaultRoot();
		m_Private->m_pRoot = GRoot::create();
		GRoot* pRoot = m_Private->m_pRoot.get<GRoot>();

		// Add to default Root (Screen mode)
		defaultRoot->addChild(pRoot);
		pRoot->setUIStyle(UI2DStyle::UI2D_STYLE_SCREEN);
		pRoot->setWorldMatrix(Echo::DBMatrix4::IDENTITY);
		pRoot->setTouchable(true);  // Enable touch events on GRoot

		// Load all items
		m_Private->m_Objects.resize(m_info->m_Items.size());
		for (size_t i = 0; i < m_info->m_Items.size(); ++i)
		{
			const auto& itemConfig = m_info->m_Items[i];

			GObject* gObject = UIPackage::createObject(packageName, itemConfig.m_ItemName);
			if (gObject)
			{
				m_Private->m_Objects[i] = gObject;
				pRoot->addChild(gObject);
				gObject->setTouchable(true);  // Enable touch on item

				// Apply transform
				ApplyTransform(static_cast<int>(i));

				// Apply visibility
				gObject->setVisible(itemConfig.m_Visible);

				// Bind close button event if configured
				BindCloseButtonEvent(static_cast<int>(i), gObject);
			}
		}

		pRoot->setVisible(true);
	}

	void NpcDialogComponent::ReloadItem(int index)
	{
		if (!m_info || index < 0 || index >= static_cast<int>(m_info->m_Items.size()))
			return;

		GRoot* pRoot = m_Private->m_pRoot.get<GRoot>();
		if (!pRoot) return;

		// Release old object at this index
		if (index < static_cast<int>(m_Private->m_Objects.size()))
		{
			GObject* oldObj = m_Private->m_Objects[index].get<GObject>();
			if (oldObj)
			{
				oldObj->removeFromParent();
				oldObj->release();
			}
		}

		// Get package name
		std::string packageName = GetDialogPackageName(m_info->m_FGUIFilePath);
		if (packageName.empty()) return;

		// Load new object
		const auto& itemConfig = m_info->m_Items[index];
		GObject* gObject = UIPackage::createObject(packageName, itemConfig.m_ItemName);
		if (gObject)
		{
			// Ensure vector is large enough
			if (index >= static_cast<int>(m_Private->m_Objects.size()))
			{
				m_Private->m_Objects.resize(index + 1);
			}

			m_Private->m_Objects[index] = gObject;
			pRoot->addChild(gObject);
			gObject->setTouchable(true);  // Enable touch on item
			ApplyTransform(index);
			gObject->setVisible(itemConfig.m_Visible);

			// Bind close button event if configured
			BindCloseButtonEvent(index, gObject);
		}
	}

	int NpcDialogComponent::GetItemCount() const
	{
		return m_info ? static_cast<int>(m_info->m_Items.size()) : 0;
	}

	void NpcDialogComponent::AddItem(const std::string& itemName)
	{
		if (!m_info) return;

		DialogItemConfig newItem;
		newItem.m_ItemName = itemName;
		newItem.m_Visible = true;
		m_info->m_Items.push_back(newItem);

		// Load this new item
		int newIndex = static_cast<int>(m_info->m_Items.size()) - 1;

		GRoot* pRoot = m_Private->m_pRoot.get<GRoot>();
		if (pRoot && !m_info->m_FGUIFilePath.empty())
		{
			std::string packageName = GetDialogPackageName(m_info->m_FGUIFilePath);
			if (!packageName.empty())
			{
				GObject* gObject = UIPackage::createObject(packageName, itemName);
				if (gObject)
				{
					m_Private->m_Objects.push_back(gObject);
					pRoot->addChild(gObject);
					gObject->setTouchable(true);  // Enable touch on item
					ApplyTransform(newIndex);
					gObject->setVisible(true);

					// Bind close button event if configured
					BindCloseButtonEvent(newIndex, gObject);
				}
			}
		}
	}

	void NpcDialogComponent::RemoveItem(int index)
	{
		if (!m_info || index < 0 || index >= static_cast<int>(m_info->m_Items.size()))
			return;

		// Release the GObject
		if (index < static_cast<int>(m_Private->m_Objects.size()))
		{
			GObject* obj = m_Private->m_Objects[index].get<GObject>();
			if (obj)
			{
				obj->removeFromParent();
				obj->release();
			}
			m_Private->m_Objects.erase(m_Private->m_Objects.begin() + index);
		}

		// Remove from config
		m_info->m_Items.erase(m_info->m_Items.begin() + index);

		// Adjust active index if needed
		if (m_info->m_ActiveItemIndex >= static_cast<int>(m_info->m_Items.size()))
		{
			m_info->m_ActiveItemIndex = m_info->m_Items.empty() ? 0 : static_cast<int>(m_info->m_Items.size()) - 1;
		}
	}

	void NpcDialogComponent::ShowItem(int index)
	{
		if (!m_info || index < 0 || index >= static_cast<int>(m_info->m_Items.size()))
			return;

		m_info->m_Items[index].m_Visible = true;

		if (index < static_cast<int>(m_Private->m_Objects.size()))
		{
			GObject* obj = m_Private->m_Objects[index].get<GObject>();
			if (obj)
			{
				obj->setVisible(true);
			}
		}
	}

	void NpcDialogComponent::HideItem(int index)
	{
		if (!m_info || index < 0 || index >= static_cast<int>(m_info->m_Items.size()))
			return;

		m_info->m_Items[index].m_Visible = false;

		if (index < static_cast<int>(m_Private->m_Objects.size()))
		{
			GObject* obj = m_Private->m_Objects[index].get<GObject>();
			if (obj)
			{
				obj->setVisible(false);
			}
		}
	}

	void NpcDialogComponent::SwitchToItem(int index)
	{
		if (!m_info || index < 0 || index >= static_cast<int>(m_info->m_Items.size()))
			return;

		// Hide all items
		for (size_t i = 0; i < m_info->m_Items.size(); ++i)
		{
			HideItem(static_cast<int>(i));
		}

		// Show only the specified item
		ShowItem(index);
		m_info->m_ActiveItemIndex = index;
	}

	int NpcDialogComponent::GetActiveItemIndex() const
	{
		return m_info ? m_info->m_ActiveItemIndex : -1;
	}

	void NpcDialogComponent::ApplyTransform(int index)
	{
		if (!m_info || index < 0 || index >= static_cast<int>(m_info->m_Items.size()))
			return;

		if (index >= static_cast<int>(m_Private->m_Objects.size()))
			return;

		GObject* pObject = m_Private->m_Objects[index].get<GObject>();
		if (!pObject) return;

		const auto& itemConfig = m_info->m_Items[index];
		pObject->setPosition(itemConfig.m_PosX, itemConfig.m_PosY);
		pObject->setScale(itemConfig.m_ScaleX, itemConfig.m_ScaleY);
		pObject->setRotation(itemConfig.m_Rotation);
	}

	void NpcDialogComponent::ApplyAllTransforms()
	{
		if (!m_info) return;

		for (size_t i = 0; i < m_info->m_Items.size(); ++i)
		{
			ApplyTransform(static_cast<int>(i));
		}
	}

	echofgui::GObject* NpcDialogComponent::getGObject(int index) const
	{
		if (!m_Private || index < 0 || index >= static_cast<int>(m_Private->m_Objects.size()))
			return nullptr;

		return m_Private->m_Objects[index].get<GObject>();
	}

	echofgui::GRoot* NpcDialogComponent::getGRoot() const
	{
		return m_Private ? m_Private->m_pRoot.get<GRoot>() : nullptr;
	}

	echofgui::GObject* NpcDialogComponent::GetCloseButton(int index) const
	{
		if (!m_info || index < 0 || index >= static_cast<int>(m_info->m_Items.size()))
			return nullptr;

		const auto& itemConfig = m_info->m_Items[index];
		if (itemConfig.m_CloseButtonName.empty())
			return nullptr;

		GObject* gObject = getGObject(index);
		if (!gObject)
			return nullptr;

		GComponent* gComp = gObject->as<GComponent>();
		if (!gComp)
			return nullptr;

		return gComp->getChild(itemConfig.m_CloseButtonName);
	}

	bool NpcDialogComponent::HitTestCloseButton(int index, float screenX, float screenY) const
	{
		GObject* closeBtn = GetCloseButton(index);
		if (!closeBtn)
			return false;

		// Get button's position and size using correct FGUI API
		//echofgui::Vec3 btnPos = closeBtn->getPosition();
		float btnWidth = closeBtn->getWidth();
		float btnHeight = closeBtn->getHeight();

		// Create a rect in local space and convert to global screen space
		echofgui::Rect localRect = { {0.f, 0.f}, {btnWidth, btnHeight} };
		echofgui::Rect globalRect = closeBtn->localToGlobal(localRect);

		// Check if screen point is inside the button
		return screenX >= globalRect.x &&
			screenX <= globalRect.x + globalRect.w &&
			screenY >= globalRect.y &&
			screenY <= globalRect.y + globalRect.h;
	}

	echofgui::GObject* NpcDialogComponent::LoadGObjectFromPackage(const std::string& packagePath, const std::string& itemName)
	{
		if (packagePath.empty() || itemName.empty())
			return nullptr;

		std::string packageName = GetDialogPackageName(packagePath);
		if (packageName.empty())
			return nullptr;

		UIPackage::addPackage(packagePath.substr(0, packagePath.find(packageName)) + packageName);
		return UIPackage::createObject(packageName, itemName);
	}

	void NpcDialogComponent::BindCloseButtonEvent(int index, echofgui::GObject* gObject)
	{
		if (!m_info || index < 0 || index >= static_cast<int>(m_info->m_Items.size()))
			return;

		auto& itemConfig = m_info->m_Items[index];

		// Default close button name is "n1" if not specified
		std::string closeButtonName = itemConfig.m_CloseButtonName;
		if (closeButtonName.empty())
		{
			closeButtonName = "n1";  // Default close button name
			itemConfig.m_CloseButtonName = closeButtonName;
		}

		// Try to get close button as child of gObject
		GComponent* gComp = gObject->as<GComponent>();
		if (!gComp)
		{
			LogManager::instance()->logMessage("[NpcDialog] ERROR: gObject is not a GComponent!");
			return;
		}

		GObject* closeBtn = gComp->getChild(closeButtonName);
		if (!closeBtn)
		{
			LogManager::instance()->logMessage("[NpcDialog] ERROR: Close button '" + closeButtonName + "' not found!");
			return;
		}

		LogManager::instance()->logMessage("[NpcDialog] Binding close button '" + closeButtonName + "' for item " + std::to_string(index));

		// Enable touch on button and set proper sorting order
		closeBtn->setTouchable(true);
		closeBtn->setSortingOrder(100);  // High sorting order to ensure it receives events

		// Remove existing listener first to avoid duplicates
		closeBtn->removeEventListener(UIEventType::TouchBegin, EventTag(closeBtn));

		// Add click event listener
		closeBtn->addEventListener(
			UIEventType::TouchBegin,
			std::bind(&NpcDialogComponent::OnCloseButtonClicked, this, index, std::placeholders::_1),
			EventTag(closeBtn)
		);

		LogManager::instance()->logMessage("[NpcDialog] Close button event bound successfully!");
	}

	void NpcDialogComponent::OnCloseButtonClicked(int index, echofgui::EventContext* context)
	{
		LogManager::instance()->logMessage("[NpcDialog] Close button clicked! Hiding item " + std::to_string(index));
		// Hide the item when close button is clicked
		HideItem(index);
	}

	void NpcDialogComponent::copyTo(const ActorComponentPtr&)
	{
	}

	void NpcDialogComponent::connectCom()
	{
		if (m_pActorPtr && m_pActorPtr->getActorType() == ActorType::ActorType_Logic)
		{
			m_pActorPtr->notifyLogicActor(static_cast<LogicType>(m_info->m_type), true);
		}
	}

	void NpcDialogComponent::disconnectCom()
	{
		if (m_pActorPtr && m_pActorPtr->getActorType() == ActorType::ActorType_Logic)
		{
			m_pActorPtr->notifyLogicActor(static_cast<LogicType>(m_info->m_type), false);
		}
	}
}
