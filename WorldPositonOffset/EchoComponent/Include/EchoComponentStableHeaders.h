///-----------------------------------------------------------------------------
// File:   EchoComponentStableHeaders.h
//
// Author: wuyunlong
//
// Date:   2024/05/07
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

/* Stable headers which will be used for precompilation if the compiler
   supports it. Add entries here when headers are unlikely to change.
   NB: a change to any of these headers will result in a full rebuild,
   so don't add things to this lightly.
*/

#ifndef __EchoComponentStableHeaders__
#define __EchoComponentStableHeaders__

#include "EchoStableHeaders.h"
#include "EchoComponentPrerequisites.h"

#include "EchoRoot.h"
//Math
#include "EchoMath.h"
#include "EchoTransform.h"
#include "EchoVector2.h"
#include "EchoVector3.h"
#include "EchoVector4.h"
#include "EchoQuaternion.h"
//Core
#include "EchoName.h"

#include "EchoNode.h"
#include "EchoSceneNode.h"
#include "EchoSceneManager.h"

#include "EchoResourceFolderManager.h"

#include "EchoSubMesh.h"
#include "EchoEntity.h"

#include "EchoManualObject.h"

#include "EchoRole.h"
#include "EchoRoleObject.h"
//Physics
#include "EchoPhysXSystem.h"
#include "EchoPhysXScene.h"
//TOD
#include "EchoTODManager.h"
#include "EchoWeatherManager.h"
//Actor
#include "Actor/EchoActor.h"
#include "Actor/EchoActorInfo.h"
#include "Actor/EchoActorHelper.h"
#include "Actor/EchoActorSystem.h"
#include "Actor/EchoReflect.h"

//Component
#include "Actor/EchoActorComFactoryManager.h"
#include "Actor/EchoActorComponent.h"
#include "Actor/EchoNodeComponent.h"
#include "Actor/EchoNodeActorComponent.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoProtocolActorComponent.h" // 基类定义
#endif
