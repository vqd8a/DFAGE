.PHONY: all mnrl dfa_engine generator copy

all: mnrl dfa_engine generator copy

mnrl:
	cd mnrl/C++ && $(MAKE)

dfa_engine:
	cd dfa_engine && $(MAKE)
	
generator:
	cd generator && $(MAKE)

copy:
	cp dfa_engine/dfa_engine bin/	
	cp generator/regex_memory bin/
	cp generator/regex_memory_regen bin/
	
clean:
	rm -f bin/dfa_engine bin/regex_memory bin/regex_memory_regen
	cd generator && $(MAKE) clean
	cd dfa_engine && $(MAKE) clean
	cd mnrl/C++ && $(MAKE) clean
