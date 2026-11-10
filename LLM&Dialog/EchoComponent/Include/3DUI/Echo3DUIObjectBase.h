
#ifndef _EchoUI3DObjectBase_h__
#define _EchoUI3DObjectBase_h__

#include "EchoComponentPrerequisites.h"

namespace Echo
{
	struct HitTest3DUIComponentSystemInfo
	{
		Echo::Vector2 screenPos;
		void* root;
	};

	class _EchoComponentExport UI3DObjectBase
	{
	public:
		UI3DObjectBase() {};
		virtual ~UI3DObjectBase() {};
		virtual void LoadGObjectFromFile(ActorComponentInfo* actorComInfo) {};
		virtual bool isNull() { return true; }
		virtual void setUI3DMovable(UI3DMovable* pMovable) {};
		virtual void changeScene3DUIStyle(UI2DStyle newStyle) {};
		virtual void clear() {};
	};


}
#endif
