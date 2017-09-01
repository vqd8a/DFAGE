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
 * File:   stdinc.h
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 * 
 * Description: Contains constant definitions, warning and message functions, 
 * 				memory allocation and de-allocation functions used all over the code.
 * 
 */

#ifndef STDINC_H_
#define STDINC_H_

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* configuration */

#define COMMON_TARGET_REDUCTION //NFA reduction
#define HYBRID_FA_REDUCTION     //NFA reduction
#define TAIL_DFAS 				//Hybrid-FA construction

#define SPECIAL_MIN_DEPTH 3     //minimum NFA depth of a special state
#define MAX_TX 250		        //Hybrid-FA:: maximum number of outgoing transitions allowed in a expanded NFA state 
#define MAX_HEAD_SIZE 5000      //Hybrid-FA:: maximum number of states in the head automaton before starting to create tails
//#define MAX_DFA_SIZE 100000     //DFA formation//orig
//#define MAX_DFA_SIZE 50000000     //DFA formation//orig
//#define MAX_DFA_SIZE 500000000     //DFA formation//orig
#define MAX_DFA_SIZE 4200000000     //DFA formation//orig

/* type definitions: state identifiers and symbols */
typedef unsigned int state_t;
typedef unsigned symbol_t;
typedef unsigned short label_t;

/* constant definition */

#define NO_STATE 0xFFFFFFFF //invalid state

#define CSIZE 256    //alphabet's size
#define _INFINITY -1

//verbosity levels
extern int VERBOSE;
extern int DEBUG; 

const int Null = 0;
const int BIGINT = 0x7fffffff;
const int EOS = '\0';

/* max and min */
inline int max(int x, int y) { return x > y ? x : y; }
inline double max(double x, double y) { return x > y ? x : y; }
inline int min(int x, int y) { return x < y ? x : y; }
inline double min(double x, double y) { return x < y ? x : y; }

/* warnings and errors */
inline void warning(char* p) { fprintf(stderr,"Warning:%s \n",p); }
inline void fatal(char* string) {fprintf(stderr,"Fatal:%s\n",string); exit(1); }

/* memory allocation operations */
void *allocate_array(int size, size_t element_size);
void *reallocate_array( void *array, int size, size_t element_size);

#define allocate_char_array(size) (char *) allocate_array( size, sizeof( char ) )
#define allocate_string_array(size) (char **) allocate_array( size, sizeof( char *) )
#define allocate_int_array(size) (int *) allocate_array( size, sizeof( int ) )
#define allocate_uint_array(size) (unsigned int *) allocate_array( size, sizeof(unsigned int ) )
#define allocate_bool_array(size) (bool *) allocate_array( size, sizeof( bool ) )
#define allocate_state_array(size) (state_t *) allocate_array( size, sizeof( state_t ) )
#define allocate_state_matrix(size) (state_t **) allocate_array( size, sizeof( state_t *) )

#define reallocate_char_array(size) (char *) reallocate_array(array, size, sizeof( char ) )
#define reallocate_int_array(array,size) (int *) reallocate_array(array, size, sizeof( int ) )
#define reallocate_bool_array(array,size) (bool *) reallocate_array( array,size, sizeof( bool ) )
#define reallocate_uint_array(array,size) (unsigned int *) reallocate_array(array, size, sizeof(unsigned int ) )
#define reallocate_state_array(array,size) (state_t *) reallocate_array(array, size, sizeof( state_t ) )
#define reallocate_state_matrix(array,size) (state_t **) reallocate_array(array, size, sizeof( state_t *) )
#define reallocate_string_array(array,size) (char **) reallocate_array(array, size, sizeof( char *) )


/* Random number generators */

// Return a random number in [0,1] 
inline double randfrac() { return ((double) random())/BIGINT; }

// Return a random integer in the range [lo,hi].
// Not very good if range is larger than 10**7.
inline int randint(int lo, int hi) { return lo + (rand() % (hi + 1 - lo)); } //random

// Return a random number from an exponential distribution with mean mu 
inline double randexp(double mu) { return -mu*log(randfrac()); }

// Return a random number from a geometric distribution with mean 1/p
inline int randgeo(double p) {
	return p > .999999 ? 1 : max(1,int(.999999 +log(randfrac())/log(1-p)));
}

// Return a random number from a Pareto distribution with mean mu and shape s
inline double randpar(double mu, double s) {
	return mu*(1-1/s)/exp((1/s)*log(randfrac()));
}

// Return a random number from a normal distribution with mean mu and variance sigma_2
inline double randnormal(double mu, double sigma_2){
	double x=(double)rand();
	return sqrt(sigma_2)*sqrt(-2*log(x+1)/(RAND_MAX+1))*sin(2*M_PI*x/(RAND_MAX+1))+mu;
}

#endif //STDINC_H_


