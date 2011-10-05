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

using namespace std;

void pipeFunction(void *);

//void setupProcesses();
void masterMergeController();
//void forkSubProcessesByType(int, int, int);
void forkSubProcessesByType(int, int, const int *);
void forkParentMergers(const int &);
void forkChildSorter(const int &);
void forkParentMergers(const int *);
void forkChildSorter(const int *);
void parentMergeProcess();
void leafChildMergeProcess(); // wait on the child processes
void spawnChildSortThread();
void updateStatus(const char*);

void merge(int, int, int);
void merge_sort(int, int);

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

int merge_cache[50];

const int SUB_PARENT_MERGE = 0;
const int SUB_CHILD_SORT = 1;
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
	   
	int master_id;
	int file_count = files.size();
	int parent_process_count = (file_count % 2 == 0 ? file_count : file_count + 1) / 2;
	
	// Create pipes
	int pipefds[parent_process_count][2];
	char buf[parent_process_count][PIPE_BUF];
	
	pid_t p_pids[parent_process_count];
	
	cout << "Forking " << parent_process_count << " parent merger processe(s)" << endl;
	
	for(int i = 0; i < parent_process_count; i++)
		pipe(pipefds[i]);
		
	master_id = fork();
		
	if(master_id < 0) {
		perror("Unable to fork parent merge process\n");
	}
	else if(master_id)
	{
		cout << "Forked master merger process ID #" << master_id << endl;

		// Read in everything from parent pipe streams
		for(int i = 0; i < parent_process_count; i++)
			read(pipefds[i][0], buf[i], PIPE_BUF);

		cout << "Master #" << getpid() << " received sorted results from all parent processes" << endl;
	
		cout << "Got messages from all pipes, everything should be completely sorted at this point\n" << endl;
		
		// merge sort EVERYTHING
	}
	else if(!master_id)
	{
		// Create parent merger processes
		for(int i = 0; i < parent_process_count; i++) {
			// close read-end of pipe since it will not be used
			close(pipefds[i][0]);
			
			if ((p_pids[i] = fork()) < 0) {
				perror("Could not fork parent process");
			} else if (p_pids[i] == 0) {
				forkParentMergers(pipefds[i][1]);
					
				exit(0);
			}
			
			// close write-end of pipe
			close(pipefds[i][1]);
		}
	}
	
	// merge results
	
	// Below is under construction
	
	/*int file_count = files.size();
	int parent_process_count = (file_count % 2 == 0 ? file_count : file_count + 1) / 2;
	
	// Create pipes
	int pipefds[parent_process_count][2];
	char buf[parent_process_count][PIPE_BUF];
	
	pid_t p_pids[parent_process_count];
	
	cout << "Forking " << parent_process_count << " parent merger processe(s)" << endl;
	
	forkSubProcessesByType(parent_process_count, SUB_PARENT_MERGE, pipefds[0]);*/
}

// can maybe just pass a reference to the master pipe like in forkChildSort
// turn this into a generic pipeWithChild() method that calls a specified function so that we don't have so much duplicate code
void forkParentMergers(const int & MASTER_PIPE_WRITE) {
	int parent_id;
	int child_count = (files.size() > 1 ? 2 : 1);
	
	pid_t c_pids[child_count];
	
	int pipefds[child_count][2];
	char buf[child_count][PIPE_BUF];
	
	const int& PIPE_READ = pipefds[0][0];
	const int& PIPE_WRITE = pipefds[0][1];

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
		
		// This actually comes up with the same id as the master.. need to do some forks BEFORE this
		cout << "Parent #" << getpid() << " received sorted results from all child processes" << endl;
		
		// merge sort
		write(MASTER_PIPE_WRITE, buf[0], PIPE_BUF);
	}
	else if(!parent_id)
	{
		// *** http://stackoverflow.com/questions/876605/multiple-child-process

		// Fork children sorter processes
		for (int i = 0; i < child_count; ++i) {
			// close read-end of pipe since it will not be used
			close(pipefds[i][0]);
			
			if ((c_pids[i] = fork()) < 0) {
				perror("Could not fork child process");
			} else if (c_pids[i] == 0) {
				forkChildSorter(pipefds[i][1]);
				
				exit(0);
			}
			
			// close write-end of pipe
			close(pipefds[i][1]);
		}
		
		exit(0);
	}
}

void forkChildSorter(const int & PIPE_WRITE) {
	// FIXME: there is always 1 child process that is the same as it's parent.
	cout << "Created sibling child sort process ID #" << getpid() << endl;
	
	// Pop a file off the file queue and sort the contents
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
		
		write(PIPE_WRITE, sortedString.c_str(), PIPE_BUF);
	}
}

