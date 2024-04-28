# Makefile Boolean Cube Calculator


COPATH = ../../c-object/co

# CFLAGS = -g -pg -Wall
# CFLAGS = -O4 -march=silvermont -Wall  -I../c-object/co -I.
# CFLAGS = -g -pg -Wall

CFLAGS = -g -Wall -fsanitize=address -I$(COPATH) -I.
# CFLAGS = -g -Wall  -I$(COPATH) -I.

SRC = bcutil.c bcp.c bcube.c bclcore.c bcselftest.c bccofactor.c 
SRC += bclcontainment.c bcltautology.c bclsubtract.c 
SRC += bclcomplement.c bclsubset.c bclintersection.c
SRC += bclexpand.c bclminimize.c
SRC += bcexpression.c bcldimacscnf.c
SRC += $(COPATH)/co.c 
SRC += bcjson.c main.c

OBJ = $(SRC:.c=.o)

bcc: $(OBJ) 
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:	
	-rm $(OBJ) bcc

