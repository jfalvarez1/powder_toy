#pragma once
#include "Particle.h"
#include "SimulationConfig.h"
#include <cstdint>
#include <algorithm>
#include <atomic>

// Check for SIMD support
#if defined(__AVX2__)
#define SIMD_AVX2 1
#include <immintrin.h>
#elif defined(__SSE4_1__) || defined(__SSE2__) || defined(_M_X64) || defined(_M_AMD64)
#define SIMD_SSE 1
#include <emmintrin.h>
#include <smmintrin.h>
#endif

// Prefetch hints for cache optimization
#if defined(__GNUC__) || defined(__clang__)
#define PREFETCH_READ(addr) __builtin_prefetch((addr), 0, 3)
#define PREFETCH_WRITE(addr) __builtin_prefetch((addr), 1, 3)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#elif defined(_MSC_VER)
#include <intrin.h>
#define PREFETCH_READ(addr) _mm_prefetch((const char*)(addr), _MM_HINT_T0)
#define PREFETCH_WRITE(addr) _mm_prefetch((const char*)(addr), _MM_HINT_T0)
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#else
#define PREFETCH_READ(addr) ((void)0)
#define PREFETCH_WRITE(addr) ((void)0)
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

// Force inline for hot paths
#if defined(_MSC_VER)
#define FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
#define FORCE_INLINE inline
#endif

// Batch size for SIMD processing
#ifdef SIMD_AVX2
constexpr int SIMD_BATCH_SIZE = 8;
#elif defined(SIMD_SSE)
constexpr int SIMD_BATCH_SIZE = 4;
#else
constexpr int SIMD_BATCH_SIZE = 1;
#endif

// Lock-free kill queue for parallel particle deletion
constexpr int KILL_QUEUE_SIZE = 8192;
struct alignas(64) LockFreeKillQueue
{
	std::atomic<int> queue[KILL_QUEUE_SIZE];
	std::atomic<int> head{0};
	std::atomic<int> count{0};

	void Clear()
	{
		head.store(0, std::memory_order_relaxed);
		count.store(0, std::memory_order_relaxed);
	}

	bool Push(int particleIdx)
	{
		int idx = head.fetch_add(1, std::memory_order_relaxed);
		if (idx >= KILL_QUEUE_SIZE)
			return false;
		queue[idx].store(particleIdx, std::memory_order_relaxed);
		count.fetch_add(1, std::memory_order_release);
		return true;
	}

	int GetCount() const
	{
		return count.load(std::memory_order_acquire);
	}

	int Get(int idx) const
	{
		return queue[idx].load(std::memory_order_relaxed);
	}
};

// Structure for batch processing particle physics
struct alignas(32) ParticleBatch
{
	float vx[SIMD_BATCH_SIZE];
	float vy[SIMD_BATCH_SIZE];
	float advection[SIMD_BATCH_SIZE];
	float loss[SIMD_BATCH_SIZE];
	float diffusion[SIMD_BATCH_SIZE];
	float cellVx[SIMD_BATCH_SIZE];
	float cellVy[SIMD_BATCH_SIZE];
	float gravX[SIMD_BATCH_SIZE];
	float gravY[SIMD_BATCH_SIZE];
	int indices[SIMD_BATCH_SIZE];
	int count;
};

// Fast neighborhood result (no heap allocation)
struct FastNeighbourhood
{
	int surround[8];
	int surround_space;
	int nt;
	float pGravX;
	float pGravY;
};

