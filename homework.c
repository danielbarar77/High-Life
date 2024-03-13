#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include <time.h>

#define OFF 0
#define ON 1

int H; // number of lines
int W; // number of columns

void getArgs(int argc, char **argv, char **inFile, char **outFile, int *totalSteps)
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
    (*totalSteps) = atoi(argv[3]);
}

void allocMatrix(int ***matrix, int l, int c)
{
    int *temp = (int *)calloc(l * c, sizeof(int));
    if (!temp)
    {
        printf("Error for allocating contiguous memeory!\n");
        exit(1);
    }
    (*matrix) = (int **)calloc(l, sizeof(int *));
    if (!(*matrix))
    {
        printf("Error for allocating matrix lines memeory!\n");
        exit(1);
    }
    for (int i = 0; i < l; i++)
    {
        (*matrix)[i] = &(temp[i * c]);
        if (!((*matrix)[i]))
        {
            printf("Error for matrix pointing to vector memory!\n");
            exit(1);
        }
    }
}

void getInfo(int **linesPerProcess, int **indexes, int **sendCount, int **displs, int nProcesses, int lines, int columns)
{
    for (int i = 0; i < nProcesses; i++)
    {
        (*linesPerProcess)[i] = lines / nProcesses;
        if (i < (lines % nProcesses))
        {
            (*linesPerProcess)[i]++;
        }

        (*sendCount)[i] = (*linesPerProcess)[i] * columns;

        if (i == 0)
        {
            (*indexes)[i] = 0;
            (*displs)[i] = 0;
        }
        else
        {
            (*indexes)[i] = (*indexes)[i - 1] + (*linesPerProcess)[i - 1];
            (*displs)[i] = (*displs)[i - 1] + (*sendCount)[i - 1];
        }
    }
}

int readMatrix(int ***matrix, char *inFile, int *lines, int *columns)
{
    FILE *fin = fopen(inFile, "r");
    if (!fin)
    {
        return -1;
    }
    if (fscanf(fin, "%d", lines) != 1)
    {
        printf("Ops! Something went wrong!\n");
    }
    if (fscanf(fin, "%d", columns) != 1)
    {
        printf("Ops! Something went wrong!\n");
    }
    allocMatrix(matrix, *lines, *columns);

    for (int i = 0; i < (*lines); i++)
    {
        for (int j = 0; j < (*columns); j++)
        {
            if (fscanf(fin, "%d", &((*matrix)[i][j])) != 1)
            {
                printf("Ops! Something went wrong!\n");
            }
        }
    }

    fclose(fin);
    return 0;
}

int main(int argc, char **argv)
{
    int rank;
    int nProcesses;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);
    char *inFile, *outFile;
    int totalSteps;
    getArgs(argc, argv, &inFile, &outFile, &totalSteps);
    int **matrix;
    int lines, columns;
    int *sendCount = (int *)calloc(nProcesses, sizeof(int));
    int *displs = (int *)calloc(nProcesses, sizeof(int));
    MPI_Status status;

    clock_t start, end;
    double cpu_time_used;
    if (rank == 0)
    {
        if (readMatrix(&matrix, inFile, &lines, &columns) == -1)
        {
            printf("Error on opening input file!\n");
            // MPI_Abort(MPI_COMM_WORLD);
            return -1;
        }

        // printf("inFile:%s\n", inFile);
        // printf("outFile:%s\n", outFile);
        // printf("totalSteps :%d\n", totalSteps);
        // for (int i = 0; i < lines; i++)
        // {
        //     for (int j = 0; j < columns; j++)
        //     {
        //         printf("%d ", matrix[i][j]);
        //     }
        //     printf("\n");
        // }

        MPI_Request request;
        int *linesPerProcess = (int *)calloc(nProcesses, sizeof(int));
        int *indexes = (int *)calloc(nProcesses, sizeof(int));

        getInfo(&linesPerProcess, &indexes, &sendCount, &displs, nProcesses, lines, columns);

        for (int i = 0; i < nProcesses; i++)
        {
            MPI_Isend(&(linesPerProcess[i]), 1, MPI_INT, i, 0, MPI_COMM_WORLD, &request);
            MPI_Isend(&(indexes[i]), 1, MPI_INT, i, 1, MPI_COMM_WORLD, &request);
            MPI_Isend(sendCount, nProcesses, MPI_INT, i, 3, MPI_COMM_WORLD, &request);
            MPI_Isend(displs, nProcesses, MPI_INT, i, 4, MPI_COMM_WORLD, &request);
        }
    }
    MPI_Bcast(&lines, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&columns, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank != 0)
    {
        allocMatrix(&matrix, lines, columns);
    }

    // printf("rank:%d  l:%d  c:%d\n", rank, lines, columns);
    int myLines;
    int myIndex;
    int **auxMatrix;
    MPI_Request reqSendCount, reqDispls;
    int flagSendCount = 0, flagDispls = 0;

    MPI_Recv(&myLines, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    MPI_Recv(&myIndex, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);

    MPI_Irecv(sendCount, nProcesses, MPI_INT, 0, 3, MPI_COMM_WORLD, &reqSendCount);
    MPI_Irecv(displs, nProcesses, MPI_INT, 0, 4, MPI_COMM_WORLD, &reqDispls);

    // printf("rank:%d  mtLines:%d  myIndex:%d\n", rank, myLines, myIndex);
    allocMatrix(&auxMatrix, myLines, columns);

    while ((flagSendCount == 0) || (flagDispls == 0))
    {
        MPI_Test(&reqSendCount, &flagSendCount, &status);
        MPI_Test(&reqDispls, &flagDispls, &status);
    }

    // this happens in a while loop
    int step = 0;
    while (step < totalSteps)
    {
        start = clock();
        MPI_Bcast(*matrix, lines * columns, MPI_INT, 0, MPI_COMM_WORLD);
        end = clock();
        cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
        printf("rank:%d step:%d time to bcast: %f\n", rank, step, cpu_time_used);

        start = clock();
        printf("rank:%d has %d lines\n", rank, myLines);
        for (int i = 0; i < myLines; i++)
        {
            // printf("rank%d: ", rank);
            for (int j = 0; j < columns; j++)
            {
                auxMatrix[i][j] = matrix[myIndex + i][j] + 1;
                // printf("%d ", auxMatrix[i][j]);
            }
            // printf("\n");
        }
        end = clock();
        cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
        printf("rank:%d step:%d time to increase: %f\n", rank, step, cpu_time_used);

        // printf("rank:%d  fSC:%d  fD:%d\n", rank, flagSendCount, flagDispls);
        start = clock();
        MPI_Gatherv(*auxMatrix, sendCount[rank], MPI_INT, *matrix, sendCount, displs, MPI_INT, 0, MPI_COMM_WORLD);
        end = clock();
        cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
        printf("rank:%d step:%d time to gather: %f\n", rank, step, cpu_time_used);

        step++;
    }
    if (rank == 0)
    {
        // for (int i = 0; i < lines; i++)
        // {
        //     for (int j = 0; j < columns; j++)
        //     {
        //         printf("%d ", matrix[i][j]);
        //     }
        //     printf("\n");
        // }
        printf("%d\n", matrix[0][0]);
    }

    MPI_Finalize();
    return 0;
}
