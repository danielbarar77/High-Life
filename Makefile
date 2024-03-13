all: homework

homework: homework.c
	mpicc homework.c -o homework -Wall -O3 -g3