/*
 * Equihash solver created by djeZo (l33tsoftw@gmail.com) for NiceHash
 * Adapted to be more compatible with older C++ compilers
 *
 * cuda_djezo solver was released by NiceHash (www.nicehash.com) under
 * GPL 3.0 license. If you don't have a copy, you can obtain one from
 * https://www.gnu.org/licenses/gpl-3.0.txt
 *
 * Based on CUDA solver by John Tromp released under MIT license.
 * Some helper functions taken out of OpenCL solver by Marc Bevand
 * released under MIT license.
 *
 * Copyright (c) 2016 John Tromp, Marc Bevand
 * Copyright (c) 2017 djeZo, Tanguy Pruvot (GPL v3)
 */
#ifdef _WIN32
#include <Windows.h>
#endif

#include <stdio.h>
#include <vector>

#include "blake2/blake2.h"
#include "eqcuda.hpp"

#define WN 184
#define WK 7
#ifndef MAX_GPUS
#define MAX_GPUS 16
#endif

#define NDIGITS		(WK+1)
#define DIGITBITS	(WN/(NDIGITS))
#define PROOFSIZE 	(1<<WK)
#define BASE 		(1<<DIGITBITS)
#define NHASHES 	(2*BASE)
#define HASHESPERBLAKE (512/WN)
#define HASHOUT 	(HASHESPERBLAKE*WN/8)
#define NBLOCKS 	((NHASHES + HASHESPERBLAKE - 1) / HASHESPERBLAKE)
#define BUCKBITS 	(DIGITBITS - RB)
#define NBUCKETS 	(1 << BUCKBITS)
#define BUCKMASK 	(NBUCKETS - 1)
#define SLOTBITS 	(RB + 2)
#define SLOTRANGE 	(1 << SLOTBITS)
#define NSLOTS SM
#define SLOTMASK 	(SLOTRANGE - 1)
#define NRESTS 		(1 << RB)
#define RESTMASK 	(NRESTS - 1)
#define CANTORBITS 	(2 * SLOTBITS - 2)
#define CANTORMASK 	((1 << CANTORBITS) - 1)
#define CANTORMAXSQRT (2 * NSLOTS)

#define RB8_NSLOTS      640
#define RB8_NSLOTS_LD   624

#define RB_NSLOTS       SLOTRANGE
#define RB_NSLOTS_LD    SLOTRANGE

#define FD_THREADS      128

#ifdef __INTELLISENSE__
// reduce vstudio editor warnings
#include <device_functions.h>
#include <device_launch_parameters.h>
#define __launch_bounds__(max_tpb, min_blocks)
#define __CUDA_ARCH__ 520
uint32_t __byte_perm(uint32_t x, uint32_t y, uint32_t z);
uint32_t __byte_perm(uint32_t x, uint32_t y, uint32_t z);
uint32_t __shfl2(uint32_t x, uint32_t y);
uint32_t __shfl_sync(uint32_t mask, uint32_t x, uint32_t y);
uint32_t atomicExch(uint32_t *x, uint32_t y);
uint32_t atomicAdd(uint32_t *x, uint32_t y);
void __syncthreads(void);
void __threadfence(void);
void __threadfence_block(void);
uint32_t __ldg(const uint32_t* address);
uint64_t __ldg(const uint64_t* address);
uint4 __ldca(const uint4 *ptr);
u32 __ldca(const u32 *ptr);
u32 umin(const u32, const u32);
u32 umax(const u32, const u32);
#endif

#define OPT_SYNC_ALL

#if CUDA_VERSION >= 9000 && __CUDA_ARCH__ >= 300
#define __shfl2(var, srcLane)  __shfl_sync(0xFFFFFFFFu, var, srcLane)
#undef __any
#define __any(p) __any_sync(0xFFFFFFFFu, p)
#else
#define __shfl2 __shfl
#endif

struct __align__(32) slot {
	u32 hash[8];
};

struct __align__(16) slotsmall {
	u32 hash[4];
};

struct __align__(8) slottiny {
	u32 hash[2];
};

struct __align__(16) slotmix{
	slotsmall treessmall[128];
	slottiny treestiny[128];
};

typedef slot		bucket[128];
typedef slotsmall	bucketsmall[128];
typedef slottiny	buckettiny[128];

template <u32 RB, u32 SM>
struct equi
{
	bucket		*round0trees;
	bucket		*round1trees;
	bucket		*round2trees;
	bucket		*round3trees;
	bucketsmall	*round4trees;
	bucketsmall	*round5trees;
	buckettiny	*round6trees;
    void        *reserved;
    union {
        u64 blake_h[8];
        u32 blake_h32[16];
    };
    struct {
		u32 nslots[7][NBUCKETS];
        u32 nonce[4];
	} edata;
	scontainerreal srealcont;
};

#define byteswap32(x) \
    ( (((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) | \
    (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24) )


// todo: use cuda_helper.h and/or cuda_vector.h
__device__ __forceinline__ uint2 operator^ (uint2 a, uint2 b)
{
	return make_uint2(a.x ^ b.x, a.y ^ b.y);
}

__device__ __forceinline__ uint4 operator^ (uint4 a, uint4 b)
{
	return make_uint4(a.x ^ b.x, a.y ^ b.y, a.z ^ b.z, a.w ^ b.w);
}

// for ROR 63 (or ROL 1); this func only support (32 <= offset < 64)
__device__ __forceinline__ uint2 ROR2(const uint2 a, const int offset)
{
	uint2 result;
#if __CUDA_ARCH__ > 300
	{
		asm("shf.r.wrap.b32 %0, %1, %2, %3;" : "=r"(result.x) : "r"(a.y), "r"(a.x), "r"(offset));
		asm("shf.r.wrap.b32 %0, %1, %2, %3;" : "=r"(result.y) : "r"(a.x), "r"(a.y), "r"(offset));
	}
#else
	result.y = ((a.x >> (offset - 32)) | (a.y << (64 - offset)));
	result.x = ((a.y >> (offset - 32)) | (a.x << (64 - offset)));
#endif
	return result;
}


__device__ __forceinline__ uint2 SWAPUINT2(uint2 value)
{
	return make_uint2(value.y, value.x);
}

__device__ __forceinline__ uint2 ROR24(const uint2 a)
{
	uint2 result;
	result.x = __byte_perm(a.y, a.x, 0x2107);
	result.y = __byte_perm(a.y, a.x, 0x6543);
	return result;
}

__device__ __forceinline__ uint2 ROR16(const uint2 a)
{
	uint2 result;
	result.x = __byte_perm(a.y, a.x, 0x1076);
	result.y = __byte_perm(a.y, a.x, 0x5432);
	return result;
}

__device__ __forceinline__ void G2(u64 & a, u64 & b, u64 & c, u64 & d, u64 x, u64 y)
{
	a = a + b + x;
	((uint2*)&d)[0] = SWAPUINT2(((uint2*)&d)[0] ^ ((uint2*)&a)[0]);
	c = c + d;
	((uint2*)&b)[0] = ROR24(((uint2*)&b)[0] ^ ((uint2*)&c)[0]);
	a = a + b + y;
	((uint2*)&d)[0] = ROR16(((uint2*)&d)[0] ^ ((uint2*)&a)[0]);
	c = c + d;
	((uint2*)&b)[0] = ROR2(((uint2*)&b)[0] ^ ((uint2*)&c)[0], 63U);
}

// untested..
struct packer_default
{
	__device__ __forceinline__ static u32 set_bucketid_and_slots(const u32 bucketid, const u32 s0, const u32 s1, const u32 RB)
	{
		return (((bucketid << SLOTBITS) | s0) << SLOTBITS) | s1;
	}

	__device__ __forceinline__ static u32 get_bucketid(const u32 bid, const u32 RB, const u32 SM)
	{
		// BUCKMASK-ed to prevent illegal memory accesses in case of memory errors
		return (bid >> (2 * SLOTBITS)) & BUCKMASK;
	}

	__device__ __forceinline__ static u32 get_slot0(const u32 bid, const u32 s1, const u32 RB, const u32 SM)
	{
		return bid & SLOTMASK;
	}

	__device__ __forceinline__ static u32 get_slot1(const u32 bid, const u32 RB, const u32 SM)
	{
		return (bid >> SLOTBITS) & SLOTMASK;
	}
};


struct packer_cantor
{
	__device__ __forceinline__ static u32 cantor(const u32 s0, const u32 s1)
	{
		u32 a = umax(s0, s1);
		u32 b = umin(s0, s1);
		return a * (a + 1) / 2 + b;
	}

	__device__ __forceinline__ static u32 set_bucketid_and_slots(const u32 bucketid, const u32 s0, const u32 s1, const u32 RB)
	{
		return (bucketid << CANTORBITS) | cantor(s0, s1);
	}

	__device__ __forceinline__ static u32 get_bucketid(const u32 bid, const u32 RB, const u32 SM)
	{
		return (bid >> CANTORBITS) & BUCKMASK;
	}

	__device__ __forceinline__ static u32 get_slot0(const u32 bid, const u32 s1, const u32 RB, const u32 SM)
	{
		return ((bid & CANTORMASK) - cantor(0, s1)) & SLOTMASK;
	}

	__device__ __forceinline__ static u32 get_slot1(const u32 bid, const u32 RB, const u32 SM)
	{
		u32 k, q, sqr = 8 * (bid & CANTORMASK) + 1;
		// this k=sqrt(sqr) computing loop averages 3.4 iterations out of maximum 9
		for (k = CANTORMAXSQRT; (q = sqr / k) < k; k = (k + q) / 2);
		return ((k - 1) / 2) & SLOTMASK;
	}
};

__device__ __constant__ const u64 blake_iv[] = {
	0x6a09e667f3bcc908, 0xbb67ae8584caa73b,
	0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
	0x510e527fade682d1, 0x9b05688c2b3e6c1f,
	0x1f83d9abfb41bd6b, 0x5be0cd19137e2179,
};

__device__ __constant__ const u8 blake2b_sigma[12][16] =
{
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 }, // 1
    { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 }, // 2
    { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 }, // 3
    {  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 }, // 4
    {  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 }, // 5
    {  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 }, // 6
    { 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 }, // 7
    { 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 }, // 8
    {  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 }, // 9
    { 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13,  0 }, // 10
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 }, // 11
    { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 }, // 12
};


