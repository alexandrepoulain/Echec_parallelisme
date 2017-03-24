target: decide

CFLAGS=-g -O3 -std=c99 -fopenmp
CC = mpicc

main.o: main.c projet.h
aux.o: aux.c projet.h


decide: main.o aux.o
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm -f *.o decide
run:
	mpirun -n 4 -hostfile hostfile --map-by node ./decide "4k//4K///4P w"  
