#include "EchoComponentStableHeaders.h"
//#define ELUA_LIB

//#include "EchoScriptReg.h"
#include "EchoStringConverter.h"
#include "Actor/EchoActorManager.h"
#include "EchoLogManager.h"
#include "EchoScriptManager.h"
#include "EchoRoleObject.h"
#include "EchoDebugScriptFunc.h"

#include "EchoProtocolComponent.h"
#include "EchoActorProFuncImp.h"
#include "EchoTrajectoryComponent.h"
#include "EchoPolylineComponent.h"
#include "EchoCameraProFunctionImp.h"
#include "EchoActorScriptComponent.h"
#include "EchoTimelineComponent.h"
#include "EchoParticleSystemComponent.h"
#include "EchoMusicControllerComponent.h"

#include "EchoRigidBodyComponent.h"
#include "EchoBoxComponent.h"
#include "EchoDecalComponent.h"
#include "EchoDirecLightComponent.h"
#include "EchoLightComponent.h"
#include "EchoActorEntityComponent.h"
#include "EchoSoundComponent.h"
#include "EchoBoxColliderComponent.h"
#include "Actor/EchoNodeComponent.h"
#include "EchoCharactorControllerComponent.h"
#include "EchoPhysicsManager.h"
#include "EchoPhysicsComponentInclude.h"
#include "EchoShowTrajectoryComponent.h"
#include "EchoActorRoleComponent.h"
#include "EchoNodeProFuncImp.h"
#include "3DUI/Echo3DUIComponent.h"
#include "EchoControllerComponent.h"
#include "EchoTODComponentManager.h"
#include "EchoCloudSphereComponent.h"
#include "EchoFogComponent.h"
#include "EchoSheetComponent.h"
#include "EchoSceneComponentManager.h"

