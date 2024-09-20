#include<bits/stdc++.h>

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
    bool inIO;
    bool completed;
    int totbursts;
    int id; // To keep track of process IDs
        int turnAroundTime = 0; 
};

struct ProcessRR
{
    int processID;
    int arrivalTime;
    vector<int> bursts; // Alternating CPU and I/O bursts
    int totbursts = 0;
    int turnaroundTime = 0;
    int currentBurstIndex = 0; // Tracks the current burst (either CPU or I/O)
    int ioEndTime = 0;         // When I/O will complete
    bool isComplete = false;   // To check if the process has finished
    bool inReadyQueue, inIOQueue,inpartialQueue;
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
        p.totbursts=0;

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
                p.totbursts+=burst.ioDuration+burst.duration;
                
            }
        }
        processes.push_back(p);
    }
    file.close();
    return processes;
}


// Comparator for the min-heap (priority queue)
struct SJFComparator {
    bool operator()(const Process* a, const Process* b) {
        return a->bursts[a->currentBurstIndex].duration > b->bursts[b->currentBurstIndex].duration;
    }
};
// Function to sort a queue based on lastIOEndTime
void sortQueueByLastIOEndTime(queue<Process*>& waitQueue) {
    // Transfer elements from queue to vector
    vector<Process*> waitVector;
    while (!waitQueue.empty()) {
        waitVector.push_back(waitQueue.front());
        waitQueue.pop();
    }

    // Sort the vector based on lastIOEndTime
    sort(waitVector.begin(), waitVector.end(), [](Process* a, Process* b) {
        return a->lastIOEndTime < b->lastIOEndTime;
    });

    // Push sorted elements back to the queue
    for (Process* p : waitVector) {
        waitQueue.push(p);
    }
}


void calculateAndDisplayMetrics(const vector<Process> &processes) {

    
    int totalCompletionTime = 0;
    int totalWaitingTime = 0;
    int maxCompletionTime = 0;
    int maxWaitingTime = 0;
    int maxexit=0;
    int minarr=0;

    for (const auto &p : processes) {
        int burstTotal = p.totbursts;
        int waitingTime = p.completionTime - p.arrivalTime - burstTotal;
        totalWaitingTime += waitingTime;
        totalCompletionTime += p.completionTime - p.arrivalTime;
        maxWaitingTime = max(maxWaitingTime, waitingTime);
        maxCompletionTime = max(maxCompletionTime, p.completionTime - p.arrivalTime);
        maxexit=max(maxexit,p.completionTime);
        minarr=min(minarr,p.arrivalTime);
    }
    cout<<totalWaitingTime<<endl;
    int makespan = maxexit-minarr;
    double avgCompletionTime = static_cast<double>(totalCompletionTime) / processes.size();
    double avgWaitingTime = static_cast<double>(totalWaitingTime) / processes.size();

    // Output metrics
    cout << "********* METRICS *********" << endl;
    for (const auto &p : processes) {
        int burstTotal = p.totbursts;
        cout << "P" << p.id+1 << " Arrival Time: " << p.arrivalTime
             << " Exit Time: " << p.completionTime
             << " Waiting Time: " << p.waitTime << endl;
    }
    cout << "-----------------" << endl;
    cout << "Make Span: " << makespan << endl;
    cout << "Completion Time: [AVG]: " << avgCompletionTime<<endl;
    cout << "Completion Time: [MAX]: " << maxCompletionTime << endl;
    cout << "Waiting Time: [AVG]: " << avgWaitingTime<<endl;
    cout<< "Waiting Time: [MAX]: " << maxWaitingTime << endl;
}

