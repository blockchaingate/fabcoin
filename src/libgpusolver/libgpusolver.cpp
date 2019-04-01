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

#include <chrono>

#include "libgpusolver.h"
#include "util.h"
#include "primitives/block.h"
#include "arith_uint256.h"

//#define DEBUG

char *s_hexdump(const void *_a, uint32_t a_len)
{
	const uint8_t	*a = (const uint8_t	*) _a;
	static char		buf[1024];
	uint32_t		i;
	for (i = 0; i < a_len && i + 2 < sizeof (buf); i++)
	    sprintf(buf + i * 2, "%02x", a[i]);
	buf[i * 2] = 0;
	return buf;
}

GPUSolver::GPUSolver(unsigned int n, unsigned k) 
{
    size_t global_work_size = 1 << 20;
    size_t local_work_size = 32;

	miner = new cl_gpuminer(n,k);

	indices = (sols_t *) malloc(sizeof(sols_t));
	if(indices == NULL)
		std::cout << "Error allocating indices array!" << std::endl;

	GPU = miner->configureGPU(0, local_work_size, global_work_size);
	if(!GPU)
		std::cout << "ERROR: No suitable GPU found! No work will be performed!" << std::endl;

	/*Initialize the kernel, compile it and create buffers
	Currently runs for the gpu-list-gen.c kernel DATA_SIZE=100 times
	*/

    if( n == 200 && k == 9 )
    {
        std::vector<std::string> kernels {"kernel_init_ht", "kernel_round0", "kernel_round1", "kernel_round2","kernel_round3", "kernel_round4", "kernel_round5", "kernel_round6", "kernel_round7", "kernel_round8", "kernel_sols"};
        if(GPU)
            initOK = miner->init(0, 0, kernels);
    }
    else if( n == 184 && k == 7 )
    {
        std::vector<std::string> kernels {"kernel_init_ht", "kernel_round0", "kernel_round1", "kernel_round2","kernel_round3", "kernel_round4", "kernel_round5", "kernel_round6", "kernel_sols"};
        if(GPU)
            initOK = miner->init(0, 0, kernels);
    }

}

GPUSolver::GPUSolver(unsigned platform, unsigned device, unsigned int n, unsigned int k) {

	/* Notes
	I've added some extra parameters in this interface to assist with dev, such as
	a kernel string to specify which kernel to run and local/global work sizes.
	*/
	//TODO This looks like IND_PER_BUCKET, enough for GPU?
	size_t global_work_size = 1 << 20;
  	size_t local_work_size = 32;

	miner = new cl_gpuminer(n,k);

	indices = (sols_t *) malloc(sizeof(sols_t));
	if(indices == NULL)
		std::cout << "Error allocating indices array!" << std::endl;

	/* Checks each device for memory requirements and sets local/global sizes
	TODO: Implement device logic for equihash kernel
	@params: unsigned platformId
	@params: unsigned localWorkSizes
	@params: unsigned globalWorkSizes
	*/
	GPU = miner->configureGPU(platform, local_work_size, global_work_size);
	if(!GPU)
		std::cout << "ERROR: No suitable GPU found! No work will be performed!" << std::endl;

	/*Initialize the kernel, compile it and create buffers
	Currently runs for the gpu-list-gen.c kernel DATA_SIZE=100 times
	TODO: pass base state and nonce's to kernel.
	@params: unsigned _platformId
	@params: unsigned _deviceId
	@params: string& _kernel - The name of the kernel for dev purposes
	*/
    if( n == 200 && k == 9 )
    {
        std::vector<std::string> kernels {"kernel_init_ht", "kernel_round0", "kernel_round1", "kernel_round2","kernel_round3", "kernel_round4", "kernel_round5", "kernel_round6", "kernel_round7", "kernel_round8", "kernel_sols"};
        if(GPU)
            initOK = miner->init(platform, device, kernels);
    }
    else if( n == 184 && k == 7 )
    {
        std::vector<std::string> kernels {"kernel_init_ht", "kernel_round0", "kernel_round1", "kernel_round2","kernel_round3", "kernel_round4", "kernel_round5", "kernel_round6", "kernel_sols"};
        if(GPU)
            initOK = miner->init(platform, device, kernels);
    }

}

GPUSolver::~GPUSolver() {

	if(GPU)
		miner->finish();

	delete miner;

	if(indices != NULL)
		free(indices);

}

bool GPUSolver::run(unsigned int n, unsigned int k, uint8_t *header, size_t header_len, uint256 nonce,
		            const std::function<bool(std::vector<unsigned char>)> validBlock,
				const std::function<bool(GPUSolverCancelCheck)> cancelled,
			crypto_generichash_blake2b_state base_state) 
{
    return GPUSolve(n, k, header, header_len, nonce, validBlock, cancelled, base_state);
}

bool GPUSolver::GPUSolve(unsigned int n, unsigned int k, uint8_t *header, size_t header_len, uint256 &nonce,
                         const std::function<bool(std::vector<unsigned char>)> validBlock,
                         const std::function<bool(GPUSolverCancelCheck)> cancelled,
                         crypto_generichash_blake2b_state base_state) 
{
	/* Run the kernel
	TODO: Optimise and figure out how we want this to go
	@params eh_HashState& base_state - Sends to kernel in a buffer. Will update for specific kernels
	*/

	if(GPU && initOK) 
    {
      //  auto t = std::chrono::high_resolution_clock::now();
		uint256 nNonce;
    	miner->run(header, header_len, nonce, indices, &n_sol, &nNonce);

//        uint256 nNonce = ArithToUint256(ptr);
        //crypto_generichash_blake2b_update(&base_state, nNonce.begin(), nNonce.size());
/*
		auto d = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t);
		auto milis = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();

		if(!counter) {
			sum = 1000.f*n_sol/milis;
		} else {
			sum += 1000.f*n_sol/milis;
		}

		avg = sum/++counter;

		if(!(counter % 10))
			std::cout << "Kernel run took " << milis << " ms. (" << avg << " H/s)" << std::endl;
*/

		size_t checkedSols = n_sol;
		size_t s = 0;
        while (checkedSols) 
        {
			++s;
			if(indices->valid[s-1])
        		--checkedSols;
			else
				continue;

            std::vector<eh_index> index_vector(512);

            index_vector.resize(1<<k);
            for (size_t i = 0; i < (unsigned int)1 << k; i++) 
            {
            	index_vector[i] = indices->values[s-1][i];
            }

            std::vector<unsigned char> sol_char = GetMinimalFromIndices(index_vector, miner->PREFIX());
            //LogPrint(BCLog::POW, "Checking with = %s, %d sols\n",nNonce.ToString(), n_sol);
#ifdef DEBUG
            bool isValid;
            EhIsValidSolution(n, k, base_state, sol_char, isValid);
            LogPrint(BCLog::POW,"is valid: \n");
            if (!isValid) 
            {
                //If we find invalid solution bail, it cannot be a valid POW
                LogPrint(BCLog::POW,"Invalid solution found!\n");
                return false;
            } 
            else 
            {
                LogPrint(BCLog::POW,"Valid solution found!\n");
            }
#endif
            if (validBlock(sol_char)) 
            {
                // If we find a POW solution, do not try other solutions
                // because they become invalid as we created a new block in blockchain.
                LogPrint(BCLog::POW,"Valid block found!\n");
                return true;
            }        
        }
		//free(indices);
	}

    return false;

}
