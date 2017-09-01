/*
 * Vinh Dang
 * vqd8a@virginia.edu
 *
 * mem_controller Object
 */

#ifndef MEM_CONTROLLER_H
#define MEM_CONTROLLER_H

#include <iostream>
#include <cuda.h>
#include <cuda_runtime.h>

#include <assert.h>
#include <vector>

using namespace std;

class MemController {
	private:
		std::vector<void *> host_;

	public:
		//MemController();
		//~MemController();

		template<typename T>
			T *alloc_host(size_t size) {
				T *ptr(0);

	            cudaError_t retval = cudaMallocHost((void **) &ptr, size);
                
	            if (retval !=cudaSuccess) {
                    cout << "Error during cudaMallocHost\n";
                    ptr = 0;
                }
                else
                    host_.push_back(ptr);				

				return ptr;
			}

		void dealloc_host_all();
		unsigned int get_host_size();
};

#endif
