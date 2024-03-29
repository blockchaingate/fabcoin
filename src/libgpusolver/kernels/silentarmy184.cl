#define PARAM_N				            184 
#define PARAM_K				            7   
#define PREFIX                          (PARAM_N / (PARAM_K + 1))
#define NR_INPUTS                       (1 << PREFIX)
// Approximate log base 2 of number of elements in hash tables
#define APX_NR_ELMS_LOG                 (PREFIX + 1)
// Number of rows and slots is affected by this; 20 offers the best performance
#define NR_ROWS_LOG                     21

// Number of collision items to track, per thread
#define COLL_DATA_SIZE_PER_TH		    (NR_SLOTS * 5)

// Make hash tables OVERHEAD times larger than necessary to store the average
// number of elements per row. The ideal value is as small as possible to
// reduce memory usage, but not too small or else elements are dropped from the
// hash tables.
//
// The actual number of elements per row is closer to the theoretical average
// (less variance) when NR_ROWS_LOG is small. So accordingly OVERHEAD can be
// smaller.
//
// Even (as opposed to odd) values of OVERHEAD sometimes significantly decrease
// performance as they cause VRAM channel conflicts.
#if NR_ROWS_LOG == 21
#define OVERHEAD                        2
#endif

#define NR_ROWS                         (1 << NR_ROWS_LOG)
#define NR_SLOTS                        (( 1 << (APX_NR_ELMS_LOG - NR_ROWS_LOG)) * OVERHEAD)
// Length of 1 element (slot) in bytes
#define SLOT_LEN                        32

// Total size of hash table
#define HT_SIZE				            (NR_ROWS * NR_SLOTS * SLOT_LEN)

// Number of bytes Zcash needs out of Blake
#define FAB_HASH_LEN                    512/PARAM_N*((PARAM_N+7)/8) // 50
// Maximum number of solutions reported by kernel to host
#define MAX_SOLS			            10

#if (NR_SLOTS < 16)
#define BITS_PER_ROW 4
#define ROWS_PER_UINT 8
#define ROW_MASK 0x0F
#else
#define BITS_PER_ROW 8
#define ROWS_PER_UINT 4
#define ROW_MASK 0xFF
#endif

// Optional features
#undef ENABLE_DEBUG

/*
** Return the offset of Xi in bytes from the beginning of the slot.
*/
#define xi_offset_for_round(round)	(8 + ((round) / 2) * 4)
typedef struct	sols_s
{
    uint	nr;
    uint	likely_invalids;
    uchar	valid[MAX_SOLS];
    uint	values[MAX_SOLS][(1 << 9)];
}sols_t;


#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

/*
** Assuming NR_ROWS_LOG == 16, the hash table slots have this layout (length in
** bytes in parens):
**
** round 0, table 0: cnt(4) i(4)                     pad(0)   Xi(23.0) pad(1)
** round 1, table 1: cnt(4) i(4)                     pad(0.5) Xi(20.5) pad(3)
** round 2, table 0: cnt(4) i(4) i(4)                pad(0)   Xi(18.0) pad(2)
** round 3, table 1: cnt(4) i(4) i(4)                pad(0.5) Xi(15.5) pad(4)
** round 4, table 0: cnt(4) i(4) i(4) i(4)           pad(0)   Xi(13.0) pad(3)
** round 5, table 1: cnt(4) i(4) i(4) i(4)           pad(0.5) Xi(10.5) pad(5)
** round 6, table 0: cnt(4) i(4) i(4) i(4) i(4)      pad(0)   Xi( 8.0) pad(4)
** round 7, table 1: cnt(4) i(4) i(4) i(4) i(4)      pad(0.5) Xi( 5.5) pad(6)
** round 8, table 0: cnt(4) i(4) i(4) i(4) i(4) i(4) pad(0)   Xi( 3.0) pad(5)
**
** If the first byte of Xi is 0xAB then:
** - on even rounds, 'A' is part of the colliding PREFIX, 'B' is part of Xi
** - on odd rounds, 'A' and 'B' are both part of the colliding PREFIX, but
**   'A' is considered redundant padding as it was used to compute the row #
**
** - cnt is an atomic counter keeping track of the number of used slots.
**   it is used in the first slot only; subsequent slots replace it with
**   4 padding bytes
** - i encodes either the 21-bit input value (round 0) or a reference to two
**   inputs from the previous round
**
** Formula for Xi length and pad length above:
** > for i in range(9):
** >   xi=(200-20*i-NR_ROWS_LOG)/8.; ci=8+4*((i)/2); print xi,32-ci-xi
**
** Note that the fractional .5-byte/4-bit padding following Xi for odd rounds
** is the 4 most significant bits of the last byte of Xi.
*/

__constant ulong blake_iv[] =
{
    0x6a09e667f3bcc908, 0xbb67ae8584caa73b,
    0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
    0x510e527fade682d1, 0x9b05688c2b3e6c1f,
    0x1f83d9abfb41bd6b, 0x5be0cd19137e2179,
};

/*
** Reset counters in hash table.
*/
__kernel
void kernel_init_ht(__global char *ht, __global uint *rowCounters)
{
    rowCounters[get_global_id(0)] = 0;
}

