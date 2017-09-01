/*
 * Vinh Dang
 * vqd8a@virginia.edu
 *
 * common configurations Object
 */

#ifndef COMMON_CONFIGS_H
#define COMMON_CONFIGS_H

#include <stdio.h>
#include <fstream>
#include <vector>

#include "common.h"
#include "mem_controller.h"

class CommonConfigs {
	private:				
		std::vector<unsigned int> state_count_;
		unsigned int threads_per_block_;
		unsigned int groups_;
		unsigned int packets_;
		char *input_file_name_;
			
		MemController ctl_;

	public:
		CommonConfigs();

		unsigned int get_state_count(unsigned int gid) const;
		unsigned int get_threads_per_block() const;
		unsigned int get_groups() const;
		unsigned int get_packets() const;
    	const char *get_input_file_name() const;
		MemController &get_controller();
		
		void set_state_count(unsigned int state_count);
		void set_threads_per_block(unsigned int threads_per_block);
		void set_groups(unsigned int ngroups);
		void set_packets(unsigned int packets);
		void set_input_file_name(char * trace_filename);
};

#endif
