#include "AsyncAppBase.h"
#include <chrono>

#ifdef CMD_STATISTIC
#undef PostToEngine
#undef PostToLogic

#define PostToEngine setNextEngineCmdInfo(__FILE__,__LINE__);postToEngine_
#define PostToLogic setNextLogicCmdInfo(__FILE__,__LINE__); postToLogic_
#endif

std::thread::id AsyncAppBase::s_LogicThreadId;
std::thread::id AsyncAppBase::s_MainThreadId;

AsyncAppBase::AsyncAppBase()
	: m_uEngineRenderMode(1)
	, m_LogicThreadInited(false)
{
	SetThreadName("MainThread");
	s_MainThreadId = std::this_thread::get_id();
	m_pBarrier = make_unique_ptr<Barrier>(2, std::bind(&AsyncAppBase::EndFrame, this));
	m_pLogicCmdChain = make_unique_ptr<CmdChain>();
	m_pMainCmdChain = make_unique_ptr<CmdChain>();
	m_pCacheCmdChain = make_unique_ptr<CmdChain>();

	m_pLogicWrapper = make_unique_ptr<LogicWrapper>(m_pBarrier.get(), m_pLogicCmdChain.get(),m_pMainCmdChain.get(),m_pCacheCmdChain.get());
}

AsyncAppBase::~AsyncAppBase()
{
}

void AsyncAppBase::onTick()
{
	using namespace std::chrono;

	RenderTick();

	if (m_pLogicWrapper->isInited())
	{
		m_pLogicCmdChain->End();
		//m_pCacheCmdChain->End();
		m_pBarrier->Wait();
		m_pLogicCmdChain->Begin();
		//m_pCacheCmdChain->Begin();

		m_LogicThreadInited = true;
	}
}

void AsyncAppBase::RenderTick()
{
	using namespace std::chrono;
	using dt_time_t = std::chrono::duration<float>;

	//*******************************************************************************************************
	// 渲染帧率开始
	static auto lastRenderBegin = steady_clock::now();
	auto renderBegin = steady_clock::now();

	static std::chrono::steady_clock::time_point lastRenderEnd = steady_clock::now();
	std::chrono::nanoseconds DTime = renderBegin - lastRenderEnd;
	//*******************************************************************************************************

	auto dt = renderBegin - lastRenderBegin;
	lastRenderBegin = renderBegin;
	m_pMainCmdChain->Excute();

	auto cmdEnd = steady_clock::now();
	auto cmdTime = cmdEnd - renderBegin;

	PostToLogic([this, cmdTime]() {
		m_CmdTime += cmdTime;
		m_CmdCount += (int)m_pMainCmdChain->GetCmdCount();
	});

	float _dt = duration_cast<dt_time_t>(dt).count();
	OnTick(_dt);

	//*******************************************************************************************************
	// 渲染帧率结束

	PostToLogic([this]() {
		m_Frame++;
		});

	auto renderEnd = steady_clock::now();
	auto lastdt = renderEnd - lastRenderEnd;
	lastRenderEnd = renderEnd;
	PostToLogic([this, lastdt]() {
		m_AllTime += lastdt;
		});

	std::chrono::nanoseconds render_time = renderEnd - renderBegin;
	PostToLogic([this, render_time]() {
		m_RenderTime += render_time;
		});

	PostToLogic([this, DTime]() {
		m_RubbishTime += DTime;
		});
	//*******************************************************************************************************
}

void AsyncAppBase::ShutDown()
{
	m_pLogicWrapper->ShutDown();
	m_pLogicCmdChain->End();
	//m_pCacheCmdChain->End();
	m_pBarrier->Wait();
	m_pLogicCmdChain->Begin();
	//m_pCacheCmdChain->Begin();
	//第一次tick 执行逻辑线程最后一帧的内容
	onTick();

	// todo：清理掉所有异步任务（有内存泄漏，待处理）
	m_pMainCmdChain->Begin();
	//第二次tick 执行 逻辑线程销毁时 applogic析构产生的命令
	//因为 第一次wait 和 第一次tick 已经交换buffer了 不需要再手动交换
	onTick();
	m_pMainCmdChain->SwapBuffer();
	m_pMainCmdChain->Begin();

	m_pLogicCmdChain->Begin();
	m_pLogicCmdChain->SwapBuffer();
	m_pLogicCmdChain->Begin();

	//m_pCacheCmdChain->Begin();
	//m_pCacheCmdChain->SwapBuffer();
	//m_pCacheCmdChain->Begin();

	m_LogicThreadInited = false;
}

#ifdef CMD_STATISTIC

void AsyncAppBase::setNextLogicCmdInfo(const char * file, int line)
{
	m_pLogicCmdChain->SetNextCmdInfo(file, line);
}