// Fully unrolled GetNeighbourhood - eliminates loop overhead
// Order: (-1,-1), (0,-1), (1,-1), (-1,0), (1,0), (-1,1), (0,1), (1,1)
FORCE_INLINE void GetNeighbourhoodFast(
	const int pmap[YRES][XRES],
	int x, int y, int particleType,
	FastNeighbourhood& n)
{
	// Direct array access - no loop overhead
	const int* row_m1 = &pmap[y - 1][x - 1]; // y-1 row
	const int* row_0  = &pmap[y][x - 1];     // y row
	const int* row_p1 = &pmap[y + 1][x - 1]; // y+1 row

	// Load all 8 neighbors with minimal instructions
	int r0 = row_m1[0];  // (-1, -1)
	int r1 = row_m1[1];  // (0, -1)
	int r2 = row_m1[2];  // (1, -1)
	int r3 = row_0[0];   // (-1, 0)
	int r4 = row_0[2];   // (1, 0)
	int r5 = row_p1[0];  // (-1, 1)
	int r6 = row_p1[1];  // (0, 1)
	int r7 = row_p1[2];  // (1, 1)

	n.surround[0] = r0;
	n.surround[1] = r1;
	n.surround[2] = r2;
	n.surround[3] = r3;
	n.surround[4] = r4;
	n.surround[5] = r5;
	n.surround[6] = r6;
	n.surround[7] = r7;

	// Extract types using bit manipulation
	// TYP(r) is typically (r & 0xFF) or similar
	int t0 = r0 & 0xFF, t1 = r1 & 0xFF, t2 = r2 & 0xFF, t3 = r3 & 0xFF;
	int t4 = r4 & 0xFF, t5 = r5 & 0xFF, t6 = r6 & 0xFF, t7 = r7 & 0xFF;

	// Count empty spaces (type == 0)
	n.surround_space = (!t0) + (!t1) + (!t2) + (!t3) + (!t4) + (!t5) + (!t6) + (!t7);

	// Count different types
	n.nt = (t0 != particleType) + (t1 != particleType) + (t2 != particleType) +
	       (t3 != particleType) + (t4 != particleType) + (t5 != particleType) +
	       (t6 != particleType) + (t7 != particleType);

	n.pGravX = 0;
	n.pGravY = 0;
}

// SIMD-optimized velocity update
FORCE_INLINE void SimdUpdateVelocities(ParticleBatch& batch, Particle* parts, float* randValues)
{
#ifdef SIMD_AVX2
	if (LIKELY(batch.count == SIMD_BATCH_SIZE))
	{
		// Load velocities
		__m256 vx = _mm256_loadu_ps(batch.vx);
		__m256 vy = _mm256_loadu_ps(batch.vy);

		// Load parameters
		__m256 loss = _mm256_loadu_ps(batch.loss);
		__m256 advection = _mm256_loadu_ps(batch.advection);
		__m256 cellVx = _mm256_loadu_ps(batch.cellVx);
		__m256 cellVy = _mm256_loadu_ps(batch.cellVy);
		__m256 gravX = _mm256_loadu_ps(batch.gravX);
		__m256 gravY = _mm256_loadu_ps(batch.gravY);
		__m256 diffusion = _mm256_loadu_ps(batch.diffusion);
		__m256 randX = _mm256_loadu_ps(randValues);
		__m256 randY = _mm256_loadu_ps(randValues + SIMD_BATCH_SIZE);

		// Apply loss: vx *= loss, vy *= loss
		vx = _mm256_mul_ps(vx, loss);
		vy = _mm256_mul_ps(vy, loss);

		// Apply advection and gravity using FMA if available
		#ifdef __FMA__
		vx = _mm256_fmadd_ps(advection, cellVx, vx);
		vy = _mm256_fmadd_ps(advection, cellVy, vy);
		#else
		vx = _mm256_add_ps(vx, _mm256_mul_ps(advection, cellVx));
		vy = _mm256_add_ps(vy, _mm256_mul_ps(advection, cellVy));
		#endif
		vx = _mm256_add_ps(vx, gravX);
		vy = _mm256_add_ps(vy, gravY);

		// Apply diffusion: v += diffusion * (2*rand - 1)
		__m256 two = _mm256_set1_ps(2.0f);
		__m256 one = _mm256_set1_ps(1.0f);
		__m256 diffRandX = _mm256_sub_ps(_mm256_mul_ps(two, randX), one);
		__m256 diffRandY = _mm256_sub_ps(_mm256_mul_ps(two, randY), one);
		#ifdef __FMA__
		vx = _mm256_fmadd_ps(diffusion, diffRandX, vx);
		vy = _mm256_fmadd_ps(diffusion, diffRandY, vy);
		#else
		vx = _mm256_add_ps(vx, _mm256_mul_ps(diffusion, diffRandX));
		vy = _mm256_add_ps(vy, _mm256_mul_ps(diffusion, diffRandY));
		#endif

		// Store back
		_mm256_storeu_ps(batch.vx, vx);
		_mm256_storeu_ps(batch.vy, vy);

		// Scatter to particles
		for (int i = 0; i < SIMD_BATCH_SIZE; i++)
		{
			parts[batch.indices[i]].vx = batch.vx[i];
			parts[batch.indices[i]].vy = batch.vy[i];
		}
		return;
	}
#elif defined(SIMD_SSE)
	if (LIKELY(batch.count == SIMD_BATCH_SIZE))
	{
		__m128 vx = _mm_loadu_ps(batch.vx);
		__m128 vy = _mm_loadu_ps(batch.vy);
		__m128 loss = _mm_loadu_ps(batch.loss);
		__m128 advection = _mm_loadu_ps(batch.advection);
		__m128 cellVx = _mm_loadu_ps(batch.cellVx);
		__m128 cellVy = _mm_loadu_ps(batch.cellVy);
		__m128 gravX = _mm_loadu_ps(batch.gravX);
		__m128 gravY = _mm_loadu_ps(batch.gravY);

		vx = _mm_mul_ps(vx, loss);
		vy = _mm_mul_ps(vy, loss);
		vx = _mm_add_ps(vx, _mm_mul_ps(advection, cellVx));
		vy = _mm_add_ps(vy, _mm_mul_ps(advection, cellVy));
		vx = _mm_add_ps(vx, gravX);
		vy = _mm_add_ps(vy, gravY);

		_mm_storeu_ps(batch.vx, vx);
		_mm_storeu_ps(batch.vy, vy);

		for (int i = 0; i < SIMD_BATCH_SIZE; i++)
		{
			parts[batch.indices[i]].vx = batch.vx[i];
			parts[batch.indices[i]].vy = batch.vy[i];
		}
		return;
	}
#endif

	// Scalar fallback
	for (int i = 0; i < batch.count; i++)
	{
		int idx = batch.indices[i];
		float newVx = batch.vx[i] * batch.loss[i] +
		              batch.advection[i] * batch.cellVx[i] + batch.gravX[i];
		float newVy = batch.vy[i] * batch.loss[i] +
		              batch.advection[i] * batch.cellVy[i] + batch.gravY[i];

		if (batch.diffusion[i] > 0)
		{
			newVx += batch.diffusion[i] * (2.0f * randValues[i] - 1.0f);
			newVy += batch.diffusion[i] * (2.0f * randValues[i + SIMD_BATCH_SIZE] - 1.0f);
		}

		parts[idx].vx = newVx;
		parts[idx].vy = newVy;
	}
}

