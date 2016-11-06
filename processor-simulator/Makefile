CC = g++
#OPT = -O3
OPT = -g

INC = -I. -I../functional-simulator -I./BP -I./CACHE -I./UTILS 
LIB = -L../functional-simulator -L./BP -L./CACHE -L./UTILS 

SIM_LINUX = -DSIM_LINUX
ENDIAN = -DBYTES_LITTLE_ENDIAN -DWORDS_LITTLE_ENDIAN

#WARN = -Wall
CFLAGS = $(OPT) $(WARN) $(INC) $(LIB) $(SIM_LINUX) $(ENDIAN)

SRC = processor.cc \
      renamer.cc trace_predictor.cc activeList.cc PE.cc \
      trace_predict.cc trace_fetch.cc trace_rename.cc trace_dispatch.cc trace_schedule.cc trace_register_read.cc trace_execute.cc \
      trace_writeback.cc trace_retire.cc \
      trace_cache.cc trace_fill.cc \
      alu.cc lsu.cc squash.cc checker.cc \
      issue_queue.cc lane.cc pipeline_register.cc payload.cc trace_payload.cc \
      simulator.cc parameters.cc globals.cc sim_input.cc sim_output.cc

OBJ = processor.o \
      renamer.o trace_predictor.o activeList.o PE.o \
      trace_predict.o trace_fetch.o trace_rename.o trace_dispatch.o trace_schedule.o trace_register_read.o trace_execute.o \
      trace_writeback.o trace_retire.o \
      trace_cache.o trace_fill.o \
      alu.o lsu.o squash.o checker.o \
      issue_queue.o lane.o pipeline_register.o payload.o trace_payload.o \
      simulator.o parameters.o globals.o sim_input.o sim_output.o

all: $(OBJ)
	$(CC) -o 721sim $(CFLAGS) \
		$(OBJ) -lFUNCSIM -lBP -lCACHE -lUTILS

.cc.o:
	$(CC) $(CFLAGS) -c $*.cc


clean:
	rm -f *.o 721sim

clobber:
	rm -f *.o