/*
** If xi0,xi1,xi2,xi3 are stored consecutively in little endian then they
** represent (hex notation, group of 5 hex digits are a group of PREFIX bits):
**   aa aa ab bb bb cc cc cd dd...  [round 0]
**         --------------------
**      ...ab bb bb cc cc cd dd...  [odd round]
**               --------------
**               ...cc cc cd dd...  [next even round]
**                        -----
** Bytes underlined are going to be stored in the slot. Preceding bytes
** (and possibly part of the underlined bytes, depending on NR_ROWS_LOG) are
** used to compute the row number.
**
** Round 0: xi0,xi1,xi2,xi3 is a 25-byte Xi (xi3: only the low byte matter)
** Round 1: xi0,xi1,xi2 is a 23-byte Xi (incl. the colliding PREFIX nibble)
** TODO: update lines below with padding nibbles
** Round 2: xi0,xi1,xi2 is a 20-byte Xi (xi2: only the low 4 bytes matter)
** Round 3: xi0,xi1,xi2 is a 17.5-byte Xi (xi2: only the low 1.5 bytes matter)
** Round 4: xi0,xi1 is a 15-byte Xi (xi1: only the low 7 bytes matter)
** Round 5: xi0,xi1 is a 12.5-byte Xi (xi1: only the low 4.5 bytes matter)
** Round 6: xi0,xi1 is a 10-byte Xi (xi1: only the low 2 bytes matter)
** Round 7: xi0 is a 7.5-byte Xi (xi0: only the low 7.5 bytes matter)
** Round 8: xi0 is a 5-byte Xi (xi0: only the low 5 bytes matter)
**
** Return 0 if successfully stored, or 1 if the row overflowed.
*/
//uint ht_store(uint round, __global char *ht, uint i,
//	ulong xi0, ulong xi1, ulong xi2, ulong xi3, __global uint *rowCounters)

#define mix(va, vb, vc, vd, x, y) \
    va = (va + vb + x); \
    vd = rotate((vd ^ va), (ulong)64 - 32); \
    vc = (vc + vd); \
    vb = rotate((vb ^ vc), (ulong)64 - 24); \
    va = (va + vb + y); \
    vd = rotate((vd ^ va), (ulong)64 - 16); \
    vc = (vc + vd); \
    vb = rotate((vb ^ vc), (ulong)64 - 63);

