//-----------------------------------------------------------------------------
// File:   EchoScriptManager.cpp
//
// Author: chenanzhi
//
// Date:  2022-3-3
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
//#define ELUA_LIB
#include "EchoScriptManager.h"
#include "EchoStringConverter.h"
#include "EchoLogManager.h"

#include <mutex>
#include <atomic>




//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------
namespace Echo
{


	//size_t thread_local g_curScriptIdx;

	//std::list<ScriptManager*> g_allScriptManager;
	ScriptManager thread_local g_scriptManager;

	//static std::mutex g_mutex;
	//static std::atomic<int> nextVersion{ 0 };

	/*int onError(elua_State* L)
	{
		elua_Debug info;
		std::string errMsg(elua_tostring(L, -1));
		errMsg.append("\n");
		for (int level = 1; elua_getstack(L, level, &info) == 1; ++level)
		{
			elua_getinfo(L, "Sl", &info);
			errMsg.append("\t");
			errMsg.append(info.short_src);
			errMsg.append(":");
			errMsg.append(StringConverter::toString(info.currentline));
			errMsg.append("\n");
		}
		elua_pushstring(L, errMsg.c_str());
		return 1;
	}*/



	ScriptManager::ScriptManager()
		/*: m_L(nullptr)
		, m_version(-1)*/
	{
		
	}
	
	void ScriptManager::init()
	{
		/*{
			std::lock_guard<std::mutex> lock(g_mutex);
			g_curScriptIdx = g_allScriptManager.size();
			g_allScriptManager.push_back(this);
		}

		m_L = eluaL_newstate();
		eluaopen_base(m_L);
		eluaL_openlibs(m_L);
		m_version = ++nextVersion;

		(*(ScriptManager**)elua_getextraspace(m_L)) = this;*/
	}

	void ScriptManager::deinit()
	{
		/*std::lock_guard<std::mutex> lock(g_mutex);
		g_allScriptManager.remove(this);
		if (m_L)
			elua_close(m_L);

		m_L = nullptr;*/
	}

	void ScriptManager::frameStart(float dt)
	{

	}

	void ScriptManager::frameEnd()
	{

	}

