target: decide

CFLAGS=-g -O3 -std=c99

main.o: main.c projet.h
aux.o: aux.c projet.h


decide: main.o aux.o
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean

run: 
	./decide "/5p/4p/4P/4KP///k w" 

clean:
	rm -f *.o decide 

