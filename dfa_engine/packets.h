/*
 * Vinh Dang
 * vqd8a@virginia.edu
 *
 * packets Object
 */

#ifndef PACKETS_H
#define PACKETS_H

#include <vector>
#include "common.h"

using namespace std;

class Packets {
	private:
		vector<symbol>payloads_;
		vector<unsigned int> payload_sizes_;
		vector<unsigned int> padded_sizes_;//store the numbers of padded bytes of each packet 
		                                   //note: padding to each packet if packet_size is not evenly divided by fetch_bytes (e.g. 4, 8)
	public:
		//Packets();
		//~Packets();
		void add_packet(const std::vector<unsigned char> payload);
		const vector<symbol> &get_payloads(void);		
		const vector<unsigned int> &get_payload_sizes(void);

		//note: padding to each packet if packet_size is not evenly divided by fetch_bytes (e.g. 4, 8)
		void set_padded_bytes(unsigned int nbytes);
		const vector<unsigned int> &get_padded_sizes(void);
};

#endif
