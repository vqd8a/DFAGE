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
 * File:   cache.h
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory 
 *
 * Description:  implements a CACHE w/ a given size, a given cache line and a given associativity.
 * 
 */


#ifndef CACHE_H_
#define CACHE_H_

#include "stdinc.h"

//type for cache line size
typedef int line_size_t;

//type for cache set associativity
typedef enum {DM=1, W2=2, W4=4} assoc_t;

//type for cache size
typedef enum {K1=1024, K2=2048, K4=4096, K8=8192, K16=16384, K32=32768, K64=65536,
			  K128=131072, K256=262144, K512=524288} cache_size_t;

//cache line
typedef struct _cache_line{
	int  line;   //content
	bool valid;  //is the cache line valid
} cache_line_t;

//type of cache access (hit or miss)
typedef enum {HIT, MISS} cache_access_t;


class cache{
	
	// cache size
	cache_size_t size;
	
	// cache line size
	line_size_t line_size;
	
	// cache associativity
	assoc_t cache_assoc;
	
	// content of the cache
	cache_line_t *content;
	
public:

	//instantiates the cache
	cache(cache_size_t,line_size_t,assoc_t);
	
	//de-allocates the cache
	~cache();
	
	//reads an address and returns whether it was a hit-miss
	cache_access_t read(int address);

	//debug information
	void debug(FILE *stream=stdout, bool all=true);

	//returns cache size
	cache_size_t get_size();
	
	//returns cache set associativity
	assoc_t get_assoc();
	
	//cleans the content of the cache
	void clean();

};

inline cache_size_t cache::get_size(){return size;}
	
inline assoc_t cache::get_assoc(){return cache_assoc;}

#endif /*CACHE_H_*/
