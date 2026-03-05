#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <vector>
#include <thread>
#include <future>
#include <functional>
#include <atomic>


template <typename T>
class ThreadSafeQueue {
public:
	ThreadSafeQueue() = default;
	ThreadSafeQueue(const ThreadSafeQueue&) = delete;
	ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

	// 入队：拷贝版本
	void push(const T& value) {
		{
			std::lock_guard<std::mutex> lk(mutex_);
			queue_.push(value);
		}
		cv_.notify_one();
	}

	// 入队：移动版本
	void push(T&& value) {
		{
			std::lock_guard<std::mutex> lk(mutex_);
			queue_.push(std::move(value));
		}
		cv_.notify_one();
	}

	// 阻塞式出队：一直等到取到一个元素
	void wait_and_pop(T& value) {
		std::unique_lock<std::mutex> lk(mutex_);
		cv_.wait(lk, [this] { return !queue_.empty(); });
		value = std::move(queue_.front());
		queue_.pop();
	}

	// 非阻塞出队：如果有就取，没有返回 false
	bool try_pop(T& value) {
		std::lock_guard<std::mutex> lk(mutex_);
		if (queue_.empty()) return false;
		value = std::move(queue_.front());
		queue_.pop();
		return true;
	}

	bool empty() const {
		std::lock_guard<std::mutex> lk(mutex_);
		return queue_.empty();
	}

	std::size_t size() const {
		std::lock_guard<std::mutex> lk(mutex_);
		return queue_.size();
	}

private:
	mutable std::mutex mutex_;
	std::queue<T> queue_;
	std::condition_variable cv_;
};

class ThreadPool {
public:
	explicit ThreadPool(std::size_t threadCount = std::thread::hardware_concurrency())
		: stop_(false), activeTasks_(0), completedTasks_(0)
	{
		if (threadCount == 0) threadCount = 1;

		for (std::size_t i = 0; i < threadCount; ++i) {
			workers_.emplace_back([this] { workerLoop(); });
		}
	}

	~ThreadPool() {
		stop();
	}

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	// 提交任务
	template <class F, class... Args>
	auto enqueue(F&& f, Args&&... args)
		-> std::future<typename std::result_of<F(Args...)>::type>
	{
		using return_type = typename std::result_of<F(Args...)>::type;

		auto taskPtr = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);

		std::future<return_type> res = taskPtr->get_future();

		{
			// 新任务：未完成数量 +1
			std::lock_guard<std::mutex> lk(doneMtx_);
			++activeTasks_;
		}

		// 包裹成 void() 放进队列
		tasks_.push([this, taskPtr]() {
			// 真正执行任务
			(*taskPtr)();

			// 执行完了，更新计数并打印
			std::size_t finished = 0;
			std::size_t remaining = 0;
			{
				std::lock_guard<std::mutex> lk(doneMtx_);
				++completedTasks_;
				--activeTasks_;
				finished = completedTasks_;
				remaining = activeTasks_;

				if (activeTasks_ == 0) {
					doneCv_.notify_all();
				}
			}

			// 打印（这里不用锁也行，最多几行交错）
			std::cout << "Task done. Completed: " << finished
				<< ", Remaining: " << remaining << std::endl;
			});

		return res;
	}

	// 等所有已提交任务执行完
	void waitAll() {
		std::unique_lock<std::mutex> lk(doneMtx_);
		doneCv_.wait(lk, [this] { return activeTasks_ == 0; });
	}

	void stop() {
		bool expected = false;
		if (stop_.compare_exchange_strong(expected, true)) {
			// 推入空任务唤醒线程退出
			for (std::size_t i = 0; i < workers_.size(); ++i) {
				tasks_.push(nullptr);
			}
			for (auto& t : workers_) {
				if (t.joinable()) t.join();
			}
		}
	}

private:
	void workerLoop() {
		while (true) {
			std::function<void()> task;
			tasks_.wait_and_pop(task); // 阻塞等待任务

			if (!task) { // nullptr 作为退出信号
				break;
			}

			task();      // 执行任务（里面会更新计数并打印）
		}
	}

private:
	std::vector<std::thread> workers_;
	ThreadSafeQueue<std::function<void()>> tasks_;
	std::atomic<bool> stop_;

	// 任务统计 & 等待全部完成
	std::mutex              doneMtx_;
	std::condition_variable doneCv_;
	std::size_t             activeTasks_;    // 未完成任务数
	std::size_t             completedTasks_; // 已完成任务数
};