#if CUDART_VERSION < 8000 || !defined(__ldca)
#define __ldca(ptr) *(ptr)
#endif

template <u32 RB, u32 SM, typename PACKER>
__global__ void digit_first(equi<RB, SM>* eq, u8 *buf, const u32 nonce)
{
	const u32 block = blockIdx.x * blockDim.x + threadIdx.x;
	__shared__ u64 hash_h[8];

	u32* hash_h32 = (u32*)hash_h;

	if (threadIdx.x < 16)
	{
		hash_h32[threadIdx.x] = __ldca(&eq->blake_h32[threadIdx.x]);
    }
    
	if ( block == 0 )
		eq->edata.nonce[0] = nonce;	

	__syncthreads();
	
    u64 val1 = 0, val9 = 0;	
	u8 len = 76;
	u64 tmpbuf[16];

	*(ulonglong4 *)(tmpbuf + 0) = *(ulonglong4 *)(buf + 0);
	*(ulonglong4 *)(tmpbuf + 4) = *(ulonglong4 *)(buf + 32);
	*(ulonglong4 *)(tmpbuf + 8) = *(ulonglong4 *)(buf + 64);
	*((u32 *)tmpbuf + 14) = nonce;

	val1 = tmpbuf[1];
    val9 = tmpbuf[9] | (u64)block<<(len%8*8);

	union
	{
		u64 v[16];
		u32 v32[32];
		uint4 v128[8];
	};

	v[0] = hash_h[0];
	v[1] = hash_h[1];
	v[2] = hash_h[2];
	v[3] = hash_h[3];
	v[4] = hash_h[4];
	v[5] = hash_h[5];
	v[6] = hash_h[6];
	v[7] = hash_h[7];
	v[8] = blake_iv[0];
	v[9] = blake_iv[1];
	v[10] = blake_iv[2];
	v[11] = blake_iv[3];
	v[12] = blake_iv[4] ^ (128 + len + 4);
	v[13] = blake_iv[5];
	v[14] = blake_iv[6] ^ 0xffffffffffffffff;
	v[15] = blake_iv[7];

	// mix 1
    const u8  *s = blake2b_sigma[0];
	G2(v[0], v[4], v[8], v[12], tmpbuf[s[0]], val1 );
	G2(v[1], v[5], v[9], v[13], tmpbuf[s[2]], tmpbuf[s[3]]);
	G2(v[2], v[6], v[10], v[14], tmpbuf[s[4]], tmpbuf[s[5]]);
	G2(v[3], v[7], v[11], v[15], tmpbuf[s[6]], tmpbuf[s[7]]);
	G2(v[0], v[5], v[10], v[15], tmpbuf[s[8]], val9);
	G2(v[1], v[6], v[11], v[12], tmpbuf[s[10]], tmpbuf[s[11]]);
	G2(v[2], v[7], v[8], v[13], 0, 0);
	G2(v[3], v[4], v[9], v[14], 0, 0);

	// mix 2
	s = blake2b_sigma[1];
	G2(v[0], v[4], v[8], v[12], 0, tmpbuf[s[1]]);
	G2(v[1], v[5], v[9], v[13], tmpbuf[s[2]], tmpbuf[s[3]]);
	G2(v[2], v[6], v[10], v[14], val9, 0);
	G2(v[3], v[7], v[11], v[15], 0, tmpbuf[s[7]]);
	G2(v[0], v[5], v[10], v[15], val1, 0);
	G2(v[1], v[6], v[11], v[12], tmpbuf[s[10]], tmpbuf[s[11]]);
	G2(v[2], v[7], v[8], v[13], tmpbuf[s[12]], tmpbuf[s[13]]);
	G2(v[3], v[4], v[9], v[14], tmpbuf[s[14]], tmpbuf[s[15]]);

	// mix 3
	s = blake2b_sigma[2];
	G2(v[0], v[4], v[8], v[12], tmpbuf[s[0]], tmpbuf[s[1]]);
	G2(v[1], v[5], v[9], v[13], 0, tmpbuf[s[3]]);
	G2(v[2], v[6], v[10], v[14], tmpbuf[s[4]], tmpbuf[s[5]]);
	G2(v[3], v[7], v[11], v[15], 0, 0);
	G2(v[0], v[5], v[10], v[15], tmpbuf[s[8]], 0);
	G2(v[1], v[6], v[11], v[12], tmpbuf[s[10]], tmpbuf[s[11]]);
	G2(v[2], v[7], v[8], v[13], tmpbuf[s[12]], val1);
	G2(v[3], v[4], v[9], v[14], val9, tmpbuf[s[15]]);

	// mix 4
	s = blake2b_sigma[3];
	G2(v[0], v[4], v[8], v[12], tmpbuf[s[0]], val9);
	G2(v[1], v[5], v[9], v[13], tmpbuf[s[2]], val1);
	G2(v[2], v[6], v[10], v[14], 0, 0);
	G2(v[3], v[7], v[11], v[15], tmpbuf[s[6]], 0);
	G2(v[0], v[5], v[10], v[15], tmpbuf[s[8]], tmpbuf[s[9]]);
	G2(v[1], v[6], v[11], v[12], tmpbuf[s[10]], tmpbuf[s[11]]);
	G2(v[2], v[7], v[8], v[13], tmpbuf[s[12]], tmpbuf[s[13]]);
	G2(v[3], v[4], v[9], v[14], 0, tmpbuf[s[15]]); 

	// mix 5
	s = blake2b_sigma[4];
	G2(v[0], v[4], v[8], v[12], val9, tmpbuf[s[1]]);
	G2(v[1], v[5], v[9], v[13], tmpbuf[s[2]], tmpbuf[s[3]]);
	G2(v[2], v[6], v[10], v[14], tmpbuf[s[4]], tmpbuf[s[5]]);
	G2(v[3], v[7], v[11], v[15], tmpbuf[s[6]], 0);
	G2(v[0], v[5], v[10], v[15], 0, val1);
	G2(v[1], v[6], v[11], v[12], tmpbuf[s[10]], 0);
	G2(v[2], v[7], v[8], v[13], tmpbuf[s[12]], tmpbuf[s[13]]);
	G2(v[3], v[4], v[9], v[14], tmpbuf[s[14]], 0);

	// mix 6
	s = blake2b_sigma[5];
	G2(v[0], v[4], v[8], v[12], tmpbuf[s[0]], 0);
	G2(v[1], v[5], v[9], v[13], tmpbuf[s[2]], tmpbuf[s[3]]);
	G2(v[2], v[6], v[10], v[14], tmpbuf[s[4]], tmpbuf[s[5]]);
	G2(v[3], v[7], v[11], v[15], tmpbuf[s[6]], tmpbuf[s[7]]);
	G2(v[0], v[5], v[10], v[15], tmpbuf[s[8]], 0);
	G2(v[1], v[6], v[11], v[12], tmpbuf[s[10]], tmpbuf[s[11]]);
	G2(v[2], v[7], v[8], v[13], 0, 0);
	G2(v[3], v[4], v[9], v[14], val1, val9);

	// mix 7
	s = blake2b_sigma[6];
	G2(v[0], v[4], v[8], v[12], 0, tmpbuf[s[1]]);
	G2(v[1], v[5], v[9], v[13], val1, 0);
	G2(v[2], v[6], v[10], v[14], 0, 0);
	G2(v[3], v[7], v[11], v[15], tmpbuf[s[6]], tmpbuf[s[7]]);
	G2(v[0], v[5], v[10], v[15], tmpbuf[s[8]], tmpbuf[s[9]]);
	G2(v[1], v[6], v[11], v[12], tmpbuf[s[10]], tmpbuf[s[11]]);
	G2(v[2], v[7], v[8], v[13], val9, tmpbuf[s[13]]);
	G2(v[3], v[4], v[9], v[14], tmpbuf[s[14]], tmpbuf[s[15]]);

	// mix 8
	s = blake2b_sigma[7];
	G2(v[0], v[4], v[8], v[12], 0, tmpbuf[s[1]]);
	G2(v[1], v[5], v[9], v[13], tmpbuf[s[2]], 0);
	G2(v[2], v[6], v[10], v[14], 0, val1);
	G2(v[3], v[7], v[11], v[15], tmpbuf[s[6]], val9);
	G2(v[0], v[5], v[10], v[15], tmpbuf[s[8]], tmpbuf[s[9]]);
	G2(v[1], v[6], v[11], v[12], 0, tmpbuf[s[11]]);
	G2(v[2], v[7], v[8], v[13], tmpbuf[s[12]], tmpbuf[s[13]]);
	G2(v[3], v[4], v[9], v[14], tmpbuf[s[14]], tmpbuf[s[15]]);

	// mix 9
	s = blake2b_sigma[8];
	G2(v[0], v[4], v[8], v[12], tmpbuf[s[0]], 0);
	G2(v[1], v[5], v[9], v[13], 0, val9);
	G2(v[2], v[6], v[10], v[14], tmpbuf[s[4]], tmpbuf[s[5]]);
	G2(v[3], v[7], v[11], v[15], tmpbuf[s[6]], tmpbuf[s[7]]);
	G2(v[0], v[5], v[10], v[15], 0, tmpbuf[s[9]]);
	G2(v[1], v[6], v[11], v[12], 0, tmpbuf[s[11]]);
	G2(v[2], v[7], v[8], v[13], val1, tmpbuf[s[13]]);
	G2(v[3], v[4], v[9], v[14], tmpbuf[s[14]], tmpbuf[s[15]]);

	// mix 10
	s = blake2b_sigma[9];
	G2(v[0], v[4], v[8], v[12], tmpbuf[s[0]], tmpbuf[s[1]]);
	G2(v[1], v[5], v[9], v[13], tmpbuf[s[2]], tmpbuf[s[3]]);
	G2(v[2], v[6], v[10], v[14], tmpbuf[s[4]], tmpbuf[s[5]]);
	G2(v[3], v[7], v[11], v[15], val1, tmpbuf[s[7]]);
	G2(v[0], v[5], v[10], v[15], 0, tmpbuf[s[9]]);
	G2(v[1], v[6], v[11], v[12], val9, 0);
	G2(v[2], v[7], v[8], v[13], tmpbuf[s[12]], 0);
	G2(v[3], v[4], v[9], v[14], 0, tmpbuf[s[15]]);

	// mix 11
	s = blake2b_sigma[10];
	G2(v[0], v[4], v[8], v[12], tmpbuf[s[0]], val1);
	G2(v[1], v[5], v[9], v[13], tmpbuf[s[2]], tmpbuf[s[3]]);
	G2(v[2], v[6], v[10], v[14], tmpbuf[s[4]], tmpbuf[s[5]]);
	G2(v[3], v[7], v[11], v[15], tmpbuf[s[6]], tmpbuf[s[7]]);
	G2(v[0], v[5], v[10], v[15], tmpbuf[s[8]], val9);
	G2(v[1], v[6], v[11], v[12], tmpbuf[s[10]], tmpbuf[s[11]]);
	G2(v[2], v[7], v[8], v[13], 0, 0);
	G2(v[3], v[4], v[9], v[14], 0, 0);

	// mix 12
	s = blake2b_sigma[11];
	G2(v[0], v[4], v[8], v[12], 0, tmpbuf[s[1]]);
	G2(v[1], v[5], v[9], v[13], tmpbuf[s[2]], tmpbuf[s[3]]);
	G2(v[2], v[6], v[10], v[14], val9, 0);
	G2(v[3], v[7], v[11], v[15], 0, tmpbuf[s[7]]);
	G2(v[0], v[5], v[10], v[15], val1, 0);
	G2(v[1], v[6], v[11], v[12], tmpbuf[s[10]], tmpbuf[s[11]]);
	G2(v[2], v[7], v[8], v[13], tmpbuf[s[12]], tmpbuf[s[13]]);
	G2(v[3], v[4], v[9], v[14], tmpbuf[s[14]], tmpbuf[s[15]]);

	v[0] ^= hash_h[0] ^ v[8];
	v[1] ^= hash_h[1] ^ v[9];
	v[2] ^= hash_h[2] ^ v[10];
	v[3] ^= hash_h[3] ^ v[11];
	v[4] ^= hash_h[4] ^ v[12];
	v[5] ^= hash_h[5] ^ v[13];
    v[6] ^= hash_h[6] ^ v[14];
    v[7] ^= hash_h[7] ^ v[15];

	u32 bucketid;
    u32 bexor = __byte_perm(v32[0], 0, 0x4012); // first 20 bits
    asm("bfe.u32 %0, %1, 6, 18;" : "=r"(bucketid) : "r"(bexor));
	u32 slotp = atomicAdd(&eq->edata.nslots[0][bucketid], 1);
	if (slotp < RB_NSLOTS)
	{
		slot* s = &eq->round0trees[bucketid][slotp];

		ulonglong4 tt;
		tt.x = __byte_perm(v32[0], v32[1], 0x2345) | ((u64)__byte_perm(v32[1], v32[2], 0x2345) << 32);
		tt.y = __byte_perm(v32[2], v32[3], 0x2345) | ((u64)__byte_perm(v32[3], v32[4], 0x2345) << 32);
		tt.z = __byte_perm(v32[4], v32[5], 0x2345) | ((u64)__byte_perm(v32[5], v32[6], 0x2345) << 32);
		tt.w = (u64)block << 33;

		*(ulonglong4*)(&s->hash[0]) = tt;
	}

    bexor = __byte_perm(v32[5], v32[6], 0x3456);
    asm("bfe.u32 %0, %1, 14, 18;" : "=r"(bucketid) : "r"(bexor));

    slotp = atomicAdd(&eq->edata.nslots[0][bucketid], 1);
	if (slotp < RB_NSLOTS)
	{
		slot* s = &eq->round0trees[bucketid][slotp];

		ulonglong4 tt;
		tt.x = __byte_perm(v32[6], v32[7], 0x1234) | ((u64)__byte_perm(v32[7], v32[8], 0x1234)<<32);
		tt.y = __byte_perm(v32[8], v32[9], 0x1234) | ((u64)__byte_perm(v32[9], v32[10], 0x1234)<<32);
		tt.z = __byte_perm(v32[10], v32[11], 0x1234) | ((u64)__byte_perm(v32[11], v32[12], 0x1234)<<32);
		tt.w = (((u64)block << 1) + 1)<<32;

		*(ulonglong4*)(&s->hash[0]) = tt;
	}
}

