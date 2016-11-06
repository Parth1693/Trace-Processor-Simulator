# Pick your favorite compiler
CC = g++
OPT = -O3
#OPT = -g

INC = -I. -I../UTILS -I../../functional-simulator

SIM_LINUX = -DSIM_LINUX
ENDIAN = -DBYTES_LITTLE_ENDIAN -DWORDS_LITTLE_ENDIAN

CFLAGS = $(OPT) $(INC) $(SIM_LINUX) $(ENDIAN)

SRC = dcache.cc
OBJ = dcache.o


all: libCACHE.a

libCACHE.a: $(OBJ)
	ar r libCACHE.a $(OBJ)

.cc.o:
	$(CC) $(CFLAGS) -c $*.cc


clean:
	rm -f *.o libCACHE.a