/*
** Execute round 0 (blake).
**
** Note: making the work group size less than or equal to the wavefront size
** allows the OpenCL compiler to remove the barrier() calls, see "2.2 Local
** Memory (LDS) Optimization 2-10" in:
** http://developer.amd.com/tools-and-sdks/opencl-zone/amd-accelerated-parallel-processing-app-sdk/opencl-optimization-guide/
*/
__kernel __attribute__((reqd_work_group_size(64, 1, 1)))
void kernel_round0(__global ulong *blake_state, __global char *ht, __global ulong *buf,
                   __global uint *rowCounters, __global uint *debug)
{
    uint                tid = get_global_id(0);
    ulong               v[16];
    uint                inputs_per_thread = NR_INPUTS / get_global_size(0);
    uint                input = tid * inputs_per_thread;
    uint                input_end = (tid + 1) * inputs_per_thread;
    uint                dropped = 0;
    ulong val1 = 0, val9 = 0;
    
    uint buflen;
        
    if( buf[0] == 2 )
    {
        buflen = 12;
    }
    else
    {
        buflen = 76;
        val1 = buf[2];
    }
    while (input < input_end)
    {
	    // init vector v
	    v[0] = blake_state[0];
	    v[1] = blake_state[1];
	    v[2] = blake_state[2];
	    v[3] = blake_state[3];
	    v[4] = blake_state[4];
	    v[5] = blake_state[5];
	    v[6] = blake_state[6];
	    v[7] = blake_state[7];
	    v[8] =  blake_iv[0];
	    v[9] =  blake_iv[1];
	    v[10] = blake_iv[2];
	    v[11] = blake_iv[3];
	    v[12] = blake_iv[4];
	    v[13] = blake_iv[5];
	    v[14] = blake_iv[6];
	    v[15] = blake_iv[7];
	    // mix in length of data
        v[12] ^= 128 + buflen + 4 ;
	    // last block
	    v[14] ^= (ulong)-1;

        if( buf[0] == 2 )
        {
            val1 = buf[2] | (ulong)input<<(buflen%8*8);
        }
        else
        {
            val9 = buf[10] | (ulong)input<<(buflen%8*8);
        }

	    // round 1
	    mix(v[0], v[4], v[8],  v[12], buf[ 1+0 ], val1);
	    mix(v[1], v[5], v[9],  v[13], buf[ 1+2 ], buf[ 1+3 ]);
	    mix(v[2], v[6], v[10], v[14], buf[ 1+4 ], buf[ 1+5 ]);
	    mix(v[3], v[7], v[11], v[15], buf[ 1+6 ], buf[ 1+7 ]);
	    mix(v[0], v[5], v[10], v[15], buf[ 1+8 ], val9);
	    mix(v[1], v[6], v[11], v[12], buf[ 1+10 ], buf[ 1+11 ]);
	    mix(v[2], v[7], v[8],  v[13], buf[ 1+12 ], buf[ 1+13 ]);
	    mix(v[3], v[4], v[9],  v[14], buf[ 1+14 ], buf[ 1+15 ]);
	    // round 2
	    mix(v[0], v[4], v[8],  v[12], buf[ 1+14 ], buf[ 1+10 ]);
	    mix(v[1], v[5], v[9],  v[13], buf[ 1+4 ], buf[ 1+8 ]);
	    mix(v[2], v[6], v[10], v[14], val9, buf[ 1+15 ]);
	    mix(v[3], v[7], v[11], v[15], buf[ 1+13 ], buf[ 1+6 ]);
	    mix(v[0], v[5], v[10], v[15], val1, buf[ 1+12 ]);
	    mix(v[1], v[6], v[11], v[12], buf[ 1+0 ], buf[ 1+2 ]);
	    mix(v[2], v[7], v[8],  v[13], buf[ 1+11 ], buf[ 1+7 ]);
	    mix(v[3], v[4], v[9],  v[14], buf[ 1+5 ], buf[ 1+3 ]);
	    // round 3
	    mix(v[0], v[4], v[8],  v[12], buf[ 1+11 ], buf[ 1+8 ]);
	    mix(v[1], v[5], v[9],  v[13], buf[ 1+12 ], buf[ 1+0 ]);
	    mix(v[2], v[6], v[10], v[14], buf[ 1+5 ], buf[ 1+2 ]);
	    mix(v[3], v[7], v[11], v[15], buf[ 1+15 ], buf[ 1+13 ]);
	    mix(v[0], v[5], v[10], v[15], buf[ 1+10 ], buf[ 1+14 ]);
	    mix(v[1], v[6], v[11], v[12], buf[ 1+3 ], buf[ 1+6 ]);
	    mix(v[2], v[7], v[8],  v[13], buf[ 1+7 ], val1);
	    mix(v[3], v[4], v[9],  v[14], val9, buf[ 1+4 ]);
	    // round 4
	    mix(v[0], v[4], v[8],  v[12], buf[ 1+7 ], val9);
	    mix(v[1], v[5], v[9],  v[13], buf[ 1+3 ], val1);
	    mix(v[2], v[6], v[10], v[14], buf[ 1+13 ], buf[ 1+12 ]);
	    mix(v[3], v[7], v[11], v[15], buf[ 1+11 ], buf[ 1+14 ]);
	    mix(v[0], v[5], v[10], v[15], buf[ 1+2 ], buf[ 1+6 ]);
	    mix(v[1], v[6], v[11], v[12], buf[ 1+5 ], buf[ 1+10 ]);
	    mix(v[2], v[7], v[8],  v[13], buf[ 1+4 ], buf[ 1+0 ]);
	    mix(v[3], v[4], v[9],  v[14], buf[ 1+15 ], buf[ 1+8 ]);
	    // round 5
	    mix(v[0], v[4], v[8],  v[12], val9, buf[ 1+0 ]);
	    mix(v[1], v[5], v[9],  v[13], buf[ 1+5 ], buf[ 1+7 ]);
	    mix(v[2], v[6], v[10], v[14], buf[ 1+2 ], buf[ 1+4 ]);
	    mix(v[3], v[7], v[11], v[15], buf[ 1+10 ], buf[ 1+15 ]);
	    mix(v[0], v[5], v[10], v[15], buf[ 1+14 ], val1);
	    mix(v[1], v[6], v[11], v[12], buf[ 1+11 ], buf[ 1+12 ]);
	    mix(v[2], v[7], v[8],  v[13], buf[ 1+6 ], buf[ 1+8 ]);
	    mix(v[3], v[4], v[9],  v[14], buf[ 1+3 ], buf[ 1+13 ]);
	    // round 6
	    mix(v[0], v[4], v[8],  v[12], buf[ 1+2 ], buf[ 1+12 ]);
	    mix(v[1], v[5], v[9],  v[13], buf[ 1+6 ], buf[ 1+10 ]);
	    mix(v[2], v[6], v[10], v[14], buf[ 1+0 ], buf[ 1+11 ]);
	    mix(v[3], v[7], v[11], v[15], buf[ 1+8 ], buf[ 1+3 ]);
	    mix(v[0], v[5], v[10], v[15], buf[ 1+4 ], buf[ 1+13 ]);
	    mix(v[1], v[6], v[11], v[12], buf[ 1+7 ], buf[ 1+5 ]);
	    mix(v[2], v[7], v[8],  v[13], buf[ 1+15 ], buf[ 1+14 ]);
	    mix(v[3], v[4], v[9],  v[14], val1, val9);
	    // round 7
	    mix(v[0], v[4], v[8],  v[12], buf[ 1+12 ], buf[ 1+5 ]);
	    mix(v[1], v[5], v[9],  v[13], val1, buf[ 1+15 ]);
	    mix(v[2], v[6], v[10], v[14], buf[ 1+14 ], buf[ 1+13 ]);
	    mix(v[3], v[7], v[11], v[15], buf[ 1+4 ], buf[ 1+10 ]);
	    mix(v[0], v[5], v[10], v[15], buf[ 1+0 ], buf[ 1+7 ]);
	    mix(v[1], v[6], v[11], v[12], buf[ 1+6 ], buf[ 1+3 ]);
	    mix(v[2], v[7], v[8],  v[13], val9, buf[ 1+2 ]);
	    mix(v[3], v[4], v[9],  v[14], buf[ 1+8 ], buf[ 1+11 ]);
	    // round 8
	    mix(v[0], v[4], v[8],  v[12], buf[ 1+13 ], buf[ 1+11 ]);
	    mix(v[1], v[5], v[9],  v[13], buf[ 1+7 ], buf[ 1+14 ]);
	    mix(v[2], v[6], v[10], v[14], buf[ 1+12 ], val1);
	    mix(v[3], v[7], v[11], v[15], buf[ 1+3 ], val9);
	    mix(v[0], v[5], v[10], v[15], buf[ 1+5 ], buf[ 1+0 ]);
	    mix(v[1], v[6], v[11], v[12], buf[ 1+15 ], buf[ 1+4 ]);
	    mix(v[2], v[7], v[8],  v[13], buf[ 1+8 ], buf[ 1+6 ]);
	    mix(v[3], v[4], v[9],  v[14], buf[ 1+2 ], buf[ 1+10 ]);
	    // round 9
	    mix(v[0], v[4], v[8],  v[12], buf[ 1+6 ], buf[ 1+15 ]);
	    mix(v[1], v[5], v[9],  v[13], buf[ 1+14 ], val9);
	    mix(v[2], v[6], v[10], v[14], buf[ 1+11 ], buf[ 1+3 ]);
	    mix(v[3], v[7], v[11], v[15], buf[ 1+0 ], buf[ 1+8 ]);
	    mix(v[0], v[5], v[10], v[15], buf[ 1+12 ], buf[ 1+2 ]);
	    mix(v[1], v[6], v[11], v[12], buf[ 1+13 ], buf[ 1+7 ]);
	    mix(v[2], v[7], v[8],  v[13], val1, buf[ 1+4 ]);
	    mix(v[3], v[4], v[9],  v[14], buf[ 1+10 ], buf[ 1+5 ]);
	    // round 10
	    mix(v[0], v[4], v[8],  v[12], buf[ 1+10 ], buf[ 1+2 ]);
	    mix(v[1], v[5], v[9],  v[13], buf[ 1+8 ], buf[ 1+4 ]);
	    mix(v[2], v[6], v[10], v[14], buf[ 1+7 ], buf[ 1+6 ]);
	    mix(v[3], v[7], v[11], v[15], val1, buf[ 1+5 ]);
	    mix(v[0], v[5], v[10], v[15], buf[ 1+15 ], buf[ 1+11 ]);
	    mix(v[1], v[6], v[11], v[12], val9, buf[ 1+14 ]);
	    mix(v[2], v[7], v[8],  v[13], buf[ 1+3 ], buf[ 1+12 ]);
	    mix(v[3], v[4], v[9],  v[14], buf[ 1+13 ], buf[ 1+0 ]);
	    // round 11
	    mix(v[0], v[4], v[8],  v[12], buf[ 1+0 ], val1);
	    mix(v[1], v[5], v[9],  v[13], buf[ 1+2 ], buf[ 1+3 ]);
	    mix(v[2], v[6], v[10], v[14], buf[ 1+4 ], buf[ 1+5 ]);
	    mix(v[3], v[7], v[11], v[15], buf[ 1+6 ], buf[ 1+7 ]);
	    mix(v[0], v[5], v[10], v[15], buf[ 1+8 ], val9);
	    mix(v[1], v[6], v[11], v[12], buf[ 1+10 ], buf[ 1+11 ]);
	    mix(v[2], v[7], v[8],  v[13], buf[ 1+12 ], buf[ 1+13 ]);
	    mix(v[3], v[4], v[9],  v[14], buf[ 1+14 ], buf[ 1+15 ]);
	    // round 12
	    mix(v[0], v[4], v[8],  v[12], buf[ 1+14 ], buf[ 1+10 ]);
	    mix(v[1], v[5], v[9],  v[13], buf[ 1+4 ], buf[ 1+8 ]);
	    mix(v[2], v[6], v[10], v[14], val9, buf[ 1+15 ]);
	    mix(v[3], v[7], v[11], v[15], buf[ 1+13 ], buf[ 1+6 ]);
	    mix(v[0], v[5], v[10], v[15], val1, buf[ 1+12 ]);
	    mix(v[1], v[6], v[11], v[12], buf[ 1+0 ], buf[ 1+2 ]);
	    mix(v[2], v[7], v[8],  v[13], buf[ 1+11 ], buf[ 1+7 ]);
	    mix(v[3], v[4], v[9],  v[14], buf[ 1+5 ], buf[ 1+3 ]);

	    // compress v into the blake state; this produces the 50-byte hash
	    // (two Xi values)
#if FAB_HASH_LEN == 46
        ulong h[7];
        h[0] = blake_state[0] ^ v[0] ^ v[8];
        h[1] = blake_state[1] ^ v[1] ^ v[9];
        h[2] = blake_state[2] ^ v[2] ^ v[10];
        h[3] = blake_state[3] ^ v[3] ^ v[11];
        h[4] = blake_state[4] ^ v[4] ^ v[12];

        h[5] = (blake_state[5] ^ v[5] ^ v[13]) & 0xffffffffffff ;
        
        uchar *hash = (uchar *)h;
        for( uint i= 0; i < 2; i++ )
        {
            hash = hash + 23 * i;
            ulong xi0, xi1, xi2;
            uint    row;
            __global char       *p;
            uint                cnt;

            xi0 = *(ulong *)(hash+0);
            xi1 = *(ulong *)(hash+8);
            xi2 = *(ulong *)(hash+16);

#if NR_ROWS_LOG == 21
            row = ((((uint)hash[0] << 8) | hash[1]) << 5) | hash[2] >> 3;
#else
#error "unsupported NR_ROWS_LOG"
#endif

            xi0 = (xi0 >> 16) | (xi1 << (64 - 16));
            xi1 = (xi1 >> 16) | (xi2 << (64 - 16));
            xi2 = (xi2 >> 16);

            p = ht + row * NR_SLOTS * SLOT_LEN;
            uint rowIdx = row/ROWS_PER_UINT;
            uint rowOffset = BITS_PER_ROW*(row%ROWS_PER_UINT);
            uint xcnt = atomic_add(rowCounters + rowIdx, 1 << rowOffset);
            xcnt = (xcnt >> rowOffset) & ROW_MASK;
            cnt = xcnt;
            if (cnt < NR_SLOTS)
            {        
                p += cnt * SLOT_LEN + xi_offset_for_round(0);

                // store "i" (always 4 bytes before Xi)
                *(__global uint *)(p - 4) = (input<<1) + i;

                // store 21 bytes
                *(__global ulong *)(p + 0) = xi0;
                *(__global ulong *)(p + 8) = xi1;
                *(__global ulong *)(p + 16) = xi2 & 0xffffffffff;                
            }
        }

#else
#error "unsupported FAB_HASH_LEN"
#endif

	    input++;
    }
#ifdef ENABLE_DEBUG
    debug[tid * 2] = 0;
    debug[tid * 2 + 1] = dropped;
#endif
}

