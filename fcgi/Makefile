# Makefile Boolean Cube Calculator


COPATH = ../../c-object/co
BCPATH = ../src

CFLAGS = -g -march=silvermont  -Wall  -I$(COPATH) -I$(BCPATH) -I. -I/usr/include/fastcgi

# CFLAGS = -g -march=silvermont  -Wall  -I$(COPATH) -I.            
# gcov -b 
# lcov --capture --directory . --output-file coverage.info
# genhtml coverage.info --output-directory out

# CFLAGS += -pg -no-pie -fno-omit-frame-pointer -fno-inline-functions -fno-inline-functions-called-once -fno-optimize-sibling-calls

# CFLAGS = -g -Wall -march=silvermont -fsanitize=address -I$(COPATH) -I.
#CFLAGS = -g -Wall -march=silvermont -I$(COPATH) -I.

SRC = $(wildcard $(BCPATH)/*.c)
SRC += $(COPATH)/co.c 
SRC += fcgi_bcc.c

OBJ = $(SRC:.c=.o)

bcc.fcgi: $(OBJ) 
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@  -lfcgi -lpthread -lrt

clean:	
	-rm $(OBJ) bcc.fcgi


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
	./bcc -test
