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
    int comptime=-1;//completion time
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
    freopen("input.txt", "r", stdin);
    //freopen("output.txt", "w", stdout);
    int nop, timeQuantum;
    //cout << "Enter the number of processes: ";
    cin >> nop;

    vector<Process> processes(nop);

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
    cin >> timeQuantum;
    //cout << "Enter the time quantum: "<<timeQuantum;

    //int currentTime = 0;
    //bool done;

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

            Process &p = processes[currentProcessID];
            if (p.currentBurstIndex < p.bursts.size()) {
                int currentBurst = p.bursts[p.currentBurstIndex];
                bool isCpuBurst = p.currentBurstIndex % 2 == 0;

                if (isCpuBurst)
                {
                    int cpburind=p.currentBurstIndex/2+1;
                    // Process the CPU burst for the current time quantum or until the burst completes
                    int timeToExecute = min(currentBurst, timeQuantum);
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
    for (int i = 0; i < nop; ++i)
    {
        maxwaittime = max(maxwaittime,processes[i].waitingTime);
        maxcomplettime = max(maxcomplettime, processes[i].turnaroundTime);
        maxmkspt=max(maxmkspt,processes[i].comptime);
        minmkspt=min(minmkspt,processes[i].arrivalTime);
        cout<<"P"<<i+1<<" arrival time: "<<processes[i].arrivalTime<<" completion time: "<<processes[i].comptime<<" waiting time: "<<processes[i].waitingTime<<endl;

    }
    int makspan=maxmkspt-minmkspt;
    cout<<"Makespan: "<<makspan<<endl;
    cout << "Average Waiting Time: " << averageWaitingTime / nop << endl;
    cout << "mean completion Time: " << averageTurnaroundTime / nop <<endl;
    cout << "maximum Waiting Time: " << maxwaittime << endl;
    cout << "maximum Turnaround Time: " << maxcomplettime <<endl;
}
