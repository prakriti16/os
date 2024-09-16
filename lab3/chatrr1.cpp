#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <climits>
using namespace std;

struct Process
{
    int processID;
    int arrivalTime;
    vector<int> bursts; // Alternating CPU and I/O bursts
    int waitingTime = 0;
    int turnaroundTime = 0;
    int currentBurstIndex = 0; // Tracks the current burst (either CPU or I/O)
    int ioEndTime = 0;         // When I/O will complete
    bool isComplete = false;   // To check if the process has finished
    bool inReadyQueue = false, inIOQueue = false;
};

// Function to check and add arrived processes to the ready queue
void checkArrivals(vector<Process>& processes, int currentTime, queue<int>& readyQueue) {
    for (auto& process : processes) {
        if (process.arrivalTime <= currentTime && !process.inReadyQueue && !process.inIOQueue && !process.isComplete) {
            readyQueue.push(process.processID);
            process.inReadyQueue = true;  // Mark it as being in the ready queue
        }
    }
}

int main()
{
    freopen("input.txt", "r", stdin);
    int nop, timeQuantum;
    cin >> nop;

    vector<Process> processes(nop);

    // Input for processes
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
        }
        processes[i].currentBurstIndex = 0;
    }

    cin >> timeQuantum;

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
                processes[pid].inReadyQueue = true;
                processes[pid].inIOQueue = false;
            }
            else
            {
                ++it;
            }
        }

        // Check for new arrivals and add them to the ready queue
        checkArrivals(processes, currentTime, readyQueue);

        if (!readyQueue.empty()) // If there are processes in the ready queue
        {
            int currentProcessID = readyQueue.front();
            readyQueue.pop();

            Process &p = processes[currentProcessID];
            p.inReadyQueue = false;

            if (p.currentBurstIndex < p.bursts.size()) {
                int currentBurst = p.bursts[p.currentBurstIndex];
                bool isCpuBurst = p.currentBurstIndex % 2 == 0;

                if (isCpuBurst)
                {
                    // Process the CPU burst for the current time quantum or until the burst completes
                    int timeToExecute = min(currentBurst, timeQuantum);

                    // Increment waiting time for all other ready processes
                    for (int i = 0; i < nop; ++i)
                    {
                        if (i != currentProcessID && processes[i].inReadyQueue)
                        {
                            processes[i].waitingTime += timeToExecute;
                        }
                    }

                    // Update current burst and time
                    p.bursts[p.currentBurstIndex] -= timeToExecute;
                    currentTime += timeToExecute;

                    // Check if the CPU burst is finished
                    if (p.bursts[p.currentBurstIndex] == 0) 
                    {
                        p.currentBurstIndex++; // Move to the next burst, which might be I/O
                        
                        // If all bursts are done
                        if (p.currentBurstIndex == p.bursts.size()) 
                        {
                            p.isComplete = true;
                            totalProcessesCompleted++;
                            p.turnaroundTime = currentTime - p.arrivalTime;
                            p.inIOQueue = false;
                            p.inReadyQueue = false;

                            cout << "Process " << p.processID + 1 << "\t\t"
                                    << p.turnaroundTime << "\t\t" << p.waitingTime << endl;
                        }
                        // If the next burst is an I/O burst, move to I/O queue
                        else if (p.currentBurstIndex % 2 != 0) 
                        {
                            int ioBurst = p.bursts[p.currentBurstIndex];
                            p.ioEndTime = currentTime + ioBurst; // Set when I/O will finish
                            ioQueue.push_back(p.processID);      // Move to I/O queue
                            p.inIOQueue = true;
                        }
                    }
                    else
                    {
                        // Re-add the process to the ready queue if it still has CPU burst left
                        readyQueue.push(currentProcessID);
                        p.inReadyQueue = true;
                    }
                }
            }
            else{
                p.isComplete = true;
                totalProcessesCompleted++;
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
            if (nextEventTime != INT_MAX)
                currentTime = nextEventTime;
            else
                break;  // Prevent infinite loop if no future event exists
        }
    }

    // Calculate the average waiting and turnaround times
    float averageWaitingTime = 0, averageTurnaroundTime = 0;
    for (int i = 0; i < nop; ++i)
    {
        averageWaitingTime += processes[i].waitingTime;
        averageTurnaroundTime += processes[i].turnaroundTime;
    }

    cout << "Average Waiting Time: " << averageWaitingTime / nop << endl;
    cout << "Average Turnaround Time: " << averageTurnaroundTime / nop << endl;
}
