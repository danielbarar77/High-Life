#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include <time.h>

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
    (*totalSteps) = atoi(argv[3]);
}

void allocMatrix(int ***matrix, int l, int c)
{
    // allocs an array that has continuous memory
    // every element of the matrix will point to a memory location of the array
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
    // calcualtes the following:
    // - how many lines each process has to calculate
    // - the index from where each process needs to calculate
    // - how many elements (integer values) each process needs to send
    // - the displacements of each process
    // the last two are used for MPI_Gatherv to gather all the locall matrixes to rank 0
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

int readFile(int ***matrix, char *inFile, int *lines, int *columns)
{
    // reads from the input file the number of lines and column, and also the initial matrix
    FILE *fin = fopen(inFile, "r");
    if (!fin)
    {
        return -1;
    }
    if (fscanf(fin, "%d", lines) < 0)
    {
        printf("Ops! Something went wrong 1!\n");
    }
    if (fscanf(fin, "%d", columns) < 0)
    {
        printf("Ops! Something went wrong 2!\n");
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

int writeFile(int ***matrix, char *outFile, int *lines, int *columns)
{
    // writes to the output file the number of lines and columns, and the matrix
    FILE *fout = fopen(outFile, "w");
    if (!fout)
    {
        return -1;
    }
    if (fprintf(fout, "%d ", *lines) < 0)
    {
        printf("Ops! Something went wrong1!\n");
    }
    if (fprintf(fout, "%d", *columns) < 0)
    {
        printf("Ops! Something went wrong2!\n");
    }
    fprintf(fout, "\n");

    for (int i = 0; i < (*lines); i++)
    {
        for (int j = 0; j < (*columns); j++)
        {
            if (fprintf(fout, "%d ", (*matrix)[i][j]) < 2)
            {
                printf("Ops! Something went wrong!\n");
            }
        }
        fprintf(fout, "\n");
    }

    fclose(fout);
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
    MPI_Request request;

    if (rank == 0)
    {
        // rank 0 reads the file
        if (readFile(&matrix, inFile, &lines, &columns) == -1)
        {
            printf("Error on opening input file!\n");
            return -1;
        }

        int *linesPerProcess = (int *)calloc(nProcesses, sizeof(int));
        int *indexes = (int *)calloc(nProcesses, sizeof(int));

        // rank 0  calcualtes the needed info
        getInfo(&linesPerProcess, &indexes, &sendCount, &displs, nProcesses, lines, columns);

        // rank 0 sends the necessary info to all processes
        for (int i = 0; i < nProcesses; i++)
        {
            MPI_Isend(&(linesPerProcess[i]), 1, MPI_INT, i, 0, MPI_COMM_WORLD, &request);
            MPI_Isend(&(indexes[i]), 1, MPI_INT, i, 1, MPI_COMM_WORLD, &request);
        }
    }
    // broadcast all the general info
    MPI_Bcast(sendCount, nProcesses, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(displs, nProcesses, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&lines, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&columns, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // each rank except 0 (it allocated before) allocs matrix
    if (rank != 0)
    {
        allocMatrix(&matrix, lines, columns);
    }

    int myLines;
    int myIndex;
    int **auxMatrix;

    // recives the individual data
    MPI_Recv(&myLines, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    MPI_Recv(&myIndex, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);

    // allocs an auxiliar matrix that will temporary store the new values
    allocMatrix(&auxMatrix, myLines, columns);

    int step = 0;
    int count;
    int i, j, l, c, ni, nj;
    // if there are more than 1 process then the initial matrix will be broadcasted
    if (nProcesses > 1)
    {
        MPI_Bcast(*matrix, lines * columns, MPI_INT, 0, MPI_COMM_WORLD);
    }

    while (step < totalSteps)
    {
        for (i = 0; i < myLines; i++)
        {
            for (j = 0; j < columns; j++)
            {
                count = 0;

                // counts the neighboring cells that have the value 1
                for (l = -1; l <= 1; l++) // lines
                {
                    for (c = -1; c <= 1; c++) // columns
                    {
                        if (l == 0 && c == 0)
                            continue; // skip current cell

                        ni = myIndex + i + l;
                        nj = j + c;

                        if (ni >= 0 && ni < lines && nj >= 0 && nj < columns)
                        {
                            if (matrix[ni][nj] == 1)
                            {
                                count++;
                            }
                        }
                    }
                }

                // respects the rule of High Life
                if (((count == 3) || (count == 6)) && (matrix[myIndex + i][j] == 0))
                {
                    auxMatrix[i][j] = 1;
                }
                else if (((count == 2) || (count == 3)) && (matrix[myIndex + i][j] == 1))
                {
                    auxMatrix[i][j] = 1;
                }
                else
                {
                    auxMatrix[i][j] = 0;
                }
            }
        }
        // copy the data to the original matrix
        for (i = 0; i < myLines; i++)
        {
            for (j = 0; j < columns; j++)
            {
                matrix[myIndex + i][j] = auxMatrix[i][j];
            }
        }
        if (nProcesses > 1)
        {
            // if there are more than one process then each process will update the marginal lines of its section with the lines calculated by its neighbours
            if (rank > 0)
            {
                MPI_Isend(auxMatrix[0], columns, MPI_INT, rank - 1, 9, MPI_COMM_WORLD, &request);
            }
            if (rank < nProcesses - 1)
            {
                MPI_Isend(auxMatrix[myLines - 1], columns, MPI_INT, rank + 1, 10, MPI_COMM_WORLD, &request);
            }

            if (rank > 0)
            {
                MPI_Recv(matrix[myIndex - 1], columns, MPI_INT, rank - 1, 10, MPI_COMM_WORLD, &status);
            }
            if (rank < nProcesses - 1)
            {
                MPI_Recv(matrix[myIndex + myLines], columns, MPI_INT, rank + 1, 9, MPI_COMM_WORLD, &status);
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }

        step++;
    }

    // after executing for every step, all the auxiliar matrixes are stored in rank 0 matrix
    MPI_Gatherv(*auxMatrix, sendCount[rank], MPI_INT, *matrix, sendCount, displs, MPI_INT, 0, MPI_COMM_WORLD);

    // rank 0 wites the final matrix in the output file
    if (rank == 0)
    {
        writeFile(&matrix, outFile, &lines, &columns);
    }

    MPI_Finalize();
    return 0;
}