//using namespace eLuaBridge;
//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------
namespace Echo
{
	void scriptRegCommon()
	{
		//ECHO_REG_SCRIPT_BEGIN
		//	ECHO_REG_CLASS_BEGIN(EchoDebugScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoDebugScript, Script_DebugInt)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoDebugScript, Script_DebugFloat)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoDebugScript, Script_DebugString)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoDebugScript, Script_DebugVector3)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoDebugScript, Script_DebugQuaternion)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoDebugScript, Script_DebugBool)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoDebugScript, Script_DebugColorValue)

		//	//// 没有 transform 组件， 暂存于此
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoDebugScript, RotateAround)
		//	////没有3DUI组件，打字机函数暂存于此
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoDebugScript, CurrentTime)
		//	////道路生成组件
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoDebugScript, CreateEntity)
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoDebugScript, DeleteEntity)
		//	////运动轨迹函数
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoDebugScript, ShowTrajectory)
		//	ECHO_REG_CLASS_END

		//	ECHO_REG_CLASS_BEGIN(Vector2)
		//	ECHO_REG_CLASS_DEFAULT_CONSTRUCTOR
		//	ECHO_REG_CLASS_DATA(Vector2, x)
		//	ECHO_REG_CLASS_DATA(Vector2, y)
		//	ECHO_REG_CLASS_END

		//	ECHO_REG_CLASS_BEGIN(Vector3)
		//	ECHO_REG_CLASS_DEFAULT_CONSTRUCTOR
		//	ECHO_REG_CLASS_DATA(Vector3, x)
		//	ECHO_REG_CLASS_DATA(Vector3, y)
		//	ECHO_REG_CLASS_DATA(Vector3, z)
		//	ECHO_REG_CLASS_FUNC(Vector3, Length)
		//	ECHO_REG_CLASS_STATIC_FUNC(Vector3, SmoothDamp)
		//	ECHO_REG_CLASS_STATIC_FUNC(Vector3, Distance)
		//	ECHO_REG_CLASS_END

		//	ECHO_REG_CLASS_BEGIN(ColorValue)
		//	ECHO_REG_CLASS_DEFAULT_CONSTRUCTOR
		//	ECHO_REG_CLASS_DATA(ColorValue, r)
		//	ECHO_REG_CLASS_DATA(ColorValue, g)
		//	ECHO_REG_CLASS_DATA(ColorValue, b)
		//	ECHO_REG_CLASS_DATA(ColorValue, a)
		//	ECHO_REG_CLASS_END

		//	ECHO_REG_CLASS_BEGIN(DVector3)
		//	ECHO_REG_CLASS_DEFAULT_CONSTRUCTOR
		//	ECHO_REG_CLASS_DATA(DVector3, x)
		//	ECHO_REG_CLASS_DATA(DVector3, y)
		//	ECHO_REG_CLASS_DATA(DVector3, z)
		//	ECHO_REG_CLASS_FUNC(DVector3, Length)
		//	ECHO_REG_CLASS_END

		//	ECHO_REG_CLASS_BEGIN(Quaternion)
		//	ECHO_REG_CLASS_DEFAULT_CONSTRUCTOR
		//	ECHO_REG_CLASS_DATA(Quaternion, x)
		//	ECHO_REG_CLASS_DATA(Quaternion, y)
		//	ECHO_REG_CLASS_DATA(Quaternion, z)
		//	ECHO_REG_CLASS_DATA(Quaternion, w)
		//	ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(RoleObject)
		//	//ECHO_REG_CLASS_FUNC(RoleObject, getPosition)
		//	//ECHO_REG_CLASS_FUNC(RoleObject, getDirection)
		//	//ECHO_REG_CLASS_FUNC(RoleObject, setPosition)
		//	//ECHO_REG_CLASS_FUNC(RoleObject, setDerivedPosition)
		//	//ECHO_REG_CLASS_END

		//	ECHO_REG_CLASS_BEGIN(EchoCameraScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoCameraScript, Script_GetMainCameraPos)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoCameraScript, Script_SetMainCameraPos)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoCameraScript, Script_GetMainCameraRot)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoCameraScript, Script_SetMainCameraRot)
		//	ECHO_REG_CLASS_END

		//	//// Actor
		//	//ECHO_REG_CLASS_BEGIN(Actor)

		//	//ECHO_REG_CLASS_FUNC(Actor, getParentActor)
		//	//ECHO_REG_CLASS_FUNC(Actor, getRootNodeCom)

		//	//ECHO_REG_CLASS_FUNC(Actor, getPosition)
		//	//ECHO_REG_CLASS_FUNC(Actor, setPosition)
		//	//ECHO_REG_CLASS_FUNC(Actor, getWorldPosition)
		//	//ECHO_REG_CLASS_FUNC(Actor, setWorldPosition)
		//	//ECHO_REG_CLASS_FUNC(Actor, getRotation)
		//	//ECHO_REG_CLASS_FUNC(Actor, setRotation)
		//	//ECHO_REG_CLASS_FUNC(Actor, getEulerAngles)
		//	//ECHO_REG_CLASS_FUNC(Actor, setEulerAngles)
		//	//ECHO_REG_CLASS_FUNC(Actor, getScale)
		//	//ECHO_REG_CLASS_FUNC(Actor, setScale)
		//	//ECHO_REG_CLASS_FUNC(Actor, getVisiable)
		//	//ECHO_REG_CLASS_FUNC(Actor, setVisiable)
		//	//ECHO_REG_CLASS_FUNC(Actor, getActorIDStr)
		//	//ECHO_REG_CLASS_FUNC(Actor, getParentActor)
		//	//ECHO_REG_CLASS_FUNC(Actor, getChildActorStr)
		//	//ECHO_REG_CLASS_END

		//	ECHO_REG_CLASS_BEGIN(EchoActorScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoActorScript, SetActorParentNode)
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoActorScript, GetEchoActorScript)
		//	ECHO_REG_CLASS_FUNC(EchoActorScript, getParentActor)
		//	ECHO_REG_CLASS_FUNC(EchoActorScript, getRootNodeCom)
		//	ECHO_REG_CLASS_FUNC(EchoActorScript, expired)
		//	//ECHO_REG_CLASS_FUNC(EchoActorScript, SetPosition)
		//	//ECHO_REG_CLASS_FUNC(EchoActorScript, SetRotation)
		//	//ECHO_REG_CLASS_FUNC(EchoActorScript, SetScale)
		//	
		//	ECHO_REG_CLASS_END

		//	//// -------------------------------------------------- Components -------------------------------------------------- //
		//	// ActorComponent
		//	ECHO_REG_CLASS_BEGIN(EchoActorComponentScript)
		//	ECHO_REG_CLASS_FUNC(EchoActorComponentScript, getComNameToString)
		//	ECHO_REG_CLASS_FUNC(EchoActorComponentScript, setPosition)
		//	ECHO_REG_CLASS_FUNC(EchoActorComponentScript, getPosition)
		//	ECHO_REG_CLASS_FUNC(EchoActorComponentScript, setWorldPosition)
		//	ECHO_REG_CLASS_FUNC(EchoActorComponentScript, getWorldPosition)
		//	ECHO_REG_CLASS_FUNC(EchoActorComponentScript, setRotation)
		//	ECHO_REG_CLASS_FUNC(EchoActorComponentScript, getRotation)
		//	ECHO_REG_CLASS_FUNC(EchoActorComponentScript, setWorldRotation)
		//	ECHO_REG_CLASS_FUNC(EchoActorComponentScript, getWorldRotation)
		//	ECHO_REG_CLASS_FUNC(EchoActorComponentScript, setEulerAngles)
		//	ECHO_REG_CLASS_FUNC(EchoActorComponentScript, getEulerAngles)
		//	ECHO_REG_CLASS_FUNC(EchoActorComponentScript, setScale)
		//	ECHO_REG_CLASS_FUNC(EchoActorComponentScript, getScale)
		//	ECHO_REG_CLASS_FUNC(EchoActorComponentScript, expired)
		//	ECHO_REG_CLASS_END

		//	// PhysicsBaseComponent
		//	//ECHO_REG_CLASS_DERIVE(PhysicsBaseScript, EchoActorComponentScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(PhysicsBaseScript, GetOtherPhysicsBaseScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(PhysicsBaseScript, GetPhysicsBaseScript)
		//	//ECHO_REG_CLASS_FUNC(PhysicsBaseScript, SetIsUsePhysics)
		//	//ECHO_REG_CLASS_END

		//	//Entity
		//	ECHO_REG_CLASS_DERIVE(EchoEntityScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoEntityScript, Script_GetEntityComponent)
		//	ECHO_REG_CLASS_END

		//	// 脚本
		//	ECHO_REG_CLASS_DERIVE(EchoActorScriptComScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoActorScriptComScript, GetActorScriptCom)
		//	ECHO_REG_CLASS_FUNC(EchoActorScriptComScript, GetFloatValue)
		//	ECHO_REG_CLASS_FUNC(EchoActorScriptComScript, SetFloatValue)
		//	ECHO_REG_CLASS_FUNC(EchoActorScriptComScript, GetIntValue)
		//	ECHO_REG_CLASS_FUNC(EchoActorScriptComScript, SetIntValue)
		//	ECHO_REG_CLASS_FUNC(EchoActorScriptComScript, GetBoolValue)
		//	ECHO_REG_CLASS_FUNC(EchoActorScriptComScript, SetBoolValue)
		//	ECHO_REG_CLASS_FUNC(EchoActorScriptComScript, GetStringValue)
		//	ECHO_REG_CLASS_FUNC(EchoActorScriptComScript, SetStringValue)
		//	ECHO_REG_CLASS_FUNC(EchoActorScriptComScript, GetVector3Value)
		//	ECHO_REG_CLASS_FUNC(EchoActorScriptComScript, SetVector3Value)
		//	ECHO_REG_CLASS_FUNC(EchoActorScriptComScript, GetQuaternionValue)
		//	ECHO_REG_CLASS_FUNC(EchoActorScriptComScript, SetQuaternionValue)
		//	ECHO_REG_CLASS_END

		//	// Timeline
		//	ECHO_REG_CLASS_DERIVE(EchoTimelineScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoTimelineScript, GetTimeLineCom)
		//	ECHO_REG_CLASS_FUNC(EchoTimelineScript, getActorTimeValue)
		//	ECHO_REG_CLASS_FUNC(EchoTimelineScript, remakeTime) //让时间轴时间置零
		//	ECHO_REG_CLASS_FUNC(EchoTimelineScript, setIsPlay) //新的让时间轴播放的接口
		//	ECHO_REG_CLASS_FUNC(EchoTimelineScript, getIsPlayScene)//新的获取时间轴是否播放的接口
		//	ECHO_REG_CLASS_FUNC(EchoTimelineScript, getTimeLineFrame)//获取时间轴当前的桢数
		//	ECHO_REG_CLASS_FUNC(EchoTimelineScript, GetTimeLineFile)//修改时间轴加载的timeline文件
		//	ECHO_REG_CLASS_FUNC(EchoTimelineScript, stopTodTimelinePlay)
		//	ECHO_REG_CLASS_END

  //          //SceneComponentManager 
		//	ECHO_REG_CLASS_DERIVE(EchoSceneComManagerScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoSceneComManagerScript, setChangeSceneState)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoSceneComManagerScript, setChangeSceneName)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoSceneComManagerScript, setCameraDirMode)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoSceneComManagerScript, setCameraPosition)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoSceneComManagerScript, setChangeSceneId)
		//	ECHO_REG_CLASS_END

  //          //TodManager
		//	ECHO_REG_CLASS_DERIVE(EchoTodComManagerScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoTodComManagerScript, GetTodComManagerCom)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoTodComManagerScript, setTodComIsUse)
		//	ECHO_REG_CLASS_END

  //          //CloudSphere
		//	ECHO_REG_CLASS_DERIVE(EchoCloudSphereComScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoCloudSphereComScript, GetCloudSphereScript)
		//	ECHO_REG_CLASS_FUNC(EchoCloudSphereComScript,setTodComImmediateComponent)
		//	ECHO_REG_CLASS_FUNC(EchoCloudSphereComScript, updateTodItemState)
		//	ECHO_REG_CLASS_FUNC(EchoCloudSphereComScript, setPriority)
		//	ECHO_REG_CLASS_END
  //          
  //          //CloudMesh
		//	ECHO_REG_CLASS_DERIVE(EchoCloudMeshComScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoCloudMeshComScript, GetCloudMeshScript)
		//	ECHO_REG_CLASS_FUNC(EchoCloudMeshComScript, setTodComImmediateComponent)
		//	ECHO_REG_CLASS_FUNC(EchoCloudMeshComScript, updateTodItemState)
		//	ECHO_REG_CLASS_FUNC(EchoCloudMeshComScript, setPriority)
		//	ECHO_REG_CLASS_END

		//	//CloudPlane
		//	ECHO_REG_CLASS_DERIVE(EchoCloudPlaneComScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoCloudPlaneComScript, GetCloudPlaneScript)
		//	ECHO_REG_CLASS_FUNC(EchoCloudPlaneComScript, setTodComImmediateComponent)
		//	ECHO_REG_CLASS_FUNC(EchoCloudPlaneComScript, updateTodItemState)
		//	ECHO_REG_CLASS_FUNC(EchoCloudPlaneComScript, setPriority)
		//	ECHO_REG_CLASS_END
		//		
		//	//Sky
		//	ECHO_REG_CLASS_DERIVE(EchoSkyComScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoSkyComScript, GetSkyScript)
		//	ECHO_REG_CLASS_FUNC(EchoSkyComScript, setTodComImmediateComponent)
		//	ECHO_REG_CLASS_FUNC(EchoSkyComScript, updateTodItemState)
		//	ECHO_REG_CLASS_FUNC(EchoSkyComScript, setPriority)
		//	ECHO_REG_CLASS_END

  //          //Fog
		//	ECHO_REG_CLASS_DERIVE(EchoFogScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoFogScript, Script_GetFogComponent)
		//	ECHO_REG_CLASS_FUNC(EchoFogScript, setTodComImmediateComponent)
		//	ECHO_REG_CLASS_FUNC(EchoFogScript, updateTodItemState)
		//	ECHO_REG_CLASS_FUNC(EchoFogScript, setPriority)
		//	ECHO_REG_CLASS_END

  //          //Sun   
		//	ECHO_REG_CLASS_DERIVE(EchoSunScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoSunScript, Script_GetSunComponent)
		//	ECHO_REG_CLASS_FUNC(EchoSunScript, setTodComImmediateComponent)
		//	ECHO_REG_CLASS_FUNC(EchoSunScript, updateTodItemState)
		//	ECHO_REG_CLASS_FUNC(EchoSunScript, setPriority)
		//	ECHO_REG_CLASS_END

  //          //DirecLight
		//	ECHO_REG_CLASS_DERIVE(EchoDirecLightScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoDirecLightScript, Script_GetDirecLightComponent)
		//	ECHO_REG_CLASS_FUNC(EchoDirecLightScript, setTodComImmediateComponent)
		//	ECHO_REG_CLASS_FUNC(EchoDirecLightScript, setPriority)
		//	ECHO_REG_CLASS_FUNC(EchoDirecLightScript, updateTodItemState)
		//	ECHO_REG_CLASS_END

		//	// Box
		//	ECHO_REG_CLASS_DERIVE(EchoBoxScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoBoxScript, Script_GetBoxComponent)
		//	ECHO_REG_CLASS_FUNC(EchoBoxScript, isPosInBox)
		//	ECHO_REG_CLASS_FUNC(EchoBoxScript, setEpitaxyDistance)
		//	ECHO_REG_CLASS_FUNC(EchoBoxScript, isPosInEpitaxyBox)
		//	//TOD相关接口
		//	ECHO_REG_CLASS_FUNC(EchoBoxScript, pushBoxStack)
		//	ECHO_REG_CLASS_FUNC(EchoBoxScript, popBoxStack)
		//	ECHO_REG_CLASS_FUNC(EchoBoxScript, getPriority)
		//	ECHO_REG_CLASS_END

		//	// Role
		//	ECHO_REG_CLASS_DERIVE(EchoRoleComScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoRoleComScript, GetRoleComponent)
		//	ECHO_REG_CLASS_END

		//	// EffectSystem
		//	ECHO_REG_CLASS_DERIVE(ParticleSystemComponentScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(ParticleSystemComponentScript, GetParticleSystemComponent)
		//	ECHO_REG_CLASS_FUNC(ParticleSystemComponentScript, LoadParticle)
		//	ECHO_REG_CLASS_FUNC(ParticleSystemComponentScript, setVisible)
		//	ECHO_REG_CLASS_FUNC(ParticleSystemComponentScript, PlayParticle)
		//	ECHO_REG_CLASS_END

		//	// MusicController
		//	ECHO_REG_CLASS_DERIVE(MusicControllerComponentScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(MusicControllerComponentScript, GetMusicControllerComponent)
		//	ECHO_REG_CLASS_END

		//	// Decal
		//	ECHO_REG_CLASS_DERIVE(EchoDecalScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoDecalScript, Script_GetDecalComponent)
		//	ECHO_REG_CLASS_FUNC(EchoDecalScript, changeDiffuseTexture)
		//	ECHO_REG_CLASS_END

		//	// Sound
		//	ECHO_REG_CLASS_DERIVE(EchoSoundScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoSoundScript, Script_GetSoundComponent)
		//	ECHO_REG_CLASS_FUNC(EchoSoundScript, reloadSound)
		//	ECHO_REG_CLASS_FUNC(EchoSoundScript, removeSound)
		//	ECHO_REG_CLASS_FUNC(EchoSoundScript, playSound)
		//	ECHO_REG_CLASS_FUNC(EchoSoundScript, playing)
		//	ECHO_REG_CLASS_FUNC(EchoSoundScript, pause)
		//	ECHO_REG_CLASS_FUNC(EchoSoundScript, stop)
		//	ECHO_REG_CLASS_END

		//	// ControllerComponent
		//	ECHO_REG_CLASS_DERIVE(EchoControllerScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoControllerScript, GetControllerCom)
		//	ECHO_REG_CLASS_FUNC(EchoControllerScript, SetParameterIntValue)
		//	ECHO_REG_CLASS_FUNC(EchoControllerScript, SetParameterFloatValue)
		//	ECHO_REG_CLASS_FUNC(EchoControllerScript, SetParameterFloatValue_Name)
		//	ECHO_REG_CLASS_FUNC(EchoControllerScript, GetParameterFloatValue)
		//	ECHO_REG_CLASS_FUNC(EchoControllerScript, SetParameterBoolValue)
		//	ECHO_REG_CLASS_FUNC(EchoControllerScript, GetParameterBoolValue)
		//	ECHO_REG_CLASS_FUNC(EchoControllerScript, SetInsteadBoolValue)
		//	ECHO_REG_CLASS_FUNC(EchoControllerScript, SetRolePosition)
		//	ECHO_REG_CLASS_FUNC(EchoControllerScript, GetDirFromRoleFront)
		//	ECHO_REG_CLASS_FUNC(EchoControllerScript, SetRolePositionToDir)
		//	ECHO_REG_CLASS_FUNC(EchoControllerScript, UpdateBlendtree)
		//	ECHO_REG_CLASS_FUNC(EchoControllerScript, isKeyDown)
		//	ECHO_REG_CLASS_FUNC(EchoControllerScript, isKeyUp)
		//	ECHO_REG_CLASS_END

		//	// 3DUI 
		//	ECHO_REG_CLASS_DERIVE(Echo3DUIComponentScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(Echo3DUIComponentScript, GetEcho3DUIComponent)
		//	ECHO_REG_CLASS_END

		//	// 3DUI Base
		//	ECHO_REG_CLASS_DERIVE(Echo3DUIBaseComponentScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_FUNC(Echo3DUIBaseComponentScript, SetVisible)
		//	ECHO_REG_CLASS_FUNC(Echo3DUIBaseComponentScript, GetVisible)
		//	ECHO_REG_CLASS_FUNC(Echo3DUIBaseComponentScript, SetControllerPage)
		//	ECHO_REG_CLASS_END

		//	// 3DUI Button
		//	ECHO_REG_CLASS_DERIVE(Echo3DUIButtonComponentScript, Echo3DUIBaseComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(Echo3DUIButtonComponentScript, GetEcho3DUIButtonComponent)
		//	ECHO_REG_CLASS_END

		//	// 3DUI InputText
		//	ECHO_REG_CLASS_DERIVE(Echo3DUIInputTextComponentScript, Echo3DUIBaseComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(Echo3DUIInputTextComponentScript, GetEcho3DUIInputTextComponent)
		//	ECHO_REG_CLASS_FUNC(Echo3DUIInputTextComponentScript, SetText)
		//	ECHO_REG_CLASS_FUNC(Echo3DUIInputTextComponentScript, GetText)
		//	ECHO_REG_CLASS_END

		//	// 3DUI Node
		//	ECHO_REG_CLASS_BEGIN(Echo3DUINodeComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(Echo3DUINodeComponentScript, GetEcho3DUINodeComponent)
		//	ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(ActorManager)
		//	//ECHO_REG_CLASS_STATIC_FUNC(ActorManager, getCharatorControllerActor)
		//	//ECHO_REG_CLASS_END

		//	//// -------------------------------------------------- Components -------------------------------------------------- //
		//	//ECHO_REG_CLASS_BEGIN(ActorComponent)
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, getComNameToString)
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, getTypeToString)
		//	//
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, getParentNodeCom)
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, setParentNodeCom)
		//	////根节点相关的节点操作
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, getVisible)
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, setVisible)
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, setPosition)
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, getPosition)
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, setWorldPosition)
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, getWorldPosition)
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, setRotation)
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, getRotation)
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, setWorldRotation)
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, getWorldRotation)
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, setScale)
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, getScale)
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, setEulerAngles)
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, getEulerAngles)
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, setWorldScale)
		//	//ECHO_REG_CLASS_FUNC(ActorComponent, getWorldScale)
		//	//ECHO_REG_CLASS_END

		//	//// EchoPhysicsSystem 
		//	//ECHO_REG_CLASS_BEGIN(EchoPhysicsSystem)
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoPhysicsSystem, StaticSetRunPhysics)
		//	//ECHO_REG_CLASS_END

		//	//// NodeComponent
		//	//ECHO_REG_CLASS_DERIVE(NodeComponent, ActorComponent)
		//	//ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(EchoNodeScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoNodeScript, Script_GetNodeComponent)
		//	//ECHO_REG_CLASS_END

		//	//// TrajectoryComponent
		//	//ECHO_REG_CLASS_DERIVE(TrajectoryComponent, ActorComponent)
		//	//ECHO_REG_CLASS_FUNC(TrajectoryComponent, GetDistance)
		//	//ECHO_REG_CLASS_FUNC(TrajectoryComponent, GetPosVec)
		//	//ECHO_REG_CLASS_FUNC(TrajectoryComponent, GetPos)
		//	//ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(EchoTrajectoryScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoTrajectoryScript, Script_ShowTrajectory)
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoTrajectoryScript, Script_GetTrajectoryComponent)
		//	//ECHO_REG_CLASS_END

		//	//// ParticleSystemComponent
		//	//ECHO_REG_CLASS_DERIVE(ParticleSystemComponent, ActorComponent)
		//	////ECHO_REG_CLASS_FUNC(ParticleSystemComponent, LoadParticlePath)
		//	//ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(ParticleSystemComponentScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(ParticleSystemComponentScript, GetParticleSystemComponent)
		//	//ECHO_REG_CLASS_END

		//	////MusicControllerComponent
		//	//ECHO_REG_CLASS_DERIVE(MusicControllerComponent, ActorComponent)
		//	////ECHO_REG_CLASS_FUNC(MusicControllerComponent, GetFrequency)
		//	////ECHO_REG_CLASS_FUNC(MusicControllerComponent, GetFrequencyList)
		//	////ECHO_REG_CLASS_FUNC(MusicControllerComponent, SetIndexTime)
		//	//ECHO_REG_CLASS_FUNC(MusicControllerComponent, GetMainFrequencyTimeVal)
		//	////ECHO_REG_CLASS_FUNC(MusicControllerComponent, GetTestMinFrequency)
		//	////ECHO_REG_CLASS_FUNC(MusicControllerComponent, GetTestMaxFrequency)
		//	//ECHO_REG_CLASS_FUNC(MusicControllerComponent, GetMusicTime)
		//	//ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(MusicControllerComponentScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(MusicControllerComponentScript, GetMusicControllerComponent)
		//	//ECHO_REG_CLASS_END

		//	//// TimelineComponent
		//	//ECHO_REG_CLASS_DERIVE(TimelineComponent, ActorComponent)
		//	//ECHO_REG_CLASS_FUNC(TimelineComponent, getActorTimeValue)
		//	//ECHO_REG_CLASS_FUNC(TimelineComponent, timelineUpdate)
		//	//ECHO_REG_CLASS_FUNC(TimelineComponent, isPlayVideo) //老接口，已经抛弃
		//	//ECHO_REG_CLASS_FUNC(TimelineComponent, remakeTime) //让时间轴时间置零
		//	//ECHO_REG_CLASS_FUNC(TimelineComponent, setHermiteNames)
		//	//ECHO_REG_CLASS_FUNC(TimelineComponent, getIsplay) //老接口，已经抛弃
		//	//ECHO_REG_CLASS_FUNC(TimelineComponent, setIsPlay) //新的让时间轴播放的接口
		//	//ECHO_REG_CLASS_FUNC(TimelineComponent, getIsPlayScene)//新的获取时间轴是否播放的接口
		// //   ECHO_REG_CLASS_FUNC(TimelineComponent, getTimeLineFrame)//获取时间轴当前的桢数
		//	//ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(EchoTimelineScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoTimelineScript, GetTimeLineCom)
		//	//ECHO_REG_CLASS_END

		//	//// DecalComponent
		//	//ECHO_REG_CLASS_DERIVE(DecalComponent, ActorComponent)
		//	//ECHO_REG_CLASS_FUNC(DecalComponent, changeDiffuseTexture)
		//	//ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(EchoDecalScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoDecalScript, Script_GetDecalComponent)
		//	//ECHO_REG_CLASS_END

		//	//// EntityComponent
		//	//ECHO_REG_CLASS_DERIVE(EntityComponent, ActorComponent)
		//	//ECHO_REG_CLASS_FUNC(EntityComponent, getDissolveStrength)
		//	//ECHO_REG_CLASS_FUNC(EntityComponent, setDissolveStrength)
		//	//ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(EchoEntityScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoEntityScript, Script_GetEntityComponent)
		//	//ECHO_REG_CLASS_END

		//	//// SoundComponent
		//	//ECHO_REG_CLASS_DERIVE(SoundComponent, ActorComponent)
		//	//ECHO_REG_CLASS_FUNC(SoundComponent, reloadSound)
		//	//ECHO_REG_CLASS_FUNC(SoundComponent, removeSound)
		//	//ECHO_REG_CLASS_FUNC(SoundComponent, playSound)
		//	//ECHO_REG_CLASS_FUNC(SoundComponent, playing)
		//	//ECHO_REG_CLASS_FUNC(SoundComponent, pause)
		//	//ECHO_REG_CLASS_FUNC(SoundComponent, stop)
		//	//ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(EchoSoundScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoSoundScript, Script_GetSoundComponent)
		//	//ECHO_REG_CLASS_END

		//	//// CharatorControllerComponent 
		//	//ECHO_REG_CLASS_DERIVE(CharatorControllerComponent, ActorComponent)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, UpdateHelper)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, SetHelperVisible)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, GetHelperVisible)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, Move)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, CreatePhysics)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, ReleasePhysics)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, GetWorldCenterPosition)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, GetWorldFootPosition)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, SetUpDirection)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, GetUpDirection)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, SetSlopeLimit)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, GetSlopeLimit)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, SetStepOffset)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, GetStepOffset)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, SetSkinWidth)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, GetSkinWidth)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, SetMinMoveDistance)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, GetMinMoveDistance)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, SetCenter)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, GetCenter)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, SetRadius)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, GetRadius)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, SetMiddleHeight)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, GetMiddleHeight)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, SetRealHeight)
		//	//	ECHO_REG_CLASS_FUNC(CharatorControllerComponent, GetRealHeigth)
		//	//ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(EchoCharatorControllerScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoCharatorControllerScript, Script_GetCharatorControllerComponent)
		//	//ECHO_REG_CLASS_END

		//	//// BoxComponent
		//	//ECHO_REG_CLASS_DERIVE(BoxComponent, ActorComponent)
		//	//ECHO_REG_CLASS_FUNC(BoxComponent, isPosInBox)
		//	//ECHO_REG_CLASS_FUNC(BoxComponent, setScale)
		//	//ECHO_REG_CLASS_FUNC(BoxComponent, getScale)
		//	//ECHO_REG_CLASS_FUNC(BoxComponent, setPriority)
		//	//ECHO_REG_CLASS_FUNC(BoxComponent, getPriority)
		//	//ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(EchoBoxScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoBoxScript, Script_GetBoxComponent)
		//	//ECHO_REG_CLASS_END

		//	//// DirecLightComponent
		//	//ECHO_REG_CLASS_DERIVE(DirecLightComponent, ActorComponent)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, setActive)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getActive)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, canUpdate)

		//	//	/*ECHO_REG_CLASS_FUNC(DirecLightComponent, setInitPBRMainLight)
		//	//	ECHO_REG_CLASS_FUNC(DirecLightComponent, setInitPBRSkyLight)
		//	//	ECHO_REG_CLASS_FUNC(DirecLightComponent, setInitPBRGroundLight)
		//	//	ECHO_REG_CLASS_FUNC(DirecLightComponent, setInitRoleSkyLight)
		//	//	ECHO_REG_CLASS_FUNC(DirecLightComponent, setInitRoleGroundLight)
		//	//	ECHO_REG_CLASS_FUNC(DirecLightComponent, setInitRoleMainLight)*/

		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getInitPBRMainLight)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getInitPBRSkyLight)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getInitPBRGroundLight)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getInitRoleMainLight)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getInitRoleSkyLight)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getInitRoleGroundLight)

		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, setPBRMainLightColor)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getPBRMainLightColor)

		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, setPBRSkyLightColor)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getPBRSkyLightColor)

		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, setPBRGroundLightColor)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getPBRGroundLightColor)

		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, setRoleMainLightColor)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getRoleMainLightColor)

		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, setRoleSkyLightColor)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getRoleSkyLightColor)

		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, setRoleGroundLightColor)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getRoleGroundLightColor)

		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, setPriority)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getPriority)

		//	////ECHO_REG_CLASS_FUNC(DirecLightComponent, setPBRMainLightPower)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getPBRMainLightPower)

		//	////ECHO_REG_CLASS_FUNC(DirecLightComponent, setPBRSkyLightPower)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getPBRSkyLightPower)

		//	////ECHO_REG_CLASS_FUNC(DirecLightComponent, setPBRGroundLightPower)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getPBRGroundLightPower)

		//	////ECHO_REG_CLASS_FUNC(DirecLightComponent, setRoleMainLightPower)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getRoleMainLightPower)
		//	////ECHO_REG_CLASS_FUNC(DirecLightComponent, setRoleSkyLightPower)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getRoleSkyLightPower)
		//	////ECHO_REG_CLASS_FUNC(DirecLightComponent, setRoleGroundLightPower)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, getRoleGroundLightPower)
		//	//ECHO_REG_CLASS_FUNC(DirecLightComponent, setScreenAttribute)

		//	//ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(EchoDirecLightScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoDirecLightScript, Script_GetDirecLightComponent)
		//	//ECHO_REG_CLASS_END

		//	//// LightComponent
		//	//ECHO_REG_CLASS_DERIVE(LightComponent, ActorComponent)
		//	//ECHO_REG_CLASS_FUNC(LightComponent, setPointLightColor)
		//	//ECHO_REG_CLASS_FUNC(LightComponent, getPointLightColor)
		//	//ECHO_REG_CLASS_FUNC(LightComponent, setLightIntensity)
		//	//ECHO_REG_CLASS_FUNC(LightComponent, getLightIntensity)
		//	//ECHO_REG_CLASS_FUNC(LightComponent, setRadius)
		//	//ECHO_REG_CLASS_FUNC(LightComponent, getRadius)
		//	//ECHO_REG_CLASS_FUNC(LightComponent, setPriority)
		//	//ECHO_REG_CLASS_FUNC(LightComponent, getPriority)
		//	//ECHO_REG_CLASS_FUNC(LightComponent, setInRadius)
		//	//ECHO_REG_CLASS_FUNC(LightComponent, setOutRadius)
		//	//ECHO_REG_CLASS_FUNC(LightComponent, getInRadius)
		//	//ECHO_REG_CLASS_FUNC(LightComponent, getOutRadius)
		//	//ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(EchoLightScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoLightScript, Script_GetLightComponent)
		//	//ECHO_REG_CLASS_END

		//	//// ShowTrajectoryComponent
		//	//ECHO_REG_CLASS_DERIVE(ShowTrajectoryComponent, ActorComponent)
		//	//ECHO_REG_CLASS_FUNC(ShowTrajectoryComponent, getTrajectory)
		//	//ECHO_REG_CLASS_FUNC(ShowTrajectoryComponent, showTrajectory)
		//	//ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(EchoShowTrajectoryScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoShowTrajectoryScript, Script_GetShowTrajectoryComponent)
		//	//ECHO_REG_CLASS_END

		//	//// RoleComponent
		//	//ECHO_REG_CLASS_DERIVE(RoleComponent, ActorComponent)
		//	//ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(EchoRoleComScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoRoleComScript, GetRoleComponent)
		//	//ECHO_REG_CLASS_END

		//	//// TODO: 仅测试 RigidBody 
		//	//ECHO_REG_CLASS_DERIVE(RigidBodyComponent, ActorComponent)
		//	//	ECHO_REG_CLASS_FUNC(RigidBodyComponent, SetLocalCenterofMass)
		//	//	ECHO_REG_CLASS_FUNC(RigidBodyComponent, GetLocalCenterofMass)
		//	//ECHO_REG_CLASS_END
		//	//ECHO_REG_CLASS_BEGIN(EchoRigidBodyScript)
		//	//	ECHO_REG_CLASS_STATIC_FUNC(EchoRigidBodyScript, Script_GetRigidBodyComponent)
		//	//ECHO_REG_CLASS_END
		//	////ControllerComponent
		//	//ECHO_REG_CLASS_DERIVE(ControllerComponent, ProtocolComponent)
		//	//	//ECHO_REG_CLASS_FUNC(ControllerComponent, GetRoleIsMove)
		//	//	//ECHO_REG_CLASS_FUNC(ControllerComponent, SetRoleIsMove)
		//	//	//ECHO_REG_CLASS_FUNC(ControllerComponent, GetRoleIsMove)
		//	//	ECHO_REG_CLASS_FUNC(ControllerComponent, SetParameterIntValue)
		//	//	ECHO_REG_CLASS_FUNC(ControllerComponent, SetParameterFloatValue)
		//	//	ECHO_REG_CLASS_FUNC(ControllerComponent, SetParameterFloatValue_Name)
		//	//	ECHO_REG_CLASS_FUNC(ControllerComponent, GetParameterFloatValue)
		//	//	ECHO_REG_CLASS_FUNC(ControllerComponent, SetParameterBoolValue)
		//	//	ECHO_REG_CLASS_FUNC(ControllerComponent, GetParameterBoolValue)
		//	//	ECHO_REG_CLASS_FUNC(ControllerComponent, SetInsteadBoolValue)
		//	//	ECHO_REG_CLASS_FUNC(ControllerComponent, SetRoleDir)
		//	//	ECHO_REG_CLASS_FUNC(ControllerComponent, SetRolePosition)
		//	//	ECHO_REG_CLASS_FUNC(ControllerComponent, GetDirFromRoleFront)
		//	//	ECHO_REG_CLASS_FUNC(ControllerComponent, SetRolePositionToDir)
		//	//	ECHO_REG_CLASS_FUNC(ControllerComponent, UpdateBlendtree)

		//	//	ECHO_REG_CLASS_STATIC_FUNC(ControllerComponent, isKeyDown)
		//	//	ECHO_REG_CLASS_STATIC_FUNC(ControllerComponent, isKeyUp)
		//	//ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(EchoControllerScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(EchoControllerScript, GetControllerCom)
		//	//ECHO_REG_CLASS_END

		//	//// WheelColloderComponent 
		//	//ECHO_REG_CLASS_BEGIN(EchoWheelFriction)
		//	//	ECHO_REG_CLASS_DEFAULT_CONSTRUCTOR
		//	//	ECHO_REG_CLASS_DATA(EchoWheelFriction, extremumSlip)
		//	//	ECHO_REG_CLASS_DATA(EchoWheelFriction, extremumValue)
		//	//	ECHO_REG_CLASS_DATA(EchoWheelFriction, asymptoteSlip)
		//	//	ECHO_REG_CLASS_DATA(EchoWheelFriction, asymptoteValue)
		//	//	ECHO_REG_CLASS_DATA(EchoWheelFriction, stiffness)
		//	//ECHO_REG_CLASS_END
		//	//ECHO_REG_CLASS_DERIVE(WheelColliderComponent, ActorComponent)

		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, setBrakeTorque)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, setDriveTorque)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, setSteerAngle)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, getBrakeTorque)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, getDriveTorque)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, getSteerAngle)

		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, setMass)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, getMass)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, setRadius)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, getRadius)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, setWheelDampingRate)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, getWheelDampingRate)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, setSuspensionDistance)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, getSuspensionDistance)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, setForceAppPointDistance)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, getForceAppPointDistance)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, setSuspSpring)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, getSuspSpring)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, setSuspDamper)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, getSuspDamper)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, setSuspTargetPos)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, getSuspTargetPos)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, setForwardFriction)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, getForwardFriction)

		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, getRealRadius)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, getRealWidth)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, getRealSuspensionDistance)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, getRealForceAppPointDistance)

		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, getWheelCenterWorldPostion)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, isInAir)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, tireContactPoint)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, tireContactNormal)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, tireFrition)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, tirePos)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, tireLongitudinalDir)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, tireLateralDir)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, longitudinalSlip)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, lateralSlip)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, rpm)
		//	//	ECHO_REG_CLASS_FUNC(WheelColliderComponent, rotationAngle)

		//	//ECHO_REG_CLASS_END
		//	//ECHO_REG_CLASS_BEGIN(EchoWheelColliderScript)
		//	//	ECHO_REG_CLASS_STATIC_FUNC(EchoWheelColliderScript, Script_GetWheelColliderComponent)
		//	//ECHO_REG_CLASS_END
		//	//// ----------------------------------------------- 3DUI -------------------------------------------------------- //
		//	//// 3DUI Base
		//	//ECHO_REG_CLASS_DERIVE(Echo3DUIBaseComponent, ActorComponent)
		//	//ECHO_REG_CLASS_FUNC(Echo3DUIBaseComponent, SetVisible)
		//	//ECHO_REG_CLASS_FUNC(Echo3DUIBaseComponent, GetVisible)
		//	//ECHO_REG_CLASS_FUNC(Echo3DUIBaseComponent, SetControllerPage)
		//	//ECHO_REG_CLASS_END
		//	//	
		//	//ECHO_REG_CLASS_BEGIN(Echo3DUIBaseComponentScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(Echo3DUIBaseComponentScript, GetEcho3DUIBaseComponent)
		//	//ECHO_REG_CLASS_END

		//	//// 3DUI 
		//	//ECHO_REG_CLASS_DERIVE(Echo3DUIComponent, Echo3DUIBaseComponent)
		//	//ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(Echo3DUIComponentScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(Echo3DUIComponentScript, GetEcho3DUIComponent)
		//	//ECHO_REG_CLASS_END

		//	//// Button
		//	//ECHO_REG_CLASS_DERIVE(Echo3DUIButtonComponent, Echo3DUIBaseComponent)
		//	//ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(Echo3DUIButtonComponentScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(Echo3DUIButtonComponentScript, GetEcho3DUIButtonComponent)
		//	//ECHO_REG_CLASS_END
		//	//// InputText
		//	//ECHO_REG_CLASS_DERIVE(Echo3DUIInputTextComponent, Echo3DUIBaseComponent)
		//	//ECHO_REG_CLASS_FUNC(Echo3DUIInputTextComponent, SetText)
		//	//ECHO_REG_CLASS_FUNC(Echo3DUIInputTextComponent, GetText)
		//	//ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(Echo3DUIInputTextComponentScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(Echo3DUIInputTextComponentScript, GetEcho3DUIInputTextComponent)
		//	//ECHO_REG_CLASS_END

		//	//// Node
		//	//ECHO_REG_CLASS_DERIVE(Echo3DUINodeComponent, Echo3DUIBaseComponent)
		//	//ECHO_REG_CLASS_END

		//	//ECHO_REG_CLASS_BEGIN(Echo3DUINodeComponentScript)
		//	//ECHO_REG_CLASS_STATIC_FUNC(Echo3DUINodeComponentScript, GetEcho3DUINodeComponent)
		//	//ECHO_REG_CLASS_END

		//	//SheetComponent
		//	ECHO_REG_CLASS_DERIVE(EchoSheetComponentScript, EchoActorComponentScript)
		//	ECHO_REG_CLASS_STATIC_FUNC(EchoSheetComponentScript, Script_GetSheetComponent)
		//	//ECHO_REG_CLASS_FUNC(EchoSheetComponentScript, changeDataBase)
		//	//ECHO_REG_CLASS_FUNC(EchoSheetComponentScript, setDataBase)
		//	//ECHO_REG_CLASS_FUNC(EchoSheetComponentScript, setDataBaseByTableName)
		//	//ECHO_REG_CLASS_FUNC(EchoSheetComponentScript, setDBTable)
		//	ECHO_REG_CLASS_FUNC(EchoSheetComponentScript, readDB_string)
		//	ECHO_REG_CLASS_FUNC(EchoSheetComponentScript, readDB_int)
		//	ECHO_REG_CLASS_FUNC(EchoSheetComponentScript, readDB_float)
		//	//ECHO_REG_CLASS_FUNC(EchoSheetComponentScript, readDBByCol_string)
		//	//ECHO_REG_CLASS_FUNC(EchoSheetComponentScript, readDBByCol_float)
		//	//ECHO_REG_CLASS_FUNC(EchoSheetComponentScript, readDBByCol_int)
		//	//ECHO_REG_CLASS_FUNC(EchoSheetComponentScript, testDBFunction)
		//	ECHO_REG_CLASS_END

		//ECHO_REG_SCRIPT_END
	}

} // namespace Echo

//-----------------------------------------------------------------------------
//	Cpp File End
//-----------------------------------------------------------------------------