#if NR_ROWS_LOG == 21 && NR_SLOTS <= (1 << 6)

#define ENCODE_INPUTS(row, slot0, slot1) \
    ((row << 8) | ((slot1 & 0xf) << 4) | (slot0 & 0xf))
#define DECODE_ROW(REF)   (REF >> 8)
#define DECODE_SLOT1(REF) ((REF >> 4) & 0xf)
#define DECODE_SLOT0(REF) (REF & 0xf)

#else
#error "unsupported NR_ROWS_LOG"
#endif

/*
** XOR a pair of Xi values computed at "round - 1" and store the result in the
** hash table being built for "round". Note that when building the table for
** even rounds we need to skip 1 padding byte present in the "round - 1" table
** (the "0xAB" byte mentioned in the description at the top of this file.) But
** also note we can't load data directly past this byte because this would
** cause an unaligned memory access which is undefined per the OpenCL spec.
**
** Return 0 if successfully stored, or 1 if the row overflowed.
*/

uint getrow(uint round, __global ulong *a, __global ulong *b)
{
    uint row;

    __global uchar *p1 = (__global uchar *)a;
    __global uchar *p2 = (__global uchar *)b;

    switch( round )
    {
    case 1:
        row = ((uint)(p1[0] ^ p2[0]) & 0x1) << 20
            | ((uint)(p1[0+1] ^ p2[0+1])) << 12
            | ((uint)(p1[0+2] ^ p2[0+2])) << 4
            | (p1[0+3] ^ p2[0+3]) >> 4;
        break;
    case 2:
        row = ((uint)(p1[0] ^ p2[0]) & 0x3) << 19
            | ((uint)(p1[0+1] ^ p2[0+1])) << 11
            | ((uint)(p1[0+2] ^ p2[0+2])) << 3
            | (p1[0+3] ^ p2[0+3]) >> 5;
        break;
    case 3:
        row = ((uint)(p1[0] ^ p2[0]) & 0x7) << 18
            | ((uint)(p1[0+1] ^ p2[0+1])) << 10
            | ((uint)(p1[0+2] ^ p2[0+2])) << 2
            | (p1[0+3] ^ p2[0+3]) >> 6;
        break;
    case 4:
        row = ((uint)(p1[0] ^ p2[0]) & 0xf) << 17
            | ((uint)(p1[0+1] ^ p2[0+1])) << 9
            | ((uint)(p1[0+2] ^ p2[0+2])) << 1
            | (p1[0+3] ^ p2[0+3]) >> 7;
        break;
    case 5:
        row = ((uint)(p1[0] ^ p2[0]) & 0x1f ) << 16
            | ((uint)(p1[1] ^ p2[1])) << 8
            | (p1[2] ^ p2[2]);
        break;
    case 6:
        row = (((uint)(p1[0] ^ p2[0]) & 0x3f ) << 15 )
            | ((uint)(p1[1] ^ p2[1])) << 7
            | (p1[2] ^ p2[2]) >> 1;
        break;
    default:
        break;
    }

    return row;
}


