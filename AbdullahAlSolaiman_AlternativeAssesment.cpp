
#include<iostream>
#include <stdio.h>
#include <cstdlib>
#include<string.h>
#include <vector>
#include <mpi.h>
#include <omp.h>

using namespace std;



 
/********************************************************** Structure Declarartion **************************************************************************/
 /* Structure to hold all coorindates for a rectangle to give a level of abstraction */
 struct Rectangle {
  int topLeftX;
  int topLeftY;
  int bottomRightX;
  int bottomRightY;
  int flag;
} tempRectangle;




/**********************************************************function prototype *******************************************************************************/
void findRightBottomCorner(int *myChunkOfTheFile, int currentRowIndex, int currentColumnIndex, int coordinates[][4], int row_index,int col_index ,int rowsNumbers, int columnsNumber, int myProcId);



/********************************************************** Read File Function **************************************************************************/
/*function to read the full "exam-data.txt" file, and put it in 2d array, then map the 2d array to 1-d array
to make it easier for communications between MPI processes*/

void fileRead(char* fName, int* rowsNumber,int* columnsNumber, int*** twoDMatrix , int** fullMatrixFromFile){

    FILE* inputFile = fopen (fName,"rb");
    if(inputFile==NULL){
        printf("Error opening file, exittin with exit-status 0 \n");
        exit(1); 
    }


    // storage allocation of matrix 
    *fullMatrixFromFile = (int*)malloc((*rowsNumber)*(*columnsNumber)*sizeof(int*));
    *twoDMatrix = (int**)malloc((*rowsNumber)*sizeof(int*));
    (*twoDMatrix)[0] = (int*)malloc((*rowsNumber)*(*columnsNumber)*sizeof(int));

    for ( int i = 1; i < (*rowsNumber); i++){
        (*twoDMatrix)[i]= (*twoDMatrix)[i-1]+(*columnsNumber);
    }
    
    for (int i = 0; i < *rowsNumber; i++){
        for(int j = 0; j < *columnsNumber+1; j++){
            fscanf(inputFile,"%c",&(*twoDMatrix)[i][j]);
        }
            
    }


    // 2-D to 1-D mapping for future use of fullMatrixFromFile in MPI functions.
    for (int i = 0; i < *rowsNumber; i++){
        for (int j = 0; j < *columnsNumber; j++){
            (*fullMatrixFromFile)[(*rowsNumber)*i+j]= (*twoDMatrix)[i][j]-48;
        }
    }


}



