# Pick your favorite compiler
CC = g++
OPT = -O3

INC = -I.

SIM_LINUX = -DSIM_LINUX
ENDIAN = -DBYTES_LITTLE_ENDIAN -DWORDS_LITTLE_ENDIAN

CFLAGS = $(OPT) $(INC) $(SIM_LINUX) $(ENDIAN)

SRC = histogram.cc
OBJ = histogram.o 


all: libUTILS.a

libUTILS.a: $(OBJ)
	ar r libUTILS.a $(OBJ)

.cc.o:
	$(CC) $(CFLAGS) -c $*.cc


clean:
	rm -f *.o libUTILS.a
