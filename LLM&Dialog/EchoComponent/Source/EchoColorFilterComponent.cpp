
#include "EchoComponentStableHeaders.h"
#include "EchoColorFilterComponent.h"
#include "EchoColorFilter.h"

namespace Echo
{
	EDITOR_REFLECT_BEGIN(ColorFilterComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_active, "active", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_hue, "hue", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_saturation, "saturation", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_lightness, "lightness", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_contrast, "contrast", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_shade, "shade", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_reverseColor, "reverseColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()

	ColorFilterComponentInfo::ColorFilterComponentInfo(const Name& type, ActorCOMMoveableType moveable)
		: ActorComponentInfo(type, moveable)
	{}

	void ColorFilterComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<ColorFilterComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void ColorFilterComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<ColorFilterComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void ColorFilterComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		ColorFilterComponentInfo* info = static_cast<ColorFilterComponentInfo*>(comInfo);
		reflect::TypeResolver<ColorFilterComponentInfo>::get()->copyTo(this, info);
	}

	void ColorFilterComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			ColorFilterComponentInfo* oriInfo = static_cast<ColorFilterComponentInfo*> (mOriginComInfo);
			reflect::TypeResolver<ColorFilterComponentInfo>::get()->overWrite(oriInfo, this);
		}
	}

	std::set<ColorFilterComponent*> g_setColorFilterComs;
	static const ColorFilterContext s_DefaultContext;

	DECLAREACTORCOMTYPE_CPP(ColorFilterComponent)
	ColorFilterComponent::ColorFilterComponent(ActorPtr _actor, ActorCOMMoveableType movableType)
		: ActorComponent(_actor, movableType)
		, m_colorFilter(nullptr)
	{
		g_setColorFilterComs.insert(this);
		if (!getProcessActive())
		{
			setProcessActive(true);
		}
	}

	ColorFilterComponent::~ColorFilterComponent()
	{
		setContext(s_DefaultContext);
		g_setColorFilterComs.erase(this);
		if (g_setColorFilterComs.empty())
		{
			setProcessActive(false);
		}
	}

	void ColorFilterComponent::init(ActorComponentInfo* info)
	{
		ColorFilterComponentInfo* pInfo = static_cast<ColorFilterComponentInfo*>(info);
		if (pInfo == nullptr)
		{
			return;
		}

		m_context.active = pInfo->m_active;
		m_context.hue = pInfo->m_hue;
		m_context.saturation = pInfo->m_saturation;
		m_context.lightness = pInfo->m_lightness;
		m_context.contrast = pInfo->m_contrast;
		m_context.shade = pInfo->m_shade;
		m_context.reverseColor = pInfo->m_reverseColor;

		if (m_context.active)
		{
			setContext(m_context);
		}
	}

	void ColorFilterComponent::copyTo(const ActorComponentPtr&)
	{
	}

	void ColorFilterComponent::setActive(bool active)
	{
		if (active)
		{
			for (auto p : g_setColorFilterComs)
			{
				p->m_context.active = false;
			}
			setContext(m_context);
		}
		else
		{
			for (auto p : g_setColorFilterComs)
			{
				p->m_context.active = false;
			}
			setContext(s_DefaultContext);
		}
		m_context.active = active;
	}

	bool ColorFilterComponent::getActive()
	{
		return m_context.active;
	}

	void ColorFilterComponent::setProcessActive(bool active)
	{
		Root::instance()->getRenderSystem()->getCurRenderStrategy()->getPostProcessChain()->setEnable(PostProcess::Type::ColorFilter, active);
	}

	bool ColorFilterComponent::getProcessActive()
	{
		return Root::instance()->getRenderSystem()->getCurRenderStrategy()->getPostProcessChain()->isEnable(PostProcess::Type::ColorFilter);
	}

	void ColorFilterComponent::setHue(float hue)
	{
		m_context.hue = hue;
		if (m_context.active)
		{
			setContext(m_context);
		}
	}

	void ColorFilterComponent::setSaturation(float saturation)
	{
		m_context.saturation = saturation;
		if (m_context.active)
		{
			setContext(m_context);
		}
	}

	void ColorFilterComponent::setLightness(float lightness)
	{
		m_context.lightness = lightness;
		if (m_context.active)
		{
			setContext(m_context);
		}
	}

	void ColorFilterComponent::setContrast(float contrast)
	{
		m_context.contrast = contrast;
		if (m_context.active)
		{
			setContext(m_context);
		}
	}

	void ColorFilterComponent::setShade(bool shade)
	{
		m_context.shade = shade;
		if (m_context.active)
		{
			setContext(m_context);
		}
	}

	void ColorFilterComponent::setReverseColor(bool reverseColor)
	{
		m_context.reverseColor = reverseColor;
		if (m_context.active)
		{
			setContext(m_context);
		}
	}

	void ColorFilterComponent::setContext(const ColorFilterContext& context)
	{
		if (!m_colorFilter)
		{
			getColorFilter();
		}

		if (m_colorFilter)
		{
			m_colorFilter->setParams(
				context.hue,
				context.saturation,
				context.lightness,
				context.contrast,
				context.shade,
				context.reverseColor);
		}
	}

	void ColorFilterComponent::getColorFilter()
	{
		auto renderSystem = Root::instance()->getRenderSystem();
		if (renderSystem)
		{
			RenderStrategy* strategy = renderSystem->getCurRenderStrategy();
			if (strategy)
			{
				PostProcessChain* chain = strategy->getPostProcessChain();
				ColorFilter* process = (ColorFilter*)(chain->getPostProcess(PostProcess::ColorFilter));
				if (process)
				{
					m_colorFilter = process;
				}
			}
		}
	}
}
