#include<math.h>

/*
* The sequential implementation of the radix sort algorithm.
* toSort - the array to be sorted
* noElements - the number of elements in the array
* base - the counting base for counting sort
*/
void radixSort(int* toSort, int noElements, int base) {
	int digitValue, maxVal = 0, noDigits = 0;
	int* auxVect = new int[noElements], * digitCounter = new int[base];

	//Find the maximum value in the array to sort.
	for (int i = 0; i < noElements; ++i) {
		maxVal = maxVal < toSort[i] ? toSort[i] : maxVal;
	}

	//Compute the maximum number of digits of the numbers to count
	//based on the base of the numbers.
	while (maxVal) {
		++noDigits;
		maxVal /= base;
	}

	//For each digit position, count the apparition of each digit aparition
	//depending on the base of the numbers.
	for (int pos = 0; pos < noDigits; ++pos) {

		//Initialize the counter vector of the digits, at the position pos.
		for (int i = 0; i < base; ++i) {
			digitCounter[i] = 0;
		}
		int digitPos = (int)pow(base, pos);

		//Copy each element in the array in a new one and
		//increment the counter for the digit at the pos.
		for (int i = 0; i < noElements; ++i) {
			auxVect[i] = toSort[i];
			digitValue = (auxVect[i] / digitPos) % base;
			++digitCounter[digitValue];
		}

		//Compute the prefix sum of the digits.
		for (int i = 1; i < base; ++i) {
			digitCounter[i] += digitCounter[i - 1];
		}

		//Change the position for each element in the array, based on the value
		//of the digit at position pos, to the position in the prefix sum.
		for (int i = noElements - 1; i >= 0; --i) {
			digitValue = (auxVect[i] / digitPos) % base;
			toSort[--digitCounter[digitValue]] = auxVect[i];
		}
	}
	delete[] digitCounter;
	delete[] auxVect;
}