/********************************************************** Calculate Coorindates of all rectangles **************************************************************************/
/*function to read the full "exam-data.txt" file, and put it in 2d array, then map the 2d array to 1-d array
to make it easier for communications between MPI processes*/
void calculateCoorindates(int *myChunkOfTheFile, int rowsNumber, int columnsNumber, int myProcId, int procsNumber){
    MPI_Request request;
    MPI_Status status;
    int coordinates[rowsNumber][4];
    int oneDArrayOfAllCoorindates[rowsNumber*4];
    int slaveCooridnates[rowsNumber*4];
    int row_index = 0;
    int col_index =0;

    for (int i = 0; i < rowsNumber; i++){
        for (int j = 0; j < columnsNumber; j++){
            if (myChunkOfTheFile[columnsNumber*i+j] == 1){
                coordinates[row_index][col_index]= (((rowsNumber)*(myProcId))+1+i);
                coordinates[row_index][++col_index] =j+1;

                findRightBottomCorner(myChunkOfTheFile, i, j, coordinates,row_index,col_index,rowsNumber,columnsNumber,myProcId);
                col_index=0;
                row_index = row_index+1;   
            }
        }
    }
    
    // mapping 2d-list of cooridnates to 1-d list of all coorindates array
    for (int i = 0; i < row_index; i++){
        for (int j = 0; j < 4; j++){
           oneDArrayOfAllCoorindates[4*i+j]=coordinates[i][j]; 
        }
    }

    // if slave --> send list of your cooridnates to the master
    if(myProcId!=0){
        /* sending the size of the data that it is about to send
           to the master process
           This happens becasue p1 could have found 1 rectangles
           and p2 could have found 4, thus, in the MPI_Recv() we 
           need to state the size, thus we send the size first   */
        MPI_Send(&row_index,1,MPI_INT,0,0,MPI_COMM_WORLD);    
        int send_size = ((row_index*4)+1);
        // now, we send to master all the cooridnates of the rectangle found by the current process 
        MPI_Send(&oneDArrayOfAllCoorindates,send_size,MPI_INT,0,0,MPI_COMM_WORLD);
    }

    if(myProcId==0){
        // declaring vector of structs to hold all coorindates from all processes 
        // starting with ourselves process 0
        vector<Rectangle> collective_coordinates; 
        int recv_size;
        int total_size=0;
        for (int k = 0; k < row_index; k++){

            // setting the tempStructure values of each rectagnle found, and pushing to the vector of structs
            tempRectangle.topLeftX = oneDArrayOfAllCoorindates[k*4];
            tempRectangle.topLeftY = oneDArrayOfAllCoorindates[(k*4)+1];
            tempRectangle.bottomRightX = oneDArrayOfAllCoorindates[(k*4)+2];
            tempRectangle.bottomRightY = oneDArrayOfAllCoorindates[(k*4)+3];

            tempRectangle.flag = 0;

            collective_coordinates.push_back(tempRectangle);
        }

        for (int i = 1; i <procsNumber; i++){
            MPI_Recv(&recv_size,1,MPI_INT,i,0,MPI_COMM_WORLD,&status);
            
            // MPI_Wait(&request,&status);

            total_size =((recv_size*4)+1);

            MPI_Recv(&slaveCooridnates,total_size,MPI_INT,i,0,MPI_COMM_WORLD,&status);
            // MPI_Wait(&request,&status);


            for (int k = 0; k < recv_size; k++){
                tempRectangle.topLeftX = slaveCooridnates[k*4];
                tempRectangle.topLeftY = slaveCooridnates[(k*4)+1];
                tempRectangle.bottomRightX = slaveCooridnates[(k*4)+2];
                tempRectangle.bottomRightY = slaveCooridnates[(k*4)+3];
                tempRectangle.flag = 0;

                collective_coordinates.push_back(tempRectangle);
            }
        }


        /*AFTER Recieving all coordinates, preparing a new vecotr that merges the connected 
          rectangles between multiple chunks together, then printing out the final version 
          of the computations*/

        vector<Rectangle> finalRectsCoordinates;
        bool flag = false;
        for(int i = 0; i<collective_coordinates.size(); i++){
            for(int j = i+1; j<collective_coordinates.size(); j++){
                if(
                    (collective_coordinates[i].topLeftY == collective_coordinates[j].topLeftY && collective_coordinates[i].bottomRightY == collective_coordinates[j].bottomRightY)
                    &&
                    ((collective_coordinates[i].bottomRightX+1) == collective_coordinates[j].topLeftX)
                ){

                    

                    tempRectangle.topLeftX = collective_coordinates[i].topLeftX;
                    tempRectangle.topLeftY = collective_coordinates[i].topLeftY;
                    tempRectangle.bottomRightX = collective_coordinates[j].bottomRightX;
                    tempRectangle.bottomRightY =  collective_coordinates[j].bottomRightY;

                    collective_coordinates[j].flag = 1;
        
                        
                    // pushing the the tempRectangle sturcture to the vecotor<Rectagnle> of structs
                    finalRectsCoordinates.push_back(tempRectangle);
                    flag = true;
                    break;
                }

                else{
                    flag = false;
                }   

            }


            if(!flag && collective_coordinates[i].flag == 0){
            flag = true;
            finalRectsCoordinates.push_back(collective_coordinates[i]);
            collective_coordinates[i].flag=1;
            }

            if(i+1 == collective_coordinates.size() && collective_coordinates[i].flag == 0){
                finalRectsCoordinates.push_back(collective_coordinates[i]);
            }           

        }

        cout <<"Number of rctangles: " << finalRectsCoordinates.size() << endl;

        for(int i=0; i<finalRectsCoordinates.size(); i++){
            if(finalRectsCoordinates.size() > 0){
                // accoridng to the sample output, it (4,3) is y,x.
                // Rectangle 1 coordinates  [top-left-corner]: (4,3)
                cout<< "Rectangle " << i << " coordinates  [top-left-corner]: ("<< finalRectsCoordinates[i].topLeftY           << ","   << finalRectsCoordinates[i].topLeftX     << ")"    << endl
                    << "Rectangle " << i << " coordinates  [bottom-right-corner]: ("<< finalRectsCoordinates[i].bottomRightY   << ","   << finalRectsCoordinates[i].bottomRightX << ")"    << endl
                    <<endl;
            }
        }
   
    } 

} // end of function calculateCoorindates()







