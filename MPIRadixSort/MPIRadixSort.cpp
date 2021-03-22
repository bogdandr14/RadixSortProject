#include<mpi.h>
#include<math.h>
#include<string>
#include<iostream>
#define BITS 8
#define BASE 1 << BITS
#pragma once

int calculateNumberOfDigits(int* elements, int elemForProc, int noProcs, int PID) {
	int globalMaxNumber = 0, localMaxNumber = -1;
	int noDigits = 0;

	int* maxSort = new int[elemForProc];

	/* Scatter data  to each processor in order cu calculate the maximum number in the array.*/
	MPI_Scatter(elements, elemForProc, MPI_INT, maxSort, elemForProc, MPI_INT, 1, MPI_COMM_WORLD);

	/* add portion of data */

	for (int i = 0; i < elemForProc; ++i) {
		localMaxNumber = (maxSort[i] > localMaxNumber) ? maxSort[i] : localMaxNumber;
	}
	//printf("I got maximum number %d from %d\n", localMaxNumber, PID);
	/* compute global sum */
	MPI_Reduce(&localMaxNumber, &globalMaxNumber, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

	if (0 == PID) {
		printf("Maximum number is %d.\n", globalMaxNumber);
	}

	while (globalMaxNumber) {
		++noDigits;
		globalMaxNumber /= BASE;
	}
	delete[] maxSort;

	return noDigits;
}
inline unsigned getDigit(int element, int pos) {
	return (element >> (pos * BITS)) & ~(~0 << pos);
}

void MPIRadixSort(int* elements, int noElements, int noProcs, int pid) {

	int noDigits = 0;
	int element, elemForProc = noElements / noProcs; /* must be an integer */
	noDigits = calculateNumberOfDigits(&elements[0], elemForProc, noProcs, pid);

	int** toSort = new int* [2]{ new int[noElements], new int[noElements] };
	for (int i = 0; i < noElements; ++i) {
		toSort[0][i] = elements[i];
	}
	MPI_Status stat;
	MPI_Request req;
	int* countingSort = new int[elemForProc] {};
	int* localDigitCounter = new int[BASE], *globalDigitCounter = new int[BASE];
	int current, next, pos;

	for (pos = 0; pos < noDigits; ++pos) {
		int digitPos = (int)pow(BASE, pos);
		int current = pos % 2, next = (pos + 1) % 2;

		/*Parallel calculation that implements the counting sort in each process for a portion of the array.*/
		
			/*Send to each process an equally sized array that contains a different data segment.*/
			MPI_Scatter(toSort[current], elemForProc, MPI_INT, countingSort, elemForProc, MPI_INT, 0, MPI_COMM_WORLD);

			/*Initialize the counter vector of the digits, at the position [pos].*/
			for (int i = 0; i < BASE; ++i) {
				localDigitCounter[i] = 0;
			}

			/*Copy each element in the array in a new one and increment the counter for the digit at the [pos].*/
			for (int i = 0; i < elemForProc; ++i) {
				++localDigitCounter[(countingSort[i] / digitPos) % BASE];
			}

			/*Create the prefix sum for the counter vector.*/
			for (int i = 1; i < BASE; ++i) {
				localDigitCounter[i] += localDigitCounter[i - 1];
			}

			/*Sums the values from each counting array [parallelDigitCounter] and store the result
			in the counter [globalDigitCounter].*/
			//MPI_Reduce(localDigitCounter, globalDigitCounter, BASE, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
		

		/*Bottleneck must be improved */
		if (!pid) {
			//for (int i = 0; i < BASE; ++i) {
			//	cout << globalDigitCounter[i] << " ";
			//}
			//cout << endl;
			for (int i = noElements - 1; i >= 0; --i) {
				//toSort[next][--globalDigitCounter[(toSort[current][i] / digitPos) % BASE]] = toSort[current][i];
			}
		}
	}
	memcpy(elements, toSort[noDigits % 2], noElements * sizeof(int));
	delete[] localDigitCounter;
	delete[] globalDigitCounter;
	delete[] countingSort;
	delete[] toSort[0];
	delete[] toSort[1];
	delete[] toSort;
}