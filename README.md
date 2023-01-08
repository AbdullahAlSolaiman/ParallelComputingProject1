# Parallel Computing Project
A Project done using OpenMP and MPI parallel computing libraries to return the coordinates of multiple rectangles hidden in an extremely large matrix of data.
## 1. Compiling and Executing the Program

At first, the source code file and the “exam-data.txt” must be in the same folder, provided that is true, the compilation and execution should be as the following from your terminal :





```c++
mpic++ -fopenmp AbdullahAlSolaiman_AlternativeAssesment.cpp -o myprogram
mpirun -np 4 myprogram  #np means number of processes,it could be changed
```





## 2. Model of Parallel Algorithm Used

In this program I used the “Master-Slave Model”. The reason behind this model is the way I viewed the problem that came to my mind. While thinking of the problem, I realized that there is a large amount of data and that should be distributed equally to each process. Then each process will calculate the coordinates of the rectangles that are visible to it within its chunk of the file that has been passed to it by the master process. 

![alt text](https://github.com/AbdullahAlSolaiman/ParallelComputingProject1/blob/main/process-distribution.png "Figure 1")

figure 1 shows the Master-Slave model for computations

The reason behind this choice was due to the “not significant” dependency of calculations between processes which means that it will be easier to implement that each process will not be affected by any other computation going on in the system, all it has to do is do the calculations on the chunk assigned to it by the master process and deliver the results to the master process. The role of the master process is to coordinate the process of distribution and calculations among all slave processes and then after receiving all the results from the slave processes it will produce the final result and print it out.

## 3. Implementation of Decomposition:
In my algorithm I used the Data-Centric approach for decompositioning (Partitioning) the problem. I use Data Decompositions , which means that the domain is partitioned into subdomains where similar tasks(such as calculating rectangle coordinates) are performed on these subdomains. I first read the full binary matrix from the master process. I then use MPI_Scatter() to distribute the domain (the Decomposition) to each process in the communicator.
```c++
void fileRead(char* fName, int* rowsNumber,int* columnsNumber, int*** twoDMatrix , int** fullMatrixFromFile)
```

The FullMatrixFromFile and twoDMatrix are pointers to int arrays declared in main function, when the function fileRead() is called, it gets the file name passed to it, and the addresses of these arrays. 
At first, the matrix will be read into twoDMatrix which is dynamically allocated, then it will be mapped into fullMatrixFromFile array.

Then, chunkSize will be calculated, which means the size of data each slave process will take, then the fullMatrixFromFile will be scattered(distributed/decomposed) on the slave processe and each process will pefrom the necessary computation on its part of the domain only.
```c++
    chunkSize = numRows/procsNumber;
    rowsePerProcess = numRows/procsNumber;
    chunkSize = chunkSize*numColumns;
```
```c++
MPI_Scatter( fullMatrixFromFile, chunkSize, MPI_INT, matrixRecieved, chunkSize, MPI_INT, 0, MPI_COMM_WORLD);
```

Then, function  calculateCoorindates() will be called from main of each process in the communicator:
```c++ 
void calculateCoorindates(int *myChunkOfTheFile, int rowsNumber, int columnsNumber, int myProcId, int procsNumber) 
```


And in this function, if the procID is 0 (means it’s a master process), then it will do its part of the calculation and wait for other processes to finish their part and send the results to process 0. 

Otherwise, for all the other procIDs each process will do the computation to find the coordinates of the rectangle in its domain and then it will send an array of these coordinates to the master process.


The master process will receive all coordinates, perform the mathematics and print the final result for all rectangles existing in the file.

## 4. Parts Using MPI and tasks assigned to them
In my solution the mpi is used in:

1-Collective: Distributing the fullMatrixFromFile array on the processes within the communicator equally:

```c++
    chunkSize = numRows/procsNumber;
    rowsePerProcess = numRows/procsNumber;
    chunkSize = chunkSize*numColumns;
// process 0 scattering the matrix to the rest of the procs in the communicator
MPI_Scatter( fullMatrixFromFile, chunkSize, MPI_INT, matrixRecieved, chunkSize, MPI_INT, 0, MPI_COMM_WORLD);
```







2- Point-to-Point: Slave Processes sending the results of “finding rectangles in their chunk of the array” as an array of coordinates to the master process
```c++    
// if slave --> send list of your coordinates to the master
    if(myProcId!=0){
        /* sending the size of the data that it is about to send
           to the master process
           This happens becasue p1 could have found 1 rectangles
           and p2 could have found 4, thus, in the MPI_Irecv() we 
           need to state the size, thus we send the size first   */
        MPI_Send(&row_index,1,MPI_INT,0,0,MPI_COMM_WORLD);    
        int send_size = ((row_index*4)+1);
        // now, we send to master all the coordinates of the rectangle found by   the current process 
           MPI_Send(&oneDArrayOfAllCoorindates,send_size,MPI_INT,0,0,MPI_COMM_WORLD);
    }

```
Tasks Assigned: For all slave-processes, computatins assigned to them is “calculateCoordinates()” part of the function to generate a list of coordinates(top-left corner, bottom-right corner) for all rectangles found within the specified chunk of the dataset that each process received, then they will send the results to the master process for further computation.









3- Point-to-Point: Master Process receives an array of coordinates of rectangles found by slave processes.



Tasks assigned to this process is to

1- Calculate coordinates of rectangles found in its own chunk of the file
2- Receive coordinates calculated from all slave-processes
3- Compute the final list of coordinates after processing the data received from slave processes and combine rectangles that are on the same line, then print out the results





## 5. Parts Using OpenMP and tasks assigned to them

1- Finding the bottom-right-corner of the rectangle using multi-threading 



The loop will start from the top-left-corner found by the single thread, then the single will call the function findRightBottomCorner()

```c++
findRightBottomCorner(myChunkOfTheFile, i, j, coordinates,row_index,col_index,rowsNumber,columnsNumber,myProcId);
```

Tasks Assigned: 
The function will now use multithreading to go through all the slices under the found top-left corner and check whether they are all filled with ones, if they are not or the new line start with 0 then that means it is the end of the rectangle, and the right bottom corner coordinates are found.



In this solution I used:
```c++

1-MPI_Send()
MPI_Send(
    void* data,
    int count,
    MPI_Datatype datatype,
    int destination,
    int tag,
    MPI_Comm communicator
)
```

Send a “count” of “data” from a single process, it is a point to point communication. Where the sending process passes the image to the “destination” process id.

In my case, I used MPI_Send() to send the array of coordinates found by the slave processes to the master process.

2- MPI_Recv(): Blocking communication: receiving the array send by slave processes.

```c++
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status);
```


Blocking communication is done using MPI_Send() and MPI_Recv(). These functions do not return (i.e., they block) until the communication is finished. Simplifying somewhat, this means that the buffer passed to MPI_Send() can be reused, either because MPI saved it somewhere, or because it has been received by the destination. Similarly, MPI_Recv() returns when the receive buffer has been filled with valid data.
Finally, the count parameter represents the maximum number of elements you are expecting to receive in the communication. The actual number received can thus be less or equal. If you receive more elements, an error will be triggered.


## Appendix: 

Sample output:

![alt text](https://github.com/AbdullahAlSolaiman/ParallelComputingProject1/blob/main/sampleoutput1.png "Sample 1")
![alt text](https://github.com/AbdullahAlSolaiman/ParallelComputingProject1/blob/main/sampleoutput2.png "Sample 2")
![alt text](https://github.com/AbdullahAlSolaiman/ParallelComputingProject1/blob/main/sampleoutput3.png "Sample 3")
