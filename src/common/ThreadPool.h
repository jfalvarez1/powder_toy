#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <memory>

class ThreadPool
{
private:
	std::vector<std::thread> workers;
	std::queue<std::function<void()>> tasks;

	std::mutex queueMutex;
	std::condition_variable condition;
	std::condition_variable doneCondition;
	std::atomic<bool> stop;
	std::atomic<int> activeTasks;

	static std::unique_ptr<ThreadPool> instance;
	static std::mutex instanceMutex;

public:
	ThreadPool(size_t numThreads);
	~ThreadPool();

	// Disable copy
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	// Submit a task to the pool
	template<class F>
	void Submit(F&& f)
	{
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			tasks.emplace(std::forward<F>(f));
			activeTasks++;
		}
		condition.notify_one();
	}

	// Wait for all tasks to complete
	void Wait();

	// Get the number of threads in the pool
	size_t GetThreadCount() const { return workers.size(); }

	// Parallel for loop - divides range [start, end) among threads
	void ParallelFor(int start, int end, const std::function<void(int, int)>& func);

	// Get the singleton instance
	static ThreadPool& Ref();

	// Get the optimal number of threads for computation
	static size_t GetOptimalThreadCount();

	// Check if multithreading is enabled
	static bool IsEnabled();

	// Enable/disable multithreading (must be called before first use)
	static void SetEnabled(bool enabled);

private:
	static bool multithreadingEnabled;
};
