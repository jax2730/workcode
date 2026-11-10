#include "LogicWrapper.h"
#include "CmdChain.h"
#include <thread>
#include <mutex>
#include <chrono>
using namespace std;
using namespace std::chrono;

#ifndef __APPLE__
static thread s_LogicThread;
void newThread(std::function<void()> workFunc)
{
	s_LogicThread = thread(workFunc);
}
#else
#import <Foundation/Foundation.h>
static NSThread* s_LogicThread = nil;
void newThread(std::function<void()> workFunc)
{
	s_LogicThread = [[NSThread alloc] initWithBlock:^{
		workFunc();
	}];
	s_LogicThread.stackSize = 16384 * 64;
	[s_LogicThread start];
}
#endif

LogicWrapper::LogicWrapper(Barrier* pBarrier, CmdChain* pLogicChain, CmdChain* pMainChain, CmdChain* pCacheChain)
	: bExit(false)
	, m_pBarrier(pBarrier)
	, m_pLogicChain(pLogicChain)
	, m_pMainChain(pMainChain)
	, m_pCacheChain(pCacheChain)
{
}

void LogicWrapper::ShutDown()
{
	if (!bExit)
	{
		bExit = true;
		m_pBarrier->SetExitFlag();
#ifndef __APPLE__
		s_LogicThread.join();
#else
		while (!s_LogicThread.finished)
			[NSThread sleepForTimeInterval : 0.2];
#endif
	}
}

#ifdef _WIN32

#include <windows.h>

typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // must be 0x1000
	LPCSTR szName; // pointer to name (in user addr space)
	DWORD dwThreadID; // thread ID (-1=caller thread)
	DWORD dwFlags; // reserved for future use, must be zero
} THREADNAME_INFO;

void SetThreadName(const char* szThreadName)
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = szThreadName;
	info.dwThreadID = -1;
	info.dwFlags = 0;

	__try
	{
		RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (const ULONG_PTR*)&info);
	}
	__except (EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}

#elif defined __ANDROID__

#include <sys/prctl.h>
#include <pthread.h>

void SetThreadName(const char *szThreadName)
{
	prctl(PR_SET_NAME, szThreadName);
}

#elif defined(__APPLE__)

void SetThreadName(const char* szThreadname)
{
	[[NSThread currentThread] setName:[NSString stringWithUTF8String : szThreadname]];
}

#else

void SetThreadName(const char* szThreadname)
{

}

#endif
