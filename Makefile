all: high_life

high_life: high_life.c
	mpicc high_life.c -o high_life -Wall -O3 -g3