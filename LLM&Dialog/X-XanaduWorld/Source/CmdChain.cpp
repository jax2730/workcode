#include "CmdChain.h"
#include <unordered_map>
#include "CmdBuffer.h"


cmd::cmd(const char* _file, int _line, std::function<void()> && _func)
	:file(_file), line(_line)
{
	std::swap(func, _func);
}

cmd::cmd(const char* _file, int _line, const std::function<void()>& _func)
	: file(_file), line(_line), func(_func)
{
}

void cmd::operator()() const
{
	func();
}


bool cmd_statistic::empty() const
{
	return m_cmd_infos.empty();
}

void cmd_statistic::clear()
{
	cmd_count = 0;
	cmd_time = std::chrono::nanoseconds(0);
	m_cmd_infos.clear();
}

size_t cmd_statistic::count() const
{
	return cmd_count;
}

const std::chrono::nanoseconds & cmd_statistic::time() const
{
	return cmd_time;
}

void cmd_statistic::push(const char* file, const int & line)
{
	m_cmd_infos[{file, line}].count += 1;
	cmd_count++;
}

void cmd_statistic::add_time(const char* file, const int & line, const std::chrono::nanoseconds & _time)
{
	m_cmd_infos[{file, line}].time += _time;
	cmd_time += _time;
}

void CmdChain::SetStatistic(bool open)
{
#ifdef CMD_STATISTIC
	m_bStatistic = open;
#endif
}


const cmd_statistic& CmdChain::GetCmdStatistic() const
{
#ifdef CMD_STATISTIC
	return m_cmd_infos;
#else
	static cmd_statistic info;
	return info;
#endif
}

void CmdChain::Post(const std::function<void()>& func)
{
#ifdef CMD_STATISTIC
	m_BackQuene.emplace_back(file,line,func);

	if (m_bStatistic)
	{
		m_cmd_infos_back.push(file, line);
	}
#else
	m_BackQuene.emplace_back(std::move(func));
#endif // CMD_STATISTIC

}

void CmdChain::Post(std::function<void()>&& func)
{
#ifdef CMD_STATISTIC
	m_BackQuene.emplace_back(file,line,std::forward<std::function<void()>>(func));

	if (m_bStatistic)
	{
		m_cmd_infos_back.push(file, line);
	}
#else
	m_BackQuene.emplace_back(std::move(func));
#endif // CMD_STATISTIC
}

#ifdef CMD_STATISTIC
void CmdChain::SetNextCmdInfo(const char* file, int line)
{
	if (m_bStatistic)
	{
		this->file = file;
		this->line = line;
	}
}
#endif

void CmdChain::SwapBuffer()
{
#ifdef CMD_STATISTIC
	std::swap(m_ForwardQuene, m_BackQuene);

	std::swap(m_cmd_infos, m_cmd_infos_forward);
	std::swap(m_cmd_infos_forward, m_cmd_infos_back);
#else
	std::swap(m_ForwardQuene, m_BackQuene);
#endif
}

void CmdChain::Begin()
{
#ifdef CMD_STATISTIC
	m_BackQuene.clear();
	m_cmd_infos_back.clear();
#else
	m_BackQuene.clear();
#endif
}

size_t CmdChain::GetCmdCount() const
{
	return m_ForwardQuene.size();
}

void CmdChain::Excute()
{
#ifdef CMD_STATISTIC
	for (auto& _cmd : m_ForwardQuene)
	{
		try
		{
			if (m_bStatistic)
			{
				auto begin = std::chrono::steady_clock::now();

				_cmd();
				auto end = std::chrono::steady_clock::now();

				auto time = end - begin;

				m_cmd_infos_forward.add_time(_cmd.file, _cmd.line, time);
			}
			else
			{
				_cmd();
			}
		}
		catch (...)
		{

		}
	}
#else

	for (auto& _cmd : m_ForwardQuene)
	{
		_cmd();
	}
#endif

}

CmdChain::CmdChain() {
	_ForwardQuene.reset(new CmdBuffer);
	_BackQuene.reset(new CmdBuffer);
}

CmdChain::~CmdChain() {
	_ForwardQuene.reset();
	_BackQuene.reset();
}

void CmdChain::_Post(const char* file, int line, std::function<void()>& fun) {
	_BackQuene->Push(file, line, fun);
}

void CmdChain::_SwapBuffer() {
	std::swap(_ForwardQuene, _BackQuene);
}

void CmdChain::_Begin() {
	_BackQuene->Rewind();
}

void CmdChain::_End() {
	;
}

void CmdChain::_Excute() {
	_ForwardQuene->Execute();
}
