# Makefile Boolean Cube Calculator


COPATH = ../../c-object/co
BCPATH = ../src
BC_EXT ?= 2

CFLAGS = -g -Wall -I$(COPATH) -I$(BCPATH) -I. -DBC_EXT=$(BC_EXT)

ifeq ($(BC_EXT),1)
CFLAGS += -msse2 -march=silvermont
else ifeq ($(BC_EXT),2)
CFLAGS += -mavx2
else ifeq ($(BC_EXT),3)
CFLAGS += -mavx512f -mavx512bw
endif

# CFLAGS = -g -march=silvermont  -Wall  -I$(COPATH) -I.            
# gcov -b 
# lcov --capture --directory . --output-file coverage.info
# genhtml coverage.info --output-directory out

# CFLAGS += -pg -no-pie -fno-omit-frame-pointer -fno-inline-functions -fno-inline-functions-called-once -fno-optimize-sibling-calls

# CFLAGS = -g -Wall -march=silvermont -fsanitize=address -I$(COPATH) -I.
#CFLAGS = -g -Wall -march=silvermont -I$(COPATH) -I.

SRC = $(wildcard $(BCPATH)/*.c)
SRC += $(COPATH)/co.c 
SRC += main.c

OBJ = $(SRC:.c=.o)

bcc: $(OBJ) 
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:	
	-rm $(OBJ) bcc


# build bcc with coverage code

coverage-report:
	lcov --capture \
		--directory . \
		--directory $(BCPATH) \
		--directory $(COPATH) \
		--output-file coverage.info
	genhtml coverage.info --output-directory out
	@echo "Coverage report in 'out/' folder."

coverage-exec:
	./bcc -test

coverage: CFLAGS += -fprofile-arcs -ftest-coverage 
coverage: LDFLAGS += -fprofile-arcs -ftest-coverage 
coverage: clean bcc coverage-exec  coverage-report

# memory leak analysis

memory: CFLAGS += -fsanitize=address
memory: LDFLAGS += -fsanitize=address
memory: clean bcc
	./bcc -test
