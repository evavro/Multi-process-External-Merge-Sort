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

void masterMergeController();
void forkSubProcessesByType(int, int, const int *);
void forkParentMergers(const int &);
void forkChildSorter(const int &);
void forkParentMergers(const int *);
void forkChildSorter(const int *);
//void parentMergeProcess();
//void leafChildMergeProcess(); // wait on the child processes
//void spawnChildSortThread();
void updateStatus(const char*);

const char * popFile();
void readFile(const char*, vector<int> &);
//vector<int> readFile(const char*);

vector<int> & mergeSort(vector<int> &);
void merge(vector<int> &, vector<int> &, vector <int> &);
const char * sortedToChars(vector<int> &);
vector<int> charsToSorted(const char *);

void mergeCopyNoDupes(vector<int> &, vector<int> &);

void sigHandler(int);

vector<const char*> files;
vector<int> sorted;

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
			
			getline(cin, filename);
			
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

void readFile(const char* filename, vector<int> & numbers) {
	ifstream fstream;
	char output[15]; // support up to a 15 digit integer
	int currentInt;
	
	fstream.open(filename);
	
	if (fstream.is_open()) {
		cout << "Loading file " << filename << endl;
		
		while (!fstream.eof()) {
			fstream >> output;
			
			currentInt = atoi(output);
			
			numbers.push_back(currentInt);
		}
		
		// Need to pop the last number, it gets duplicated for some reason
		numbers.pop_back();
		fstream.close();
	} else {
		cout << "Could not load file " << filename << "." << endl;
	}
}

// This is a grandfather process
void masterMergeController() {
	int file_count = files.size();
	int parent_process_count = (file_count % 2 == 0 ? file_count : file_count + 1) / 2;
	
	// Create pipes
	int pipefds[parent_process_count][2];
	char buf[parent_process_count][PIPE_BUF];
	
	pid_t p_pids[parent_process_count];
	
	cout << "Forking " << parent_process_count << " parent merger processe(s)" << endl;
	
	for(int i = 0; i < parent_process_count; i++)
		pipe(pipefds[i]);

	// Create parent merger processes
	for(int i = 0; i < parent_process_count; i++) {	
		if ((p_pids[i] = fork()) < 0) {
			perror("Could not fork parent process");
		} else if (p_pids[i] == 0) {
			forkParentMergers(pipefds[i][1]);
					
			exit(0);
		}
			
		close(pipefds[i][1]); // close write-end of pipe
	}
	
	int status, num_parents = parent_process_count;
	pid_t wait_pid;
	
	// Wait for parent processes to finish before we read
	while(num_parents > 0) {
		wait_pid = wait(&status);
		
		num_parents--;
	}
	
	string sorted_parents_s;
	vector<int> sorted_parents;
	
	for(int i = 0; i < parent_process_count; i++) {
		read(pipefds[i][0], buf[i], PIPE_BUF);
		
		// Combine sorted results
		sorted_parents_s += buf[i];
		sorted_parents = charsToSorted(sorted_parents_s.c_str());
		
		mergeSort(sorted_parents);
	}

	cout << "Master controller process #" << getpid() << " received sorted results from all " << parent_process_count << " parent processes " << endl;
	
	cout << "Got messages from all pipes, everything should be completely sorted at this point\nFinal result: " << sortedToChars(sorted_parents) << endl;
}

// can maybe just pass a reference to the master pipe like in forkChildSort
// turn this into a generic pipeWithChild() method that calls a specified function so that we don't have so much duplicate code
void forkParentMergers(const int & MASTER_PIPE_WRITE) {
	int parent_id;
	int child_count = (files.size() > 1 ? 2 : 1);
	
	pid_t c_pids[child_count];
	
	int pipefds[child_count][2];
	char buf[child_count][PIPE_BUF];

	for(int i = 0; i < child_count; i++)
		pipe(pipefds[i]);

	// Fork children sorter processes
	for (int i = 0; i < child_count; ++i) {
		if ((c_pids[i] = fork()) < 0) {
			perror("Could not fork child process");
		} else if (c_pids[i] == 0) {
			forkChildSorter(pipefds[i][1]);
				
			exit(0);
		}
			
		close(pipefds[i][1]); // Close write-end of pipe
	}
		
	int status, num_children = child_count;
	pid_t wait_pid;
	
	// Wait for child threads to finish before we read
	while(num_children > 0) {
		wait_pid = wait(&status);
		
		num_children--;
	}
	
	string sorted_children_s;
	vector<int> sorted_children;
	
	for(int i = 0; i < child_count; i++) {
		read(pipefds[i][0], buf[i], PIPE_BUF);
		
		cout << "Parent #" << getpid() << " received a result from child process:\n" << buf[i] << endl;
	
		// Combine sorted results
		sorted_children_s += buf[i];
		sorted_children = charsToSorted(sorted_children_s.c_str());
		
		mergeSort(sorted_children);
	}
	
	cout << "Parent #" << getpid() << " merged and sorted results from all child processes! " << sortedToChars(sorted_children) << endl;
		
	write(MASTER_PIPE_WRITE, sortedToChars(sorted_children), PIPE_BUF);
	
	// CREATE closePipe(const * int) method
}

