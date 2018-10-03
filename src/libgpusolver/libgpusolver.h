/* MIT License
 *
 * Copyright (c) 2016 Omar Alvarez <omar.alvarez@udc.es>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __GPU_SOLVER_H
#define __GPU_SOLVER_H

#include <cstdio>
#include <csignal>
#include <iostream>

#include "crypto/equihash.h"
#include "libclwrapper.h"
#include "uint256.h"

#include <cassert>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

// The maximum size of the .cl file we read in and compile
#define MAX_SOURCE_SIZE 	(0x200000)

class GPUSolverCancelledException : public std::exception
{
    virtual const char* what() const throw() {
        return "GPU Equihash solver was cancelled";
    }
};

enum GPUSolverCancelCheck
{
    ListGenerationGPU,
    ListSortingGPU
};

class GPUSolver {

public:
	GPUSolver(unsigned int n, unsigned k);
	GPUSolver(unsigned platform, unsigned device, unsigned int n, unsigned int k);
	~GPUSolver();

    std::pair<int, int> getparam() 
    { 
        std::pair<int, int> param;
        if(  miner )
        {
            param.first = miner->PARAM_N;
            param.second = miner->PARAM_K;
        }
        else
        {
            param.first = 0;
            param.second = 0;
        }            
        return param;
    }
    bool run(unsigned int n, unsigned int k, uint8_t *header, size_t header_len, uint256 nonce,
        const std::function<bool(std::vector<unsigned char>)> validBlock,
        const std::function<bool(GPUSolverCancelCheck)> cancelled,
        crypto_generichash_blake2b_state base_state);

private:
	cl_gpuminer * miner;
	bool GPU;
	bool initOK;
	//TODO 20?
	sols_t *indices;
	uint32_t n_sol;
	//Avg
	uint32_t counter = 0;
	float sum = 0.f;
	float avg = 0.f;

	bool GPUSolve(unsigned int n, unsigned int k, uint8_t *header, size_t header_len, uint256 &nonce,
        const std::function<bool(std::vector<unsigned char>)> validBlock,
        const std::function<bool(GPUSolverCancelCheck)> cancelled,
        crypto_generichash_blake2b_state base_state);

};

#endif // __GPU_SOLVER_H
