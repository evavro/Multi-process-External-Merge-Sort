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
#include <pthread.h>

#define READ 0 
#define WRITE 1 
#define MAX 1024

void pipeFunction(void *);

//void setupProcesses();
void masterMergeController();
//void forkParentMergers(int *);
void forkParentMergers(const int &);
void forkChildSorter(const int &);
void parentMergeProcess();
void leafChildMergeProcess(); // wait on the child processes
void spawnChildSortThread();
void updateStatus(const char*);

const char * popFile();
void readFile(const char*);
void sigHandler(int);

using namespace std;

/*struct parsed_int_file {
	char * filename;
	vector<int> ints;
};

vector<parsed_int_file> ints;*/

vector<int> mergedParentInts;
vector<int> mergedChildrenInts;
vector<const char*> files;

//map<char*, vector<int> > fileDataMap;

int fileCount = 0;

const int PIPE_BUF = 256;
const int THREADS = 2;

int main(int argc, char *argv[]) {
	// Handle interrupts
	signal (SIGINT, sigHandler);
	
	string filename;
	
	cout << "Welcome to my multi-process, multi-threaded and parallel external merge sort\n" << endl;
	
	// This way we can handle argument values AND individually typed in files
	if(argc == 1) {
		cout << endl << "Type in files one at a time" << endl;
		
		while(1) {
			cout << "Enter filename (.int): ";
			
			getline (cin, filename);
			
			files.push_back(filename.c_str());
			
			//Move this to childSorter, the file should only be parsed then
			//readFile(filename.c_str());
		}
	} else if(argc > 1) {
		//for(int i = 1; i < argc; i++)
		//	readFile(argv[i]);
		
		// WARNING THIS WON'T CHECK IF THE FILE EXISTS
		fileCount = argc - 1;
	}
	
	// Main sorting and process controller
	masterMergeController();
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
		while (!fstream.eof()) {
			fstream >> output;
			
			currentInt = atoi(output);
			
			cout << filename << " - Pushed " << currentInt << endl;
			
			numbers.push_back(currentInt);
		}
		
		fileCount++;
		
		fstream.close();
	} else {
		cout << "Could not load file " << filename << "." << endl;
	}
}

void masterMergeController() {
		/* the child processes should each input and sort a file
	   the child processes should then send their sorted values via a pipe to their parent process
	   the parent processes should simply merge (not re-sort) the incoming values
	   the parent processes should send their sorted values via a pipe to the Master process, which merges them and outputs the final sorted file */
	
	int parentProcessCount = (fileCount % 2 == 0 ? fileCount : fileCount + 1) / 2; 
	int childProcessCount = fileCount;	
	
	// Create pipes
	int pipefds[parentProcessCount][2]; // This should maybe be moved to inside the loop
	char buf[PIPE_BUF];
	
	cout << "Forking " << parentProcessCount << " parent merger processe(s)" << endl;
    
    for(int i = 0; i < parentProcessCount; i++) {
		pipe(pipefds[i]);
		
		//int parent_id 
		
		// ACTUALLY NEED TO FORK OFF THIS PROCESS!
		//fork();
		forkParentMergers(pipefds[i][1]);
		
		read(pipefds[i][0], buf, PIPE_BUF);
		
		cout << "Received pipe buffer in master merge controller: " << buf << endl;
	}
	
	cout << "Got messages from all pipes" << endl;
	
	// merge results
}

//void setupProcesses() {
// can maybe just pass a reference to the master pipe like in forkChildSort
void forkParentMergers(const int & MASTER_PIPE_WRITE) {
	int parent_id, status;
	
	int pipefds[2];
	char buf[PIPE_BUF];
		
	//const int& MASTER_PIPE_READ = pipe_master[0];
	//const int& MASTER_PIPE_WRITE = pipe_master[1];
	const int& PIPE_READ = pipefds[0];
	const int& PIPE_WRITE = pipefds[1];
		
	status = pipe(pipefds);
	//master_status = pipe(pipe_master); 
	
	parent_id = fork();
		
	if(parent_id < 0) {
		perror("Unable to fork parent merge process\n");
	}
			
	// Main controller
	else if(parent_id)
	{
		cout << "Spawned parent merger process: ID# " << parent_id << endl;
				
		read(PIPE_READ, buf, PIPE_BUF);
		// WRITE BACK TO MASTER
		//read(PIPE_READ, buf, PIPE_BUF);
		write(MASTER_PIPE_WRITE, "WHAT UP, HELL YEA", PIPE_BUF);

		cout << "Received message from pipe: " << buf << endl;
		
		return;
	}
			
	// Parent merger process
	else if(!parent_id)
	{
		close(PIPE_READ);
			
		// Fork 2 child children for each parent merger process
		// Check how many children actually need to be created!!!!!!!!
		forkChildSorter(PIPE_WRITE);
		//forkChildSorter(PIPE_WRITE);
		
		// sort everything

		close(PIPE_WRITE);

		exit(0);
	}
}

void forkChildSorter(const int & PIPE_WRITE) {
	int child_id;
	char * sortedResults;
	
	//readFile(popFile());
	
	if(PIPE_WRITE) {
		cout << "forkChildSorter: " << PIPE_WRITE << endl;
		
		write(PIPE_WRITE, "1,2,3,4,5,6,7,10", PIPE_BUF);
	}
}

/*vector<int> merge(const vector<int> list1, const vector<int> list2) {
	
}*/

void spawnChildSortThread() {
	// http://randu.org/tutorials/threads/
	/*phread_t thread[THREADS];
	char *sorted = "";
	int t_create;*/
}

void parentMergeProcess() {
	
}

void childSortProcess() {
	
}

// I need to use a global variable, probably should create a MergeSort class
void mergeSort(int * nums[]) {
	
}

void merge(int left, int right) {
    
}

const char * popFile() {
	const char * filename = files[files.size() - 1]; // may want to check if files is null

	// remove the file we just pulled so other threads don't process the same files
	files.pop_back();
	
	return filename;
}

void sigHandler(int sig) {
	cout << endl;
	
	exit(0);
}

void updateStatus(const char* status) {
	cout << "Action - " << status << endl;
}
