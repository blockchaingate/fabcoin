// Equihash CUDA solver
// Copyright (c) 2016 John Tromp

#define htole32(x) (x)
#define HAVE_DECL_HTOLE32 1

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <functional>
#include <vector>
#include <iostream>

#include "eqcuda.hpp"
#include "blake2b.cu"

#define WN	210
#define WK	9

#if WN == 200
#define XINTREE
#define UNROLL
#define RESTBITS	4 
#endif


#if WN == 210
#define RESTBITS	7
#endif

#define NDIGITS		(WK+1)
#define DIGITBITS	(WN/(NDIGITS))
#define BASE (1<<DIGITBITS)
#define NHASHES (2*BASE)
#define HASHESPERBLAKE (512/WN)
#define HASHLEN (WN+7)/8
#define HASHOUT (HASHESPERBLAKE*(HASHLEN))

// 2_log of number of buckets
#define BUCKBITS (DIGITBITS-RESTBITS)

#ifndef SAVEMEM
#if RESTBITS < 8
// can't save memory in such small buckets
#define SAVEMEM 1
#elif RESTBITS >= 8
// take advantage of law of large numbers (sum of 2^8 random numbers)
// this reduces (200,9) memory to under 144MB, with negligible discarding
#define SAVEMEM 9/14
#endif
#endif

// number of buckets
static const u32 NBUCKETS = 1 << BUCKBITS;
// bucket mask
static const u32 BUCKMASK = NBUCKETS - 1;
// 2_log of number of slots per bucket
static const u32 SLOTBITS = RESTBITS + 1 + 1;
static const u32 SLOTRANGE = 1 << SLOTBITS;
// number of slots per bucket
static const u32 NSLOTS = SLOTRANGE * SAVEMEM;
// SLOTBITS mask
static const u32 SLOTMASK = SLOTRANGE - 1;
// number of possible values of xhash (rest of n) bits
static const u32 NRESTS = 1 << RESTBITS;
// RESTBITS mask
static const u32 RESTMASK = NRESTS - 1;
// number of blocks of hashes extracted from single 512 bit blake2b output
static const u32 NBLOCKS = (NHASHES + HASHESPERBLAKE - 1) / HASHESPERBLAKE;
// nothing larger found in 100000 runs
static const u32 MAXSOLS = 10;











void setheader(blake2b_state *ctx, const unsigned char *header, const u32 headerLen, const unsigned char* nce, const u32 nonceLen) 
{
  uint32_t le_N = WN;
  uint32_t le_K = WK;
  uchar personal[] = "ZcashPoW01230123";
  memcpy(personal+8,  &le_N, 4);
  memcpy(personal+12, &le_K, 4);
  blake2b_param P[1];
  P->digest_length = HASHOUT;
  P->key_length    = 0;
  P->fanout        = 1;
  P->depth         = 1;
  P->leaf_length   = 0;
  P->node_offset   = 0;
  P->node_depth    = 0;
  P->inner_length  = 0;
  memset(P->reserved, 0, sizeof(P->reserved));
  memset(P->salt,     0, sizeof(P->salt));
  memcpy(P->personal, (const uint8_t *)personal, 16);
  eq_blake2b_init_param(ctx, P);
  eq_blake2b_update(ctx, (const uchar *)header, headerLen);
  eq_blake2b_update(ctx, (const uchar *)nce, nonceLen);
}




// tree node identifying its children as two different slots in
// a bucket on previous layer with the same rest bits (x-tra hash)
struct tree {
	u32 bid_s0_s1_x; // manual bitfields

