target: decide

CFLAGS=-g -O3 -std=c99

main.o: main.c projet.h
aux.o: aux.c projet.h


decide: main.o aux.o
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm -f *.o decide
	
exec:
	./decide "4k//4K//4P w"