uint ht_store(uint round, __global char *ht_src, __global char *ht, uint tree, __global uint *rowCounters)
{
    ulong xi0, xi1, xi2, xi3 = 0;
    uint    row;
    __global char      *p;
    uint                cnt;
    __global ulong	*a8, *b8;
    __global uint	*a4, *b4;

    uint collisionThreadId = DECODE_ROW(tree);
    uint slot_a = DECODE_SLOT0(tree);
    uint slot_b = DECODE_SLOT1(tree);
    
    __global uchar *ptr = (__global uchar *)ht_src + collisionThreadId * NR_SLOTS * SLOT_LEN + xi_offset_for_round(round - 1);

    a8 = (__global ulong *)(ptr + slot_a * SLOT_LEN);
    b8 = (__global ulong *)(ptr + slot_b * SLOT_LEN);
    a4 = (__global uint *)(ptr + slot_a * SLOT_LEN);
    b4 = (__global uint *)(ptr + slot_b * SLOT_LEN);

    {
#if NR_ROWS_LOG == 21
        if (round == 1 )
        {
            // xor 21 bytes
            xi0 = a8[0] ^ b8[0];
            xi1 = a8[1] ^ b8[1];
            xi2 = a8[2] ^ b8[2];
        }
        else if (round == 2)
        {
            // xor 18 bytes
            xi0 = a8[0] ^ b8[0];
            xi1 = a8[1] ^ b8[1];
            xi2 = a8[2] ^ b8[2];
        }
        else if (round == 3)
        {
            // xor 15 bytes
            xi0 = (a4[0] ^ b4[0]) | ((ulong)(a4[1] ^ b4[1]) << 32);
            xi1 = (a4[2] ^ b4[2]) | ((ulong)((a4[3] ^ b4[3]) & 0xffffff ) << 32);
            xi2 = 0;
        }
        else if (round == 4 )
        {         
            // xor 12 bytes
            xi0 = (a4[0] ^ b4[0]) | ((ulong)(a4[1] ^ b4[1]) << 32);
            xi1 = (a4[2] ^ b4[2]) ;
            xi2 = 0;
        }
        else if (round == 5 )
        {
            // xor 9 bytes
            xi0 = a8[0] ^ b8[0];
            xi1 = a8[1] ^ b8[1];
            xi2 = 0;
        }
        else if (round == 6)
        {
            // xor 6 bytes
            xi0 = a8[0] ^ b8[0];
            xi1 = 0;
            xi2 = 0;
        }

        if (!xi0 && !xi1)
            return 0;
#else
#error "unsupported NR_ROWS_LOG"
#endif
		if( round == 6 )
		{
			xi0 = (xi0 >> 16) | (xi1 << (64 - 16));
			xi1 = (xi1 >> 16) | (xi2 << (64 - 16));
			xi2 = (xi2 >> 16) ;
		}
		else
		{
			xi0 = (xi0 >> 24) | (xi1 << (64 - 24));
			xi1 = (xi1 >> 24) | (xi2 << (64 - 24));
			xi2 = (xi2 >> 24) ;	
		}
    }

    row = getrow(round, a8, b8);

    p = ht + row * NR_SLOTS * SLOT_LEN;
    uint rowIdx = row/ROWS_PER_UINT;
    uint rowOffset = BITS_PER_ROW*(row%ROWS_PER_UINT);
    uint xcnt = atomic_add(rowCounters + rowIdx, 1 << rowOffset);
    xcnt = (xcnt >> rowOffset) & ROW_MASK;
    cnt = xcnt;
    if (cnt >= NR_SLOTS)
    {
        // avoid overflows
        atomic_sub(rowCounters + rowIdx, 1 << rowOffset);
        return 1;
    }

    p += cnt * SLOT_LEN + xi_offset_for_round(round);

    // store "i" (always 4 bytes before Xi)
    *(__global uint *)(p - 4) = tree;
    if (round == 1)
    {
        // store 18 bytes
        *(__global ulong *)(p + 0) = xi0;
        *(__global ulong *)(p + 8) = xi1;
        *(__global ulong *)(p + 16) = xi2 & 0xffff;
    }
    else if (round == 2)
    {
        // store 15 bytes
        *(__global uint *)(p + 0) = xi0;
        *(__global ulong *)(p + 4) = (xi0 >> 32) | (xi1 << 32);
        *(__global uint *)(p + 12) = ((xi1 >> 32) & 0xffffff ); 
    }
    else if (round == 3)
    {
        // store 12 bytes
        *(__global uint *)(p + 0) = xi0 & 0xffffffff;
        *(__global ulong *)(p + 4) = (xi0 >> 32) | (xi1 << 32);
    }
    else if (round == 4)
    {
        // store 9 bytes
        *(__global ulong *)(p + 0) = xi0;
        *(__global ulong *)(p + 8) = xi1 & 0xff;
    }
    else if (round == 5)
    {
        // store 6 bytes
        *(__global ulong *)(p + 0) = xi0 & 0xffffffffffff;
    }
    else if (round == 6 )
    {
        // store 4 bytes
        *(__global uint *)(p + 0) = xi0 & 0xffffffff;
    }

    return 0;
}

