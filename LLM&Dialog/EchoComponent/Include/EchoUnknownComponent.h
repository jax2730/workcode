#ifndef _EchoUnknownComponent_h__
#define _EchoUnknownComponent_h__
#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "EchoComDefineMacro.h"

namespace Echo
{
	//一种空的Component，如果没有注册某种类型，或者引擎未支持的客户端类型。使用该组件。
	//该组件只用于记录json中的信息，不进行其他任何操作
	//=============================================================================
	class _EchoComponentExport UnknownComponentInfo : public ActorComponentInfo
	{
	public:
		UnknownComponentInfo(const Name& type, ActorCOMMoveableType movable);
		~UnknownComponentInfo();
	public:
		virtual void							ExportData(cJSON* jsonNode)override;
		virtual void							ImportData(cJSON* jsonNode)override;
		virtual void							CopyTo(ActorComponentInfo* comInfo)override;
		virtual void							Overlay(ActorComponentInfo* comInfo) override;
		virtual void							OverWrite() override;

		const char*								getCopy() { return mCopyJson; }
	private:
		char*									mCopyJson;
	};

	//============================================================================
	class _EchoComponentExport UnknownComponent : public NodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(UnknownComponent)
	public:
		REGISTER_LUA_CLASS_KEY;
	public:
		virtual void							init(ActorComponentInfo* info) override;
	};
	DEFINECOMFACTORY(UnknownComponent,ALL_MOVEABLE);
}
// namespace Echo
#endif 
