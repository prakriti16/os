#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>

using namespace std;

int main(int argc, char **argv)
{
    if (argc != 5)
    {
        cerr << "usage: ./partitioner.out <path-to-file> <pattern> <search-start-position> <search-end-position>\nprovided arguments:\n";
        for (int i = 0; i < argc; i++)
            cerr << argv[i] << "\n";
        return -1;
    }

    char *file_to_search_in = argv[1];
    char *pattern_to_search_for = argv[2];
    int search_start_position = atoi(argv[3]);
    int search_end_position = atoi(argv[4]);

    int pattern_len = strlen(pattern_to_search_for);
    ifstream file(file_to_search_in);

    if (!file) {
        cerr << "Error opening file" << endl;
        return -1;
    }

    

    if (search_end_position < search_start_position) {
        cerr << "Invalid search range" << endl;
        return -1;
    }

    file.seekg(search_start_position);

    string window;
    char c;
    int i = search_start_position;

    while (i <= search_end_position && i - search_start_position < pattern_len && file.get(c)) {
        window += c;
        i++;
    }

    while (i <= search_end_position) {
        if (window == pattern_to_search_for) {
            cout << "[" << getpid() << "] found at " << (i - pattern_len) << endl;
            return 0; 
        }

        window.erase(0, 1);
        if (file.get(c)) {
            window += c;
            i++;
        } else {
            break; 
        }
    }

    if (window == pattern_to_search_for) {
        cout << "[" << getpid() << "] found at " << (i - pattern_len) << endl;
    } else {
        cout << "[" << getpid() << "] didn't find" << endl;
    }

    return 0;
}

