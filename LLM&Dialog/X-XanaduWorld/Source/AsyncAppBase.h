#pragma once
#include <memory>
#include "LogicWrapper.h"
#include "CmdChain.h"
#include "AppBase.h"

class AsyncAppBase: public AppBase
{
public:
	AsyncAppBase();
	virtual ~AsyncAppBase();

	//启动逻辑线程
	template<typename T,typename ...Args>
	void StartUp(Args&& ...args)
	{
		m_pBarrier->Reset();
		m_pLogicWrapper->Startup<T>(s_LogicThreadId,std::forward<Args>(args)...);
		//启动时 主线程先等一下, 就可以利用barrier实现下面的效果
		//logic       |----1----|-----2----|  |------3-----|
		//render |wait|         |-----1-------|-----2---|  |-----3----|
		//使 逻辑第n帧 与 渲染第n-1帧 先结束的等一下对方.
		// 主线程不等了，返回后在逻辑线程未完成初始化之前走空tick --- by snail_kbb
		// 鉴于现在逻辑线程初始化速度加快，可以让渲染线程等了 --- by snail_kbb
		//m_pBarrier->Wait();
		m_pLogicCmdChain->Begin();
		//m_pCacheCmdChain->Begin();
	}

	void onTick();
	//中止逻辑线程
	void ShutDown();

#ifdef CMD_STATISTIC
#define PostToMain setNextEngineCmdInfo(__FILE__,__LINE__); get_appview()->postToMain_
#define PostToEngine setNextEngineCmdInfo(__FILE__,__LINE__); get_appview()->postToEngine_
#define PostToLogic setNextLogicCmdInfo(__FILE__,__LINE__); get_appview()->postToLogic_


	void setNextLogicCmdInfo(const char* file, int line);
	void postToLogic_(const std::function<void()>& cmd);
	void postToLogic_(std::function<void()>&& cmd);

	void postToCache_(const std::function<void()>& cmd);
	void postToCache_(std::function<void()>&& cmd);

	void setNextEngineCmdInfo(const char* file, int line);
	void postToEngine_(const std::function<void()>& cmd);
	void postToEngine_(std::function<void()>&& cmd);

	void postToMain_(const std::function<void()>& cmd);
	void postToMain_(std::function<void()>&& cmd);
#else
	void postToLogic(const std::function<void()>& cmd);
	void postToEngine(const std::function<void()>& cmd);
	void postToEngine(std::function<void()>&& cmd);
	void postToMain(std::function<void()>&& cmd);
#endif

	int GetFrame() const
	{
		return m_Frame;
	}
	void ClearFrame()
	{
		m_Frame = 0;
	}

	const cmd_statistic& GetMainCmdStatistic() const
	{
		if (m_uEngineRenderMode == 3)// Echo::ECHO_THREAD_MT1
			return m_engineCmd_infos;
		else
			return m_pMainCmdChain->GetCmdStatistic();
	}

	void SetStatistic(bool open)
	{
		m_bCmdStatistic = open;
		m_pMainCmdChain->SetStatistic(open);
	}

	std::chrono::nanoseconds GetAllTime() const
	{
		return m_AllTime;
	}
	void ClearAllTime()
	{
		m_AllTime = std::chrono::nanoseconds(0);
	}

	std::chrono::nanoseconds GetCmdTime() const
	{
		return m_CmdTime;
	}

	int GetCmdCount() const 
	{
		return m_CmdCount;
	}
	std::chrono::nanoseconds GetRenderTime() const
	{
		return m_RenderTime;
	}
	void ClearRenderTime()
	{
		m_RenderTime = std::chrono::nanoseconds(0);
	}
	std::chrono::nanoseconds GetRubbishTime() const
	{
		return m_RubbishTime;
	}
	void ClearRubbishTime()
	{
		m_RubbishTime = std::chrono::nanoseconds(0);
	}
	void ClearCmdTime()
	{
		m_CmdTime = std::chrono::nanoseconds(0);
	}
	void ClearCmdCount()
	{
		m_CmdCount = 0;
	}
	std::chrono::nanoseconds GetLogicTime() const
	{
		if (m_pLogicWrapper)
		{
			return m_pLogicWrapper->GetLogicTime();
		}
		return std::chrono::nanoseconds(0);
	}
	void ClearLogicTime()
	{
		if (m_pLogicWrapper)
		{
			m_pLogicWrapper->ClearLogicTime();
		}
	}

	bool GetLogicThreadInited() const { return m_LogicThreadInited; }

	unsigned int  GetEngineRenderMode() const { return m_uEngineRenderMode; }
	
protected:
	virtual void OnTick(float dt) = 0;
	virtual void onFrameEnd() {}
	//virtual void setSwapBufferTime(unsigned int time) = 0;

protected:
	unsigned int m_uEngineRenderMode;

private:
	void EndFrame();
	void RenderTick();
	static std::thread::id			s_LogicThreadId;
	static std::thread::id			s_MainThreadId;
	bool							m_LogicThreadInited;
	std::unique_ptr<LogicWrapper>	m_pLogicWrapper;
	std::unique_ptr<Barrier>		m_pBarrier;
	std::unique_ptr<CmdChain>		m_pLogicCmdChain;
	std::unique_ptr<CmdChain>		m_pMainCmdChain;
	std::unique_ptr<CmdChain>		m_pCacheCmdChain;
	
	cmd_statistic					m_engineCmd_infos;
	const char*						m_engineCmd_file = nullptr;
	int								m_engineCmd_line = 0;
	bool							m_bCmdStatistic = false;
	//*******************************************************************************************************
	// 帧率相关
	int								m_Frame = 0;
	int								m_CmdCount = 0;			// 命令数量
	std::chrono::nanoseconds		m_AllTime{ 0 };			// 核算期内总时间
	std::chrono::nanoseconds		m_RenderTime{ 0 };		// 核算期内总计渲染时间
	std::chrono::nanoseconds		m_RubbishTime{ 0 };		// 垃圾时间
	std::chrono::nanoseconds		m_CmdTime{ 0 };			// 命令耗时
	//*******************************************************************************************************
};

//#define USE_MULTITHREAD 1