	__device__ tree(const u32 idx, const u32 xh) {
		bid_s0_s1_x = idx << RESTBITS | xh;
	}
	__device__ tree(const u32 idx) {
		bid_s0_s1_x = idx;
	}

#ifdef XINTREE
  	__device__ tree(const u32 bid, const u32 s0, const u32 s1, const u32 xh) {
  	bid_s0_s1_x = ((((bid << SLOTBITS) | s0) << SLOTBITS) | s1) << RESTBITS | xh;
#else
  	__device__ tree(const u32 bid, const u32 s0, const u32 s1) {
  	bid_s0_s1_x = (((bid << SLOTBITS) | s0) << SLOTBITS) | s1;
#endif
	}
/*
	__device__ tree(const u32 bid, const u32 s0, const u32 s1, const u32 xh) {
#ifdef XINTREE
		bid_s0_s1_x = ((((bid << SLOTBITS) | s0) << SLOTBITS) | s1) << RESTBITS | xh;
#else
		bid_s0_s1_x = (((bid << SLOTBITS) | s0) << SLOTBITS) | s1;
#endif
	}
	*/

	__device__ u32 getindex() const {
#ifdef XINTREE
		return bid_s0_s1_x >> RESTBITS;
#else
		return bid_s0_s1_x;
#endif
	}
	__device__ u32 bucketid() const {
#ifdef XINTREE
		return bid_s0_s1_x >> (2 * SLOTBITS + RESTBITS);
#else
		return bid_s0_s1_x >> (2 * SLOTBITS);
#endif
	}
	__device__ u32 slotid0() const {
#ifdef XINTREE
		return (bid_s0_s1_x >> SLOTBITS + RESTBITS) & SLOTMASK;
#else
		return (bid_s0_s1_x >> SLOTBITS) & SLOTMASK;
#endif
	}
	__device__ u32 slotid1() const {
#ifdef XINTREE
		return (bid_s0_s1_x >> RESTBITS) & SLOTMASK;
#else
		return bid_s0_s1_x & SLOTMASK;
#endif
	}
	__device__ u32 xhash() const {
		return bid_s0_s1_x & RESTMASK;
	}
};

union hashunit {
	u32 word;
	uchar bytes[sizeof(u32)];
};

#define WORDS(bits)     ((bits + 31) / 32)
#define HASHWORDS0 WORDS(WN - DIGITBITS + RESTBITS)
#define HASHWORDS1 WORDS(WN - 2*DIGITBITS + RESTBITS)

struct slot0 {
	tree attr;
	hashunit hash[HASHWORDS0];
};

struct slot1 {
	tree attr;
	hashunit hash[HASHWORDS1]; 
};

// a bucket is NSLOTS treenodes
typedef slot0 bucket0[NSLOTS];
typedef slot1 bucket1[NSLOTS];
// the N-bit hash consists of K+1 n-bit "digits"
// each of which corresponds to a layer of NBUCKETS buckets
typedef bucket0 digit0[NBUCKETS];
typedef bucket1 digit1[NBUCKETS];

// size (in bytes) of hash in round 0 <= r < WK
u32 hhashsize(const u32 r) {
#ifdef XINTREE
	const u32 hashbits = WN - (r + 1) * DIGITBITS;
#else
	u32 hashbits; 
	if(WN == 210 && WK == 9){
		//Refer to the AION mining wiki for explanation of these hash size values
		switch(r){
			case 0:
				return 26;
			case 1:
				return 23;
			case 2:
				return 20;
			case 3:
				return 18;
			case 4:
				return 15;
			case 5:
				return 13;
			case 6:
				return 10;
			case 7:
				return 7;
			case 8:
				return 5;
			case 9:
				return 0;
			default:
				return 0;
		}
	}else{
		hashbits = 	WN - (r + 1) * DIGITBITS + RESTBITS;
	}
#endif
	return (hashbits + 7) / 8;
}
// size (in bytes) of hash in round 0 <= r < WK
__device__ u32 hashsize(const u32 r) {
#ifdef XINTREE
	const u32 hashbits = WN - (r + 1) * DIGITBITS;
#else
	u32 hashbits; 
	if(WN == 210 && WK == 9){
		//Refer to the AION mining wiki for explanation of these hash size values
		switch(r){
			case 0:
				return 26;
			case 1:
				return 23;
			case 2:
				return 20;
			case 3:
				return 18;
			case 4:
				return 15;
			case 5:
				return 13;
			case 6:
				return 10;
			case 7:
				return 7;
			case 8:
				return 5;
			case 9:
				return 0;
			default:
				return 0;
		}
	}else{
		hashbits = WN - (r + 1) * DIGITBITS + RESTBITS;
	}
#endif
	return (hashbits + 7) / 8;
}

u32 hhashwords(u32 bytes) {
	return (bytes + 3) / 4;
}

__device__ u32 hashwords(u32 bytes) {
	return (bytes + 3) / 4;
}

// manages hash and tree data
struct htalloc {
	bucket0 *trees0[(WK + 1) / 2];
	bucket1 *trees1[WK / 2];
};

typedef u32 bsizes[NBUCKETS];

struct equi210_9 {
	blake2b_state blake_ctx;
	htalloc hta;
	bsizes *nslots;
	proof *sols;
	u32 nsols;
	u32 nthreads;
	equi210_9(const u32 n_threads) {
		nthreads = n_threads;
	}
	void setheadernonce(unsigned char *header, const u32 len, unsigned char* nonce, const u32 nlen) {
		setheader(&blake_ctx, header, len, nonce, nlen);
		checkCudaErrors(cudaMemset(nslots, 0, NBUCKETS * sizeof(u32)));
		nsols = 0;
	}
	__device__ u32 getnslots0(const u32 bid) {
		u32 &nslot = nslots[0][bid];
		const u32 n = min(nslot, NSLOTS);
		nslot = 0;
		return n;
	}
	__device__ u32 getnslots1(const u32 bid) {
		u32 &nslot = nslots[1][bid];
		const u32 n = min(nslot, NSLOTS);
		nslot = 0;
		return n;
	}
	__device__ void orderindices(u32 *indices, u32 size) {
		if (indices[0] > indices[size]) {
			for (u32 i = 0; i < size; i++) {
				const u32 tmp = indices[i];
				indices[i] = indices[size + i];
				indices[size + i] = tmp;
			}
		}
	}
	__device__ void listindices1(const tree t, u32 *indices) {
		const bucket0 &buck = hta.trees0[0][t.bucketid()];
		const u32 size = 1 << 0;
		indices[0] = buck[t.slotid0()].attr.getindex();
		indices[size] = buck[t.slotid1()].attr.getindex();
		orderindices(indices, size);
	}
	__device__ void listindices2(const tree t, u32 *indices) {
		const bucket1 &buck = hta.trees1[0][t.bucketid()];
		const u32 size = 1 << 1;
		listindices1(buck[t.slotid0()].attr, indices);
		listindices1(buck[t.slotid1()].attr, indices + size);
		orderindices(indices, size);
	}
	__device__ void listindices3(const tree t, u32 *indices) {
		const bucket0 &buck = hta.trees0[1][t.bucketid()];
		const u32 size = 1 << 2;
		listindices2(buck[t.slotid0()].attr, indices);
		listindices2(buck[t.slotid1()].attr, indices + size);
		orderindices(indices, size);
	}
	__device__ void listindices4(const tree t, u32 *indices) {
		const bucket1 &buck = hta.trees1[1][t.bucketid()];
		const u32 size = 1 << 3;
		listindices3(buck[t.slotid0()].attr, indices);
		listindices3(buck[t.slotid1()].attr, indices + size);
		orderindices(indices, size);
	}
	__device__ void listindices5(const tree t, u32 *indices) {
		const bucket0 &buck = hta.trees0[2][t.bucketid()];
		const u32 size = 1 << 4;
		listindices4(buck[t.slotid0()].attr, indices);
		listindices4(buck[t.slotid1()].attr, indices+size);
		orderindices(indices, size);
	}
	__device__ void listindices6(const tree t, u32 *indices) {
		const bucket1 &buck = hta.trees1[2][t.bucketid()];
		const u32 size = 1 << 5;
		listindices5(buck[t.slotid0()].attr, indices);
		listindices5(buck[t.slotid1()].attr, indices+size);
		orderindices(indices, size);
	}
	__device__ void listindices7(const tree t, u32 *indices) {
		const bucket0 &buck = hta.trees0[3][t.bucketid()];
		const u32 size = 1 << 6;
		listindices6(buck[t.slotid0()].attr, indices);
		listindices6(buck[t.slotid1()].attr, indices+size);
		orderindices(indices, size);
	}
	__device__ void listindices8(const tree t, u32 *indices) {
		const bucket1 &buck = hta.trees1[3][t.bucketid()];
		const u32 size = 1 << 7;
		listindices7(buck[t.slotid0()].attr, indices);
		listindices7(buck[t.slotid1()].attr, indices+size);
		orderindices(indices, size);
	}
	__device__ void listindices9(const tree t, u32 *indices) {
		const bucket0 &buck = hta.trees0[4][t.bucketid()];
		const u32 size = 1 << 8;
		listindices8(buck[t.slotid0()].attr, indices);
		listindices8(buck[t.slotid1()].attr, indices+size);
		orderindices(indices, size);
	}
	__device__ void candidate(const tree t) {
		proof prf;
#if WK==9
		listindices9(t, prf);
#elif WK==5
		listindices5(t, prf);
#else
#error not implemented
#endif
		if (probdupe(prf))
			return;
		u32 soli = atomicAdd(&nsols, 1);
		if (soli < MAXSOLS)
#if WK==9
			listindices9(t, sols[soli]);
#elif WK==5
			listindices5(t, sols[soli]);
#else
#error not implemented
#endif
	}
	void showbsizes(u32 r) {
#if defined(HIST) || defined(SPARK) || defined(LOGSPARK)
		u32 ns[NBUCKETS];
		checkCudaErrors(cudaMemcpy(ns, nslots[r & 1], NBUCKETS * sizeof(u32), cudaMemcpyDeviceToHost));
		u32 binsizes[65];
		memset(binsizes, 0, 65 * sizeof(u32));
		for (u32 bucketid = 0; bucketid < NBUCKETS; bucketid++) {
			u32 bsize = min(ns[bucketid], NSLOTS) >> (SLOTBITS - 6);
			binsizes[bsize]++;
		}
		for (u32 i = 0; i < 65; i++) {
#ifdef HIST
			printf(" %d:%d", i, binsizes[i]);
#else
#ifdef SPARK
			u32 sparks = binsizes[i] / SPARKSCALE;
#else
			u32 sparks = 0;
			for (u32 bs = binsizes[i]; bs; bs >>= 1) sparks++;
			sparks = sparks * 7 / SPARKSCALE;
#endif
			printf("\342\226%c", '\201' + sparks);
#endif
		}
		printf("\n");
#endif
		}
	// proper dupe test is a little costly on GPU, so allow false negatives
	__device__ bool probdupe(u32 *prf) {
		unsigned short susp[PROOFSIZE1];
		memset(susp, 0xffff, PROOFSIZE1 * sizeof(unsigned short));
		for (u32 i=0; i<PROOFSIZE1; i++) {
			u32 bin = prf[i] & (PROOFSIZE1-1);
			unsigned short msb = prf[i]>>WK;
			if (msb == susp[bin])
				return true;
			susp[bin] = msb;
		}
		return false;
	}
	struct htlayout {
		htalloc hta;
		u32 prevhashunits;
		u32 nexthashunits;
		u32 dunits;
		u32 prevbo;
		u32 nextbo;

		__device__ htlayout(equi210_9 *eq, u32 r) : hta(eq->hta), prevhashunits(0), dunits(0) {
			u32 nexthashbytes = hashsize(r);
			nexthashunits = hashwords(nexthashbytes);
			prevbo = 0;
			nextbo = nexthashunits * sizeof(hashunit) - nexthashbytes; // 0-3
			if (r) {
				u32 prevhashbytes = hashsize(r-1);
				prevhashunits = hashwords(prevhashbytes);
				prevbo = prevhashunits * sizeof(hashunit) - prevhashbytes; // 0-3
				dunits = prevhashunits - nexthashunits;
			}
		}
		__device__ u32 getxhash0(const slot0* pslot) const {
#ifdef XINTREE
			return pslot->attr.xhash();
#elif WN == 200 && RESTBITS == 4
			return pslot->hash->bytes[prevbo] >> 4;
#elif WN == 200 && RESTBITS == 8
			return (pslot->hash->bytes[prevbo] & 0xf) << 4 | pslot->hash->bytes[prevbo + 1] >> 4;
#elif WN == 144 && RESTBITS == 4
			return pslot->hash->bytes[prevbo] & 0xf;
#elif WN == 200 && RESTBITS == 6
			return (pslot->hash->bytes[prevbo] & 0x3) << 4 | pslot->hash->bytes[prevbo+1] >> 4;
#elif WN == 210 && RESTBITS == 7
			//Maintained for backwards compatibiilty
			return 0;
#else
#error non implemented
#endif
		}
		__device__ u32 getxhash1(const slot1* pslot) const {
#ifdef XINTREE
			return pslot->attr.xhash();
#elif WN == 200 && RESTBITS == 4
			return pslot->hash->bytes[prevbo] & 0xf;
#elif WN == 200 && RESTBITS == 8
			return pslot->hash->bytes[prevbo];
#elif WN == 144 && RESTBITS == 4
			return pslot->hash->bytes[prevbo] & 0xf;
#elif WN == 200 && RESTBITS == 6
			return pslot->hash->bytes[prevbo] & 0x3f;
#elif WN == 210 && RESTBITS == 7
			//Maintained for backwards compatibiilty
			return 0;
#else
#error non implemented
#endif
		}
		__device__ bool equal(const hashunit *hash0, const hashunit *hash1) const {
			return hash0[prevhashunits - 1].word == hash1[prevhashunits - 1].word;
		}