void forkChildSorter(const int & PIPE_WRITE) {
	cout << "Created child sort process ID #" << getpid() << endl;

	readFile(popFile(), sorted);
	
	if(PIPE_WRITE) {
		if(sorted.size() > 0) {
			mergeSort(sorted);
			
			//cout << "Writing output up pipe stream to parent from Child #" << getpid() << "\n" << sortedToChars(sorted) << endl; 
	
			write(PIPE_WRITE, sortedToChars(sorted), PIPE_BUF);
		} else {
			write(PIPE_WRITE, NULL, PIPE_BUF);
		}
	} else {
		perror("Could not write to parent process from child");
	}
}

// under construction
void forkParentMergers(const int * pipe) {
	int child_count = (files.size() > 1 ? 2 : 1);
	
	int pipefds[2];
	int buf[PIPE_BUF];
	
	forkSubProcessesByType(child_count, SUB_CHILD_SORT, pipefds);
	
	//int buf[PIPE_BUF] = pipfds[1];
	read(pipefds[0], buf, PIPE_BUF);
	
	// does not like pipefds for some reason
	//write(pipe[1], pipefds[0], PIPE_BUF);
}

// under construction
void forkChildSorter(const int * pipe) {
	// FIXME: there is always 1 child process that is the same as it's parent.
	cout << "Created sibling child sort process ID #" << getpid() << endl;
	
	// Pop a file off the file queue and sort the contents
	//vector<int> sorted = readFile(popFile());
	vector<int> sorted;
	
	readFile(popFile(), sorted);
	string sortedString = "";
	
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
	int fork_id;
	
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
		cout << "Forked process ID #" << fork_id << endl;

		for(int i = 0; i < sp_count; i++)
			read(pipefds[i][0], buf[i], PIPE_BUF);
		
		cout << "Parent #" << getpid() << " received sorted results from all child sub processes: " << buf[0] << endl;
		
		// merge sort
		
		write(parent_pipe[1], buf[0], PIPE_BUF);
		
		close(parent_pipe[0]);
		close(parent_pipe[1]);
	}
	else if(!fork_id)
	{
		// close each open pipe[0]
		

		// *** http://stackoverflow.com/questions/876605/multiple-child-process

		// Fork sub processes based on their type
		for (int i = 0; i < sp_count; ++i) {
			// close read-end of pipe since it will not be used
			close(pipefds[i][0]);
			
			if ((s_pids[i] = fork()) < 0) {
				perror("Could not fork sub process");
			} else if (s_pids[i] == 0) {
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
			
			// close writte-end of pipe
			close(pipefds[i][1]);
		}

		exit(0);
	}
}

vector<int> & mergeSort(vector<int> & data) {
	// http://www.cplusplus.com/forum/beginner/27748/
	
	if(data.size() <= 1 )
		return data;

	vector<int> leftHalf(data.size() / 2);
	vector<int> rightHalf(data.size() - data.size() / 2);
	
	for(unsigned int i = 0; i < data.size() / 2; i++)
		leftHalf[i] = data[i];
		
	for(unsigned int i = data.size() / 2; i < data.size(); i++)
		rightHalf[i - data.size() / 2] = data[i];
		
	leftHalf = mergeSort(leftHalf);
	rightHalf = mergeSort(rightHalf);
	
	merge(leftHalf, rightHalf, data);
	
	return data;
}

void merge(vector<int> & arrayA, vector<int> & arrayB, vector<int> & original) {
	unsigned int indexA = 0;
	unsigned int indexB = 0;
	unsigned int indexC = 0;

	while(indexA < arrayA.size() && indexB < arrayB.size()) {
		if (arrayA[indexA] < arrayB[indexB]) {
			original[indexC] = arrayA[indexA];
			
			indexA++;
		} else {
			original[indexC] = arrayB[indexB];
			
			indexB++;
		}
		
        indexC++;
     }
     
     while (indexA < arrayA.size()) {
           original[indexC] = arrayA[indexA];
           indexA++;
           indexC++;
     }
     
     while (indexB < arrayB.size())
     {
           original[indexC] = arrayB[indexB];
           indexB++;
           indexC++;
     }
}

//void mergeSortedCharsNoDupes(vector<int> & a, vector<int> & b) {
void mergeSortedCharsNoDupes(string a, string b) {
	//a.insert( a.end(), b.begin(), b.end() );
	
	//if(
}

const char * popFile() {
	const char * filename = files[files.size() - 1];
	
	// remove the file we just pulled so other threads don't process the same files
	// NOT SYNCHRONIZING!
	files.pop_back();
	
	return filename;
}

const char * sortedToChars(vector<int> & numbers) {
	string sortedString = "";
	
	// Convert vector to string (temporary)
	for(vector<int>::size_type i = 0; i != numbers.size(); i++) {
		stringstream ss;
		ss << numbers[i];
	
		//sortedString += itoa(sorted[i]);
		sortedString += ss.str() + " ";
	}
	
	return sortedString.c_str();
}

// could use global variable instead throwing around vectors
vector<int> charsToSorted(const char * numbers) {
	vector<int> numbers_vec;
	string s = string(numbers); 
    istringstream iss(s);
    
    int n;

    while (iss >> n)
        numbers_vec.push_back(n);
        
    return numbers_vec;
}

// void saveSortedFile()

void sigHandler(int sig) {
	cout << "Exiting" << endl;
	
	exit(0);
}

void updateStatus(const char* status) {
	cout << "Action - " << status << endl;
}