/*
  Functions digit_1 to digit_8 works by the same principle;
  Each thread does 2-3 slot loads (loads are coalesced).
  Xorwork of slots is loaded into shared memory and is kept in registers (except for digit_1).
  At the same time, restbits (8 or 9 bits) in xorwork are used for collisions.
  Restbits determine position in ht.
  Following next is pair creation. First one (or two) pairs' xorworks are put into global memory
  as soon as possible, the rest pairs are saved in shared memory (one u32 per pair - 16 bit indices).
  In most cases, all threads have one (or two) pairs so with this trick, we offload memory writes a bit in last step.
  In last step we save xorwork of pairs in memory.
*/
template <u32 RB, u32 SM, int SSM, typename PACKER, u32 MAXPAIRS, u32 THREADS>
__global__ void digit_1(equi<RB, SM>* eq)
{
	__shared__ u16 ht[THREADS][SSM - 1];
	__shared__ uint2 lastword1[RB_NSLOTS];
	__shared__ uint4 lastword2[RB_NSLOTS];
	__shared__ int ht_len[MAXPAIRS];
	__shared__ u32 pairs_len;
	__shared__ u32 next_pair;

	const u32 threadid = threadIdx.x;
	const u32 bucketid = blockIdx.x;   

	// reset hashtable len
	if (threadid < NRESTS)
    {
		ht_len[threadid] = 0;
    }

    pairs_len = 0;
    next_pair = 0;    

	u32 bsize = umin(eq->edata.nslots[0][bucketid], RB_NSLOTS);

	u32 hr[2];
	int pos[2];
	pos[0] = pos[1] = SSM;

	uint2 ta[2];
	uint4 tb[2];

	u32 si[2];

#ifdef OPT_SYNC_ALL
	// enable this to make fully safe shared mem operations;
	// disabled gains some speed, but can rarely cause a crash
	__syncthreads();
#endif
#pragma unroll
	for (u32 i = 0; i < 2; ++i)
	{
		si[i] = i * THREADS + threadid;
		if (si[i] >= bsize) break;

        const slot* pslot1 = eq->round0trees[bucketid] + si[i];
		// get xhash
		uint4 a1 = *(uint4*)(&pslot1->hash[0]);
		uint2 a2 = *(uint2*)(&pslot1->hash[4]);
		ta[i].x = a1.x;
		ta[i].y = a1.y;
		lastword1[si[i]] = ta[i];
		tb[i].x = a1.z;
		tb[i].y = a1.w;
		tb[i].z = a2.x;
		tb[i].w = a2.y;
		lastword2[si[i]] = tb[i];

        asm("bfe.u32 %0, %1, 25, 5;" : "=r"(hr[i]) : "r"(ta[i].x));

        pos[i] = atomicAdd(&ht_len[hr[i]], 1);
		if (pos[i] < (SSM - 1)) 
            ht[hr[i]][pos[i]] = si[i];
	}
	__syncthreads();
	int* pairs = ht_len;

	u32 xors[6];
	u32 xorbucketid, xorslot;

#pragma unroll
	for (u32 i = 0; i < 2; ++i)
	{
		if (pos[i] >= SSM) continue;

		if (pos[i] > 0)
		{
			u16 p = ht[hr[i]][0];

			*(uint2*)(&xors[0]) = ta[i] ^ lastword1[p];

            asm("bfe.u32 %0, %1, 7, %2;" : "=r"(xorbucketid) : "r"(xors[0]), "r"(BUCKBITS));

            xorslot = atomicAdd(&eq->edata.nslots[1][xorbucketid], 1);

			if (xorslot < NSLOTS)
			{
				*(uint4*)(&xors[2]) = lastword2[si[i]] ^ lastword2[p];

				slot &xs = eq->round1trees[xorbucketid][xorslot];
                		ulonglong4 ttx;
				ttx.x = __byte_perm(xors[0], xors[1], 0x0765) | ((u64)__byte_perm(xors[1], xors[2], 0x0765)<<32);
				ttx.y = __byte_perm(xors[2], xors[3], 0x0765) | ((u64)__byte_perm(xors[3], xors[4], 0x0765)<<32);
				ttx.z = __byte_perm(xors[4], xors[5], 0x0765) | ((u64)PACKER::set_bucketid_and_slots(bucketid, si[i], p, RB)<<32);
				*(ulonglong4*)(&xs.hash[0]) = ttx;
			}

			for (int k = 1; k < pos[i]; ++k)
			{
				u32 pindex = atomicAdd(&pairs_len, 1);
				if (pindex >= MAXPAIRS) break;
				u16 prev = ht[hr[i]][k];
                pairs[pindex] = __byte_perm(si[i], prev, 0x1054);
			}
		}
	}

	__syncthreads();

	// process pairs
	u32 plen = umin(pairs_len, MAXPAIRS);

	u32 i, k;
	for (u32 s = atomicAdd(&next_pair, 1); s < plen; s = atomicAdd(&next_pair, 1))
	{
		int pair = pairs[s];
		i = __byte_perm(pair, 0, 0x4510);
		k = __byte_perm(pair, 0, 0x4532);

		*(uint2*)(&xors[0]) = lastword1[i] ^ lastword1[k];

        asm("bfe.u32 %0, %1, 7, %2;" : "=r"(xorbucketid) : "r"(xors[0]), "r"(BUCKBITS));
		
        xorslot = atomicAdd(&eq->edata.nslots[1][xorbucketid], 1);

		if (xorslot < NSLOTS)
		{
			*(uint4*)(&xors[2]) = lastword2[i] ^ lastword2[k];

			slot &xs = eq->round1trees[xorbucketid][xorslot];
			ulonglong4 ttx;
			ttx.x = __byte_perm(xors[0], xors[1], 0x0765) | ((u64)__byte_perm(xors[1], xors[2], 0x0765) << 32);
			ttx.y = __byte_perm(xors[2], xors[3], 0x0765) | ((u64)__byte_perm(xors[3], xors[4], 0x0765) << 32);
			ttx.z = __byte_perm(xors[4], xors[5], 0x0765) | ((u64)PACKER::set_bucketid_and_slots(bucketid, i, k, RB) << 32);
			*(ulonglong4*)(&xs.hash[0]) = ttx;
		}
	}
}

