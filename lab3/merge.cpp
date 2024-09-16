#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <tuple>
#include <fstream>
#include <sstream>
#include <numeric>
#include <climits>
using namespace std;

struct Burst {
    int duration;
    int ioDuration;
};

struct Process {
    int arrivalTime;
    vector<Burst> bursts;
    int remainingTime;
    int waitTime;
    int completionTime;
    int lastIOEndTime;
    int currentBurstIndex;
    int totruntime;
    bool inIO;
    bool completed;
    int id; // To keep track of process IDs
};
struct ProcessRR
{
    int processID;
    int arrivalTime;
    vector<int> bursts; // Alternating CPU and I/O bursts
    int waitingTime = 0;
    int turnaroundTime = 0;
    int currentBurstIndex = 0; // Tracks the current burst (either CPU or I/O)
    //bool isIOBurst = false;     // True if the current burst is I/O
    int ioEndTime = 0;         // When I/O will complete
    bool isComplete = false;   // To check if the process has finished
    bool inReadyQueue, inIOQueue;
    int comptime=-1;//completion time
};
vector<Process> parseProcesses(const string &filename) {
    ifstream file(filename);
    vector<Process> processes;
    int id = 0;

    if (!file.is_open()) {
        cerr << "Failed to open file." << endl;
        return processes;
    }

    string line;
    getline(file, line);
    getline(file, line);
    getline(file, line);
    while (getline(file, line)) {
        if(line.find("</pre>") != string::npos){
            break;
        }
        stringstream iss(line);
        Process p;
        p.id = id++;
        p.arrivalTime = -1;
        p.remainingTime = 0;
        p.waitTime = 0;
        p.completionTime = 0;
        p.lastIOEndTime = 0;
        p.currentBurstIndex = 0;
        p.inIO = false;
        p.completed = false;
        p.totruntime=0;

        int value;
        while (iss >> value) {
            if (value == -1) {
                break;
            }
            if (p.arrivalTime == -1) {
                p.arrivalTime = value;
            } else {
                Burst burst;
                burst.duration = value;
                if (!(iss >> value) || value == -1) {
                    burst.ioDuration = 0;
                } else {
                    burst.ioDuration = value;
                }
                p.bursts.push_back(burst);
            }
        }
        processes.push_back(p);
    }
    file.close();
    return processes;
}

