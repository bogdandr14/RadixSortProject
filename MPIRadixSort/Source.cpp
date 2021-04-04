#include<iostream>
#include<fstream>
#include<time.h>
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<mpi.h>
#include<chrono>
#include<ctime>
#include<ratio>
using namespace std;
ifstream fin("../Inputs & Output/input5.txt");
ofstream fout("../Inputs & Output/output.txt");
#define BASE 256

int main(int& argc, char** argv) {
	int PID, NO_PROCS, ELEM_FOR_PROCS, NO_ELEMENTS, element, addedZeros;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &NO_PROCS);
	MPI_Comm_rank(MPI_COMM_WORLD, &PID);

	fin >> NO_ELEMENTS;
	
	addedZeros = (NO_PROCS - NO_ELEMENTS % NO_PROCS) % NO_PROCS;
	NO_ELEMENTS += addedZeros;

	//Initialize array for elements to be sorted.
	int** toSort = new int*[2];
	toSort[0] = new int[NO_ELEMENTS];
	toSort[1] = new int[NO_ELEMENTS];
	for (int i = 0; i < NO_ELEMENTS; ++i) {
		if (i < addedZeros) {
			toSort[0][i] = 0;
		}
		else {
			fin >> element;
			toSort[0][i] = element;
		}
	}

	ELEM_FOR_PROCS = NO_ELEMENTS / NO_PROCS;

	//Get the current moment of the clock for timing the sorting duration.
	std::chrono::steady_clock::time_point t1;
	if (!PID) {
		t1 = std::chrono::high_resolution_clock::now();
	}

	/*--------------------------------------------------------------------------------------------------*/
	/*Compute the maximum value of numbers in the given array*/

	//Initialize array for local elements and variables
	int* maxSort = new int[ELEM_FOR_PROCS];
	int globalMaxNumber, localMaxNumber = -1, noDigits = 0;

	//Scatter data  to each processor in order cu calculate the maximum number in the array.
	MPI_Scatter(toSort[0], ELEM_FOR_PROCS, MPI_INT, maxSort, ELEM_FOR_PROCS, MPI_INT, 1, MPI_COMM_WORLD);

	// Compute the local maximum number from the array of the current process.
	for (int i = 0; i < ELEM_FOR_PROCS; i++) {
		localMaxNumber = (maxSort[i] > localMaxNumber) ? maxSort[i] : localMaxNumber;
	}

	// Compute the global maximum number by comparing the local maximum numbers.
	MPI_Reduce(&localMaxNumber, &globalMaxNumber, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

	/*if (0 == PID) {
		printf("Maximum number is %d.\n", globalMaxNumber);
	}*/

	// Compute the number of digits of the maximum number.
	while (globalMaxNumber) {
		++noDigits;
		globalMaxNumber /= BASE;
	}
	delete[] maxSort;

	// Initialize the arrays for counting.
	int* countingSort = new int[ELEM_FOR_PROCS];
	int* globalDigitCounter = new int[BASE], * localDigitCounter = new int[BASE];

	//Repeat counting sort for each digit position.
	for (int pos = 0; pos < noDigits; ++pos) {
		int digitPos = (int)pow(BASE, pos);
		int current = pos % 2, next = (pos + 1) % 2;

		/*--------------------------------------------------------------------------------------------------*/
		/*Parallel calculation that implements the counting sort in each process for a portion of the array.*/
		
		//Send to each process an equally sized array that contains a different data segment.
		MPI_Scatter(toSort[current], ELEM_FOR_PROCS, MPI_INT, countingSort, ELEM_FOR_PROCS, MPI_INT, 0, MPI_COMM_WORLD);

		//Initialize the counter vector of the digits, at the position [pos].
		for (int i = 0; i < BASE; ++i) {
			localDigitCounter[i] = 0;
		}

		//Copy each element in the array in a new one and increment the counter for the digit at the [pos].
		for (int i = 0; i < ELEM_FOR_PROCS; ++i) {
			++localDigitCounter[(countingSort[i] / digitPos) % BASE];
		}

		//Create the prefix sum for the counter vector.
		for (int i = 1; i < BASE; ++i) {
			localDigitCounter[i] += localDigitCounter[i - 1];
		}

		//Sums the values from each counting array [parallelDigitCounter] and store the result in the counter [globalDigitCounter].
		MPI_Reduce(localDigitCounter, globalDigitCounter, BASE, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
		
		/*------------------------------------------------------------------------------------------------------*/
		/* Move the elements to their new position in the array, according to the radix sort algorithm.*/
		//Bottleneck must be improved
		if (!PID) {
			//for (int i = 0; i < BASE; ++i) {
			//	cout << globalDigitCounter[i] << " ";
			//}
			//cout << endl;
			for (int i = NO_ELEMENTS - 1; i >= 0; --i) {
				toSort[next][--globalDigitCounter[(toSort[current][i] / digitPos) % BASE]] = toSort[current][i];
			}
		}
	}

	if (!PID) {
		//Get the current moment of the clock for the ending time of the sorting algorithm.
		auto t2 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);

		/*for (int i = addedZeros; i < NO_ELEMENTS; ++i) {
			cout << toSort[noDigits % 2][i] << " ";
		}*/
		fout << time_span.count() * 1000;
	}
	
	delete[] localDigitCounter;
	delete[] globalDigitCounter;
	delete[] countingSort;
	delete[] toSort[0];
	delete[] toSort[1];
	delete[] toSort;

	MPI_Finalize();
	return 0;
}