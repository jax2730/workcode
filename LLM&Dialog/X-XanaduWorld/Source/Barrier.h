#pragma once
#include <thread>
#include <mutex>
#include <functional>
#include <memory>

// make_unique is not available in C++11
// Taken from Herb Sutter's blog (https://herbsutter.com/gotw/_102/)
// 跟c++14重名，暂时不打算支持c++14，所以改一下名字 by OZP
template<typename T, typename ...Args>
std::unique_ptr<T> make_unique_ptr(Args&& ...args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

class Barrier
{
	struct impl_t;
	std::unique_ptr<impl_t> pImpl;
public:

	Barrier(unsigned int count, std::function<void()> _pass);
	~Barrier();

	bool Wait();
	void Reset();
	void SetExitFlag();
};