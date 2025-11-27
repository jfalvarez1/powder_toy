#include "ThreadPool.h"
#include <algorithm>

std::unique_ptr<ThreadPool> ThreadPool::instance;
std::mutex ThreadPool::instanceMutex;
bool ThreadPool::multithreadingEnabled = true;

ThreadPool::ThreadPool(size_t numThreads) : stop(false), activeTasks(0)
{
	for (size_t i = 0; i < numThreads; ++i)
	{
		workers.emplace_back([this]() {
			while (true)
			{
				std::function<void()> task;
				{
					std::unique_lock<std::mutex> lock(queueMutex);
					condition.wait(lock, [this]() {
						return stop || !tasks.empty();
					});

					if (stop && tasks.empty())
					{
						return;
					}

					task = std::move(tasks.front());
					tasks.pop();
				}

				task();

				{
					std::unique_lock<std::mutex> lock(queueMutex);
					activeTasks--;
					if (activeTasks == 0 && tasks.empty())
					{
						doneCondition.notify_all();
					}
				}
			}
		});
	}
}

ThreadPool::~ThreadPool()
{
	{
		std::unique_lock<std::mutex> lock(queueMutex);
		stop = true;
	}
	condition.notify_all();

	for (auto& worker : workers)
	{
		worker.join();
	}
}

void ThreadPool::Wait()
{
	std::unique_lock<std::mutex> lock(queueMutex);
	doneCondition.wait(lock, [this]() {
		return activeTasks == 0 && tasks.empty();
	});
}

void ThreadPool::ParallelFor(int start, int end, const std::function<void(int, int)>& func)
{
	if (start >= end)
	{
		return;
	}

	int range = end - start;
	int numThreads = static_cast<int>(workers.size());

	// If range is small or only one thread, run sequentially
	if (range < numThreads * 2 || numThreads <= 1)
	{
		func(start, end);
		return;
	}

	int chunkSize = (range + numThreads - 1) / numThreads;

	for (int t = 0; t < numThreads; ++t)
	{
		int chunkStart = start + t * chunkSize;
		int chunkEnd = std::min(chunkStart + chunkSize, end);

		if (chunkStart >= end)
		{
			break;
		}

		Submit([func, chunkStart, chunkEnd]() {
			func(chunkStart, chunkEnd);
		});
	}

	Wait();
}

ThreadPool& ThreadPool::Ref()
{
	std::lock_guard<std::mutex> lock(instanceMutex);
	if (!instance)
	{
		size_t numThreads = multithreadingEnabled ? GetOptimalThreadCount() : 1;
		instance = std::make_unique<ThreadPool>(numThreads);
	}
	return *instance;
}

size_t ThreadPool::GetOptimalThreadCount()
{
	unsigned int hwThreads = std::thread::hardware_concurrency();
	if (hwThreads == 0)
	{
		hwThreads = 2; // Default fallback
	}
	// Use all available cores but cap at reasonable maximum
	return std::min(hwThreads, 16u);
}

bool ThreadPool::IsEnabled()
{
	return multithreadingEnabled && GetOptimalThreadCount() > 1;
}

void ThreadPool::SetEnabled(bool enabled)
{
	std::lock_guard<std::mutex> lock(instanceMutex);
	if (instance)
	{
		// Can't change after initialization
		return;
	}
	multithreadingEnabled = enabled;
}
