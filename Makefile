# NO TOCAR / NOT MODIFIED ME ##
CC=gcc
FLAGS=-Wno-implicit-function-declaration
CFLAGS=-I.
###############################

# MODIFIED ME ##

OBJ = scripter.o
OBJ2 = mygrep.o
TARGETS = scripter mygrep

all: $(TARGETS)

%.o: %.c 
	$(CC) $(FLAGS) -c -o $@ $< $(CFLAGS)

scripter: $(OBJ)
	$(CC) $(FLAGS) -L. -o $@ $< $(LIBS)

mygrep: $(OBJ2)
	$(CC) $(FLAGS) -L. -o $@ $< $(LIBS)
clean:
	rm -f *.o $(TARGETS)