	/*elua_State* ScriptManager::getLuaState() const
	{
		return m_L;
	}*/

//	bool ScriptManager::runScript(const String& file)
//	{
//		DataStreamPtr stream(Root::instance()->GetMemoryDataStream(file.c_str()));
//		if (stream.isNull() || stream->size() == 0)
//		{
//			LogManager::instance()->logMessage("script not exist : " + file, LML_CRITICAL);
//			return false;
//		}
//
//		if (!m_L)
//			return false;
//
//		elua_pushcfunction(m_L, onError);
//
//		// 加载lua
//		int err = eluaL_loadbuffer(m_L, stream->getData(), stream->size(), file.c_str());
//		if (err != 0)
//		{
//			// 获取栈顶错误信息
//			const char * error = elua_tostring(m_L, -1);
//			// 打印
//			LogManager::instance()->logMessage("Script Error: "+ String(error) , LML_CRITICAL);
//			elua_pop(m_L, 2);
//			return false;
//		}
//
//		// 执行脚本
//		if (0 == elua_pcall(m_L, 0, 0, -2))
//			elua_pop(m_L, 1);
//		else
//		{
//			// 获取栈顶错误信息
//			const char * error = elua_tostring(m_L, -1);
//			// 打印
//			LogManager::instance()->logMessage("***Script Error: "+ String(error) , LML_CRITICAL);
//			elua_pop(m_L, 2);
//			return false;
//		}
//
//		return true;
//	}
//
//	int ScriptManager::runScript(elua_State* L)
//	{
//		size_t len = 0;
//		const char* _filepath = elua_tolstring(L, 1, &len);
//		if (!_filepath)
//		{
//			return 1;
//		}
//
//		DataStreamPtr stream(Root::instance()->GetMemoryDataStream(_filepath));
//		if (stream.isNull() || stream->size() == 0)
//		{
//			LogManager::instance()->logMessage("script not exist : " + String(_filepath), LML_CRITICAL);
//			return 1;
//		}
//
//		elua_pushcfunction(L, onError);
//
//		// 加载lua
//		int err = eluaL_loadbuffer(L, stream->getData(), stream->size(), _filepath);
//
//		if (err != 0)
//		{
//			// 获取栈顶错误信息
//			const char * error = elua_tostring(L, -1);
//			// 打印
//			LogManager::instance()->logMessage("Script Error: " + String(error), LML_CRITICAL);
//			elua_pop(L, 2);
//			return 1;
//		}
//
//		// 设置环境
//		if (!elua_isnone(L, 2))
//		{
//			elua_pushvalue(L, 2);
//			if (!elua_setupvalue(L, -2, 1))
//			{
//				// remove 'env' if not used by previous call 
//				elua_pop(L, 3);
//				return 1;
//			}
//		}
//		// 执行脚本
//		if (0 == elua_pcall(L, 0, 0, -2))
//			elua_pop(L, 1);
//		else
//		{
//			// 获取栈顶错误信息
//			const char * error = elua_tostring(L, -1);
//			// 打印
//			LogManager::instance()->logMessage("***Script Error: " + String(error), LML_CRITICAL);
//			elua_pop(L, 2);
//			return 1;
//		}
//
//		return 0;
//	}
//
//	int ScriptManager::loadScript(elua_State* L)
//	{
//		size_t len = 0;
//		const char* _filepath = elua_tolstring(L, 1, &len);
//		if (!_filepath)
//		{
//			return 1;
//		}
//				
//		DataStreamPtr stream(Root::instance()->GetMemoryDataStream(_filepath));
//		if (stream.isNull() || stream->size() == 0)
//		{
//			LogManager::instance()->logMessage("script not exist : " + String(_filepath), LML_CRITICAL);
//			return 1;
//		}
//
//		// 加载lua
//		int err = eluaL_loadbuffer(L, stream->getData(), stream->size(), _filepath);
//		if (err != 0)
//		{
//			// 获取栈顶错误信息
//			const char * error = elua_tostring(L, -1);
//			// 打印
//			LogManager::instance()->logMessage("Script Error: " + String(error), LML_CRITICAL);
//			elua_pop(L, 1);
//			return 1;
//		}
//
//		return 0;
//	}
//
//	void ScriptManager::executeScript(const String& _script)
//	{
//		int err = eluaL_dostring(m_L, _script.c_str());
//		if (err)
//		{
//			// 获取栈顶错误信息
//			const char * error = elua_tostring(m_L, -1);
//			// 打印
//			LogManager::instance()->logMessage("Script Error: " + String(error), LML_CRITICAL);
//			elua_pop(m_L, 1);
//		}
//	}
//
	bool ScriptManager::LoadScriptModule(const std::string& scriptName)
	{
//#ifdef _WIN32
//		//RegistScriptReloadModule(_filepath, _module, _package);
//#endif
//		return ReloadScriptModule(scriptName);
		return false;
	}
//
	bool ScriptManager::ReloadScriptModule(const std::string& scriptName)
	{
//		if (scriptName.empty())
//		{
//			return false;
//		}
//		std::size_t posstart = scriptName.find_last_of("/\\");
//		std::size_t posend = scriptName.find_last_of(".");
//
//		if (posend== std::string::npos || posstart == std::string::npos)
//		{
//			return false;
//		}
//		std::string packageName = scriptName.substr(posstart+1, posend - posstart-1);
//		const char* _package = packageName.c_str();
//		const char* _filepath = scriptName.c_str();
//
//
//
//		DataStreamPtr stream(Root::instance()->GetMemoryDataStream(scriptName.c_str()));
//		if (stream.isNull() || stream->size() == 0)
//		{
//			LogManager::instance()->logMessage("script not exist : " + scriptName, LML_CRITICAL);
//			return false;
//		}
//
//		if (!m_L)
//			return false;
//
//		// 设置元表
//		int topSet = elua_gettop(m_L);
//		elua_getglobal(m_L, "_G");
//		elua_getfield(m_L, -1, _package);
//		if (!elua_istable(m_L, -1))
//		{
//			elua_pop(m_L, 1);
//			// 创建模块的 table 
//			elua_newtable(m_L);
//			// metatable
//			elua_newtable(m_L);
//			elua_getglobal(m_L, "_G");
//			elua_setfield(m_L, -2, "__index");
//			// metatable设置为模块表的元表
//			elua_setmetatable(m_L, -2);
//			// 将模块表设置为_G表的属性
//			elua_setfield(m_L, -2, _package);
//		}
//		else
//		{
//			elua_pop(m_L, 1);
//		}
//
//		elua_pushcfunction(m_L, onError);
//
//		// 加载lua
//		int err = eluaL_loadbuffer(m_L, stream->getData(), stream->size(), _filepath);
//
//		if (err != ELUA_OK)
//		{
//			// 获取栈顶错误信息
//			const char * error = elua_tostring(m_L, -1);
//			LogManager::instance()->logMessage("Script Error: " + String(error), LML_CRITICAL);
//			// 打印
//			//CSCRIPTERROR("Lua Error:%s\n", error);
//
//			int topCur = elua_gettop(m_L);
//			elua_pop(m_L, topCur - topSet);
//			return false;
//		}
//
//		elua_getfield(m_L, -3, _package);
//
//		// 设置环境
//		if (!elua_isnone(m_L, -1))//确保模块表不是空不是无效值
//		{
//			if (!elua_setupvalue(m_L, -2, 1))
//			{
//				// remove 'env' if not used by previous call 
//				int topCur = elua_gettop(m_L);
//				elua_pop(m_L, topCur - topSet);
//				return false;
//			}
//		}
//		// 执行脚本
//		if (0 != elua_pcall(m_L, 0, 0, -2))
//		{
//			// 获取栈顶错误信息
//			const char * error = elua_tostring(m_L, -1);
//
//			LogManager::instance()->logMessage("Script Error: " + String(error), LML_CRITICAL);
//			// 打印
//			//CSCRIPTERROR("Lua Error:%s\n", error);
//
//			int topCur = elua_gettop(m_L);
//			elua_pop(m_L, topCur - topSet);
//			return false;
//		}
//
//		int topCur = elua_gettop(m_L);
//		elua_pop(m_L, topCur - topSet);
//		m_version = ++nextVersion;
//		return true;
		return false;
	}

