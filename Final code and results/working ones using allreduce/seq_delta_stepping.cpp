#include<iostream>
#include<vector>
#include<string>
#include<sstream>
#include<utility>
#include<limits>
#include<fstream>
#include<chrono>
#include<set>

#define FILENAME "test_graph.mtx"
#define MAX_WEIGHT 200
#define MAX_DEGREE 12
#define DELTA 20
#define VERTICES 1957027
#define INF numeric_limits<int>::max()

using namespace std;
using namespace std::chrono;

int main(int argc, char *argv[]){

    int delta=DELTA;
    
    string line;
    ifstream file(FILENAME);
    getline(file, line); //to reject the first line
    getline(file, line);
    string str1, str2, str3;
    stringstream s(line);
    s>>str1>>str2>>str3;
    //int total_vertices=VERTICES;
    int total_vertices=stoi(str1);
    //int total_edges=stoi(str3);
    
    //adj_list[0] indicates light edges and 1 for heavy
    vector<vector<vector<pair<int,int>>>> \
    adj_list(2,(vector<vector<pair<int,int>>>(total_vertices+1)));   
    
    while(getline(file, line) && line.size()>0){
        stringstream s(line);
        s>>str1>>str2>>str3;
        int source=stoi(str1);
        int dest=stoi(str2);
        int weight=stoi(str3);
        if(weight<delta){
            adj_list[0][source].push_back(make_pair(dest, weight));
            adj_list[0][dest].push_back(make_pair(source, weight));
        } else {
            adj_list[1][source].push_back(make_pair(dest, weight));
            adj_list[1][dest].push_back(make_pair(source, weight));
        }
    }

    file.close();

    if(argc>1){
        if(stoi(argv[1])>0){
            delta = stoi(argv[1]);
        }
    }

    cout<<"The given input parameters are:\n";
    cout<<"Vertices:"<<total_vertices<<", Delta:"<<delta<<endl;
    cout<<"Maximum Degree:"<<MAX_DEGREE<<", Maximum Edge weight:"<<MAX_WEIGHT<<endl;

    auto start_time = high_resolution_clock::now();

    /*Initializing distances*/

    vector<int> d(total_vertices+1);

    
    for(int i=1;i<=total_vertices;i++){
        d[i]=INF;
    }


    //Below code for printing adjacency list of all verices
    // for(int i=1;i<=total_vertices;i++){
    //     d[i]=INF;
    //     if(!adj_list[0][i].empty() || !adj_list[1][i].empty()){
    //         cout<<i<<" has list: ";
    //         for(int j=0;j<2;j++){
    //             int length=adj_list[j][i].size();
    //             for(int k=0; k<length; k++){
    //                 cout<<"("<<adj_list[j][i][k].first<<','<<adj_list[j][i][k].second<<")\t";
    //             }
    //         }
    //         cout<<endl;
    //     }
    // }

    d[1]=0;

    //Number of buckets estimate
    //int number_of_buckets = total_vertices*MAX_WEIGHT/delta;
    //cout<<number_of_buckets<<endl;
    
    vector<vector<int>> Bucket;
    //cout<<Bucket.empty()<<endl;

    vector<int> temp={1};
    Bucket.push_back(temp);
    //cout<<Bucket.empty()<<endl;

    //cout<<INF+1<<endl;
    int pos=0;
    int max_pos=0;
    while(pos<=max_pos){
        // cout<<"Current position in Bucket.empty() loop is "<<pos<<endl;
        // for(int i=1;i<=total_vertices;i++){
        //     if(d[i]!=INF){
        //         cout<<"Vertex "<<i<<" distance:"<<d[i]<<endl;
        //     }
        // }
        set<int> S; //for storing unique bucket elements before emptying it
        vector<pair<int,int>> R; //for each neighbor and tentative distance updata
        while(pos<Bucket.size() && !Bucket[pos].empty()){
            /* cout<<"Current bucket has elements:";
            for(int i=0;i<Bucket[pos].size();i++){
                cout<<Bucket[pos][0]<<", ";
            }
            cout<<endl; */
            copy(Bucket[pos].begin(),Bucket[pos].end(),inserter(S,S.end()));
            for(int i=0;i<Bucket[pos].size();i++){
                int element=Bucket[pos][i];
                for(int j=0;j<adj_list[0][element].size();j++){
                        R.push_back(make_pair(adj_list[0][element][j].first, \
                        adj_list[0][element][j].second + \
                        d[element]));                    
                }
            }
            /* cout<<"R element, distance:("<<R[0].first<<','\
            <<R[0].second<<"), "<<d[R[0].first]<<endl; */
            Bucket[pos].clear();
            /*relaxing the light edges (all) in R*/
            for(int k=0;k<R.size();k++){
                int element = R[k].first;
                //cout<<"R element is "<<element<<endl;
                if(R[k].second<d[element]){
                    //cout<<"R light relaxation is "<<element<<endl;
                    if(d[element]<INF && R[k].second<d[element]){
                        //cout<<"Loop entered less INF\n";
                        int old_bucket_pos=d[element]/delta;
                        int new_bucket_pos=R[k].second/delta;
                        for(int i=0;i<Bucket[old_bucket_pos].size();i++){
                            if(element==Bucket[old_bucket_pos][i]){
                                Bucket[old_bucket_pos].erase(Bucket[old_bucket_pos].begin()+i);
                                break;
                            }
                        }
                        Bucket[new_bucket_pos].push_back(element);
                        d[element]=R[k].second;
                    } 
                    if(d[element]==INF && R[k].second<d[element]) {
                        //cout<<"Loop entered INF\n";
                        int new_bucket_pos=R[k].second/delta;
                        if(Bucket.size()<=new_bucket_pos){
                            Bucket.resize(new_bucket_pos+1);
                            max_pos=new_bucket_pos+1;
                        }
                        Bucket[new_bucket_pos].push_back(element);
                        d[element]=R[k].second;
                    }
                }
            }
            R.clear();
            //cout<<"Distance of 2 is "<<d[2]<<endl;
        }

        /*Now to relax all the heavy edges*/
        R.clear();
        for(auto itr = S.begin();itr!=S.end();itr++){
            //cout<<"S:"<<*itr<<endl;
            for(int j=0;j<adj_list[1][*itr].size();j++){
                R.push_back(make_pair(adj_list[1][*itr][j].first, \
                        adj_list[1][*itr][j].second + \
                        d[*itr]));
            }
        }
        // cout<<"Heavy elements are:";
        // for(int i=0;i<R.size();i++){
        //     cout<<"("<<R[i].first<<','\
        //     <<R[i].second<<"), "<<d[R[i].first]<<"\t";
        // }
        // cout<<endl;
        for(int k=0;k<R.size();k++){
                int element = R[k].first;
                if(R[k].second<d[element]){
                    if(d[element]<INF && R[k].second<d[element]){
                        int old_bucket_pos=d[element]/delta;
                        int new_bucket_pos=R[k].second/delta;
                        for(int i=0;i<Bucket[old_bucket_pos].size();i++){
                            if(element==Bucket[old_bucket_pos][i]){
                                Bucket[old_bucket_pos].erase(Bucket[old_bucket_pos].begin()+i);
                                break;
                            }
                        }
                        Bucket[new_bucket_pos].push_back(element);
                        d[element]=R[k].second;
                    } 
                    if(d[element]==INF && R[k].second<d[element]) {
                        //cout<<"Current heavy element is "<<element<<endl;
                        int new_bucket_pos=R[k].second/delta;
                        //cout<<"New position: "<<new_bucket_pos<<" Bucket size:"<<Bucket.size()<<endl;
                        if(Bucket.size()<=new_bucket_pos){
                            //cout<<"Hello from "<<element<<endl;
                            Bucket.resize(new_bucket_pos+1);
                            max_pos=new_bucket_pos+1;
                        }
                        Bucket[new_bucket_pos].push_back(element);
                        d[element]=R[k].second;
                    }
                }
            }
        pos++;
    }

    auto stop_time = high_resolution_clock::now();

    auto duration = duration_cast<microseconds>(stop_time - start_time);

    // cout<<"The number of iterations done are "<<max_pos<<endl;

    cout<<"Time elapsed (exluding file handling) in ms: "<<(double)duration.count()/1000<<endl;;

    cout<<"The final distances are as follows:\n";

    cout<<"Vertex "<<983<<" distance:"<<d[983]<<endl;

    // for(int i=1;i<=total_vertices;i++){
    //     if(d[i]!=INF){
    //         cout<<"Vertex "<<i<<" distance:"<<d[i]<<endl;
    //     }
    // }
    
    return 0;

}