/*
 * Copyright (c) 2007 Michela Becchi and Washington University in St. Louis.
 * All rights reserved
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. The name of the author or Washington University may not be used
 *       to endorse or promote products derived from this source code
 *       without specific prior written permission.
 *    4. Conditions of any other entities that contributed to this are also
 *       met. If a copyright notice is present from another entity, it must
 *       be maintained in redistributions of the source code.
 *
 * THIS INTELLECTUAL PROPERTY (WHICH MAY INCLUDE BUT IS NOT LIMITED TO SOFTWARE,
 * FIRMWARE, VHDL, etc) IS PROVIDED BY  THE AUTHOR AND WASHINGTON UNIVERSITY
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR WASHINGTON UNIVERSITY
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS INTELLECTUAL PROPERTY, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * */

/*
 * File:   memory.h
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory 
 */

#ifndef __MEMORY_H      
         
#define __MEMORY_H

#include "stdinc.h"
#include "comp_dfa.h"
#include "comp_nfa.h"
#include "hybrid_fa.h"

#define MAX_PKT 1000000

#define TX_THRESHOLD   7

#define NFA_STATIC 1
//#define STATISTICS 1

#include <netinet/in.h>
#include <arpa/inet.h>
#include <pcap.h>
typedef u_long tcp_seq;
/* Ethernet addresses are 6 bytes */
#define ETHER_ADDR_LEN	6

	/* Ethernet header */
	struct sniff_ethernet {
		u_char ether_dhost[ETHER_ADDR_LEN]; /* Destination host address */
		u_char ether_shost[ETHER_ADDR_LEN]; /* Source host address */
		u_short ether_type; /* IP? ARP? RARP? etc */
	};

	/* IP header */
	struct sniff_ip {
		u_char ip_vhl;		/* version << 4 | header length >> 2 */
		u_char ip_tos;		/* type of service */
		u_short ip_len;		/* total length */
		u_short ip_id;		/* identification */
		u_short ip_off;		/* fragment offset field */
	#define IP_RF 0x8000		/* reserved fragment flag */
	#define IP_DF 0x4000		/* dont fragment flag */
	#define IP_MF 0x2000		/* more fragments flag */
	#define IP_OFFMASK 0x1fff	/* mask for fragmenting bits */
		u_char ip_ttl;		/* time to live */
		u_char ip_p;		/* protocol */
		u_short ip_sum;		/* checksum */
		struct in_addr ip_src,ip_dst; /* source and dest address */
	};
	#define IP_HL(ip)		(((ip)->ip_vhl) & 0x0f)
	#define IP_V(ip)		(((ip)->ip_vhl) >> 4)

	/* TCP header */
	struct sniff_tcp {
		u_short th_sport;	/* source port */
		u_short th_dport;	/* destination port */
		tcp_seq th_seq;		/* sequence number */
		tcp_seq th_ack;		/* acknowledgement number */

		u_char th_offx2;	/* data offset, rsvd */
	#define TH_OFF(th)	(((th)->th_offx2 & 0xf0) >> 4)
		u_char th_flags;
	#define TH_FIN 0x01
	#define TH_SYN 0x02
	#define TH_RST 0x04
	#define TH_PUSH 0x08
	#define TH_ACK 0x10
	#define TH_URG 0x20
	#define TH_ECE 0x40
	#define TH_CWR 0x80
	#define TH_FLAGS (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
		u_short th_win;		/* window */
		u_short th_sum;		/* checksum */
		u_short th_urp;		/* urgent pointer */
};

enum type_t {DFA_T, NFA_T};

class memory{
	
	type_t type;
	FILE *fout;
	
        pcap_t *PcapHandle;
        char ErrBuf[PCAP_ERRBUF_SIZE + 1];
	struct bpf_program FilterCode;
	
	/* automaton*/
	//DFA
	unsigned num_dfas;
	compDFA **dfas;
    //NFA
	compNFA *nfa;
	//HFA and border
	HybridFA *hfa;
	map <NFA *, unsigned> *mapping;
	
	/* tail address for HFA */
	unsigned *tail_address;
	
	/* list of state indexes */
	unsigned **state_address;
	
	//number of entries in each FA
	unsigned *size;
	
	//memory layout
	unsigned **fa;
	
	// is an NFA state stable?
	bool *stable;
	
	unsigned *num_full_states;
	unsigned *num_compressed_states;
	unsigned *num_compressed_tx;	
	
	symbol_t alphabet;
	symbol_t *alphabet_tx;
	
public:
	memory();
	memory(compDFA **, unsigned);
	memory(compNFA *);
	memory(HybridFA *);
	~memory();
	
	unsigned **get_fa();
	
	unsigned get_num_dfas();
	
	symbol_t get_alphabet();
	
	symbol_t *get_alphabet_tx();
	
	void put(FILE *stream=stdout);
	
	void get(FILE *stream);
	
	void traverse_dfa(unsigned trace_size, char *data, FILE *log=stdout);
	
	void traverse_nfa(unsigned trace_size, char *data, FILE *log=stdout);
	
	void traverse_hfa(unsigned trace_size, char *data, FILE *log=stdout);
	
	int open_capture(char *filename, char *dumpfilename);
	
private: 
	void print_layout();
	void compute_dfa_memory_layout();
	void compute_dfa_memory_map();
	void compute_nfa_memory_layout();
	void compute_nfa_memory_map();
	void compute_hfa_memory_layout();
	void compute_hfa_memory_map();

        void get_payload(char *payload, int *size);
};	 

inline unsigned **memory::get_fa(){ return fa;}

inline unsigned memory::get_num_dfas(){ return num_dfas;}

inline symbol_t memory::get_alphabet(){return alphabet;}
	
inline symbol_t *memory::get_alphabet_tx(){return alphabet_tx;}


	

#endif /*__MEMORY_H*/
