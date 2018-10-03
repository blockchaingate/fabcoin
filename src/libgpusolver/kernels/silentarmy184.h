const unsigned char CL_MINER_KERNEL184[] = R"_mrb_(
# 1 "input.cl"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 1 "<command-line>" 2
# 1 "input.cl"
# 58 "input.cl"
typedef struct sols_s
{
    uint nr;
    uint likely_invalids;
    uchar valid[10];
    uint values[10][(1 << 9)];
}sols_t;


#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
# 102 "input.cl"
__constant ulong blake_iv[] =
{
    0x6a09e667f3bcc908, 0xbb67ae8584caa73b,
    0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
    0x510e527fade682d1, 0x9b05688c2b3e6c1f,
    0x1f83d9abfb41bd6b, 0x5be0cd19137e2179,
};




__kernel
void kernel_init_ht(__global char *ht, __global uint *rowCounters)
{
    rowCounters[get_global_id(0)] = 0;
}
# 166 "input.cl"
__kernel __attribute__((reqd_work_group_size(64, 1, 1)))
void kernel_round0(__global ulong *blake_state, __global char *ht, __global ulong *buf,
                   __global uint *rowCounters, __global uint *debug)
{
    uint tid = get_global_id(0);
    ulong v[16];
    uint inputs_per_thread = (1 << (184 / (7 + 1))) / get_global_size(0);
    uint input = tid * inputs_per_thread;
    uint input_end = (tid + 1) * inputs_per_thread;
    uint dropped = 0;
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

     v[0] = blake_state[0];
     v[1] = blake_state[1];
     v[2] = blake_state[2];
     v[3] = blake_state[3];
     v[4] = blake_state[4];
     v[5] = blake_state[5];
     v[6] = blake_state[6];
     v[7] = blake_state[7];
     v[8] = blake_iv[0];
     v[9] = blake_iv[1];
     v[10] = blake_iv[2];
     v[11] = blake_iv[3];
     v[12] = blake_iv[4];
     v[13] = blake_iv[5];
     v[14] = blake_iv[6];
     v[15] = blake_iv[7];

        v[12] ^= 128 + buflen + 4 ;

     v[14] ^= (ulong)-1;

        if( buf[0] == 2 )
        {
            val1 = buf[2] | (ulong)input<<(buflen%8*8);
        }
        else
        {
            val9 = buf[10] | (ulong)input<<(buflen%8*8);
        }


     v[0] = (v[0] + v[4] + buf[ 1+0 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 32); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 24); v[0] = (v[0] + v[4] + val1); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 16); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 63);;
     v[1] = (v[1] + v[5] + buf[ 1+2 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 32); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 24); v[1] = (v[1] + v[5] + buf[ 1+3 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 16); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 63);;
     v[2] = (v[2] + v[6] + buf[ 1+4 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 32); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 24); v[2] = (v[2] + v[6] + buf[ 1+5 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 16); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 63);;
     v[3] = (v[3] + v[7] + buf[ 1+6 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 32); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 24); v[3] = (v[3] + v[7] + buf[ 1+7 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 16); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 63);;
     v[0] = (v[0] + v[5] + buf[ 1+8 ]); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 32); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 24); v[0] = (v[0] + v[5] + val9); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 16); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 63);;
     v[1] = (v[1] + v[6] + buf[ 1+10 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 32); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 24); v[1] = (v[1] + v[6] + buf[ 1+11 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 16); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 63);;
     v[2] = (v[2] + v[7] + buf[ 1+12 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 32); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 24); v[2] = (v[2] + v[7] + buf[ 1+13 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 16); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 63);;
     v[3] = (v[3] + v[4] + buf[ 1+14 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 32); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 24); v[3] = (v[3] + v[4] + buf[ 1+15 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 16); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 63);;

     v[0] = (v[0] + v[4] + buf[ 1+14 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 32); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 24); v[0] = (v[0] + v[4] + buf[ 1+10 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 16); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 63);;
     v[1] = (v[1] + v[5] + buf[ 1+4 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 32); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 24); v[1] = (v[1] + v[5] + buf[ 1+8 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 16); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 63);;
     v[2] = (v[2] + v[6] + val9); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 32); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 24); v[2] = (v[2] + v[6] + buf[ 1+15 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 16); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 63);;
     v[3] = (v[3] + v[7] + buf[ 1+13 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 32); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 24); v[3] = (v[3] + v[7] + buf[ 1+6 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 16); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 63);;
     v[0] = (v[0] + v[5] + val1); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 32); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 24); v[0] = (v[0] + v[5] + buf[ 1+12 ]); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 16); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 63);;
     v[1] = (v[1] + v[6] + buf[ 1+0 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 32); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 24); v[1] = (v[1] + v[6] + buf[ 1+2 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 16); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 63);;
     v[2] = (v[2] + v[7] + buf[ 1+11 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 32); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 24); v[2] = (v[2] + v[7] + buf[ 1+7 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 16); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 63);;
     v[3] = (v[3] + v[4] + buf[ 1+5 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 32); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 24); v[3] = (v[3] + v[4] + buf[ 1+3 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 16); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 63);;

     v[0] = (v[0] + v[4] + buf[ 1+11 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 32); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 24); v[0] = (v[0] + v[4] + buf[ 1+8 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 16); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 63);;
     v[1] = (v[1] + v[5] + buf[ 1+12 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 32); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 24); v[1] = (v[1] + v[5] + buf[ 1+0 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 16); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 63);;
     v[2] = (v[2] + v[6] + buf[ 1+5 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 32); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 24); v[2] = (v[2] + v[6] + buf[ 1+2 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 16); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 63);;
     v[3] = (v[3] + v[7] + buf[ 1+15 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 32); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 24); v[3] = (v[3] + v[7] + buf[ 1+13 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 16); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 63);;
     v[0] = (v[0] + v[5] + buf[ 1+10 ]); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 32); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 24); v[0] = (v[0] + v[5] + buf[ 1+14 ]); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 16); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 63);;
     v[1] = (v[1] + v[6] + buf[ 1+3 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 32); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 24); v[1] = (v[1] + v[6] + buf[ 1+6 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 16); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 63);;
     v[2] = (v[2] + v[7] + buf[ 1+7 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 32); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 24); v[2] = (v[2] + v[7] + val1); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 16); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 63);;
     v[3] = (v[3] + v[4] + val9); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 32); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 24); v[3] = (v[3] + v[4] + buf[ 1+4 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 16); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 63);;

     v[0] = (v[0] + v[4] + buf[ 1+7 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 32); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 24); v[0] = (v[0] + v[4] + val9); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 16); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 63);;
     v[1] = (v[1] + v[5] + buf[ 1+3 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 32); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 24); v[1] = (v[1] + v[5] + val1); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 16); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 63);;
     v[2] = (v[2] + v[6] + buf[ 1+13 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 32); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 24); v[2] = (v[2] + v[6] + buf[ 1+12 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 16); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 63);;
     v[3] = (v[3] + v[7] + buf[ 1+11 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 32); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 24); v[3] = (v[3] + v[7] + buf[ 1+14 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 16); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 63);;
     v[0] = (v[0] + v[5] + buf[ 1+2 ]); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 32); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 24); v[0] = (v[0] + v[5] + buf[ 1+6 ]); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 16); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 63);;
     v[1] = (v[1] + v[6] + buf[ 1+5 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 32); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 24); v[1] = (v[1] + v[6] + buf[ 1+10 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 16); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 63);;
     v[2] = (v[2] + v[7] + buf[ 1+4 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 32); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 24); v[2] = (v[2] + v[7] + buf[ 1+0 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 16); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 63);;
     v[3] = (v[3] + v[4] + buf[ 1+15 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 32); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 24); v[3] = (v[3] + v[4] + buf[ 1+8 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 16); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 63);;

     v[0] = (v[0] + v[4] + val9); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 32); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 24); v[0] = (v[0] + v[4] + buf[ 1+0 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 16); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 63);;
     v[1] = (v[1] + v[5] + buf[ 1+5 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 32); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 24); v[1] = (v[1] + v[5] + buf[ 1+7 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 16); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 63);;
     v[2] = (v[2] + v[6] + buf[ 1+2 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 32); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 24); v[2] = (v[2] + v[6] + buf[ 1+4 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 16); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 63);;
     v[3] = (v[3] + v[7] + buf[ 1+10 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 32); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 24); v[3] = (v[3] + v[7] + buf[ 1+15 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 16); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 63);;
     v[0] = (v[0] + v[5] + buf[ 1+14 ]); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 32); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 24); v[0] = (v[0] + v[5] + val1); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 16); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 63);;
     v[1] = (v[1] + v[6] + buf[ 1+11 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 32); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 24); v[1] = (v[1] + v[6] + buf[ 1+12 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 16); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 63);;
     v[2] = (v[2] + v[7] + buf[ 1+6 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 32); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 24); v[2] = (v[2] + v[7] + buf[ 1+8 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 16); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 63);;
     v[3] = (v[3] + v[4] + buf[ 1+3 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 32); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 24); v[3] = (v[3] + v[4] + buf[ 1+13 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 16); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 63);;

     v[0] = (v[0] + v[4] + buf[ 1+2 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 32); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 24); v[0] = (v[0] + v[4] + buf[ 1+12 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 16); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 63);;
     v[1] = (v[1] + v[5] + buf[ 1+6 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 32); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 24); v[1] = (v[1] + v[5] + buf[ 1+10 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 16); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 63);;
     v[2] = (v[2] + v[6] + buf[ 1+0 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 32); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 24); v[2] = (v[2] + v[6] + buf[ 1+11 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 16); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 63);;
     v[3] = (v[3] + v[7] + buf[ 1+8 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 32); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 24); v[3] = (v[3] + v[7] + buf[ 1+3 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 16); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 63);;
     v[0] = (v[0] + v[5] + buf[ 1+4 ]); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 32); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 24); v[0] = (v[0] + v[5] + buf[ 1+13 ]); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 16); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 63);;
     v[1] = (v[1] + v[6] + buf[ 1+7 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 32); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 24); v[1] = (v[1] + v[6] + buf[ 1+5 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 16); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 63);;
     v[2] = (v[2] + v[7] + buf[ 1+15 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 32); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 24); v[2] = (v[2] + v[7] + buf[ 1+14 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 16); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 63);;
     v[3] = (v[3] + v[4] + val1); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 32); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 24); v[3] = (v[3] + v[4] + val9); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 16); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 63);;

     v[0] = (v[0] + v[4] + buf[ 1+12 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 32); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 24); v[0] = (v[0] + v[4] + buf[ 1+5 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 16); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 63);;
     v[1] = (v[1] + v[5] + val1); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 32); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 24); v[1] = (v[1] + v[5] + buf[ 1+15 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 16); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 63);;
     v[2] = (v[2] + v[6] + buf[ 1+14 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 32); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 24); v[2] = (v[2] + v[6] + buf[ 1+13 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 16); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 63);;
     v[3] = (v[3] + v[7] + buf[ 1+4 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 32); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 24); v[3] = (v[3] + v[7] + buf[ 1+10 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 16); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 63);;
     v[0] = (v[0] + v[5] + buf[ 1+0 ]); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 32); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 24); v[0] = (v[0] + v[5] + buf[ 1+7 ]); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 16); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 63);;
     v[1] = (v[1] + v[6] + buf[ 1+6 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 32); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 24); v[1] = (v[1] + v[6] + buf[ 1+3 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 16); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 63);;
     v[2] = (v[2] + v[7] + val9); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 32); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 24); v[2] = (v[2] + v[7] + buf[ 1+2 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 16); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 63);;
     v[3] = (v[3] + v[4] + buf[ 1+8 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 32); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 24); v[3] = (v[3] + v[4] + buf[ 1+11 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 16); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 63);;

     v[0] = (v[0] + v[4] + buf[ 1+13 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 32); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 24); v[0] = (v[0] + v[4] + buf[ 1+11 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 16); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 63);;
     v[1] = (v[1] + v[5] + buf[ 1+7 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 32); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 24); v[1] = (v[1] + v[5] + buf[ 1+14 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 16); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 63);;
     v[2] = (v[2] + v[6] + buf[ 1+12 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 32); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 24); v[2] = (v[2] + v[6] + val1); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 16); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 63);;
     v[3] = (v[3] + v[7] + buf[ 1+3 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 32); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 24); v[3] = (v[3] + v[7] + val9); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 16); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 63);;
     v[0] = (v[0] + v[5] + buf[ 1+5 ]); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 32); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 24); v[0] = (v[0] + v[5] + buf[ 1+0 ]); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 16); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 63);;
     v[1] = (v[1] + v[6] + buf[ 1+15 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 32); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 24); v[1] = (v[1] + v[6] + buf[ 1+4 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 16); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 63);;
     v[2] = (v[2] + v[7] + buf[ 1+8 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 32); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 24); v[2] = (v[2] + v[7] + buf[ 1+6 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 16); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 63);;
     v[3] = (v[3] + v[4] + buf[ 1+2 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 32); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 24); v[3] = (v[3] + v[4] + buf[ 1+10 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 16); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 63);;

     v[0] = (v[0] + v[4] + buf[ 1+6 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 32); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 24); v[0] = (v[0] + v[4] + buf[ 1+15 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 16); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 63);;
     v[1] = (v[1] + v[5] + buf[ 1+14 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 32); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 24); v[1] = (v[1] + v[5] + val9); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 16); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 63);;
     v[2] = (v[2] + v[6] + buf[ 1+11 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 32); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 24); v[2] = (v[2] + v[6] + buf[ 1+3 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 16); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 63);;
     v[3] = (v[3] + v[7] + buf[ 1+0 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 32); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 24); v[3] = (v[3] + v[7] + buf[ 1+8 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 16); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 63);;
     v[0] = (v[0] + v[5] + buf[ 1+12 ]); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 32); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 24); v[0] = (v[0] + v[5] + buf[ 1+2 ]); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 16); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 63);;
     v[1] = (v[1] + v[6] + buf[ 1+13 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 32); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 24); v[1] = (v[1] + v[6] + buf[ 1+7 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 16); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 63);;
     v[2] = (v[2] + v[7] + val1); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 32); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 24); v[2] = (v[2] + v[7] + buf[ 1+4 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 16); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 63);;
     v[3] = (v[3] + v[4] + buf[ 1+10 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 32); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 24); v[3] = (v[3] + v[4] + buf[ 1+5 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 16); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 63);;

     v[0] = (v[0] + v[4] + buf[ 1+10 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 32); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 24); v[0] = (v[0] + v[4] + buf[ 1+2 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 16); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 63);;
     v[1] = (v[1] + v[5] + buf[ 1+8 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 32); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 24); v[1] = (v[1] + v[5] + buf[ 1+4 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 16); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 63);;
     v[2] = (v[2] + v[6] + buf[ 1+7 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 32); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 24); v[2] = (v[2] + v[6] + buf[ 1+6 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 16); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 63);;
     v[3] = (v[3] + v[7] + val1); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 32); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 24); v[3] = (v[3] + v[7] + buf[ 1+5 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 16); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 63);;
     v[0] = (v[0] + v[5] + buf[ 1+15 ]); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 32); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 24); v[0] = (v[0] + v[5] + buf[ 1+11 ]); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 16); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 63);;
     v[1] = (v[1] + v[6] + val9); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 32); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 24); v[1] = (v[1] + v[6] + buf[ 1+14 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 16); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 63);;
     v[2] = (v[2] + v[7] + buf[ 1+3 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 32); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 24); v[2] = (v[2] + v[7] + buf[ 1+12 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 16); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 63);;
     v[3] = (v[3] + v[4] + buf[ 1+13 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 32); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 24); v[3] = (v[3] + v[4] + buf[ 1+0 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 16); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 63);;

     v[0] = (v[0] + v[4] + buf[ 1+0 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 32); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 24); v[0] = (v[0] + v[4] + val1); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 16); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 63);;
     v[1] = (v[1] + v[5] + buf[ 1+2 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 32); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 24); v[1] = (v[1] + v[5] + buf[ 1+3 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 16); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 63);;
     v[2] = (v[2] + v[6] + buf[ 1+4 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 32); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 24); v[2] = (v[2] + v[6] + buf[ 1+5 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 16); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 63);;
     v[3] = (v[3] + v[7] + buf[ 1+6 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 32); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 24); v[3] = (v[3] + v[7] + buf[ 1+7 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 16); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 63);;
     v[0] = (v[0] + v[5] + buf[ 1+8 ]); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 32); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 24); v[0] = (v[0] + v[5] + val9); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 16); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 63);;
     v[1] = (v[1] + v[6] + buf[ 1+10 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 32); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 24); v[1] = (v[1] + v[6] + buf[ 1+11 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 16); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 63);;
     v[2] = (v[2] + v[7] + buf[ 1+12 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 32); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 24); v[2] = (v[2] + v[7] + buf[ 1+13 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 16); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 63);;
     v[3] = (v[3] + v[4] + buf[ 1+14 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 32); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 24); v[3] = (v[3] + v[4] + buf[ 1+15 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 16); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 63);;

     v[0] = (v[0] + v[4] + buf[ 1+14 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 32); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 24); v[0] = (v[0] + v[4] + buf[ 1+10 ]); v[12] = rotate((v[12] ^ v[0]), (ulong)64 - 16); v[8] = (v[8] + v[12]); v[4] = rotate((v[4] ^ v[8]), (ulong)64 - 63);;
     v[1] = (v[1] + v[5] + buf[ 1+4 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 32); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 24); v[1] = (v[1] + v[5] + buf[ 1+8 ]); v[13] = rotate((v[13] ^ v[1]), (ulong)64 - 16); v[9] = (v[9] + v[13]); v[5] = rotate((v[5] ^ v[9]), (ulong)64 - 63);;
     v[2] = (v[2] + v[6] + val9); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 32); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 24); v[2] = (v[2] + v[6] + buf[ 1+15 ]); v[14] = rotate((v[14] ^ v[2]), (ulong)64 - 16); v[10] = (v[10] + v[14]); v[6] = rotate((v[6] ^ v[10]), (ulong)64 - 63);;
     v[3] = (v[3] + v[7] + buf[ 1+13 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 32); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 24); v[3] = (v[3] + v[7] + buf[ 1+6 ]); v[15] = rotate((v[15] ^ v[3]), (ulong)64 - 16); v[11] = (v[11] + v[15]); v[7] = rotate((v[7] ^ v[11]), (ulong)64 - 63);;
     v[0] = (v[0] + v[5] + val1); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 32); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 24); v[0] = (v[0] + v[5] + buf[ 1+12 ]); v[15] = rotate((v[15] ^ v[0]), (ulong)64 - 16); v[10] = (v[10] + v[15]); v[5] = rotate((v[5] ^ v[10]), (ulong)64 - 63);;
     v[1] = (v[1] + v[6] + buf[ 1+0 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 32); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 24); v[1] = (v[1] + v[6] + buf[ 1+2 ]); v[12] = rotate((v[12] ^ v[1]), (ulong)64 - 16); v[11] = (v[11] + v[12]); v[6] = rotate((v[6] ^ v[11]), (ulong)64 - 63);;
     v[2] = (v[2] + v[7] + buf[ 1+11 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 32); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 24); v[2] = (v[2] + v[7] + buf[ 1+7 ]); v[13] = rotate((v[13] ^ v[2]), (ulong)64 - 16); v[8] = (v[8] + v[13]); v[7] = rotate((v[7] ^ v[8]), (ulong)64 - 63);;
     v[3] = (v[3] + v[4] + buf[ 1+5 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 32); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 24); v[3] = (v[3] + v[4] + buf[ 1+3 ]); v[14] = rotate((v[14] ^ v[3]), (ulong)64 - 16); v[9] = (v[9] + v[14]); v[4] = rotate((v[4] ^ v[9]), (ulong)64 - 63);;




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
            uint row;
            __global char *p;
            uint cnt;

            xi0 = *(ulong *)(hash+0);
            xi1 = *(ulong *)(hash+8);
            xi2 = *(ulong *)(hash+16);


            row = ((((uint)hash[0] << 8) | hash[1]) << 5) | hash[2] >> 3;




            xi0 = (xi0 >> 16) | (xi1 << (64 - 16));
            xi1 = (xi1 >> 16) | (xi2 << (64 - 16));
            xi2 = (xi2 >> 16);

            p = ht + row * (( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2) * 32;
            uint rowIdx = row/4;
            uint rowOffset = 8*(row%4);
            uint xcnt = atomic_add(rowCounters + rowIdx, 1 << rowOffset);
            xcnt = (xcnt >> rowOffset) & 0xFF;
            cnt = xcnt;
            if (cnt < (( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2))
            {
                p += cnt * 32 + (8 + ((0) / 2) * 4);


                *(__global uint *)(p - 4) = (input<<1) + i;


                *(__global ulong *)(p + 0) = xi0;
                *(__global ulong *)(p + 8) = xi1;
                *(__global ulong *)(p + 16) = xi2 & 0xffffffffff;
            }
        }





     input++;
    }




}
# 421 "input.cl"
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
    uint row;
    __global uchar *p;
    uint cnt;
    __global ulong *a8, *b8;
    __global uint *a4, *b4;

    uint collisionThreadId = (tree >> 8);
    uint slot_a = (tree & 0xf);
    uint slot_b = ((tree >> 4) & 0xf);

    __global uchar *ptr = (__global uchar *)ht_src + collisionThreadId * (( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2) * 32 + (8 + ((round - 1) / 2) * 4);

    a8 = (__global ulong *)(ptr + slot_a * 32);
    b8 = (__global ulong *)(ptr + slot_b * 32);
    a4 = (__global uint *)(ptr + slot_a * 32);
    b4 = (__global uint *)(ptr + slot_b * 32);

    {

        if (round == 1 )
        {

            xi0 = a8[0] ^ b8[0];
            xi1 = a8[1] ^ b8[1];
            xi2 = a8[2] ^ b8[2];
        }
        else if (round == 2)
        {

            xi0 = a8[0] ^ b8[0];
            xi1 = a8[1] ^ b8[1];
            xi2 = a8[2] ^ b8[2];
        }
        else if (round == 3)
        {

            xi0 = (a4[0] ^ b4[0]) | ((ulong)(a4[1] ^ b4[1]) << 32);
            xi1 = (a4[2] ^ b4[2]) | ((ulong)((a4[3] ^ b4[3]) & 0xffffff ) << 32);
            xi2 = 0;
        }
        else if (round == 4 )
        {

            xi0 = (a4[0] ^ b4[0]) | ((ulong)(a4[1] ^ b4[1]) << 32);
            xi1 = (a4[2] ^ b4[2]) ;
            xi2 = 0;
        }
        else if (round == 5 )
        {

            xi0 = a8[0] ^ b8[0];
            xi1 = a8[1] ^ b8[1];
            xi2 = 0;
        }
        else if (round == 6)
        {

            xi0 = a8[0] ^ b8[0];
            xi1 = 0;
            xi2 = 0;
        }

        if (!xi0 && !xi1)
            return 0;



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

    p = ht + row * (( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2) * 32;
    uint rowIdx = row/4;
    uint rowOffset = 8*(row%4);
    uint xcnt = atomic_add(rowCounters + rowIdx, 1 << rowOffset);
    xcnt = (xcnt >> rowOffset) & 0xFF;
    cnt = xcnt;
    if (cnt >= (( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2))
    {

        atomic_sub(rowCounters + rowIdx, 1 << rowOffset);
        return 1;
    }

    p += cnt * 32 + (8 + ((round) / 2) * 4);


    *(__global uint *)(p - 4) = tree;
    if (round == 1)
    {

        *(__global ulong *)(p + 0) = xi0;
        *(__global ulong *)(p + 8) = xi1;
        *(__global ulong *)(p + 16) = xi2 & 0xffff;
    }
    else if (round == 2)
    {

        *(__global uint *)(p + 0) = xi0;
        *(__global ulong *)(p + 4) = (xi0 >> 32) | (xi1 << 32);
        *(__global uint *)(p + 12) = ((xi1 >> 32) & 0xffffff );
    }
    else if (round == 3)
    {

        *(__global uint *)(p + 0) = xi0 & 0xffffffff;
        *(__global ulong *)(p + 4) = (xi0 >> 32) | (xi1 << 32);
    }
    else if (round == 4)
    {

        *(__global ulong *)(p + 0) = xi0;
        *(__global ulong *)(p + 8) = xi1 & 0xff;
    }
    else if (round == 5)
    {

        *(__global ulong *)(p + 0) = xi0 & 0xffffffffffff;
    }
    else if (round == 6 )
    {

        *(__global uint *)(p + 0) = xi0 & 0xffffffff;
    }

    return 0;
}





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
    uint tid = get_global_id(0);
    uint tlid = get_local_id(0);
    __global uchar *p;
    uint cnt;

    __local uchar *first_words = &first_words_data[((( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2)+2)*tlid];

    uchar mask;
    uint i, j;


    uint n;
    uint dropped_coll = 0;
    uint dropped_stor = 0;
    __global ulong *a, *b;
    uint xi_offset;


    xi_offset = (8 + ((round - 1) / 2) * 4);

    uint thCollNum = 0;
    *collisionsNum = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
    p = (ht_src + tid * (( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2) * 32);
    uint rowIdx = tid/4;
    uint rowOffset = 8*(tid%4);
    cnt = (rowCountersSrc[rowIdx] >> rowOffset) & 0xFF;
    cnt = min(cnt, (uint)(( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2));

    if (!cnt)
    {

        goto part2;
    }

    p += xi_offset;

    {
        for (i = 0; i < cnt; i++, p += 32)
        {
            switch(round)
            {
            case 1:
                first_words[i] = (p[0] & 0x07 ) >> 1;
                break;
            case 2:
                first_words[i] = (p[0] & 0x0f ) >> 2;
                break;
            case 3:
                first_words[i] = (p[0] & 0x1f ) >> 3;
                break;
            case 4:
                first_words[i] = (p[0] & 0x3f ) >> 4;
                break;
            case 5:
                first_words[i] = (p[0] & 0x7f ) >> 5;
                break;
            case 6:
                first_words[i] = p[0] >> 6;
                break;
            case 7:
                first_words[i] = (( p[0] & 0x1 ) << 1 )| ( p[1] >> 7 );
                break;
            }
        }
    }


    for (i = 0; i < cnt-1 && thCollNum < ((( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2) * 5); i++)
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
        dropped_stor += ht_store(round, ht_src, ht_dst, collisionsData[index], rowCountersDst);
    }




}
# 791 "input.cl"
__kernel __attribute__((reqd_work_group_size(64, 1, 1))) void kernel_round1(__global char *ht_src, __global char *ht_dst, __global uint *rowCountersSrc, __global uint *rowCountersDst, __global uint *debug) { __local uchar first_words_data[((( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2)+2)*64]; __local uint collisionsData[((( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2) * 5) * 64]; __local uint collisionsNum; equihash_round(1, ht_src, ht_dst, debug, first_words_data, collisionsData, &collisionsNum, rowCountersSrc, rowCountersDst); }
__kernel __attribute__((reqd_work_group_size(64, 1, 1))) void kernel_round2(__global char *ht_src, __global char *ht_dst, __global uint *rowCountersSrc, __global uint *rowCountersDst, __global uint *debug) { __local uchar first_words_data[((( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2)+2)*64]; __local uint collisionsData[((( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2) * 5) * 64]; __local uint collisionsNum; equihash_round(2, ht_src, ht_dst, debug, first_words_data, collisionsData, &collisionsNum, rowCountersSrc, rowCountersDst); }
__kernel __attribute__((reqd_work_group_size(64, 1, 1))) void kernel_round3(__global char *ht_src, __global char *ht_dst, __global uint *rowCountersSrc, __global uint *rowCountersDst, __global uint *debug) { __local uchar first_words_data[((( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2)+2)*64]; __local uint collisionsData[((( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2) * 5) * 64]; __local uint collisionsNum; equihash_round(3, ht_src, ht_dst, debug, first_words_data, collisionsData, &collisionsNum, rowCountersSrc, rowCountersDst); }
__kernel __attribute__((reqd_work_group_size(64, 1, 1))) void kernel_round4(__global char *ht_src, __global char *ht_dst, __global uint *rowCountersSrc, __global uint *rowCountersDst, __global uint *debug) { __local uchar first_words_data[((( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2)+2)*64]; __local uint collisionsData[((( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2) * 5) * 64]; __local uint collisionsNum; equihash_round(4, ht_src, ht_dst, debug, first_words_data, collisionsData, &collisionsNum, rowCountersSrc, rowCountersDst); }
__kernel __attribute__((reqd_work_group_size(64, 1, 1))) void kernel_round5(__global char *ht_src, __global char *ht_dst, __global uint *rowCountersSrc, __global uint *rowCountersDst, __global uint *debug) { __local uchar first_words_data[((( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2)+2)*64]; __local uint collisionsData[((( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2) * 5) * 64]; __local uint collisionsNum; equihash_round(5, ht_src, ht_dst, debug, first_words_data, collisionsData, &collisionsNum, rowCountersSrc, rowCountersDst); }


__kernel __attribute__((reqd_work_group_size(64, 1, 1)))
void kernel_round6(__global char *ht_src, __global char *ht_dst,
 __global uint *rowCountersSrc, __global uint *rowCountersDst,
 __global uint *debug, __global sols_t *sols)
{
    uint tid = get_global_id(0);

    __local uchar first_words_data[((( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2)+2)*64];
    __local uint collisionsData[((( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2) * 5) * 64];
    __local uint collisionsNum;

    equihash_round(6, ht_src, ht_dst, debug, first_words_data, collisionsData,
     &collisionsNum, rowCountersSrc, rowCountersDst);

    if (!tid)
        sols->nr = sols->likely_invalids = 0;
}

uint expand_ref(__global char *ht, uint xi_offset, uint row, uint slot)
{
    return *(__global uint *)(ht + row * (( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2) * 32 +
     slot * 32 + xi_offset - 4);
}






uint expand_refs(uint *ins, uint nr_inputs, __global char **htabs, uint round)
{
    __global char *ht = htabs[round % 2];
    uint i = nr_inputs - 1;
    uint j = nr_inputs * 2 - 1;
    uint xi_offset = (8 + ((round) / 2) * 4);
    int dup_to_watch = -1;

    do
    {
        ins[j] = expand_ref(ht, xi_offset,
            (ins[i] >> 8), ((ins[i] >> 4) & 0xf));
        ins[j - 1] = expand_ref(ht, xi_offset,
            (ins[i] >> 8), (ins[i] & 0xf));

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




void potential_sol(__global char **htabs, __global sols_t *sols,
 uint ref0, uint ref1)
{
    uint nr_values;
    uint values_tmp[(1 << 7)];
    uint sol_i;
    uint i;
    nr_values = 0;
    values_tmp[nr_values++] = ref0;
    values_tmp[nr_values++] = ref1;
    uint round = 7 - 1;

    do
    {
        round--;
        if (!expand_refs(values_tmp, nr_values, htabs, round))
            return ;
        nr_values *= 2;
    }while (round > 0);


    sol_i = atomic_inc(&sols->nr);
    if (sol_i >= 10)
        return ;

    for (i = 0; i < (1 << 7); i++)
        sols->values[sol_i][i] = values_tmp[i];
    sols->valid[sol_i] = 1;
}




__kernel __attribute__((reqd_work_group_size(64, 1, 1)))
void kernel_sols(__global char *ht0, __global char *ht1, __global sols_t *sols,
 __global uint *rowCountersSrc, __global uint *rowCountersDst)
{
    uint tid = get_global_id(0);

    __global char *htabs[2] = { ht0, ht1 };
    __global char *hcounters[2] = { rowCountersSrc, rowCountersDst };
    uint ht_i = (7 - 1) % 2;
    uint cnt;
    uint xi_offset = (8 + ((7 - 1) / 2) * 4);
    uint i, j;
    __global char *a, *b;
    uint ref_i, ref_j;



    ulong collisions;
    uint coll;




    uint mask = 0xffffffff;




    a = htabs[ht_i] + tid * (( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2) * 32;
    uint rowIdx = tid/4;
    uint rowOffset = 8*(tid%4);
    cnt = (rowCountersSrc[rowIdx] >> rowOffset) & 0xFF;
    cnt = min(cnt, (uint)(( 1 << (((184 / (7 + 1)) + 1) - 21)) * 2));
    coll = 0;
    a += xi_offset;

    for (i = 0; i < cnt; i++, a += 32)
    {
        uint a_data = ((*(__global uint *)a) & mask);
        ref_i = *(__global uint *)(a - 4);

        for (j = i + 1, b = a + 32; j < cnt; j++, b += 32)
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
)_mrb_";

const size_t CL_MINER_KERNEL_SIZE184 = strlen((char *)CL_MINER_KERNEL184);