template <u32 RB, u32 SM, int SSM, typename PACKER, u32 MAXPAIRS, u32 THREADS>
__global__ void digit_2(equi<RB, SM>* eq)
{
	__shared__ u16 ht[NRESTS][SSM - 1];
	__shared__ uint4 lastword1[NSLOTS];
	__shared__ uint2 lastword2[NSLOTS];
	__shared__ int ht_len[NRESTS];
	__shared__ int pairs[MAXPAIRS];
	__shared__ u32 pairs_len;
	__shared__ u32 next_pair;

	const u32 threadid = threadIdx.x;
	const u32 bucketid = blockIdx.x;

	// reset hashtable len
	if (threadid < NRESTS)
		ht_len[threadid] = 0;
	else if (threadid == (THREADS - 1))
		pairs_len = 0;
	else if (threadid == (THREADS - 33))
		next_pair = 0;

    pairs_len = 0;
    next_pair = 0;    

	u32 bsize = umin(eq->edata.nslots[1][bucketid], NSLOTS);

	u32 hr[2];
	int pos[2];
	pos[0] = pos[1] = SSM;

	uint4 ta[2];
	uint2 tb[2];

	u32 si[2];
#ifdef OPT_SYNC_ALL
	__syncthreads();
#endif
#pragma unroll 2
	for (u32 i = 0; i < 2; i++)
	{
		si[i] = i * THREADS + threadid;
		if (si[i] >= bsize) break;

        slot &xs = eq->round1trees[bucketid][si[i]];

        ta[i] = *(uint4*)(&xs.hash[0]);
        lastword1[si[i]] = ta[i];

        tb[i] = *(uint2*)(&xs.hash[4]);
        lastword2[si[i]] = tb[i];

        asm("bfe.u32 %0, %1, 26, %2;" : "=r"(hr[i]) : "r"(ta[i].x), "r"(RB));
        pos[i] = atomicAdd(&ht_len[hr[i]], 1);
        if (pos[i] < (SSM - 1)) ht[hr[i]][pos[i]] = si[i];
	}

    __syncthreads();

	u32 xors[5];
	u32 xorbucketid, xorslot;

#pragma unroll 2
	for (u32 i = 0; i < 2; i++)
	{
		if (pos[i] >= SSM) continue;

		if (pos[i] > 0)
		{
			u16 p = ht[hr[i]][0];

            *(uint4*)(&xors[0]) = ta[i] ^ lastword1[p];

            asm("bfe.u32 %0, %1, 8, %2;" : "=r"(xorbucketid) : "r"(xors[0]), "r"(BUCKBITS));

            xorslot = atomicAdd(&eq->edata.nslots[2][xorbucketid], 1);
			if (xorslot < NSLOTS)
			{
                		*(uint2*)(&xors[4]) = lastword2[si[i]] ^ lastword2[p];

				slot &xs = eq->round2trees[xorbucketid][xorslot];

				ulonglong4 ttx;
				ttx.x = __byte_perm(xors[0], xors[1], 0x0765) | ((u64)__byte_perm(xors[1], xors[2], 0x0765)<<32);
				ttx.y = __byte_perm(xors[2], xors[3], 0x0765) | ((u64)__byte_perm(xors[3], xors[4], 0x0765)<<32);				
				ttx.z = (u64)PACKER::set_bucketid_and_slots(bucketid, si[i], p, RB)<<32;
				ttx.w = 0;
				*(ulonglong4 *)(&xs.hash[0]) = ttx;

			}

			for (int k = 1; k != pos[i]; ++k)
			{
				u32 pindex = atomicAdd(&pairs_len, 1);
				if (pindex >= MAXPAIRS) break;
				u16 prev = ht[hr[i]][k];
				pairs[pindex] = __byte_perm(si[i], prev, 0x1054);
			}            
		}
	}

    __syncthreads();

	// process pairs
	u32 plen = umin(pairs_len, MAXPAIRS);

	u32 i, k;
	for (u32 s = atomicAdd(&next_pair, 1); s < plen; s = atomicAdd(&next_pair, 1))
	{
		int pair = pairs[s];
		i = __byte_perm(pair, 0, 0x4510);
		k = __byte_perm(pair, 0, 0x4532);
        *(uint4*)(&xors[0]) = lastword1[i] ^ lastword1[k];
        asm("bfe.u32 %0, %1, 8, %2;" : "=r"(xorbucketid) : "r"(xors[0]), "r"(BUCKBITS));

		xorslot = atomicAdd(&eq->edata.nslots[2][xorbucketid], 1);
		if (xorslot < NSLOTS)
		{
            		*(uint2*)(&xors[4]) = lastword2[i] ^ lastword2[k];

			slot &xs = eq->round2trees[xorbucketid][xorslot];

			ulonglong4 ttx;
			ttx.x = __byte_perm(xors[0], xors[1], 0x0765) | ((u64)__byte_perm(xors[1], xors[2], 0x0765)<<32);
			ttx.y = __byte_perm(xors[2], xors[3], 0x0765) | ((u64)__byte_perm(xors[3], xors[4], 0x0765)<<32);
			ttx.z = (u64)PACKER::set_bucketid_and_slots(bucketid, i, k, RB) << 32;
			ttx.w = 0;
			*(ulonglong4 *)(&xs.hash[0]) = ttx;
		}
	}
}


template <u32 RB, u32 SM, int SSM, typename PACKER, u32 MAXPAIRS, u32 THREADS>
__global__ void digit_3(equi<RB, SM>* eq)
{
	__shared__ u16 ht[NRESTS][(SSM - 1)];
	__shared__ uint4 lastword1[NSLOTS];
	__shared__ int ht_len[NRESTS];
	__shared__ int pairs[MAXPAIRS];
	__shared__ u32 pairs_len;
	__shared__ u32 next_pair;

	const u32 threadid = threadIdx.x;
	const u32 bucketid = blockIdx.x;

	// reset hashtable len
	if (threadid < NRESTS)
		ht_len[threadid] = 0;
	else if (threadid == (THREADS - 1))
		pairs_len = 0;
	else if (threadid == (THREADS - 33))
		next_pair = 0;

    pairs_len = 0;
    next_pair = 0;    

	u32 bsize = umin(eq->edata.nslots[2][bucketid], NSLOTS);

	u32 hr[2];
	int pos[2];
	pos[0] = pos[1] = SSM;

	u32 si[2];
	uint4 tt[2];

#ifdef OPT_SYNC_ALL
	__syncthreads();
#endif

#pragma unroll 2
	for (u32 i = 0; i < 2; i++)
	{
		si[i] = i * THREADS + threadid;
		if (si[i] >= bsize) break;
		slot &xs = eq->round2trees[bucketid][si[i]];

		tt[i] = *(uint4*)(&xs.hash[0]);
		lastword1[si[i]] = tt[i];
		asm("bfe.u32 %0, %1, 27, %2;" : "=r"(hr[i]) : "r"(tt[i].x), "r"(RB));
		pos[i] = atomicAdd(&ht_len[hr[i]], 1);
		if (pos[i] < (SSM - 1)) ht[hr[i]][pos[i]] = si[i];        
	}

	__syncthreads();

	u32 xors[5];
	u32 xorbucketid, xorslot;

#pragma unroll 2
	for (u32 i = 0; i < 2; i++)
	{
		if (pos[i] >= SSM) continue;

		if (pos[i] > 0)
		{
			u16 p = ht[hr[i]][0];

			*(uint4*)(&xors[0]) = tt[i] ^ lastword1[p];

            asm("bfe.u32 %0, %1, 9, %2;" : "=r"(xorbucketid) : "r"(xors[0]), "r"(BUCKBITS));
            
            xorslot = atomicAdd(&eq->edata.nslots[3][xorbucketid], 1);

			if (xorslot < NSLOTS)
			{
				slot &xs = eq->round3trees[xorbucketid][xorslot];

				ulonglong4 ttx;
				ttx.x = __byte_perm(xors[0], xors[1], 0x1076) | ((u64)__byte_perm(xors[1], xors[2], 0x1076)<<32);
				ttx.y = __byte_perm(xors[2], xors[3], 0x1076) | ((u64)__byte_perm(xors[3], xors[4], 0x1076)<<32);
				ttx.z = (u64)PACKER::set_bucketid_and_slots(bucketid, si[i], p, RB) << 32;
				*(ulonglong4 *)(&xs.hash[0]) = ttx;
			}

			for (int k = 1; k != pos[i]; ++k)
			{
				u32 pindex = atomicAdd(&pairs_len, 1);
				if (pindex >= MAXPAIRS) break;
				u16 prev = ht[hr[i]][k];
				pairs[pindex] = __byte_perm(si[i], prev, 0x1054);
			}
		}
	}

	__syncthreads();

	// process pairs
	u32 plen = umin(pairs_len, MAXPAIRS);

	u32 i, k;
	for (u32 s = atomicAdd(&next_pair, 1); s < plen; s = atomicAdd(&next_pair, 1))
	{
		int pair = pairs[s];
		i = __byte_perm(pair, 0, 0x4510);
		k = __byte_perm(pair, 0, 0x4532);

        *(uint4*)(&xors[0]) = lastword1[i] ^ lastword1[k];

        asm("bfe.u32 %0, %1, 9, %2;" : "=r"(xorbucketid) : "r"(xors[0]), "r"(BUCKBITS));
        xorslot = atomicAdd(&eq->edata.nslots[3][xorbucketid], 1);
        
        if (xorslot < NSLOTS)
		{
			slot &xs = eq->round3trees[xorbucketid][xorslot];

			ulonglong4 ttx;
			ttx.x = __byte_perm(xors[0], xors[1], 0x1076) | ((u64)__byte_perm(xors[1], xors[2], 0x1076)<<32);
			ttx.y = __byte_perm(xors[2], xors[3], 0x1076) | ((u64)__byte_perm(xors[3], xors[4], 0x1076)<<32);
			ttx.z = (u64)PACKER::set_bucketid_and_slots(bucketid, i, k, RB) << 32;
			ttx.w = 0;
			*(ulonglong4 *)(&xs.hash[0]) = ttx;
		}		
	}
}

