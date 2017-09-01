/*
 * Vinh Dang
 * vqd8a@virginia.edu
 *
 * finite automaton Object
 */

#ifndef FINITE_AUTOMATON_H
#define FINITE_AUTOMATON_H

#include <fstream>
#include <map>
#include <set>
#include <vector>
#include <stdio.h>
#include "common.h"
#include "finite_automaton.h"

/*------------MNRL------------*/
#include <unordered_map>
#include <queue>
#include "mnrl.hpp"
/*------------MNRL------------*/

class FiniteAutomaton {
    private:
        size_t dfa_state_table_size_;
        state_t *dfa_state_table_;
        std::map<unsigned int, std::set<unsigned int> > states2rules_;

    public:
        FiniteAutomaton(std::istream &, std::istream &, const char *, MemController &, unsigned int, int);
        void mapping_states2rules(unsigned int *match_count, match_type *match_array, unsigned int match_vec_size, std::vector<unsigned int> pkt_size_vec, std::vector<unsigned int> pad_size_vec, std::ofstream &fp, int *rulestartvec, unsigned int gid) const;//version 2: multi-byte fetching
        state_t *get_dfa_state_table();
        size_t get_dfa_state_table_size() const;
};

FiniteAutomaton *load_dfa_file(const char *pattern_name, unsigned int gid, int automata_format);

#endif
