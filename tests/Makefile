
TESTS =  argcargv logging updatetest ktcc-generics ktcc-autoxf ktcc-union
TESTS-CLEAN = $(foreach I,$(TESTS),$(I)-clean)

.PHONY: $(TESTS) clean

all: $(TESTS) 

$(TESTS):
	$(MAKE) -C $@ run-test

##ktcc-uint64:  This is not compatible with cil (Unimplemented: Cannot represent integer 18446744073709551615UL)
##	../bin/bin/ktcc ktcc-uint64.c

clean: $(TESTS-CLEAN)

$(TESTS-CLEAN):
	$(MAKE) -C $(subst -clean,,$@) clean


