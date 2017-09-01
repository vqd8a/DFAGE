#include "ixp_memory.h"
#include <list>

#define SCRATCH 1
#define SRAM 2
#define DRAM 3

using namespace std;

ixp_memory::ixp_memory(unsigned in_num_dfas, compDFA **in_dfas,
					   unsigned alphabet_size, symbol_t *alphabet_tx,
					   char *scratch_file, char *sram_file, char *dram_file){
	num_dfas=in_num_dfas;
	dfas=in_dfas;
	nfa=NULL;
	hfa=NULL;
	mapping=NULL;
	
	stable=NULL;
	
	alphabet=alphabet_size;
	alph_tx=alphabet_tx;
	
	state_address = (unsigned **)allocate_array(sizeof(unsigned *),num_dfas);
	for (int i=0;i<num_dfas;i++) state_address[i]=new unsigned[dfas[i]->size];
	
	scratch_map=fopen(scratch_file,"w");
	if (scratch_map==NULL) fatal("cannot open scratch map file");
	sram_map=fopen(sram_file,"w");
	if (sram_map==NULL) fatal("cannot open sram map file");
	dram_map=fopen(dram_file,"w");
	if (dram_map==NULL) fatal("cannot open dram map file");
	
	scratch_base = new unsigned[num_dfas];
	sram_base	 = new unsigned[num_dfas];
	dram_base	 = new unsigned[num_dfas];
	
	num_full_states		  = new unsigned[num_dfas];
	num_compressed_states = new unsigned[num_dfas];
	num_compressed_tx     = new unsigned[num_dfas];
	size                  = new unsigned[num_dfas];
	
	for (int i=0;i<num_dfas;i++){
		scratch_base[i]=sram_base[i]=dram_base[i]=0xFFFFFFFF;
		num_full_states[i]=0;
		num_compressed_states[i]=0;
		num_compressed_tx[i]=0;
		size[i]=0;
	}
	
	last_addr=0xFFFFFFFF;
	
	compute_dfa_memory_layout();
	compute_dfa_memory_map();
}

ixp_memory::ixp_memory(HybridFA *hfa_in,
					   char *scratch_file, char *sram_file, char *dram_file){
	
	nfa=NULL;
	hfa=hfa_in;
	mapping = new map <NFA *, unsigned>(); 
	
	nfa_set *border_states=new nfa_set();
	for(border_it it=hfa->get_border()->begin();it!=hfa->get_border()->end();it++)
		border_states->insert(((*it).second)->begin(),((*it).second)->end());
	
	num_dfas=1+border_states->size(); //head+tails
	
	DFA *tail_dfas[num_dfas];
	
	unsigned i=0;
	tail_dfas[i++]=hfa->get_head();
	if (VERBOSE) printf("ixp_memory:: HFA: head - size=%d\n",hfa->get_head()->size());
	FOREACH_SET(border_states,it){
		(*mapping)[*it]=i;
		if (DEBUG) printf("ixp_memory:: generating tail-DFA[%d] for NFA-state %d (w/ depth=%d,size=%d)...\n",i,(*it)->get_id(),(*it)->get_depth(),(*it)->size());
		tail_dfas[i]=(*it)->nfa2dfa();
		if (tail_dfas[i]==NULL){
			printf("ixp_memory:: could not create tail-DFA[%d] for NFA-state %d (w/ size=%d)\n",i,(*it)->get_id(),(*it)->size());
			//exit(-1);
		}else{
			if (tail_dfas[i]->size()<100000) tail_dfas[i]->minimize();
			if (VERBOSE) printf("ixp_memory:: HFA: tail[%d] for NFA-state %d (w/ depth=%d,size=%d) - size=%d\n",i,(*it)->get_id(),(*it)->get_depth(),(*it)->size(),tail_dfas[i]->size());
		}
		i++;
		fflush(stdout);
	}
	
	alphabet = 0;
	alph_tx=new symbol_t[CSIZE];
	for (symbol_t c=0;c<CSIZE;c++) alph_tx[c]=0;
	for (int i=0;i<num_dfas;i++) alphabet = tail_dfas[i]->alphabet_reduction(alph_tx,false);
	if (VERBOSE) printf("alphabet size of Hybrid-FA %d\n",alphabet);
	
	dfas=(compDFA**)allocate_array(sizeof(compDFA*),num_dfas);
	for(int i=0;i<num_dfas;i++){
		dfas[i]=new compDFA(tail_dfas[i],alphabet,alph_tx);
		if (i!=0) delete tail_dfas[i];
	}
	delete border_states;
	
	stable=NULL;
	
	state_address = (unsigned **)allocate_array(sizeof(unsigned *),num_dfas);
	for (int i=0;i<num_dfas;i++) state_address[i]=new unsigned[dfas[i]->size];
	
	scratch_map=fopen(scratch_file,"w");
	if (scratch_map==NULL) fatal("cannot open scratch map file");
	sram_map=fopen(sram_file,"w");
	if (sram_map==NULL) fatal("cannot open sram map file");
	dram_map=fopen(dram_file,"w");
	if (dram_map==NULL) fatal("cannot open dram map file");
	
	scratch_base = new unsigned[num_dfas];
	sram_base	 = new unsigned[num_dfas];
	dram_base	 = new unsigned[num_dfas];
	
	num_full_states		  = new unsigned[num_dfas];
	num_compressed_states = new unsigned[num_dfas];
	num_compressed_tx     = new unsigned[num_dfas];
	size                  = new unsigned[num_dfas];
	
	for (int i=0;i<num_dfas;i++){
		scratch_base[i]=sram_base[i]=dram_base[i]=0xFFFFFFFF;
		num_full_states[i]=0;
		num_compressed_states[i]=0;
		num_compressed_tx[i]=0;
		size[i]=0;
	}
	
	last_addr=0xFFFFFFFF;
	
	compute_hfa_memory_layout();
	compute_hfa_memory_map();
}

