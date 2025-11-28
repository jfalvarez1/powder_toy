#include "ThreadPool.h"
#include <algorithm>
#include <string>
#include <sstream>

std::unique_ptr<ThreadPool> ThreadPool::instance;
std::mutex ThreadPool::instanceMutex;
bool ThreadPool::multithreadingEnabled = true;
PerformanceProfile ThreadPool::currentProfile = PerformanceProfile::Auto;

ThreadPool::ThreadPool(size_t numThreads, int minChunk) : stop(false), activeTasks(0), minChunkSize(minChunk)
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

	// Use minChunkSize to determine if parallelization is worthwhile
	if (range < minChunkSize * numThreads || numThreads <= 1)
	{
		func(start, end);
		return;
	}

	int chunkSize = (range + numThreads - 1) / numThreads;

	// Ensure minimum chunk size for efficiency
	if (chunkSize < minChunkSize)
	{
		chunkSize = minChunkSize;
		numThreads = (range + chunkSize - 1) / chunkSize;
	}

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

unsigned int ThreadPool::GetHardwareThreadCount()
{
	unsigned int hwThreads = std::thread::hardware_concurrency();
	if (hwThreads == 0)
	{
		hwThreads = 2; // Default fallback
	}
	return hwThreads;
}

size_t ThreadPool::GetThreadCountForProfile(PerformanceProfile profile)
{
	unsigned int hwThreads = GetHardwareThreadCount();

	switch (profile)
	{
	case PerformanceProfile::Conservative:
		// Use at most 4 threads, good for laptops/low power systems
		return std::min(hwThreads, 4u);

	case PerformanceProfile::Balanced:
		// Use half of available threads
		return std::max(hwThreads / 2, 2u);

	case PerformanceProfile::HighPerformance:
		// Use all hardware threads (ideal for 8-core/16-thread CPUs like Ryzen 9800X3D)
		return hwThreads;

	case PerformanceProfile::Extreme:
		// Use all threads with no caps (for 16+ core systems)
		// Also allows more aggressive parallelization thresholds
		return std::max(hwThreads, 16u);

	case PerformanceProfile::Auto:
	default:
		// Auto-detect based on hardware
		if (hwThreads >= 16)
		{
			// High-end system (16+ threads), use all
			return hwThreads;
		}
		else if (hwThreads >= 8)
		{
			// Mid-high system (8-15 threads), use all
			return hwThreads;
		}
		else if (hwThreads >= 4)
		{
			// Mid system (4-7 threads), use most
			return std::max(hwThreads - 1, 2u);
		}
		else
		{
			// Low-end system, use what's available
			return std::max(hwThreads, 2u);
		}
	}
}

ThreadPool& ThreadPool::Ref()
{
	std::lock_guard<std::mutex> lock(instanceMutex);
	if (!instance)
	{
		size_t numThreads = multithreadingEnabled ? GetThreadCountForProfile(currentProfile) : 1;

		// Determine minimum chunk size based on profile
		int minChunk;
		switch (currentProfile)
		{
		case PerformanceProfile::Extreme:
			minChunk = 2; // Very aggressive parallelization
			break;
		case PerformanceProfile::HighPerformance:
			minChunk = 3; // Aggressive parallelization
			break;
		case PerformanceProfile::Balanced:
			minChunk = 4; // Normal
			break;
		case PerformanceProfile::Conservative:
			minChunk = 8; // Less parallelization overhead
			break;
		case PerformanceProfile::Auto:
		default:
			// Auto-select based on thread count
			if (numThreads >= 16)
				minChunk = 2;
			else if (numThreads >= 8)
				minChunk = 3;
			else
				minChunk = 4;
			break;
		}

		instance = std::make_unique<ThreadPool>(numThreads, minChunk);
	}
	return *instance;
}

size_t ThreadPool::GetOptimalThreadCount()
{
	return GetThreadCountForProfile(currentProfile);
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

void ThreadPool::SetPerformanceProfile(PerformanceProfile profile)
{
	std::lock_guard<std::mutex> lock(instanceMutex);
	if (instance)
	{
		// Can't change after initialization
		return;
	}
	currentProfile = profile;
}

PerformanceProfile ThreadPool::GetPerformanceProfile()
{
	return currentProfile;
}

const char* ThreadPool::GetProfileName(PerformanceProfile profile)
{
	switch (profile)
	{
	case PerformanceProfile::Conservative:
		return "Conservative";
	case PerformanceProfile::Balanced:
		return "Balanced";
	case PerformanceProfile::HighPerformance:
		return "High Performance";
	case PerformanceProfile::Extreme:
		return "Extreme";
	case PerformanceProfile::Auto:
	default:
		return "Auto";
	}
}

std::string ThreadPool::GetStatusString()
{
	std::ostringstream ss;
	if (multithreadingEnabled)
	{
		size_t threads = GetThreadCountForProfile(currentProfile);
		ss << "MT: " << threads << " threads (" << GetProfileName(currentProfile) << ")";
	}
	else
	{
		ss << "MT: Disabled (single-threaded)";
	}
	return ss.str();
}
