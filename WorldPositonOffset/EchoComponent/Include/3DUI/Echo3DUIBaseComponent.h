// --------------------------------------------------------------------------------------
// File:   Echo3DUIBaseComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_3DUI_BASE_COMPONENT_H__
#define __ECHO_3DUI_BASE_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "Actor/EchoProtocolActorComponent.h"

#include "UI/GObject.h"
#include "UI/GComponent.h"

#include "EchocJsonHelper.h"
#include "EchoHelperObj.h"
#include "Actor/EchoActorManager.h"

namespace Echo {

	USING_NS_FGUI;

	class Echo3DUIBaseComponentInfo;

	class _EchoComponentExport Echo3DUIBaseComponent : public ProtocolNodeActorComponent {
	public:
        //NOTE(yanghang):Bit flag.
        enum SubUI3DComponentType
            {
                UI3D_COMPONENT_UNKONWN_TYPE = 0x00000,
                UI3D_NODECOMPONENT = 0x00001,
                UI3D_BUTTONCOMPONENT = 0x00002,
                UI3D_COMBOBOXCOMPONENT = 0x00004,
                UI3D_LABELCOMPONENT = 0x00008,
                UI3D_LISTCOMPONENT = 0x00010,
                UI3D_PROGRESSBARCOMPONENT = 0x00020,
                UI3D_SCROLLBARCOMPONENT = 0x00040,
                UI3D_SLIDERCOMPONENT = 0x00080,
                UI3D_TREEVIEWCOMPONENT = 0x0100,
            
                UI3D_GROUPCOMPONENT = 0x00200,
                UI3D_GRAPHCOMPONENT = 0x00400,
                UI3D_IMAGECOMPONENT = 0x00800,
                UI3D_INPUTTEXTCOMPONENT = 0x01000,
                UI3D_MOVIECLIPCOMPONENT = 0x02000,
                UI3D_RICHTEXTCOMPONENT = 0x04000,
                UI3D_SWFCOMPONENT = 0x08000,
                UI3D_TEXTCOMPONENT = 0x10000,
                UI3D_LOADERCOMPONENT = 0x20000,
            
                //NOTE(yanghang):To FGUI GComponent sub-class type.
                FGUI_GCOMPONENT_TYPE = UI3D_NODECOMPONENT |
                UI3D_BUTTONCOMPONENT |
                UI3D_COMBOBOXCOMPONENT |
                UI3D_LABELCOMPONENT |
                UI3D_LISTCOMPONENT | 
                UI3D_PROGRESSBARCOMPONENT |
                UI3D_SCROLLBARCOMPONENT |
                UI3D_SLIDERCOMPONENT |
                UI3D_TREEVIEWCOMPONENT,

                FGUI_GOBJECT_TYPE = UI3D_GROUPCOMPONENT |
                UI3D_GRAPHCOMPONENT |
                UI3D_IMAGECOMPONENT |
                UI3D_INPUTTEXTCOMPONENT |
                UI3D_MOVIECLIPCOMPONENT |
                UI3D_RICHTEXTCOMPONENT |
                UI3D_SWFCOMPONENT |
                UI3D_TEXTCOMPONENT |
                UI3D_LOADERCOMPONENT,
            };

		enum UI3D_Object_Type
		{
			UI3D_Object_UNKONWN_TYPE = 0x00000,
			UI3D_Object_NODE = 0x1 << uint16(ObjectType::COMPONENT),
			UI3D_Object_BUTTON = 0x1 << uint16(ObjectType::BUTTON),
			UI3D_Object_COMBOBOX = 0x1 << uint16(ObjectType::COMBOBOX),
			UI3D_Object_LABEL = 0x1 << uint16(ObjectType::LABEL),
			UI3D_Object_LIST = 0x1 << uint16(ObjectType::LIST),
			UI3D_Object_PROGRESSBAR = 0x1 << uint16(ObjectType::PROGRESSBAR),
			UI3D_Object_SCROLLBAR = 0x1 << uint16(ObjectType::SCROLLBAR),
			UI3D_Object_SLIDER = 0x1 << uint16(ObjectType::SLIDER),
			UI3D_Object_TREEVIEW = 0x1 << uint16(ObjectType::TREEVIEW),

