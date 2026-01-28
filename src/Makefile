# Makefile Boolean Cube Calculator


COPATH = ../../c-object/co

CFLAGS = -g -march=silvermont  -Wall  -I$(COPATH) -I.


# CFLAGS = -g -march=silvermont  -Wall  -I$(COPATH) -I.            
# gcov -b 
# lcov --capture --directory . --output-file coverage.info
# genhtml coverage.info --output-directory out

# CFLAGS += -pg -no-pie -fno-omit-frame-pointer -fno-inline-functions -fno-inline-functions-called-once -fno-optimize-sibling-calls

# CFLAGS = -g -Wall -march=silvermont -fsanitize=address -I$(COPATH) -I.
#CFLAGS = -g -Wall -march=silvermont -I$(COPATH) -I.

SRC = bcutil.c bcp.c bcube.c bclcore.c bcselftest.c bccofactor.c 
SRC += bclcontainment.c bcltautology.c bclsubtract.c 
SRC += bclcomplement.c bclsubset.c bclintersection.c
SRC += bclexpand.c bclminimize.c
SRC += bcexpression.c bclexclude.c bcldimacscnf.c
SRC += $(COPATH)/co.c 
SRC += bcjson.c main.c

OBJ = $(SRC:.c=.o)

bcc: $(OBJ) 
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:	
	-rm $(OBJ) bcc


# build bcc with coverage code

coverage-report:
	lcov --capture --directory . --output-file coverage.info
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
