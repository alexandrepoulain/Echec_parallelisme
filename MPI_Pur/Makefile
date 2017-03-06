target: decide

CFLAGS=-g -O3 -std=c99
CC = mpicc

main.o: main.c projet.h
aux.o: aux.c projet.h


decide: main.o aux.o
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm -f *.o decide
run:
	mpirun -n 4 -hostfile hostfile --map-by node ./decide "/ppp//PPP//7k//7K w"  
