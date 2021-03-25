#include<iostream>
#include<fstream>
#include<mpi.h>
#include<chrono>
#include<ctime>
#include<ratio>
using namespace std;
ifstream fin("../Inputs & Output/input6.txt");
ofstream fout("../Inputs & Output/output.txt");
#define BASE 256

/* Returns the number of digits of the maximum number in the given array.
 */
int getDigits(int* elements, int noElements) {
	int maxValue = 0, noDigits = 0;
	for (int i = 0; i < noElements; ++i) {
		maxValue = (elements[i] > maxValue) ? elements[i] : maxValue;
	}
	while (maxValue) {
		maxValue /= BASE;
		++noDigits;
	}
	return noDigits;
}

/* Sort the input by the digit at the digitPos position.
 */
void countingSort(int* toSort, int noElements, int digitPos) {
	int count[BASE] = { 0 };
	int* aux = new int[noElements];

	// Build the counting array.
	for (int i = 0; i < noElements; ++i) {
		aux[i] = toSort[i];
		++count[(toSort[i] / digitPos) % BASE];
	}

	// Create the prefix sum of the array.
	for (int i = 0; i < BASE - 1; ++i) {
		count[i + 1] += count[i];
	}

	// Sort the array according to the prefix sums.
	for (int i = noElements - 1; i >= 0; --i) {
		toSort[--count[(aux[i] / digitPos) % BASE]] = aux[i];
	}

	delete[] aux;
}

/* Check the equality of two arrays of the same length.
 */
int compareArrays(int* array1, int* array2, int noElements) {
	for (int i = 0; i < noElements; ++i) {
		//cout << array1[i] << " " << array2[i] << "\n";
		if (array1[i] != array2[i]) {
			return 0;
		}
	}
	return 1;
}

/* Sorts the input sequentially using radix sort with counting sort subroutine.
 */
void sequentialRadixSort(int* toSort, int noElements) {
	long long noDigits = getDigits(toSort, noElements);
	int digitPos = 1;
	while(noDigits--) {
		countingSort(toSort, noElements, digitPos);
		digitPos *= BASE;
	}
}

/* Check the correctness of parallel radix sort. Sorts the original array with the sequential implementation,
 * then compares the 2 arrays.
 */
void checkCorrect(int* originalInput, int* sortedInput, int noElements) {
	int* toCompare = new int[noElements];
	for (int i = 0; i < noElements; ++i) {
		toCompare[i] = originalInput[i];
	}
	sequentialRadixSort(toCompare, noElements);
	if (!compareArrays(toCompare, sortedInput, noElements)) {
		cout << "incorrect";
	}

	delete[] toCompare;
}

/* Sorts the input parralel using radix sort with counting sort subroutine and MPI for parallelization.
 */
