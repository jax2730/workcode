#ifndef _BendHorizonComponent_H_
#define _BendHorizonComponent_H_

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
	class _EchoComponentExport BendHorizonComponentInfo : public ActorComponentInfo//, public Singleton<BendHorizonComponentInfo>
	{
	public:
		BendHorizonComponentInfo();
		BendHorizonComponentInfo(const Name& comType, ActorCOMMoveableType type);
		~BendHorizonComponentInfo();
		virtual void									ExportData(cJSON* jsonNode) override;
		virtual void									ImportData(cJSON* jsonNode) override;
		virtual void									CopyTo(ActorComponentInfo* comInfo) override;
		virtual void                                    Overlay(ActorComponentInfo* srcInfo) override;
		virtual void                                    OverWrite() override;
	public:
		float mBendCurveFactorA = 0.f;
		float mBendCurveFactorC = 0.f;
		float mStartSmoothBendHorizonHeigh = 0.f;
		float mEndSmoothBendHorizonHeigh = 0.f;
		bool misOpenBendHorizon = false;
		EDITOR_REFLECT
	};

	class _EchoComponentExport BendHorizonComponent : public ActorComponent//, public Singleton<BendHorizonComponent>
	{
		DECLAREACTORCOMTYPE_H(BendHorizonComponent)
	public:
		virtual void							init(ActorComponentInfo* info) override;
		virtual void							copyTo(const ActorComponentPtr&) override;
		void									switchBendHorizon(bool& isOpen);
		void									setStartSmoothBendHeigh(float& value);
		void									setEndSmoothBendHeigh(float& value);
		void									setBendCurveFactorA(float& value);
		void									setBendCurveFactorC(float& value);
	public:
		//BendHorizonComponentInfo*			mBendHorizonInfo = nullptr;
	};

	DEFINECOMFACTORY(BendHorizonComponent, UNMOVEABLE)
}
#endif
