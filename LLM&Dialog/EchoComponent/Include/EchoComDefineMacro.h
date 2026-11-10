//-----------------------------------------------------------------------------
// File:  EchoComDefineMacro.h
//
// Author: wuyunlong
//
// Date:   2022-5-20
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoComDefineMacro_H__
#define __EchoComDefineMacro_H__

#include "Actor/EchoActorComFactoryManager.h"

#define COMPONENTTYPE(COMNAME)		Echo::Name(#COMNAME)
#define COMPONENTINFO(COMNAME)		COMNAME##Info
#define COMPONENTFACTORY(COMNAME)	COMNAME##Factory

//--------------------------------------------------------------------------------------------------//
//用于自动化生成指定Component的工厂类，该宏引擎使用带有_EchoComponentExport导出宏。
//第一个参数为Component的类名
//第二个参数为是否可以移动组件:只有*ALL_MOVEABLE的Component支持附加子Component。
//		EDITOR_MOVEABLE-编辑器模式下才生成node
//		CLIENT_MOVEABLE-客户端模式才生成node
//		UNMOVEABLE都不会生成node
//		ALL_MOVEABLE所有模式都有node
// 注意需要类型前面需要加上Echo
//--------------------------------------------------------------------------------------------------//
#define DEFINECOMFACTORY(COMNAME,MOVEABLE) \
class _EchoComponentExport COMNAME##Factory : public Echo::ActorComFactory\
{\
	static_assert(std::is_same<decltype(MOVEABLE),Echo::ActorCOMMoveableType>::value,"MOVEABLE must be ActorCOMMoveableType type!");\
public:\
	COMNAME##Factory():ActorComFactory(){ mType = Echo::Name(#COMNAME); mMoveableType = MOVEABLE; bIsEditorCom = true; mPathGizmoIcon = Echo::Name(#COMNAME);}\
	virtual ~COMNAME##Factory(){;}\
	virtual Echo::EchoSharedPtr<Echo::ActorComponent>		create(const Echo::ActorPtr& pActor)	override{ return Echo::EchoSharedPtr<Echo::ActorComponent>( new COMNAME(pActor,mMoveableType), Echo::ActorComponentPtrDeleter);}\
	virtual Echo::ActorComponentInfo*						createComponentInfo()	override{return new COMNAME##Info(mType,mMoveableType);}\
};\

//--------------------------------------------------------------------------------------------------//
//用于自动化生成指定Component的工厂类，该宏客户端使用不带_EchoComponentExport导出宏。
//第一个参数为Component的类名
//第二个参数为是否可以移动组件:只有*ALL_MOVEABLE的Component支持附加子Component。
//		EDITOR_MOVEABLE-编辑器模式下才生成node
//		CLIENT_MOVEABLE-客户端模式才生成node
//		UNMOVEABLE都不会生成node
//		ALL_MOVEABLE所有模式都有node
// 注意需要类型前面需要加上Echo
//--------------------------------------------------------------------------------------------------//
#define CLIENTDEFINECOMFACTORY(COMNAME,MOVEABLE) \
class COMNAME##Factory : public Echo::ActorComFactory\
{\
	static_assert(std::is_same<decltype(MOVEABLE),Echo::ActorCOMMoveableType>::value,"MOVEABLE must be ActorCOMMoveableType type!");\
public:\
	COMNAME##Factory():ActorComFactory(){ mType = Echo::Name(#COMNAME); mMoveableType = MOVEABLE; bIsEditorCom = false;}\
	virtual ~COMNAME##Factory(){;}\
	virtual Echo::EchoSharedPtr<Echo::ActorComponent>		create(const Echo::ActorPtr& pActor)	override{return Echo::EchoSharedPtr<Echo::ActorComponent>	(new COMNAME(pActor,mMoveableType), Echo::ActorComponentPtrDeleter);}\
	virtual Echo::ActorComponentInfo*						createComponentInfo()	override{return new COMNAME##Info(mType,mMoveableType);}\
};\

//指定GizmoTips图标的客户端component工厂类
#define CLIENTDEFINECOMFACTORYWITHGIZMO(COMNAME,MOVEABLE,GIZMOICONPATH) \
class COMNAME##Factory : public Echo::ActorComFactory\
{\
	static_assert(std::is_same<decltype(MOVEABLE),Echo::ActorCOMMoveableType>::value,"MOVEABLE must be ActorCOMMoveableType type!");\
public:\
	COMNAME##Factory():ActorComFactory(){ mType = Echo::Name(#COMNAME); mMoveableType = MOVEABLE; bIsEditorCom = false; mPathGizmoIcon = Echo::Name(GIZMOICONPATH);}\
	virtual ~COMNAME##Factory(){;}\
	virtual Echo::EchoSharedPtr<Echo::ActorComponent>		create(const Echo::ActorPtr& pActor)	override{ return Echo::EchoSharedPtr<Echo::ActorComponent>	(new COMNAME(pActor,mMoveableType), Echo::ActorComponentPtrDeleter);}\
	virtual Echo::ActorComponentInfo*						createComponentInfo()	override{return new COMNAME##Info(mType,mMoveableType);}\
};\

//注册工厂类使用函数
#define REGISTEROMFACTORY(COMNAME) Echo::ActorComFactoryManager::getSingleton().RegisterFactoryFunc(Echo::Name(#COMNAME), new COMNAME##Factory());

#ifndef UNREGISTEROMFACTORY
//取消注册工厂类使用函数,非dll中注册的组件请勿使用此宏
#define UNREGISTEROMFACTORY(COMNAME) Echo::ActorComFactoryManager::getSingleton().UnRegisterFactoryFunc(Echo::Name(#COMNAME));
#endif // !UNREGISTEROMFACTORY


//Info参数声明
#ifndef _DECLCOMINFO_PARAM_BEGIN
#define _DECLCOMINFO_PARAM_BEGIN(_COMPONENT_TYPE) \
using _SelfCompType = _COMPONENT_TYPE;\
using _SelfCompInfoType = _COMPONENT_TYPE##Info;
#endif // !_DECLCOMINFO_PARAM_BEGIN

/*
	_DATATYPE 变量类型
	_NAME 变量名
	_VALUE 变量初始值
	_COM_SET_FUNC 组件设置变量接口
*/
#ifndef _DECLCOMINFO_PARAM
#define _DECLCOMINFO_PARAM(_DATATYPE, _NAME, _VALUE, _COM_SET_FUNC) \
public: \
inline const _DATATYPE& Get##_NAME() { return m_##_NAME; } \
inline void Set##_NAME(const _DATATYPE& value) { m_##_NAME = value; } \
void Set##_NAME(const _DATATYPE& value, _SelfCompType* pComp) { COMPONENT_PARAM_CHANGE(_SelfCompType, pComp, _SelfCompInfoType, this ,m_##_NAME, value,_COM_SET_FUNC); } \
private: \
_DATATYPE m_##_NAME = _VALUE;
#endif // !_DECLCOMINFO_PARAM

#ifndef _DECLCOMINFO_PARAM_SIM
#define _DECLCOMINFO_PARAM_SIM(_DATATYPE, _NAME, _VALUE, _COM_SET_FUNC) \
public: \
inline _DATATYPE Get##_NAME() { return m_##_NAME; } \
inline void Set##_NAME(_DATATYPE value) { m_##_NAME = value; } \
void Set##_NAME(_DATATYPE value, _SelfCompType* pComp) { COMPONENT_PARAM_CHANGE(_SelfCompType, pComp, _SelfCompInfoType, this ,m_##_NAME, value,_COM_SET_FUNC); } \
private: \
_DATATYPE m_##_NAME = _VALUE;
#endif // !_DECLCOMINFO_PARAM_SIM

//Component的Set函数名与ComponentInfo的Set函数名相同的宏
#ifndef _DECLCOMINFO_PARAMS
#define _DECLCOMINFO_PARAMS(_DATATYPE, _NAME, _VALUE) \
public: \
inline const _DATATYPE& Get##_NAME() { return m_##_NAME; } \
inline void Set##_NAME(const _DATATYPE& value) { m_##_NAME = value; } \
void Set##_NAME(const _DATATYPE& value, _SelfCompType* pComp) { COMPONENT_PARAM_CHANGE(_SelfCompType, pComp, _SelfCompInfoType, this ,m_##_NAME, value, Set##_NAME); } \
private: \
_DATATYPE m_##_NAME = _VALUE;
#endif // !_DECLCOMINFO_PARAMS

//Component的Set函数名与ComponentInfo的Set函数名相同的宏
#ifndef _DECLCOMINFO_PARAMS_SIM
#define _DECLCOMINFO_PARAMS_SIM(_DATATYPE, _NAME, _VALUE) \
public: \
inline _DATATYPE Get##_NAME() { return m_##_NAME; } \
inline void Set##_NAME(_DATATYPE value) { m_##_NAME = value; } \
void Set##_NAME(_DATATYPE value, _SelfCompType* pComp) { COMPONENT_PARAM_CHANGE(_SelfCompType, pComp, _SelfCompInfoType, this ,m_##_NAME, value, Set##_NAME); } \
private: \
_DATATYPE m_##_NAME = _VALUE;
#endif // !_DECLCOMINFO_PARAMS_SIM

#ifndef _DECLCOMINFO_PARAM_END
#define _DECLCOMINFO_PARAM_END() \
public: \
EDITOR_REFLECT
#endif // !_DECLCOMINFO_PARAM_END





#endif




