.PHONY: release real

CUDA_OBJ = udfa_gpu udfa_host udfa_main packets

HOST_OBJ = mem_controller common_configs finite_automaton
COMMON_HEADERS = common.h

NVCC=nvcc
SM=sm_35
#SM=sm_52
CXX=g++
CC=gcc

##CUDA_INCLUDE=-I/usr/local/cuda/include
CUDA_INCLUDE=-I/usr/local/cuda-8.0/include

MNRL         = ../mnrl/C++
MNRL_INCLUDE = $(MNRL)/include
MNRL_LIB     = $(MNRL)/lib
JSON11       = $(MNRL_LIB)/json11
VALIJSON     = $(MNRL_LIB)/valijson/include
JSON         = $(MNRL_LIB)/valijson/thirdparty/nlohmann-json-1.1.0

NAME = libmnrl
SNAME = $(NAME).a
DNAME = $(NAME).so
LIBFLAGS = -lmnrl

LDFLAGS = -L$(MNRL) -lmnrl

CXXFLAGS+=$(CUDA_INCLUDE) -Wno-deprecated -I$(MNRL_INCLUDE) -I$(VALIJSON) -I$(JSON) -I$(JSON11) --std=c++11 -fPIC

#NVCCFLAGS+=$(CUDA_INCLUDE) -Xptxas -v -arch ${SM} --compiler-options -Wno-deprecated -lineinfo -I$(MNRL_INCLUDE) -I$(VALIJSON) -I$(JSON) -I$(JSON11) --std=c++11
NVCCFLAGS+=$(CUDA_INCLUDE) -Xptxas -v -arch ${SM} --compiler-options -Wno-deprecated -lineinfo -DTEXTURE_MEM_USE -I$(MNRL_INCLUDE) -I$(VALIJSON) -I$(JSON) -I$(JSON11) --std=c++11

export DYN_LIB=

NVCCFLAGS_REL = $(NVCCFLAGS) -O4
CXXFLAGS_REL = $(CXXFLAGS) -O4

release:
	$(MAKE) -e real NVCCFLAGS="$(NVCCFLAGS_REL)" CXXFLAGS="$(CXXFLAGS_REL)"

real: dfa_engine

$(addsuffix .o, $(HOST_OBJ)) $(addsuffix .o, $(CUDA_OBJ)) : $(COMMON_HEADERS)

$(addsuffix .o, $(HOST_OBJ)) : $(addsuffix .cpp, $(basename $@)) $(addsuffix .h, $(basename $@))

$(addsuffix .o, $(CUDA_OBJ)) : $(addsuffix .cu, $(basename $@)) $(addsuffix .h, $(basename $@))
	${NVCC} $(NVCCFLAGS) -c -o $(addsuffix .o, $(basename $@)) $(addsuffix .cu, $(basename $@))
	
dfa_engine: $(addsuffix .o, $(HOST_OBJ)) $(addsuffix .o, $(CUDA_OBJ))
	${NVCC} $(NVCCFLAGS) -o dfa_engine $(addsuffix .o, $(HOST_OBJ)) $(addsuffix .o, $(CUDA_OBJ)) ${DYN_LIB} $(LDFLAGS)	
	cp $(MNRL)/$(DNAME) ../bin
	cp dfa_engine ../bin
	
clean:
	rm -f *.o dfa_engine ../bin/$(DNAME) ../bin/dfa_engine