ixp_memory::ixp_memory(compNFA* comp_nfa,
					   char *scratch_file, char *sram_file, char *dram_file){
	nfa=comp_nfa;
	num_dfas=0;
	dfas=NULL;
	hfa=NULL;
	mapping=NULL;
	
	alphabet=nfa->alphabet_size;
	alph_tx=nfa->alphabet_tx;
	
	state_address = (unsigned **)allocate_array(sizeof(unsigned *),1);
	state_address[0]=new unsigned[nfa->size];
	
	stable=new bool[nfa->size];
	for (state_t s=0;s<nfa->size;s++) stable[s]=false;
	
	scratch_map=fopen(scratch_file,"w");
	if (scratch_map==NULL) fatal("cannot open scratch map file");
	sram_map=fopen(sram_file,"w");
	if (sram_map==NULL) fatal("cannot open sram map file");
	dram_map=fopen(dram_file,"w");
	if (dram_map==NULL) fatal("cannot open dram map file");
	
	scratch_base = NULL;
	sram_base	 = NULL;
	dram_base	 = NULL;
	
	num_full_states		  = NULL;
	num_compressed_states = NULL;
	num_compressed_tx     = NULL;
	size				  = NULL;
	
	last_addr=0xFFFFFFFF;
	
	compute_nfa_memory_layout();
	compute_nfa_memory_map();
}

ixp_memory::~ixp_memory(){
	if (dfas!=NULL || hfa!=NULL){
		delete [] scratch_base;
		delete [] sram_base;
		delete [] dram_base;
		delete [] num_full_states;
		delete [] num_compressed_states;
		delete [] num_compressed_tx;
		delete [] size;
		for (int i=0;i<num_dfas;i++) delete [] state_address[i];
		if (hfa!=NULL){
			delete mapping;
			for (int i=0;i<num_dfas;i++) delete dfas[i];
			free(dfas);
		}
	}else{
		delete [] state_address[0];
		delete [] stable;
	}
	free(state_address);
	fclose(scratch_map);
	fclose(sram_map);
	fclose(dram_map);
}

void ixp_memory::print_layout(){
	printf("\nMEMORY LAYOUT:\n");
	printf("SCRATCH:: base=0x%x, offset=0x%x, size=0x%x\n",SCRATCH_BASE,SCRATCH_BASE+SCRATCH_SIZE,SCRATCH_SIZE);
	printf("SRAM   :: base=0x%x, offset=0x%x, size=0x%x\n",SRAM_BASE,SRAM_BASE+SRAM_SIZE,SRAM_SIZE);
	printf("DRAM   :: base=0x%x, offset=0x%x, size=0x%x\n",DRAM_BASE,DRAM_BASE+DRAM_SIZE,DRAM_SIZE);
	for (int i=0;i<num_dfas;i++){
		printf("DFA #%d,states=%d: scratch_base=0x%x, sram_base=0x%x, dram_base=0x%x, size=%ldB/%ldKB\n",
				i,dfas[i]->size,scratch_base[i],sram_base[i],dram_base[i],size[i],size[i]/1024);
	}
	printf("last address: 0x%x\n",last_addr);
	printf("\n");
}

