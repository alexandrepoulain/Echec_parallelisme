target: decide

CFLAGS=-g -O3 -std=c99 -fopenmp

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
	./decide "4k//4K//4P w"

