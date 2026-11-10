#pragma once

#ifndef EchoColorFilterComponent_h__
#define EchoColorFilterComponent_h__
#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"
namespace Echo
{
	class ColorFilter;
	class _EchoComponentExport ColorFilterComponentInfo : public ActorComponentInfo
	{
	public:
		ColorFilterComponentInfo() {}
		ColorFilterComponentInfo(const Name& type, ActorCOMMoveableType moveable);
		virtual void ExportData(cJSON* jsonNode) override;
		virtual void ImportData(cJSON* jsonNode) override;
		virtual void CopyTo(ActorComponentInfo* comInfo) override;
		virtual void OverWrite() override;
	public:
		bool m_active = false;
		float m_hue = 0.0f;
		float m_saturation = 0.0f;
		float m_lightness = 0.0f;
		float m_contrast = 1.0f;
		bool m_shade = false;
		bool m_reverseColor = false;
		EDITOR_REFLECT
	};

	struct ColorFilterContext
	{
		bool active = false;
		float hue = 0.0f;
		float saturation = 0.0f;
		float lightness = 0.0f;
		float contrast = 1.0f;
		bool shade = false;
		bool reverseColor = false;
	};

	class _EchoComponentExport ColorFilterComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(ColorFilterComponent)
	public:
		virtual void init(ActorComponentInfo* info) override;
		virtual void copyTo(const ActorComponentPtr&) override;

		void setActive(bool active);
		bool getActive();

		void setProcessActive(bool active);
		bool getProcessActive();

		void setHue(float hue);
		void setSaturation(float saturation);
		void setLightness(float lightness);
		void setContrast(float contrast);
		void setShade(bool shade);
		void setReverseColor(bool reverseColor);
	private:
		void setContext(const ColorFilterContext& context);
		void getColorFilter();

		ColorFilterContext m_context;
		ColorFilter* m_colorFilter;
	};

	DEFINECOMFACTORY(ColorFilterComponent, UNMOVEABLE)
}
#endif // !EchoColorFilterComponent_h__