/* DFA */
void ixp_memory::compute_dfa_memory_layout(){
	
	/* initially give each DFA the same scratchpad share */
	scratch_base[0]=SCRATCH_BASE;
	for (int i=1;i<num_dfas;i++) scratch_base[i]=scratch_base[i-1]+SCRATCH_SIZE/num_dfas;
	
	/* initially give each DFA the same sram share - start after the configuration */
	sram_base[0]=SRAM_BASE + 260 + 12 * num_dfas; //256+4=alphabet_size+alphabet_tx
	for (int i=1;i<num_dfas;i++) sram_base[i]=sram_base[i-1]+(SRAM_SIZE-(sram_base[0]-SRAM_BASE))/num_dfas;
	
	/* NOTE: delay DRAM to later: it may not be necessary to use it! */
	
	if (DEBUG) print_layout();
	
	/* state analysis */
	unsigned base_addr,limit,where;
	unsigned dram_base_addr=DRAM_BASE;
	list <state_t> *queue=new list<state_t>();
	//EXTERNAL LOOP: for each DFA
	for (int i=0;i<num_dfas;i++){
		//initialize addresses
		where=SCRATCH;
		base_addr=scratch_base[i];
		if (i<num_dfas-1) limit=scratch_base[i+1];
		else limit=SCRATCH_BASE+SCRATCH_SIZE-4*(SRAM_WORD_READ-1); 
		//get DFA
		compDFA *dfa=dfas[i];
		
		//LOOP: compute state addresses 
		dfa->mark(0);
		queue->push_back(0);
		if (dfa->labeled_tx[0]->size()!=dfa->alphabet_size) fatal("state 0 is not fully specified!");
			
		while(!queue->empty()){
			state_t s=queue->front(); queue->pop_front();
			sorted_tx_list *tx=dfa->labeled_tx[s];
			/* FULLY SPECIFIED STATE */
			if (tx->size()>TX_THRESHOLD){
				num_full_states[i]++;
				while (base_addr+4*dfa->alphabet_size>=limit){
					if(where==SCRATCH){
						where=SRAM;
						size[i]+=base_addr-scratch_base[i];
						if (base_addr==scratch_base[i]){
							scratch_base[i]=0xffffffff;
							if (DEBUG) printf("NO scratchpad for DFA %d..\n",i);
						}
						if (i<num_dfas-1){
							limit=sram_base[i+1];
							scratch_base[i+1]=base_addr;
							if (DEBUG) printf("shifting scratch_base for DFA %d..\n",i+1);
						}else{
							if (DEBUG) printf("%dB in scratchpad lost on last DFA\n",limit-base_addr);
							limit=SRAM_BASE+SRAM_SIZE-4*(SRAM_WORD_READ-1);
						}
						base_addr=sram_base[i];
					}else if (where==SRAM){
						where=DRAM;
						size[i]+=base_addr-sram_base[i];
						if (base_addr==sram_base[i]){
							sram_base[i]=0xffffffff;
							if (DEBUG) printf("NO SRAM for DFA %d..\n",i);
						}
						if (i<num_dfas-1){
							sram_base[i+1]=base_addr;
							if (DEBUG) printf("shifting sram_base for DFA %d..\n",i+1);
						}else if (DEBUG) printf("%dB in SRAM lost on last DFA\n",limit-base_addr);
						limit=DRAM_BASE+DRAM_SIZE-4*(DRAM_WORD_READ-1);
						base_addr=dram_base_addr;							
					}else{
						fatal("DRAM is FULL!");
					}
				}
				state_address[i][s]=base_addr;
				base_addr=base_addr+4*dfa->alphabet_size;
			
		    /* COMPRESSED STATE */	
			}else{
				num_compressed_states[i]++;
				num_compressed_tx[i]+=(tx->size()+1);
				while (base_addr+4*(tx->size()+1)>=limit){ 
					if(where==SCRATCH){
						where=SRAM;
						size[i]+=base_addr-scratch_base[i];
						if (i<num_dfas-1){
							limit=sram_base[i+1];
							scratch_base[i+1]=base_addr;
							if (DEBUG) printf("shifting scratch_base for DFA %d..\n",i+1);
						}else{
							if (DEBUG) printf("%dB in scratchpad lost on last DFA\n",limit-base_addr);
							limit=SRAM_BASE+SRAM_SIZE-4*(SRAM_WORD_READ-1);
						}
						base_addr=sram_base[i];
					}else if (where==SRAM){
						where=DRAM;
						size[i]+=base_addr-sram_base[i];
						if (i<num_dfas-1){
							sram_base[i+1]=base_addr;
							if (DEBUG) printf("shifting sram_base for DFA %d..\n",i+1);
						}else if (DEBUG) printf("%dB in SRAM lost on last DFA\n",limit-base_addr);
						limit=DRAM_BASE+DRAM_SIZE-4*(DRAM_WORD_READ-1);
						base_addr=dram_base_addr;
					}else{
						fatal("DRAM is FULL!");
					}
				}
				state_address[i][s]=base_addr;
				base_addr=base_addr+4*(tx->size()+1);
			}
			//enqueue the connected states (breath-first traversal)
			while (tx!=NULL && !tx->empty()){
				state_t t=tx->state();
				if (!dfa->marking[t]){
					dfa->mark(t);
					queue->push_back(t);
				}
				tx=tx->next();
			}
		}//end LOOP
		
		dfa->reset_marking();
		if (i<num_dfas-1){
			if (where==SCRATCH){
				if (DEBUG) printf("shifting scratch_base and sram_base for DFA %d..\n",i+1);
				sram_base[i]=0xFFFFFFFF;
				scratch_base[i+1]=base_addr;
				sram_base[i+1]=sram_base[i];
			}else if (where==SRAM){
				sram_base[i+1]=base_addr;
				if (DEBUG) printf("shifting sram_base for DFA %d..\n",i+1);
			}else{
				dram_base_addr=base_addr;
			}
		}
		if (where==SCRATCH) size[i]+=base_addr-scratch_base[i];
		else if (where==SRAM) size[i]+=base_addr-sram_base[i];
		else size[i]+=base_addr-dram_base[i];
		
	}//end EXTERNAL LOOP (for each DFA)
	last_addr=base_addr;
	
	if (VERBOSE) print_layout();
}

