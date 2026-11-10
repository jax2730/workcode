//-----------------------------------------------------------------------------
// File:   EchoScriptManager.h
//
// Author: chenanzhi
// Desc:
// Date:  2022-3-3
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef __EchoScriptManager_H__
#define __EchoScriptManager_H__

#include "EchoComponentPrerequisites.h"
//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------
//struct elua_State;

//#include "eLuaBridge.h"
namespace Echo
{
//
//#define ECHO_REG_SCRIPT_BEGIN								getGlobalNamespace(GetScriptManager()->getLuaState())
//#define ECHO_REG_SCRIPT_END									;
//
//#define ECHO_REG_NAMESPACE_BEGIN( __namespace )				.beginNamespace( #__namespace )
//#define ECHO_REG_NAMESPACE_END								.endNamespace()
//
//#define ECHO_REG_FREE_VAR( __var )							.addVariable( #__var, &__var, false )
//#define ECHO_REG_FREE_POINT( __var )						.addVariable( #__var, __var, false )
//#define ECHO_REG_FREE_VAR_WRITE( __var )					.addVariable( #__var, &__var )
//#define ECHO_REG_FREE_FUNC( __func )						.addFunction( #__func, &__func)
//#define ECHO_REG_FREE_CFUNC( __func )						.addCFunction( #__func, &__func)
//#define ECHO_REG_FREE_FUNC_EXT( __name, __func )			.addFunction( #__name, &__func)
//#define ECHO_REG_FREE_CFUNC_EXT( __name,__func )			.addCFunction( #__name, &__func)
//#define ECHO_REG_CLASS_FREE_CFUNC( __class,__name)			.addCFunction( #__name, &__class::__name)
//
//#define ECHO_REG_CLASS_BEGIN( __class )						.beginClass<__class>( #__class )
//#define ECHO_REG_CLASS_BEGIN_EXT( __class, __name )			.beginClass<__class>( #__name )
//#define ECHO_REG_CLASS_DERIVE( __classB, __classA)			.deriveClass <__classB, __classA>(#__classB)
//#define ECHO_REG_CLASS_DERIVE_EXT(__classB,__classA, __name)\
//															.deriveClass <__classB, __classA>(#__name)
//#define ECHO_REG_CLASS_END									.endClass()
//#define ECHO_REG_CLASS_DATA( __class, __data )				.addData( #__data, &__class::__data )
//#define ECHO_REG_CLASS_DATA_EXT( __class, __data, __name)	.addData( #__name, &__class::__data )//成员变量重命名
//#define ECHO_REG_CLASS_PROPERTY_R( __class, __getter, __name) .addProperty(#__name, &__class::__getter)
//#define ECHO_REG_CLASS_PROPERTY( __class, __getter, __setter, __name) .addProperty(#__name, &__class::__getter, &__class::__setter)
//#define ECHO_REG_CLASS_FUNC( __class, __func )				.addFunction( #__func, &__class::__func )
//#define ECHO_REG_CLASS_FUNC_EXT( __class, __func, __name )	.addFunction( #__name, &__class::__func )//函数重命名
//#define ECHO_REG_CLASS_CFUNC( __class, __func )				.addCFunction( #__func, &__class::__func )
//#define ECHO_REG_CLASS_CFUNC_EXT( __class, __func, __name )	.addCFunction( #__name, &__class::__func )
//#define ECHO_REG_CLASS_FUNC_EXT_PARAM( __class, __name, __func, __ret, __param )\
//	.addFunction<__ret (__class::*)( __param )>( #__name, &__class::__func )//设置重载
//#define ECHO_REG_CLASS_STATIC_DATA( __class, __data ) 		.addStaticData( #__data, &__class::__data )
//#define ECHO_REG_CLASS_STATIC_FUNC( __class, __func ) 		.addStaticFunction( #__func, &__class::__func )
//#define ECHO_REG_CLASS_STATIC_FUNC_EXT( __class, __func, __name) \
//															.addStaticFunction( #__name, &__class::__func )
//#define ECHO_REG_CLASS_STATIC_CFUNC( __class, __func ) 		.addStaticCFunction( #__func, &__class::__func )
//#define ECHO_REG_CLASS_OPERATOR_CALL(__class)				REG_CLASS_FUNC_EXT(__class, operator(), __call)
//
//	// 5个构造函数只能注册一个,因为lua表是字串映射，一个名字只能对应一个函数对象
//#define ECHO_REG_CLASS_DEFAULT_CONSTRUCTOR					.addConstructor<void(*)(void)>()
//#define ECHO_REG_CLASS_CONSTRUCTOR_1( T )					.addConstructor<void(*)(T)>()
//#define ECHO_REG_CLASS_CONSTRUCTOR_2( T , S )				.addConstructor<void(*)(T, S)>()
//#define ECHO_REG_CLASS_CONSTRUCTOR_3( T , S , W)			.addConstructor<void(*)(T, S, W)>()
//#define ECHO_REG_CLASS_CONSTRUCTOR_4( T , S , W, Z)			.addConstructor<void(*)(T, S, W, Z)>()
//
//
//#define ECHO_REG_CLASS_FUNCTION(__class,__func) ECHO_REG_SCRIPT_BEGIN\
//	ECHO_REG_CLASS_BEGIN(__class)\
//	ECHO_REG_CLASS_FUNC( __class, __func )\
//	ECHO_REG_CLASS_END\
//	ECHO_REG_SCRIPT_END
//
//#define ECHO_REG_CLASS_STATIC_FUNCTION(__class,__func) ECHO_REG_SCRIPT_BEGIN\
//	ECHO_REG_CLASS_BEGIN(__class)\
//	ECHO_REG_CLASS_STATIC_FUNC( __class, __func )\
//	ECHO_REG_CLASS_END\
//	ECHO_REG_SCRIPT_END
	
	