template <u32 RB, u32 SM, int SSM, typename PACKER, u32 MAXPAIRS, u32 THREADS>
__global__ void digit_4(equi<RB, SM>* eq)
{
	__shared__ u16 ht[NRESTS][(SSM - 1)];
	__shared__ uint4 lastword[NSLOTS];
	__shared__ int ht_len[NRESTS];
	__shared__ int pairs[MAXPAIRS];
	__shared__ u32 pairs_len;
	__shared__ u32 next_pair;

	const u32 threadid = threadIdx.x;
	const u32 bucketid = blockIdx.x;

	// reset hashtable len
	if (threadid < NRESTS)
		ht_len[threadid] = 0;
	else if (threadid == (THREADS - 1))
		pairs_len = 0;
	else if (threadid == (THREADS - 33))
		next_pair = 0;

    pairs_len = 0;
    next_pair = 0;

	u32 bsize = umin(eq->edata.nslots[3][bucketid], NSLOTS);

	u32 hr[2];
	int pos[2];
	pos[0] = pos[1] = SSM;

	u32 si[2];
	uint4 tt[2];
#ifdef OPT_SYNC_ALL
	__syncthreads();
#endif
#pragma unroll 2
	for (u32 i = 0; i < 2; i++)
	{
		si[i] = i * THREADS + threadid;
		if (si[i] >= bsize) break;

		slot &xs = eq->round3trees[bucketid][si[i]];

		// get xhash
		tt[i] = *(uint4*)(&xs.hash[0]);
		lastword[si[i]] = tt[i];

        asm("bfe.u32 %0, %1, 20, %2;" : "=r"(hr[i]) : "r"(tt[i].x), "r"(RB));

		pos[i] = atomicAdd(&ht_len[hr[i]], 1);
		if (pos[i] < (SSM - 1)) ht[hr[i]][pos[i]] = si[i];
	}
   
	__syncthreads();
	u32 xors[4];
	u32 xorbucketid, xorslot;

#pragma unroll 2
	for (u32 i = 0; i < 2; i++)
	{
		if (pos[i] >= SSM) continue;

		if (pos[i] > 0)
		{
			u16 p = ht[hr[i]][0];

			*(uint4*)(&xors[0]) = tt[i] ^ lastword[p];

			asm("bfe.u32 %0, %1, 2, %2;" : "=r"(xorbucketid) : "r"(xors[0]), "r"(BUCKBITS));
			xorslot = atomicAdd(&eq->edata.nslots[4][xorbucketid], 1);
			if (xorslot < NSLOTS)
			{
				slotsmall &xs = eq->round4trees[xorbucketid][xorslot];

                uint4 ttx;
                ttx.x = __byte_perm(xors[0], xors[1], 0x0765);
                ttx.y = __byte_perm(xors[1], xors[2], 0x0765);
                ttx.z = __byte_perm(xors[2], xors[3], 0x0765);

                ttx.w = PACKER::set_bucketid_and_slots(bucketid, si[i], p, RB);
                *(uint4*)(&xs.hash[0]) = ttx;
			}

			for (int k = 1; k != pos[i]; ++k)
			{
				u32 pindex = atomicAdd(&pairs_len, 1);
				if (pindex >= MAXPAIRS) break;
				u16 prev = ht[hr[i]][k];
				pairs[pindex] = __byte_perm(si[i], prev, 0x1054);
			}
		}
	}

	__syncthreads();

	// process pairs
	u32 plen = umin(pairs_len, MAXPAIRS);
	u32 i, k;
	for (u32 s = atomicAdd(&next_pair, 1); s < plen; s = atomicAdd(&next_pair, 1))
	{
		int pair = pairs[s];
		i = __byte_perm(pair, 0, 0x4510);
		k = __byte_perm(pair, 0, 0x4532);

		*(uint4*)(&xors[0]) = lastword[i] ^ lastword[k];

        asm("bfe.u32 %0, %1, 2, %2;" : "=r"(xorbucketid) : "r"(xors[0]), "r"(BUCKBITS));
        xorslot = atomicAdd(&eq->edata.nslots[4][xorbucketid], 1);
        if (xorslot < NSLOTS)
        {
            slotsmall &xs = eq->round4trees[xorbucketid][xorslot];

            uint4 ttx;
            ttx.x = __byte_perm(xors[0], xors[1], 0x0765);
            ttx.y = __byte_perm(xors[1], xors[2], 0x0765);
            ttx.z = __byte_perm(xors[2], xors[3], 0x0765);

            ttx.w = PACKER::set_bucketid_and_slots(bucketid, i, k, RB);
            *(uint4*)(&xs.hash[0]) = ttx;
        }
    }   
}

template <u32 RB, u32 SM, int SSM, typename PACKER, u32 MAXPAIRS, u32 THREADS>
__global__ void digit_5(equi<RB, SM>* eq)
{
	__shared__ u16 ht[NRESTS][(SSM - 1)];
	__shared__ uint4 lastword[NSLOTS];
	__shared__ int ht_len[NRESTS];
	__shared__ int pairs[MAXPAIRS];
	__shared__ u32 pairs_len;
	__shared__ u32 next_pair;

	const u32 threadid = threadIdx.x;
	const u32 bucketid = blockIdx.x;

	if (threadid < NRESTS)
		ht_len[threadid] = 0;
	else if (threadid == (THREADS - 1))
		pairs_len = 0;
	else if (threadid == (THREADS - 33))
		next_pair = 0;

    pairs_len = 0;
    next_pair = 0;

	u32 bsize = umin(eq->edata.nslots[4][bucketid], NSLOTS);

	u32 hr[2];
	int pos[2];
	pos[0] = pos[1] = SSM;

	u32 si[2];
	uint4 tt[2];
#ifdef OPT_SYNC_ALL
	__syncthreads();
#endif
#pragma unroll 2
	for (u32 i = 0; i < 2; i++)
	{
		si[i] = i * THREADS + threadid;
		if (si[i] >= bsize) break;

		slotsmall &xs = eq->round4trees[bucketid][si[i]];

		tt[i] = *(uint4*)(&xs.hash[0]);
		lastword[si[i]] = tt[i];

		asm("bfe.u32 %0, %1, 21, %2;" : "=r"(hr[i]) : "r"(tt[i].x), "r"(RB));
		pos[i] = atomicAdd(&ht_len[hr[i]], 1);
		if (pos[i] < (SSM - 1)) ht[hr[i]][pos[i]] = si[i];
	}

	__syncthreads();
	u32 xors[4];
	u32 xorbucketid, xorslot;

#pragma unroll 2
	for (u32 i = 0; i < 2; i++)
	{
		if (pos[i] >= SSM) continue;

		if (pos[i] > 0)
		{
			u16 p = ht[hr[i]][0];

			*(uint4*)(&xors[0]) = tt[i] ^ lastword[p];
				
            asm("bfe.u32 %0, %1, 3, %2;" : "=r"(xorbucketid) : "r"(xors[0]), "r"(BUCKBITS));

			xorslot = atomicAdd(&eq->edata.nslots[5][xorbucketid], 1);
			if (xorslot < NSLOTS)
			{
				slotsmall &xs = eq->round5trees[xorbucketid][xorslot];

                uint4 ttx;
                ttx.x = __byte_perm(xors[0], xors[1], 0x0765);
                ttx.y = __byte_perm(xors[1], xors[2], 0x0765);
                ttx.z = 0;
                ttx.w = PACKER::set_bucketid_and_slots(bucketid, si[i], p, RB);
                *(uint4*)(&xs.hash[0]) = ttx;
			}

			for (int k = 1; k != pos[i]; ++k)
			{
				u32 pindex = atomicAdd(&pairs_len, 1);
				if (pindex >= MAXPAIRS) break;
				u16 prev = ht[hr[i]][k];
				pairs[pindex] = __byte_perm(si[i], prev, 0x1054);
			}
		}
	}

	__syncthreads();

	// process pairs
	u32 plen = umin(pairs_len, MAXPAIRS);
	u32 i, k;
	for (u32 s = atomicAdd(&next_pair, 1); s < plen; s = atomicAdd(&next_pair, 1))
	{
		int pair = pairs[s];
		i = __byte_perm(pair, 0, 0x4510);
		k = __byte_perm(pair, 0, 0x4532);

		*(uint4*)(&xors[0]) = lastword[i] ^ lastword[k];

        asm("bfe.u32 %0, %1, 3, %2;" : "=r"(xorbucketid) : "r"(xors[0]), "r"(BUCKBITS));
        xorslot = atomicAdd(&eq->edata.nslots[5][xorbucketid], 1);
        if (xorslot < NSLOTS)
        {
            slotsmall &xs = eq->round5trees[xorbucketid][xorslot];

            uint4 ttx;
            ttx.x = __byte_perm(xors[0], xors[1], 0x0765);
            ttx.y = __byte_perm(xors[1], xors[2], 0x0765);
            ttx.z = 0;

            ttx.w = PACKER::set_bucketid_and_slots(bucketid, i, k, RB);
            *(uint4*)(&xs.hash[0]) = ttx;
        }
	}
}