void ixp_memory::compute_dfa_memory_map(){
	printf("computing memory map...\n");
	int address;
	
	/* print configuration (in SRAM) */
	address = SRAM_BASE;
	//alphabet size
	fprintf(sram_map,"0x%x 0x%x\n",address,alphabet); address+=4;
	//alphabet
	for (int j=0;j<CSIZE;j=j+4){
		int word = ((alph_tx[j] << 24) & 0xFF000000) |
					((alph_tx[j+1] << 16) & 0x00FF0000) |
					((alph_tx[j+2] << 8) & 0x0000FF00) |
					(alph_tx[j+3]  & 0x000000FF);
		fprintf(sram_map,"0x%x 0x%x\n",address,word); address+=4;
	}
	//DFA data
	for (int i=0;i<num_dfas;i++){
		fprintf(sram_map,"0x%x 0x%x\n",address,scratch_base[i]); address+=4;
		fprintf(sram_map,"0x%x 0x%x\n",address,sram_base[i]); address+=4;
		fprintf(sram_map,"0x%x 0x%x\n",address,dram_base[i]); address+=4;
	}
	
	/* print state information */	
	int word;
	for (int i=0;i<num_dfas;i++){
		for(state_t s=0;s<dfas[i]->size;s++){
			address=state_address[i][s];
			sorted_tx_list *tx=dfas[i]->labeled_tx[s];
			
			/* state is fully specified*/
			if (tx->size()>TX_THRESHOLD){
				for (int c=0;c<dfas[i]->alphabet_size;c++){
					state_t t=dfas[i]->next_state(s,c);
					word = state_address[i][t] >> 2;
					if (!dfas[i]->accepted_rules[t]->empty()) word = word | 0x80000000;
					if ((dfas[i]->labeled_tx[t])->size() <= TX_THRESHOLD) word = word | 0x40000000;
					if (state_address[i][s]>=SCRATCH_BASE && state_address[i][s]<SCRATCH_BASE+SCRATCH_SIZE) fprintf(scratch_map,"0x%x 0x%x\n",address,word);
					else if (state_address[i][s]>=SRAM_BASE && state_address[i][s]<SRAM_BASE+SRAM_SIZE) fprintf(sram_map,"0x%x 0x%x\n",address,word);
					else fprintf(dram_map,"0x%x 0x%x\n",address,word);
					address+=4;
				}
				
			/* state is compressed */		
			}else{
				//inserting the default transition first
				word = (state_address[i][dfas[i]->default_tx[s]]) >> 2;
				if ((dfas[i]->labeled_tx[dfas[i]->default_tx[s]])->size() <= TX_THRESHOLD) word = word | 0x40000000;
				if (tx->empty()) word = word | 0x80000000;
				if (state_address[i][s]>=SCRATCH_BASE && state_address[i][s]<SCRATCH_BASE+SCRATCH_SIZE) fprintf(scratch_map,"0x%x 0x%x\n",address,word);
				else if (state_address[i][s]>=SRAM_BASE && state_address[i][s]<SRAM_BASE+SRAM_SIZE) fprintf(sram_map,"0x%x 0x%x\n",address,word);
				else fprintf(dram_map,"0x%x 0x%x\n",address,word);
				address+=4;
				//if any, insert all the labeled transitions
				while (tx!=NULL && !tx->empty()){
					word = 0;
					int diff_addr;
					if (tx->next()==NULL) word = word | 0x80000000; //end of state marker
					if (!dfas[i]->accepted_rules[tx->state()]->empty()) word = word | 0x40000000; //match marker
					word = word | ((tx->c() & 0x000000FF) << 22); //symbol
					if (state_address[i][tx->state()] >= SCRATCH_BASE && state_address[i][tx->state()] < SCRATCH_BASE+SCRATCH_SIZE){ //where bits
						diff_addr=state_address[i][tx->state()]-scratch_base[i];
					}
					else if (state_address[i][tx->state()] >= SRAM_BASE && state_address[i][tx->state()] < SRAM_BASE+SRAM_SIZE){
						diff_addr=state_address[i][tx->state()]-sram_base[i];
						word = word | (0x01 << 20);
					}
					else{
						diff_addr=state_address[i][tx->state()]-dram_base[i];
						word = word | (0x02 << 20);
					}
					if ((dfas[i]->labeled_tx[tx->state()])->size()<= TX_THRESHOLD) word = word | 0x00080000;
					word = word | ((diff_addr >> 2) & 0x0007FFFF); //differential word address
					if (state_address[i][s]>=SCRATCH_BASE && state_address[i][s]<SCRATCH_BASE+SCRATCH_SIZE) fprintf(scratch_map,"0x%x 0x%x\n",address,word);
					else if (state_address[i][s]>=SRAM_BASE && state_address[i][s]<SRAM_BASE+SRAM_SIZE) fprintf(sram_map,"0x%x 0x%x\n",address,word);
					else fprintf(dram_map,"0x%x 0x%x\n",address,word);
					address+=4;
					
					tx=tx->next();
				}
			}
		}
	}
}