// under construction
void forkParentMergers(const int * pipe) {
	int child_count = (files.size() > 1 ? 2 : 1);
	
	int pipefds[2];
	
	forkSubProcessesByType(child_count, SUB_CHILD_SORT, pipefds);
	
	// does not like pipefds for some reason
	//write(pipe[1], pipefds[1], PIPE_BUF);
}

// under construction
void forkChildSorter(const int * pipe) {
	// FIXME: there is always 1 child process that is the same as it's parent.
	cout << "Created sibling child sort process ID #" << getpid() << endl;
	
	// Pop a file off the file queue and sort the contents
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
	
	if(pipe[1]) {
		cout << "Writing output up pipe stream to parent from Child #" << getpid() << " - " << sortedString.c_str() << endl;
		
		write(pipe[1], sortedString.c_str(), PIPE_BUF);
	}
}

// generic subprocess creator, under construction
// must make the forkProcesses take in int * pipefds instead of a PIPE_WRITE
void forkSubProcessesByType(int sp_count, int type, const int * parent_pipe) {
	/*int fork_id;
	
	pid_t s_pids[sp_count];
	
	int pipefds[sp_count][2];
	char buf[sp_count][PIPE_BUF];
	
	const int& PIPE_READ = pipefds[0][0];
	const int& PIPE_WRITE = pipefds[0][1];

	for(int i = 0; i < sp_count; i++)
		pipe(pipefds[i]);
	
	fork_id = fork();
		
	if(fork_id < 0) {
		perror("Unable to fork sub process\n");
	}
			
	// Main process
	else if(fork_id)
	{
		cout << "Forked parent merger process ID #" << fork_id << endl;

		for(int i = 0; i < sp_count; i++)
			read(pipefds[i][0], buf[i], PIPE_BUF);
		
		cout << "Parent #" << getpid() << " received sorted results from all child SUB processes" << endl;
		
		// merge sort
		write(parent_pipe[1], buf[0], PIPE_BUF);
	}
	else if(!fork_id)
	{
		// close each open pipe[0]
		close(PIPE_READ);

		// *** http://stackoverflow.com/questions/876605/multiple-child-process

		// Fork sub processes based on their type
		for (int i = 0; i < sp_count; ++i) {
			if ((s_pids[i] = fork()) < 0) {
				perror("Could not fork sub process");
			} else if (s_pids[i] == 0) {
				//forkChildSorter(pipefds[i][1]);
				
				cout << "Forkin some subs" << endl;
				
				switch(type) {
					case SUB_PARENT_MERGE:
						forkParentMergers(pipefds[i]);
					break;
					
					case SUB_CHILD_SORT:
						forkChildSorter(pipefds[i]);
					break;
				}
				
				exit(0);
			}
		}

		// close each open pipe[1]
		close(PIPE_WRITE);

		exit(0);
	} */
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
/*void mergeSort(int * nums[]) {
	
}

void merge(int left, int right) {
    
}*/

void merge_sort(int low, int high) {
	int mid;
	
	if(low < high) {
		mid = (low+high) / 2;
		merge_sort(low, mid);
		merge_sort(mid + 1, high);
		merge(low, mid, high);
	}
}

void merge(int low, int mid, int high) {
	int h, i, j, merge_cache_b[50], k;
	h = low;
	i = low;
	j = mid+1;

	while((h <= mid) && (j <= high)) {
		if(merge_cache[h] <= merge_cache[j]) {
			merge_cache_b[i] = merge_cache[h];
			
			h++;
		} else {
			merge_cache_b[i] = merge_cache[j];
			
			j++;
		}
		
		i++;
	}
	
	if(h > mid) {
		for(k = j; k <= high; k++) {
			merge_cache_b[i] = merge_cache[k];
			
			i++;
		}
	} else {
		for(k = h; k <= mid; k++) {
			merge_cache_b[i] = merge_cache[k];
			
			i++;
		}
	}
	
	for(k = low; k <= high; k++)
		merge_cache[k] = merge_cache_b[k];
}

const char * popFile() {
	const char * filename = files[files.size() - 1];
	
	// remove the file we just pulled so other threads don't process the same files
	// NOT SYNCHRONIZING!
	files.pop_back();
	
	return filename;
}

void sigHandler(int sig) {
	cout << "Exiting" << endl;
	
	exit(0);
}

void updateStatus(const char* status) {
	cout << "Action - " << status << endl;
}
