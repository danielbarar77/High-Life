#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>

#define OFF 0
#define ON 1

int H; // number of lines
int W; // number of columns

void getArgs(int argc, char **argv, char **inFile, char **outFile, int *steps)
{
    if (argc < 4)
    {
        printf("Error! Usage: ./homework IN_FILENAME OUT_FILENAME NUM_STEPS\n");
        exit(1);
    }
    (*inFile) = (char *)calloc(strlen(argv[1]), sizeof(char));
    (*outFile) = (char *)calloc(strlen(argv[2]), sizeof(char));
    memcpy((*inFile), argv[1], strlen(argv[1]));
    memcpy((*outFile), argv[2], strlen(argv[2]));
    // (*inFile)[strlen(argv[1])] = 0;
    // (*outFile)[strlen(argv[2])] = 0;
    (*steps) = atoi(argv[3]);
}

int main(int argc, char **argv)
{
    int rank;
    int nProcesses;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);
    char *inFile, *outFile;
    int steps;
    getArgs(argc, argv, &inFile, &outFile, &steps);

    if (rank == 0)
    {
        printf("inFile:%s\n", inFile);
        printf("outFile:%s\n", outFile);
        printf("steps :%d\n", steps);
    }
    else
    {
    }

    MPI_Finalize();
    return 0;
}
