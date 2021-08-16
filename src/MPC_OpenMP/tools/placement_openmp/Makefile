all: main

main: placement.o main.c
	mpc_cc -g -fopenmp  main.c placement.o -o main

placement.o: placement.c placement.h
	mpc_cc -g -c -fopenmp  placement.c 
clean:
	rm *.o main