// Optimized pmap neighborhood lookup with prefetching
FORCE_INLINE void PrefetchNeighborhood(const int pmap[YRES][XRES], int x, int y)
{
	// Prefetch the 3x3 neighborhood for next iteration
	if (LIKELY(y > 0 && y < YRES - 1 && x > 0 && x < XRES - 1))
	{
		PREFETCH_READ(&pmap[y - 1][x - 1]);
		PREFETCH_READ(&pmap[y][x - 1]);
		PREFETCH_READ(&pmap[y + 1][x - 1]);
	}
}

// Batch prefetch multiple particle positions
FORCE_INLINE void PrefetchParticles(const Particle* parts, const int* indices, int count, int offset)
{
	for (int i = 0; i < count && i + offset < count; i++)
	{
		PREFETCH_READ(&parts[indices[i + offset]]);
	}
}

// Fast integer to cell coordinate conversion
FORCE_INLINE int FastCellCoord(int pos)
{
	return pos >> 2; // Equivalent to pos / CELL when CELL = 4
}

// Optimized bounds check
FORCE_INLINE bool IsInBounds(int x, int y)
{
	// Single branch for all bounds using unsigned comparison trick
	return static_cast<unsigned>(x - CELL) < static_cast<unsigned>(XRES - 2 * CELL) &&
	       static_cast<unsigned>(y - CELL) < static_cast<unsigned>(YRES - 2 * CELL);
}

// Fast float to int with rounding (avoids function call overhead)
FORCE_INLINE int FastRound(float f)
{
	return static_cast<int>(f + 0.5f);
}
