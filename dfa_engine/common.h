/*
 * Vinh Dang
 * vqd8a@virginia.edu
 *
 * common Object
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cuda_runtime.h>
#include <cuda.h>

#define TRUE 1
#define FALSE 0

#define CSIZE 256 //alphabet's size

typedef unsigned char symbol;//note: each symbol has 1 byte
typedef unsigned int symboln;//4-byte fetches
//typedef unsigned long long symbol_fetch;//version 2 -- 8-byte fetches
#define fetch_bytes 4              //version 2: -- note: change this constant to the respective symbol_fetch

//typedef unsigned int state_t;//Orig
typedef int state_t;//version 2 -- note: use negative numbers for accepting states, which limits the total number of states

typedef struct _match_type{//version 2 -- note: merge two memory transactions into one in matching operations
    unsigned int off;
    unsigned int stat;
} match_type;

#endif