void calculateAndDisplayMetrics(const vector<Process> &processes) {
    int totalCompletionTime = 0;
    int totalWaitingTime = 0;
    int maxCompletionTime = 0;
    int maxWaitingTime = 0;
    int totruntime=0;

    for (const auto &p : processes) {
        int burstTotal = accumulate(p.bursts.begin(), p.bursts.end(), 0,
            [](int sum, const Burst &b) { return sum + b.duration; });
        int waitingTime = p.completionTime - p.arrivalTime - burstTotal;
        totalWaitingTime += waitingTime;
        totalCompletionTime += p.completionTime;
        totruntime+=p.totruntime;
        maxWaitingTime = max(maxWaitingTime, waitingTime);
        maxCompletionTime = max(maxCompletionTime, p.completionTime);
    }

    int makespan = maxCompletionTime;
    double avgCompletionTime = static_cast<double>(totalCompletionTime) / processes.size();
    double avgWaitingTime = static_cast<double>(totalWaitingTime) / processes.size();

    // Output metrics
    cout << "********* METRICS *********" << endl;
    for (const auto &p : processes) {
        int burstTotal = accumulate(p.bursts.begin(), p.bursts.end(), 0,
            [](int sum, const Burst &b) { return sum + b.duration; });
        int waitingTime = p.completionTime - p.arrivalTime - burstTotal;
        cout << "P" << p.id+1 << " Arrival Time: " << p.arrivalTime
             << " Completion Time: " << p.completionTime
             << " Waiting Time: " << waitingTime << endl;
    }
    cout << "-----------------" << endl;
    cout << "Make Span: " << makespan << endl;
    cout<<"Total execution time: "<<totruntime<<endl;
    cout << "Completion Time: [AVG]: " << avgCompletionTime<<endl;
    cout << "Completion Time: [MAX]: " << maxCompletionTime << endl;
    cout << "Waiting Time: [AVG]: " << avgWaitingTime<<endl;
    cout<< "Waiting Time: [MAX]: " << maxWaitingTime << endl;
    cout << "-----------------" << endl;
}
void rrsched(const char* filename){
    
    freopen(filename, "r", stdin);
    string h1,h2,h3,hend;
    cin>>h1>>h2>>h3;
    //cout<<h1<<h2<<h3<<endl;
    //freopen("output.txt", "w", stdout);
    int timeQuantum=10;
    //cout << "Enter the number of processes: ";
    //cin >> nop;
    ifstream file(filename);
    //vector<Process> processes;
    int id = 0,nop=-4;

    if (!file.is_open()) {
        cerr << "Failed to open file." << endl;
        exit(0);
    }

    string line;
    while (getline(file, line)) {
        nop++;
    }
    file.close();
    //cout<<nop1;
    vector<ProcessRR> processes(nop);

    for (int i = 0; i < nop; ++i)
    {
        processes[i].processID = i;
        //cout << "Enter the arrival time for process " << i + 1 << ": ";
        cin >> processes[i].arrivalTime;
        //cout << "Enter the arrival time for process " << i + 1 << ": "<<processes[i].arrivalTime<<endl;
        //cout << "Enter the bursts (alternating CPU/I-O bursts) for process " << i + 1 << " (end with -1): ";
        int burst;
        while (true)
        {
            cin >> burst;
            if (burst == -1)
                break;
            processes[i].bursts.push_back(burst);
            //cout << "Enter the bursts (alternating CPU/I-O bursts) for process " << i + 1 << " (end with -1): "<<processes[i].bursts.back()<<endl;
        }

        processes[i].currentBurstIndex = 0;
    }

    //cout << "Enter the time quantum: ";
    cin >> hend;
    //cout << "Enter the time quantum: "<<timeQuantum;

    //int currentTime = 0;
    //bool done;

    int currentTime = 0;
    int totalProcessesCompleted = 0;
    int totruntime=0; //total runtime without waiting time
    queue<int> readyQueue;  // Queue to manage processes that are ready for CPU
    vector<int> ioQueue;    // Store processes in I/O burst with their completion times

    // Add all processes that arrive at time 0 to the ready queue
    for (int i = 0; i < nop; ++i)
    {
        if (processes[i].arrivalTime == 0){
            readyQueue.push(i);
            processes[i].inReadyQueue = true; 
            //cout<<i<<processes[i].inReadyQueue<<endl;
        }
    }

    // Loop until all processes have completed
    while (totalProcessesCompleted < nop)
    {
        // Move processes that have completed their I/O back to the ready queue
        for (auto it = ioQueue.begin(); it != ioQueue.end();)
        {
            int pid = *it;
            if (processes[pid].ioEndTime <= currentTime)
            {
                readyQueue.push(pid); // Move to ready queue
                it = ioQueue.erase(it); // Remove from I/O queue
                processes[pid].inReadyQueue=true;
                processes[pid].inIOQueue=false;
                //processes[pid].currentBurstIndex+=1;
                //cout<<processes[pid].processID<<"ct"<<currentTime<<endl;
            }
            else
            {
                ++it;
            }
        }

        for (auto& process : processes) {
        // If process has arrived and is not in the IO queue and not already in the ready queue
        if (process.arrivalTime <= currentTime && !process.inReadyQueue && !process.inIOQueue && !process.isComplete) {
            readyQueue.push(process.processID);
            process.inReadyQueue = true;  // Mark it as being in the ready queue
            
        }
        }

        if (!readyQueue.empty()) // If there are processes in the ready queue
        {
            int currentProcessID = readyQueue.front();
            readyQueue.pop();

            ProcessRR &p = processes[currentProcessID];
            if (p.currentBurstIndex < p.bursts.size()) {
                int currentBurst = p.bursts[p.currentBurstIndex];
                bool isCpuBurst = p.currentBurstIndex % 2 == 0;

                if (isCpuBurst)
                {
                    int cpburind=p.currentBurstIndex/2+1;
                    
                    // Process the CPU burst for the current time quantum or until the burst completes
                    int timeToExecute = min(currentBurst, timeQuantum);
                    totruntime+=timeToExecute;
                    cout << "P" << p.processID + 1 << ","<<cpburind
                                    <<"\t"<< currentTime << "\t" << currentTime+ timeToExecute<< endl;
                        
                    // Increment waiting time for all other ready processes
                    for (int i = 0; i < nop; ++i)
                    {
                        if (i != currentProcessID && processes[i].inReadyQueue )
                        {
                            processes[i].waitingTime += timeToExecute;
                        }
                    }

                    // Update current burst and time
                    p.bursts[p.currentBurstIndex] -= timeToExecute;
                    currentTime += timeToExecute;
                    for (auto& process : processes) {
                        // If process has arrived and is not in the IO queue and not already in the ready queue
                        if (process.arrivalTime <= currentTime && !process.inReadyQueue && !process.inIOQueue && !process.isComplete) {
                            readyQueue.push(process.processID);
                            process.inReadyQueue = true;  // Mark it as being in the ready queue
                            
                        }
                        
                    }

                    if (p.bursts[p.currentBurstIndex] == 0) // If the CPU burst is finished
                    {
                        p.currentBurstIndex++; // Move to the next burst, which might be I/O
                        if (p.currentBurstIndex == p.bursts.size()) // If all bursts are done
                        {
                            p.isComplete = true;
                            p.comptime=currentTime;
                            totalProcessesCompleted++;
                            p.turnaroundTime = currentTime - p.arrivalTime;
                            p.inIOQueue=false;
                            p.inReadyQueue=false;

                            //cout << "P" << p.processID + 1 << ","<< p.turnaroundTime << "\t\t" << p.waitingTime << endl;
                        }
                        else
                        {
                            // If the next burst is an I/O burst, move to I/O queue
                            if (p.currentBurstIndex % 2 != 0)
                            {
                                int ioBurst = p.bursts[p.currentBurstIndex];
                                p.ioEndTime = currentTime + ioBurst; // Set when I/O will finish
                                ioQueue.push_back(p.processID);      // Move to I/O queue
                                p.currentBurstIndex++;               // Move past the I/O burst
                                p.inIOQueue=true;
                                p.inReadyQueue=false;
                            }
                            else
                            {
                                // If the next burst is CPU, re-add the process to ready queue
                                //cout<<"errcpuuagain?"<<endl;
                            }
                        }
                    }
                    else
                    {
                        // Re-add the process to the ready queue if it still has CPU burst left
                        readyQueue.push(currentProcessID);
                        p.inIOQueue=false;
                        p.inReadyQueue=true;
                        //cout<<"how much more ugh"<<p.bursts[p.currentBurstIndex]<<endl;
                        //cout<<"waitwattttt"<<p.currentBurstIndex<<"omgggg"<<p.bursts.size()<<endl;
                        
                    }
                }
                else{
                    //cout<<"nevercomeback"<<endl;
                }
                
            }
            else{
                p.isComplete = true;
                p.comptime=currentTime;
                //cout<<p.processID<<"finishd at"<<currentTime<<endl;
                totalProcessesCompleted++;
                p.turnaroundTime = currentTime - p.arrivalTime;
                p.inIOQueue=false;
                p.inReadyQueue=false;
            }
        }
        else
        {
            // If no processes are ready, advance time to the next I/O completion or process arrival
            int nextEventTime = INT_MAX;

            // Find the nearest I/O completion time
            for (int i = 0; i < ioQueue.size(); ++i)
            {
                nextEventTime = min(nextEventTime, processes[ioQueue[i]].ioEndTime);
            }

            // Find the nearest process arrival time
            for (int i = 0; i < nop; ++i)
            {
                if (processes[i].arrivalTime > currentTime && !processes[i].isComplete)
                {
                    nextEventTime = min(nextEventTime, processes[i].arrivalTime);
                }
            }

            // Advance the current time to the next significant event
            if(nextEventTime!=INT_MAX)
                currentTime = nextEventTime;
        }
    }

    // Calculate the average waiting and turnaround times
    float averageWaitingTime = 0, averageTurnaroundTime = 0;
    for (int i = 0; i < nop; ++i)
    {
        averageWaitingTime += processes[i].waitingTime;
        averageTurnaroundTime += processes[i].turnaroundTime;
    }
    int maxwaittime=0,maxcomplettime=0,minmkspt=0,maxmkspt=0;
    
    
    cout << "********* METRICS *********" << endl;
    for (int i = 0; i < nop; ++i)
    {
        maxwaittime = max(maxwaittime,processes[i].waitingTime);
        maxcomplettime = max(maxcomplettime, processes[i].turnaroundTime);
        maxmkspt=max(maxmkspt,processes[i].comptime);
        minmkspt=min(minmkspt,processes[i].arrivalTime);
        cout<<"P"<<i+1<<" Arrival time: "<<processes[i].arrivalTime<<" Completion time: "<<processes[i].comptime<<" Waiting time: "<<processes[i].waitingTime<<endl;

    }
    int makspan=maxmkspt-minmkspt;
    cout << "-----------------" << endl;
    cout<<"Makespan: "<<makspan<<endl;
    cout<<"Total execution time: "<<totruntime<<endl;
    cout << "Completion Time: [AVG]: " << averageTurnaroundTime / nop <<endl;
    cout << "Completion Time: [MAX]: " << maxcomplettime <<endl;
    cout << "Waiting Time: [AVG]: " << averageWaitingTime / nop << endl;
    cout << "Waiting Time: [MAX]: " << maxwaittime << endl;
    cout << "-----------------" << endl;
}
void fifoScheduler(vector<Process> &processes) {
    queue<Process*> readyQueue;
    queue<Process*> waitQueue;
    vector<tuple<int,int, int, int>> schedule; // (ProcessID, current burst, Start Time, End Time)
    int currentTime = 0;

    // Copy and sort processes by arrival time
    vector<Process*> processPtrs;
    for (auto &p : processes) {
        processPtrs.push_back(&p);
    }
    sort(processPtrs.begin(), processPtrs.end(), [](Process *a, Process *b) {
        return a->arrivalTime < b->arrivalTime;
    });

    // Add processes to the ready queue initially
    for (auto p : processPtrs) {
        readyQueue.push(p);
    }

    while (!readyQueue.empty() || !waitQueue.empty()) {
        while (!waitQueue.empty() && waitQueue.front()->lastIOEndTime <= currentTime) {
            Process* p = waitQueue.front();
            waitQueue.pop();
            p->inIO = false;
            readyQueue.push(p);
        }

        if (readyQueue.empty()) {
            if (processPtrs.empty()) break;

            auto nextArrivalIt = min_element(processPtrs.begin(), processPtrs.end(),
                [currentTime](Process *a, Process *b) {
                    return (a->arrivalTime > currentTime) ? false : (a->arrivalTime < b->arrivalTime);
                });

            if (nextArrivalIt != processPtrs.end()) {
                currentTime = (*nextArrivalIt)->arrivalTime;
            }
            continue;
        }

        Process* p = readyQueue.front();
        readyQueue.pop();

        if (p->arrivalTime > currentTime) {
            currentTime = p->arrivalTime;
        }

        if (p->currentBurstIndex < p->bursts.size()) {
            Burst &burst = p->bursts[p->currentBurstIndex];
            int burstStart = currentTime;
            int burstEnd = burstStart + burst.duration;
            schedule.emplace_back(p->id+1,p->currentBurstIndex+1, burstStart, burstEnd);

            currentTime = burstEnd;
            p->totruntime+=burst.duration;
            p->currentBurstIndex++;
            p->remainingTime -= burst.duration;

            if (p->currentBurstIndex < p->bursts.size()) {
                Burst &nextBurst = p->bursts[p->currentBurstIndex];
                p->lastIOEndTime = currentTime + nextBurst.ioDuration;
                p->inIO = true;
                waitQueue.push(p);
            } else {
                p->completionTime = currentTime;
                p->completed = true;
            }
        }
    }

    for (const auto &entry : schedule) {
        cout << "P" << get<0>(entry) << "," << get<1>(entry) << "\t " << get<2>(entry)<< "\t " << get<3>(entry) << endl;
    }

    calculateAndDisplayMetrics(processes);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <scheduling_algorithm> <file>" << endl;
        return 1;
    }

    string algorithm = argv[1];
    string filename = argv[2];

    if (algorithm == "FIFO") {
        vector<Process> processes = parseProcesses(filename);
        if (processes.empty()) {
            cerr << "No processes to schedule." << endl;
            return 1;
        }
        fifoScheduler(processes);
    }
    else if(algorithm=="RR"){
        const char* filename1=argv[2];
        rrsched(filename1);
    }
else {
        cerr << "Unknown scheduling algorithm: " << algorithm << endl;
        return 1;
    }

    return 0;
}