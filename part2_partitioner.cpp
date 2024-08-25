#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <cerrno>

using namespace std;

int main(int argc, char **argv)
{
    if (argc != 6)
    {
        cerr << "Usage: ./part2_partitioner.out <path-to-file> <pattern> <search-start-position> <search-end-position> <max-chunk-size>\n";
        return -1;
    }

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
            execl("./part2_partitioner.out", "part2_partitioner.out", file_to_search_in, pattern_to_search_for, argv[3], to_string(mid).c_str(),
                  to_string(max_chunk_size).c_str(), NULL);
            cerr << "Failed to execute left partitioner.\n";
            return -1;
        }
        else if (left_pid > 0)
        {
            pid_t right_pid = fork();
            if (right_pid == 0)
            {
                
                cout << "[" << getppid() << "] forked right child " << getpid() << "\n";
                execl("./part2_partitioner.out", "part2_partitioner.out", file_to_search_in, pattern_to_search_for, to_string(mid + 1).c_str(),
                      argv[4], to_string(max_chunk_size).c_str(), NULL);
                cerr << "Failed to execute right partitioner.\n";
                return -1;
            }
            else if (right_pid > 0)
            {

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
            execl("./part2_searcher.out", "part2_searcher.out", file_to_search_in, pattern_to_search_for, to_string(search_start_position).c_str(), to_string(search_end_position).c_str(), NULL);
            cerr << "Failed to execute searcher.\n";
            return -1;
        }
        else if (searcher_pid > 0)
        {
            cout << "[" << getpid() << "] forked searcher child " << searcher_pid << "\n";
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
