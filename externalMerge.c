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
void forkParentMergers();
void forkChildSorter();
void spawnChildSortThread();
void parentMergeProcess();
void leafChildMergeProcess(); // wait on the child processes
void updateStatus(const char*);

void readFile(const char*);
void sigHandler(int);

using namespace std;

/*struct parsed_int_file {
	char * filename;
	vector<int> ints;
};

vector<parsed_int_file> ints;*/

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
			cout << "Input filename (.int): ";
			
			getline (cin, filename);
			
			files.push_back(filename.c_str());
			
			//Move this to childSorter, the file should only be parsed then
			//readFile(filename.c_str());
		}
	} else if(argc > 1) {
		for(int i = 1; i < argc; i++)
			readFile(argv[i]);
	}
	
	//setupProcesses();
	forkParentMergers();
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
			
			numbers.push_back(currentInt);
		}
		
		fileCount++;
		
		fstream.close();
	} else {
		cout << "Could not load file " << filename << "." << endl;
	}
}

// parses a specific function and waits for the response to come back
// under construction
/*char * pipeFunction(void (* func)(int)) {
	int parent_id, status;
	
	int pipefd[2];
	const int& PIPE_READ = pipefd[0];
	const int& PIPE_WRITE = pipefd[1];
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
		read(PIPE_READ, buf, BSIZE);

		cout << "pipeFunction: Received message from pipe: " << buf << endl;
	}
		
	// Parent merger process
	else if(!parent_id)
	{
		close(PIPE_READ);
		
		(*func);

		write(PIPE_WRITE, "Sorted ints from child process", BSIZE);  

		close(PIPE_WRITE);

		exit(0);
	}
	
	return NULL;
}*/

//void setupProcesses() {
void forkParentMergers() {
	/* the child processes should each input and sort a file
	   the child processes should then send their sorted values via a pipe to their parent process
	   the parent processes should simply merge (not re-sort) the incoming values
	   the parent processes should send their sorted values via a pipe to the Master process, which merges them and outputs the final sorted file */
	
	int parentProcessCount = (fileCount % 2 == 0 ? fileCount : fileCount + 1) / 2; 
	int childProcessCount = fileCount;	
	
	int pipefd[parentProcessCount][2]; // This should maybe be moved to inside the loop
	char buf[PIPE_BUF];
	
	cout << "Forking " << parentProcessCount << " parent merger processe(s)" << endl;
    
    for(int i = 0; i < parentProcessCount; i++) {
		int parent_id, status;
		
		// This loop may be blocked by the piping stuff
		const int& PIPE_READ = pipefd[i][0];
		const int& PIPE_WRITE = pipefd[i][1];
		
		status = pipe(pipefd[i]); 
	
		parent_id = fork();
		
		if(parent_id < 0) {
			perror("Unable to fork parent merge process\n");
		}
			
		// Main controller
		else if(parent_id)
		{
			cout << "Spawned parent merger process: ID# " << parent_id << endl;
				
			read(PIPE_READ, buf, PIPE_BUF);

			cout << "Received message from pipe: " << buf << endl;
		}
			
		// Parent merger process
		else if(!parent_id)
		{
			close(PIPE_READ);

			write(PIPE_WRITE, "1,2,3,4,5,6,7,10", PIPE_BUF);  

			close(PIPE_WRITE);

			exit(0);
		}
	}
}

void forkChildSorter() {
	char * sortedResults;
}

/*vector<int> merge(const vector<int> list1, const vector<int> list2) {
	
}*/

void spawnChildSortThread() {
	// http://randu.org/tutorials/threads/
	/*phread_t thread[THREADS];
	char *sorted = "";
	int t_create;*/
}

// I need to use a global variable, probably should create a MergeSort class
void mergeSort(int[] nums) {
	
}

void merge(int left, int right) {
    
}

void sigHandler(int sig) {
	cout << endl;
	
	exit(0);
}

void updateStatus(const char* status) {
	cout << "Action - " << status << endl;
}
