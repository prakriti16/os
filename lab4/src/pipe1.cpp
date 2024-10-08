#include <bits/stdc++.h>
using namespace std;
int main(){
    pid_t p2pid,p3pid;
    p2pid=fork();
    int pipefd[2];
    if(p2pid>0){//parent p1 s1
        //do s1
        //send to s2
        
        //receive hash okay from s1 then continue (else receive wrong hash then output error and break or send again?)

    }
    else{//child p2 s2
        int pipefd2[2];
        //pause();//not parallel anymore hmm
        p3pid=fork();
        if(p3pid>0){//child s2
            //read pipefd do hash check
            //s2
            //write to pipefd2
        }
        else{//grandchild s3
            //read pipefd2 do hash check
            //s3
        }

    }
}