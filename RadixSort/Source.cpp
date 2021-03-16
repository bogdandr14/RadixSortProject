#include<fstream>
#include<algorithm>
#include "SeqRadixSort.cpp"
#include "NumberGenerator.cpp"
#include<chrono>
#include<ctime>
#include<ratio>

#define BASE 256
#define NO_ELEMENTS 10000000
using namespace std;
ofstream fout("output.txt");

/*
* Method used for comparison in the quick sort algorithm.
*/
inline int cmpfunc3(const void* a, const void* b) {
	return (*(int*)a - *(int*)b);
}

/*
* Tests the duration for sorting the array using radix sort algorithm.
* toSort - the array to be sorted
* noElements - the total number of elements in the array
*/
void testRadixSort(int* toSort, int noElements) {

	typedef std::chrono::high_resolution_clock Clock;
	auto t1 = Clock::now();
	radixSort(toSort, noElements, BASE);
	auto t2 = Clock::now();

	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);

	fout << " The time for the radix sort is: " << time_span.count()*1000 << "\n";
	/*for (int i = 0; i < noElements;++i) {
		cout << toSort[i] << " ";
	}*/
	cout << "\n";
}

/*
* Tests the duration for sorting the array using std sort algorithm.
* toSort - the array to be sorted
* noElements - the total number of elements in the array
*/
void testStdSort(int* toSort, int noElements) {

	typedef std::chrono::high_resolution_clock Clock;
	auto t1 = Clock::now();
	sort(toSort, toSort + noElements);
	auto t2 = Clock::now();

	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);

	fout << " The time for the gcc sort is: " << time_span.count()*1000 << "\n";
}

/*
* Tests the duration for sorting the array using quick sort algorithm.
* toSort - the array to be sorted
* noElements - the total number of elements in the array
*/
void testQuickSort(int* toSort, int noElements) {

	typedef std::chrono::high_resolution_clock Clock;
	auto t1 = Clock::now();
	qsort(toSort, noElements, sizeof(int), cmpfunc3);
	auto t2 = Clock::now();

	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);

	fout << " The time for the quick sort is: " << time_span.count()*1000 << "\n";
}

int main() {
	generateInputNumbers(0, INT_MAX, NO_ELEMENTS);

	ifstream fin("input.txt");
	int element, i = 0, noElements;
	fin >> noElements;
	int* toSortRadix = new int[noElements];
	int* toSortGcc = new int[noElements];
	int* toSortQuick = new int[noElements];
	for (int i = 0; i < noElements; ++i) {
		if (i % 1000000 == 0) {
			cout << "\nRead so far: " << i;
		}
		fin >> element;
		toSortRadix[i] = toSortGcc[i] = toSortQuick[i] = element;
	}

	testStdSort(toSortGcc, noElements);
	testQuickSort(toSortQuick, noElements);
	testRadixSort(toSortRadix, noElements);

	delete[] toSortGcc;
	delete[] toSortQuick;
	delete[] toSortRadix;
	fin.close();
	fout.close();
	return 0;
}