void psjfScheduler(vector<Process> &processes) {
    priority_queue<Process*, vector<Process*>, SJFComparator> readyQueue;  // Min-heap priority queue for CPU-ready processes
    queue<Process*> waitQueue;  // Queue for processes in I/O
    vector<tuple<int, int, int, int>> schedule;  // (ProcessID, current burst, Start Time, End Time)
    int currentTime = 0;

    // Copy and sort processes by arrival time
    vector<Process*> processPtrs;
    for (auto &p : processes) {
        processPtrs.push_back(&p);
    }
    sort(processPtrs.begin(), processPtrs.end(), [](Process *a, Process *b) {
        return a->arrivalTime < b->arrivalTime;
    });

    // Initialize remaining time for all processes
    for (auto &p : processes) {
        p.remainingTime = accumulate(p.bursts.begin(), p.bursts.end(), 0,
                                     [](int sum, const Burst &b) { return sum + b.duration; });
        p.currentBurstIndex = 0;  // Reset current burst index
    }

    // Add the first process that arrives at time 0 or earliest time to the ready queue
    if (!processPtrs.empty() && processPtrs[0]->arrivalTime <= currentTime) {
        readyQueue.push(processPtrs[0]);
        processPtrs.erase(processPtrs.begin());
    }

    while (!readyQueue.empty() || !waitQueue.empty() || !processPtrs.empty()) {
        // Check for processes that completed their I/O and move them back to the ready queue
                  // Sort the queue
    sortQueueByLastIOEndTime(waitQueue);
        while (!waitQueue.empty() && waitQueue.front()->lastIOEndTime <= currentTime) {
            Process* p = waitQueue.front();
            waitQueue.pop();
            p->inIO = false;  // Mark as no longer in I/O
            readyQueue.push(p);  // Now ready for CPU
        }

        // Check for new processes that have arrived and add them to the ready queue
        while (!processPtrs.empty() && processPtrs.front()->arrivalTime <= currentTime) {
            readyQueue.push(processPtrs.front());
            processPtrs.erase(processPtrs.begin());
        }

        if (readyQueue.empty()) {
            // If no processes are in the ready queue, advance time to the next I/O completion or process arrival
            int nextEventTime = INT_MAX;

            // Find the nearest I/O completion time
            if (!waitQueue.empty()) {
                nextEventTime = waitQueue.front()->lastIOEndTime;
            }

            // Find the nearest process arrival time
            if (!processPtrs.empty()) {
                nextEventTime = min(nextEventTime, processPtrs.front()->arrivalTime);
            }

            // Advance current time to the next event
            if (nextEventTime != INT_MAX) {
                currentTime = nextEventTime;
            }
            continue;
        }

        // Pick the process with the shortest remaining time
        Process* p = readyQueue.top();
        readyQueue.pop();

        if (p->arrivalTime > currentTime) {
            currentTime = p->arrivalTime;
        }

        if (p->currentBurstIndex < p->bursts.size()) {
            Burst &burst = p->bursts[p->currentBurstIndex];
            int burstStart = currentTime;
            int timeToExecute = min(1, burst.duration);  // Execute for only 1 second
            int burstEnd = burstStart + timeToExecute;
            schedule.emplace_back(p->id + 1, p->currentBurstIndex + 1, burstStart, burstEnd);

            // Update current time after executing for 1 second
            currentTime = burstEnd;
            p->remainingTime -= timeToExecute;
            burst.duration -= timeToExecute;  // Reduce the burst duration by the time executed
            priority_queue<Process*, vector<Process*>, SJFComparator> temp=readyQueue; 
            while(!readyQueue.empty()){
                Process* p1 = readyQueue.top();
                readyQueue.pop();
                p1->waitTime+=timeToExecute;
            }
            readyQueue=temp;
            // If the burst is not yet completed, push the process back into the ready queue
            if (burst.duration > 0) {
                // Re-insert the process into the ready queue with updated remaining time
                readyQueue.push(p);  
            } else {
                // If the process has more bursts (I/O or CPU)
                if (p->currentBurstIndex < p->bursts.size()) {
                    Burst &nextBurst = p->bursts[p->currentBurstIndex];
                    p->lastIOEndTime = currentTime + nextBurst.ioDuration;  // Calculate the I/O end time
                    p->inIO = true;  // Mark as in I/O
                    waitQueue.push(p); 
                    p->currentBurstIndex++;
                    // Mark as completed
                    p->completionTime = currentTime;
                    p->completed = true;
                }
            }
        }
    }

    // Print the scheduling results, consolidating continuous entries for the same process
    if (!schedule.empty()) {
        int lastProcessID = get<0>(schedule[0]);
        int lastBurstIndex = get<1>(schedule[0]);
        int lastStartTime = get<2>(schedule[0]);
        int lastEndTime = get<3>(schedule[0]);

        for (size_t i = 1; i < schedule.size(); ++i) {
            int currentProcessID = get<0>(schedule[i]);
            int currentBurstIndex = get<1>(schedule[i]);
            int currentStartTime = get<2>(schedule[i]);
            int currentEndTime = get<3>(schedule[i]);

            if (currentProcessID == lastProcessID && currentBurstIndex == lastBurstIndex && currentStartTime == lastEndTime) {
                // Extend the end time of the last entry if the current entry is for the same process and contiguous
                lastEndTime = currentEndTime;
            } else {
                // Print the last entry
                cout << "P" << lastProcessID << "," << lastBurstIndex << "\t " << lastStartTime << "\t " << lastEndTime - 1 << endl;
                // Update to the current entry
                lastProcessID = currentProcessID;
                lastBurstIndex = currentBurstIndex;
                lastStartTime = currentStartTime;
                lastEndTime = currentEndTime;
            }
        }
        // Print the last entry
        cout << "P" << lastProcessID << "," << lastBurstIndex << "\t " << lastStartTime << "\t " << lastEndTime - 1 << endl;
    }

    // Calculate and display metrics (waiting time, turnaround time, etc.)
    calculateAndDisplayMetrics(processes);
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

    // Add only the first process that arrives at time 0 or earliest time to the ready queue
    if (!processPtrs.empty() && processPtrs[0]->arrivalTime <= currentTime) {
        readyQueue.push(processPtrs[0]);
        processPtrs.erase(processPtrs.begin());
    }

    while (!readyQueue.empty() || !waitQueue.empty() || !processPtrs.empty()) {
        // Move processes that have completed their I/O back to the ready queue
          // Sort the queue
    sortQueueByLastIOEndTime(waitQueue);
        while (!waitQueue.empty() && waitQueue.front()->lastIOEndTime <= currentTime) {
            Process* p = waitQueue.front();
            waitQueue.pop();
            p->inIO = false;
            readyQueue.push(p);
        }

        // Check if any process has arrived and push it into the ready queue
        while (!processPtrs.empty() && processPtrs.front()->arrivalTime <= currentTime) {
            readyQueue.push(processPtrs.front());
            processPtrs.erase(processPtrs.begin());
        }

        if (readyQueue.empty()) {
            // If no processes are in the ready queue, advance time to the next I/O completion or process arrival
            int nextEventTime = INT_MAX;

            // Find the nearest I/O completion time
            if (!waitQueue.empty()) {
                nextEventTime = waitQueue.front()->lastIOEndTime;
            }

            // Find the nearest process arrival time
            if (!processPtrs.empty()) {
                nextEventTime = min(nextEventTime, processPtrs.front()->arrivalTime);
            }

            // Advance the current time to the next significant event
            if (nextEventTime != INT_MAX) {
                currentTime = nextEventTime;
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
            schedule.emplace_back(p->id+1, p->currentBurstIndex+1, burstStart, burstEnd);

            currentTime = burstEnd;
            p->remainingTime -= burst.duration;
            queue<Process*> temp=readyQueue; 
            while(!readyQueue.empty()){
                Process* p1 = readyQueue.front();
                readyQueue.pop();
                p1->waitTime+= burst.duration;
            }
            readyQueue=temp;

            if (p->currentBurstIndex < p->bursts.size()) {
                Burst &nextBurst = p->bursts[p->currentBurstIndex];
                p->lastIOEndTime = currentTime + nextBurst.ioDuration;
                p->inIO = true;
                waitQueue.push(p);
                 p->currentBurstIndex++;
                 p->completionTime = currentTime;
                p->completed = true;
            }
        }
    }

    for (const auto &entry : schedule) {
        cout << "P" << get<0>(entry) << "," << get<1>(entry) << "\t " << get<2>(entry)<< "\t " << get<3>(entry)-1 << endl;
    }

    calculateAndDisplayMetrics(processes);
}





void sjfScheduler(vector<Process> &processes) {
    priority_queue<Process*, vector<Process*>, SJFComparator> readyQueue;  // Min-heap priority queue for CPU-ready processes
    queue<Process*> waitQueue;  // Queue for processes in I/O
    vector<tuple<int, int, int, int>> schedule;  // (ProcessID, current burst, Start Time, End Time)
    int currentTime = 0;

    // Copy and sort processes by arrival time
    vector<Process*> processPtrs;
    for (auto &p : processes) {
        processPtrs.push_back(&p);
    }
    sort(processPtrs.begin(), processPtrs.end(), [](Process *a, Process *b) {
        return a->arrivalTime < b->arrivalTime;
    });

    // Add the first process that arrives at time 0 or earliest time to the ready queue
    if (!processPtrs.empty() && processPtrs[0]->arrivalTime <= currentTime) {
        readyQueue.push(processPtrs[0]);
        processPtrs.erase(processPtrs.begin());
    }

    while (!readyQueue.empty() || !waitQueue.empty() || !processPtrs.empty()) {
        // Check for processes that completed their I/O and move them back to the ready queue

    sortQueueByLastIOEndTime(waitQueue);

        while (!waitQueue.empty() && waitQueue.front()->lastIOEndTime <= currentTime) {
            Process* p = waitQueue.front();
            waitQueue.pop();

            p->inIO = false;  // Mark as no longer in I/O
            readyQueue.push(p);  // Now ready for CPU
        }

        // Check for new processes that have arrived and add them to the ready queue
        while (!processPtrs.empty() && processPtrs.front()->arrivalTime <= currentTime) {
            readyQueue.push(processPtrs.front());
            processPtrs.erase(processPtrs.begin());
        }

        if (readyQueue.empty()) {
            // If no processes are in the ready queue, advance time to the next I/O completion or process arrival
            int nextEventTime = INT_MAX;

            // Find the nearest I/O completion time
            if (!waitQueue.empty()) {
                nextEventTime = waitQueue.front()->lastIOEndTime;
            }

            // Find the nearest process arrival time
            if (!processPtrs.empty()) {
                nextEventTime = min(nextEventTime, processPtrs.front()->arrivalTime);
            }

            // Advance current time to the next event
            if (nextEventTime != INT_MAX) {
                currentTime = nextEventTime;
            }
            continue;
        }

        // Pick the process with the shortest burst time
        Process* p = readyQueue.top();
        readyQueue.pop();


        if (p->arrivalTime > currentTime) {
            currentTime = p->arrivalTime;
        }
        
        if (p->currentBurstIndex < p->bursts.size()) {
            Burst &burst = p->bursts[p->currentBurstIndex];
            int burstStart = currentTime;
            int burstEnd = burstStart + burst.duration;
            schedule.emplace_back(p->id + 1, p->currentBurstIndex + 1, burstStart, burstEnd);
            priority_queue<Process*, vector<Process*>, SJFComparator> temp=readyQueue; 
            while(!readyQueue.empty()){
                Process* p1 = readyQueue.top();
                readyQueue.pop();
                p1->waitTime+=burst.duration;
            }
            readyQueue=temp;
            // Update current time after executing the CPU burst
            currentTime = burstEnd;
            p->remainingTime -= burst.duration;

            // If the process has more bursts (I/O or CPU)
            if (p->currentBurstIndex < p->bursts.size()) {
                Burst &nextBurst = p->bursts[p->currentBurstIndex];
                p->lastIOEndTime = currentTime + nextBurst.ioDuration;  // Calculate the I/O end time
                p->inIO = true;  // Mark as in I/O
                waitQueue.push(p);  
                p->currentBurstIndex++;
                p->completionTime = currentTime;
                p->completed = true;
                // Move the process to the wait queue for I/O
            } 
        }
    }

    // Print the scheduling results
    for (const auto &entry : schedule) {
        cout << "P" << get<0>(entry) << "," << get<1>(entry) << "\t " << get<2>(entry) << "\t " << get<3>(entry) - 1 << endl;
    }

    // Calculate and display metrics (waiting time, turnaround time, etc.)
    calculateAndDisplayMetrics(processes);
}



void rrsched(const char* filename){
    
    freopen(filename, "r", stdin);
    string h1,h2,h3,hend;
    cin>>h1>>h2>>h3;
    int timeQuantum=10;
    ifstream file(filename);
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
    vector<ProcessRR> processes(nop);

    for (int i = 0; i < nop; ++i)
    {
        processes[i].processID = i;
        cin >> processes[i].arrivalTime;
        int burst;
        while (true)
        {
            cin >> burst;
            if (burst == -1)
                break;
            processes[i].bursts.push_back(burst);
            processes[i].totbursts+=burst;
        }

        processes[i].currentBurstIndex = 0;
    }
    cin >> hend;

    int currentTime = 0;
    int totalProcessesCompleted = 0;
    queue<int> readyQueue;  // Queue to manage processes that are ready for CPU
    vector<int> ioQueue;    // Store processes in I/O burst with their completion times

    // Add all processes that arrive at time 0 to the ready queue
    for (int i = 0; i < nop; ++i)
    {
        if (processes[i].arrivalTime == 0){
            readyQueue.push(i);
            processes[i].inReadyQueue = true; 
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
                    cout << "P" << p.processID + 1 << ","<<cpburind
                                    <<"\t"<< currentTime << "\t" << currentTime+ timeToExecute-1<< endl;
                        


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
                        }
                    }
                    else
                    {
                        // Re-add the process to the ready queue if it still has CPU burst left
                        readyQueue.push(currentProcessID);
                        p.inIOQueue=false;
                        p.inReadyQueue=true;
                    }
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
    int maxwaittime=0,maxcomplettime=0,minmkspt=0,maxmkspt=0;
    
    
    cout << "********* METRICS *********" << endl;
    for (int i = 0; i < nop; ++i)
    {
        
        int waitingTime=processes[i].turnaroundTime-processes[i].totbursts;
        maxwaittime = max(maxwaittime,waitingTime);
        maxcomplettime = max(maxcomplettime, processes[i].turnaroundTime);
        maxmkspt=max(maxmkspt,processes[i].comptime);
        minmkspt=min(minmkspt,processes[i].arrivalTime);
        averageWaitingTime += waitingTime;
        averageTurnaroundTime += processes[i].turnaroundTime;
        cout<<"P"<<i+1<<" Arrival time: "<<processes[i].arrivalTime<<" Exit time: "<<processes[i].comptime<<" Waiting time: "<<waitingTime<<endl;

    }
    int makspan=maxmkspt-minmkspt;
    cout << "-----------------" << endl;
    cout<<"Makespan: "<<makspan<<endl;
    cout << "Completion Time: [AVG]: " << (float)averageTurnaroundTime / (float)nop <<endl;
    cout << "Completion Time: [MAX]: " << maxcomplettime <<endl;
    cout << "Waiting Time: [AVG]: " <<(float) averageWaitingTime / (float)nop << endl;
    cout << "Waiting Time: [MAX]: " << maxwaittime << endl;
}


int main(int argc, char *argv[]) {
    clock_t start, end;
    start = clock(); 

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
       else if (algorithm == "SJF") {
        vector<Process> processes = parseProcesses(filename);
        if (processes.empty()) {
            cerr << "No processes to schedule." << endl;
            return 1;
        }
        sjfScheduler(processes);
    }
       else if (algorithm == "PSJF") {
        vector<Process> processes = parseProcesses(filename);
        if (processes.empty()) {
            cerr << "No processes to schedule." << endl;
            return 1;
        }
        psjfScheduler(processes);
    }
else {
        cerr << "Unknown scheduling algorithm: " << algorithm << endl;
        return 1;
    }
    end = clock();
    double totruntime = double(end - start) / double(CLOCKS_PER_SEC);
    cout<<"Total execution time (in seconds): "<<totruntime<<endl;
    cout << "-----------------" << endl;

    return 0;
}
