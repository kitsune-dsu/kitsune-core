# source files.
HEADERS = interval.h
SRC = interval.c
OBJ = $(SRC:.c=.o)
OUT = interval.a

TEST_SRC = interval_test.c
TEST_OUT = interval_test

# include directories
INCLUDE = -I. 

# C compiler flags (-g -O2 -Wall)
CFLAGS = -fPIC

.SUFFIXES: .c

default: $(OUT) $(TEST_OUT)

.c.o: $(HEADERS)
	$(CC) $(INCLUDE) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

$(TEST_OUT): $(TEST_SRC) $(OUT) $(HEADERS)
	$(CC) $(INCLUDE) $(CFLAGS) -o $@ $(TEST_SRC) $(OUT)

clean:
	rm -f $(OBJ) $(OUT) $(TEST_OUT) Makefile.bak