IDIR =../include
CC=gcc
CFLAGS=-I$(IDIR)
CCFLAGS= -Wall -O2

ODIR=obj
LDIR =../lib

LIBS=-lrt

PROG=main

_DEPS = connection.h pack.h adjtime.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o connection.o pack.o adjtime.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c $(CCFLAGS) -o $@ $< $(CFLAGS)

$(PROG): $(OBJ)
	$(CC) $(CCFLAGS) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(PROG) $(ODIR)/*.o *~ core $(INCDIR)/*~ 