/*
** Execute one Equihash round. Read from ht_src, XOR colliding pairs of Xi,
** store them in ht_dst.
*/
void equihash_round(uint round,
	__global char *ht_src,
	__global char *ht_dst,
	__global uint *debug,
    __local uchar *first_words_data,
	__local uint *collisionsData,
	__local uint *collisionsNum,
	__global uint *rowCountersSrc,
	__global uint *rowCountersDst)
{
    uint		tid = get_global_id(0);
    uint		tlid = get_local_id(0);
    __global uchar	*p;
    uint		cnt;
    
    __local uchar	*first_words = &first_words_data[(NR_SLOTS+2)*tlid];
    
    uchar		mask;
    uint		i, j;
    // NR_SLOTS is already oversized (by a factor of OVERHEAD), but we want to
    // make it even larger
    uint		n;
    uint		dropped_coll = 0;
    uint		dropped_stor = 0;
    __global ulong	*a, *b;
    uint		xi_offset;

    // read first words of Xi from the previous (round - 1) hash table
    xi_offset = xi_offset_for_round(round - 1);

    uint thCollNum = 0;
    *collisionsNum = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
    p = (ht_src + tid * NR_SLOTS * SLOT_LEN);
    uint rowIdx = tid/ROWS_PER_UINT;
    uint rowOffset = BITS_PER_ROW*(tid%ROWS_PER_UINT);
    cnt = (rowCountersSrc[rowIdx] >> rowOffset) & ROW_MASK;
    cnt = min(cnt, (uint)NR_SLOTS); // handle possible overflow in prev. round   

    if (!cnt)
    {
        // no elements in row, no collisions
        goto part2;
    }

    p += xi_offset;

    {
        for (i = 0; i < cnt; i++, p += SLOT_LEN)
        {
            switch(round)
            {
            case 1:
                first_words[i]  = (p[0] & 0x07 ) >> 1;
                break;
            case 2:
                first_words[i]  = (p[0] & 0x0f ) >> 2;
                break;
            case 3:
                first_words[i]  = (p[0] & 0x1f ) >> 3;
                break;
            case 4:
                first_words[i]  = (p[0] & 0x3f ) >> 4;
                break;
            case 5:
                first_words[i]  = (p[0] & 0x7f ) >> 5;
                break;
            case 6:
                first_words[i]  = p[0] >> 6;
                break;
            case 7:
                first_words[i]  = (( p[0] & 0x1 ) << 1 )| ( p[1] >> 7 );
                break;
            }
        }
    }

    // find collisions
    for (i = 0; i < cnt-1 && thCollNum < COLL_DATA_SIZE_PER_TH; i++)
    {
        uint collision = (tid << 8) | (i << 4) | (i + 1);

        for (j = i+1; (j+4) < cnt;)
        {
            {
                if( first_words[i] == first_words[j] )
                {
                    {
                        thCollNum++;
                        uint index = atomic_inc(collisionsNum);
                        collisionsData[index] = collision;
                    }
                }
                collision++;
                j++;
            }

            {
                if (first_words[i] == first_words[j])
                {
                    thCollNum++;
                    uint index = atomic_inc(collisionsNum);
                    collisionsData[index] = collision;
                }
                collision++;
                j++;
            }
     
            {
                if (first_words[i] == first_words[j])
                {
                    thCollNum++;
                    uint index = atomic_inc(collisionsNum);
                    collisionsData[index] = collision;
                }
                collision++;
                j++;
            }

            {
                if (first_words[i] == first_words[j])
                {
                    thCollNum++;
                    uint index = atomic_inc(collisionsNum);
                    collisionsData[index] = collision;
                }
                collision++;
                j++;
            }
        }

        for (; j < cnt; j++)
        {
            if (first_words[i] == first_words[j])
            {
                thCollNum++;
                uint index = atomic_inc(collisionsNum);
                collisionsData[index] = collision;
            }
            collision++;
        }
    }

part2:
    barrier(CLK_LOCAL_MEM_FENCE);

    uint totalCollisions = *collisionsNum;
    for (uint index = tlid; index < totalCollisions; index += get_local_size(0))
    {
        dropped_stor += ht_store(round,  ht_src, ht_dst, collisionsData[index], rowCountersDst);
    }
#ifdef ENABLE_DEBUG
    debug[tid * 2] = dropped_coll;
    debug[tid * 2 + 1] = dropped_stor;
#endif
}

