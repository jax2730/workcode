#ifndef __FrameControl_H__
#define __FrameControl_H__

#include "EchoTimer.h"
//帧率控制器
class	whframecontrol
{
protected:
	long			m_nMSPerTick;								// 每个Tick需要的毫秒数
	long			m_nStatPeriod;								// 一次统计的周期(毫秒)
	unsigned long	m_nExpected;								// 期望在这个Tick达到的时刻
	unsigned long	m_nStatEnd;									// 本次统计结束的时刻
	float		m_fFR;										// 原始帧率（即没有乘以TimeScale之前的帧率）
	float		m_fTimeScale;								// 时间放缩因子（实际上是时间运行速度因子），默认为1，如果>1，表示时间变快，<1表示时间变慢，==0表示时间停止
	int			m_tick;										// tick计数（用于统计帧率）
	int			m_lastFR;									// 最近一次统计的帧率
	unsigned long	m_nextFRStatTime;							// 下次记录FR的时刻
	Echo::Timer m_timer;
public:
	whframecontrol();
	void	SetFR(float fFR);								// 设置帧率(帧/秒)，注意最后产生的帧率不一定和这个一致，可能会高一些。
															// 因为内部是通过每帧的时间来计算的。
	void	SetStatPeriod(int nMS);							// 设置统计周期(毫秒)
	void	SetTimeScale(float fTimeScale);					// 设置时间因子
	inline float	GetTimeScale()							// 获取当前的时间因子
	{
		return	m_fTimeScale;
	}
	void	Tick();											// 放在需要帧率控制的主循环中，里面会调用sleep
	void	Reset();										// 避免调节时间造成的大时间差
	inline int	GetFR()										// 获得设置的帧率（固定值）
	{
		return	(int)m_fFR;
	}
	int		GetRealFR();									// 获得当前帧率（统计值）
	void	wh_sleep(int __ms);
	unsigned long	wh_gettickcount();

	inline long wh_tickcount_diff(unsigned long t1, unsigned long t2)
	{
		return long(t1 - t2);
	}
};
#endif
