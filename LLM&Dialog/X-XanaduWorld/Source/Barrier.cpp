
#include "Barrier.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>


using namespace std;
struct Barrier::impl_t
{
	mutex m_mutex;
	condition_variable m_cond;
	unsigned int m_count;
	unsigned int m_generation;
	const unsigned int m_count_init;
	bool m_exit;
	function<void()> fn_pass;


	impl_t(unsigned int count, function<void()> _pass)
		: m_count(count), m_generation(0), m_count_init(count), fn_pass(_pass)
	{
		m_exit = false;
	}

	bool Wait()
	{
		unique_lock <mutex> lock(m_mutex);
		unsigned int gen = m_generation;

		if (--m_count == 0)
		{
			m_generation++;
			m_count = m_count_init;
			lock.unlock();
			if (fn_pass) {
				fn_pass();
			}
			m_cond.notify_all();
			return true;
		}

		while ((!m_exit) && gen == m_generation) {
			m_cond.wait(lock);
		}
		return false;
	}
	void SetExitFlag() {
		m_exit = true;
		m_cond.notify_all();
	}
	void Reset()
	{
		m_exit = false;
		m_count = m_count_init;
	}
};

Barrier::Barrier(unsigned int count, std::function<void()> _pass)
	:pImpl(make_unique_ptr<impl_t>(count, _pass))
{
}

Barrier::~Barrier()
{
}

bool Barrier::Wait()
{
	return pImpl->Wait();
}

void Barrier::Reset()
{
	pImpl->Reset();
}

void Barrier::SetExitFlag()
{
	pImpl->SetExitFlag();
}
