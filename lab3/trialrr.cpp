#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <sstream>
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
    //bool isIOBurst = false;     // True if the current burst is I/O
    int ioEndTime = 0;         // When I/O will complete
    bool isComplete = false;   // To check if the process has finished
    bool inReadyQueue, inIOQueue;
};
// Function to check and add arrived processes to the ready queue
void checkArrivals(vector<Process>& processes, int currentTime, queue<int>& readyQueue) {
    for (auto& process : processes) {
        // If process has arrived and is not in the IO queue and not already in the ready queue
        if (process.arrivalTime <= currentTime && !process.inReadyQueue && !process.inIOQueue) {
            readyQueue.push(process.processID);
            process.inReadyQueue = true;  // Mark it as being in the ready queue
        }
    }
}

int main()
{
    int nop, timeQuantum;
    cout << "Enter the number of processes: ";
    cin >> nop;

    vector<Process> processes(nop);

    for (int i = 0; i < nop; ++i)
    {
        processes[i].processID = i;
        cout << "Enter the arrival time for process " << i + 1 << ": ";
        cin >> processes[i].arrivalTime;

        cout << "Enter the bursts (alternating CPU/I-O bursts) for process " << i + 1 << " (end with -1): ";
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

    cout << "Enter the time quantum: ";
    cin >> timeQuantum;

    //int currentTime = 0;
    //bool done;

    int currentTime = 0;
    int totalProcessesCompleted = 0;
    queue<int> readyQueue;  // Queue to manage processes that are ready for CPU
    vector<int> ioQueue;    // Store processes in I/O burst with their completion times

    // Add all processes that arrive at time 0 to the ready queue
    for (int i = 0; i < nop; ++i)
    {
        if (processes[i].arrivalTime == 0)
            readyQueue.push(i);
            processes[i].inReadyQueue = true; 
    }

    //cout << "\nProcess No\tBurst Time\tTurnaround Time\tWaiting Time\n";

    /*do
    {
        done = true;

        for (int i = 0; i < nop; ++i)
        {
            Process &p = processes[i];

            if (p.currentBurstIndex < p.bursts.size()) // Process still has bursts left
            {
                done = false; // There is a pending burst

                int currentBurst = p.bursts[p.currentBurstIndex];
                bool isCpuBurst = p.currentBurstIndex % 2 == 0; // Even index -> CPU burst, Odd index -> I/O burst

                if (isCpuBurst)
                {
                    cout<< "process: "<< "burst number: "<<"start: "<<"end: "<<endl;
                    if (p.arrivalTime <= currentTime) // If the process has arrived
                    {
                        int timeToExecute = min(currentBurst, timeQuantum); // Time the process can run for either the quantum or the burst whichever is smaller

                        // Increase the waiting time for all other processes in the ready queue
                        for (int j = 0; j < nop; ++j)
                        {
                            if (i != j && processes[j].arrivalTime <= currentTime && !processes[j].isComplete && processes[j].currentBurstIndex % 2 == 0)
                            {
                                // Only increase waiting time for processes that have arrived and are not in I/O
                                processes[j].waitingTime += timeToExecute;
                            }
                        }
                        cout<<i<<"\t"<<p.currentBurstIndex+1<<"\t"<<currentTime<<"\t"<<currentTime+timeToExecute<<endl;
                        // Update the process burst time and current time
                        p.bursts[p.currentBurstIndex] -= timeToExecute;
                        currentTime += timeToExecute;

                        if (p.bursts[p.currentBurstIndex] == 0) // If this CPU burst is done
                        {
                            p.currentBurstIndex++; // Move to the next burst (which may be I/O)
                        }
                    }
                    // CPU burst
                    /*
                    if (currentBurst > timeQuantum)
                    {
                        // Process not done yet, reduce remaining burst
                        p.bursts[p.currentBurstIndex] -= timeQuantum;
                        currentTime += timeQuantum;
                        cout << i+1 <<"\t" << p.currentBurstIndex/2+1 <<"\t" << currentTime-timeQuantum <<"\t" << currentTime << endl;
                    }
                    else
                    {
                        // Process completed this CPU burst
                        cout << i+1 << p.currentBurstIndex/2+1 << currentTime << currentTime+ currentBurst<< endl;
                        currentTime += currentBurst;

                        p.waitingTime += currentTime - p.bursts[p.currentBurstIndex]; // Update waiting time for CPU burst
                        p.currentBurstIndex++; // Move to the next burst (I/O)
                    }//*
                }
                else
                {
                    // I/O burst, move to the next burst without affecting CPU
                    currentTime += currentBurst; // Simulate time spent in I/O
                    p.currentBurstIndex++;        // Move to next burst (CPU)
                }

                // If it's the last CPU burst, we print the final results
                if (p.currentBurstIndex == p.bursts.size())
                {
                    p.turnaroundTime = currentTime - p.arrivalTime;
                    cout << "Process " << p.processID + 1 << "\t\t" << p.turnaroundTime << "\t\t" << p.waitingTime << endl;
                }
            }
        }
    } while (!done);*/

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
        if (process.arrivalTime <= currentTime && !process.inReadyQueue && !process.inIOQueue) {
            readyQueue.push(process.processID);
            process.inReadyQueue = true;  // Mark it as being in the ready queue
        }
        }

        if (!readyQueue.empty()) // If there are processes in the ready queue
        {
            int currentProcessID = readyQueue.front();
            readyQueue.pop();

            Process &p = processes[currentProcessID];
            int currentBurst = p.bursts[p.currentBurstIndex];
            bool isCpuBurst = p.currentBurstIndex % 2 == 0;

            if (isCpuBurst)
            {
                // Process the CPU burst for the current time quantum or until the burst completes
                int timeToExecute = min(currentBurst, timeQuantum);

                // Increment waiting time for all other ready processes
                for (int i = 0; i < nop; ++i)
                {
                    if (i != currentProcessID && processes[i].arrivalTime <= currentTime && processes[i].currentBurstIndex % 2 == 0 && !processes[i].isComplete)
                    {
                        processes[i].waitingTime += timeToExecute;
                        processes[i].inReadyQueue=true;
                        processes[i].inIOQueue=false;
                    }
                }

                // Update current burst and time
                p.bursts[p.currentBurstIndex] -= timeToExecute;
                currentTime += timeToExecute;
                for (auto& process : processes) {
                    // If process has arrived and is not in the IO queue and not already in the ready queue
                    if (process.arrivalTime <= currentTime && !process.inReadyQueue && !process.inIOQueue) {
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
                        totalProcessesCompleted++;
                        p.turnaroundTime = currentTime - p.arrivalTime;
                        p.inIOQueue=false;
                        p.inReadyQueue=false;

                        cout << "Process " << p.processID + 1 << "\t\t"
                                  << p.turnaroundTime << "\t\t" << p.waitingTime << endl;
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
                            readyQueue.push(p.processID);
                            p.inIOQueue=false;
                            p.inReadyQueue=true;
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

    cout << "Average Waiting Time: " << averageWaitingTime / nop << endl;
    cout << "Average Turnaround Time: " << averageTurnaroundTime / nop <<endl;
}