		__device__ bool equal_210(const slot0 *pslot0, const slot0 *pslot1) const {
			//Check last 21 bits for collision
			return ((pslot0->hash->bytes[prevbo + 1] & 0x7) == (pslot1->hash->bytes[prevbo + 1] & 0x7) &&
				    (pslot0->hash->bytes[prevbo + 2]) == (pslot1->hash->bytes[prevbo + 2]) &&
				    (pslot0->hash->bytes[prevbo + 3]) == (pslot1->hash->bytes[prevbo + 3]) &&
				    (pslot0->hash->bytes[prevbo + 4] >> 6) == (pslot1->hash->bytes[prevbo + 4] >> 6));
		}
	};

	struct collisiondata {
#ifdef XBITMAP
#if NSLOTS > 64
#error cant use XBITMAP with more than 64 slots
#endif
		u64 xhashmap[NRESTS];
		u64 xmap;
#else
#if RESTBITS <= 6
		typedef uchar xslot;
#else
		typedef u16 xslot;
#endif
		static const xslot xnil = ~0;
		xslot xhashslots[NRESTS];
		xslot nextxhashslot[NSLOTS];
		xslot nextslot;
#endif
		u32 s0;

		__device__ void clear() {
#ifdef XBITMAP
			memset(xhashmap, 0, NRESTS * sizeof(u64));
#else
			memset(xhashslots, xnil, NRESTS * sizeof(xslot));
			memset(nextxhashslot, xnil, NSLOTS * sizeof(xslot));
#endif
		}
		__device__ bool addslot(u32 s1, u32 xh) {
#ifdef XBITMAP
			xmap = xhashmap[xh];
			xhashmap[xh] |= (u64)1 << s1;
			s0 = ~0;
			return true;
#else
			nextslot = xhashslots[xh];
			nextxhashslot[s1] = nextslot;
			xhashslots[xh] = s1;
			return true;
#endif
		}
		__device__ bool nextcollision() const {
#ifdef XBITMAP
			return xmap != 0;
#else
			return nextslot != xnil;
#endif
		}
		__device__ u32 slot() {
#ifdef XBITMAP
			const u32 ffs = __ffsll(xmap);
			s0 += ffs; xmap >>= ffs;
#else
			nextslot = nextxhashslot[s0 = nextslot];
#endif
			return s0;
		}
	};
		};

__global__ void digitH(equi210_9 *eq) {
	uchar hash[HASHOUT];
	blake2b_state state;
	equi210_9::htlayout htl(eq, 0);
	const u32 hashbytes = hashsize(0); // always 23 ?
	const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
	for (u32 block = id; block < NBLOCKS; block += eq->nthreads) {
		state = eq->blake_ctx;
		blake2b_gpu_hash(&state, block, hash, HASHOUT);
		for (u32 i = 0; i<HASHESPERBLAKE; i++) {
			const uchar *ph = hash + i * HASHLEN;
#if BUCKBITS == 16 && RESTBITS == 4
			const u32 bucketid = ((u32)ph[0] << 8) | ph[1];
#ifdef XINTREE
			const u32 xhash = ph[2] >> 4;
#endif
#elif BUCKBITS == 14 && RESTBITS == 6
			const u32 bucketid = ((u32)ph[0] << 6) | ph[1] >> 2;
#elif BUCKBITS == 12 && RESTBITS == 8
			const u32 bucketid = ((u32)ph[0] << 4) | ph[1] >> 4;
#elif BUCKBITS == 20 && RESTBITS == 4
			const u32 bucketid = ((((u32)ph[0] << 8) | ph[1]) << 4) | ph[2] >> 4;
#ifdef XINTREE
			const u32 xhash = ph[2] & 0xf;
#endif
#elif BUCKBITS == 12 && RESTBITS == 4
			const u32 bucketid = ((u32)ph[0] << 4) | ph[1] >> 4;
			const u32 xhash = ph[1] & 0xf;
#elif BUCKBITS == 14 && RESTBITS == 7
      		const u32 bucketid = ((u32)ph[0] << (BUCKBITS - 8)) | ph[1] >> (16 - BUCKBITS);
#else
#error not implemented
#endif
			const u32 slot = atomicAdd(&eq->nslots[0][bucketid], 1);
			if (slot >= NSLOTS)
				continue;
			slot0 &s = eq->hta.trees0[0][bucketid][slot];
#ifdef XINTREE
			s.attr = tree(block*HASHESPERBLAKE+i, xhash);
#else
			s.attr = tree(block*HASHESPERBLAKE+i);
#endif
			memcpy(s.hash->bytes+htl.nextbo, ph+HASHLEN-hashbytes, hashbytes);
		}
	}
}

__global__ void digitO(equi210_9 *eq, const u32 r) {
	equi210_9::htlayout htl(eq, r);
	equi210_9::collisiondata cd;
	const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
	for (u32 bucketid = id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
		cd.clear();
		slot0 *buck = htl.hta.trees0[(r - 1) / 2][bucketid];
		u32 bsize = eq->getnslots0(bucketid);
		for (u32 s1 = 0; s1 < bsize; s1++) {
			const slot0 *pslot1 = buck + s1;
			if (!cd.addslot(s1, htl.getxhash0(pslot1)))
				continue;
			for (; cd.nextcollision();) {
				const u32 s0 = cd.slot();
				const slot0 *pslot0 = buck + s0;
				if (htl.equal(pslot0->hash, pslot1->hash))
					continue;
				u32 xorbucketid;
				u32 xhash;
				const uchar *bytes0 = pslot0->hash->bytes, *bytes1 = pslot1->hash->bytes;
#if WN == 200 && BUCKBITS == 16 && RESTBITS == 4 && defined(XINTREE)
				xorbucketid = ((((u32)(bytes0[htl.prevbo] ^ bytes1[htl.prevbo]) & 0xf) << 8)
					| (bytes0[htl.prevbo + 1] ^ bytes1[htl.prevbo + 1])) << 4
					| (xhash = bytes0[htl.prevbo + 2] ^ bytes1[htl.prevbo + 2]) >> 4;
				xhash &= 0xf;
#elif WN == 144 && BUCKBITS == 20 && RESTBITS == 4
				xorbucketid = ((((u32)(bytes0[htl.prevbo + 1] ^ bytes1[htl.prevbo + 1]) << 8)
					| (bytes0[htl.prevbo + 2] ^ bytes1[htl.prevbo + 2])) << 4)
					| (xhash = bytes0[htl.prevbo + 3] ^ bytes1[htl.prevbo + 3]) >> 4;
				xhash &= 0xf;
#elif WN == 96 && BUCKBITS == 12 && RESTBITS == 4
				xorbucketid = ((u32)(bytes0[htl.prevbo + 1] ^ bytes1[htl.prevbo + 1]) << 4)
					| (xhash = bytes0[htl.prevbo + 2] ^ bytes1[htl.prevbo + 2]) >> 4;
				xhash &= 0xf;
#elif WN == 200 && BUCKBITS == 14 && RESTBITS == 6
				xorbucketid = ((((u32)(bytes0[htl.prevbo + 1] ^ bytes1[htl.prevbo + 1]) & 0xf) << 8)
					| (bytes0[htl.prevbo + 2] ^ bytes1[htl.prevbo + 2])) << 2
					| (bytes0[htl.prevbo + 3] ^ bytes1[htl.prevbo + 3]) >> 6;
#elif WN == 210 && BUCKBITS == 14 && RESTBITS == 7
				//Included to maintain backwards compatibility
				xorbucketid = 0;
				xhash = 0;
#else
#error not implemented
#endif
				const u32 xorslot = atomicAdd(&eq->nslots[1][xorbucketid], 1);
				if (xorslot >= NSLOTS)
					continue;
				slot1 &xs = htl.hta.trees1[r/2][xorbucketid][xorslot];
#ifdef XINTREE
				xs.attr = tree(bucketid, s0, s1, xhash);
#else
				xs.attr = tree(bucketid, s0, s1);
#endif
				for (u32 i=htl.dunits; i < htl.prevhashunits; i++)
					xs.hash[i - htl.dunits].word = pslot0->hash[i].word ^ pslot1->hash[i].word;
			}
		}
	}
}

__global__ void digitE(equi210_9 *eq, const u32 r) {
	equi210_9::htlayout htl(eq, r);
	equi210_9::collisiondata cd;
	const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
	for (u32 bucketid = id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
		cd.clear();
		slot1 *buck = htl.hta.trees1[(r - 1) / 2][bucketid];
		u32 bsize = eq->getnslots1(bucketid);
		for (u32 s1 = 0; s1 < bsize; s1++) {
			const slot1 *pslot1 = buck + s1;
			if (!cd.addslot(s1, htl.getxhash1(pslot1)))
				continue;
			for (; cd.nextcollision();) {
				const u32 s0 = cd.slot();
				const slot1 *pslot0 = buck + s0;
				if (htl.equal(pslot0->hash, pslot1->hash))
					continue;
				u32 xorbucketid;
				u32 xhash;
				const uchar *bytes0 = pslot0->hash->bytes, *bytes1 = pslot1->hash->bytes;
#if WN == 200 && BUCKBITS == 16 && RESTBITS == 4 && defined(XINTREE)
				xorbucketid = ((u32)(bytes0[htl.prevbo] ^ bytes1[htl.prevbo]) << 8)
					| (bytes0[htl.prevbo + 1] ^ bytes1[htl.prevbo + 1]);
				xhash = (bytes0[htl.prevbo + 2] ^ bytes1[htl.prevbo + 2]) >> 4;
#elif WN == 144 && BUCKBITS == 20 && RESTBITS == 4
				xorbucketid = ((((u32)(bytes0[htl.prevbo + 1] ^ bytes1[htl.prevbo + 1]) << 8)
					| (bytes0[htl.prevbo + 2] ^ bytes1[htl.prevbo + 2])) << 4)
					| (bytes0[htl.prevbo + 3] ^ bytes1[htl.prevbo + 3]) >> 4;
#elif WN == 96 && BUCKBITS == 12 && RESTBITS == 4
				xorbucketid = ((u32)(bytes0[htl.prevbo + 1] ^ bytes1[htl.prevbo + 1]) << 4)
					| (bytes0[htl.prevbo + 2] ^ bytes1[htl.prevbo + 2]) >> 4;
#elif WN == 200 && BUCKBITS == 14 && RESTBITS == 6
				xorbucketid = ((u32)(bytes0[htl.prevbo + 1] ^ bytes1[htl.prevbo + 1]) << 6)
					| (bytes0[htl.prevbo + 2] ^ bytes1[htl.prevbo + 2]) >> 2;
#elif WN == 210 && BUCKBITS == 14 && RESTBITS == 7
				//Included to maintain backwards compatibility
				xorbucketid = 0;
				xhash = 0;
#else
#error not implemented
#endif
				const u32 xorslot = atomicAdd(&eq->nslots[0][xorbucketid], 1);
				if (xorslot >= NSLOTS)
					continue;
				slot0 &xs = htl.hta.trees0[r / 2][xorbucketid][xorslot];
#ifdef XINTREE
				xs.attr = tree(bucketid, s0, s1, xhash);
#else
				xs.attr = tree(bucketid, s0, s1);
#endif
				for (u32 i = htl.dunits; i < htl.prevhashunits; i++)
					xs.hash[i - htl.dunits].word = pslot0->hash[i].word ^ pslot1->hash[i].word;
			}
		}
	}
}

#ifdef UNROLL
__global__ void digit_1(equi210_9 *eq) {
	equi210_9::htlayout htl(eq, 1);
	equi210_9::collisiondata cd;
	const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
	for (u32 bucketid = id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
		cd.clear();
		slot0 *buck = htl.hta.trees0[0][bucketid];
		u32 bsize = eq->getnslots0(bucketid);
		for (u32 s1 = 0; s1 < bsize; s1++) {
			const slot0 *pslot1 = buck + s1;
			if (!cd.addslot(s1, htl.getxhash0(pslot1)))
				continue;
			for (; cd.nextcollision();) {
				const u32 s0 = cd.slot();
				const slot0 *pslot0 = buck + s0;
				if (htl.equal(pslot0->hash, pslot1->hash))
					continue;
				const u32 xor0 = pslot0->hash->word ^ pslot1->hash->word;
				const u32 bexor = __byte_perm(xor0, 0, 0x0123);
				const u32 xorbucketid = bexor >> 4 & BUCKMASK;
				const u32 xhash = bexor & 0xf;
				const u32 xorslot = atomicAdd(&eq->nslots[1][xorbucketid], 1);
				if (xorslot >= NSLOTS)
					continue;
				slot1 &xs = htl.hta.trees1[0][xorbucketid][xorslot];
				xs.attr = tree(bucketid, s0, s1, xhash);
				xs.hash[0].word = pslot0->hash[1].word ^ pslot1->hash[1].word;
				xs.hash[1].word = pslot0->hash[2].word ^ pslot1->hash[2].word;
				xs.hash[2].word = pslot0->hash[3].word ^ pslot1->hash[3].word;
				xs.hash[3].word = pslot0->hash[4].word ^ pslot1->hash[4].word;
				xs.hash[4].word = pslot0->hash[5].word ^ pslot1->hash[5].word;
			}
		}
	}
}
__global__ void digit2(equi210_9 *eq) {
	equi210_9::htlayout htl(eq, 2);
	equi210_9::collisiondata cd;
	const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
	for (u32 bucketid = id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
		cd.clear();
		slot1 *buck = htl.hta.trees1[0][bucketid];
		u32 bsize = eq->getnslots1(bucketid);
		for (u32 s1 = 0; s1 < bsize; s1++) {
			const slot1 *pslot1 = buck + s1;
			if (!cd.addslot(s1, htl.getxhash1(pslot1)))
				continue;
			for (; cd.nextcollision();) {
				const u32 s0 = cd.slot();
				const slot1 *pslot0 = buck + s0;
				if (htl.equal(pslot0->hash, pslot1->hash))
					continue;
				const u32 xor0 = pslot0->hash->word ^ pslot1->hash->word;
				const u32 bexor = __byte_perm(xor0, 0, 0x0123);
				const u32 xorbucketid = bexor >> 16;
				const u32 xhash = bexor >> 12 & 0xf;
				const u32 xorslot = atomicAdd(&eq->nslots[0][xorbucketid], 1);
				if (xorslot >= NSLOTS)
					continue;
				slot0 &xs = htl.hta.trees0[1][xorbucketid][xorslot];
				xs.attr = tree(bucketid, s0, s1, xhash);
				xs.hash[0].word = xor0;
				xs.hash[1].word = pslot0->hash[1].word ^ pslot1->hash[1].word;
				xs.hash[2].word = pslot0->hash[2].word ^ pslot1->hash[2].word;
				xs.hash[3].word = pslot0->hash[3].word ^ pslot1->hash[3].word;
				xs.hash[4].word = pslot0->hash[4].word ^ pslot1->hash[4].word;
			}
		}
	}
}
__global__ void digit3(equi210_9 *eq) {
	equi210_9::htlayout htl(eq, 3);
	equi210_9::collisiondata cd;
	const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
	for (u32 bucketid = id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
		cd.clear();
		slot0 *buck = htl.hta.trees0[1][bucketid];
		u32 bsize = eq->getnslots0(bucketid);
		for (u32 s1 = 0; s1 < bsize; s1++) {
			const slot0 *pslot1 = buck + s1;
			if (!cd.addslot(s1, htl.getxhash0(pslot1)))
				continue;
			for (; cd.nextcollision();) {
				const u32 s0 = cd.slot();
				const slot0 *pslot0 = buck + s0;
				if (htl.equal(pslot0->hash, pslot1->hash))
					continue;
				const u32 xor0 = pslot0->hash->word ^ pslot1->hash->word;
				const u32 xor1 = pslot0->hash[1].word ^ pslot1->hash[1].word;
				const u32 bexor = __byte_perm(xor0, xor1, 0x1234);
				const u32 xorbucketid = bexor >> 4 & BUCKMASK;
				const u32 xhash = bexor & 0xf;
				const u32 xorslot = atomicAdd(&eq->nslots[1][xorbucketid], 1);
				if (xorslot >= NSLOTS)
					continue;
				slot1 &xs = htl.hta.trees1[1][xorbucketid][xorslot];
				xs.attr = tree(bucketid, s0, s1, xhash);
				xs.hash[0].word = xor1;
				xs.hash[1].word = pslot0->hash[2].word ^ pslot1->hash[2].word;
				xs.hash[2].word = pslot0->hash[3].word ^ pslot1->hash[3].word;
				xs.hash[3].word = pslot0->hash[4].word ^ pslot1->hash[4].word;
			}
		}
	}
}
__global__ void digit4(equi210_9 *eq) {
	equi210_9::htlayout htl(eq, 4);
	equi210_9::collisiondata cd;
	const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
	for (u32 bucketid = id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
		cd.clear();
		slot1 *buck = htl.hta.trees1[1][bucketid];
		u32 bsize = eq->getnslots1(bucketid);
		for (u32 s1 = 0; s1 < bsize; s1++) {
			const slot1 *pslot1 = buck + s1;
			if (!cd.addslot(s1, htl.getxhash1(pslot1)))
				continue;
			for (; cd.nextcollision();) {
				const u32 s0 = cd.slot();
				const slot1 *pslot0 = buck + s0;
				if (htl.equal(pslot0->hash, pslot1->hash))
					continue;
				const u32 xor0 = pslot0->hash->word ^ pslot1->hash->word;
				const u32 bexor = __byte_perm(xor0, 0, 0x4123);
				const u32 xorbucketid = bexor >> 8;
				const u32 xhash = bexor >> 4 & 0xf;
				const u32 xorslot = atomicAdd(&eq->nslots[0][xorbucketid], 1);
				if (xorslot >= NSLOTS)
					continue;
				slot0 &xs = htl.hta.trees0[2][xorbucketid][xorslot];
				xs.attr = tree(bucketid, s0, s1, xhash);
				xs.hash[0].word = xor0;
				xs.hash[1].word = pslot0->hash[1].word ^ pslot1->hash[1].word;
				xs.hash[2].word = pslot0->hash[2].word ^ pslot1->hash[2].word;
				xs.hash[3].word = pslot0->hash[3].word ^ pslot1->hash[3].word;
			}
		}
	}
}
__global__ void digit5(equi210_9 *eq) {
	equi210_9::htlayout htl(eq, 5);
	equi210_9::collisiondata cd;
	const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
	for (u32 bucketid = id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
		cd.clear();
		slot0 *buck = htl.hta.trees0[2][bucketid];
		u32 bsize = eq->getnslots0(bucketid);
		for (u32 s1 = 0; s1 < bsize; s1++) {
			const slot0 *pslot1 = buck + s1;
			if (!cd.addslot(s1, htl.getxhash0(pslot1)))
				continue;
			for (; cd.nextcollision();) {
				const u32 s0 = cd.slot();
				const slot0 *pslot0 = buck + s0;
				if (htl.equal(pslot0->hash, pslot1->hash))
					continue;
				const u32 xor0 = pslot0->hash->word ^ pslot1->hash->word;
				const u32 xor1 = pslot0->hash[1].word ^ pslot1->hash[1].word;
				const u32 bexor = __byte_perm(xor0, xor1, 0x2345);
				const u32 xorbucketid = bexor >> 4 & BUCKMASK;
				const u32 xhash = bexor & 0xf;
				const u32 xorslot = atomicAdd(&eq->nslots[1][xorbucketid], 1);
				if (xorslot >= NSLOTS)
					continue;
				slot1 &xs = htl.hta.trees1[2][xorbucketid][xorslot];
				xs.attr = tree(bucketid, s0, s1, xhash);
				xs.hash[0].word = xor1;
				xs.hash[1].word = pslot0->hash[2].word ^ pslot1->hash[2].word;
				xs.hash[2].word = pslot0->hash[3].word ^ pslot1->hash[3].word;
			}
		}
	}
}
__global__ void digit6(equi210_9 *eq) {
	equi210_9::htlayout htl(eq, 6);
	equi210_9::collisiondata cd;
	const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
	for (u32 bucketid = id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
		cd.clear();
		slot1 *buck = htl.hta.trees1[2][bucketid];
		u32 bsize = eq->getnslots1(bucketid);
		for (u32 s1 = 0; s1 < bsize; s1++) {
			const slot1 *pslot1 = buck + s1;
			if (!cd.addslot(s1, htl.getxhash1(pslot1)))
				continue;
			for (; cd.nextcollision();) {
				const u32 s0 = cd.slot();
				const slot1 *pslot0 = buck + s0;
				if (htl.equal(pslot0->hash, pslot1->hash))
					continue;
				const u32 xor0 = pslot0->hash->word ^ pslot1->hash->word;
				const u32 xor1 = pslot0->hash[1].word ^ pslot1->hash[1].word;
				const u32 bexor = __byte_perm(xor0, xor1, 0x2345);
				const u32 xorbucketid = bexor >> 16;
				const u32 xhash = bexor >> 12 & 0xf;
				const u32 xorslot = atomicAdd(&eq->nslots[0][xorbucketid], 1);
				if (xorslot >= NSLOTS)
					continue;
				slot0 &xs = htl.hta.trees0[3][xorbucketid][xorslot];
				xs.attr = tree(bucketid, s0, s1, xhash);
				xs.hash[0].word = xor1;
				xs.hash[1].word = pslot0->hash[2].word ^ pslot1->hash[2].word;
			}
		}
	}
}
__global__ void digit7(equi210_9 *eq) { 
	equi210_9::htlayout htl(eq, 7);
	equi210_9::collisiondata cd;
	const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
	for (u32 bucketid = id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
		cd.clear();
		slot0 *buck = htl.hta.trees0[3][bucketid];
		u32 bsize = eq->getnslots0(bucketid);
		for (u32 s1 = 0; s1 < bsize; s1++) {
			const slot0 *pslot1 = buck + s1;
			if (!cd.addslot(s1, htl.getxhash0(pslot1)))
				continue;
			for (; cd.nextcollision();) {
				const u32 s0 = cd.slot();
				const slot0 *pslot0 = buck + s0;
				if (htl.equal(pslot0->hash, pslot1->hash))
					continue;
				const u32 xor0 = pslot0->hash->word ^ pslot1->hash->word;
				const u32 bexor = __byte_perm(xor0, 0, 0x4012);
				const u32 xorbucketid = bexor >> 4 & BUCKMASK;
				const u32 xhash = bexor & 0xf;
				const u32 xorslot = atomicAdd(&eq->nslots[1][xorbucketid], 1);
				if (xorslot >= NSLOTS)
					continue;
				slot1 &xs = htl.hta.trees1[3][xorbucketid][xorslot];
				xs.attr = tree(bucketid, s0, s1, xhash);
				xs.hash[0].word = xor0;
				xs.hash[1].word = pslot0->hash[1].word ^ pslot1->hash[1].word;
			}
		}
	}
}
__global__ void digit8(equi210_9 *eq) {
	equi210_9::htlayout htl(eq, 8);
	equi210_9::collisiondata cd;
	const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
	for (u32 bucketid = id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
		cd.clear();
		slot1 *buck = htl.hta.trees1[3][bucketid];
		u32 bsize = eq->getnslots1(bucketid);
		for (u32 s1 = 0; s1 < bsize; s1++) {
			const slot1 *pslot1 = buck + s1;
			if (!cd.addslot(s1, htl.getxhash1(pslot1)))
				continue;
			for (; cd.nextcollision();) {
				const u32 s0 = cd.slot();
				const slot1 *pslot0 = buck + s0;
				if (htl.equal(pslot0->hash, pslot1->hash))
					continue;
				const u32 xor0 = pslot0->hash->word ^ pslot1->hash->word;
				const u32 xor1 = pslot0->hash[1].word ^ pslot1->hash[1].word;
				const u32 bexor = __byte_perm(xor0, xor1, 0x3456);
				const u32 xorbucketid = bexor >> 16;
				const u32 xhash = bexor >> 12 & 0xf;
				const u32 xorslot = atomicAdd(&eq->nslots[0][xorbucketid], 1);
				if (xorslot >= NSLOTS)
					continue;
				slot0 &xs = htl.hta.trees0[4][xorbucketid][xorslot];
				xs.attr = tree(bucketid, s0, s1, xhash);
				xs.hash[0].word = xor1;
			}
		}
	}
}
#endif


#if WN == 210 && WK == 9
__global__ void digit_1(equi210_9 *eq, const u32 r) {
  equi210_9::htlayout htl(eq, r);
  equi210_9::collisiondata cd;
  const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
  for (u32 bucketid=id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
    cd.clear();
    slot0 *buck = htl.hta.trees0[(r-1)/2][bucketid];
    u32 bsize = eq->getnslots0(bucketid);
    for (u32 s1 = 0; s1 < bsize; s1++) {
      const slot0 *pslot1 = buck + s1;
      u32 xhash0 = (pslot1->hash->bytes[htl.prevbo] & 0x3) << 5 | (pslot1->hash->bytes[htl.prevbo+1] >> 3);
      if(!cd.addslot(s1, xhash0))
	  	continue;
      for (; cd.nextcollision(); ) {
        const u32 s0 = cd.slot();
        const slot0 *pslot0 = buck + s0;
        if (htl.equal(pslot0->hash, pslot1->hash))
        	continue;
        u32 xorbucketid;
        const uchar *bytes0 = pslot0->hash->bytes, *bytes1 = pslot1->hash->bytes;

        xorbucketid = ((((u32)(bytes0[htl.prevbo + 1] ^ bytes1[htl.prevbo + 1]) & 0x7) << 8) | (bytes0[htl.prevbo + 2] ^ bytes1[htl.prevbo + 2])) << 3 | (bytes0[htl.prevbo + 3] ^ bytes1[htl.prevbo + 3]) >> 5;

        const u32 xorslot = atomicAdd(&eq->nslots[1][xorbucketid], 1);
        if (xorslot >= NSLOTS)
          continue;
        slot1 &xs = htl.hta.trees1[r/2][xorbucketid][xorslot];

        xs.attr = tree(bucketid, s0, s1);

        for (u32 i=htl.dunits; i < htl.prevhashunits; i++)
          xs.hash[i-htl.dunits].word = pslot0->hash[i].word ^ pslot1->hash[i].word;
      }
    }
  }
}

__global__ void digit_2(equi210_9 *eq, const u32 r) {
  equi210_9::htlayout htl(eq, r);
  equi210_9::collisiondata cd;
  const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
  for (u32 bucketid=id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
    cd.clear();
    slot1 *buck = htl.hta.trees1[(r-1)/2][bucketid];
    u32 bsize = eq->getnslots1(bucketid);
    for (u32 s1 = 0; s1 < bsize; s1++) {
      const slot1 *pslot1 = buck + s1;
      u32 xhash1 = (pslot1->hash->bytes[htl.prevbo] & 0x1f) << 2 | (pslot1->hash->bytes[htl.prevbo+1] >> 6);
		if(!cd.addslot(s1, xhash1))
	  		continue;
      for (; cd.nextcollision(); ) {
        const u32 s0 = cd.slot();
        const slot1 *pslot0 = buck + s0;
        if (htl.equal(pslot0->hash, pslot1->hash))
        	continue;
        u32 xorbucketid;
        const uchar *bytes0 = pslot0->hash->bytes, *bytes1 = pslot1->hash->bytes;

        xorbucketid = ((((u32)(bytes0[htl.prevbo + 1] ^ bytes1[htl.prevbo + 1]) & 0x3f) << 8) | ((bytes0[htl.prevbo + 2] ^ bytes1[htl.prevbo + 2])));

        const u32 xorslot = atomicAdd(&eq->nslots[0][xorbucketid], 1);
        if (xorslot >= NSLOTS)
          continue;
        slot0 &xs = htl.hta.trees0[r/2][xorbucketid][xorslot];

        xs.attr = tree(bucketid, s0, s1);

        for (u32 i=htl.dunits; i < htl.prevhashunits; i++){
          xs.hash[i-htl.dunits].word = pslot0->hash[i].word ^ pslot1->hash[i].word;
        }
      }
    }
  }
}

__global__ void digit_3(equi210_9 *eq, const u32 r) {
  equi210_9::htlayout htl(eq, r);
  equi210_9::collisiondata cd;
  const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
  for (u32 bucketid=id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
    cd.clear();
    slot0 *buck = htl.hta.trees0[(r-1)/2][bucketid];
    u32 bsize = eq->getnslots0(bucketid);
    for (u32 s1 = 0; s1 < bsize; s1++) {
      const slot0 *pslot1 = buck + s1;
      u32 xhash0 = pslot1->hash->bytes[htl.prevbo] >>1;
      if(!cd.addslot(s1, xhash0))
	  	continue;
      for (; cd.nextcollision(); ) {
        const u32 s0 = cd.slot();
        const slot0 *pslot0 = buck + s0;
        if (htl.equal(pslot0->hash, pslot1->hash))
        	continue;
        u32 xorbucketid;
        const uchar *bytes0 = pslot0->hash->bytes, *bytes1 = pslot1->hash->bytes;

        xorbucketid = ((((u32)(bytes0[htl.prevbo] ^ bytes1[htl.prevbo]) & 0x1) << 8) | (bytes0[htl.prevbo + 1] ^ bytes1[htl.prevbo + 1])) << 5 | ((bytes0[htl.prevbo + 2] ^ bytes1[htl.prevbo + 2]) >> 3);

        const u32 xorslot = atomicAdd(&eq->nslots[1][xorbucketid], 1);
        if (xorslot >= NSLOTS)
          continue;
        slot1 &xs = htl.hta.trees1[r/2][xorbucketid][xorslot];

        xs.attr = tree(bucketid, s0, s1);

        for (u32 i=htl.dunits; i < htl.prevhashunits; i++)
          xs.hash[i-htl.dunits].word = pslot0->hash[i].word ^ pslot1->hash[i].word;
      }
    }
  }
}

__global__ void digit_4(equi210_9 *eq, const u32 r) {
  equi210_9::htlayout htl(eq, r);
  equi210_9::collisiondata cd;
  const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
  for (u32 bucketid=id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
    cd.clear();
    slot1 *buck = htl.hta.trees1[(r-1)/2][bucketid];
    u32 bsize = eq->getnslots1(bucketid);
    for (u32 s1 = 0; s1 < bsize; s1++) {
      const slot1 *pslot1 = buck + s1;
      u32 xhash1 = (pslot1->hash->bytes[htl.prevbo] & 0x7) << 4 | (pslot1->hash->bytes[htl.prevbo+1] >> 4);
		if(!cd.addslot(s1, xhash1))
	  		continue;
      for (; cd.nextcollision(); ) {
        const u32 s0 = cd.slot();
        const slot1 *pslot0 = buck + s0;
        if (htl.equal(pslot0->hash, pslot1->hash))
        	continue;
        
        u32 xorbucketid;
        const uchar *bytes0 = pslot0->hash->bytes, *bytes1 = pslot1->hash->bytes;
        xorbucketid = (((((u32)(bytes0[htl.prevbo + 1] ^ bytes1[htl.prevbo + 1]) & 0xf)) << 8) | (bytes0[htl.prevbo + 2] ^ bytes1[htl.prevbo + 2])) << 2 | ((bytes0[htl.prevbo + 3] ^ bytes1[htl.prevbo + 3]) >> 6);

        const u32 xorslot = atomicAdd(&eq->nslots[0][xorbucketid], 1);
        if (xorslot >= NSLOTS)
          continue;
        slot0 &xs = htl.hta.trees0[r/2][xorbucketid][xorslot];

        xs.attr = tree(bucketid, s0, s1);

        for (u32 i=htl.dunits; i < htl.prevhashunits; i++){
          xs.hash[i-htl.dunits].word = pslot0->hash[i].word ^ pslot1->hash[i].word;
        }
      }
    }
  }
}

__global__ void digit_5(equi210_9 *eq, const u32 r) {
  equi210_9::htlayout htl(eq, r);
  equi210_9::collisiondata cd;
  const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
  for (u32 bucketid=id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
    cd.clear();
    slot0 *buck = htl.hta.trees0[(r-1)/2][bucketid];
    u32 bsize = eq->getnslots0(bucketid);
    for (u32 s1 = 0; s1 < bsize; s1++) {
      const slot0 *pslot1 = buck + s1;
      u32 xhash0 = (pslot1->hash->bytes[htl.prevbo] & 0x3f) << 1 | (pslot1->hash->bytes[htl.prevbo+1] >> 7);
	  	if(!cd.addslot(s1, xhash0))
	  		continue;
      for (; cd.nextcollision(); ) {
        const u32 s0 = cd.slot();
        const slot0 *pslot0 = buck + s0;
        if (htl.equal(pslot0->hash, pslot1->hash))
        	continue;
        
        u32 xorbucketid;
        const uchar *bytes0 = pslot0->hash->bytes, *bytes1 = pslot1->hash->bytes;
       
	    xorbucketid = (((u32)(bytes0[htl.prevbo + 1] ^ bytes1[htl.prevbo + 1]) & 0x7f) << 7) | (bytes0[htl.prevbo + 2] ^ bytes1[htl.prevbo + 2]) >> 1;

        const u32 xorslot = atomicAdd(&eq->nslots[1][xorbucketid], 1);
        if (xorslot >= NSLOTS)
          continue;
        slot1 &xs = htl.hta.trees1[r/2][xorbucketid][xorslot];

        xs.attr = tree(bucketid, s0, s1);

        for (u32 i=htl.dunits; i < htl.prevhashunits; i++){
          xs.hash[i-htl.dunits].word = pslot0->hash[i].word ^ pslot1->hash[i].word;
        }
      }
    }
  }
}

__global__ void digit_6(equi210_9 *eq, const u32 r) {
  equi210_9::htlayout htl(eq, r);
  equi210_9::collisiondata cd;
  const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
  for (u32 bucketid=id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
    cd.clear();
    slot1 *buck = htl.hta.trees1[(r-1)/2][bucketid];
    u32 bsize = eq->getnslots1(bucketid);
    for (u32 s1 = 0; s1 < bsize; s1++) {
      const slot1 *pslot1 = buck + s1;
      u32 xhash1 = (pslot1->hash->bytes[htl.prevbo] & 0x1) << 6 | (pslot1->hash->bytes[htl.prevbo+1] >> 2);
		if(!cd.addslot(s1, xhash1))
	  		continue;
      for (; cd.nextcollision(); ) {
        const u32 s0 = cd.slot();
        const slot1 *pslot0 = buck + s0;
        if (htl.equal(pslot0->hash, pslot1->hash))
        	continue;
        
        u32 xorbucketid;
        const uchar *bytes0 = pslot0->hash->bytes, *bytes1 = pslot1->hash->bytes;

        xorbucketid = ((((u32)(bytes0[htl.prevbo + 1] ^ bytes1[htl.prevbo + 1]) & 0x3) << 8) | (bytes0[htl.prevbo + 2] ^ bytes1[htl.prevbo + 2])) << 4 | (bytes0[htl.prevbo + 3] ^ bytes1[htl.prevbo + 3]) >> 4;

        const u32 xorslot = atomicAdd(&eq->nslots[0][xorbucketid], 1);
        if (xorslot >= NSLOTS)
          continue;
        slot0 &xs = htl.hta.trees0[r/2][xorbucketid][xorslot];

        xs.attr = tree(bucketid, s0, s1);

        for (u32 i=htl.dunits; i < htl.prevhashunits; i++){
          xs.hash[i-htl.dunits].word = pslot0->hash[i].word ^ pslot1->hash[i].word;
        }
      }
    }
  }
}

__global__ void digit_7(equi210_9 *eq, const u32 r) {
  equi210_9::htlayout htl(eq, r);
  equi210_9::collisiondata cd;
  const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
  for (u32 bucketid=id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
    cd.clear();
    slot0 *buck = htl.hta.trees0[(r-1)/2][bucketid];
    u32 bsize = eq->getnslots0(bucketid);
    for (u32 s1 = 0; s1 < bsize; s1++) {
      const slot0 *pslot1 = buck + s1;
      u32 xhash0 = (pslot1->hash->bytes[htl.prevbo] & 0xf) << 3 | (pslot1->hash->bytes[htl.prevbo+1] >> 5);
	  	if(!cd.addslot(s1, xhash0))
	  		continue;
      for (; cd.nextcollision(); ) {
        const u32 s0 = cd.slot();
        const slot0 *pslot0 = buck + s0;
        if (htl.equal(pslot0->hash, pslot1->hash))
        	continue;
        
        u32 xorbucketid;
        const uchar *bytes0 = pslot0->hash->bytes, *bytes1 = pslot1->hash->bytes;

        xorbucketid = ((((u32)(bytes0[htl.prevbo + 1] ^ bytes1[htl.prevbo + 1]) & 0x1f) << 8) | (bytes0[htl.prevbo + 2] ^ bytes1[htl.prevbo + 2])) << 1 | (bytes0[htl.prevbo + 3] ^ bytes1[htl.prevbo + 3]) >> 7;

        const u32 xorslot = atomicAdd(&eq->nslots[1][xorbucketid], 1);
        if (xorslot >= NSLOTS)
          continue;
        slot1 &xs = htl.hta.trees1[r/2][xorbucketid][xorslot];

        xs.attr = tree(bucketid, s0, s1);

        for (u32 i=htl.dunits; i < htl.prevhashunits; i++){
          xs.hash[i-htl.dunits].word = pslot0->hash[i].word ^ pslot1->hash[i].word;
        }
      }
    }
  }
}

__global__ void digit_8(equi210_9 *eq, const u32 r) {
  equi210_9::htlayout htl(eq, r);
  equi210_9::collisiondata cd;
  const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
  for (u32 bucketid=id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
    cd.clear();
    slot1 *buck = htl.hta.trees1[(r-1)/2][bucketid];
    u32 bsize = eq->getnslots1(bucketid);
    for (u32 s1 = 0; s1 < bsize; s1++) {
      const slot1 *pslot1 = buck + s1;
      u32 xhash1 = (pslot1->hash->bytes[htl.prevbo] & 0x7f);
		if(!cd.addslot(s1, xhash1))
	  		continue;
      for (; cd.nextcollision(); ) {
        const u32 s0 = cd.slot();
        const slot1 *pslot0 = buck + s0;
        if (htl.equal(pslot0->hash, pslot1->hash))
        	continue;
        
        u32 xorbucketid;
        const uchar *bytes0 = pslot0->hash->bytes, *bytes1 = pslot1->hash->bytes;

        xorbucketid = ((u32)(bytes0[htl.prevbo+1] ^ bytes1[htl.prevbo+1]) << 6) | (bytes0[htl.prevbo + 2] ^ bytes1[htl.prevbo + 2]) >> 2;

        const u32 xorslot = atomicAdd(&eq->nslots[0][xorbucketid], 1);
        if (xorslot >= NSLOTS)
          continue;
        slot0 &xs = htl.hta.trees0[r/2][xorbucketid][xorslot];

        xs.attr = tree(bucketid, s0, s1);

        for (u32 i=htl.dunits; i < htl.prevhashunits; i++){
          xs.hash[i-htl.dunits].word = pslot0->hash[i].word ^ pslot1->hash[i].word;
        }
      }
    }
  }
}

__global__ void digit_9(equi210_9 *eq) {
  equi210_9::collisiondata cd;
  equi210_9::htlayout htl(eq, WK);
  const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
  for (u32 bucketid = id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
    cd.clear();
    slot0 *buck = htl.hta.trees0[(WK-1)/2][bucketid];
    u32 bsize = eq->getnslots0(bucketid); // assume WK odd
    for (u32 s1 = 0; s1 < bsize; s1++) {
      const slot0 *pslot1 = buck + s1;
      u32 xhash0 = ((pslot1->hash->bytes[htl.prevbo] & 0x3) << 5) | ((pslot1->hash->bytes[htl.prevbo+1]) >> 3);
      cd.addslot(s1, xhash0);
      for (; cd.nextcollision(); ) { // assume WK odd
        const u32 s0 = cd.slot();
        const slot0 *pslot0 = buck + s0;

        //Check last 21 bits for collision, disjoint and process candidate.

        if (htl.equal_210(pslot0,pslot1)) {
          eq->candidate(tree(bucketid, s0, s1));
        }
      }
    }
  }
}
#endif


__global__ void digitK(equi210_9 *eq) {
	equi210_9::collisiondata cd;
	equi210_9::htlayout htl(eq, WK);
	const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
	for (u32 bucketid = id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
		cd.clear();
		slot0 *buck = htl.hta.trees0[(WK - 1) / 2][bucketid];
		u32 bsize = eq->getnslots0(bucketid); // assume WK odd
		for (u32 s1 = 0; s1 < bsize; s1++) {
			const slot0 *pslot1 = buck + s1;
			if (!cd.addslot(s1, htl.getxhash0(pslot1))) // assume WK odd
				continue;
			for (; cd.nextcollision();) {
				const u32 s0 = cd.slot();
				const slot0 *pslot0 = buck + s0;
				if (htl.equal(pslot0->hash, pslot1->hash)) {
#ifdef XINTREE
					eq->candidate(tree(bucketid, s0, s1, 0));
#else
					eq->candidate(tree(bucketid, s0, s1));
#endif
				}
			}
		}
	}
}


eq_cuda_context210_9::eq_cuda_context210_9(int thrid, int devid, fn_validate validate, fn_cancel cancel)	
{
    threadsperblock = 64;
    device_id = devid;
    thread_id = thrid;

    m_fnValidate = validate;
    m_fnCancel = cancel;

    cudaDeviceProp device_props;
    checkCudaErrors(cudaGetDeviceProperties(&device_props, device_id));
    totalblocks = device_props.multiProcessorCount * 7;
    
	eq = new equi210_9(threadsperblock * totalblocks);
	sol_memory = malloc(sizeof(proof) * MAXSOLS + 4096);
	solutions = (proof*)(((long long)sol_memory + 4095) & -4096);

	checkCudaErrors(cudaSetDevice(device_id));
	checkCudaErrors(cudaDeviceReset());
	checkCudaErrors(cudaSetDeviceFlags(cudaDeviceScheduleBlockingSync));
	checkCudaErrors(cudaDeviceSetCacheConfig(cudaFuncCachePreferL1));

	checkCudaErrors(cudaMalloc((void**)&heap0, sizeof(digit0)));
	checkCudaErrors(cudaMalloc((void**)&heap1, sizeof(digit1)));
	for (u32 r = 0; r < WK; r++)
		if ((r & 1) == 0)
			eq->hta.trees0[r / 2] = (bucket0 *)(heap0 + r / 2);
		else
			eq->hta.trees1[r / 2] = (bucket1 *)(heap1 + r / 2);

	checkCudaErrors(cudaMalloc((void**)&eq->nslots, 2 * NBUCKETS * sizeof(u32)));
	checkCudaErrors(cudaMalloc((void**)&eq->sols, MAXSOLS * sizeof(proof)));

	checkCudaErrors(cudaMalloc((void**)&device_eq, sizeof(equi210_9)));
}


eq_cuda_context210_9::~eq_cuda_context210_9()
{
	/*checkCudaErrors(cudaFree(eq->nslots));
	checkCudaErrors(cudaFree(eq->sols));
	checkCudaErrors(cudaFree(eq->hta.trees0[0]));
	checkCudaErrors(cudaFree(eq->hta.trees1[0]));*/
	checkCudaErrors(cudaSetDevice(device_id));
	checkCudaErrors(cudaDeviceReset());
	free(sol_memory);
	delete eq;
}

std::vector<unsigned char> GetMinimalFromIndices(std::vector<uint32_t> indices,
                                                 size_t cBitLen);

bool eq_cuda_context210_9::solve(unsigned char *pblock, unsigned char *header, unsigned int headerlen)
{
	checkCudaErrors(cudaSetDevice(device_id)); 

	eq->setheadernonce(header, headerlen-32, header+headerlen-32, 32);
	checkCudaErrors(cudaMemcpy(device_eq, eq, sizeof(equi210_9), cudaMemcpyHostToDevice));

	digitH << <totalblocks, threadsperblock >> >(device_eq);
	if (m_fnCancel()) return false;
#if BUCKBITS == 16 && RESTBITS == 4 && defined XINTREE && defined(UNROLL)
	digit_1 << <totalblocks, threadsperblock >> >(device_eq);
	if (m_fnCancel()) return false;
	digit2 << <totalblocks, threadsperblock >> >(device_eq);
	if (m_fnCancel()) return false;
	digit3 << <totalblocks, threadsperblock >> >(device_eq);
	if (m_fnCancel()) return false;
	digit4 << <totalblocks, threadsperblock >> >(device_eq);
	if (m_fnCancel()) return false;
	digit5 << <totalblocks, threadsperblock >> >(device_eq);
	if (m_fnCancel()) return false;
	digit6 << <totalblocks, threadsperblock >> >(device_eq);
	if (m_fnCancel()) return false;
	digit7 << <totalblocks, threadsperblock >> >(device_eq);
	if (m_fnCancel()) return false;
	digit8 << <totalblocks, threadsperblock >> >(device_eq);
	if (m_fnCancel()) return false;
	digitK << <totalblocks, threadsperblock >> >(device_eq);

#elif BUCKBITS == 14 && RESTBITS == 7
	digit_1 << <totalblocks, threadsperblock >> >(device_eq, 1);
	if (m_fnCancel()) return false;
	digit_2 << <totalblocks, threadsperblock >> >(device_eq, 2);
	if (m_fnCancel()) return false;
	digit_3 << <totalblocks, threadsperblock >> >(device_eq, 3);
	if (m_fnCancel()) return false;
	digit_4 << <totalblocks, threadsperblock >> >(device_eq, 4);
	if (m_fnCancel()) return false;
	digit_5 << <totalblocks, threadsperblock >> >(device_eq, 5);
	if (m_fnCancel()) return false;
	digit_6 << <totalblocks, threadsperblock >> >(device_eq, 6);
	if (m_fnCancel()) return false;
	digit_7 << <totalblocks, threadsperblock >> >(device_eq, 7);
	if (m_fnCancel()) return false;
	digit_8 << <totalblocks, threadsperblock >> >(device_eq, 8);
	if (m_fnCancel()) return false;
	digit_9 << <totalblocks, threadsperblock >> >(device_eq);
#else
	for (u32 r = 1; r < WK; r++) {
		r & 1 ? digitO << <totalblocks, threadsperblock >> >(device_eq, r)
			: digitE << <totalblocks, threadsperblock >> >(device_eq, r);
	}

	if (m_fnCancel()) return false;
	digitK << <totalblocks, threadsperblock >> >(device_eq);
#endif

	checkCudaErrors(cudaMemcpy(eq, device_eq, sizeof(equi210_9), cudaMemcpyDeviceToHost));
	checkCudaErrors(cudaMemcpy(solutions, eq->sols, MAXSOLS * sizeof(proof), cudaMemcpyDeviceToHost));

	for (unsigned s = 0; (s < eq->nsols) && (s < MAXSOLS); s++)
	{
		std::vector<uint32_t> index_vector(PROOFSIZE1);
		for (u32 i = 0; i < PROOFSIZE1; i++) {
			index_vector[i] = solutions[s][i];
		}

		std::vector<unsigned char> sol_char = GetMinimalFromIndices(index_vector, DIGITBITS);
		if (m_fnValidate(sol_char, pblock, thread_id)) 
        {
             // If we find a POW solution, do not try other solutions
             // because they become invalid as we created a new block in blockchain.
             return true;
        }        
	}
	return false;
}