/* NFA */
void ixp_memory::compute_nfa_memory_layout(){

	int where = SCRATCH;
	unsigned base_addr=SCRATCH_BASE;
	unsigned limit=SCRATCH_BASE+SCRATCH_SIZE-4*(SRAM_WORD_READ-1); 
	unsigned memory_size=0;
	
	/* state analysis */
	list <state_t> *queue=new list<state_t>();

	//LOOP: compute state addresses 
	nfa->mark(0);
	queue->push_back(0);
		
	while(!queue->empty()){
		state_t s=queue->front(); queue->pop_front();
		sorted_tx_list *tx=nfa->tx[s];
		
		//check if the state is stable
		unsigned self_tx=0;
		FOREACH_SORTEDLIST(tx,sl) if (sl->state()==s) self_tx++;
		if (self_tx==alphabet) stable[s]=true;
		else self_tx=0;
		
		while (base_addr+4*(tx->size()-self_tx)>=limit){ 
			if(where==SCRATCH){
				where=SRAM;
				if (DEBUG) printf("%dB in scratchpad lost on last DFA\n",limit-base_addr);
				limit=SRAM_BASE+SRAM_SIZE-4*(SRAM_WORD_READ-1);
				memory_size=memory_size+(base_addr-SCRATCH_BASE);
				base_addr=SRAM_BASE+264;
			}else if (where==SRAM){
				where=DRAM;
				if (DEBUG) printf("%dB in SRAM lost on last DFA\n",limit-base_addr);
				limit=DRAM_BASE+DRAM_SIZE-4*(DRAM_WORD_READ-1);
				memory_size=memory_size+(base_addr-(SRAM_BASE+264));
				base_addr=DRAM_BASE;
			}else{
				fatal("DRAM is FULL!");
			}
		}
		state_address[0][s]=base_addr;
		base_addr=base_addr+4*(tx->size()-self_tx);
		
		//enqueue the connected states (breath-first traversal)
		while (tx!=NULL && !tx->empty()){
			state_t t=tx->state();
			if (!nfa->marking[t]){
				nfa->mark(t);
				queue->push_back(t);
			}
			tx=tx->next();
		}
	}//end LOOP
		
	if (where==SCRATCH) memory_size+=(base_addr-SCRATCH_BASE);
	else if (where==SRAM) memory_size+=(base_addr-(SRAM_BASE+264));
	else memory_size+=(base_addr-DRAM_BASE);
	
	printf("memory size: %dB/%dKB\n",memory_size,memory_size/1024);
	
	delete queue;
}

void ixp_memory::compute_nfa_memory_map(){
	printf("computing memory map...\n");
	int address;
	
	/* print configuration (in SRAM) */
	address = SRAM_BASE;
	//alphabet size
	fprintf(sram_map,"0x%x 0x%x\n",address,alphabet); address+=4;
	//alphabet
	for (int j=0;j<CSIZE;j=j+4){
		int word = ((alph_tx[j] << 24) & 0xFF000000) |
					((alph_tx[j+1] << 16) & 0x00FF0000) |
					((alph_tx[j+2] << 8) & 0x0000FF00) |
					(alph_tx[j+3]  & 0x000000FF);
		fprintf(sram_map,"0x%x 0x%x\n",address,word); address+=4;
	}
	//entry state
	int word = state_address[0][0] >> 2;
	if (!nfa->accepted_rules[0]->empty()) word = word | 0x80000000;
	if (stable[0]) word = word | 0x40000000;
	fprintf(sram_map,"0x%x 0x%x\n",address,word); address+=4;
	
	/* print state information */	
	for(state_t s=0;s<nfa->size;s++){
		address=state_address[0][s];
		sorted_tx_list *tx_list=nfa->tx[s];
		if (stable[s]){
			tx_list = new sorted_tx_list();
			FOREACH_SORTEDLIST(nfa->tx[s],tx) if (tx->state()!=s) tx_list->insert(tx->c(),tx->state());
		}
		FOREACH_SORTEDLIST(tx_list,tx){ 
			word = 0;
			if (!nfa->accepted_rules[tx->state()]->empty()) word = word | 0x80000000; //accepting state marker
			if (stable[tx->state()]) word = word | 0x40000000; 						  //stable market
			if (tx->next()==NULL) word = word | 0x20000000; 						  //end of state marker
			word = word | ((tx->c() & 0x000000FF) << 21); //symbol
			int diff_addr;
			if (state_address[0][tx->state()] >= SCRATCH_BASE && state_address[0][tx->state()] < SCRATCH_BASE+SCRATCH_SIZE){ //where bits
				diff_addr=state_address[0][tx->state()]-SCRATCH_BASE;
			}
			else if (state_address[0][tx->state()] >= SRAM_BASE && state_address[0][tx->state()] < SRAM_BASE+SRAM_SIZE){
				diff_addr=state_address[0][tx->state()]-SRAM_BASE;
				word = word | (0x1 << 20); //in-SRAM indicator
			}
			else{
				fatal("NFA in DRAM");
			}
			word = word | ((diff_addr >> 2) & 0x000FFFFF); //differential word address
			if (address>=SCRATCH_BASE && address<SCRATCH_BASE+SCRATCH_SIZE) fprintf(scratch_map,"0x%x 0x%x\n",address,word);
			else if (address>=SRAM_BASE && address<SRAM_BASE+SRAM_SIZE) fprintf(sram_map,"0x%x 0x%x\n",address,word);
			else fprintf(dram_map,"0x%x 0x%x\n",address,word);
			address+=4;
		}
		if (stable[s]) delete tx_list;	
	}
}