/*
** This defines kernel_round1, kernel_round2, ..., kernel_round7.
*/
#define KERNEL_ROUND(N) \
__kernel __attribute__((reqd_work_group_size(64, 1, 1))) \
void kernel_round ## N(__global char *ht_src, __global char *ht_dst, \
	__global uint *rowCountersSrc, __global uint *rowCountersDst, \
       	__global uint *debug) \
{ \
    __local uchar first_words_data[(NR_SLOTS+2)*64]; \
    __local uint    collisionsData[COLL_DATA_SIZE_PER_TH * 64]; \
    __local uint    collisionsNum; \
    equihash_round(N, ht_src, ht_dst, debug, first_words_data, collisionsData, \
	    &collisionsNum, rowCountersSrc, rowCountersDst); \
}
KERNEL_ROUND(1)
KERNEL_ROUND(2)
KERNEL_ROUND(3)
KERNEL_ROUND(4)
KERNEL_ROUND(5)

// kernel_round6 takes an extra argument, "sols"
__kernel __attribute__((reqd_work_group_size(64, 1, 1)))
void kernel_round6(__global char *ht_src, __global char *ht_dst,
	__global uint *rowCountersSrc, __global uint *rowCountersDst,
	__global uint *debug, __global sols_t *sols)
{
    uint		tid = get_global_id(0);

    __local uchar first_words_data[(NR_SLOTS+2)*64];
    __local uint	collisionsData[COLL_DATA_SIZE_PER_TH * 64];
    __local uint	collisionsNum;

    equihash_round(6, ht_src, ht_dst, debug, first_words_data, collisionsData,
	    &collisionsNum, rowCountersSrc, rowCountersDst);

    if (!tid)
        sols->nr = sols->likely_invalids = 0;
}

