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

#define WN	184
#define WK	7
#define RESTBITS	4 

#define NDIGITS		(WK+1)
#define DIGITBITS	(WN/(NDIGITS))
#define BASE        (1<<DIGITBITS)
#define NHASHES     (2*BASE)
#define HASHESPERBLAKE (512/WN)
#define HASHOUT     (HASHESPERBLAKE*WN/8)
#define BUCKBITS    (DIGITBITS-RESTBITS)

#ifndef SAVEMEM
#if RESTBITS == 4
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

static const u32 HASHLEN = (WN+7)/8;
void setheader(blake2b_state *ctx, const unsigned char *header, const u32 headerLen, const unsigned char* nce, const u32 nonceLen) 
{
  uint32_t le_N = WN;
  uint32_t le_K = WK;
  uchar personal[] = "ZcashPoW01230123";
  
  if( le_N != 200 || le_K != 9 )
      memcpy(personal, "FABcoin_", 8);

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
  __device__ bool prob_disjoint(const tree other) const {
    tree xort(bid_s0_s1_x ^ other.bid_s0_s1_x);
    return xort.bucketid() || (xort.slotid0() && xort.slotid1());
    // next two tests catch much fewer cases and are therefore skipped
    // && slotid0() != other.slotid1() && slotid1() != other.slotid0()
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
  const u32 hashbits = WN - (r+1) * DIGITBITS;
#else
  const u32 hashbits = WN - (r+1) * DIGITBITS + RESTBITS;
#endif
	return (hashbits + 7) / 8;
}
// size (in bytes) of hash in round 0 <= r < WK
__device__ u32 hashsize(const u32 r) {
#ifdef XINTREE
  const u32 hashbits = WN - (r+1) * DIGITBITS;
  return (hashbits + 7) / 8;
#else
    const u32 hashbits = WN - (r+1) * DIGITBITS + RESTBITS;
    return (hashbits + 7) / 8;
#endif
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

struct equi1847 {
	blake2b_state blake_ctx;
	htalloc hta;
	bsizes *nslots;
	proof *sols;
	u32 nsols;
	u32 nthreads;
	equi1847(const u32 n_threads) {
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
  __device__ bool orderindices(u32 *indices, u32 size) {
		if (indices[0] > indices[size]) {
			for (u32 i = 0; i < size; i++) {
				const u32 tmp = indices[i];
				indices[i] = indices[size + i];
				indices[size + i] = tmp;
			}
		}
    return false;
	}
  __device__ bool listindices1(const tree t, u32 *indices) {
		const bucket0 &buck = hta.trees0[0][t.bucketid()];
		const u32 size = 1 << 0;
		indices[0] = buck[t.slotid0()].attr.getindex();
		indices[size] = buck[t.slotid1()].attr.getindex();
		orderindices(indices, size);
    return false;
	}
  __device__ bool listindices2(const tree t, u32 *indices) {
    const bucket1 &buck = hta.trees1[0][t.bucketid()];
    const u32 size = 1 << 1;
    return listindices1(buck[t.slotid0()].attr, indices) || 
           listindices1(buck[t.slotid1()].attr, indices+size) ||
           orderindices(indices, size) || indices[0] == indices[size];
  }
  __device__ bool listindices3(const tree t, u32 *indices) {
    const bucket0 &buck = hta.trees0[1][t.bucketid()];
    const u32 size = 1 << 2;
    return listindices2(buck[t.slotid0()].attr, indices) || 
           listindices2(buck[t.slotid1()].attr, indices+size) ||
           orderindices(indices, size) || indices[0] == indices[size];
  }
  __device__ bool listindices4(const tree t, u32 *indices) {
    const bucket1 &buck = hta.trees1[1][t.bucketid()];
    const u32 size = 1 << 3;
    return listindices3(buck[t.slotid0()].attr, indices) || 
           listindices3(buck[t.slotid1()].attr, indices+size) ||
           orderindices(indices, size) || indices[0] == indices[size];
  }
  __device__ bool listindices5(const tree t, u32 *indices) {
    const bucket0 &buck = hta.trees0[2][t.bucketid()];
    const u32 size = 1 << 4;
    return listindices4(buck[t.slotid0()].attr, indices) || 
           listindices4(buck[t.slotid1()].attr, indices+size) ||
           orderindices(indices, size) || indices[0] == indices[size];
  }

  __device__ bool listindices6(const tree t, u32 *indices) {
    const bucket1 &buck = hta.trees1[2][t.bucketid()];
    const u32 size = 1 << 5;
    return listindices5(buck[t.slotid0()].attr, indices) || 
           listindices5(buck[t.slotid1()].attr, indices+size) ||
           orderindices(indices, size) || indices[0] == indices[size];
  }
  __device__ bool listindices7(const tree t, u32 *indices) {
    const bucket0 &buck = hta.trees0[3][t.bucketid()];
    const u32 size = 1 << 6;
    return listindices6(buck[t.slotid0()].attr, indices) || 
           listindices6(buck[t.slotid1()].attr, indices+size) ||
           orderindices(indices, size) || indices[0] == indices[size];
  }

#if WK >= 8
  __device__ bool listindices8(const tree t, u32 *indices) {
    const bucket1 &buck = hta.trees1[3][t.bucketid()];
    const u32 size = 1 << 7;
    return listindices7(buck[t.slotid0()].attr, indices) || 
           listindices7(buck[t.slotid1()].attr, indices+size) ||
           orderindices(indices, size) || indices[0] == indices[size];
  }
  __device__ bool listindices9(const tree t, u32 *indices) {
    const bucket0 &buck = hta.trees0[4][t.bucketid()];
    const u32 size = 1 << 8;
    return listindices8(buck[t.slotid0()].attr, indices) || 
           listindices8(buck[t.slotid1()].attr, indices+size) ||
           orderindices(indices, size) || indices[0] == indices[size];
  }
#endif

  __device__ void candidate(const tree t) {
    proof prf;
#if WK==9
    if (listindices9(t, prf)) return;
#elif WK==7
    if (listindices7(t, prf)) return;
#elif WK==5
    if (listindices5(t, prf)) return;
#else
#error not implemented
#endif
    u32 soli = atomicAdd(&nsols, 1);
    if (soli < MAXSOLS)
#if WK==9
      listindices9(t, sols[soli]);
#elif WK==7
      listindices7(t, sols[soli]);
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
  struct htlayout {
    htalloc hta;
    u32 prevhashunits;
    u32 nexthashunits;
    u32 dunits;
    u32 prevbo;
    u32 nextbo;

		__device__ htlayout(equi1847 *eq, u32 r) : hta(eq->hta), prevhashunits(0), dunits(0) {
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
    __device__ u32 getxhash0(const slot0* pslot, int r = 0 ) const {
#ifdef XINTREE
      return pslot->attr.xhash();
#elif DIGITBITS % 8 == 4 && RESTBITS == 4
      return pslot->hash->bytes[prevbo] >> 4;
#elif DIGITBITS % 8 == 4 && RESTBITS == 6
      return (pslot->hash->bytes[prevbo] & 0x3) << 4 | pslot->hash->bytes[prevbo+1] >> 4;
#elif DIGITBITS % 8 == 4 && RESTBITS == 8
      return (pslot->hash->bytes[prevbo] & 0xf) << 4 | pslot->hash->bytes[prevbo+1] >> 4;
#elif DIGITBITS % 8 == 4 && RESTBITS == 10
      return (pslot->hash->bytes[prevbo] & 0x3f) << 4 | pslot->hash->bytes[prevbo+1] >> 4;
#elif DIGITBITS % 8 == 7 && RESTBITS == 4  // 184,7
      if( r != 0 )
      {
          switch(r)
          {
          case 1:
              return (pslot->hash->bytes[prevbo] & 0x1f) >>1;
          case 3:
              return ((pslot->hash->bytes[prevbo] & 0x7f) >>3);
          case 5:
              return ((pslot->hash->bytes[prevbo] & 0x1) << 3) | (pslot->hash->bytes[prevbo+1] >> 5);
          case 7:
              return ((pslot->hash->bytes[prevbo] & 0x7) << 1) | (pslot->hash->bytes[prevbo+1] >> 7);
          default:
              return 0;
          }
      }      
#elif DIGITBITS % 8 == 0 && RESTBITS == 4
      return pslot->hash->bytes[prevbo] & 0xf;
#elif RESTBITS == 0
      return 0;
#else
#error non implemented
#endif
        return 0;
    }
    __device__ u32 getxhash1(const slot1* pslot, int r = 0 ) const {
#ifdef XINTREE
      return pslot->attr.xhash();
#elif DIGITBITS % 4 == 0 && RESTBITS == 4
      return pslot->hash->bytes[prevbo] & 0xf;
#elif DIGITBITS % 4 == 0 && RESTBITS == 6
      return pslot->hash->bytes[prevbo] & 0x3f;
#elif DIGITBITS % 4 == 0 && RESTBITS == 8
      return pslot->hash->bytes[prevbo];
#elif DIGITBITS % 4 == 0 && RESTBITS == 10
      return (pslot->hash->bytes[prevbo] & 0x3) << 8 | pslot->hash->bytes[prevbo+1];
#elif DIGITBITS % 8 == 7 && RESTBITS == 4  // 184,7
        if( r != 0 )
        {
            switch(r)
            {
            case 2:
                return ((pslot->hash->bytes[prevbo] & 0x3f) >> 2);
            case 4:
                return pslot->hash->bytes[prevbo] >> 4;
            case 6:
                return ((pslot->hash->bytes[prevbo] & 0x3 ) << 2) | (pslot->hash->bytes[prevbo+1] >> 6);
            default:
                return 0;
            }
        }      
#elif RESTBITS == 0
      return 0;
#else
#error non implemented
#endif
        return 0;
    }
    __device__ void getxorbucket(const uchar *bytes0, const uchar *bytes1, u32 &xorbucketid, u32 &xhash, int r = 0 ) const {
    
#if WN == 184 && WK == 7
        switch( r )
        {
            case 1:
                xorbucketid = ((u32)(bytes0[prevbo] ^ bytes1[prevbo]) & 0x1) << 18
                            | ((u32)(bytes0[prevbo+1] ^ bytes1[prevbo+1])) << 10
                            | ((u32)(bytes0[prevbo+2] ^ bytes1[prevbo+2])) << 2
                            | (bytes0[prevbo+3] ^ bytes1[prevbo+3]) >> 6;
                break;
            case 2:
                xorbucketid = ((u32)(bytes0[prevbo] ^ bytes1[prevbo]) & 0x3) << 17
                    | ((u32)(bytes0[prevbo+1] ^ bytes1[prevbo+1])) << 9
                    | ((u32)(bytes0[prevbo+2] ^ bytes1[prevbo+2])) << 1
                    | (bytes0[prevbo+3] ^ bytes1[prevbo+3]) >> 7;
                break;
            case 3:
                xorbucketid = (((u32)(bytes0[prevbo] ^ bytes1[prevbo]) & 0x7) << 16)
                    | ((u32)(bytes0[prevbo+1] ^ bytes1[prevbo+1])) << 8
                    | (bytes0[prevbo+2] ^ bytes1[prevbo+2]);
                break;
            case 4:
                xorbucketid = ((u32)(bytes0[prevbo] ^ bytes1[prevbo]) & 0xf) << 15
                    | ((u32)(bytes0[prevbo+1] ^ bytes1[prevbo+1])) << 7
                    | (bytes0[prevbo+2] ^ bytes1[prevbo+2]) >> 1;
                break;
            case 5:
                xorbucketid = ((u32)(bytes0[prevbo+1] ^ bytes1[prevbo+1]) & 0x1f ) << 14
                    | ((u32)(bytes0[prevbo+2] ^ bytes1[prevbo+2])) << 6
                    | (bytes0[prevbo+3] ^ bytes1[prevbo+3]) >> 2;
                break;
            case 6:
                xorbucketid = (((u32)(bytes0[prevbo+1] ^ bytes1[prevbo+1]) & 0x3f ) << 13 )
                    | ((u32)(bytes0[prevbo+2] ^ bytes1[prevbo+2])) << 5
                    | (bytes0[prevbo+3] ^ bytes1[prevbo+3]) >> 3;
                break;
        }
#endif
    }        

    __device__ bool equal(const hashunit *hash0, const hashunit *hash1) const {
      return hash0[prevhashunits-1].word == hash1[prevhashunits-1].word;
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
    __device__ void addslot(u32 s1, u32 xh) {
#ifdef XBITMAP
      xmap = xhashmap[xh];
      xhashmap[xh] |= (u64)1 << s1;
      s0 = ~0;
#else
      nextslot = xhashslots[xh];
      nextxhashslot[s1] = nextslot;
      xhashslots[xh] = s1;
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

__global__ void digitH(equi1847 *eq) {
	uchar hash[HASHOUT];
	blake2b_state state;
	equi1847::htlayout htl(eq, 0);
	const u32 hashbytes = hashsize(0); // always 23 ?
	const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
	for (u32 block = id; block < NBLOCKS; block += eq->nthreads) {
		state = eq->blake_ctx;
		blake2b_gpu_hash(&state, block, hash, HASHOUT);
		for (u32 i = 0; i<HASHESPERBLAKE; i++) {
      const uchar *ph = hash + i * (HASHLEN);
#if BUCKBITS == 16 && RESTBITS == 4
			const u32 bucketid = ((u32)ph[0] << 8) | ph[1];
#ifdef XINTREE
			const u32 xhash = ph[2] >> 4;
#endif
#elif BUCKBITS == 14 && RESTBITS == 6
			const u32 bucketid = ((u32)ph[0] << 6) | ph[1] >> 2;
#elif BUCKBITS == 12 && RESTBITS == 8
      const u32 bucketid = ((u32)ph[0] << 4) | ph[1] >> 4;
#elif BUCKBITS == 19 && RESTBITS == 4 // 184,7     
      const u32 bucketid = ((((u32)ph[0] << 8) | ph[1]) << 3) | ph[2] >> 5;
#elif BUCKBITS == 20 && RESTBITS == 4
			const u32 bucketid = ((((u32)ph[0] << 8) | ph[1]) << 4) | ph[2] >> 4;
#ifdef XINTREE
			const u32 xhash = ph[2] & 0xf;
#endif
#elif BUCKBITS == 12 && RESTBITS == 4
      const u32 bucketid = ((u32)ph[0] << 4) | ph[1] >> 4;
#ifdef XINTREE
      const u32 xhash = ph[1] & 0xf;
#endif
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

__global__ void digitO(equi1847 *eq, const u32 r) {
	equi1847::htlayout htl(eq, r);
	equi1847::collisiondata cd;
	const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
	for (u32 bucketid = id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
		cd.clear();
		slot0 *buck = htl.hta.trees0[(r - 1) / 2][bucketid];
		u32 bsize = eq->getnslots0(bucketid);
		for (u32 s1 = 0; s1 < bsize; s1++) {
			const slot0 *pslot1 = buck + s1;
            u32 xhash0 = htl.getxhash0( pslot1, r );
            for (cd.addslot(s1, xhash0); cd.nextcollision(); ) 
            {
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
#elif WN % 24 == 0 && BUCKBITS == 20 && RESTBITS == 4
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
#elif WN == 184 && WK == 7 // 184,7
                htl.getxorbucket(bytes0, bytes1, xorbucketid, xhash, r );
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

__global__ void digitE(equi1847 *eq, const u32 r) {
	equi1847::htlayout htl(eq, r);
	equi1847::collisiondata cd;
	const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
	for (u32 bucketid = id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
		cd.clear();
		slot1 *buck = htl.hta.trees1[(r - 1) / 2][bucketid];
		u32 bsize = eq->getnslots1(bucketid);
		for (u32 s1 = 0; s1 < bsize; s1++) {
			const slot1 *pslot1 = buck + s1;
      u32 xhash1 = htl.getxhash1( pslot1, r );
      for (cd.addslot(s1, xhash1); cd.nextcollision(); ) {
				const u32 s0 = cd.slot();
				const slot1 *pslot0 = buck + s0;
				if (htl.equal(pslot0->hash, pslot1->hash))
					continue;
				u32 xorbucketid;
				u32 xhash;
				const uchar *bytes0 = pslot0->hash->bytes, *bytes1 = pslot1->hash->bytes;
#if WN == 200 && BUCKBITS == 16 && RESTBITS == 4 && defined(XINTREE)
        xorbucketid = ((u32)(bytes0[htl.prevbo] ^ bytes1[htl.prevbo]) << 8)
                        | (bytes0[htl.prevbo+1] ^ bytes1[htl.prevbo+1]);
        u32 xhash = (bytes0[htl.prevbo+2] ^ bytes1[htl.prevbo+2]) >> 4;
#elif WN % 24 == 0 && BUCKBITS == 20 && RESTBITS == 4
        xorbucketid = ((((u32)(bytes0[htl.prevbo+1] ^ bytes1[htl.prevbo+1]) << 8)
                            | (bytes0[htl.prevbo+2] ^ bytes1[htl.prevbo+2])) << 4)
                            | (bytes0[htl.prevbo+3] ^ bytes1[htl.prevbo+3]) >> 4;
#elif WN == 96 && BUCKBITS == 12 && RESTBITS == 4
				xorbucketid = ((u32)(bytes0[htl.prevbo + 1] ^ bytes1[htl.prevbo + 1]) << 4)
					| (bytes0[htl.prevbo + 2] ^ bytes1[htl.prevbo + 2]) >> 4;
#elif WN == 200 && BUCKBITS == 14 && RESTBITS == 6
        xorbucketid = ((u32)(bytes0[htl.prevbo+1] ^ bytes1[htl.prevbo+1]) << 6)
                          | (bytes0[htl.prevbo+2] ^ bytes1[htl.prevbo+2]) >> 2;
#elif WN == 184 && WK == 7
        htl.getxorbucket(bytes0, bytes1, xorbucketid, xhash, r );
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

__global__ void digitK(equi1847 *eq) {
  equi1847::collisiondata cd;
  equi1847::htlayout htl(eq, WK);
  const u32 id = blockIdx.x * blockDim.x + threadIdx.x;
  for (u32 bucketid = id; bucketid < NBUCKETS; bucketid += eq->nthreads) {
    cd.clear();
    slot0 *buck = htl.hta.trees0[(WK-1)/2][bucketid];
    u32 bsize = eq->getnslots0(bucketid); // assume WK odd
    for (u32 s1 = 0; s1 < bsize; s1++) {
      const slot0 *pslot1 = buck + s1;

      u32 xhash0 = htl.getxhash0( pslot1, WK );
      
      for (cd.addslot(s1, xhash0); cd.nextcollision(); ) { // assume WK odd
        const u32 s0 = cd.slot();
        const slot0 *pslot0 = buck + s0;
        if (htl.equal(pslot0->hash, pslot1->hash) && pslot0->attr.prob_disjoint(pslot1->attr)) {
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


eq_cuda_context1847::eq_cuda_context1847(int thrid, int devid, fn_validate validate, fn_cancel cancel)	
{
    threadsperblock = 64;
    device_id = devid;
    thread_id = thrid;

    m_fnValidate = validate;
    m_fnCancel = cancel;

    cudaDeviceProp device_props;
    checkCudaErrors(cudaGetDeviceProperties(&device_props, device_id));
    totalblocks = device_props.multiProcessorCount * 7;
    
	eq = new equi1847(threadsperblock * totalblocks);
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

	checkCudaErrors(cudaMalloc((void**)&device_eq, sizeof(equi1847)));
}


eq_cuda_context1847::~eq_cuda_context1847()
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

bool eq_cuda_context1847::solve(unsigned char *pblock, unsigned char *header, unsigned int headerlen)
{
	checkCudaErrors(cudaSetDevice(device_id)); 

	eq->setheadernonce(header, headerlen-32, header+headerlen-32, 32);
	checkCudaErrors(cudaMemcpy(device_eq, eq, sizeof(equi1847), cudaMemcpyHostToDevice));

	digitH << <totalblocks, threadsperblock >> >(device_eq);
	if (m_fnCancel()) return false;

    for (u32 r = 1; r < WK; r++) {
		r & 1 ? digitO << <totalblocks, threadsperblock >> >(device_eq, r)
			: digitE << <totalblocks, threadsperblock >> >(device_eq, r);
	}

	if (m_fnCancel()) return false;
	digitK << <totalblocks, threadsperblock >> >(device_eq);

	checkCudaErrors(cudaMemcpy(eq, device_eq, sizeof(equi1847), cudaMemcpyDeviceToHost));
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