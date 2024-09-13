#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <sstream>

using namespace std;

struct Burst {
    int cpu_burst;
    int io_burst;
};

struct Process {
    int id;
    int arrival_time;
    vector<Burst> bursts;
    int current_burst; // Track current CPU burst being executed buttttt dont we need to track with two seperat epointers 
    //because one goes in 
    int remaining_time; // Time left for current CPU burst (for preemption)

    Process(int id, int arrival) : id(id), arrival_time(arrival), current_burst(1), remaining_time(0) {}
};

vector<Process> parseWorkload(char* workload_description_file) {
    vector<Process> processes;
    ifstream file(workload_description_file);
    string line;
    int process_id = 1;

    while (getline(file, line)) {
        stringstream ss(line);
        int arrival_time, cpu_burst, io_burst;
        char comma;
        ss >> arrival_time; // Arrival time of process
        Process p(process_id++, arrival_time);

        while (ss >> cpu_burst) {
            if (cpu_burst == -1) break; // End of process description
            ss >> comma >> io_burst >> comma; // Skip commas
            p.bursts.push_back({cpu_burst, io_burst});
        }
        processes.push_back(p);
    }

    return processes;
}

void fifoScheduling(vector<Process>& processes) {
    queue<Process*> ready_queue;
    int current_time = 0;

    for (auto& process : processes) {
        ready_queue.push(&process);
    }

    // Run through the processes in FIFO order
    while (!ready_queue.empty()) {
        Process* current_process = ready_queue.front();
        ready_queue.pop();

        for (auto& burst : current_process->bursts) {
            cout << "Process P" << current_process->id << " starts CPU burst at " << current_time;
            current_time += burst.cpu_burst;
            cout << " and finishes at " << current_time << endl;

            // I/O Burst
            if (burst.io_burst != -1) {
                cout << "Process P" << current_process->id << " starts I/O burst." << endl;
            }
        }
    }
}

void calculateMetrics(const vector<Process>& processes) {
    int total_completion_time = 0;
    int max_completion_time = 0;
    int makespan = 0;
    int total_waiting_time = 0;
    int max_waiting_time = 0;

    for (const auto& process : processes) {
        int completion_time = process.arrival_time; // Calculate when the process finishes
        total_completion_time += completion_time;
        max_completion_time = max(max_completion_time, completion_time);
        
        // Calculate waiting times
        int waiting_time = 0; // This needs to consider preemption
        total_waiting_time += waiting_time;
        max_waiting_time = max(max_waiting_time, waiting_time);
    }

    double average_completion_time = (double)total_completion_time / processes.size();
    double average_waiting_time = (double)total_waiting_time / processes.size();

    cout << "Average Completion Time: " << average_completion_time << endl;
    cout << "Maximum Completion Time: " << max_completion_time << endl;
    cout << "Makespan: " << makespan << endl;
    cout << "Average Waiting Time: " << average_waiting_time << endl;
    cout << "Maximum Waiting Time: " << max_waiting_time << endl;
}

int main(int argc, char **argv)
{
	if(argc != 3)
	{
		cout <<"usage: ./partitioner.out  <scheduling-algorithm> <path-to-workload-description-file> \nprovided arguments:\n";
		for(int i = 0; i < argc; i++)
			cout << argv[i] << "\n";
		return -1;
	}

    char *scheduling_algo = argv[1];
	char *workload_description_file = argv[2];
    ifstream file(workload_description_file);
    
    vector<Process> p1= parseWorkload(workload_description_file);
    for( auto i: p1){
        cout<<i.current_burst<<endl;
    }

    fifoScheduling(p1);
    calculateMetrics(p1);
}