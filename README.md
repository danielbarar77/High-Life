## High-Life Simulation with MPI

### Description
This project implements a High Life simulation in C using parallel programming techniques with the MPI library. High Life is a cellular automaton similar to Conway's Game of Life but with slightly different rules.

### Usage
To compile the code, you'll need MPI installed. Then, use the following command:
```bash
mpicc -o high_life high_life.c
```
Execute the program with the input filename, output filename, and number of steps:
```bash
mpirun -np <num_processes> ./high_life <input_filename> <output_filename> <num_steps>
```

## Input Format
The input file should contain:
- The number of rows and columns in the matrix.
- The initial matrix configuration.

## Output Format
The output file will contain:
- The number of rows and columns
- The final matrix configuration after the specified number of steps.

## Example
```bash
mpirun -np 4 ./high_life input.txt output.txt 100
```

## Notes
- Ensure that the input file exists and is properly formatted.
- The program utilizes MPI for parallel processing, distributing computation across multiple processes.
- Each process handles a portion of the matrix, updating it based on the High Life rules.
- Communication between processes is managed using MPI communication routines.
- A Makefile is provided for easy compilation
- Sample input and output files are available for testing.
