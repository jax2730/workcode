#pragma once
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#endif
#include "Barrier.h"
#include "CmdChain.h"
#include <thread>
#include <mutex>
#include <chrono>
#include <iostream>
#include "RewindHeap.h"

void SetThreadName(const char *szThreadName);
void newThread(std::function<void()> workFunc);

class LogicWrapper 
{
public:
	LogicWrapper(Barrier* pBarrier, CmdChain* pLogicChain,CmdChain* pMainChain, CmdChain* pCacheChain);

	template<typename T,typename ...Args>
	void Startup(std::thread::id& logicThreadId,Args&& ...args)
	{
		bInited = false;
		bExit = false;
		newThread([&logicThreadId, this, args...]() {
			logicThreadId = std::this_thread::get_id();
			this->LogicThreadMain<T>(args...);
		});
	}

	template<typename T,typename ...Args>
	void LogicThreadMain(Args&& ...args)
	{
		using namespace std;
		using namespace std::chrono;

		SetThreadName("LogicThread");
		m_pMainChain->Begin();
		{
			//用大括号限定一下 Logic的生命周期
			T Logic(std::forward<Args>(args)...);
			bInited = true;
			m_pMainChain->End();
			m_pBarrier->Wait();
			if (!bExit)
			{
				m_pMainChain->Begin();
			}
			while (!bExit)
			{
				//*******************************************************************************************************
				// 逻辑帧率开始
				static auto lastLogicBegin = steady_clock::now();
				auto logicBegin = steady_clock::now();
				//*******************************************************************************************************
				auto dt = logicBegin - lastLogicBegin;
				lastLogicBegin = logicBegin;
				float _dt = duration_cast<dt_time_t>(dt).count();
				Logic.PreTick();
				m_pLogicChain->Excute();
				//m_pCacheChain->Excute();
				Logic.OnTick(_dt);

				//*******************************************************************************************************
				// 逻辑帧率结束
				auto logicEnd = steady_clock::now();
				m_LogicTime += (logicEnd - logicBegin);
				//*******************************************************************************************************
				Logic.OnTickEnd();
				CmdMemAllocator::Global()->NextFrame();
				m_pMainChain->End();

				auto logicWaitStart = steady_clock::now();

				m_pBarrier->Wait();
				auto logicWaitEnd = steady_clock::now();

				auto waitTime = logicWaitEnd - logicWaitStart;
				m_LogicWaitTime = (int)waitTime.count() / 1000;

				if (!bExit)
				{
					m_pMainChain->Begin();
				}
			}
		}
		m_pMainChain->End();
		m_pBarrier->Wait();
		m_pMainChain->Begin();
	}

	void ShutDown();

	std::chrono::nanoseconds GetLogicTime() const
	{
		return m_LogicTime;
	}
	void ClearLogicTime()
	{
		m_LogicTime = std::chrono::nanoseconds(0);
	}
	int GetLogicWaitTime() const
	{
		return m_LogicWaitTime;
	}

	~LogicWrapper() = default;

	bool isInited()
	{
		return bInited;
	}

private:

	typedef std::chrono::duration<float> dt_time_t;

	std::thread m_LogicThread;
	bool bExit;
	bool bInited;

	Barrier* m_pBarrier;
	CmdChain* m_pLogicChain;
	CmdChain* m_pMainChain;
	CmdChain* m_pCacheChain;

	std::chrono::nanoseconds m_LogicTime{ 0 };		// 核算期内总计逻辑时间
	std::chrono::nanoseconds m_DTime{ 0 };			// 其他时间
	int m_LogicWaitTime{ 0 };  // 逻辑线程等待时间
};