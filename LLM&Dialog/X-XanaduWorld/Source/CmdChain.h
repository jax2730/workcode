#pragma once
#include <functional>
#include <string>
#include <chrono>
#include <unordered_map>
#include <deque>
#include <memory>


#define CMD_STATISTIC


struct cmd_info
{
	int count;
	std::chrono::nanoseconds time;
};


using cmd_key = std::pair<const char*, int>;

struct cmd
{
	cmd(const char* _file, int _line, std::function<void()> && _func);

	cmd(const char* _file, int _line, const std::function<void()>& _func);

	const char* file;
	int line;

	std::function<void()> func;

	void operator()() const;
};

namespace std
{
	template<> struct hash<cmd_key>
	{
		typedef cmd_key argument_type;
		typedef std::size_t result_type;
		result_type operator()(argument_type const& s) const noexcept
		{
			result_type const h1(std::hash<const char*>{}(s.first));
			result_type const h2(std::hash<int>{}(s.second));
			return h1 ^ (h2 << 1); // or use boost::hash_combine (see Discussion)
		}
	};
}

struct cmd_statistic
{
	bool empty() const;
	void clear();
	size_t count() const;
	const std::chrono::nanoseconds& time() const;
	void push(const char* file, const int& line);
	void add_time(const char* file, const int& line, const std::chrono::nanoseconds& _time);
	std::unordered_map<cmd_key, cmd_info> m_cmd_infos;
private:
	size_t cmd_count = 0;
	std::chrono::nanoseconds cmd_time;

	//返回给逻辑线程的结果.
};

class CmdBuffer;

class CmdChain
{
public:

	void SetStatistic(bool open);
	const cmd_statistic& GetCmdStatistic() const;

#ifdef CMD_STATISTIC
	void SetNextCmdInfo(const char* file, int line);
#endif
	void Post(const std::function<void()>& cmd);
	void Post(std::function<void()>&& cmd);


	void SwapBuffer();

	void Begin();
	void End(){}
	
	size_t GetCmdCount() const;

	void Excute();

private:

#ifdef CMD_STATISTIC
	const char* file;
	int line;

	bool m_bStatistic = false;
	int m_bStatisticMinLimit = 1000;

	//逻辑线程统计次数的
	cmd_statistic m_cmd_infos_back;
	//主线程统计时间用的
	cmd_statistic m_cmd_infos_forward;
	cmd_statistic m_cmd_infos;

	std::deque<cmd> m_ForwardQuene;
	std::deque<cmd> m_BackQuene;
#else
	std::deque<std::function<void()>> m_ForwardQuene;
	std::deque<std::function<void()>> m_BackQuene;
#endif

protected:

	std::unique_ptr<CmdBuffer>	_ForwardQuene;
	std::unique_ptr<CmdBuffer>	_BackQuene;

public:

	CmdChain();
	~CmdChain();

	void _Post(const char* file, int line, std::function<void()>& fun);
	void _SwapBuffer();
	void _Begin();
	void _End();
	void _Excute();
};
