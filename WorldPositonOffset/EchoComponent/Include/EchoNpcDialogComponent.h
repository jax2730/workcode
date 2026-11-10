#ifndef _EchoNpcDialogComponent_H
#define _EchoNpcDialogComponent_H

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoMovableObject.h"

#include "EchoHelperObj.h"
#include "EchoComDefineMacro.h"
#include <vector>

namespace echofgui
{
	class GObject;
	class Handle;
	class GRoot;
	class EventContext;
}

namespace Echo
{
	// Single dialog item configuration
	struct _EchoComponentExport DialogItemConfig
	{
		std::string m_ItemName;      // FGUI object name in package
		std::string m_CloseButtonName;  // Child button name for close/hide action
		float m_PosX = 0.0f;
		float m_PosY = 0.0f;
		float m_ScaleX = 1.0f;
		float m_ScaleY = 1.0f;
		float m_Rotation = 0.0f;
		bool m_Visible = true;
		bool m_UniformScale = true;  // Lock scale X/Y to same value
	};

	class _EchoComponentExport NpcDialogComponentInfo : public ActorComponentInfo
	{
	public:
		NpcDialogComponentInfo() = default;
		NpcDialogComponentInfo(const NpcDialogComponentInfo&) = delete;
		NpcDialogComponentInfo& operator = (const NpcDialogComponentInfo&) = delete;
		NpcDialogComponentInfo(const Name& comType, ActorCOMMoveableType type);
		~NpcDialogComponentInfo() override = default;
		void ExportData(cJSON* jsonNode) override;
		void ImportData(cJSON* jsonNode) override;
		void CopyTo(ActorComponentInfo* comInfo) override;
		void Overlay(ActorComponentInfo* srcInfo) override;
		void OverWrite() override;

		int m_type = 2001;

		// FGUI package path
		std::string GetFGUIFilePath() const;
		void SetFGUIFilePath(const std::string& path);
		std::string m_FGUIFilePath;

		// Multiple items support
		std::vector<DialogItemConfig> m_Items;
		int m_ActiveItemIndex = 0;  // Currently active/visible item index

#ifdef ECHO_EDITOR
		bool getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif

		EDITOR_REFLECT
	};

	class _EchoComponentExport NpcDialogComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(NpcDialogComponent)

	public:
		void init(ActorComponentInfo* info) override;
		void unInit() override;
		void copyTo(const ActorComponentPtr&) override;
		void connectCom() override;
		void disconnectCom() override;

		// FGUI loading
		void Reload();
		void ReloadItem(int index);

		// Item management
		int GetItemCount() const;
		void AddItem(const std::string& itemName);
		void RemoveItem(int index);

		// Item switching (for interaction)
		void ShowItem(int index);
		void HideItem(int index);
		void SwitchToItem(int index);  // Hide all, show only this one
		int GetActiveItemIndex() const;

		// Transform control per item
		void ApplyTransform(int index);
		void ApplyAllTransforms();

		// Access
		echofgui::GObject* getGObject(int index = 0) const;
		echofgui::GRoot* getGRoot() const;

		// Screen space hit test for close button (for Editor mode)
		bool HitTestCloseButton(int index, float screenX, float screenY) const;
		echofgui::GObject* GetCloseButton(int index) const;

	private:
		echofgui::GObject* LoadGObjectFromPackage(const std::string& packagePath, const std::string& itemName);

		// Button event binding helper
		void BindCloseButtonEvent(int index, echofgui::GObject* gObject);
		void OnCloseButtonClicked(int index, echofgui::EventContext* context);

	protected:
		NpcDialogComponentInfo* m_info = nullptr;

	private:
		class Private;
		Private* m_Private = nullptr;
	};

	DEFINECOMFACTORY(NpcDialogComponent, UNMOVEABLE)
}

#endif
