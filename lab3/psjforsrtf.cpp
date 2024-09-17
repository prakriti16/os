#include <iostream>
#include <vector>
#include <queue>
#include <climits>

using namespace std;

struct Process
{
    int processID;
    int arrivalTime;
    vector<int> bursts;       // Alternating CPU and I/O bursts
    int currentBurstIndex = 0; // Tracks current burst (either CPU or I/O)
    int remainingTime = 0;     // Remaining time of the current CPU burst
    int waitingTime = 0;
    int turnaroundTime = 0;
    int ioEndTime = 0;         // When I/O will complete
    bool isComplete = false;
    bool inReadyQueue = false;
    bool inIOQueue = false;
};

void checkArrivals(vector<Process>& processes, int currentTime, queue<int>& readyQueue) {
    for (auto& process : processes) {
        // Add process to ready queue if it has arrived and isn't in any queue
        if (process.arrivalTime <= currentTime && !process.isComplete && !process.inReadyQueue && !process.inIOQueue) {
            readyQueue.push(process.processID);
            process.inReadyQueue = true;
            process.inIOQueue = false;
        }
    }
}

// Move processes from IO to the ready queue when I/O is done
void moveIOToReady(vector<Process>& processes, int currentTime, vector<int>& ioQueue, queue<int>& readyQueue) {
    for (auto it = ioQueue.begin(); it != ioQueue.end();) {
        int pid = *it;
        if (processes[pid].ioEndTime <= currentTime) {
            readyQueue.push(pid); // Move to ready queue
            processes[pid].inReadyQueue = true;
            processes[pid].inIOQueue = false;
            it = ioQueue.erase(it); // Remove from I/O queue
        } else {
            ++it;
        }
    }
}

// Find the next process to run based on the shortest remaining CPU time
int findNextProcess(vector<Process>& processes, int currentTime, queue<int>& readyQueue) {
    int shortestProcessIndex = -1;
    int shortestRemainingTime = INT_MAX;

    // Go through ready queue to find the process with shortest remaining time
    queue<int> tempQueue=readyQueue;
    while (!readyQueue.empty()) {
        int pid = readyQueue.front();
        readyQueue.pop();

        if (processes[pid].remainingTime < shortestRemainingTime && processes[pid].arrivalTime <= currentTime) {
            shortestRemainingTime = processes[pid].remainingTime;
            shortestProcessIndex = pid;
        }
    }

    // Restore the ready queue after searching
    readyQueue = tempQueue;
    return shortestProcessIndex;
}

void srtScheduling(vector<Process>& processes) {
    int currentTime = 0;
    int totalProcessesCompleted = 0;
    queue<int> readyQueue;  // Queue for processes ready for CPU
    vector<int> ioQueue;    // List of processes doing I/O

    // Initially, check if any processes have arrived at time 0
    checkArrivals(processes, currentTime, readyQueue);

    while (totalProcessesCompleted < processes.size()) {
        // Move processes from I/O to ready if they have finished I/O
        moveIOToReady(processes, currentTime, ioQueue, readyQueue);

        // Check for new arrivals at the current time
        checkArrivals(processes, currentTime, readyQueue);

        // Find the process with the shortest remaining time
        int nextProcessID = findNextProcess(processes, currentTime, readyQueue);

        if (nextProcessID == -1) {
            // If no process is ready, advance time to the next event (either IO completion or process arrival)
            int nextEventTime = INT_MAX;

            // Find the nearest I/O completion
            for (int pid : ioQueue) {
                nextEventTime = min(nextEventTime, processes[pid].ioEndTime);
            }

            // Find the nearest process arrival
            for (const auto& process : processes) {
                if (!process.isComplete && process.arrivalTime > currentTime) {
                    nextEventTime = min(nextEventTime, process.arrivalTime);
                }
            }

            currentTime = nextEventTime;
            continue;
        }

        Process& currentProcess = processes[nextProcessID];
        currentProcess.inReadyQueue = false;

        // Get the current CPU burst and process it
        int timeToExecute = min(currentProcess.remainingTime, 1); // Process 1 unit of time
        currentProcess.remainingTime -= timeToExecute;
        currentTime += timeToExecute;

        // If the current CPU burst finishes
        if (currentProcess.remainingTime == 0) {
            currentProcess.currentBurstIndex++;

            // If all bursts are complete, mark process as finished
            if (currentProcess.currentBurstIndex >= currentProcess.bursts.size()) {
                currentProcess.isComplete = true;
                totalProcessesCompleted++;
                currentProcess.turnaroundTime = currentTime - currentProcess.arrivalTime;
                currentProcess.inReadyQueue = false;
                currentProcess.inIOQueue = false;

                cout << "Process " << currentProcess.processID + 1 << "\t\t"
                     << currentProcess.turnaroundTime << "\t\t" << currentProcess.waitingTime << endl;
            }
            // If the next burst is I/O, move process to the I/O queue
            else if (currentProcess.currentBurstIndex % 2 == 1) {
                int ioBurst = currentProcess.bursts[currentProcess.currentBurstIndex];
                currentProcess.ioEndTime = currentTime + ioBurst;
                ioQueue.push_back(nextProcessID);
                currentProcess.inIOQueue = true;
                currentProcess.inReadyQueue = false;
            }
            // If there are more CPU bursts, prepare for the next CPU burst
            else {
                /*currentProcess.remainingTime = currentProcess.bursts[currentProcess.currentBurstIndex];
                readyQueue.push(nextProcessID);
                currentProcess.inReadyQueue = true;*/
                cout<<"not posssssssss";
            }
        }
    }

    // Calculate average waiting and turnaround times
    float averageWaitingTime = 0, averageTurnaroundTime = 0;
    for (const auto& process : processes) {
        averageWaitingTime += process.waitingTime;
        averageTurnaroundTime += process.turnaroundTime;
    }

    cout << "Average Waiting Time: " << averageWaitingTime / processes.size() << endl;
    cout << "Average Turnaround Time: " << averageTurnaroundTime / processes.size() << endl;
}

int main() {
    int nop; // Number of processes
    
    freopen("input.txt","r",stdin);
    cin >> nop;
    vector<Process> processes(nop);

    // Input each process's arrival time and bursts
    for (int i = 0; i < nop; i++) {
        processes[i].processID = i;
        cin >> processes[i].arrivalTime;
        int burst;
        while (cin >> burst && burst != -1) {
            processes[i].bursts.push_back(burst);
            //cout<<processes[i].bursts.back()<<" ";
        }
        //cout<<endl;
        processes[i].remainingTime = processes[i].bursts[0]; // Initial CPU burst
    }

    // Run the SRT scheduling
    srtScheduling(processes);

    return 0;
}
