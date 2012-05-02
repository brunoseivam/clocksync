IDIR =../include
CC=gcc
CFLAGS=-I$(IDIR)
CCFLAGS= -Wall -O2

ODIR=obj

LDIR =../lib

LIBS=-lrt

PROG=clocksync
PROGTEST=list_test

.PHONY: all
all: $(PROG) $(PROGTEST)

_DEPS = connection.h pack.h adjtime.h list.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o connection.o pack.o adjtime.o list.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

_DEPSTEST = list.h
DEPSTEST = $(patsubst %,$(IDIR)/%,$(_DEPSTEST))

_OBJTEST = list_test.o list.o
OBJTEST = $(patsubst %,$(ODIR)/%,$(_OBJTEST))
 
$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c $(CCFLAGS) -o $@ $< $(CFLAGS)

$(PROG): $(OBJ)
	$(CC) $(CCFLAGS) -o $@ $^ $(CFLAGS) $(LIBS)

$(ODIR)/lib_test.o: %.c $(DEPSTEST)
	$(CC) -c $(CCFLAGS) -o $@ $< $(CFLAGS)

$(PROGTEST): $(OBJTEST)
	$(CC) $(CCFLAGS) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(PROGTEST) $(PROG) $(ODIR)/*.o *~ core $(INCDIR)/*~ 
