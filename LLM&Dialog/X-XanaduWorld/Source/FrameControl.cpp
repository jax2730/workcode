
#include "FrameControl.h"
//#include <timeapi.h>
whframecontrol::whframecontrol()
	: m_nMSPerTick(0)
	, m_nStatPeriod(10000)
	, m_fFR(30)
	, m_fTimeScale(1)
	, m_tick(0)
	, m_lastFR(0)
	, m_nextFRStatTime(0)
{
	m_timer.reset();
	Reset();
}

void	whframecontrol::SetFR(float fFR)
{
	m_fFR = fFR;
	if (m_fTimeScale > 0)
	{
		m_nMSPerTick = int(1000 / (fFR*m_fTimeScale));
	}
	else
	{
		m_nMSPerTick = 10;	// 上层的逻辑暂停，这里不能停（上层应该根据别的来判断是否走逻辑）
	}
}

void	whframecontrol::SetStatPeriod(int nMS)
{
	m_nStatPeriod = nMS;
	Reset();
}

void	whframecontrol::SetTimeScale(float fTimeScale)
{
	if (m_fTimeScale == fTimeScale)	return;
	m_fTimeScale = fTimeScale;
	SetFR(m_fFR);
}

void	whframecontrol::Tick()
{
	// 获取当前和下一个tick期望的起始时刻的差异
	unsigned long	nNow = wh_gettickcount();
	long			nDiff = wh_tickcount_diff(m_nExpected, nNow);
	long			nToEnd = wh_tickcount_diff(nNow, m_nStatEnd);
	if (nDiff <= 0)
	{
		// 比预期慢
		if (nToEnd >= 0)
		{
			m_nExpected = nNow;
		}
	}
	else
	{
		// 停止一一段时间
		wh_sleep(int(nDiff));
	}
	// 重新矫正统计间隔（如果nToEnd<0，说明已经落后的太多了）
	if (nToEnd >= 0)
	{
		m_nStatEnd = m_nExpected + m_nStatPeriod;
	}
	// 下一个期望的时间点
	m_nExpected += m_nMSPerTick;
	//
	m_tick++;
	nDiff = wh_tickcount_diff(nNow, m_nextFRStatTime);
	if (nDiff >= 0)
	{
		m_lastFR = m_tick * 1000 / (nDiff + 1000);
		m_nextFRStatTime = nNow + 1000;
		m_tick = 0;
	}
}

void	whframecontrol::Reset()
{
	m_nExpected = wh_gettickcount();
	m_nStatEnd = m_nExpected + m_nStatPeriod;
	m_nextFRStatTime = m_nExpected + 1000;
}

int		whframecontrol::GetRealFR()
{
	return	m_lastFR;
}

void whframecontrol::wh_sleep(int __ms)
{ 
	if (__ms != 0)
	{
#ifdef _WIN32
		Sleep(__ms);
#endif

#ifdef __ANDROID__
		usleep(__ms * 1000);
#endif

#ifdef __APPLE__
		usleep(__ms * 1000L);
#endif
	}
}

unsigned long whframecontrol::wh_gettickcount()
{
	return m_timer.getMilliseconds();
}
