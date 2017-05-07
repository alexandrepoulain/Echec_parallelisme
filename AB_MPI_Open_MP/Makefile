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
exec:
	export OMP_NUM_THREADS=2
	export OMP_SCHEDULE="dynamic"
	export OMP_NESTED=FALSE
	mpirun -n 2 -hostfile hostfile --map-by node ./decide "5k//2K1P b"
