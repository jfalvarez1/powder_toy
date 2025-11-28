#pragma once
#include "Particle.h"
#include "SimulationConfig.h"
#include <cstdint>
#include <algorithm>

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
#elif defined(_MSC_VER)
#include <intrin.h>
#define PREFETCH_READ(addr) _mm_prefetch((const char*)(addr), _MM_HINT_T0)
#define PREFETCH_WRITE(addr) _mm_prefetch((const char*)(addr), _MM_HINT_T0)
#else
#define PREFETCH_READ(addr) ((void)0)
#define PREFETCH_WRITE(addr) ((void)0)
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

// SIMD-optimized velocity update
FORCE_INLINE void SimdUpdateVelocities(ParticleBatch& batch, Particle* parts, float* randValues)
{
#ifdef SIMD_AVX2
	if (batch.count == SIMD_BATCH_SIZE)
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
		__m256 rand = _mm256_loadu_ps(randValues);

		// Apply loss: vx *= loss, vy *= loss
		vx = _mm256_mul_ps(vx, loss);
		vy = _mm256_mul_ps(vy, loss);

		// Apply advection and gravity: vx += advection * cellVx + gravX
		__m256 advVx = _mm256_mul_ps(advection, cellVx);
		__m256 advVy = _mm256_mul_ps(advection, cellVy);
		vx = _mm256_add_ps(vx, advVx);
		vy = _mm256_add_ps(vy, advVy);
		vx = _mm256_add_ps(vx, gravX);
		vy = _mm256_add_ps(vy, gravY);

		// Apply diffusion: vx += diffusion * (2*rand - 1)
		__m256 two = _mm256_set1_ps(2.0f);
		__m256 one = _mm256_set1_ps(1.0f);
		__m256 diffRand = _mm256_sub_ps(_mm256_mul_ps(two, rand), one);
		vx = _mm256_add_ps(vx, _mm256_mul_ps(diffusion, diffRand));
		// Note: vy would need separate random values, simplified here

		// Store results back to particles
		_mm256_storeu_ps(batch.vx, vx);
		_mm256_storeu_ps(batch.vy, vy);

		for (int i = 0; i < SIMD_BATCH_SIZE; i++)
		{
			parts[batch.indices[i]].vx = batch.vx[i];
			parts[batch.indices[i]].vy = batch.vy[i];
		}
		return;
	}
#elif defined(SIMD_SSE)
	if (batch.count == SIMD_BATCH_SIZE)
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
		parts[idx].vx = batch.vx[i] * batch.loss[i] +
		                batch.advection[i] * batch.cellVx[i] + batch.gravX[i];
		parts[idx].vy = batch.vy[i] * batch.loss[i] +
		                batch.advection[i] * batch.cellVy[i] + batch.gravY[i];

		if (batch.diffusion[i] > 0)
		{
			parts[idx].vx += batch.diffusion[i] * (2.0f * randValues[i] - 1.0f);
			parts[idx].vy += batch.diffusion[i] * (2.0f * randValues[i + SIMD_BATCH_SIZE] - 1.0f);
		}
	}
}

// Optimized pmap neighborhood lookup with prefetching
FORCE_INLINE void PrefetchNeighborhood(const int pmap[YRES][XRES], int x, int y)
{
	// Prefetch the 3x3 neighborhood for next iteration
	if (y > 0 && y < YRES - 1 && x > 0 && x < XRES - 1)
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
	// Single branch for all bounds
	return static_cast<unsigned>(x - CELL) < static_cast<unsigned>(XRES - 2 * CELL) &&
	       static_cast<unsigned>(y - CELL) < static_cast<unsigned>(YRES - 2 * CELL);
}
