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
 * File:   int_set.h
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 * 
 * Description: Implements a  set of positive integer
 */

#ifndef __INT_SET_H
#define __INT_SET_H

#define UNDEF 0xFFFFFFFF

class int_set {
	unsigned int N;				// max number of elements in the set
	bool *item;					// item[i] is true if i is in the set
	unsigned int num_el;        // real number of elements in the set
	unsigned int first;			// pointer to the first element in the set
	
public:		int_set(unsigned int=100); 	//constructor
		   ~int_set();					//de-allocator
	void	operator=(int_set&);	// set assignment
	void	insert(unsigned int);	// insert item in set
	void	remove(unsigned int);	// remove item from set
	bool	mbr(unsigned int);		// return true if member of set
	bool	empty();				// return true if set is empty
	unsigned int size();         	// return size of set
	unsigned int head();			// return first element - for browsing
	unsigned int suc(unsigned int); // return successor in set - browsing routing
	void	clear();				// remove everything
	void    reset(unsigned int);    // reset the set
	void	print();				// print the items on list
	void    add(int_set *);         // add all the elements of the parameter in set 
	void 	negate();			    //complements the current set
};

inline unsigned int int_set::size() {return num_el;}

inline unsigned int int_set::head() {return first;}

inline bool int_set::empty(){ return (num_el==0);}

#endif /*__INT_SET_H*/
