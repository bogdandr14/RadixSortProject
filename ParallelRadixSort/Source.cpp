#include<iostream>
#include<fstream>
#include<mpi.h>
#include<chrono>
#include<ctime>
#include<ratio>
#include "ParallelRadixSortBucket.cpp"
using namespace std;
ifstream fin("../Inputs & Output/input4.txt");
ofstream fout("../Inputs & Output/output.txt");
#define BUFFER_SIZE 100000000

int main(int& argc, char** argv) {

	int element, noElemForProc, PID, NO_PROCS, NO_ELEMENTS;
	fin >> NO_ELEMENTS;
	int* toSort = new int [NO_ELEMENTS];
	for (int i = 0; i < NO_ELEMENTS; ++i) {
		fin >> element;
		toSort[i] = element;
	}

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &NO_PROCS);
	MPI_Comm_rank(MPI_COMM_WORLD, &PID);

    int* buffer = new int[BUFFER_SIZE];
    MPI_Buffer_attach( malloc(BUFFER_SIZE), BUFFER_SIZE);

    noElemForProc = NO_ELEMENTS / NO_PROCS;

    if (noElemForProc < 1) {
        if (PID == 0) {
            cout << "Number of elements must be bigger than the number of processes\n";
        }
        MPI_Finalize();
        return 1;
    }

    if (BASE % NO_PROCS) {
        if (PID == 0) {
            cout<< "The number of buckets [" << BASE <<"] must be divisible by number of processes\n";
        }
        MPI_Finalize();
        return 1;
    }

    typedef std::chrono::high_resolution_clock Clock;
    std::chrono::steady_clock::time_point t1;
    if (!PID) {
        t1 = Clock::now();
    }

    toSort = ParallelRadixSortBucket(toSort, NO_ELEMENTS, NO_PROCS, PID);

	if (!PID) {
		auto t2 = Clock::now();
		std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
		fout << time_span.count() * 1000;
	}

    /*if (!PID) {
        for (int i = 0; i < NO_ELEMENTS; ++i) {
            cout << toSort[i] << " ";
        }
    }*/
    
	delete[] toSort;
    delete[] buffer;
	MPI_Finalize();
	return 0;
}