// Take in multiple unsorted files
// Product a single sorted file
// ^^ Essentially an ordered aggregation of files
// Sort each file individually, then do a master merge
// pipe sorted lists from child processes to their parent processes so that they can be merged
// create a logging system

// Functionality
// Count the number of files
// Create a child process that manages the sort of a file. (Make this multithreaded for extra credit)

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <errno.h>

//void updateStatus(const char* message);

void updateStatus(const char*);
void createParentMergeProcess();
void createChildMergeProcess(); // wait on the child processes

void readFile(const char*);
void sigHandler(int);

using namespace std;

int main() {
	// Handle interrupts
	signal (SIGINT, sigHandler);
	
	string filename;
	
	while(1) {
		cout << "Input filename (.int): ";
		getline (cin, filename);
		//cin >> file;
		
		updateStatus(filename.c_str());
		
		readFile(filename.c_str());
	}
}

void readFile(const char* filename) {
	ifstream fstream;
	char output[100];
	int currentInt;
	vector<int> numbers;
	
	fstream.open(filename);
	
	if (fstream.is_open()) {
		while (!fstream.eof()) {
			fstream >> output;
			
			//currentInt = atoi(output);
			
			// look up sstream for the easiest data conversion ever
			
			//numbers.push_back(currentInt);
		}
	}
	
	fstream.close();
}

void sigHandler(int sig) {
	exit(0);
}

void updateStatus(const char* status) {
	cout << "Action - " << status << endl;
}
