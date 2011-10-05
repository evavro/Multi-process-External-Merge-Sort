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
#define SEMA_KEY 1582

using namespace std;

void pipeFunction(void *);

//void setupProcesses();
void masterMergeController();
void forkParentMergers(const int &);
void forkChildSorter(const int &);
void parentMergeProcess();
void leafChildMergeProcess(); // wait on the child processes
void spawnChildSortThread();
void updateStatus(const char*);

const char * popFile();
vector<int> readFile(const char*);
void sigHandler(int);

/*struct parsed_int_file {
	char * filename;
	vector<int> ints;
};

vector<parsed_int_file> ints;*/

vector<int> mergedParentInts;
vector<int> mergedChildrenInts;
vector<const char*> files;

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
			
			//fileCount = files.size();
		}
	} else if(argc > 1) {
		// Log the files we need
		for(int i = 1; i < argc; i++)
			files.push_back(argv[i]);
	}
	
	// Main sorting and process controller
	masterMergeController();
}

vector<int> readFile(const char* filename) {
	ifstream fstream;
	char output[100];
	int currentInt;
	vector<int> numbers; // change this to a map so you can't get duplicate values. push this vector to 
	
	fstream.open(filename);
	
	if (fstream.is_open()) {
		cout << "Loading file " << filename << endl;
		
		// This keeps adding the last number in a file twice for some stupid reason
		while (!fstream.eof()) {
			fstream >> output;
			
			currentInt = atoi(output);
			
			numbers.push_back(currentInt);
		}
		
		fstream.close();
	} else {
		cout << "Could not load file " << filename << "." << endl;
	}
	
	return numbers;
}

void masterMergeController() {
		/* the child processes should each input and sort a file
	   the child processes should then send their sorted values via a pipe to their parent process
	   the parent processes should simply merge (not re-sort) the incoming values
	   the parent processes should send their sorted values via a pipe to the Master process, which merges them and outputs the final sorted file */
	int fileCount = files.size();
	int parentProcessCount = (fileCount % 2 == 0 ? fileCount : fileCount + 1) / 2; 
	int childProcessCount = fileCount;	// fileCount > 1 ? 2 : 1
	
	// Create pipes
	int pipefds[parentProcessCount][2];
	char buf[PIPE_BUF];
	
	cout << "Forking " << parentProcessCount << " parent merger processe(s)" << endl;
    
    for(int i = 0; i < parentProcessCount; i++) {
		//fork();
		
		/*int parent_id = fork();
		
		// PIPE MAY NEED TO BE MOVED TO forkParentMerges(const int *)
		pipe(pipefds[i]);
		
		if(parent_id < 0) {
			perror("Unable to fork parent merge process\n");
		}
				
		// Main controller
		else if(parent_id)
		{
			read(pipefds[i][0], buf, PIPE_BUF);
			
			cout << "Received pipe buffer in master merge controller: " << buf << endl;
			
			close(pipefds[i][0]);
			close(pipefds[i][1]);
		} 
		
		else if(!parent_id)
		{
			
		
			forkParentMergers(pipefds[i][1]);
			
			//read(pipefds[i][0], buf, PIPE_BUF);
			//write(pipefds[i][1], buf, PIPE_BUF);
		
			//cout << "Received pipe buffer in master merge controller: " << buf << endl;
		
			//close(pipefds[i][0]);
			//close(pipefds[i][1]);
			
			exit(0);
		} */
		
		
		// FIXME: Right now doing the pipes / forks makes the parents block each other. Need to do it more like the child threads
		pipe(pipefds[i]);
		
		forkParentMergers(pipefds[i][1]);
		
		read(pipefds[i][0], buf, PIPE_BUF);
		
		cout << "Received pipe buffer in master merge controller: " << buf << endl;
		
		close(pipefds[i][0]);
		close(pipefds[i][1]);
	}
	
	cout << "Got messages from all pipes, everything should be completely sorted at this point\n" << endl;
	
	// merge results
}

//void setupProcesses() {
// can maybe just pass a reference to the master pipe like in forkChildSort
void forkParentMergers(const int & MASTER_PIPE_WRITE) {
	int parent_id, status;
	int child_count = (files.size() > 1 ? 2 : 1);
	
	pid_t c_pids[child_count];
	
	int pipefds[child_count][2];
	char buf[child_count][PIPE_BUF];
	
	const int& PIPE_READ = pipefds[0][0];
	const int& PIPE_WRITE = pipefds[0][1];
		
	//status = pipe(pipefds[0]);
	
	for(int i = 0; i < child_count; i++)
		pipe(pipefds[i]);
	
	parent_id = fork();
		
	if(parent_id < 0) {
		perror("Unable to fork parent merge process\n");
	}
			
	// Main process
	else if(parent_id)
	{
		cout << "Forked parent merger process ID #" << parent_id << endl;

		for(int i = 0; i < child_count; i++)
			read(pipefds[i][0], buf[i], PIPE_BUF);

		// waitpid(-1, &status, 0); //block
		
		// This actually comes up with the same id as the master.. need to do some forks BEFORE this
		cout << "Parent #" << getpid() << " received sorted results from all child processes" << endl;
		
		// merge sort
		write(MASTER_PIPE_WRITE, buf[0], PIPE_BUF);
	}
			
	// Parent merger process
	else if(!parent_id)
	{
		close(PIPE_READ);

		// *** http://stackoverflow.com/questions/876605/multiple-child-process

		// Fork children sorter processes
		for (int i = 0; i < child_count; ++i) {
			if ((c_pids[i] = fork()) < 0) {
				perror("Could not fork child process");
			} else if (c_pids[i] == 0) {
				forkChildSorter(pipefds[i][1]);
				
				exit(0);
			}
		}
		
		//forkChildSorter(PIPE_WRITE);

		close(PIPE_WRITE);

		exit(0);
	}
}

void forkChildSorter(const int & PIPE_WRITE) {
	int child_id;

	// If we can split into 2 child sorters (>= 2 more files to sort), fork sibling
	//if(files.size() - 1 > 0)
	//	child_id = fork();
	
	// FIXME: there is always 1 child process that is the same as it's parent.
	cout << "Created sibling child sort process ID #" << getpid() << endl;
	
	vector<int> sorted = readFile(popFile());
	string sortedString = "";
	int tempInt;
	
	// Convert vector to string (temporary)
	for(vector<int>::size_type i = 0; i != sorted.size(); i++) {
		stringstream ss;
		ss << sorted[i];
	
		//sortedString += itoa(sorted[i]);
		sortedString += ss.str() + ",";
	}
	
	if(PIPE_WRITE) {
		cout << "Writing output up pipe stream to parent from Child #" << getpid() << " - " << sortedString.c_str() << endl;
		
		// kill(getppid(), random_interrupt);
		
		write(PIPE_WRITE, sortedString.c_str(), PIPE_BUF);
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
	const char * filename = files[files.size() - 1];
	
	// remove the file we just pulled so other threads don't process the same files
	// NOT SYNCHRONIZING!
	files.pop_back();
	
	return filename;
}

void sigHandler(int sig) {
	exit(0);
}

void updateStatus(const char* status) {
	cout << "Action - " << status << endl;
}