template <u32 RB, u32 SM, int SSM, typename PACKER, u32 MAXPAIRS>
__global__ void digit_6(equi<RB, SM>* eq)
{
	__shared__ u16 ht[NRESTS][(SSM - 1)];
	__shared__ uint2 lastword1[NSLOTS];
	__shared__ int ht_len[MAXPAIRS];
	__shared__ u32 pairs_len;
	__shared__ u32 bsize_sh;
	__shared__ u32 next_pair;

	const u32 threadid = threadIdx.x;
	const u32 bucketid = blockIdx.x;

	// reset hashtable len
	ht_len[threadid] = 0;
	if (threadid == (NRESTS - 1))
	{
		pairs_len = 0;
		next_pair = 0;
	}
	else if (threadid == (NRESTS - 33))
		bsize_sh = umin(eq->edata.nslots[5][bucketid], NSLOTS);

    pairs_len = 0;
    next_pair = 0;

	u32 hr[3];
	int pos[3];
	pos[0] = pos[1] = pos[2] = SSM;

	u32 si[3];
	uint2 tt[3];

	__syncthreads();

    bsize_sh = umin(eq->edata.nslots[5][bucketid], NSLOTS);
	u32 bsize = bsize_sh;

#pragma unroll 3
	for (u32 i = 0; i < 3; i++)
	{
		si[i] = i * NRESTS + threadid;
		if (si[i] >= bsize) break;

		slotsmall &xs = eq->round5trees[bucketid][si[i]];

		tt[i] = *(uint2*)(&xs.hash[0]);
		lastword1[si[i]] = tt[i];

		asm("bfe.u32 %0, %1, 22, %2;" : "=r"(hr[i]) : "r"(tt[i].x), "r"(RB));
		pos[i] = atomicAdd(&ht_len[hr[i]], 1);
		if (pos[i] < (SSM - 1)) ht[hr[i]][pos[i]] = si[i];
	}

	// doing this to save shared memory
	int* pairs = ht_len;
	__syncthreads();

	u32 xors[3];
	u32 xorbucketid, xorslot;

#pragma unroll 3
	for (u32 i = 0; i < 3; i++)
	{
		if (pos[i] >= SSM) continue;

		if (pos[i] > 0)
		{
			u16 p = ht[hr[i]][0];

			*(uint2*)(&xors[0]) = *(uint2*)(&tt[i].x) ^ lastword1[p];

            asm("bfe.u32 %0, %1, 4, %2;" : "=r"(xorbucketid) : "r"(xors[0]), "r"(BUCKBITS));
            xorslot = atomicAdd(&eq->edata.nslots[6][xorbucketid], 1);
			if (xorslot < NSLOTS)
			{
				slottiny &xst = eq->round6trees[xorbucketid][xorslot];

				uint2 ttx;
                ttx.x = __byte_perm(xors[0], xors[1], 0x0765);
				ttx.y = PACKER::set_bucketid_and_slots(bucketid, si[i], p, RB);
				*(uint2*)(&xst.hash[0]) = ttx;
			}

			if (pos[i] > 1)
			{
				p = ht[hr[i]][1];
				*(uint2*)(&xors[0]) = *(uint2*)(&tt[i].x) ^ lastword1[p];

                asm("bfe.u32 %0, %1, 4, %2;" : "=r"(xorbucketid) : "r"(xors[0]), "r"(BUCKBITS));
				xorslot = atomicAdd(&eq->edata.nslots[6][xorbucketid], 1);
				if (xorslot < NSLOTS)
				{
                    slottiny &xst = eq->round6trees[xorbucketid][xorslot];

                    uint2 ttx;
                    ttx.x = __byte_perm(xors[0], xors[1], 0x0765);
                    ttx.y = PACKER::set_bucketid_and_slots(bucketid, si[i], p, RB);
                    *(uint2*)(&xst.hash[0]) = ttx;                    
				}

				for (int k = 2; k != pos[i]; ++k)
				{
					u32 pindex = atomicAdd(&pairs_len, 1);
					if (pindex >= MAXPAIRS) break;
					u16 prev = ht[hr[i]][k];
					pairs[pindex] = __byte_perm(si[i], prev, 0x1054);
				}
			}
		}
	}

	__syncthreads();

	// process pairs
	u32 plen = umin(pairs_len, MAXPAIRS);
	for (u32 s = atomicAdd(&next_pair, 1); s < plen; s = atomicAdd(&next_pair, 1))
	{
		u32 pair = pairs[s];
		u32 i = __byte_perm(pair, 0, 0x4510);
		u32 k = __byte_perm(pair, 0, 0x4532);

		*(uint2*)(&xors[0]) = lastword1[i] ^ lastword1[k];

        asm("bfe.u32 %0, %1, 4, %2;" : "=r"(xorbucketid) : "r"(xors[0]), "r"(BUCKBITS));

		xorslot = atomicAdd(&eq->edata.nslots[6][xorbucketid], 1);
		if (xorslot >= NSLOTS) continue;

        slottiny &xst = eq->round6trees[xorbucketid][xorslot];

        uint2 ttx;
        ttx.x = __byte_perm(xors[0], xors[1], 0x0765);
        ttx.y = PACKER::set_bucketid_and_slots(bucketid, i, k, RB);
        *(uint2*)(&xst.hash[0]) = ttx;
	}
}

