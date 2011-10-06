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

using namespace std;

void forkParentMergers();
void forkChildSorters(const int &);
void leafChildSortProcess(const int &);

const char * popFile();
void readFile(const char *, vector<int> &);
bool doesFileExist(const char *);
void saveSortedFile(vector<int> &);

vector<int> & mergeSort(vector<int> &);
void merge(vector<int> &, vector<int> &, vector <int> &);
const char * sortedToChars(vector<int> &);
vector<int> charsToSorted(const char *);

void closePipe(int (*)) ;
void sigHandler(int);

vector<const char*> files;
vector<int> sorted;

const int PIPE_BUF = 256;
const int THREADS = 2;

int main(int argc, char *argv[]) {
	
	// Handle interrupts
	signal (SIGINT, sigHandler);
	
	string filename;
	
	cout << "Welcome to my multi-process, multi-threaded and parallel external merge sort\nBy: Erik Vavro\n" << endl;
	
	// This way we can handle argument values AND individually typed in files
	if(argc > 1) {
		
		// Log the files we need if they exist.
		for(int i = 1; i < argc; i++) {
			if(doesFileExist(argv[i])) {
				files.push_back(argv[i]);
			} else {
				perror(argv[i]);
				
				exit(0);
			}
		}
	} else {
		cerr << "You need to specifiy at least one plain-text file of integers for sorting" << endl;
		
		exit(0);
	}
	
	// Main sorting and process controller, starts creating process tree
	forkParentMergers();
}

// Read in a file and parse the integers within
void readFile(const char* filename, vector<int> & numbers) {
	ifstream fstream;
	char output[15]; // support up to a 15 digit integer
	int currentInt;
	
	fstream.open(filename);
	
	if (fstream.is_open()) {
		cout << "Loading and sorting file " << filename << " in child #" << getpid() << endl;
		
		while (!fstream.eof()) {
			fstream >> output;
			
			currentInt = atoi(output);
			
			// Add the number on each line to the vector
			numbers.push_back(currentInt);
		}
		
		// Need to pop the last number, it gets duplicated for some reason
		numbers.pop_back();
		fstream.close();
	} else {
		perror(filename);
	}
}

// Determines if a file exists at the given path
bool doesFileExist(const char * filename) {
	ifstream ifile(filename);
	
	return ifile;
}

// Saves a merged sorted list to a new file
void saveSortedFile(vector<int> & numbers) {
	const char * filename = "merged.txt";
	fstream file;

	file.open(filename, ios::out);

	if(file.is_open()) {
		// Add a number from the sorted list on a line
		for(unsigned int i = 0; i < numbers.size(); i++)
			file << numbers[i] << endl;
	} else {
		perror(filename);
	}
		
	file.close();
	
	cout << "Saved aggregated sort file as " << filename << endl;
}


const char * popFile() {
	const char * filename = files[files.size() - 1];
	
	// Remove the file we just pulled so other threads don't process the same files
	// NOT SYNCHRONIZING!
	files.pop_back();
	
	return filename;
}

