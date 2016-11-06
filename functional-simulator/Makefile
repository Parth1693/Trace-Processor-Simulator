# Pick your favorite compiler
CC = g++

# Choose your optimization level here
OPT  = -g -O3 -finline-functions -funroll-loops
OPT2 = -g -O
#OPT  = -O3 -finline-functions -funroll-loops
#OPT2 = -O

SIM_LINUX = -DSIM_LINUX
ENDIAN = -DBYTES_LITTLE_ENDIAN -DWORDS_LITTLE_ENDIAN

CFLAGS  = $(OPT)  $(SIM_LINUX) $(ENDIAN)
CFLAGS2 = $(OPT2) $(SIM_LINUX) $(ENDIAN)

OBJ = main.o syscall.o memory.o regs.o loader.o ss.o endian.o misc.o \
      arch.o func_sim.o info.o sim_main.o


all: libFUNCSIM.a

libFUNCSIM.a: $(OBJ)
	ar r libFUNCSIM.a $(OBJ)

func_sim.o: func_sim.cc
	$(CC) $(CFLAGS2) -c $*.cc
	
endian.o: endian.cc
	$(CC) $(CFLAGS2) -c $*.cc
	
.cc.o:
	$(CC) $(CFLAGS) -c $*.cc



clean:
	rm -f *.o libFUNCSIM.a
