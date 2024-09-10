#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <bits/stdc++.h>

using namespace std;

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

}