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

// Performance profiles for different system configurations
enum class PerformanceProfile
{
	Auto,           // Automatically detect optimal settings
	Conservative,   // Use fewer threads, lower overhead (4 threads max)
	Balanced,       // Default balanced performance (hardware threads / 2)
	HighPerformance,// Use all hardware threads, aggressive parallelization
	Extreme         // Maximum parallelization for high-end systems (16+ cores)
};

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

	// Performance settings
	int minChunkSize; // Minimum work items per thread before parallelization

public:
	ThreadPool(size_t numThreads, int minChunk = 4);
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

	// Get minimum chunk size for parallelization
	int GetMinChunkSize() const { return minChunkSize; }

	// Parallel for loop - divides range [start, end) among threads
	void ParallelFor(int start, int end, const std::function<void(int, int)>& func);

	// Get the singleton instance
	static ThreadPool& Ref();

	// Get the optimal number of threads for computation
	static size_t GetOptimalThreadCount();

	// Get thread count for a specific profile
	static size_t GetThreadCountForProfile(PerformanceProfile profile);

	// Check if multithreading is enabled
	static bool IsEnabled();

	// Enable/disable multithreading (must be called before first use)
	static void SetEnabled(bool enabled);

	// Set performance profile (must be called before first use)
	static void SetPerformanceProfile(PerformanceProfile profile);

	// Get current performance profile
	static PerformanceProfile GetPerformanceProfile();

	// Get hardware thread count
	static unsigned int GetHardwareThreadCount();

	// Get profile name as string
	static const char* GetProfileName(PerformanceProfile profile);

	// Get status string for display (e.g., "MT: 16 threads (High Performance)")
	static std::string GetStatusString();

private:
	static bool multithreadingEnabled;
	static PerformanceProfile currentProfile;
};
