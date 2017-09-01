/*
 * Vinh Dang
 * vqd8a@virginia.edu
 *
 * common_configs.cpp
 */

#include "common_configs.h"

using namespace std;

CommonConfigs::CommonConfigs() {
	packets_ = 1;
	threads_per_block_ = 64;
	groups_ = 1;
	input_file_name_ = NULL;
}

unsigned int CommonConfigs::get_state_count(unsigned int gid) const {
	return state_count_[gid];
}

unsigned int CommonConfigs::get_threads_per_block() const {
	return threads_per_block_;
}

unsigned int CommonConfigs::get_groups() const {
	return groups_;
}

unsigned int CommonConfigs::get_packets() const {
	return packets_;
}

const char *CommonConfigs::get_input_file_name() const {
	return input_file_name_;
}

void CommonConfigs::set_state_count(unsigned int state_count) {
	state_count_.push_back(state_count);
}

void CommonConfigs::set_threads_per_block(unsigned int threads_per_block) {
	threads_per_block_ = threads_per_block;
}

void CommonConfigs::set_groups(unsigned int ngroups) {
	groups_ = ngroups;
}

void CommonConfigs::set_packets(unsigned int packets) {
	packets_ = packets;
}

void CommonConfigs::set_input_file_name(char *input_file_name) {
	input_file_name_ = input_file_name;
}

MemController &CommonConfigs::get_controller() {
	return ctl_;
}