void parallelRadixSort(int* toSort, int ELEM_FOR_PROC, int NO_PROCS, int PID) {

	//Initialize array for local elements and variables
	int* localElements = new int[ELEM_FOR_PROC] { 0 };
	//Send to each process an equally sized array that contains a different data segment.
	MPI_Scatter(toSort, ELEM_FOR_PROC, MPI_INT, localElements, ELEM_FOR_PROC, MPI_INT, 0, MPI_COMM_WORLD);

	/*--------------------------------------------------------------------------------------------------*/
	/*Find the maximum value of the numbers in the given array and compute its number of digits*/

	int globalMaxNumber, localMaxNumber = -1, noDigits = 0;

	// Compute the local maximum number from the array of the current process.
	for (int i = 0; i < ELEM_FOR_PROC; i++) {
		localMaxNumber = (localElements[i] > localMaxNumber) ? localElements[i] : localMaxNumber;
	}

	// Compute the global maximum number by comparing all local maximum numbers found.
	MPI_Allreduce(&localMaxNumber, &globalMaxNumber, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

	// Compute the number of digits of the global maximum number.
	while (globalMaxNumber) {
		++noDigits;
		globalMaxNumber /= BASE;
	}

	// Initialize the arrays for counting.
	int globalPrefixSum[BASE]{ 0 };
	int sortingPrefixSum[BASE]{ 0 };
	int* localSorted = new int[ELEM_FOR_PROC];
	int* allLocalCounters = new int[BASE * NO_PROCS];
	int* allLocalPrefixSums = new int[BASE * NO_PROCS];

	// Repeat counting sort for each digit position.
	int digitPos = 1;
	while (noDigits--) {
		/*--------------------------------------------------------------------------------------------------*/
		/*Parallel calculation that implements the counting sort in each process for a portion of the array.*/

		// Initialize the counter vector of the digits and the prefix sum used.
		int localDigitCounter[BASE]{ 0 }, localPrefixSum[BASE]{ 0 };

		// Increment the counter for the digit at the [pos].
		for (int i = 0; i < ELEM_FOR_PROC; ++i) {
			++localDigitCounter[(localElements[i] / digitPos) % BASE];
		}

		// Create the prefix sums for sorting the array locally and for redistributing the sorted elements.
		for (int i = 0; i < BASE - 1; ++i) {
			sortingPrefixSum[i] = localPrefixSum[i + 1] = localPrefixSum[i] + localDigitCounter[i];
		}
		sortingPrefixSum[BASE - 1] = localPrefixSum[BASE - 1] + localDigitCounter[BASE - 1];

		// Sort the local array.
		for (int i = ELEM_FOR_PROC - 1; i >= 0; --i) {
			localSorted[--sortingPrefixSum[(localElements[i] / digitPos) % BASE]] = localElements[i];
		}

		/*--------------------------------------------------------------------------------------------------*/
		/*Concatenate the sorted arrays, digit counters and prefix sums and send them to all processes.*/

		MPI_Allgather(localSorted, ELEM_FOR_PROC, MPI_INT, toSort, ELEM_FOR_PROC, MPI_INT, MPI_COMM_WORLD);
		MPI_Allgather(localDigitCounter, BASE, MPI_INT, allLocalCounters, BASE, MPI_INT, MPI_COMM_WORLD);
		MPI_Allgather(localPrefixSum, BASE, MPI_INT, allLocalPrefixSums, BASE, MPI_INT, MPI_COMM_WORLD);
		// Sums the values from each counting array [localPrefixSum] and store the result in the counter [globalPrefixSum].
		MPI_Allreduce(localPrefixSum, globalPrefixSum, BASE, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
		// Does the same thing.
		/*for (int digit = 0; digit < BASE; ++digit) {
			for (int process = 0; process < NO_PROCS; ++process) {
				globalPrefixSum[digit] += allLocalPrefixSums[process * BASE + digit];
			}
		}*/

		/*--------------------------------------------------------------------------------------------------*/
		/*Parallel calculation for redistributing the concatenated sorted elements to each process.*/
		int localStartPoint = PID * ELEM_FOR_PROC;
		int digit, process, index, counterSum;

		// Compute the digit that the local process must start receiving the elements.
		for (digit = 0; globalPrefixSum[digit + 1] <= localStartPoint; ++digit);
		counterSum = globalPrefixSum[digit];

		// Compute the process from which the local process must start receiving elements. 
		for (process = 0; counterSum + allLocalCounters[digit + process * BASE] <= localStartPoint; ++process) {
			counterSum += allLocalCounters[digit + process * BASE];
		}

		// Compute the index in the array at the position of [digit] in the [process] data segment from where the local process to receive elements.
		index = localStartPoint - counterSum;
		int counterPosition = digit + process * BASE;
		int processPosition = process * ELEM_FOR_PROC;
		// Copy in the local array the elements that need to be sorted at the next step.
		for (int i = 0; i < ELEM_FOR_PROC; ++i) {
			localElements[i] = toSort[processPosition + allLocalPrefixSums[counterPosition] + index++];
			if (index == allLocalCounters[counterPosition]) {
				index = 0;
				++process;
				if (process == NO_PROCS) {
					processPosition = process = 0;
					counterPosition = ++digit;
				}
				else {
					processPosition += ELEM_FOR_PROC;
					counterPosition += BASE;
				}
			}
		}
		digitPos *= BASE;
	}
	// Copy the elements back in the initial array.
	MPI_Gather(localElements, ELEM_FOR_PROC, MPI_INT, toSort, ELEM_FOR_PROC, MPI_INT, 0, MPI_COMM_WORLD);

	delete[] localElements;
	delete[] localSorted;
	delete[] allLocalCounters;
	delete[] allLocalPrefixSums;
}

int main(int& argc, char** argv) {
	int PID, NO_PROCS, ELEM_FOR_PROC, NO_ELEMENTS;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &NO_PROCS);
	MPI_Comm_rank(MPI_COMM_WORLD, &PID);

	fin >> NO_ELEMENTS;
	//Initialize array for elements to be sorted.
	int* toSort = new int[NO_ELEMENTS] { 0 };
	int* copyArray = new int[NO_ELEMENTS] { 0 };

	if (!PID) {
		for (int i = 0; i < NO_ELEMENTS; ++i) {
			fin >> toSort[i];
			copyArray[i] = toSort[i];
		}
	}

	if (NO_ELEMENTS % NO_PROCS) {
		cout << "can not equally divide the input";
		MPI_Finalize();
		return 0;
	}
	ELEM_FOR_PROC = NO_ELEMENTS / NO_PROCS;

	//Get the current moment of the clock for timing the sorting duration.
	std::chrono::steady_clock::time_point t1 = std::chrono::high_resolution_clock::now();

	parallelRadixSort(toSort, ELEM_FOR_PROC, NO_PROCS, PID);

	//Get the current moment of the clock for the ending time of the sorting algorithm.
	auto t2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
	
	if (!PID) {
		/*for (int i = 0; i < NO_ELEMENTS; ++i) {
			cout << toSort[i] << " ";
		}*/
		checkCorrect(copyArray, toSort, NO_ELEMENTS);
		fout << time_span.count() * 1000;
	}

	delete[] toSort;
	delete[] copyArray;
	MPI_Finalize();
	return 0;
}