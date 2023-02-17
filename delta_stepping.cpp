#include<iostream>
#include<vector>
#include<string>
#include<sstream>
#include<utility>
#include "mpi.h"

#define FILENAME "test_graph.mtx"
#define DELTA 20
#define VERTICES 1957027

using namespace std;

int main(int argc, char *argv[]){
    
    MPI_Init(&argc,&argv);

    int rank, size, delta=DELTA;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_File fh;
    MPI_File_open(MPI_COMM_WORLD, FILENAME, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);

    /*Distribute file among processes approximately equally*/
    MPI_Offset fileSize;
    MPI_File_get_size(fh, &fileSize);
    fileSize--;//to remove EOF
    //cout<<fileSize<<endl;

    MPI_Offset mySize = fileSize/size;

    if(fileSize%size!=0)
        mySize++;

    MPI_Offset globalStart = rank*mySize;
    MPI_Offset globalEnd = globalStart + mySize - 1;

    if(rank == size-1)
        globalEnd = fileSize-1;

    int overlap = 30; //assuming '\n' occurs within 30 characters
    if(rank!=0)
        globalStart-=overlap; 
    
    mySize = globalEnd - globalStart + 1;

    char *chunk;
    chunk = (char *)malloc((mySize+1)*sizeof(char));

    MPI_File_read_at_all(fh, globalStart, chunk, mySize, MPI_CHAR, MPI_STATUS_IGNORE);
    chunk[mySize] = '\0';
    
    int locstart=0, locend=mySize-1;
    if (rank != 0) {
        while(chunk[locstart] != '\n') {
            locstart++;
            globalStart++;
        }
        locstart++;
        globalStart++;
    }

    if (rank != size-1) {
        locend-=overlap;
        globalEnd-=overlap;
        while(chunk[locend] != '\n') {
            locend++;
            globalEnd++;
        }
    }
    mySize = locend-locstart+1;

    free(chunk);
    
    //At this point, each processor has globalStart as starting point, globalEnd as end point of file to access
    
    chunk = (char *)malloc((mySize+1)*sizeof(char));
    MPI_File_read_at_all(fh, globalStart, chunk, mySize, MPI_CHAR, MPI_STATUS_IGNORE);
    chunk[mySize] = '\0';
    //cout<<chunk<<endl;    

    /*Now each processor starts creating adjacency list*/

    int n=VERTICES;     //no of vertices
    vector<vector<pair<int,int>>> adj_list(n+1);

    //split chunk into lines and then pair of 3 ints

    stringstream ss(chunk);
    string lines;

    while(getline(ss,lines,'\n')){        
        stringstream sslines(lines);
        string word1, word2, word3;
        //we know that there are 3 ints
        while (sslines >> word1 >> word2 >> word3){
            int source = stoi(word1);
            int dest = stoi(word2);
            int weight = stoi(word3);
            adj_list[source].push_back(make_pair(dest,weight));   
            adj_list[dest].push_back(make_pair(source,weight));      
        }
    }
    free(chunk);

    /* for(int i=1;i<=n;i++){
        if(adj_list[i].empty()){
            continue;
        } else {
            cout<<i<<'\t';
            int length = adj_list[i].size();
            for(int j=0; j<length;j++){
                cout<<"("<<adj_list[i][j].first<<","<<adj_list[i][j].second<<")\t";
            }
            cout<<endl;
        }
    } */
    //adj_list creation is completed above

    /*Splitting/partition of adj_list and vertices among processes*/

    int local_no_of_vertices = n/size;
    int offset; //extra elements i.e. remainder

    if(n%size!=0){
        offset = n%size;
        if(rank<offset){
            local_no_of_vertices++;
        }
    }

    int global_start_index;
    if(rank==offset){
        global_start_index=rank*(local_no_of_vertices+1);
    } 
    if(rank<offset) {
        global_start_index=rank*(local_no_of_vertices);
    }
    if(rank>offset) {
        global_start_index=rank*(local_no_of_vertices)+offset;
    }

    int global_end_index = global_start_index + local_no_of_vertices-1;

    //since vertex starts from 1 and not 0
    global_end_index++;
    global_start_index++;

    cout<<rank<<'\t'<<global_start_index<<'\t'<<global_end_index<<'\t'<<local_no_of_vertices<<endl;



    MPI_File_close(&fh);
    
    return 0;

}