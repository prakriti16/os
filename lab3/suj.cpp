#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <tuple>
#include <fstream>
#include <sstream>
#include <numeric>
#include <limits>

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

vector<Process> parseProcesses(const string &filename) {
    ifstream file(filename);
    vector<Process> processes;
    int id = 0;

    if (!file.is_open()) {
        cerr << "Failed to open file." << endl;
        return processes;
    }

    string line;
    while (getline(file, line)) {
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

void calculateAndDisplayMetrics(const vector<Process> &processes) {
    int totalCompletionTime = 0;
    int totalWaitingTime = 0;
    int maxCompletionTime = 0;
    int maxWaitingTime = 0;

    for (const auto &p : processes) {
        int burstTotal = accumulate(p.bursts.begin(), p.bursts.end(), 0,
            [](int sum, const Burst &b) { return sum + b.duration; });
        int waitingTime = p.completionTime - p.arrivalTime - burstTotal;
        totalWaitingTime += waitingTime;
        totalCompletionTime += p.completionTime;
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
        cout << "ProcessId " << p.id << " Arrival Time: " << p.arrivalTime
             << " Completion Time: " << p.completionTime
             << " Waiting Time: " << waitingTime << endl;
    }
    cout << "-----------------" << endl;
    cout << "Make Span: " << makespan << endl;
    cout << "Completion Time: [AVG]: " << avgCompletionTime
         << " [MAX]: " << maxCompletionTime << endl;
    cout << "Waiting Time: [AVG]: " << avgWaitingTime
         << " [MAX]: " << maxWaitingTime << endl;
    cout << "-----------------" << endl;
}

void fifoScheduler(vector<Process> &processes) {
    queue<Process*> readyQueue;
    queue<Process*> waitQueue;
    vector<tuple<int, int, int>> schedule; // (ProcessID, Start Time, End Time)
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
            schedule.emplace_back(p->id, burstStart, burstEnd);

            currentTime = burstEnd;
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
        cout << "P" << get<0>(entry) << " " << get<1>(entry) << ", " << get<2>(entry) << endl;
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
else {
        cerr << "Unknown scheduling algorithm: " << algorithm << endl;
        return 1;
    }

    return 0;
}