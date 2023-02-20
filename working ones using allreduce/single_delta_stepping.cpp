#include<iostream>
#include<vector>
#include<string>
#include<sstream>
#include<utility>
#include<limits>
#include<fstream>
#include<set>
#include<mpi.h>

#define FILENAME "test_graph.mtx"
#define MAX_WEIGHT 200
#define DELTA 20
#define MAX_DEGREE 12
#define VERTICES 1957027
#define INF numeric_limits<int>::max()

using namespace std;

int main(int argc, char *argv[]){

    MPI_Init(&argc,&argv);

    double local_start_time = 0; //to measure time taken
    double local_end_time = 0; //to measure time taken

    int rank, size, delta=DELTA;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int total_vertices = VERTICES; //no of vertices
    const int n = total_vertices + size - total_vertices%size - 1;    //for padding 
    int local_no_of_vertices = (n+1)/size;
    int *local_adj_list=new int[2*local_no_of_vertices*MAX_DEGREE*2]();
    int* adj_list_array = new int[2*(n+1)*MAX_DEGREE*2]();
    int* adj_list_size = new int[2*(n+1)]();

    if(argc>1){
        if(stoi(argv[1])>0){
            delta = stoi(argv[1]);
        }
    }

    if(rank==0){

        /*File reading below
        The input line first line must be some comment
        The second line first character must contain total number of vertices*/
        string lines;
        ifstream file(FILENAME);
        getline(file,lines,'\n'); //to reject the first line
        getline(file,lines,'\n');
        string str1, str2, str3;
        stringstream s(lines);
        s>>str1>>str2>>str3;
        // int total_vertices=VERTICES;
        int total_vertices=stoi(str1);
        
        //adj_list[0] indicates light edges and 1 for heavy
        // vector<vector<vector<pair<int,int>>>> \
        // adj_list(2,(vector<vector<pair<int,int>>>(total_vertices+1)));  
        //cout << n << endl;
        //vector<vector<vector<pair<int,int>>>> adj_list(2,(vector<vector<pair<int,int>>>(n+1))); 
        
        while(getline(file,lines,'\n')){        
            stringstream sslines(lines);
            string word1, word2, word3;
            //we know that there are 3 ints
            while (sslines >> word1 >> word2 >> word3){
                int source = stoi(word1);
                int dest = stoi(word2);
                int weight = stoi(word3);
                int heavy = (weight<delta)?0:1;
                adj_list_array[heavy*(n+1)*MAX_DEGREE*2 + source*MAX_DEGREE*2 + \
                adj_list_size[heavy*(n+1)+source]*2+0]=dest;
                adj_list_array[heavy*(n+1)*MAX_DEGREE*2 + source*MAX_DEGREE*2 + \
                adj_list_size[heavy*(n+1)+source]*2+1]=weight;
                adj_list_size[heavy*(n+1)+source]++; 
                adj_list_array[heavy*(n+1)*MAX_DEGREE*2 + dest*MAX_DEGREE*2 + \
                adj_list_size[heavy*(n+1)+dest]*2 + 0]=source;
                adj_list_array[heavy*(n+1)*MAX_DEGREE*2 + dest*MAX_DEGREE*2 + \
                adj_list_size[heavy*(n+1)+dest]*2 + 1]=weight;
                adj_list_size[heavy*(n+1)+dest]++;        
            }
        }
        file.close();

        cout<<"The given input parameters are:\n";
        cout<<"Vertices:"<<total_vertices<<", Delta:"<<delta<<endl;
        cout<<"Maximum Degree:"<<MAX_DEGREE<<", Maximum Edge weight:"<<MAX_WEIGHT<<endl;
        cout<<"Processes invoked:"<<size<<endl;
    }

    MPI_Scatter(&adj_list_array[0], local_no_of_vertices*MAX_DEGREE*2, MPI_INT,\
        &local_adj_list[0], local_no_of_vertices*MAX_DEGREE*2, MPI_INT, 0,\
        MPI_COMM_WORLD);

    MPI_Scatter(&adj_list_array[(n+1)*MAX_DEGREE*2], local_no_of_vertices*MAX_DEGREE*2, MPI_INT,\
        &local_adj_list[local_no_of_vertices*MAX_DEGREE*2], local_no_of_vertices*MAX_DEGREE*2, MPI_INT, 0,\
        MPI_COMM_WORLD);

    //cout<<"Line 83\n";

    free(adj_list_array);
    free(adj_list_size);

    int global_start_index = rank*local_no_of_vertices;
    int global_end_index = global_start_index + local_no_of_vertices - 1;

    /*The below code for printing local adj list to check for errors*/

    // if(rank==2){
    //     cout<<"Processor "<<rank<<" has the following adj_list:\n";
    //     for(int i=0;i<local_no_of_vertices;i++){ 
    //         if(local_adj_list[1*local_no_of_vertices*MAX_DEGREE*2\
    //          + i*MAX_DEGREE*2]!=0 || \
    //          local_adj_list[i*MAX_DEGREE*2]!=0){
    //             cout<<i+global_start_index<<'\t';
    //             for(int k=0; k<2;k++){
    //                 int j=0;
    //                 while(local_adj_list[k*local_no_of_vertices*MAX_DEGREE*2\
    //                 + i*MAX_DEGREE*2 + j*2 + 0]!=0){
    //                     int dest = local_adj_list[k*local_no_of_vertices*MAX_DEGREE*2\
    //                     + i*MAX_DEGREE*2 + j*2 + 0];
    //                     int weight = local_adj_list[k*local_no_of_vertices*MAX_DEGREE*2\
    //                     + i*MAX_DEGREE*2 + j*2 + 1];
    //                     cout<<"k:"<<k<<" i:"<<i<<"("<<dest<<","<<\
    //                     weight<<")\t";
    //                     j++;
    //                 }
    //             }
    //             cout<<endl;
    //         } 
    //     }
    // }  

    local_start_time=MPI_Wtime();  

    /*Initializing distances*/

    int *d=new int[n+1];
    int *newd=new int[n+1];
    //vector<int> d(total_vertices+1);
    
    for(int i=0;i<=n;i++){
        d[i]=INF;
        newd[i]=INF;
    }

    d[1]=0;

    // //Number of buckets estimate
    // //int number_of_buckets = total_vertices*MAX_WEIGHT/delta;
    // //cout<<number_of_buckets<<endl;
    
    vector<vector<int>> Bucket;
    //The below values indicate the first non-empty bucket
    int local_bucket_min_pos=INF, global_bucket_min_pos=0;

    //cout<<Bucket.empty()<<endl;

    if(rank==0){
        vector<int> temp={1};
        Bucket.push_back(temp);
        local_bucket_min_pos=0;
        //cout<<Bucket.empty()<<endl;
    }

    // MPI_Allreduce(&local_bucket_min_pos, &global_bucket_min_pos, 1,
    //               MPI_INT, MPI_MIN, MPI_COMM_WORLD);  

    // if(rank==0){
    //     cout<<"Min pos is "<<global_bucket_min_pos<<endl;
    //     cout<<d[1]<<','<<d[2]<<','<<d[3]<<endl;
    // } 

    int max_pos=0; //local max position of bucket for each process

    while(global_bucket_min_pos!=INF){
        set<int> S; //for storing unique bucket elements before emptying it
        // if(local_bucket_min_pos!=global_bucket_min_pos){
        //     continue;
        // }
        while(local_bucket_min_pos==global_bucket_min_pos \
        && !Bucket[global_bucket_min_pos].empty()){
            // cout<<"Current bucket has elements:\n";
            // for(int i=0;i<Bucket[pos].size();i++){
            //     cout<<Bucket[pos][0]<<", ";
            // }
            // cout<<endl;
            int pos = global_bucket_min_pos;
            copy(Bucket[pos].begin(), Bucket[pos].end(), inserter(S,S.end()));
            Bucket[pos].clear();
            for(auto itr = S.begin(); itr!=S.end(); itr++){
                //int i=*itr;
                int element=*itr;
                // cout<<"Line 175, element:"<<element<<endl;
                int local_element_pos = element-global_start_index;
                /*Both light and heavy simulataneous relaxation*/
                // cout<<"Hello from line 181\n";
                for(int k=0;k<2;k++){
                    int j=0;
                    //cout<<"k value:"<<k<<'\t'<<d[3]<<endl;
                    // if(k==1){
                    //     cout<<"Line 183, local_element_pos:"<<local_element_pos\
                    //     <<", Element:"<<element<<endl;
                    // }
                    // {    
                    //     int dest = local_adj_list[1*local_no_of_vertices*MAX_DEGREE*2\
                    //     +1*MAX_DEGREE*2 + j*2 + 0];
                    //     int weight = local_adj_list[1*local_no_of_vertices*MAX_DEGREE*2\
                    //     +1*MAX_DEGREE*2 + j*2 + 1];
                    //     cout<<"k:"<<k<<" i:"<<element-global_start_index<<"("<<dest<<","<<\
                    //     weight<<")\t"<<"Global element:"<<local_element_pos+global_start_index;
                    //     cout<<endl;
                    // }
                    while(local_adj_list[k*local_no_of_vertices*MAX_DEGREE*2\
                    +local_element_pos*MAX_DEGREE*2 + j*2 + 0]!=0){
                        int dest = local_adj_list[k*local_no_of_vertices*MAX_DEGREE*2\
                        +local_element_pos*MAX_DEGREE*2 + j*2 + 0];
                        int weight = local_adj_list[k*local_no_of_vertices*MAX_DEGREE*2\
                        +local_element_pos*MAX_DEGREE*2 + j*2 + 1];
                        if(d[dest]>d[element]+weight){
                            if(dest>=global_start_index && dest<=global_end_index){
                                if(d[dest]==INF){
                                    // cout<<"Hello from line 193\n";
                                    int new_bucket_pos=(d[element]+weight)/delta;
                                    // cout<<Bucket.size()<<'\t'<<new_bucket_pos<<endl;
                                    if(Bucket.size()<=new_bucket_pos){
                                        Bucket.resize(new_bucket_pos+1);
                                        max_pos=new_bucket_pos;
                                    }
                                    // cout<<"Hello from line 199\n";
                                    Bucket[new_bucket_pos].push_back(dest);  
                                    d[dest]=d[element]+weight;
                                    //cout<<"Hello from line 201\n";                              
                                } else {
                                    int old_bucket_pos=d[dest]/delta;
                                    int new_bucket_pos=(d[element]+weight)/delta;
                                    for(int i=0;i<Bucket[old_bucket_pos].size();i++){
                                        if(dest==Bucket[old_bucket_pos][i]){
                                            Bucket[old_bucket_pos].erase(Bucket[old_bucket_pos].begin()+i);
                                            break;
                                        }
                                    }
                                    Bucket[new_bucket_pos].push_back(dest);
                                    d[dest]=d[element]+weight;
                                }
                            } else {
                                d[dest]=d[element]+weight;
                            }
                        }
                        j++;
                    }
                }
            }
            // cout<<"Hello from line 218\n";
        }
        while(local_bucket_min_pos<=max_pos && Bucket[local_bucket_min_pos].empty()){
            local_bucket_min_pos++;
        }
        if(local_bucket_min_pos>max_pos && local_bucket_min_pos!=global_bucket_min_pos){
            local_bucket_min_pos=INF;
        }
        // cout<<"Hello from line 226\n";
        MPI_Allreduce(&d[0], &newd[0], n+1,
                  MPI_INT, MPI_MIN, MPI_COMM_WORLD);

        // cout<<"From line 238,"<<d[3]<<endl;
        // cout<<"From line 239,"<<newd[3]<<endl;

        // cout<<"Hello from line 230\n";

        for(int i=0;i<local_no_of_vertices;i++){
            int source = i + global_start_index;
            if(d[source]!=newd[source]){
                if(d[source]==INF){
                    d[source]=newd[source];
                    int new_bucket_pos=(d[source])/delta;
                    if(new_bucket_pos<local_bucket_min_pos){
                        local_bucket_min_pos=new_bucket_pos;
                    }
                    // cout<<Bucket.size()<<'\t'<<new_bucket_pos<<endl;
                    if(Bucket.size()<=new_bucket_pos){
                        Bucket.resize(new_bucket_pos+1);
                        max_pos=new_bucket_pos;
                    }
                    // cout<<"Hello from line 199\n";
                    Bucket[new_bucket_pos].push_back(source);  
                    //cout<<"Hello from line 201\n";                              
                } else {
                    int old_bucket_pos=d[source]/delta;
                    int new_bucket_pos=(newd[source])/delta;
                    if(new_bucket_pos<local_bucket_min_pos){
                        local_bucket_min_pos=new_bucket_pos;
                    }
                    for(int i=0;i<Bucket[old_bucket_pos].size();i++){
                        if(source==Bucket[old_bucket_pos][i]){
                            Bucket[old_bucket_pos].erase(Bucket[old_bucket_pos].begin()+i);
                            break;
                        }
                    }
                    Bucket[new_bucket_pos].push_back(source);
                    d[source]=newd[source];
                }
            }
        }
        // cout<<"From line 277,"<<d[3]<<endl;
        // cout<<"From line 278,"<<newd[3]<<endl;
        /*Now each processor updates its buckets*/
        MPI_Allreduce(&local_bucket_min_pos, &global_bucket_min_pos, 1,
                  MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    } 

    MPI_Allreduce(&d[0], &newd[0], n+1,
                  MPI_INT, MPI_MIN, MPI_COMM_WORLD);

    local_end_time=MPI_Wtime();

    double global_start_time, global_end_time;

    MPI_Allreduce(&local_start_time, &global_start_time, 1,
                  MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);

    MPI_Allreduce(&local_end_time, &global_end_time, 1,
                  MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

    // cout<<"From line 397, rank:"<<rank<<" d[3]:"<<d[3];

    double duration = global_end_time-global_start_time;

    if(rank==0){
        cout<<"Time elapsed (exluding file handling) in ms: "<<duration*1000<<endl;;
        cout<<"The final distances are as follows:\n";
        cout<<"Vertex "<<983<<" distance:"<<newd[983]<<endl;
        // for(int i=0;i<=n;i++){
        //     if(newd[i]!=INF){
        //         cout<<"Vertex "<<i<<" distance:"<<newd[i]<<endl;
        //     }
        // }
    } 

    free(d);
    free(newd);
    Bucket.clear();   
    
    MPI_Finalize();
    
    return 0;

}