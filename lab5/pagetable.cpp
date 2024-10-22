#include <bits/stdc++.h>
using namespace std;
class PageTable {
    public:
        unordered_map<int64_t, int> pageTable;
        void addPage(int page, int frame){}
        void removePage(int page){}
        int getFrame(int page){ 
            for (auto it = pageTable.begin(); it != pageTable.end(); it++){
                if(it->second == page){
                    return it->first;
                }
            }
            return -1;
        }
        int findfreeframe(){}
        void printPageTable(){}
        void printFrameTable(){}//stores frames in fifo order
        PageTable(){
        }


};
int main(){
    
    return 0;
}