/*
  Last round function is similar to previous ones but has different ending.
  We use warps to process final candidates. Each warp process one candidate.
  First two bidandsids (u32 of stored bucketid and two slotids) are retreived by
  lane 0 and lane 16, next four bidandsids by lane 0, 8, 16 and 24, ... until
  all lanes in warp have bidandsids from round 4. Next, each thread retreives
  16 indices. While doing so, indices are put into comparison using atomicExch
  to determine if there are duplicates (tromp's method). At the end, if no
  duplicates are found, candidate solution is saved (all indices). Note that this
  dup check method is not exact so CPU dup checking is needed after.
*/
template <u32 RB, u32 SM, int SSM, u32 FCT, typename PACKER, u32 MAXPAIRS, u32 DUPBITS, u32 W>
__global__ void digit_last_wdc(equi<RB, SM>* eq)
{
	__shared__ u8 shared_data[8192];
	int* ht_len = (int*)(&shared_data[0]);
	int* pairs = ht_len;
	u32* lastword = (u32*)(&shared_data[256 * 4]);
	u16* ht = (u16*)(&shared_data[256 * 4 + RB_NSLOTS_LD * 4]);
	u32* pairs_len = (u32*)(&shared_data[8188]);

	const u32 threadid = threadIdx.x;
	const u32 bucketid = blockIdx.x;

	// reset hashtable len
#pragma unroll
	for (u32 i = 0; i < FCT; i++)
	{
		//ht_len[(i * (256 / FCT)) + threadid] = 0;
		ht_len[(i * (64 / FCT)) + threadid] = 0;
	}

	//if (threadid == ((256 / FCT) - 1))
	if (threadid == ((64 / FCT) - 1))
		*pairs_len = 0;

	slottiny* buck = eq->round6trees[bucketid];
	u32 bsize = umin(eq->edata.nslots[6][bucketid], RB_NSLOTS_LD);

	u32 si[3 * FCT];
	u32 hr[3 * FCT];
	int pos[3 * FCT];
	u32 lw[3 * FCT];

#pragma unroll
	for (u32 i = 0; i < (3 * FCT); i++)
		pos[i] = SSM;

	__syncthreads();

#pragma unroll
	for (u32 i = 0; i < (3 * FCT); i++)
	{
		//si[i] = i * (256 / FCT) + threadid;
		si[i] = i * (64 / FCT) + threadid;
		if (si[i] >= bsize) break;

		const slottiny* pslot1 = buck + si[i];

		// get xhash
		uint2 tt = *(uint2*)(&pslot1->hash[0]);
		lw[i] = tt.x;
		lastword[si[i]] = lw[i];

		u32 a;
		asm("bfe.u32 %0, %1, 23, 5;" : "=r"(a) : "r"(lw[i]));
		hr[i] = a;

		pos[i] = atomicAdd(&ht_len[hr[i]], 1);
		if (pos[i] < (SSM - 1))
			ht[hr[i] * (SSM - 1) + pos[i]] = si[i];
	}

	__syncthreads();

#pragma unroll
	for (u32 i = 0; i < (3 * FCT); i++)
	{
		if (pos[i] >= SSM) continue;

		for (int k = 0; k != pos[i]; ++k)
		{
			u16 prev = ht[hr[i] * (SSM - 1) + k];
			if (lw[i] != lastword[prev]) continue;
			u32 pindex = atomicAdd(pairs_len, 1);
			if (pindex >= MAXPAIRS) break;
			
            pairs[pindex] = __byte_perm(si[i], prev, 0x1054);
		}
	}


    __syncthreads();
	u32 plen = umin(*pairs_len, MAXPAIRS);

	u32 lane = threadIdx.x & 0x1f;
	u32 par = threadIdx.x >> 5;

	u32* levels = (u32*)&pairs[MAXPAIRS + (par << DUPBITS)];
	u32* susp = levels;

	while (par < plen)
	{
		int pair = pairs[par];
		par += W;

        u32 slot00 = 0; 
		if (lane % 16 == 0)
		{
			u32 plvl;
			if (lane == 0) 
                slot00 = __byte_perm(pair, 0, 0x4510);
            else
                slot00 = __byte_perm(pair, 0, 0x4532);
            
            plvl = buck[slot00].hash[1]; // slot1 r6

			slotsmall* bucks = eq->round5trees[PACKER::get_bucketid(plvl, RB, SM)];

			u32 slot1 = PACKER::get_slot1(plvl, RB, SM);
			u32 slot0 = PACKER::get_slot0(plvl, slot1, RB, SM);

			levels[lane]        = bucks[slot1].hash[3]; // slot1 r5
			levels[lane + 8]    = bucks[slot0].hash[3]; // slot0 r5
		}

        if (lane % 8 == 0)
        {
            u32 plvl = levels[lane];
            slotsmall* bucks = eq->round4trees[PACKER::get_bucketid(plvl, RB, SM)]; // r4
            u32 slot1 = PACKER::get_slot1(plvl, RB, SM);
            u32 slot0 = PACKER::get_slot0(plvl, slot1, RB, SM);

            levels[lane] = bucks[slot1].hash[3];
            levels[lane+4] = bucks[slot0].hash[3];
        }

		if (lane % 4 == 0)
        {
            u32 plvl = levels[lane];
            u32 slot1 = PACKER::get_slot1(plvl, RB, SM);
            u32 slot0 = PACKER::get_slot0(plvl, slot1, RB, SM);
			levels[lane] = eq->round3trees[PACKER::get_bucketid(plvl, RB, SM)][slot1].hash[5];
			levels[lane + 2] = eq->round3trees[PACKER::get_bucketid(plvl, RB, SM)][slot0].hash[5];
        }

		if (lane % 2 == 0)
        {
            u32 plvl = levels[lane]; 
            u32 slot1 = PACKER::get_slot1(plvl, RB, SM); 
            u32 slot0 = PACKER::get_slot0(plvl, slot1, RB, SM); 
            
			levels[lane] = eq->round2trees[PACKER::get_bucketid(plvl, RB, SM)][slot1].hash[5];
			levels[lane + 1] = eq->round2trees[PACKER::get_bucketid(plvl, RB, SM)][slot0].hash[5];
		}

		u32 ind[4];

		u32 f1 = levels[lane];
		const u32 slot1_v4 = PACKER::get_slot1(f1, RB, SM);
		const u32 slot0_v4 = PACKER::get_slot0(f1, slot1_v4, RB, SM);

		susp[lane] = 0xffffffff;
		susp[lane + 32] = 0xffffffff;
		susp[lane + 64] = 0xffffffff;
		susp[lane + 96] = 0xffffffff;
        susp[128 + lane] = 0xffffffff;
        susp[160 + lane] = 0xffffffff;
        susp[192 + lane] = 0xffffffff;
        susp[224 + lane] = 0xffffffff;

#define CHECK_DUP(a) \
	__any(atomicExch(&susp[(ind[a] & ((1 << DUPBITS) - 1))], (ind[a] >> DUPBITS)) == (ind[a] >> DUPBITS))

		u32 f2 = eq->round1trees[PACKER::get_bucketid(f1, RB, SM)][slot1_v4].hash[5];
		const slot* buck_v3_1 = &eq->round0trees[PACKER::get_bucketid(f2, RB, SM)][0]; // r0
		const u32 slot1_v3_1 = PACKER::get_slot1(f2, RB, SM);
		const u32 slot0_v3_1 = PACKER::get_slot0(f2, slot1_v3_1, RB, SM);

        ind[0] = buck_v3_1[slot1_v3_1].hash[7];
        if (CHECK_DUP(0)) continue;
        ind[1] = buck_v3_1[slot0_v3_1].hash[7];
        if (CHECK_DUP(1)) continue;

		u32 f8 = eq->round1trees[PACKER::get_bucketid(f1, RB, SM)][slot0_v4].hash[5]; // r1
        const slot* buck_v3_2 = &eq->round0trees[PACKER::get_bucketid(f8, RB, SM)][0]; // r0
        const u32 slot1_v3_2 = PACKER::get_slot1(f8, RB, SM);
        const u32 slot0_v3_2 = PACKER::get_slot0(f8, slot1_v3_2, RB, SM);

        ind[2] = buck_v3_2[slot1_v3_2].hash[7];
        if (CHECK_DUP(2)) continue;
        ind[3] = buck_v3_2[slot0_v3_2].hash[7];
        if (CHECK_DUP(3)) continue;
		u32 soli;
		if (lane == 0) 
        {
			soli = atomicAdd(&eq->srealcont.nsols, 1);
		}
#if __CUDA_ARCH__ >= 300
		// all threads get the value from lane 0
		soli = __shfl2(soli, 0);
#else
		__syncthreads();
		soli = eq->srealcont.nsols;
#endif
		if (soli < MAXREALSOLS)
		{
			if ( lane == 0)
				*(uint2*)(&eq->srealcont.sols[soli][0]) = *(uint2*)(&eq->edata.nonce[0]);

			u32 pos = 4 + (lane << 2); // (0-31) * 4
			*(uint4*)(&eq->srealcont.sols[soli][pos]) = *(uint4*)(&ind[ 0]);
		}        
	}
}

//std::mutex dev_init;
int dev_init_done[MAX_GPUS] = { 0 };

__host__
static int compu32(const void *pa, const void *pb)
{
	uint32_t a = *(uint32_t *)pa, b = *(uint32_t *)pb;
	return a<b ? -1 : a == b ? 0 : +1;
}

__host__
static bool duped(uint32_t* prf)
{
	uint32_t sortprf[128];
	memcpy(sortprf, prf, sizeof(uint32_t) * 128);
	qsort(sortprf, 128, sizeof(uint32_t), &compu32);
	for (uint32_t i = 1; i<128; i++) {
		if (sortprf[i] <= sortprf[i - 1])
        {
			return true;
        }
	}
	return false;
}

__host__
static void sort_pair(uint32_t *a, uint32_t len)
{
	uint32_t *b = a + len;
	uint32_t  tmp, need_sorting = 0;
	for (uint32_t i = 0; i < len; i++) {
		if (need_sorting || a[i] > b[i])
		{
			need_sorting = 1;
			tmp = a[i];
			a[i] = b[i];
			b[i] = tmp;
		}
		else if (a[i] < b[i])
			return;
	}
}

__host__
static void setheader(blake2b_state *ctx, const char *header, const u32 headerLen, const char* nonce, const u32 nonceLen)
{
	uint32_t le_N = WN;
	uint32_t le_K = WK;
	uchar personal[] = "FABcoin_01230123";
		    
	memcpy(personal + 8, &le_N, 4);
	memcpy(personal + 12, &le_K, 4);
	blake2b_param P[1];
	P->digest_length = HASHOUT;
	P->key_length = 0;
	P->fanout = 1;
	P->depth = 1;
	P->leaf_length = 0;
	P->node_offset = 0;
	P->node_depth = 0;
	P->inner_length = 0;
	memset(P->reserved, 0, sizeof(P->reserved));
	memset(P->salt, 0, sizeof(P->salt));
	memcpy(P->personal, (const uint8_t *)personal, 16);
	eq_blake2b_init_param(ctx, P);
	eq_blake2b_update(ctx, (const uchar *)header, headerLen);
	if (nonceLen) eq_blake2b_update(ctx, (const uchar *)nonce, nonceLen);
}

#ifdef _WIN32
dec_cuDeviceGet _cuDeviceGet = nullptr;
dec_cuCtxCreate _cuCtxCreate = nullptr;
dec_cuCtxPushCurrent _cuCtxPushCurrent = nullptr;
dec_cuCtxDestroy _cuCtxDestroy = nullptr;
#endif

