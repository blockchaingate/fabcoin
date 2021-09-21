#pragma once

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdexcept>
#include <vector>

#include <cuda.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#ifdef WIN32
#define _SNPRINTF _snprintf
#else
#define _SNPRINTF snprintf
#endif

#ifndef nullptr
#define nullptr NULL
#endif


#ifdef WIN32
#define rt_error std::runtime_error
#else
class rt_error : public std::runtime_error
{
public:
    explicit rt_error(const std::string& str) : std::runtime_error(str) {}
};
#endif

#define checkCudaErrors(call)								\
do {														\
	cudaError_t err = call;									\
	if (cudaSuccess != err) {								\
		char errorBuff[512];								\
        _SNPRINTF(errorBuff, sizeof(errorBuff) - 1,			\
			"CUDA error '%s' in func '%s' line %d",			\
			cudaGetErrorString(err), __FUNCTION__, __LINE__);	\
		throw std::runtime_error(errorBuff);				\
		}														\
} while (0)

#define checkCudaDriverErrors(call)							\
    do {														\
    CUresult err = call;									\
    if (CUDA_SUCCESS != err) {								\
    char errorBuff[512];								\
    _SNPRINTF(errorBuff, sizeof(errorBuff) - 1,			\
    "CUDA error DRIVER: '%d' in func '%s' line %d", \
    err, __FUNCTION__, __LINE__);	\
    throw rt_error(errorBuff);				\
    }											\
} while (0)

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef unsigned char uchar;


typedef bool (*fn_cancel)();
typedef bool (*fn_validate)(std::vector<unsigned char>, unsigned char *, int);



// ************************** 200,9 begin ****************************
#define PROOFSIZE1 (1<<9)
typedef u32 proof[PROOFSIZE1];
struct equi200;
struct eq_cuda_context200x9
{
	int threadsperblock;
	int totalblocks;
	int device_id;
    int thread_id;
	equi200* eq;
	equi200* device_eq;
	uint32_t *heap0, *heap1;
	void* sol_memory;
	proof* solutions;

    fn_validate m_fnValidate;
    fn_cancel m_fnCancel;

	eq_cuda_context200x9(int thrid, int devid, fn_validate validate, fn_cancel cancel);
	~eq_cuda_context200x9();

    bool solve(unsigned char *pblock, unsigned char *header, unsigned int headerlen);
};
// ************************** 200,9 end ****************************


// ************************** 184,7 begin ****************************
struct packer_default;
struct packer_cantor;

#define MAXREALSOLS 20
struct scontainerreal {
    u32 sols[MAXREALSOLS][132];
    u32 nsols;
};

template <u32 RB, u32 SM> struct equi;
template <u32 RB, u32 SM, u32 SSM, u32 THREADS, typename PACKER>
class eq_cuda_context
{
    equi<RB, SM>* eq;
    equi<RB, SM>* device_eq;
	unsigned char *testsol;
	unsigned char *dev_buf;
	scontainerreal* solutions;
	CUcontext pctx;
	u8 *m_buf;

public:
    eq_cuda_context(int thr_id, int dev_id, fn_validate validate, fn_cancel cancel);
    ~eq_cuda_context();
    void freemem();

    bool solve(unsigned char *pblock,
        unsigned char *header,
        unsigned int headerlen);

    fn_validate m_fnValidate;
    fn_cancel m_fnCancel;
    int thread_id;
    int device_id;
    int throughput;
    int totalblocks;
    int threadsperblock;
    int threadsperblock_digits;
    size_t equi_mem_sz;
};
// RB, SM, SSM, TPB, PACKER... but any change only here will fail..
#define CONFIG_MODE_184x7 5, 128, 12, 640, packer_cantor
// ************************** 184,7 end ****************************
