#include<mpi.h>
#include<math.h>
#include<chrono>
#include<ctime>
#include<ratio>
#include<iostream>
#include<fstream>
using namespace std;
//ofstream fout("output.txt");

#define BITS 8
#define BASE (1 << BITS)
#define NO_DIGITS 32 / BITS
#define END_SIZE_TAG 999
#define END_ELEM_TAG 1000
typedef struct list List;
struct list {
    int* array;
    size_t size;
    size_t capacity;
};

// add item to a dynamic array encapsulated in a structure
int addElem(List* list, int elem) {
    if (list->size == list->capacity) {
        size_t newCapacity = list->capacity * 2;
        int* newArray = (int*)realloc(list->array, newCapacity * sizeof(int));
        if (!newArray) {
            std::cout << "Can not realloc memory for a bucket of size" << (int)newCapacity;
            return 1;
        }
        list->capacity = newCapacity;
        list->array = newArray;
    }
    list->array[list->size++] = elem;
    return 0;
}

inline unsigned getDigit(unsigned x, int k, int j) {
    return (x >> k) & ~(~0 << j);
}

int* ParallelRadixSortBucket(int* elementsToSort, int NO_ELEMENTS, int NO_PROCS, int PID) {

    int noLocalElem = NO_ELEMENTS / NO_PROCS; // number of elements for current process
    const int startingPoint = noLocalElem * PID; // starting point for retrieving the elements for current process
    if (PID == NO_PROCS - 1) {
        noLocalElem += NO_ELEMENTS % NO_PROCS; // add the remaining elements to the last process
    }
    int localCapacity = noLocalElem;
    int* localElements = new int[noLocalElem]; // the array for local elements

    //retrieve the elements for current process
    for (int i = 0; i < noLocalElem; ++i) { 
        localElements[i] = elementsToSort[i + startingPoint];
    }

    // Initialize the list of buckets.
    List* buckets = new List[BASE]; 
    int localBuckets = BASE / NO_PROCS;   // number of local buckets for each process
    int bucketCapacity = noLocalElem / BASE + 1;
    if (bucketCapacity < BASE) {
        bucketCapacity = BASE;
    }
    // Initialize each bucket.
    for (int j = 0; j < BASE; ++j) {
        buckets[j].array = new int[bucketCapacity];
        buckets[j].capacity = bucketCapacity;
        buckets[j].size = 0;
    }
    //Initialize the arrays for counting and prefix sums.
    int* prefixSum = new int[BASE]; // the array of prefix sums
    int** count = new int* [NO_PROCS];   // the arrays of counts for all processes
    for (int i = 0; i < NO_PROCS; ++i) {
        count[i] = new int[BASE];
    }
    int* localCount = count[PID]; //Retrieve the local count

    MPI_Request req;
    MPI_Status stat;
   
    MPI_Barrier(MPI_COMM_WORLD);

    typedef std::chrono::high_resolution_clock Clock; // clock for contoring the time
    std::chrono::steady_clock::time_point t1;
    std::chrono::duration<double> time_span;
    if (!PID) {
        t1 = Clock::now(); //start the clock
    }

    for (int digitPos = 0; digitPos < NO_DIGITS; ++digitPos) { // Execute for each position of a digit.

        // Initialize local count array and buckets.
        for (int i = 0; i < BASE; ++i) {
            localCount[i] = 0;
            buckets[i].size = 0;
        }

        // Count all elements and add them to the corresponding bucket.
        for (int i = 0; i < noLocalElem; ++i) { 
            unsigned int digit = getDigit(localElements[i], digitPos * BITS, BITS);
            if (addElem(&buckets[digit], localElements[i])) {
                return NULL;
            }
            ++localCount[digit];
        }

        // Send the count array of this process to all other processes.
        for (int process = 0; process < NO_PROCS; ++process) {
            if (process != PID) {
                MPI_Isend(localCount, BASE, MPI_INT, process, digitPos, MPI_COMM_WORLD, &req);
            }
        }

        // Receive count array from all other processes.
        for (int process = 0; process < NO_PROCS; ++process) {
            if (process != PID) {
                MPI_Recv(count[process], BASE, MPI_INT, process, digitPos, MPI_COMM_WORLD, &stat);
            }
        }

        // Compute new size based on values received from all processes and the prefix sum for receiving the elements.
        int newSize = 0;
        for (int i = 0; i < localBuckets; ++i) {
            int digit = i + PID * localBuckets;
            for (int p = 0; p < NO_PROCS; ++p) {
                prefixSum[p + i * NO_PROCS] = newSize;
                newSize += count[p][digit];
            }
        }

        // Reallocate array if the new size is larger than the current one.
        if (newSize > localCapacity) {
            localCapacity = newSize;
            delete[] localElements;
            localElements = new int[localCapacity];
        }
        noLocalElem = newSize; // Update new size of the array.

        // Send elements of this process to others, depending on the bucket they are in.
        for (int digit = 0; digit < BASE; ++digit) {
            int process = digit / localBuckets; // Determine which process this buckets belongs to.
            int place = digit % localBuckets; // Determine the place in which the elements must go in the receiving process.
            if (process != PID && buckets[digit].size > 0) {
                MPI_Isend(buckets[digit].array, buckets[digit].size, MPI_INT, process, place, MPI_COMM_WORLD, &req);
            }
        }

        // Receive elements from other processes and place them in the local elements array
        for (int i = 0; i < localBuckets; ++i) {
            int digit = i + PID * localBuckets; // Transform from local bucket the global digit in the array of counts.
            for (int process = 0; process < NO_PROCS; ++process) {
                int receiveCount = count[process][digit];  // Get the size of the bucket to receive.
                if (receiveCount > 0) {
                    int* dest = &localElements[prefixSum[process + i * NO_PROCS]]; // Point to the index where to place the received elements.
                    if (PID != process) {
                        MPI_Recv(dest, receiveCount, MPI_INT, process, i, MPI_COMM_WORLD, &stat); // Receive elements from other processes.
                    }
                    else {
                       memcpy(dest, &buckets[digit].array[0], receiveCount * sizeof(int)); // Copy from local bucket to local array. 
                    }
                }
            }
        }
    }

    // Store number of items for each process after the sort.
    /*for (int i = 0; i < noLocalElem; ++i) {
        if (PID) {
            break;
        }
        cout << localElements[i] << " ";
    }*/
    /*if (PID!= 0) {
        MPI_Recv(&noLocalElem, 1, MPI_INT, PID - 1, 1001, MPI_COMM_WORLD, &stat);
    }
    for (int i = 0; i < noLocalElem; ++i) {
        cout << localElements[i] << " ";
    }
    if (PID!= NO_PROCS - 1) {
        MPI_Isend((int*)1, 1, MPI_INT, PID + 1, 1001, MPI_COMM_WORLD, &req);
    }*/
    if (PID > 0) {
        MPI_Isend(&noLocalElem, 1, MPI_INT, 0, END_SIZE_TAG, MPI_COMM_WORLD, &req);
        if (noLocalElem) {
            MPI_Isend(localElements, noLocalElem, MPI_INT, 0, END_ELEM_TAG, MPI_COMM_WORLD, &req);
        }
    }
    else {
        /* auto t2 = Clock::now();
         time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
         fout << time_span.count() * 1000;
         fout.close();*/
        memcpy(&elementsToSort[0], localElements, noLocalElem * sizeof(int));
        //cout << "\nProcess 0 verification size: " << noLocalElem;
        int start = noLocalElem;
        for (int i = 1; i < NO_PROCS; ++i) {
            MPI_Recv(&noLocalElem, 1, MPI_INT, i, END_SIZE_TAG, MPI_COMM_WORLD, &stat);
            if (noLocalElem) {
                //cout << "\nProcess " << i << " verification size: " << noLocalElem;
                int *newV = new int[noLocalElem];
                MPI_Recv(newV, noLocalElem, MPI_INT, i, END_ELEM_TAG, MPI_COMM_WORLD, &stat);
                /*for (int i = 0; i < noLocalElem; ++i) {
                    cout << newV[i] << " ";
                }*/
                memcpy(&elementsToSort[start], newV, noLocalElem * sizeof(int));
            }
            start += noLocalElem;
        }
    }

    // Release memory allocated resources.
    for (int i = 0; i < NO_PROCS; ++i) {
        delete[] count[i];
    }
    for (int j = 0; j < BASE; ++j) {
        delete[] buckets[j].array;
    }
    delete[] count;
    delete[] buckets;
    delete[] localElements;
    delete[] prefixSum;
    
    return elementsToSort;
}

