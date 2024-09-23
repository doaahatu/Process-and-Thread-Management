#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>

#define MATRIXSIZE 100
#define THREADSNUM 4
#define PROCESSESNUM 4


int M1[MATRIXSIZE][MATRIXSIZE];
int M2[MATRIXSIZE][MATRIXSIZE];
int result[MATRIXSIZE][MATRIXSIZE];


// A Function to get the current time in microseconds
long long getCurrentTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}


// A Function for matrix multiplication - Naive approach
void MatrixMultiplication(int m1[MATRIXSIZE][MATRIXSIZE], int m2[MATRIXSIZE][MATRIXSIZE], int result[MATRIXSIZE][MATRIXSIZE])
{

    for(int i=0 ; i<MATRIXSIZE ; i++)
    {
        for(int j=0 ; j<MATRIXSIZE ; j++)
        {
            result[i][j]=0;
            for(int k=0 ; k<MATRIXSIZE ; k++)
            {
                result[i][j] += m1[i][k]*m2[k][j];

            }
        }

    }
}


// A Function for process-based matrix multiplication
void processMatrixMultiplication(int m1[MATRIXSIZE][MATRIXSIZE], int m2[MATRIXSIZE][MATRIXSIZE], int result[MATRIXSIZE][MATRIXSIZE], int startRow, int endRow, int fd[2])
{

    for(int i=startRow ; i<endRow ; i++)
    {
        for(int j=0 ; j<MATRIXSIZE ; j++)
        {
            result[i][j]=0;
            for(int k=0 ; k<MATRIXSIZE ; k++)
            {
                result[i][j] += m1[i][k]*m2[k][j];

            }
        }
    }

}

/*
// Child process
void childProcess(int m1[][MATRIXSIZE], int m2[][MATRIXSIZE], int startRow, int endRow, int fd[2])
{
    // Child process
    int result[MATRIXSIZE][MATRIXSIZE];
    processMatrixMultiplication(m1,m2,result, startRow, endRow,fd);
    // Close the read end of the pipe in the child process
    close(fd[0]);
    // Write data into the pipe
    write(fd[1], result, sizeof(result));
    // Close the write end of the pipe in the child process
    close(fd[1]);
    exit(EXIT_SUCCESS);


}
*/

// A Function for thread-based matrix multiplication
void* threadMatrixMultiplication(void* arg)
{
    // Define the thread ID
    int threadID = *((int*)arg);

    // Calculate the number of rows per Thread
    int rowsPerThread = MATRIXSIZE / THREADSNUM;

    int rstart = threadID * rowsPerThread;
    int rend = (threadID == THREADSNUM - 1) ? MATRIXSIZE : (threadID + 1) * rowsPerThread;

    // Matrix multiplication for rows
    int localResult[MATRIXSIZE][MATRIXSIZE];
    for (int i = rstart; i < rend; i++)
    {
        for (int j = 0; j < MATRIXSIZE; j++)
        {
            localResult[i][j] = 0;
            for (int k = 0; k < MATRIXSIZE; k++)
            {
                localResult[i][j] += M1[i][k] * M2[k][j];
            }
        }
    }

    // Copy local result to the global result matrix
    for (int i = rstart; i < rend; i++)
    {
        for (int j = 0; j < MATRIXSIZE; j++)
        {
            result[i][j] = localResult[i][j];
        }
    }

    pthread_exit(NULL);
}



