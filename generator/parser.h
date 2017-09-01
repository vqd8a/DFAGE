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
 * File:   parser.h
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 * 
 * Description: 
 * 
 * 		This class implements a regular expression parser.
 * 		Use the parse function to parse a file containing a list of regular expressions and generate 
 * 		the corresponding NFA.
 * 
 * 		Regular expression beginning with tilde (^) are considered to be anchored at the beginning of the
 * 		input text, the other ones are considered to appear at any position (the initial .* is implicit)
 */

#ifndef REGEX_PARSER_H_
#define REGEX_PARSER_H_

#include "stdinc.h"
#include "nfa.h"
#include "dfa.h"
#include "int_set.h"

// definition of special characters
#define ANY '.'
#define ESCAPE '\\'
#define STAR '*'
#define PLUS '+'
#define OPT '?'
#define OPEN_RBRACKET '('
#define CLOSE_RBRACKET ')'
#define OPEN_SBRACKET '['
#define CLOSE_SBRACKET ']'
#define OPEN_QBRACKET '{'
#define CLOSE_QBRACKET '}'
#define COMMA ','
#define TILDE '^'
#define OR '|'
#define MINUS_RANGE '-'  
 
class regex_parser{
	
	bool i_modifier; //the parsed regular expressions must be in ignore case mode (see PCRE specs)	
		
	bool m_modifier; // the m modifier must be applied (see PCRE specs)

public:

	//instantiates the parser
	regex_parser(bool i_mod, bool m_mod);
	
	//parser de-allocator
	~regex_parser();
	
	//parses all the regular expressions contained in file and returns the corresponding NFA
	NFA *parse(FILE *file, int from=1, int to=-1);
	
	//parses all the regular expressions containted in file and returns a set of DFAs
	dfa_set *parse_to_dfa(FILE *file);
	
private:

	//parses a regular expressions into the given NFA
	void *parse_re(NFA* nfa, const char *re);

	//parses a substring of a regular expression and returns the corresponding NFA
	NFA *parse_re(const char *re, int *ptr, bool bracket);
	
	//process an escape sequence
	int process_escape(const char *re, int ptr, int_set *chars);

	//process a  bounded repetition ({})
	int process_quantifier(const char *re, int ptr,int *lb, int *ub);

	//process a range of characters ([-])
	int process_range(NFA **fa, NFA **to_link, const char *re, int ptr);
	
};

// returns true if the given character is special
inline bool is_special (char c){
	return ((c==ANY)||(c==ESCAPE)||(c==STAR)||(c==PLUS) || (c==OPT) ||
		   (c==OPEN_RBRACKET) || (c==CLOSE_RBRACKET) ||
		   (c==OPEN_SBRACKET) || (c==CLOSE_SBRACKET) ||
		   (c==OPEN_QBRACKET) || (c==CLOSE_QBRACKET) ||
		   (c==OR));
		   
};

// returns true if the character is a digit
inline bool is_digit(char c){
	return ((c>='0') && (c<='9'));
}

//returns true if the character is a oct. digit
inline bool is_oct_digit(char c){
	return ((c>='0') && (c<='7'));
}

//returns true if the character is a hex digit
inline bool is_hex_digit(char c){
	return (is_digit(c) || (c>='a' && c<='f') || (c>='A' && c<='F'));
}

// returns true if the given character represents a (bounded or unbounded) repetition
inline bool is_repetition (char c){
	return ((c==STAR)||(c==PLUS)||(c==OPT)||(c==OPEN_QBRACKET));
};

// returns true is the character is x or X
inline bool is_x(char c){
	return (c=='x' || c=='X');
}

// returns the escaped form  of the given character
inline char escaped(char c){
	switch (c){
		case 'a': return '\a';
		case 'b': return '\b';
		case 'f': return '\f';
		case 'n': return '\n';
		case 'r': return '\r';
		case 't': return '\t';
		case 'v': return '\v';
		default: return c;
	}
}	

#endif /*REGEX_PARSER_H_*/
