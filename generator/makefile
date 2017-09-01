CC = /usr/bin/g++
#CFLAGS = -Wall -g -O4 -I..
CFLAGS = -g -O4 -I.. 

all:	main main_tracegen main_nfa main_dfas main_ixp main_ixp_nfa main_memory main_memory_regen exe

tcp:	tcpserver tcpclient tcpclients

clean:
	-rm *.o regex* 

.c.o:
	${CC} ${CFLAGS} -c $*.c 

stdinc.o : stdinc.h 
int_set.o : stdinc.h int_set.h
linked_set.o : stdinc.h linked_set.h
wgraph.o : stdinc.h wgraph.h
partition.o : stdinc.h partition.h
dheap.o : stdinc.h dheap.h
dfa.o : stdinc.h dfa.h 
subset.o : stdinc.h subset.h dfa.h nfa.h
nfa.o : stdinc.h nfa.h dfa.h 
hybrid_fa.o : stdinc.h nfa.h dfa.h hybrid_fa.h
trace.o : stdinc.h trace.h 
parser.o :  stdinc.h nfa.h dfa.h parser.h
ixp_memory.o  : ixp_memory.h comp_dfa.h comp_nfa.h stdinc.h
memory.o  : memory.h comp_dfa.h comp_nfa.h stdinc.h
comp_dfa.o  : sorted_tx_list.h comp_dfa.h dfa.h stdinc.h
comp_nfa.o  : sorted_tx_list.h comp_nfa.h nfa.h stdinc.h
tcpclient.o : tcp.h
tcpserver.o : tcp.h
main_memory.o: tcp.h
#//////////////////test
main_memory_regen.o: tcp.h
#//////////////////test
main_pthread.o: tcp.h
#profiling-rdtsc.o: profiling-rdtsc-common.h profiling-rdtsc.h

main: stdinc.o int_set.o linked_set.o dheap.o partition.o wgraph.o dfa.o nfa.o subset.o parser.o trace.o hybrid_fa.o main.o cache.o fa_memory.o dfas_memory.o
	${CC} ${CFLAGS} stdinc.o int_set.o linked_set.o partition.o wgraph.o dheap.o dfa.o nfa.o subset.o parser.o main.o trace.o hybrid_fa.o cache.o fa_memory.o dfas_memory.o -o regex -lpcap

main_dfas: stdinc.o int_set.o linked_set.o dheap.o partition.o wgraph.o cache.o nfa.o dfa.o hybrid_fa.o subset.o parser.o trace.o dfas_memory.o fa_memory.o main_dfas.o 
	${CC} ${CFLAGS} stdinc.o int_set.o linked_set.o dheap.o partition.o wgraph.o cache.o nfa.o dfa.o hybrid_fa.o subset.o parser.o dfas_memory.o fa_memory.o main_dfas.o trace.o -o regex_dfas -lpcap

main_nfa: stdinc.o int_set.o linked_set.o dheap.o partition.o wgraph.o cache.o nfa.o dfa.o hybrid_fa.o subset.o parser.o trace.o dfas_memory.o fa_memory.o main_nfa.o 
	${CC} ${CFLAGS} stdinc.o int_set.o linked_set.o dheap.o partition.o wgraph.o cache.o nfa.o dfa.o hybrid_fa.o subset.o parser.o dfas_memory.o fa_memory.o main_nfa.o trace.o -o regex_nfa -lpcap
	
main_tracegen: stdinc.o int_set.o linked_set.o dheap.o partition.o wgraph.o cache.o nfa.o dfa.o hybrid_fa.o subset.o parser.o trace.o dfas_memory.o fa_memory.o main_tracegen.o 
	${CC} ${CFLAGS} stdinc.o int_set.o linked_set.o dheap.o partition.o wgraph.o cache.o nfa.o dfa.o hybrid_fa.o subset.o parser.o dfas_memory.o fa_memory.o main_tracegen.o trace.o -o regex_tracegen -lpcap

main_ixp: stdinc.o int_set.o linked_set.o dheap.o partition.o wgraph.o cache.o sorted_tx_list.o nfa.o dfa.o hybrid_fa.o comp_dfa.o comp_nfa.o subset.o parser.o ixp_memory.o main_ixp.o 
	${CC} ${CFLAGS} stdinc.o int_set.o linked_set.o dheap.o partition.o wgraph.o cache.o nfa.o sorted_tx_list.o dfa.o comp_dfa.o comp_nfa.o hybrid_fa.o subset.o parser.o ixp_memory.o main_ixp.o -o regex_ixp -lpcap

main_ixp_nfa: stdinc.o int_set.o linked_set.o dheap.o partition.o wgraph.o cache.o sorted_tx_list.o nfa.o dfa.o hybrid_fa.o comp_dfa.o comp_nfa.o subset.o parser.o ixp_memory.o main_ixp_nfa.o 
	${CC} ${CFLAGS} stdinc.o int_set.o linked_set.o dheap.o partition.o wgraph.o cache.o nfa.o sorted_tx_list.o dfa.o comp_dfa.o comp_nfa.o hybrid_fa.o subset.o parser.o ixp_memory.o main_ixp_nfa.o -o regex_ixp_nfa -lpcap

main_memory: stdinc.o int_set.o linked_set.o dheap.o partition.o wgraph.o cache.o sorted_tx_list.o nfa.o dfa.o hybrid_fa.o comp_dfa.o comp_nfa.o subset.o parser.o memory.o main_memory.o 
	${CC} ${CFLAGS} stdinc.o int_set.o linked_set.o dheap.o partition.o wgraph.o cache.o nfa.o sorted_tx_list.o dfa.o comp_dfa.o comp_nfa.o hybrid_fa.o subset.o parser.o memory.o main_memory.o -o regex_memory -lpcap

#//////////////////test
main_memory_regen: stdinc.o int_set.o linked_set.o dheap.o partition.o wgraph.o cache.o sorted_tx_list.o nfa.o dfa.o hybrid_fa.o comp_dfa.o comp_nfa.o subset.o parser.o memory.o main_memory_regen.o 
	${CC} ${CFLAGS} stdinc.o int_set.o linked_set.o dheap.o partition.o wgraph.o cache.o nfa.o sorted_tx_list.o dfa.o comp_dfa.o comp_nfa.o hybrid_fa.o subset.o parser.o memory.o main_memory_regen.o -o regex_memory_regen -lpcap
#//////////////////test
	
main_pthread: stdinc.o int_set.o linked_set.o dheap.o partition.o wgraph.o cache.o sorted_tx_list.o nfa.o dfa.o hybrid_fa.o comp_dfa.o comp_nfa.o subset.o parser.o memory.o main_pthread.o 
	${CC} -pthread ${CFLAGS} stdinc.o int_set.o linked_set.o dheap.o partition.o wgraph.o cache.o nfa.o sorted_tx_list.o dfa.o comp_dfa.o comp_nfa.o hybrid_fa.o subset.o parser.o memory.o main_pthread.o -o regex_pthread -lpcap

tcp_server: tcpserver.o 
	${CC} ${CFLAGS} tcpserver.o - o tcpserver

tcp_client: tcpclient.o 
	${CC} ${CFLAGS} tcpclient.o - o tcpclient

tcp_clients: tcpclients.o 
	${CC} ${CFLAGS} tcpclients.o - o tcpclients

exe:
	-chmod a+x regex*
