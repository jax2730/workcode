
#ifndef _EchoUI3DObject_h__
#define _EchoUI3DObject_h__

#include "Echo3DUIComponent.h"
#include "Echo3DUIObjectBase.h"
#include "UI/GObject.h"
#include "UI/GRoot.h"

namespace Echo
{
	class UI3DObject : public UI3DObjectBase
	{
	public:
		UI3DObject();
		virtual ~UI3DObject();
		echofgui::GObject* m_pGobject = NULL;

		virtual void LoadGObjectFromFile(ActorComponentInfo* actorComInfo);
		virtual bool isNull() { return m_pGobject == NULL; }
		Echo::UI3DDummyMovable* getDummyMovable();
		virtual void setUI3DMovable(UI3DMovable* pMovable);
		virtual void changeScene3DUIStyle(UI2DStyle newStyle);
		virtual void clear();

		static void* rayHit3DUIComponentSystem(const HitTest3DUIComponentSystemInfo& hitTestInfo);
		static echofgui::GObject* recursivelyRayHit3DUI(const Ray& ray, echofgui::GObject* pObject);
		static UI3DObjectBase* CreateUI3DObject();
	};

}
#endif
