#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <sstream>
using namespace std;
struct Process
{
    int processID;
    int arrivalTime;
    vector<int> bursts; // Alternating CPU and I/O bursts
    int waitingTime = 0;
    int turnaroundTime = 0;
    int currentBurstIndex = 0; // Tracks the current burst (either CPU or I/O)
    bool isIOBurst = false;     // True if the current burst is I/O
};

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

    std::cout << "Enter the time quantum: ";
    std::cin >> timeQuantum;

    int currentTime = 0;
    bool done;

    std::cout << "\nProcess No\tBurst Time\tTurnaround Time\tWaiting Time\n";

    do
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
                    // CPU burst
                    if (currentBurst > timeQuantum)
                    {
                        // Process not done yet, reduce remaining burst
                        p.bursts[p.currentBurstIndex] -= timeQuantum;
                        currentTime += timeQuantum;
                    }
                    else
                    {
                        // Process completed this CPU burst
                        currentTime += currentBurst;
                        p.waitingTime += currentTime - p.bursts[p.currentBurstIndex]; // Update waiting time for CPU burst
                        p.currentBurstIndex++; // Move to the next burst (I/O)
                    }
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
                    std::cout << "Process " << p.processID + 1 << "\t\t" << p.turnaroundTime << "\t\t" << p.waitingTime << std::endl;
                }
            }
        }
    } while (!done);

    // Calculate the average waiting and turnaround times
    float averageWaitingTime = 0, averageTurnaroundTime = 0;
    for (int i = 0; i < nop; ++i)
    {
        averageWaitingTime += processes[i].waitingTime;
        averageTurnaroundTime += processes[i].turnaroundTime;
    }

    std::cout << "Average Waiting Time: " << averageWaitingTime / nop << std::endl;
    std::cout << "Average Turnaround Time: " << averageTurnaroundTime / nop << std::endl;
}