int main()
{

    long long start_time, end_time;
    // Throughput calculations
    double total_elements = MATRIXSIZE * MATRIXSIZE * MATRIXSIZE;

    // Define the ID and the number using arrays
    int ID[]= {1,2,1,1,0,8,8};
    int Num[]= {2,4,2,5,8,0,9,2,6,4};


    // Fill in the first matrix with my University ID
    for(int i=0 ; i<MATRIXSIZE ; i++)
    {
        for(int j=0 ; j<MATRIXSIZE ; j++)
        {
            M1[i][j] = ID[(i*MATRIXSIZE+j)%7];
        }
    }

    // Fill in the second matrix with the multiplication result of my ID and my birth year
    for(int i=0 ; i<MATRIXSIZE ; i++)
    {
        for(int j=0 ; j<MATRIXSIZE ; j++)
        {
            M2[i][j] = Num[(i*MATRIXSIZE+j)%10];
        }
    }


    // Naive approach
    start_time = getCurrentTime();
    MatrixMultiplication(M1, M2, result);
    end_time = getCurrentTime();
    printf("Naive Approach: Execution Time = %lld microseconds\n", end_time - start_time);
    // Naive approach
    double throughput1 = total_elements / (end_time - start_time);
    printf("Naive Approach: Throughput = %.2f elements/microsecond\n\n", throughput1);


    // Processes approach
    start_time = getCurrentTime();
    // Create a pipe
    int fd[2];
    // fd[0] for read
    // fd[1] for write
    if (pipe(fd) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Dividing the matrix into parts, each process will have specific number of rows
    int rowsPerProcess = MATRIXSIZE/PROCESSESNUM;
    int remainingRows = MATRIXSIZE%PROCESSESNUM;
    int startRow = 0 ;

    for (int i = 0; i < PROCESSESNUM; ++i)
    {

        int endRow = startRow + rowsPerProcess + (i < remainingRows ? 1 : 0);

        // Fork a child process
        pid_t child_pid = fork();

        // Check the return value of pipe
        if (pipe(fd) == -1)
        {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        if (child_pid == -1)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (child_pid == 0)
        {
            // Child process
            processMatrixMultiplication(M1,M2,result, startRow, endRow,fd);
            exit(0);

        }


        else
        {
            // Parent process
            close(fd[1]); // Close the write end of the pipe in the parent process
            // Read data from the pipe
            int Result[MATRIXSIZE][MATRIXSIZE];
            read(fd[0], Result, sizeof(Result));
            // Close the read end of the pipe in the parent process
            close(fd[0]);
            // Wait for the child process to finish
            waitpid(child_pid, NULL, 0);

            // Child process
            close(fd[0]); // Close the read end of the pipe in the child process
            // Write data into the pipe
            write(fd[1], result, sizeof(result));
            // Close the write end of the pipe in the child process
            close(fd[1]);

        }

        startRow = endRow;
    }

    // Wait for processes to finish
    for (int i = 0; i < PROCESSESNUM; i++)
    {
        wait(NULL);
    }
    end_time = getCurrentTime();
    printf("Processes Approach: Execution Time = %lld microseconds\n", end_time - start_time);
    // Processes
    double throughput2 = total_elements / (end_time - start_time);
    printf("Processes Approach: Throughput = %.2f elements/microsecond\n\n", throughput2);




    // Pthreads - Joinable
    start_time = getCurrentTime();

    pthread_t threads[THREADSNUM];
    int threadID[THREADSNUM]; // Array to store thread IDs

    // Create threads
    for (int i = 0; i < THREADSNUM; i++)
    {
        threadID[i] = i;
        pthread_create(&threads[i], NULL, threadMatrixMultiplication, &threadID[i]);
    }

    // Wait for threads to finish
    for (int i = 0; i < THREADSNUM; i++)
    {
        pthread_join(threads[i], NULL);
    }

    end_time = getCurrentTime();
    printf("Pthreads - Joinable Approach: Execution Time = %lld microseconds\n", end_time - start_time);
    // Pthreads - Joinable
    double throughput3 = total_elements / (end_time - start_time);
    printf("Pthreads - Joinable Approach: Throughput = %.2f elements/microsecond\n\n", throughput3);





    // Pthreads - Detached
    start_time = getCurrentTime();

    pthread_t Dthreads[THREADSNUM];
    int* DthreadID[THREADSNUM]; // Array to store thread IDs
    pthread_attr_t attributes;

    // Set thread attributes
    pthread_attr_init(&attributes);
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);


    // Create threads

    for(int i=0 ; i<THREADSNUM ; i++)
    {
        DthreadID[i] = malloc(sizeof(int));
        *DthreadID[i] = i;
        if(pthread_create(&Dthreads[i], &attributes, threadMatrixMultiplication, (void*)DthreadID[i]) != 0)
        {
            fprintf(stderr, "Failed to create threads.\n");
            exit(EXIT_FAILURE);

        }

    }

    // Destroy the thread attributes after thread creation
    pthread_attr_destroy(&attributes);

    end_time = getCurrentTime();
    printf("Pthreads - Detached Approach: Execution Time = %lld microseconds\n", end_time - start_time);
    // Pthreads - Detached
    double throughput4 = total_elements / (end_time - start_time);
    printf("Pthreads - Detached Approach: Throughput = %.2f elements/microsecond\n", throughput4);




    return 0;
}