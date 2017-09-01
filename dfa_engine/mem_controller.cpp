/*
 * Vinh Dang
 * vqd8a@virginia.edu
 *
 * mem_controller.cpp
 */

#include "mem_controller.h"
#include <iostream>

using namespace std;

/*MemController::MemController() {
	return;
}*/

/*MemController::~MemController() {
}*/

void MemController::dealloc_host_all() {
	for(unsigned int i=0; i < host_.size(); i++){
        cudaError_t retVal = cudaFreeHost(host_[i]);
        if (retVal != cudaSuccess) cout << "Error during cudaFreeHost" << endl;
	}
    host_.clear();
    return;
}

unsigned int MemController::get_host_size(){
	return host_.size();
}
