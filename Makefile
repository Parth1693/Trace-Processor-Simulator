all: 721sim

721sim:
	@cd functional-simulator; make; cd ..;
	@cd processor-simulator/BP; make; cd ../..;
	@cd processor-simulator/CACHE; make; cd ../..;
	@cd processor-simulator/UTILS; make; cd ../..;
	@cd processor-simulator; make; cd ..;
	@mv processor-simulator/721sim .
	@echo "-----------DONE WITH 721SIM-----------"

clobber:
	rm -f 721sim

clean:
	@cd functional-simulator; make clean; cd ..;
	@cd processor-simulator/BP; make clean; cd ../..;
	@cd processor-simulator/CACHE; make clean; cd ../..;
	@cd processor-simulator/UTILS; make clean; cd ../..;
	@cd processor-simulator; make clean; cd ..;
	@rm -f 721sim