	class ScriptManagerPrivate;

	class _EchoComponentExport ScriptManager
	{
	public:
		ScriptManager();
		~ScriptManager();

		ScriptManager(const ScriptManager &) = delete;
		ScriptManager(ScriptManager &&) = delete;
		ScriptManager & operator = (const ScriptManager &) = delete;
		ScriptManager & operator = (ScriptManager &&) = delete;

		void init();
		void deinit();

		void frameStart(float dt);
		void frameEnd();

	//	elua_State* getLuaState() const;

	//	//加载并执行脚本 CPP用
	//	bool runScript(const String& file);

	//	//加载并执行脚本(脚本用) 返回 0:成功    1:失败
	//	int	 runScript(elua_State* L);

	//	//用作脚本内加载脚本 返回 0:成功    1:失败
	//	int loadScript(elua_State* L);

	//	//执行脚本内容
	//	void executeScript( const String& _script );

		bool LoadScriptModule(const std::string& scriptName);
		bool ReloadScriptModule(const std::string& scriptName);

	//	template<class T, class MemFn>
	//	void registerProtocolFunc(const char* className,char const* funName, MemFn mf);

	//public:
	//	elua_State *	m_L;
	//	int			m_version;
	};


	/*template<typename T, bool isArray = std::is_array<typename std::remove_reference<T>::type>::value>
	struct ScriptPushHelper;

	template<typename T>
	struct ScriptPushHelper<T, true>
	{
		static inline void push(elua_State* L, T v)
		{
			eLuaBridge::Stack<typename std::add_pointer<typename std::remove_extent<typename std::remove_reference<T>::type>::type>::type>::push(L, v);
		}
	};

	template<typename T>
	struct ScriptPushHelper<T, false>
	{
		static inline void push(elua_State* L, T&& v)
		{
			eLuaBridge::Stack<T>::push(L, std::forward<T>(v));
		}
	};

	inline void scriptPushValue(elua_State* L)
	{}

	template <typename T, typename... Args>
	inline void scriptPushValue(elua_State* L, T&& v, Args&&... args)
	{
		ScriptPushHelper<T>::push(L, std::forward<T>(v));
		scriptPushValue(L, std::forward<Args>(args)...);
	}
	int onError(elua_State*L);
	void scriptPushGlobal(elua_State *L, char const* name);*/
	_EchoComponentExport ScriptManager * GetScriptManager();
} // namespace Echo

//-----------------------------------------------------------------------------
//	Head File End
//-----------------------------------------------------------------------------
#endif