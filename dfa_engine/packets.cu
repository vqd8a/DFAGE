/*
 * Vinh Dang
 * vqd8a@virginia.edu
 *
 * packets.cu
 */
 
#include "packets.h"
#include <vector>

using namespace std;

/*Packets::Packets() {
}
Packets::~Packets() {
}*/

void Packets::add_packet(const std::vector<unsigned char> payload) {
	payloads_.insert(payloads_.end(), payload.begin(), payload.end());
	payload_sizes_.push_back(payload.size());
}

const vector<symbol> &Packets::get_payloads(void) {
	return payloads_;
}

const vector<unsigned int> &Packets::get_payload_sizes() {
	return payload_sizes_;
}

//note: padding to each packet if packet_size is not evenly divided by fetch_bytes (e.g. 4, 8)
void Packets::set_padded_bytes(unsigned int nbytes){
	padded_sizes_.push_back(nbytes + (padded_sizes_.empty() ? 0 : padded_sizes_.back()));//Note: Accumulating the numbers of padded bytes of each packet
}

const vector<unsigned int> &Packets::get_padded_sizes() {
	return padded_sizes_;
}