/********************************************************** find bottom right corner **************************************************************************/
/*function to find bootom corner for a given top-left corner, it gets called by calculaeCoorindates as a helper function for it to help it do its job via multi threading
*/
void findRightBottomCorner(int *myChunkOfTheFile, int currentRowIndex, int currentColumnIndex, int coordinates[][4], int row_index,int col_index ,int rowsNumbers, int columnsNumber, int myProcId){
    int x = rowsNumbers;
    int y = columnsNumber;
    int end_of_matrix,end_of_rectangle;
    int row,col;
    bool inner_loop_flag,outerloop;
    row=col=0; 
    end_of_matrix = end_of_rectangle=0;
    inner_loop_flag,outerloop = false;
     int columnFlag = 0;     
    int rowFlag = 0;     
    int thread_id=0;
    int chunk =0;

    #pragma omp parallel num_threads(4) private(thread_id,row,col,rowFlag) shared(coordinates,outerloop)
    {
        #pragma pragma omp for schedule(static)
        for (row = currentRowIndex; row < x; row++)
        {
            if (myChunkOfTheFile[y*row+currentColumnIndex] == 0)
            {

                rowFlag = 1;
                outerloop = true;
            
            }
  
 
            if (myChunkOfTheFile[y*row+currentColumnIndex] == 2)
            {}
 
 
            for (col = currentColumnIndex; col < y; col++)
            {
               if (myChunkOfTheFile[y*row+col] == 0)
                {
 
                    columnFlag = 1; 
                    end_of_rectangle = col;
                    break;
                }
 
                end_of_matrix = col;
                myChunkOfTheFile[y*row+col] = 2;   
            }
 
            if(!outerloop)
            {
 
                if (rowFlag == 1)
 
                {
 
                #pragma omp critical
 
                coordinates[row_index][2]= row;
 
                }
 
                else
 
                {
 
                #pragma omp critical
 
                coordinates[row_index][2]= ((x*(myProcId))+1+row);
 
                }
 
                if (columnFlag == 1){
                coordinates[row_index][3]= end_of_rectangle;
                }
 
                else{
                coordinates[row_index][3]= end_of_matrix+1;
                }
 
            }
 
 
 
        }
 
    }
}



/********************************************************** Main **************************************************************************/

int main(int argc, char *argv[]){
    // intialization of MPI 
	MPI_Init(NULL,NULL);

	int myProcId, procsNumber,chunkSize,rowsePerProcess;
    int numRows=10000;
    int numColumns=10000;

    MPI_Comm_rank(MPI_COMM_WORLD, &myProcId);
	MPI_Comm_size(MPI_COMM_WORLD, & procsNumber);

    int *matrixRecieved=(int*)malloc((numRows)*(numColumns)*sizeof(int*));
    int *oneDMatrix=(int*)malloc((numRows)*(numColumns)*sizeof(int*));
    int *fullMatrixFromFile;


    if(numRows%procsNumber!= 0){
        printf("Please enter a number of processes that the matrix size can be divded by it equaly");
        MPI_Finalize();
        free(matrixRecieved);
        free(oneDMatrix);
        free(fullMatrixFromFile);
        exit(99);
    }



    if(myProcId==0){
        int **twoDMatrix;
        fileRead("exam-data.txt", &numRows, &numColumns,&twoDMatrix,&fullMatrixFromFile);
    }   

    // calculation the size of chunk of the fullMatrixFrom file that each process will get
    chunkSize = numRows/procsNumber;
    rowsePerProcess = numRows/procsNumber;
    chunkSize = chunkSize*numColumns;

    MPI_Scatter( fullMatrixFromFile, chunkSize, MPI_INT, matrixRecieved, chunkSize, MPI_INT, 0, MPI_COMM_WORLD);
    calculateCoorindates(matrixRecieved,rowsePerProcess,numColumns,myProcId,procsNumber);

    /// freeing all dynamically allocated memmory:

    free(matrixRecieved);
    free(oneDMatrix);
    free(fullMatrixFromFile);
    MPI_Finalize();
    return 0;

}