/* HFA */
void ixp_memory::compute_hfa_memory_layout(){
	
	//border
	map <state_t, nfa_set*> *border=hfa->get_border();
	
	/* only the head-DFA will have states in SCRATCH */
	scratch_base[0]=SCRATCH_BASE;
	for (int i=1;i<num_dfas;i++) scratch_base[i]=0xffffffff;
	
	/* initially:
	 * - head has 2MB in SRAM
	 * - tail-DFAs have an equal share of the rest 
	 */
	sram_base[0]=SRAM_BASE + 260 + (num_dfas-1)*4+num_dfas*8; //alphabet_size/alph_tx+mapping for each tail+sram_base,dram_base for each DFA
	unsigned available_sram=SRAM_SIZE-(sram_base[0]-SRAM_BASE);
	sram_base[1]=sram_base[0]+0x200000;
	for (int i=2;i<num_dfas;i++) sram_base[i]=sram_base[i-1]+(available_sram-0x200000)/(num_dfas-1);
	
	/* NOTE: delay DRAM to later: it may not be necessary to use it! */
	
	if (DEBUG) print_layout();
	
	/* state analysis */
	unsigned base_addr,limit,where;
	unsigned dram_base_addr=DRAM_BASE;
	list <state_t> *queue=new list<state_t>();
	//EXTERNAL LOOP: for each DFA
	for (int i=0;i<num_dfas;i++){
		//initialize addresses
		if(i==0){
			where=SCRATCH;
			base_addr=scratch_base[i];
			limit=SCRATCH_BASE+SCRATCH_SIZE-4*(SRAM_WORD_READ-1);
		}
		else{
			where=SRAM;
			base_addr=sram_base[i];
			if (i<num_dfas-1) limit=sram_base[i+1];
			else limit=SRAM_BASE+SRAM_SIZE-4*(SRAM_WORD_READ-1);
		} 
		
		//get DFA
		compDFA *dfa=dfas[i];
		
		//LOOP: compute state addresses 
		dfa->mark(0);
		queue->push_back(0);
		if (dfa->labeled_tx[0]->size()!=dfa->alphabet_size) fatal("state 0 is not fully specified!");
			
		while(!queue->empty()){
			state_t s=queue->front(); queue->pop_front();
			sorted_tx_list *tx=dfa->labeled_tx[s];
			unsigned state_size=0;
			if (i==0){
				if ((*border)[s]!=NULL) state_size=(*border)[s]->size()*4;
			}
			/* FULLY SPECIFIED STATE */
			if (tx->size()>TX_THRESHOLD){
				num_full_states[i]++;
				state_size+=4*dfa->alphabet_size;
				while (base_addr+state_size>=limit){
					if(where==SCRATCH){
						where=SRAM;
						size[i]+=(base_addr-scratch_base[i]);
						if (base_addr==scratch_base[i]){
							scratch_base[i]=0xffffffff;
							if (DEBUG) printf("NO scratchpad for DFA %d..\n",i);
						}
						/*
						if (i<num_dfas-1){
							limit=sram_base[i+1];
							scratch_base[i+1]=base_addr;
							if (DEBUG) printf("shifting scratch_base for DFA %d..\n",i+1);
						}else{
							if (DEBUG) printf("%dB in scratchpad lost on last DFA\n",limit-base_addr);
							limit=SRAM_BASE+SRAM_SIZE-4*(SRAM_WORD_READ-1);
						}
						*/
						if (DEBUG) printf("%dB in scratchpad lost on DFA\n",SCRATCH_SIZE+SCRATCH_BASE-base_addr);
					    limit=sram_base[i+1];
						base_addr=sram_base[i];
					}else if (where==SRAM){
						where=DRAM;
						size[i]+=(base_addr-sram_base[i]);
						if (base_addr==sram_base[i]){
							sram_base[i]=0xffffffff;
							if (DEBUG) printf("NO SRAM for DFA %d..\n",i);
						}
						if (i<num_dfas-1){
							sram_base[i+1]=base_addr;
							if (DEBUG) printf("shifting sram_base for DFA %d..\n",i+1);
						}else if (DEBUG) printf("%dB in SRAM lost on last DFA\n",limit-base_addr);
						limit=DRAM_BASE+DRAM_SIZE-4*(DRAM_WORD_READ-1);
						base_addr=dram_base_addr;							
					}else{
						fatal("DRAM is FULL!");
					}
				}
				state_address[i][s]=base_addr;
				base_addr=base_addr+state_size;
			
		    /* COMPRESSED STATE */	
			}else{
				num_compressed_states[i]++;
				num_compressed_tx[i]+=(tx->size()+1);
				state_size+=4*(tx->size()+1);
				while (base_addr+state_size>=limit){ 
					if(where==SCRATCH){
						where=SRAM;
						size[i]+=(base_addr-scratch_base[i]);
						/*
						if (i<num_dfas-1){
							limit=sram_base[i+1];
							scratch_base[i+1]=base_addr;
							if (DEBUG) printf("shifting scratch_base for DFA %d..\n",i+1);
						}else{
							if (DEBUG) printf("%dB in scratchpad lost on last DFA\n",limit-base_addr);
							limit=SRAM_BASE+SRAM_SIZE-4*(SRAM_WORD_READ-1);
						}
						*/
						if (DEBUG) printf("%dB in scratchpad lost on DFA\n",SCRATCH_SIZE+SCRATCH_BASE-base_addr);
						limit=sram_base[i+1];
						base_addr=sram_base[i];
					}else if (where==SRAM){
						where=DRAM;
						size[i]+=(base_addr-sram_base[i]);
						if (i<num_dfas-1){
							sram_base[i+1]=base_addr;
							if (DEBUG) printf("shifting sram_base for DFA %d..\n",i+1);
						}else if (DEBUG) printf("%dB in SRAM lost on last DFA\n",limit-base_addr);
						limit=DRAM_BASE+DRAM_SIZE-4*(DRAM_WORD_READ-1);
						base_addr=dram_base_addr;
					}else{
						fatal("DRAM is FULL!");
					}
				}
				state_address[i][s]=base_addr;
				base_addr=base_addr+state_size;
			}
			//enqueue the connected states (breath-first traversal)
			while (tx!=NULL && !tx->empty()){
				state_t t=tx->state();
				if (i!=0 && t==dfas[i]->get_dead_state()) dfa->mark(t); //dead state don't take room in memory
				if (!dfa->marking[t]){
					dfa->mark(t);
					queue->push_back(t);
				}
				tx=tx->next();
			}
		}//end LOOP
		
		dfa->reset_marking();
		if (i<num_dfas-1){
			if (where==SCRATCH){
				/*
				if (DEBUG) printf("shifting scratch_base and sram_base for DFA %d..\n",i+1);
				sram_base[i]=0xFFFFFFFF;
				scratch_base[i+1]=base_addr;
				sram_base[i+1]=sram_base[i];
				*/
			}else if (where==SRAM){
				sram_base[i+1]=base_addr;
				if (DEBUG) printf("shifting sram_base for DFA %d..\n",i+1);
			}else{
				dram_base_addr=base_addr;
			}
		}
		if (where==SCRATCH) size[i]+=(base_addr-scratch_base[i]);
		else if (where==SRAM) size[i]+=(base_addr-sram_base[i]);
		else size[i]+=(base_addr-dram_base[i]);
		
	}//end EXTERNAL LOOP (for each DFA)
	last_addr=base_addr;
	
	if (VERBOSE) print_layout();
}

