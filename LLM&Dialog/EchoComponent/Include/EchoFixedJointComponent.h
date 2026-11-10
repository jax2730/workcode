//-----------------------------------------------------------------------------
// File:   EchoFixedJointComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-8-17
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef EchoFixedJointComponent_h__
#define EchoFixedJointComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoJointComponent.h"

namespace Echo
{
	class _EchoComponentExport FixedJointComponentInfo :public JointComponentInfo
	{
	public:
		FixedJointComponentInfo() {}
		FixedJointComponentInfo(const Name& comType, ActorCOMMoveableType MoveType);
		virtual ~FixedJointComponentInfo();

		using JointComponentInfo::ExportData; 
		using JointComponentInfo::ImportData;
		using JointComponentInfo::CopyTo;
		using JointComponentInfo::Overlay;
		using JointComponentInfo::OverWrite;

	};

	class _EchoComponentExport FixedJointComponent :public JointComponent
	{
	public:
		typedef FixedJointComponentInfo InfoType, *InfoPtrType;
		DECLAREACTORCOMTYPE_H(FixedJointComponent)
	};
	DEFINECOMFACTORY(FixedJointComponent, EDITOR_MOVEABLE)

}

#endif // EchoFixedJointComponent_h__