uint expand_ref(__global char *ht, uint xi_offset, uint row, uint slot)
{
    return *(__global uint *)(ht + row * NR_SLOTS * SLOT_LEN +
	    slot * SLOT_LEN + xi_offset - 4);
}

/*
** Expand references to inputs. Return 1 if so far the solution appears valid,
** or 0 otherwise (an invalid solution would be a solution with duplicate
** inputs, which can be detected at the last step: round == 0).
*/
uint expand_refs(uint *ins, uint nr_inputs, __global char **htabs,	uint round)
{
    __global char	*ht = htabs[round % 2];
    uint		i = nr_inputs - 1;
    uint		j = nr_inputs * 2 - 1;
    uint		xi_offset = xi_offset_for_round(round);
    int			dup_to_watch = -1;
    
    do
    {
        ins[j] = expand_ref(ht, xi_offset,
            DECODE_ROW(ins[i]), DECODE_SLOT1(ins[i]));
        ins[j - 1] = expand_ref(ht, xi_offset,
            DECODE_ROW(ins[i]), DECODE_SLOT0(ins[i]));
        
        if (!round)
        {
            if (dup_to_watch == -1)
                dup_to_watch = ins[j];
            else if (ins[j] == dup_to_watch || ins[j - 1] == dup_to_watch)
                return 0;
        }

        if (!i)
            break ;
        i--;
        j -= 2;
    }while (1);

    return 1;
}

/*
** Verify if a potential solution is in fact valid.
*/
void potential_sol(__global char **htabs, __global sols_t *sols,
	uint ref0, uint ref1)
{
    uint	nr_values;
    uint	values_tmp[(1 << PARAM_K)];
    uint	sol_i;
    uint	i;
    nr_values = 0;
    values_tmp[nr_values++] = ref0;
    values_tmp[nr_values++] = ref1;
    uint round = PARAM_K - 1;
    
    do
    {
        round--;
        if (!expand_refs(values_tmp, nr_values, htabs, round))
            return ;
        nr_values *= 2;
    }while (round > 0);

    // solution appears valid, copy it to sols
    sol_i = atomic_inc(&sols->nr);
    if (sol_i >= MAX_SOLS)
        return ;
    
    for (i = 0; i < (1 << PARAM_K); i++)
        sols->values[sol_i][i] = values_tmp[i];
    sols->valid[sol_i] = 1;
}

/*
** Scan the hash tables to find Equihash solutions.
*/
__kernel __attribute__((reqd_work_group_size(64, 1, 1)))
void kernel_sols(__global char *ht0, __global char *ht1, __global sols_t *sols,
	__global uint *rowCountersSrc, __global uint *rowCountersDst)
{
    uint		tid = get_global_id(0);

    __global char	*htabs[2] = { ht0, ht1 };
    __global char	*hcounters[2] = { rowCountersSrc, rowCountersDst };
    uint		ht_i = (PARAM_K - 1) % 2; // table filled at last round
    uint		cnt;
    uint		xi_offset = xi_offset_for_round(PARAM_K - 1);
    uint		i, j;
    __global char	*a, *b;
    uint		ref_i, ref_j;

    // it's ok for the collisions array to be so small, as if it fills up
    // the potential solutions are likely invalid (many duplicate inputs)
    ulong		collisions;
    uint		coll;

#if NR_ROWS_LOG == 21
    // in the final hash table, we are looking for a match on both the bits
    // part of the previous PREFIX colliding bits, and the last PREFIX bits.
    uint		mask = 0xffffffff;
#else
#error "unsupported NR_ROWS_LOG"
#endif

    a = htabs[ht_i] + tid * NR_SLOTS * SLOT_LEN;
    uint rowIdx = tid/ROWS_PER_UINT;
    uint rowOffset = BITS_PER_ROW*(tid%ROWS_PER_UINT);
    cnt = (rowCountersSrc[rowIdx] >> rowOffset) & ROW_MASK;
    cnt = min(cnt, (uint)NR_SLOTS); // handle possible overflow in last round
    coll = 0;
    a += xi_offset;

    for (i = 0; i < cnt; i++, a += SLOT_LEN)
    {
        uint a_data = ((*(__global uint *)a) & mask);
        ref_i = *(__global uint *)(a - 4);

        for (j = i + 1, b = a + SLOT_LEN; j < cnt; j++, b += SLOT_LEN)
        {
            if (a_data == ((*(__global uint *)b) & mask))
            {
                ref_j = *(__global uint *)(b - 4);
                collisions = ((ulong)ref_i << 32) | ref_j;
                goto exit1;
            }
        }
    }
    return;

exit1:    
    potential_sol(htabs, sols, collisions >> 32, collisions & 0xffffffff);    
}
