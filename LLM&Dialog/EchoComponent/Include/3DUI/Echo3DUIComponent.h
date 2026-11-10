// --------------------------------------------------------------------------------------
// File:   Echo3DUIComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#ifndef __ECHO_3DUI_COMPONENT_H__
#define __ECHO_3DUI_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"
#include "Actor/EchoProtocolActorComponent.h"
#include "EchoUI3DTypeDefine.h"

namespace echofgui
{
    class GObject;
	class Handle;
	class GRoot;
	enum class UI3dScalerMode :uint32_t;
}

namespace Echo
{
	class UI3DMovable;
	class Echo3DUIComponentInfo;
    class Echo3DUIBaseComponent;

	class _EchoComponentExport Echo3DUIComponent : public NodeActorComponent
    {
		DECLAREACTORCOMTYPE_H(Echo3DUIComponent)

public:
		virtual void init(ActorComponentInfo*  p_info) override;
        virtual void unInit() override;
		virtual void bindNode() override;
		virtual void copyTo(const ActorComponentPtr& actCom) override;
		virtual void connectCom() override;
		//virtual void registerTickTimeBegin() override;

        void		setTouchable(bool b);

protected:
    virtual void	createComRootNodeImpl() override;

private:
       echofgui::GObject*	LoadGObjectFromFile(ActorComponentInfo* actorComInfo);
       echofgui::GObject*	LoadGObjectFromFile(const Echo::Name& path, const Echo::Name& item_path);
public:
		// 在点击生成3DUI组件时调用
		void		Reload();
        void		Reload(const Echo::Name& pack_name, const Echo::Name& item_name, UI2DStyle type);
		// 在切换显示风格时调用
        //IMPORTANT(yanghang):Internal only use for editor mode.
        // 1. It's extremly heavy operation.
        // 2. Free the FGUI node tree and memory then rebuild it. so all it's state be erased.
        // 3. If want to run-time switch 3DUI Type, try to use changeScene3DUIStyle(); 
		void		changeUI2DStyle(UI2DStyle style);
        void		setScaleType(echofgui::UI3dScalerMode type);

public:
        echofgui::GObject*	getGObject() const;
        echofgui::GRoot*		getGRoot() const;
		void		updateAABB(bool _isScale = true);
		/**
		 * @brief Set the transition's framerate of the 3d ui compoennt
		 * @param frameRate 
		 */
		void		setTransitionFramerate(float frameRate);
		void		stopTransitions();
		// LOD 有一种是加载静态页面
		// 只提供一个静态贴图就行，本来想存储一个额外的选项，但是会占空间，而且绝大多数不需要
		void		staticImageMode(char const* imgUrl);

private:
        
		class Private;
		Private* m_Private;
		UI3DMovable* m_movable;
        bool m_bTouchDisable;
	};

	// ------------------------------------------------ Echo3DUIComponentInfo ------------------------------------------------------------ //
	class _EchoComponentExport Echo3DUIComponentInfo : public ActorComponentInfo {
	public:
		Echo3DUIComponentInfo() = default;
		Echo3DUIComponentInfo(const Name& type, ActorCOMMoveableType movable);

		void ExportData(cJSON* p_json) override;
		void ImportData(cJSON* p_json) override;
		void CopyTo(ActorComponentInfo* p_actor_com_info) override;
		void OverWrite() override;

	public:
		inline UI2DStyle getUI2DStyle() const { return static_cast<UI2DStyle>(this->m_UI2DStyle); }
		inline void setUI2DStyle(UI2DStyle type) { SetUI2DStyle(static_cast<int>(type)); }

		EDITOR_REFLECT

	public:
		int GetUI2DStyle() const { return m_UI2DStyle; }
		void SetUI2DStyle(int style)  { COMPONENT_INFO_PARAM_CHANGE(Echo3DUIComponentInfo, this, m_UI2DStyle, style) }

		std::string GetFGUIFilePath() const { return m_FGUIFilePath; }
		void SetFGUIFilePath(const std::string& path) { COMPONENT_INFO_PARAM_CHANGE(Echo3DUIComponentInfo, this, m_FGUIFilePath, path) }

		std::string GetPackageItemName() const { return m_PackageItemName; }
		void SetPackageItemName(const std::string& itemName) { COMPONENT_INFO_PARAM_CHANGE(Echo3DUIComponentInfo, this, m_PackageItemName, itemName) }
#ifdef ECHO_EDITOR
		bool getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif
	private:
		int m_UI2DStyle{ UI2DStyle::UI2D_STYLE_SCENE };

		std::string m_FGUIFilePath;
		std::string m_PackageItemName;
	};
    
    std::string GetPackageName(const std::string& filePath);
    
	// -------------------------------------------------- Factory ---------------------------------------------------------------------- // 
	DEFINECOMFACTORY(Echo3DUIComponent, ALL_MOVEABLE)


    //IMPORTANT(yanghang):Use for 3D UI component to chase or orient a direction.
    // Screen 3D UI chasing the point in camera view space.
    // Secne 3D UI chasing the point in world absolute space.
    // return the new direction.
    // chaseSpeedX|Y: Chase angle speed in X|Y axis.
    //IMPORTANT(yanghang): Can be used for other usage.
    Quaternion ChaseDirectionInXYAxis(const Vector3&    chaseDir,
                                      const Quaternion& currentOrientation,
                                      float             chaseAngleSpeedX,
                                      float             chaseAngleSpeedY,
                                      float             maxLookUpAngle);

    Quaternion OrientDirection(const Vector3&    orientDir,
                               const Quaternion& currentOrientation,
                               float             maxLookUpAngle);
} /// namespace Echo

#endif // !__ECHO_3DUI_COMPONENT_H__

