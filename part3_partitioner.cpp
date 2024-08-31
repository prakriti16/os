#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <cerrno>
#include <vector>
#include <csignal>

using namespace std;

vector<pid_t> child_pids;

void sigusr1_handler(int signal)
{
    if (signal == SIGUSR1)
    {
        for (pid_t pid : child_pids)
        {
            kill(pid, SIGTERM);
        }
        if (getppid() > 1) {
            kill(getppid(), SIGUSR1);
        }
        exit(0);
    }
}

void signal_handler(int signal)
{
    if (signal == SIGTERM)
    {
        cout << "[" << getpid() << "] received SIGTERM\n" << std::flush;
        for (pid_t pid : child_pids)
        {
            kill(pid, SIGTERM);//recursively calls signal_handler to kill all children.
        }
        exit(0);
    }
}

int main(int argc, char **argv)
{
    if (argc != 6)
    {
        cerr << "Usage: ./part3_partitioner.out <path-to-file> <pattern> <search-start-position> <search-end-position> <max-chunk-size>\n";
        return -1;
    }

    signal(SIGUSR1, sigusr1_handler);
    signal(SIGTERM, signal_handler);

    char *file_to_search_in = argv[1];
    char *pattern_to_search_for = argv[2];
    int search_start_position = atoi(argv[3]);
    int search_end_position = atoi(argv[4]);
    int max_chunk_size = atoi(argv[5]);

    int chunk_size = search_end_position - search_start_position + 1;

    cout << "[" << getpid() << "] start position = " << search_start_position << " ; end position = " << search_end_position << "\n";

    if (chunk_size > max_chunk_size)
    {
        int mid = search_start_position + (chunk_size / 2);

        pid_t left_pid = fork();
        if (left_pid == 0)
        {
            cout << "[" << getppid() << "] forked left child " << getpid() << "\n";
            execl("./part3_partitioner.out", "part3_partitioner.out", file_to_search_in, pattern_to_search_for, argv[3], to_string(mid).c_str(),
                  to_string(max_chunk_size).c_str(), NULL);
            cerr << "Failed to execute left partitioner.\n";
            return -1;
        }
        else if (left_pid > 0)
        {
            child_pids.push_back(left_pid);
            pid_t right_pid = fork();
            if (right_pid == 0)
            {
                cout << "[" << getppid() << "] forked right child " << getpid() << "\n";
                execl("./part3_partitioner.out", "part3_partitioner.out", file_to_search_in, pattern_to_search_for, to_string(mid + 1).c_str(),
                      argv[4], to_string(max_chunk_size).c_str(), NULL);
                cerr << "Failed to execute right partitioner.\n";
                return -1;
            }
            else if (right_pid > 0)
            {
                child_pids.push_back(right_pid);
                int status;

                if (waitpid(left_pid, &status, 0) == -1)
                {
                    cerr << "Error waiting for left child process: " << strerror(errno) << "\n";
                    exit(1);
                }
                cout << "[" << getpid() << "] left child " << left_pid << " returned\n";

                if (waitpid(right_pid, &status, 0) == -1)
                {
                    cerr << "Error waiting for right child process: " << strerror(errno) << "\n";
                    exit(1);
                }
                cout << "[" << getpid() << "] right child " << right_pid << " returned\n";
            }
            else
            {
                cerr << "Failed to fork right child.\n";
                return -1;
            }
        }
        else
        {
            cerr << "Failed to fork left child.\n";
            return -1;
        }
    }
    else
    {
        pid_t searcher_pid = fork();
        if (searcher_pid == 0)
        {
            cout << "[" << getppid() << "] forked searcher child " << getpid() << "\n";
            execl("./part3_searcher.out", "part3_searcher.out", file_to_search_in, pattern_to_search_for, 
            to_string(search_start_position).c_str(), to_string(search_end_position).c_str(), NULL);
            cerr << "Failed to execute searcher.\n";
            return -1;
        }
        else if (searcher_pid > 0)
        {
            child_pids.push_back(searcher_pid);
            int status;

            if (waitpid(searcher_pid, &status, 0) == -1)
            {
                cerr << "Error waiting for searcher child process: " << strerror(errno) << "\n";
                exit(1);
            }
            cout << "[" << getpid() << "] searcher child " << searcher_pid << " returned\n";
        }
        else
        {
            cerr << "Failed to fork searcher child.\n";
            return -1;
        }
    }

    return 0;
}
