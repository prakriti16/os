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
        int burstTotal = accumulate(p.bursts.begin(), p.bursts.end(), 0,
            [](int sum, const Burst &b) { return sum + b.duration; });
        totalWaitingTime += p.waitTime;
        totalCompletionTime += p.completionTime - p.arrivalTime;
        maxWaitingTime = max(maxWaitingTime, p.waitTime);
        maxCompletionTime = max(maxCompletionTime, p.completionTime - p.arrivalTime);
        maxexit=max(maxexit,p.completionTime);
        minarr=min(minarr,p.arrivalTime);
    }

    int makespan = maxexit-minarr;
    double avgCompletionTime = static_cast<double>(totalCompletionTime) / processes.size();
    double avgWaitingTime = static_cast<double>(totalWaitingTime) / processes.size();

    // Output metrics
    cout << "********* METRICS *********" << endl;
    for (const auto &p : processes) {
        int burstTotal = accumulate(p.bursts.begin(), p.bursts.end(), 0,
            [](int sum, const Burst &b) { return sum + b.duration; });
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


void fifowith2Processor(vector<Process> &processes) {
    queue<Process*> readyQueue;
    queue<Process*> waitQueue;
    vector<tuple<int, int, int, int, int>> schedule; // (ProcessID, BurstIndex, Start Time, End Time, CPU ID)
    
    int currentTimeCPU0 = 0;  // Track time for CPU 0
    int currentTimeCPU1 = 0;  // Track time for CPU 1

    // Copy and sort processes by arrival time
    vector<Process*> processPtrs;
    for (auto &p : processes) {
        processPtrs.push_back(&p);
    }
    sort(processPtrs.begin(), processPtrs.end(), [](Process *a, Process *b) {
        return a->arrivalTime < b->arrivalTime;
    });

    // Add the first process that arrives at time 0 or earliest time to the ready queue
    if (!processPtrs.empty() && processPtrs[0]->arrivalTime <= min(currentTimeCPU0, currentTimeCPU1)) {
        readyQueue.push(processPtrs[0]);
        processPtrs.erase(processPtrs.begin());
    }

    while (!readyQueue.empty() || !waitQueue.empty() || !processPtrs.empty()) {
        // Move processes that have completed their I/O back to the ready queue
          // Sort the queue
    sortQueueByLastIOEndTime(waitQueue);
        while (!waitQueue.empty() && waitQueue.front()->lastIOEndTime <= min(currentTimeCPU0, currentTimeCPU1)) {
            Process* p = waitQueue.front();
            waitQueue.pop();
            p->inIO = false;
            readyQueue.push(p);
        }

        // Check if any process has arrived and push it into the ready queue
        while (!processPtrs.empty() && processPtrs.front()->arrivalTime <= min(currentTimeCPU0, currentTimeCPU1)) {
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

            // Advance both CPU times to the next significant event
            if (nextEventTime != INT_MAX) {
                currentTimeCPU0 = max(currentTimeCPU0, nextEventTime);
                currentTimeCPU1 = max(currentTimeCPU1, nextEventTime);
            }
            continue;
        }

        // Assign process to the CPU that's available first (either CPU 0 or CPU 1)
        int currentCPU = (currentTimeCPU0 <= currentTimeCPU1) ? 0 : 1;
        int &currentTime = (currentCPU == 0) ? currentTimeCPU0 : currentTimeCPU1;

        Process* p = readyQueue.front();
        readyQueue.pop();

        if (p->arrivalTime > currentTime) {
            currentTime = p->arrivalTime;
        }

        if (p->currentBurstIndex < p->bursts.size()) {
            Burst &burst = p->bursts[p->currentBurstIndex];
            int burstStart = currentTime;
            int burstEnd = burstStart + burst.duration;
            
            // Assign the current process to a CPU and add to the schedule
            schedule.emplace_back(p->id + 1, p->currentBurstIndex + 1, burstStart, burstEnd, currentCPU);
            currentTime = burstEnd;
            p->remainingTime -= burst.duration;

            // Update waiting times for all other processes in the ready queue
            queue<Process*> temp = readyQueue;
            while (!readyQueue.empty()) {
                Process* p1 = readyQueue.front();
                readyQueue.pop();
                p1->waitTime += burst.duration;
            }
            readyQueue = temp;

            // After the current burst, push the process to the wait queue if there's an I/O burst
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

    // Output the schedule separately for CPU 0 and CPU 1
    cout << "********* CPU 0 Schedule *********" << endl;
    for (const auto &entry : schedule) {
        if (get<4>(entry) == 0) {
            cout << "P" << get<0>(entry) << "," << get<1>(entry)
                 << "\t Start: " << get<2>(entry)
                 << "\t End: " << get<3>(entry) - 1 << endl;
        }
    }

    cout << "********* CPU 1 Schedule *********" << endl;
    for (const auto &entry : schedule) {
        if (get<4>(entry) == 1) {
            cout << "P" << get<0>(entry) << "," << get<1>(entry)
                 << "\t Start: " << get<2>(entry)
                 << "\t End: " << get<3>(entry) - 1 << endl;
        }
    }

    // Calculate and display metrics
    calculateAndDisplayMetrics(processes);
}


void sjfwith2Processor(vector<Process> &processes) {
    priority_queue<Process*, vector<Process*>, SJFComparator> readyQueue;  // Min-heap priority queue for CPU-ready processes
    queue<Process*> waitQueue;  // Queue for processes in I/O
     // Queue for processes in I/O
    vector<tuple<int, int, int, int, int>> schedule; // (ProcessID, BurstIndex, Start Time, End Time, CPU ID)
    int currentTimeCPU0 = 0;  // Track time for CPU 0
    int currentTimeCPU1 = 0;  // Track time for CPU 1

    // Copy and sort processes by arrival time
    vector<Process*> processPtrs;
    for (auto &p : processes) { 
        processPtrs.push_back(&p);
    }
    sort(processPtrs.begin(), processPtrs.end(), [](Process *a, Process *b) {
        return a->arrivalTime < b->arrivalTime;
    });

    // Add the first process that arrives at time 0 or earliest time to the ready queue
    while (!processPtrs.empty() && processPtrs[0]->arrivalTime <= min(currentTimeCPU0, currentTimeCPU1)) {
        readyQueue.push(processPtrs[0]);
        processPtrs.erase(processPtrs.begin());
    }

    while (!readyQueue.empty() || !waitQueue.empty() || !processPtrs.empty()) {
        // Check for processes that completed their I/O and move them back to the ready queue
  // Sort the queue
    sortQueueByLastIOEndTime(waitQueue);

        while (!waitQueue.empty() && waitQueue.front()->lastIOEndTime <= min(currentTimeCPU0, currentTimeCPU1)) {

            Process* p = waitQueue.front();
            waitQueue.pop();
            p->inIO = false;  // Mark as no longer in I/O

            readyQueue.push(p);  // Now ready for CPU
        }

        // Check for new processes that have arrived and add them to the ready queue
        while (!processPtrs.empty() && processPtrs.front()->arrivalTime <= min(currentTimeCPU0, currentTimeCPU1)) {

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

            // Advance both CPU times to the next significant event
            if (nextEventTime != INT_MAX) {
                currentTimeCPU0 = max(currentTimeCPU0, nextEventTime);
                currentTimeCPU1 = max(currentTimeCPU1, nextEventTime);
            }
            continue;
        }

        // Assign process to the CPU that's available first (either CPU 0 or CPU 1)
        int currentCPU = (currentTimeCPU0 <= currentTimeCPU1) ? 0 : 1;
        int &currentTime = (currentCPU == 0) ? currentTimeCPU0 : currentTimeCPU1;

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

            schedule.emplace_back(p->id + 1, p->currentBurstIndex + 1, burstStart, burstEnd, currentCPU);
            
            // Update waiting times for other processes in the ready queue
            priority_queue<Process*, vector<Process*>, SJFComparator> temp = readyQueue; 
            while(!readyQueue.empty()){
                Process* p1 = readyQueue.top();
                readyQueue.pop();
                p1->waitTime += burst.duration;
            }
            readyQueue = temp;

            // Update current time after executing the CPU burst
            currentTime = burstEnd;

            if(currentCPU==0)
            {
                currentTimeCPU0=currentTime;
            }
            else{
                currentTimeCPU1=currentTime;
            }

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
            } 
        }
    }

    // Output the schedule separately for CPU 0 and CPU 1
    cout << "********* CPU 0 Schedule *********" << endl;
    for (const auto &entry : schedule) {
        if (get<4>(entry) == 0) {
            cout << "P" << get<0>(entry) << "," << get<1>(entry)
                 << "\t Start: " << get<2>(entry)
                 << "\t End: " << get<3>(entry) - 1 << endl;
        }
    }

    cout << "********* CPU 1 Schedule *********" << endl;
    for (const auto &entry : schedule) {
        if (get<4>(entry) == 1) {
            cout << "P" << get<0>(entry) << "," << get<1>(entry)
                 << "\t Start: " << get<2>(entry)
                 << "\t End: " << get<3>(entry) - 1 << endl;
        }
    }

    // Calculate and display metrics (waiting time, turnaround time, etc.)
    calculateAndDisplayMetrics(processes);
}


void psjfSchedulerTwoProcessors(vector<Process> &processes) {
    priority_queue<Process*, vector<Process*>, SJFComparator> readyQueue;  // Min-heap priority queue for CPU-ready processes
    queue<Process*> waitQueue;  // Queue for processes in I/O
    vector<tuple<int, int, int, int, int>> schedule;  // (ProcessID, current burst, Start Time, End Time, CPU ID)
    int currentTimeCPU0 = 0, currentTimeCPU1 = 0;  // Track current time for both CPUs

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
    if (!processPtrs.empty() && processPtrs[0]->arrivalTime <= min(currentTimeCPU0, currentTimeCPU1)) {
        readyQueue.push(processPtrs[0]);
        processPtrs.erase(processPtrs.begin());
    }

    while (!readyQueue.empty() || !waitQueue.empty() || !processPtrs.empty()) {
        // Check for processes that completed their I/O and move them back to the ready queue
        sortQueueByLastIOEndTime(waitQueue);
        while (!waitQueue.empty() && waitQueue.front()->lastIOEndTime <= min(currentTimeCPU0, currentTimeCPU1)) {
            Process* p = waitQueue.front();
            waitQueue.pop();
            p->inIO = false;  // Mark as no longer in I/O
            readyQueue.push(p);  // Now ready for CPU
        }

        // Check for new processes that have arrived and add them to the ready queue
        while (!processPtrs.empty() && processPtrs.front()->arrivalTime <= min(currentTimeCPU0, currentTimeCPU1)) {
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

            // Advance both CPU times to the next event
            if (nextEventTime != INT_MAX) {
                currentTimeCPU0 = max(currentTimeCPU0, nextEventTime);
                currentTimeCPU1 = max(currentTimeCPU1, nextEventTime);
            }
            continue;
        }

        // Assign process to the CPU that's available first (either CPU 0 or CPU 1)
        int currentCPU = (currentTimeCPU0 <= currentTimeCPU1) ? 0 : 1;
        int &currentTime = (currentCPU == 0) ? currentTimeCPU0 : currentTimeCPU1;

        // Pick the process with the shortest remaining time
        Process* p = readyQueue.top();
        readyQueue.pop();

        if (p->arrivalTime > currentTime) {
            currentTime = p->arrivalTime;
        }

        if (p->currentBurstIndex < p->bursts.size()) {
            Burst &burst = p->bursts[p->currentBurstIndex];
            int burstStart = currentTime;
            int timeToExecute = min(1, burst.duration);  // Execute for only 1 second (PSJF logic)
            int burstEnd = burstStart + timeToExecute;

            schedule.emplace_back(p->id + 1, p->currentBurstIndex + 1, burstStart, burstEnd, currentCPU);

            // Update current time after executing for 1 second
            currentTime = burstEnd;
            p->remainingTime -= timeToExecute;
            burst.duration -= timeToExecute;  // Reduce the burst duration by the time executed

            // Update waiting times for other processes in the ready queue
            priority_queue<Process*, vector<Process*>, SJFComparator> temp = readyQueue; 
            while (!readyQueue.empty()) {
                Process* p1 = readyQueue.top();
                readyQueue.pop();
                p1->waitTime += timeToExecute;
            }
            readyQueue = temp;

            // If the burst is not yet completed, push the process back into the ready queue
            if (burst.duration > 0) {
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

    // Consolidate and print the scheduling results for each CPU
    for (int cpu = 0; cpu <= 1; cpu++) {
        cout << "********* CPU " << cpu << " Schedule *********" << endl;
        vector<tuple<int, int, int, int>> consolidatedSchedule;
        int lastPID = -1, lastBurst = -1, lastEndTime = -1, lastStartTime = -1;
        
        for (const auto &entry : schedule) {
            if (get<4>(entry) == cpu) {  // For the current CPU
                int pid = get<0>(entry);
                int burst = get<1>(entry);
                int startTime = get<2>(entry);
                int endTime = get<3>(entry);

                if (pid == lastPID && burst == lastBurst && startTime == lastEndTime) {
                    // Consolidate continuous execution
                    lastEndTime = endTime;
                } else {
                    // If not continuous, push the previous entry and reset
                    if (lastPID != -1) {
                        consolidatedSchedule.emplace_back(lastPID, lastBurst, lastStartTime, lastEndTime);
                    }
                    lastPID = pid;
                    lastBurst = burst;
                    lastStartTime = startTime;
                    lastEndTime = endTime;
                }
            }
        }
        // Push the last consolidated entry
        if (lastPID != -1) {
            consolidatedSchedule.emplace_back(lastPID, lastBurst, lastStartTime, lastEndTime);
        }

        // Print the consolidated schedule
        for (const auto &entry : consolidatedSchedule) {
            cout << "P" << get<0>(entry) << "," << get<1>(entry)
                 << "\t Start: " << get<2>(entry)
                 << "\t End: " << get<3>(entry) - 1 << endl;
        }
    }

    // Calculate and display metrics (waiting time, turnaround time, etc.)
    calculateAndDisplayMetrics(processes);
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
        fifowith2Processor(processes);
    }
      else  if (algorithm == "SJF") {
        vector<Process> processes = parseProcesses(filename);
        if (processes.empty()) {
            cerr << "No processes to schedule." << endl;
            return 1;
        }
        sjfwith2Processor(processes);
    }
      else  if (algorithm == "PSJF") {
        vector<Process> processes = parseProcesses(filename);
        if (processes.empty()) {
            cerr << "No processes to schedule." << endl;
            return 1;
        }
        psjfSchedulerTwoProcessors(processes);
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