			UI3D_Object_GROUP = 0x1 << uint16(ObjectType::GROUP),
			UI3D_Object_GRAPH = 0x1 << uint16(ObjectType::GRAPH),
			UI3D_Object_IMAGE = 0x1 << uint16(ObjectType::IMAGE),
			UI3D_Object_INPUTTEXT = 0x1 << uint16(ObjectType::INPUTTEXT),
			UI3D_Object_MOVIECLIP = 0x1 << uint16(ObjectType::MOVIECLIP),
			UI3D_Object_RICHTEXT = 0x1 << uint16(ObjectType::RICHTEXT),
			UI3D_Object_SWF = 0x1 << uint16(ObjectType::SWF),
			UI3D_Object_TEXT = 0x1 << uint16(ObjectType::TEXT),
			UI3D_Object_LOADER = 0x1 << uint16(ObjectType::LOADER),

			//NOTE(yanghang):To FGUI G sub-class type.
			UI3D_GCOMPONENT_TYPE = UI3D_Object_NODE |
			UI3D_Object_BUTTON |
			UI3D_Object_COMBOBOX |
			UI3D_Object_LABEL |
			UI3D_Object_LIST |
			UI3D_Object_PROGRESSBAR |
			UI3D_Object_SCROLLBAR |
			UI3D_Object_SLIDER |
			UI3D_Object_TREEVIEW,

			UI3D_GOBJECT_TYPE = UI3D_Object_GROUP |
			UI3D_Object_GRAPH |
			UI3D_Object_IMAGE |
			UI3D_Object_INPUTTEXT |
			UI3D_Object_MOVIECLIP |
			UI3D_Object_RICHTEXT |
			UI3D_Object_SWF |
			UI3D_Object_TEXT |
			UI3D_Object_LOADER,
		};
        
		REGISTER_LUA_CLASS_KEY;
		explicit Echo3DUIBaseComponent(ActorPtr   actor, ActorCOMMoveableType movableType);
		virtual ~Echo3DUIBaseComponent();

	public:
		virtual void init(ActorComponentInfo* actorComInfo) override;
        virtual void unInit() override;

	public:
		virtual void registerTickTimeBegin() override;

		virtual void beginFrame(EventID, ProtocolHandle) override;
		virtual void endFrame(EventID, ProtocolHandle) override {}

		virtual void preUpdateAnimation(EventID, ProtocolHandle) override {}
		virtual void postUpdateAnimation(EventID, ProtocolHandle) override {}

		virtual void preUpdateScene(EventID, ProtocolHandle) override {}
		virtual void postUpdateScene(EventID, ProtocolHandle) override {}

		virtual void preRenderScene(EventID, ProtocolHandle) override {}
		virtual void postRenderScene(EventID, ProtocolHandle) override {}

	public:
		void registerProtocolCom();
		void unregisterProtocolCom();

	public: 
		GObject* GetGObject() const { return this->mGObject; }
		virtual void SetGObject(RenderPriorityLookupIndex lookupIndex, GObject* gObject);

        //IMPORTANT(yanghang):Free FGUI reference of component.
        void releaseGObject();


	public:
		/// --------------------------------- In order for script to call. ------------------------------------------ ///
		// Set the current status of the controller.
		void SetControllerPage(int controller_index, int page);

        //IMPORTANT(yanghang):Use top-level 3DUI-Component pointer as the lookup index.
        // Must maintain the lookup index validition.
        void setRenderPriorityLookupIndex(RenderPriorityLookupIndex lookupIndex);
        RenderPriorityLookupIndex getRenderPriorityLookupIndex(){return  m_priorityLookupIndex;}
        SubUI3DComponentType get3DUIComponentType();
        
        //void setPivot(Vector3 pivot);
        //void freshPivot();

        void changeScene3DUIStyle(UI2DStyle newStyle);

		const ActorComponentPtr& getCreatorComponent() const { return m_pCreatorComponent; }
		void				setCreatorComponent(const ActorComponentPtr& creator);
protected:
        //IMPORTANT(yanghang):Sub-component type that maybe have one or more RenderBase like [Image, Text],need to override
        // this func.
		virtual void AfterSetGObject() {}

        virtual void createComRootNodeImpl() override;

        //TODO(yanghang):Optimize InputText of FGUI can remove those virtual function.
        virtual void afterChangeScene3DUIStyle(UI2DStyle newStyle){}

        //IMPORTANT(yanghang):Internal use only. Don't call it.
        // Sub-class Free reference to FGUI engine render-object.
        virtual void PostReleaseGObject() {};
protected:
		GObject* mGObject = nullptr;