	ScriptManager::~ScriptManager()
	{
		deinit();
	}

//	void scriptPushGlobal(elua_State *L, char const* name)
//	{
//		if (!name)
//		{
//			elua_pushnil(L);
//			return;
//		}
//#define LOCAL_NAME_MAX 128
//		static char namebuff[LOCAL_NAME_MAX];
//		char const * str_beg = name;
//		char const * str_end = strchr(str_beg, '.');
//		if (str_end == NULL)
//		{
//			elua_getglobal(L, name);
//		}
//		else
//		{
//			if (str_end - str_beg > LOCAL_NAME_MAX)
//			{
//				elua_pushstring(L, "field name too large!!!");
//				elua_error(L);
//				elua_pushnil(L);
//				return;
//			}
//			memcpy(namebuff, str_beg, str_end - str_beg);
//			namebuff[str_end - str_beg] = 0;
//			elua_getglobal(L, namebuff);
//
//			if (!elua_istable(L, -1))
//			{
//				elua_pop(L, 1);
//				elua_pushnil(L);
//				return;
//			}
//
//			str_beg = str_end + 1;
//			str_end = NULL;
//
//			while (*str_beg != '\0')
//			{
//				str_end = strchr(str_beg, '.');
//				if (str_end)
//				{
//					if (str_end - str_beg > LOCAL_NAME_MAX)
//					{
//						elua_pushstring(L, "field name too large!!!");
//						elua_error(L);
//						elua_pushnil(L);
//						elua_replace(L, -2);
//						break;
//					}
//					memcpy(namebuff, str_beg, str_end - str_beg);
//					namebuff[str_end - str_beg] = 0;
//					elua_getfield(L, -1, namebuff);
//				}
//				else
//				{
//					elua_getfield(L, -1, str_beg);
//				}
//
//				elua_replace(L, -2);
//				if (str_end == NULL)
//				{
//					break;
//				}
//				if (!elua_istable(L, -1))
//				{
//					elua_pushnil(L);
//					elua_replace(L, -2);
//					break;
//				}
//
//				str_beg = str_end + 1;
//				str_end = NULL;
//			}
//		}
//	}
	ScriptManager * GetScriptManager()
	{
		return &g_scriptManager;
	}

} // namespace Echo

//-----------------------------------------------------------------------------
//	Cpp File End
//-----------------------------------------------------------------------------
