include ../shared.mk

TESTS = array raw ptr ptrarray ntstr ntarray

all: $(TESTS)

.PHONY : clean run-test

$(TESTS): %: test_util.h
	gcc -ldl $@.c $(EKLIB) $(EKINC) -o $@

%-runtest: %
	./$*

run-test: $(patsubst %,%-runtest,$(TESTS))

clean:
	-$(RM) $(TESTS) $(OBJS) *~