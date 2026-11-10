// --------------------------------------------------------------------------------------
// File:   Echo3DUIImageComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_3DUI_IMAGE_COMPONENT_H__
#define __ECHO_3DUI_IMAGE_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "3DUI/Echo3DUIBaseComponent.h"

#include  "UI/GImage.h"

namespace Echo {

	class Echo3DUIImageComponentInfo;
	// ------------------------------------------------ Echo3DUIImageComponent --------------------------------------------------------------- //
	class _EchoComponentExport Echo3DUIImageComponent : public Echo3DUIBaseComponent {
		DECLAREACTORCOMTYPE_H(Echo3DUIImageComponent)

	public:
        virtual void AfterSetGObject() override;

	public:
		REGISTER_LUA_CLASS_KEY;

	public:
		void applyRate();
	};

	// ------------------------------------------------ Echo3DUIImageComponentInfo ------------------------------------------------------------ //
	class _EchoComponentExport Echo3DUIImageComponentInfo : public Echo3DUIBaseComponentInfo {
	public:
		Echo3DUIImageComponentInfo(const Name& type, ActorCOMMoveableType movable);
		Echo3DUIImageComponentInfo() = default;

	public:
		virtual void AfterExportData(cJSON* jsonNode) override;
		virtual void AfterImportData(cJSON* jsonNode) override;

		virtual void AfterCopyTo(ActorComponentInfo* comInfo) override;
		virtual void AfterOverWrite() override;

	public:
		EDITOR_REFLECT

	public:
		Vector2 getRate() const {
			return mRate;
		}

		void setRate(const Vector2& rate) {
			COMPONENT_INFO_PARAM_CHANGE(Echo3DUIImageComponentInfo, this, mRate, rate);
		}

	private:
		Vector2 mRate = { 0.01f, 0.01f }; ///< [0, 0.3]
	};

	// -------------------------------------------------- Factory ---------------------------------------------------------------------- // 
	DEFINECOMFACTORY(Echo3DUIImageComponent, ALL_MOVEABLE)

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//class Echo3DUIImageComponentScript {
	//public:
	//	REGISTER_LUA_CLASS_KEY;

	//public:
	//	static Echo3DUIImageComponent* GetEcho3DUIImageComponent(ActorPtr  p_actor, const std::string& name);
	//};

} /// namespace Echo

#endif // !__ECHO_3DUI_IMAGE_COMPONENT_H__