void AsyncAppBase::postToLogic_(const std::function<void()>& cmd)
{
	auto curThreadId = std::this_thread::get_id();
	assert(curThreadId == s_MainThreadId);
	m_pLogicCmdChain->Post(cmd);
}

void AsyncAppBase::postToLogic_(std::function<void()>&& cmd)
{
	auto curThreadId = std::this_thread::get_id();
	assert(curThreadId == s_MainThreadId);
	m_pLogicCmdChain->Post(std::forward<std::function<void()>>(cmd));
}

void AsyncAppBase::postToCache_(const std::function<void()>& cmd)
{
	m_pCacheCmdChain->Post(cmd);
}

void AsyncAppBase::postToCache_(std::function<void()>&& cmd)
{
	m_pCacheCmdChain->Post(std::forward<std::function<void()>>(cmd));
}

void AsyncAppBase::setNextEngineCmdInfo(const char * file, int line)
{
	if (m_uEngineRenderMode == 3)// Echo::ECHO_THREAD_MT1
	{
		if (m_bCmdStatistic)
		{
			m_engineCmd_file = file;
			m_engineCmd_line = line;
		}
	}
	else
	{
		m_pMainCmdChain->SetNextCmdInfo(file, line);
	}
}

void AsyncAppBase::postToEngine_(const std::function<void()>& cmd)
{
	auto curThreadId = std::this_thread::get_id();
	assert(curThreadId == s_LogicThreadId);

	if (m_uEngineRenderMode == 3)// Echo::ECHO_THREAD_MT1
	{
		if (m_bCmdStatistic)
		{
			m_engineCmd_infos.push(m_engineCmd_file, m_engineCmd_line);

			auto begin = std::chrono::steady_clock::now();

			cmd();

			auto end = std::chrono::steady_clock::now();

			auto time = end - begin;

			m_engineCmd_infos.add_time(m_engineCmd_file, m_engineCmd_line, time);
		}
		else
		{
			cmd();
		}
	}
	else
	{
		m_pMainCmdChain->Post(cmd);
	}
}
void AsyncAppBase::postToEngine_(std::function<void()>&& cmd)
{
	auto curThreadId = std::this_thread::get_id();
	assert(curThreadId == s_LogicThreadId);

	if (m_uEngineRenderMode == 3)// Echo::ECHO_THREAD_MT1
	{
		if (m_bCmdStatistic)
		{
			m_engineCmd_infos.push(m_engineCmd_file, m_engineCmd_line);

			auto begin = std::chrono::steady_clock::now();

			cmd();

			auto end = std::chrono::steady_clock::now();

			auto time = end - begin;

			m_engineCmd_infos.add_time(m_engineCmd_file, m_engineCmd_line, time);
		}
		else
		{
			cmd();
		}
	}
	else
	{
		m_pMainCmdChain->Post(std::forward<std::function<void()>>(cmd));
	}
}
void AsyncAppBase::postToMain_(const std::function<void()>& cmd)
{
	m_pMainCmdChain->Post(cmd);
}

void AsyncAppBase::postToMain_(std::function<void()>&& cmd)
{
	m_pMainCmdChain->Post(std::forward<std::function<void()>>(cmd));
}

#else
//只能从主线程投递到逻辑线程
void AsyncAppBase::postToLogic(const std::function<void()>& cmd)
{
	auto curThreadId = std::this_thread::get_id();
	assert(curThreadId == m_MainThreadId);
	m_pLogicCmdChain->Post(cmd);
}

//只能从逻辑线程投递到主线程
void AsyncAppBase::postToEngine(const std::function<void()>& cmd)
{
	auto curThreadId = std::this_thread::get_id();
	assert(curThreadId == m_LogicThreadId);

	if (m_uEngineRenderMode == 3)// Echo::ECHO_THREAD_MT1)
	{
		cmd();
	}
	else
	{
		m_pMainCmdChain->Post(cmd);
	}
}

void AsyncAppBase::postToEngine(std::function<void()>&& cmd)
{
	auto curThreadId = std::this_thread::get_id();
	assert(curThreadId == m_LogicThreadId);

	if (m_uEngineRenderMode == 3)// Echo::ECHO_THREAD_MT1)
	{
		cmd();
	}
	else
	{
		m_pMainCmdChain->Post(std::move(cmd));
	}
}
void AsyncAppBase::postToMain(std::function<void()>&& cmd)
{
	m_pMainCmdChain->Post(std::move(cmd));
}
#endif

void AsyncAppBase::EndFrame()
{
	m_pLogicCmdChain->SwapBuffer();
	m_pMainCmdChain->SwapBuffer();
	//m_pCacheCmdChain->SwapBuffer();
	m_engineCmd_infos.clear();
	onFrameEnd();
}

