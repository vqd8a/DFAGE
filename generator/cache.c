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
 * File:   cache.c
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory 
 */


#include "cache.h"

cache::cache(cache_size_t c_size, line_size_t line_s, assoc_t associativity){
	size = c_size;
	line_size = line_s;
	cache_assoc = associativity;
	content = new cache_line_t[size/line_size];
	for (int i=0;i<size/line_size;i++) content[i].valid=false; 
}

cache::~cache(){
	delete [] content;
}

void cache::clean(){
	for (int i=0;i<size/line_size;i++) content[i].valid=false;
}

cache_access_t cache::read(int address){
	int shift_amount = (int)log2(line_size);
	int line_address = address >> shift_amount;
	int lines_per_set = size/(cache_assoc*line_size);
	int line_nr=line_address % lines_per_set;
	//cache hit
	for (int i=0;i<cache_assoc;i++) { // look in each set
		if (content[line_nr+i*lines_per_set].valid &&
			content[line_nr+i*lines_per_set].line == line_address)
			return HIT;
	}
	// cache miss - insertion witout eviction
	for (int i=0;i<cache_assoc;i++) { // look in each set
		if (!content[line_nr+i*lines_per_set].valid){
			content[line_nr+i*lines_per_set].valid = true;
			content[line_nr+i*lines_per_set].line = line_address;
			return MISS;
		}
	}
	//cache miss - insertion with eviction (random policy)
	int set=randint(1,cache_assoc)-1;
	content[line_nr+set*lines_per_set].valid = true;
	content[line_nr+set*lines_per_set].line = line_address;
	//printf("cache eviction\n");
	return MISS;
}

void cache::debug(FILE *stream,bool all){
	fprintf(stream, "\nCACHE:: size=%dB, line=%dB, associativity=%d-way\n\n",size,line_size,cache_assoc);
	int lines_per_set = size/(cache_assoc*line_size);
	if (all){
		for(int i=0;i<cache_assoc;i++){
			fprintf(stream,"set %d::\n",i+1);
			for (int j=0;j<lines_per_set;j++){
				if (content[j+i*lines_per_set].valid){
					fprintf(stream,"line %d - block address %d,0x%x\n",j,content[j+i*lines_per_set].line,content[j+i*lines_per_set].line);
				}
			}
		}
	}
}
