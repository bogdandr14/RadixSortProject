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
ifstream fin("input.txt");
ofstream fout("output.txt");

int PID, NO_PROCS, ELEM_PER_PROC, NO_ELEMENTS, BASE = 10;

int main(int& argc, char** argv) {

	int globalMaxNumber, maxNumber = -1;
	int element, noDigits = 0, digitValue;
	double timeM;
	fin >> NO_ELEMENTS;
	cout << NO_ELEMENTS;
	typedef std::chrono::high_resolution_clock Clock;

	int** toSort = new int*[2], *globalDigitCounter = new int[BASE], *localDigitCounter = new int[BASE];
	toSort[0] = new int[NO_ELEMENTS];
	toSort[1] = new int[NO_ELEMENTS];

	for (int i = 0; i < NO_ELEMENTS; ++i) {
		fin >> element;
		toSort[0][i] = element;
	}

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &NO_PROCS);
	MPI_Comm_rank(MPI_COMM_WORLD, &PID);

	if (NO_ELEMENTS % NO_PROCS) {
		cout << "can not equally divide the input";
		return 0;
	}
	auto t1 = Clock::now();
	if (!PID) {
		timeM = clock();
		t1 = Clock::now();
	}
	{
		ELEM_PER_PROC = NO_ELEMENTS / NO_PROCS; /* must be an integer */
		int* maxSort = new int[ELEM_PER_PROC];

		/* Scatter data  to each processor in order cu calculate the maximum number in the array.*/
		MPI_Scatter(toSort[0], ELEM_PER_PROC, MPI_INT, maxSort, ELEM_PER_PROC, MPI_INT, 1, MPI_COMM_WORLD);

		/* add portion of data */

		for (int i = 0; i < ELEM_PER_PROC; i++) {
			maxNumber = (maxSort[i] > maxNumber) ? maxSort[i] : maxNumber;
		}
		//printf("I got maximum number %d from %d\n", maxNumber, PID);
		/* compute global sum */
		MPI_Reduce(&maxNumber, &globalMaxNumber, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

		/*if (0 == PID) {
			printf("Maximum number is %d.\n", globalMaxNumber);
		}*/

		while (globalMaxNumber) {
			++noDigits;
			globalMaxNumber /= BASE;
		}
		delete[] maxSort;
	}


	int* countingSort = new int[ELEM_PER_PROC];

	for (int pos = 0; pos < noDigits; ++pos) {

		int digitPos = (int)pow(BASE, pos);
		int current = pos % 2, next = (pos + 1) % 2;
		/*Parallel calculation that implements the counting sort in each process for a portion of the array.*/
		{
			/*Send to each process an equally sized array that contains a different data segment.*/
			MPI_Scatter(toSort[current], ELEM_PER_PROC, MPI_INT, countingSort, ELEM_PER_PROC, MPI_INT, 0, MPI_COMM_WORLD);

			/*Initialize the counter vector of the digits, at the position [pos].*/
			for (int i = 0; i < BASE; ++i) {
				localDigitCounter[i] = 0;
			}

			/*Copy each element in the array in a new one and increment the counter for the digit at the [pos].*/
			for (int i = 0; i < ELEM_PER_PROC; ++i) {
				++localDigitCounter[(countingSort[i] / digitPos) % BASE];
			}

			/*Create the prefix sum for the counter vector.*/
			for (int i = 1; i < BASE; ++i) {
				localDigitCounter[i] += localDigitCounter[i - 1];
			}

			/*Sums the values from each counting array [parallelDigitCounter] and store the result 
			in the counter [globalDigitCounter].*/
			MPI_Reduce(localDigitCounter, globalDigitCounter, BASE, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
		}

		/*Bottleneck must be improved */
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
		timeM = clock() - timeM;
		auto t2 = Clock::now();
		std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);

		/*for (int i = 0; i < NO_ELEMENTS/100; ++i) {
			cout << toSort[noDigits % 2][i] << " ";
		}*/
		fout << timeM <<"\n"<< time_span.count();
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