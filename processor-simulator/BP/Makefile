# Pick your favorite compiler
CC = g++
OPT = -O3

INC = -I. -I.. -I../../functional-simulator

SIM_LINUX = -DSIM_LINUX
ENDIAN = -DBYTES_LITTLE_ENDIAN -DWORDS_LITTLE_ENDIAN

CFLAGS = $(OPT) $(INC) $(SIM_LINUX) $(ENDIAN)

SRC = bpred_interface.cc
OBJ = bpred_interface.o


all: libBP.a

libBP.a: $(OBJ)
	ar r libBP.a $(OBJ)

.cc.o:
	$(CC) $(CFLAGS) -c $*.cc


clean:
	rm -f *.o libBP.a