		//IMPORTANT(yanghang):FGUI widget that will be actually rendered,
		// like Image, Text and so on. m_dummyMovable point to UI2DRenderBase Object.
		UI3DDummyMovable* m_dummyMovable = nullptr;

        //NOTE:Cached the index for dynamic create child-component.
        RenderPriorityLookupIndex m_priorityLookupIndex;

        SubUI3DComponentType m_UI3DType;

		ActorComponentPtr			m_pCreatorComponent;

	public:
		// 调试球相关
		inline UI2DStyle getUI2DStyle() const { return this->mUI2DStyle; }
		inline void setUI2DStyle(UI2DStyle style) { this->mUI2DStyle = style; }

		inline const ColorValue& getDebugSphereColor() const { return this->mDebugSphereColor; }
		inline void setDebugSphereColor(const ColorValue& color) { this->mDebugSphereColor = color; }

		inline float getDebugSphereRadius() const { return this->mDebugSphereRadius; }
		inline void setDebugSphereRadius(float radius) { 
			this->mDebugSphereRadius = radius; 
			if (mHelperObject) {
				mHelperObject->setScale({ mDebugSphereRadius, mDebugSphereRadius, mDebugSphereRadius });
			}  
		}

		// 重置调试球属性。
		void resetDebugSphere();

		void setDebugSpherePosition(const Vector3& postition) {
			if (mHelperObject) {
				mHelperObject->setPosition(postition);
			}
		}

		void setDebugSphereIsVisible(bool val) {
			if (mHelperObject) {
				mHelperObject->setVisible(val);
			}
		}

	protected: // 调试球

		UI2DStyle mUI2DStyle = UI2DStyle::UI2D_STYLE_SCENE;
		ColorValue mDebugSphereColor = ColorValue::Green;
		float mDebugSphereRadius = 0.01f;

		HelperObject* mHelperObject{ nullptr };
	};

	class _EchoComponentExport Echo3DUIBaseComponentInfo : public ActorComponentInfo {

	public:
		Echo3DUIBaseComponentInfo() = default;
		explicit Echo3DUIBaseComponentInfo(const Name& type, ActorCOMMoveableType movable);
		virtual ~Echo3DUIBaseComponentInfo() {}

		Echo3DUIBaseComponentInfo(const Echo3DUIBaseComponentInfo& echo3DUIBaseComponentInfo) = delete;
		Echo3DUIBaseComponentInfo& operator=(const Echo3DUIBaseComponentInfo& other) = delete;

	public:
		virtual void ExportData(cJSON* jsonNode) override;
		virtual void ImportData(cJSON* jsonNode) override;

		virtual void CopyTo(ActorComponentInfo* comInfo) override;
		virtual void OverWrite() override;

	protected:
		virtual void AfterExportData(cJSON* jsonNode) {}
		virtual void AfterImportData(cJSON* jsonNode) {}

		virtual void AfterCopyTo(ActorComponentInfo* comInfo) {}
		virtual	void AfterOverWrite() {}

	public:
		EDITOR_REFLECT

	public:
		inline std::string GetUniqueID() const {
			return m_UniqueID;
		}

		void SetUniqueID(const std::string& id) {
			COMPONENT_INFO_PARAM_CHANGE(Echo3DUIBaseComponentInfo, this, m_UniqueID, id)
		}

		inline Vector3 GetDebugSpherePosition() const {
			return m_DebugSpherePosition;
		}

		void SetDebugSpherePosition(const Vector3& pos, Echo3DUIBaseComponent* ui_com) {
			COMPONENT_PARAM_CHANGE(Echo3DUIBaseComponent, ui_com, Echo3DUIBaseComponentInfo, this, m_DebugSpherePosition, pos, setDebugSpherePosition)
		}

		inline bool GetDebugSphereIsVisible() const {
			return m_DebugSphereIsVisible;
		}

		void SetDebugSphereIsVisible(const bool& visible, Echo3DUIBaseComponent* ui_com) {
			COMPONENT_PARAM_CHANGE(Echo3DUIBaseComponent, ui_com, Echo3DUIBaseComponentInfo, this, m_DebugSphereIsVisible, visible, setDebugSphereIsVisible)
		}

	protected:
		std::string m_UniqueID{};

	protected:
		Vector3 m_DebugSpherePosition{ Vector3::ZERO };
		bool m_DebugSphereIsVisible{ false };
	};

} /// namespace Echo

#endif /// __ECHO_3DUI_BASE_COMPONENT_H__
