// Take in multiple unsorted files
// Produce a single sorted file
// ^^ Essentially an ordered aggregation of files
// Sort each file individually, then do a master merge
// pipe sorted lists from child processes to their parent processes so that they can be merged
// create a logging system

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <utility>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <errno.h>

#define READ 0 
#define WRITE 1 
#define MAX 1024

void updateStatus(const char*);
void forkParentMergers();
void createParentMergeProcesses();
void createLeafChildSortProcesses();
void parentMergeProcess();
void leafChildMergeProcess(); // wait on the child processes

void readFile(const char*);
void sigHandler(int);

using namespace std;

map<char*, vector<int> > fileDataMap;
int fileCount = 0;

int main(int argc, char *argv[]) {
	// Handle interrupts
	signal (SIGINT, sigHandler);
	
	string filename;
	
	cout << "Welcome to my multi-process, multi-threaded and parallel external merge sort\n" << endl;
	
	// This way we can handle argument values AND individually typed in files
	if(argc == 1) {
		cout << endl << "Type in files one at a time" << endl;
		
		while(1) {
			cout << "Input filename (.int): ";
			
			getline (cin, filename);
			
			readFile(filename.c_str());
		}
	} else if(argc > 1) {
		for(int i = 1; i < argc; i++)
			readFile(argv[i]);
	}
}

void readFile(const char* filename) {
	ifstream fstream;
	char output[100];
	int currentInt;
	vector<int> numbers; // change this to a map so you can't get duplicate values. push this vector to 
	
	fstream.open(filename);
	
	if (fstream.is_open()) {
		cout << "Loading file " << filename << "\n\n";
		
		// This keeps adding the last number in a file twice for some stupid reason
		// look up sstream for the easiest data conversion ever
		while (!fstream.eof()) {
			fstream >> output;
			
			currentInt = atoi(output);
			
			numbers.push_back(currentInt);
		}
		
		fileCount++;
		
		fstream.close();
		
		forkParentMergers();
	} else {
		cout << "Could not load file " << filename << "." << endl;
	}
}

void forkParentMergers() {
	// spawn 2 internal parent processes
	// the parent processes should each spawn 2 "leaf-node" child processes
	// the child processes should each input and sort a file
	// the child processes should then send their sorted values via a pipe to their parent process
	// the parent processes should simply merge (not re-sort) the incoming values
	// the parent processes should send their sorted values via a pipe to the Master process, which merges them and outputs the final sorted file
	
	int parentProcessCount = (fileCount == 1 ? 2 : fileCount) / 2;
	int childProcessCount = fileCount;
	
	int parent_id, status;
	
	cout << "Forking " << parentProcessCount << " parent merger processes" << endl;
    
    for(int i = 0; i < parentProcessCount; i++) 
    {
		// Create pipe to parent merger
		int pipefd[2];
		const int BSIZE = 256;
		char buf[BSIZE];
		
		status = pipe(pipefd); 
	
		// Create a parent process fork that handles 2 file merges
		parent_id = fork();
		
		// Fork parent fails
		if(parent_id < 0) {
			perror("Unable to fork parent merge process\n");
		}
		
		// Main controller
		else if(parent_id)
		{
			printf("Spawned parent: ID# %i\n", parent_id);

			// NOTE: May not be a bad idea to have the parent sorters as a separate executable (execv)
			//parent_id = wait(&status);
			
			read(pipefd[0], buf, BSIZE);
			
			// TODO: Create a "ParsedSortFile" object so that we can pass the object through a stream easily and print it in cout using operands
			//for(int i = 0; i < sizeof(indata); i++)
				//cout << indata[i] << endl;
			
			cout << "After wait: " << buf << endl;
		}
		
		// Parent merger process
		else if(!parent_id)
		{
			// close unused end of pipe because this process only needs to write
			close(pipefd[0]);
			
			//int test[] = {10, 20, 30, 40};
			
			//createLeafChildSortProcesses
			write(pipefd[1], "test message", BSIZE);  
			
			close(pipefd[1]);
			
			exit(0);
		}
	}
}

void createParentMergeProcesses() {
	cout << "Parent merge process" << endl;
}

void createLeafChildSortProcesses() {
	cout << "Child sort process created" << endl;
}

void sigHandler(int sig) {
	cout << endl;
	
	exit(0);
}

void updateStatus(const char* status) {
	cout << "Action - " << status << endl;
}
