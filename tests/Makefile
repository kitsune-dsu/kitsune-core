
TESTS =  addrtable argcargv logging transform updatetest ktcc-uint64
TESTS-CLEAN = $(foreach I,$(TESTS),$(I)-clean)

.PHONY: $(TESTS) clean

all: $(TESTS) 

$(TESTS):
	$(MAKE) -C $@ run-test

ktcc-uint64:
	../bin/ktcc ktcc-uint64.c

clean: $(TESTS-CLEAN)

$(TESTS-CLEAN):
	$(MAKE) -C $(subst -clean,,$@) clean