template <u32 RB, u32 SM, u32 SSM, u32 THREADS, typename PACKER>
__host__ eq_cuda_context<RB, SM, SSM, THREADS, PACKER>::eq_cuda_context(int thr_id, int dev_id, fn_validate validate, fn_cancel cancel)
{   
	thread_id = thr_id;
	device_id = dev_id;
	solutions = nullptr;
	equi_mem_sz = 0;
	throughput = NBLOCKS;
	totalblocks = NBLOCKS/FD_THREADS;
	threadsperblock = FD_THREADS;
	threadsperblock_digits = THREADS;

    m_fnValidate = validate;
    m_fnCancel = cancel;

    m_buf = NULL;

	if (!dev_init_done[device_id])
	{
		// only first thread shall init device
		checkCudaErrors(cudaSetDevice(device_id));
		checkCudaErrors(cudaDeviceReset());
		checkCudaErrors(cudaSetDeviceFlags(cudaDeviceScheduleBlockingSync));

		pctx = nullptr;
	}
	else
	{
		// create new context
		CUdevice dev;

#ifdef _WIN32
		if (_cuDeviceGet == nullptr)
		{
			HMODULE hmod = LoadLibraryA("nvcuda.dll");
			if (hmod == NULL)
				throw std::runtime_error("Failed to load nvcuda.dll");
			_cuDeviceGet = (dec_cuDeviceGet)GetProcAddress(hmod, "cuDeviceGet");
			if (_cuDeviceGet == nullptr)
				throw std::runtime_error("Failed to get cuDeviceGet address");
			_cuCtxCreate = (dec_cuCtxCreate)GetProcAddress(hmod, "cuCtxCreate_v2");
			if (_cuCtxCreate == nullptr)
				throw std::runtime_error("Failed to get cuCtxCreate address");
			_cuCtxPushCurrent = (dec_cuCtxPushCurrent)GetProcAddress(hmod, "cuCtxPushCurrent_v2");
			if (_cuCtxPushCurrent == nullptr)
				throw std::runtime_error("Failed to get cuCtxPushCurrent address");
			_cuCtxDestroy = (dec_cuCtxDestroy)GetProcAddress(hmod, "cuCtxDestroy_v2");
			if (_cuCtxDestroy == nullptr)
				throw std::runtime_error("Failed to get cuCtxDestroy address");
		}

		checkCudaDriverErrors(_cuDeviceGet(&dev, device_id));
		checkCudaDriverErrors(_cuCtxCreate(&pctx, CU_CTX_SCHED_BLOCKING_SYNC, dev));
		checkCudaDriverErrors(_cuCtxPushCurrent(pctx));
#else
		checkCudaDriverErrors(cuDeviceGet(&dev, device_id));
		checkCudaDriverErrors(cuCtxCreate(&pctx, CU_CTX_SCHED_BLOCKING_SYNC, dev));
		checkCudaDriverErrors(cuCtxPushCurrent(pctx));
#endif
	}
	++dev_init_done[device_id];
	//dev_init.unlock();

	checkCudaErrors(cudaMallocHost((void**)&eq, sizeof(equi<RB, SM>)));

	equi_mem_sz += sizeof(equi<RB, SM>);
	checkCudaErrors(cudaMalloc((void**)&device_eq, sizeof(equi<RB, SM>)));

	equi_mem_sz += sizeof(bucket)*NBUCKETS;
	checkCudaErrors(cudaMalloc((void**)&eq->round0trees, sizeof(bucket)*NBUCKETS));

	equi_mem_sz += sizeof(bucket)*NBUCKETS;
	checkCudaErrors(cudaMalloc((void**)&eq->round1trees, sizeof(bucket)*NBUCKETS));

	equi_mem_sz += sizeof(bucket)*NBUCKETS;
	checkCudaErrors(cudaMalloc((void**)&eq->round2trees, sizeof(bucket)*NBUCKETS));

	equi_mem_sz += sizeof(bucket)*NBUCKETS;
	checkCudaErrors(cudaMalloc((void**)&eq->round3trees, sizeof(bucket)*NBUCKETS));

	equi_mem_sz += sizeof(bucketsmall)*NBUCKETS;
	checkCudaErrors(cudaMalloc((void**)&eq->round4trees, sizeof(bucketsmall)*NBUCKETS));

	equi_mem_sz += sizeof(bucketsmall)*NBUCKETS;
	checkCudaErrors(cudaMalloc((void**)&eq->round5trees, sizeof(bucketsmall)*NBUCKETS));

	equi_mem_sz += sizeof(buckettiny)*NBUCKETS;
	checkCudaErrors(cudaMalloc((void**)&eq->round6trees, sizeof(buckettiny)*NBUCKETS));

	equi_mem_sz += 128;
	checkCudaErrors(cudaMalloc((void**)&m_buf, 128));

	checkCudaErrors(cudaMallocHost((void**)&solutions, sizeof(scontainerreal)));
}

std::vector<unsigned char> GetMinimalFromIndices(std::vector<uint32_t> indices,
                                                 size_t cBitLen);

template <u32 RB, u32 SM, u32 SSM, u32 THREADS, typename PACKER>
__host__ bool eq_cuda_context<RB, SM, SSM, THREADS, PACKER>::solve(
	unsigned char *pblock,
	unsigned char *header,
	unsigned int headerlen)
{
	blake2b_state blake_ctx;                  
	int blocks = NBUCKETS;
    uchar *ptrnonce = header+headerlen-32;

	checkCudaErrors(cudaMemset(device_eq, 0, sizeof(equi<RB, SM>)));
	checkCudaErrors(cudaMemcpy(device_eq, eq, sizeof(equi<RB, SM>), cudaMemcpyHostToDevice));

	setheader(&blake_ctx, (const char *)header, headerlen - 32, (const char *)ptrnonce, 32);
    checkCudaErrors(cudaMemcpy(m_buf, blake_ctx.buf, blake_ctx.buflen, cudaMemcpyHostToDevice));
    checkCudaErrors(cudaMemcpy(&device_eq->blake_h, &blake_ctx.h, sizeof(u64) * 8, cudaMemcpyHostToDevice));      
	{
		checkCudaErrors(cudaMemset(&device_eq->edata, 0, sizeof(device_eq->edata)));		

	    digit_first<RB, SM, PACKER> <<<NBLOCKS / FD_THREADS, FD_THREADS >>>(device_eq, m_buf, *(u32 *)(ptrnonce+12));
	    if (m_fnCancel()) return false;

        digit_1<RB, SM, SSM, PACKER, NRESTS<<6, 64> <<<blocks, 64>>>(device_eq);
	    if (m_fnCancel()) return false;
		
		digit_2<RB, SM, SSM, PACKER, NRESTS << 6, 64> << <blocks, 64 >> >(device_eq);
	    if (m_fnCancel()) return false;

        digit_3<RB, SM, SSM, PACKER, NRESTS<<6, 64> <<<blocks, 64 >>>(device_eq);
	    if (m_fnCancel()) return false;

        digit_4<RB, SM, SSM, PACKER, NRESTS<<6, 64> <<<blocks, 64 >>>(device_eq);
	    if (m_fnCancel()) return false;

        digit_5<RB, SM, SSM, PACKER, NRESTS<<6, 64 > <<<blocks, 64 >>>(device_eq);
	    if (m_fnCancel()) return false;

        digit_6<RB, SM, SSM - 1, PACKER, NRESTS<<6 > <<<blocks, NRESTS >>>(device_eq);
	    if (m_fnCancel()) return false;

		digit_last_wdc<RB, SM, SSM - 3, 2, PACKER, 512, 5, 4> << <blocks, 32 >> >(device_eq);
		*(u32 *)(ptrnonce+12) = (*(u32 *)(ptrnonce+12)) + 1;
	}
    checkCudaErrors(cudaMemcpy(solutions, &device_eq->srealcont, (MAXREALSOLS * (132 * 4)) + 4, cudaMemcpyDeviceToHost));    	

    for (u32 s = 0; (s < solutions->nsols) && (s < MAXREALSOLS); s++)
	{
		// remove dups on CPU (dup removal on GPU is not fully exact and can pass on some invalid solutions)
		if (duped(solutions->sols[s] + 4))
		{
//			printf("duplicated\n");
			continue;
		}

		// perform sort of pairs
		for (uint32_t level = 0; level < 7; level++)
			for (uint32_t i = 0; i < (1 << 7); i += (2 << level))
				sort_pair(&solutions->sols[s][i+4], 1 << level);

        std::vector<uint32_t> index_vector(PROOFSIZE+4);		
        for (u32 i = 0; i < PROOFSIZE+4; i++) 
        {
			index_vector[i] = solutions->sols[s][i];
		}
		std::vector<unsigned char> sol_char = GetMinimalFromIndices(std::vector<uint32_t>(index_vector.begin()+4,index_vector.end()), DIGITBITS);
		if (m_fnValidate(sol_char, pblock, thread_id)) 
        {
             // If we find a POW solution, do not try other solutions
             // because they become invalid as we created a new block in blockchain.
             return true;
        }
	}
    return false;
}

// destructor
template <u32 RB, u32 SM, u32 SSM, u32 THREADS, typename PACKER>
__host__
void eq_cuda_context<RB, SM, SSM, THREADS, PACKER>::freemem()
{
	if (device_eq) {
		cudaFree(device_eq);
		device_eq = NULL;
	}

    if (m_buf) {
        cudaFree(m_buf);
        m_buf = NULL;
    }

	checkCudaErrors(cudaFree(eq->round0trees));
	checkCudaErrors(cudaFree(eq->round1trees));
	checkCudaErrors(cudaFree(eq->round2trees));
	checkCudaErrors(cudaFree(eq->round3trees));
	checkCudaErrors(cudaFree(eq->round4trees));
	checkCudaErrors(cudaFree(eq->round5trees));
	checkCudaErrors(cudaFree(eq->round6trees));

	checkCudaErrors(cudaFreeHost(solutions));
	checkCudaErrors(cudaFreeHost(eq));

    if (pctx) {
		// non primary thread, destroy context
#ifdef _WIN32
		checkCudaDriverErrors(_cuCtxDestroy(pctx));
#else
		checkCudaDriverErrors(cuCtxDestroy(pctx));
#endif
	} else {
		checkCudaErrors(cudaDeviceReset());
		dev_init_done[device_id] = 0;
	}
}

template <u32 RB, u32 SM, u32 SSM, u32 THREADS, typename PACKER>
__host__
eq_cuda_context<RB, SM, SSM, THREADS, PACKER>::~eq_cuda_context()
{
	freemem();
}

#ifdef CONFIG_MODE_184x7
template class eq_cuda_context<CONFIG_MODE_184x7>;
#endif
