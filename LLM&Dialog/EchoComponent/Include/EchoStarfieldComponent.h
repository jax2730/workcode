#ifndef _StarfieldComponent_H_
#define _StarfieldComponent_H_

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoMovableObject.h"

#include "EchoHelperObj.h"
#include "EchoHelperObjProFuncImp.h"
#include "EchoComDefineMacro.h"

#include "EchoRenderTarget.h"

namespace Echo
{
	class _EchoComponentExport StarfieldComponentInfo :public ActorComponentInfo
	{
	public:
		StarfieldComponentInfo();
		StarfieldComponentInfo(const Name& comType, ActorCOMMoveableType type);
		~StarfieldComponentInfo();
		void						ExportData(cJSON* jsonNode)override;
		void						ImportData(cJSON* jsonNode)override;
		void						CopyTo(ActorComponentInfo* comInfo)override;
		void						Overlay(ActorComponentInfo* srcInfo)override;
		void						OverWrite()override;
	public:
		Vector2 mCenter = Vector2(0.5f,0.5f);
		float	mSpeed = 0.2f;
		float	mEffectLength = 1.0f;
		float   mEffectWidth = 1.0f;
		float	mLoopCount = 1.0f;
		bool	mIsOpenStarfield;

		EDITOR_REFLECT
	};

	class _EchoComponentExport StarfieldComponent : public ActorComponent {
		DECLAREACTORCOMTYPE_H(StarfieldComponent)
	public:
		virtual void	init(ActorComponentInfo* info)override;
		virtual void	copyTo(const ActorComponentPtr&)override;
		void			switchStarfield(bool isOpen);
		void			setCenter(const Vector2& value);
		void			setSpeed(float value);
		void			setEffectLength(float value);
		void			setEffectWidth(float value);
		void			setLoopCount(float value);
	};

	DEFINECOMFACTORY(StarfieldComponent, UNMOVEABLE)
}

#endif
