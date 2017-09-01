/*
 * Vinh Dang
 * vqd8a@virginia.edu
 *
 * finite_automaton.cpp
 */

#include <boost/foreach.hpp>
#include <boost/iterator.hpp>
#include <cstdlib>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "common_configs.h"
#include "mem_controller.h"
#include "finite_automaton.h"

#include <algorithm>//for "find" function

using namespace std;
using namespace MNRL;//MNRL

extern CommonConfigs cfg;

/*------------------------------------------------------------------------------------*/
FiniteAutomaton::FiniteAutomaton(istream &file1, istream &file2, const char *pattern_name, MemController &allocator, unsigned int gid, int automata_format)
{
    if (automata_format == 1) {//MNRL file
        cout << "DFA filename " << gid + 1 << ": "<< string(pattern_name)+"_dfa.mnrl" << endl;
        shared_ptr<MNRL::MNRLNetwork> mnrl_graph = MNRL::loadMNRL(string(pattern_name) + "_dfa.mnrl");//load MNRL network from mnrl file

        map<string, shared_ptr<MNRLNode>> mnrl_nodes = mnrl_graph->getNodes();//get all nodes in the network

        unordered_map<string, int> id_map;//map MNRLNode ID to DFA ID 
        unordered_map<string, bool> marked;//mark nodes already processed
        queue<string> to_process;//store nodes going to be processed
        std::vector<string> start_states;
        unsigned int state_counter = 0;
        unsigned int accept_counter = 1;

        unordered_map<unsigned int, unsigned int *> state_table_map;//DFA transition table
        unordered_map<unsigned int, unsigned int> accepting_codes_map;
        unsigned int *tmp_next_states;

        /*Because each AP state is an DFA edge, we must instantiate
        * a start DFA state and add transitions to all start states 
        * in the automata.*/
        state_counter++;

        // find start states by looping over all nodes
        tmp_next_states = new unsigned int [CSIZE];//allocate an array for transitions from the current state to next states upon 256 possible input symbols

        for(auto &n : mnrl_nodes) {
            string node_id = n.first;
            if(n.second->getNodeType() != MNRLDefs::NodeType::HSTATE) {
                cout << "found node that wasn't hState: " << node_id << endl;
                exit(1);
            }

            shared_ptr<MNRLNode> node_tmp = n.second;
            shared_ptr<MNRLHState> node   = dynamic_pointer_cast<MNRLHState>(node_tmp);//cast to a MNRLHState

            MNRLDefs::EnableType start_type = node->getEnable();//check Enable type
            if ((start_type == MNRLDefs::ENABLE_ALWAYS)||(start_type == MNRLDefs::ENABLE_ON_START_AND_ACTIVATE_IN)){
                id_map[node_id] = state_counter++;

                //add a transition rule for every symbol in the symbol_set
                for(uint32_t i=1 ; i<node->getSymbolSet().length(); i+=4) {//string format: \xFA\xFB\x96\....
                    std::string str_hex = "0" + node->getSymbolSet().substr(i,3);
                    int str_int = std::stoi (str_hex,nullptr,16);
                    tmp_next_states[str_int] = id_map[node_id];//register the next state upon receving symbol str_int 
                }

                start_states.push_back(node_id);//register start state
            }
        }

        state_table_map[0] =  tmp_next_states;//assign the array pointer to the corresponding row in the state_table_map

        // for every start state, start building the DFA
        for(string start : start_states) {        
            shared_ptr<MNRLNode> node_tmp = mnrl_nodes[start];;
            shared_ptr<MNRLHState> node   = dynamic_pointer_cast<MNRLHState>(node_tmp);// cast to a MNRLHState

            //create a new state and make a transition
            if(id_map.find(start) == id_map.end())
                id_map[start] = state_counter++;

            marked[start] = true;

            if(node->getReport()) {
                accepting_codes_map[id_map[start]] = accept_counter++;//register accepting states
            }

            tmp_next_states = new unsigned int [CSIZE];//allocate an array for transitions from the current state to next states upon 256 possible input symbols

            // for every output, 
            for(auto to : *(node_tmp->getOutputConnections())) {
                //ask port pointer to get connections
                shared_ptr<MNRLPort> out_port = to.second;
                //for each connection in the port
                for(auto sink : out_port->getConnections()) {
                    shared_ptr<MNRLNode> sinkNode = sink.first;

                    //create a new state and make a transition
                    if(id_map.find(sinkNode->getId()) == id_map.end())
                        id_map[sinkNode->getId()] = state_counter++;

                    shared_ptr<MNRLHState> sinknode = dynamic_pointer_cast<MNRLHState>(sinkNode);

                    //add a transition rule for every symbol in the symbol_set
                    for(uint32_t i=1 ; i < sinknode->getSymbolSet().length(); i+=4) {
                        std::string str_hex = "0" + sinknode->getSymbolSet().substr(i,3);
                        int str_int = std::stoi (str_hex,nullptr,16);
                        tmp_next_states[str_int] = id_map[sinkNode->getId()];//register the next state upon receving symbol i
                    }

                    //push to todo list if we haven't already been here
                    if(marked[sinkNode->getId()] != true) {
                        to_process.push(sinkNode->getId());
                    }
                }
            }

            state_table_map[id_map[start]] =  tmp_next_states;//assign the array pointer to the corresponding row in the state_table_map
        }
    
        while(!to_process.empty()) {
            // get an node id
            string id = to_process.front();

            shared_ptr<MNRLNode> node_tmp = mnrl_nodes[id];;
            shared_ptr<MNRLHState> node = dynamic_pointer_cast<MNRLHState>(node_tmp);//cast to a MNRLHState

            // make sure not to double add states
            // still not sure why this is necessary but it is...
            // maybe because of self references in output lists?
            if(marked[id] == true) {
                to_process.pop();
                continue;
            }

            marked[id] = true;
            to_process.pop();

            if(id_map.find(id) == id_map.end())
                id_map[id] = state_counter++;

            //add element as an output node
            if(node->getReport()) {
                accepting_codes_map[id_map[id]] = accept_counter++;//register accepting states
            }

            tmp_next_states = new unsigned int [CSIZE];//allocate an array for transitions from the current state to next states upon 256 possible input symbols

            // for every output, 
            for(auto to : *(node_tmp->getOutputConnections())) {
                //ask port pointer to get connections
                shared_ptr<MNRLPort> out_port = to.second;
                //for each connection in the port
                for(auto sink : out_port->getConnections()) {
                    shared_ptr<MNRLNode> sinkNode = sink.first;

                    //create a new state and make a transition
                    if(id_map.find(sinkNode->getId()) == id_map.end())
                        id_map[sinkNode->getId()] = state_counter++;

                    shared_ptr<MNRLHState> sinknode = dynamic_pointer_cast<MNRLHState>(sinkNode);

                    //add a transition rule for every symbol in the symbol_set
                    for(uint32_t i=1 ; i<sinknode->getSymbolSet().length(); i+=4) {
                        std::string str_hex = "0" + sinknode->getSymbolSet().substr(i,3);
                        int str_int = std::stoi (str_hex,nullptr,16);
                        tmp_next_states[str_int] = id_map[sinkNode->getId()];//register the next state upon receving symbol i
                    }

                    //push to todo list if we haven't already been here
                    if(marked[sinkNode->getId()] != true) {
                        to_process.push(sinkNode->getId());
                    }
                }
            }

            state_table_map[id_map[id]] =  tmp_next_states;//assign the array pointer to the corresponding row in the state_table_map
        }

        cfg.set_state_count(state_counter);

        cout << "DFA "<< (gid + 1) << " has " << cfg.get_state_count(gid) << " states." <<endl;

        // Allocate the DFA data structure in host memory and fill it
        dfa_state_table_size_ = cfg.get_state_count(gid) * CSIZE * sizeof(*dfa_state_table_);
        dfa_state_table_  = allocator.alloc_host<state_t>(dfa_state_table_size_);
        for (unsigned int i=0; i < state_counter; i++)
            memcpy(&dfa_state_table_[i*CSIZE], state_table_map[i], CSIZE * sizeof(*dfa_state_table_));

        //Handle accepting states and their related rules
        unsigned int *accepting_states_;
        accepting_states_ = (unsigned int*)malloc (cfg.get_state_count(gid) * sizeof(unsigned int));
        memset(accepting_states_, 0, cfg.get_state_count(gid) * sizeof(unsigned int));
              
        for (unsigned int i=0; i < state_counter; i++) {
            if (accepting_codes_map[i]!=0){
                states2rules_[i].insert(accepting_codes_map[i]);//insert a new value of rule to the set of this state
                accepting_states_[i] = 1;
            }
        }

        //Represent accepting states in the DFA state table as negative numbers 
        for (unsigned int i = 0; i < cfg.get_state_count(gid) * CSIZE; i++) {
            if(accepting_states_[dfa_state_table_[i]] == 1)
                dfa_state_table_[i] = -dfa_state_table_[i];	
        }

        free(accepting_states_);

        // release allocated memory //VINH
        for (unsigned int i = 0; i < state_counter; i++) {
            delete[] state_table_map[i];
        }
    }
    else {//Binary file
        cout << "DFA filename " << gid + 1 << ": "<< string(pattern_name)+"_dfa.bin" << endl;
        //Handle accepting states and their related rules
        unsigned int tmp_st, tmp_rule;

        unsigned int *accepting_states_;
        accepting_states_ = (unsigned int*)malloc (cfg.get_state_count(gid) * sizeof(unsigned int));
        memset(accepting_states_, 0, cfg.get_state_count(gid) * sizeof(unsigned int));

        while (1) {
            file1.read ((char *)&tmp_st,  1 * sizeof(unsigned int));//cout << "file1.eof()=" << file1.eof() << endl;
            if (file1.eof()) break;//Check EOF
            file1.read ((char *)&tmp_rule, 1 * sizeof(unsigned int));
            states2rules_[tmp_st].insert(tmp_rule);//insert a new value of rule to the set of this state
            //cout << "ACCE_states_ " << tmp_st << ", Rule: " << tmp_rule << endl;
            accepting_states_[tmp_st] = 1;
        }

        // Allocate the DFA data structure in host memory and fill it
        dfa_state_table_size_ = cfg.get_state_count(gid) * CSIZE * sizeof(*dfa_state_table_);
        dfa_state_table_  = allocator.alloc_host<state_t>(dfa_state_table_size_);
        file2.read((char *)dfa_state_table_, cfg.get_state_count(gid) * CSIZE * sizeof(state_t));

        //Represent accepting states in the DFA state table as negative numbers 
        for (unsigned int i = 0; i < cfg.get_state_count(gid) * CSIZE; i++) {
            if(accepting_states_[dfa_state_table_[i]] == 1)
                dfa_state_table_[i] = -dfa_state_table_[i];	
        }

        free(accepting_states_);
    }

    //cout << "DFA loading done.\n";
    return;
}
/*------------------------------------------------------------------------------------*/
void FiniteAutomaton::mapping_states2rules(unsigned int *match_count, match_type *match_array, unsigned int match_vec_size, std::vector<unsigned int> pkt_size_vec, std::vector<unsigned int> pad_size_vec, std::ofstream &fp, int *rulestartvec, unsigned int gid) const {//version 2: multi-byte fetching
    unsigned int total_matches=0;	
    for (int j = 0; j < pkt_size_vec.size(); j++)	total_matches += match_count[j];
    fp   << "REPORTS: Total matches: " << total_matches << endl;

    for (int j = 0; j < pkt_size_vec.size(); j++) {
        for (unsigned i = 0; i < match_count[j]; i++) {
            map<unsigned, set<unsigned> >::const_iterator it = states2rules_.find(match_array[match_vec_size*j + i].stat);		
            if (j==0) fp   << match_array[match_vec_size*j + i].off  << "::" << endl;
            else      fp   << match_array[match_vec_size*j + i].off + j * pkt_size_vec[0] - (pad_size_vec.empty() ? 0 : pad_size_vec[j-1]) << "::" << endl;
            if (it != states2rules_.end()) {
                set<unsigned>::iterator iitt;
                for (iitt = it->second.begin();	iitt != it->second.end(); ++iitt) {
                    fp   << "    Rule: " << *iitt + rulestartvec[gid] << endl;
                }
            }
        }
    }
}
/*------------------------------------------------------------------------------------*/
FiniteAutomaton *load_dfa_file(const char *pattern_name, unsigned int gid, int automata_format) {
    ifstream file1;//accepting states
    ifstream file2;//DFA transition table

    /* Load automata from file */
    if (pattern_name) {

        string dfabin_filename;
        string accstbin_filename;

        string tmpstr(pattern_name);

        dfabin_filename = tmpstr + "_dfa.bin";
        accstbin_filename = tmpstr + "_accst.bin";
        //cout << "pattern_name = " << pattern_name << ", dfabin_filename = " << dfabin_filename.c_str() << ", accstbin_filename = " << accstbin_filename.c_str() << endl;


        if (automata_format == 0) {
            file1.open(accstbin_filename.c_str(),ios::binary | ios::in);
            file2.open(dfabin_filename.c_str(),ios::binary | ios::in);
		
            if (!file1.good()) {
                cout << "Can't open the file " << accstbin_filename << endl;
                return NULL;
            }

            if (!file2.good()) {
                cout << "Cann't open the file " << dfabin_filename << endl;
                return NULL;
            }
		
            // Read the state count from the first value of the dumpbin_file
            unsigned int t; //cout << "sizeof(unsigned) = " << sizeof(unsigned) << " bytes" << endl;
            file2.read ((char *)&t, 1*sizeof(unsigned int));
            cfg.set_state_count(t);

            /* Read the transition graph from disk and copy it to the device */
            cout << "DFA "<< (gid + 1) << " has " << cfg.get_state_count(gid) << " states." <<endl;
        }

        FiniteAutomaton *fa = new FiniteAutomaton(file1, file2, pattern_name, cfg.get_controller(), gid, automata_format);

        if (automata_format == 0) {
            file1.close();
            file2.close();
        }

        return fa;

    } else {
        cout << "Automaton name is invalid" << endl;
        return NULL;
    }
}
/*------------------------------------------------------------------------------------*/
state_t *FiniteAutomaton::get_dfa_state_table() {
    return dfa_state_table_;
}
/*------------------------------------------------------------------------------------*/
size_t FiniteAutomaton::get_dfa_state_table_size() const {
    return dfa_state_table_size_;
}