void ixp_memory::compute_hfa_memory_map(){
	
	//border
	map <state_t, nfa_set*> *border=hfa->get_border();	
	
	printf("computing memory map...\n");
	int address;
	
	/* print configuration (in SRAM) */
	address = SRAM_BASE;
	
	//alphabet size
	fprintf(sram_map,"0x%x 0x%x\n",address,alphabet); address+=4;
	
	//alphabet
	for (int j=0;j<CSIZE;j=j+4){
		int word = ((alph_tx[j] << 24) & 0xFF000000) |
					((alph_tx[j+1] << 16) & 0x00FF0000) |
					((alph_tx[j+2] << 8) & 0x0000FF00) |
					(alph_tx[j+3]  & 0x000000FF);
		fprintf(sram_map,"0x%x 0x%x\n",address,word); address+=4;
	}
	
	//number of tails
	fprintf(sram_map,"0x%x 0x%x\n",address,num_dfas-1); address+=4;
	
	//mapping
	for (int i=1;i<num_dfas;i++){
		unsigned word = state_address[i][0] >> 2;
		fprintf(sram_map,"0x%x 0x%x\n",address,word); address+=4;
	}
	
	//for each DFA, sram_base and dram_base
	for (int i=0;i<num_dfas;i++){
		fprintf(sram_map,"0x%x 0x%x\n",address,sram_base[i]); address+=4;
		fprintf(sram_map,"0x%x 0x%x\n",address,dram_base[i]); address+=4;
	}
	
	/* print state information */	
	int word;
	for (int i=0;i<num_dfas;i++){
		for(state_t s=0;s<dfas[i]->size;s++){
			if (i!=0 && s==dfas[i]->get_dead_state()) continue; //dead state are not printed
			address=state_address[i][s];
			sorted_tx_list *tx=dfas[i]->labeled_tx[s];
			
			/* if we are in the head-DFA and on a border state, print the border state information */
			if (i==0){
				nfa_set *bs=(*border)[s];
				if (bs!=NULL){
					int b=1;
					FOREACH_SET(bs,it){
						word = (*mapping)[*it];
						if (b==bs->size()) word=word | 0x80000000;
						if (address>=SCRATCH_BASE && address<SCRATCH_BASE+SCRATCH_SIZE) fprintf(scratch_map,"0x%x 0x%x\n",address,word);
						else if (address>=SRAM_BASE && address<SRAM_BASE+SRAM_SIZE) fprintf(sram_map,"0x%x 0x%x\n",address,word);
						else fprintf(dram_map,"0x%x 0x%x\n",address,word);
						address+=4;
						b++;
					}
				}
			}
			
			/* state is fully specified*/
			if (tx->size()>TX_THRESHOLD){
				for (int c=0;c<dfas[i]->alphabet_size;c++){
					state_t t=dfas[i]->next_state(s,c);
					if (i!=0 && t==dfas[i]->get_dead_state()) word=0x1fffffff;
					else{
						word = state_address[i][t] >> 2;
						if (!dfas[i]->accepted_rules[t]->empty()) word = word | 0x80000000; //match indicator
						if ((dfas[i]->labeled_tx[t])->size() <= TX_THRESHOLD) word = word | 0x40000000; //compressed indicator
						if (i==0 && (*border)[t]!=NULL) word = word | 0x08000000; //border indicator
					}
					if (address>=SCRATCH_BASE && address <SCRATCH_BASE+SCRATCH_SIZE) fprintf(scratch_map,"0x%x 0x%x\n",address,word);
					else if (address>=SRAM_BASE && address<SRAM_BASE+SRAM_SIZE) fprintf(sram_map,"0x%x 0x%x\n",address,word);
					else fprintf(dram_map,"0x%x 0x%x\n",address,word);
					address+=4;
				}
				
			/* state is compressed */		
			}else{
				//inserting the default transition first
				if (i!=0 && dfas[i]->default_tx[s]==dfas[i]->get_dead_state()){
					word = 0x1fffffff;
					if (tx->empty()) word = word | 0x80000000; //end of state marker on def tx
				}else{
					word = (state_address[i][dfas[i]->default_tx[s]]) >> 2;
					if (tx->empty()) word = word | 0x80000000; //end of state marker on def tx
					if ((dfas[i]->labeled_tx[dfas[i]->default_tx[s]])->size() <= TX_THRESHOLD) word = word | 0x40000000; //compressed
				}
				
				if (address>=SCRATCH_BASE && address<SCRATCH_BASE+SCRATCH_SIZE) fprintf(scratch_map,"0x%x 0x%x\n",address,word);
				else if (address>=SRAM_BASE && address<SRAM_BASE+SRAM_SIZE) fprintf(sram_map,"0x%x 0x%x\n",address,word);
				else fprintf(dram_map,"0x%x 0x%x\n",address,word);
				address+=4;
				
				//if any, insert all the labeled transitions
				while (tx!=NULL && !tx->empty()){
					word = 0;
					//dead state
					if (i!=0 && tx->state()==dfas[i]->get_dead_state()){
						if (tx->next()==NULL) word = word | 0x10000000; //end of state marker
						word = word | ((tx->c() & 0x0000007F) << 21); //symbol
						word = word | 0x1FFFFF; //dead state indicator
					//regular state	
					}else{
						if (!dfas[i]->accepted_rules[tx->state()]->empty()) word = word | 0x80000000; //match marker
						if ((dfas[i]->labeled_tx[tx->state()])->size()<=TX_THRESHOLD) word = word | 0x40000000;
						if (i==0 && (*border)[tx->state()]!=NULL) word = word | 0x20000000;
						if (tx->next()==NULL) word = word | 0x10000000; //end of state marker
						word = word | ((tx->c() & 0x0000007F) << 21); //symbol
						//differential address
						int diff_addr;
						if (state_address[i][tx->state()] >= SCRATCH_BASE && state_address[i][tx->state()] < SCRATCH_BASE+SCRATCH_SIZE){ //where bits
							diff_addr=state_address[i][tx->state()]-scratch_base[i];
						}
						else if (state_address[i][tx->state()] >= SRAM_BASE && state_address[i][tx->state()] < SRAM_BASE+SRAM_SIZE){
							diff_addr=state_address[i][tx->state()]-sram_base[i];
							word = word | (0x01 << 19);
						}
						else{
							diff_addr=state_address[i][tx->state()]-dram_base[i];
							word = word | (0x02 << 19);
						}					
						word = word | ((diff_addr >> 2) & 0x0007FFFF); //differential word address
					}
				
					//print the entry to memory
					if (address>=SCRATCH_BASE && address<SCRATCH_BASE+SCRATCH_SIZE) fprintf(scratch_map,"0x%x 0x%x\n",address,word);
					else if (address>=SRAM_BASE && address<SRAM_BASE+SRAM_SIZE) fprintf(sram_map,"0x%x 0x%x\n",address,word);
					else fprintf(dram_map,"0x%x 0x%x\n",address,word);
					address+=4;
					
					tx=tx->next();
				}
			}
		}
	}
}