// Forks off and controls enough parents to have at most 2 child processes
void forkParentMergers() {
	int file_count = files.size();
	int parent_process_count = (file_count % 2 == 0 ? file_count : file_count + 1) / 2;
	
	// Create pipes
	int pipefds[parent_process_count][2];
	char buf[parent_process_count][PIPE_BUF];
	
	pid_t p_pids[parent_process_count];
	
	cout << "Forking " << parent_process_count << " parent merger processe(s) from master process #" << getpid() << endl;
	
	// Establish pipes
	for(int i = 0; i < parent_process_count; i++)
		pipe(pipefds[i]);

	// Create parent merger processes
	for(int i = 0; i < parent_process_count; i++) {	
		if ((p_pids[i] = fork()) < 0) {
			perror("Could not fork parent process");
		} else if (p_pids[i] == 0) {
			cout << "Forked parent merger process #" << getpid() << endl;
			
			forkChildSorters(pipefds[i][1]);
					
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
	
	// Read in parent pipes and merge their results
	for(int i = 0; i < parent_process_count; i++) {
		read(pipefds[i][0], buf[i], sizeof(buf[i]));
		
		// Combine sorted results
		sorted_parents_s += buf[i];
		sorted_parents = charsToSorted(sorted_parents_s.c_str());
		
		// Merge sort results
		mergeSort(sorted_parents);
		
		// Close read pipe to this parent since we have its results
		close(pipefds[i][0]);
	}

	// Final point in the aggregated merge sort process. Could have this in the super-function call, but that would require tossing around more variables
	cout << "Master process #" << getpid() << " received sorted results from all " << parent_process_count << " parent processes " << endl;
	
	cout << "Everything should be completely merged and sorted at this point:\n" << sortedToChars(sorted_parents) << endl;
	
	saveSortedFile(sorted_parents);
}

// Forks off and controls at most 2 children sorting processes.
void forkChildSorters(const int & PARENT_PIPE_WRITE) {
	
	// If there are at least 2 more files to parse, then count 2. Otherwise, only 1 remains
	int child_count = files.size() > 1 ? 2 : 1;
	
	pid_t c_pids[child_count];
	
	// Create pipes
	int pipefds[child_count][2];
	char buf[child_count][PIPE_BUF];

	// Establish pipes
	for(int i = 0; i < child_count; i++)
		pipe(pipefds[i]);

	// Fork children sorter processes
	for (int i = 0; i < child_count; ++i) {
		if ((c_pids[i] = fork()) < 0) {
			perror("Could not fork child process");
		} else if (c_pids[i] == 0) {
			leafChildSortProcess(pipefds[i][1]);
				
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
	
	// Read in child pipes and merge their results
	for(int i = 0; i < child_count; i++) {
		read(pipefds[i][0], buf[i], sizeof(buf[i]));
		
		cout << "Parent #" << getpid() << " received a result from child process" << endl;
	
		// Merge results
		sorted_children_s += buf[i];
		
		// Close read pipe to this child since we have its results
		close(pipefds[i][0]);
	}

	cout << "Parent #" << getpid() << " merged results from all child processes\n" << sorted_children_s << endl;
	
	write(PARENT_PIPE_WRITE, sorted_children_s.c_str(), PIPE_BUF);
}

// Process that opens a file, parses it, sorts it, and sends the result up to a respective parent process.
void leafChildSortProcess(const int & PIPE_WRITE) {
	cout << "Created child sort process #" << getpid() << endl;

	readFile(popFile(), sorted);
	
	if(PIPE_WRITE) {
		if(sorted.size() > 0) {
			mergeSort(sorted);
			
			write(PIPE_WRITE, sortedToChars(sorted), PIPE_BUF);
		} else {
			write(PIPE_WRITE, NULL, PIPE_BUF);
		}
	} else {
		perror("Could not write to parent process from child");
	}
}

// Generic merge sort algorithm implementation
vector<int> & mergeSort(vector<int> & data) {
	if(data.size() <= 1)
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

// Generic merge sort algorithm implementation
void merge(vector<int> & vector_a, vector<int> & vector_b, vector<int> & original) {
	unsigned int index_a = 0;
	unsigned int index_b = 0;
	unsigned int index_c = 0;

	while(index_a < vector_a.size() && index_b < vector_b.size()) {
		if (vector_a[index_a] < vector_b[index_b]) {
			original[index_c] = vector_a[index_a];
			
			index_a++;
		} else {
			original[index_c] = vector_b[index_b];
			
			index_b++;
		}
		
        index_c++;
     }
     
     while (index_a < vector_a.size()) {
		original[index_c] = vector_a[index_a];
           
		index_a++;
		index_c++;
     }
     
     while (index_b < vector_b.size()) {
		original[index_c] = vector_b[index_b];
           
		index_b++;
		index_c++;
     }
}

// Convert sorted numbers into a string of characters
const char * sortedToChars(vector<int> & numbers) {
	string sortedString = "";
	
	for(vector<int>::size_type i = 0; i != numbers.size(); i++) {
		stringstream ss;
		ss << numbers[i];
	
		sortedString += ss.str() + " ";
	}
	
	return sortedString.c_str();
}

// Converts sorted chars seperated by a space into a vector
vector<int> charsToSorted(const char * numbers) {
	vector<int> numbers_vec;
	string s = string(numbers); 
    istringstream iss(s);
    
    int n;

    while (iss >> n)
        numbers_vec.push_back(n);
        
    return numbers_vec;
}

void sigHandler(int sig) {
	cout << "Exiting, good bye" << endl;
	
	exit(0);
}
