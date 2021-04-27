package counter;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Semaphore;

import bucket.Generator;
import bucket.RadixSortBucket;
import bucket.RadixThreadBucket;

public class RadixSortCounter {

	private int noThreads = 1;
	private int base;
	private volatile Semaphore semaphore;

	private volatile int[] toSort;
	private volatile int[][] allCounters;
	private volatile int[][] allPrefixSum;
	private volatile int[][] allLocalSorted;
	private volatile int[] globalPrefixSum;
	private volatile int digitsSorted;
	private int noDigits;
	private int elementsPerThread;
	private int addedZeros = 0;

	public RadixSortCounter(int noThreads, int base) {
		this.noThreads = noThreads;
		this.base = base;

		semaphore = new Semaphore(noThreads);
		allCounters = new int[noThreads][base];
		allPrefixSum = new int[noThreads][base];
		allLocalSorted = new int[noThreads][base];
		globalPrefixSum = new int[base];
	}

	public void sort(int[] input, int size) {
		toSort = new int[size];
		digitsSorted = 0;
		addedZeros = (noThreads - (size % noThreads)) % noThreads;
		for (int i = 0; i < addedZeros; ++i) {
			toSort[i] = 0;
		}
		for (int i = addedZeros; i < addedZeros + size; ++i) {
			toSort[i] = input[i - addedZeros];
		}
		elementsPerThread = (addedZeros + size) / noThreads;
		int maxVal = 0;
		for (int i = 0; i < size; ++i) {
			maxVal = maxVal < input[i] ? input[i] : maxVal;
		}
		while (maxVal != 0) {
			++noDigits;
			maxVal /= base;
		}

		for (int TID = 0; TID < noThreads; ++TID) {
			new RadixThreadCounter(TID, base, noDigits, toSort, elementsPerThread, this).start();
		}

		while (digitsSorted != noDigits)
			;
		for (int i = 0; i < elementsPerThread - addedZeros; ++i) {
			input[i] = allLocalSorted[0][i + addedZeros];
		}
		for (int thread = 1; thread < noThreads; ++thread) {
			for (int pos = 0; pos < elementsPerThread; ++pos) {
				input[thread * elementsPerThread + pos - addedZeros] = allLocalSorted[thread][pos];
			}
		}
	}

	public synchronized void computed(int[] localSorted, int[] localDigitCounter, int[] localPrefixSum, int TID) {
		allLocalSorted[TID] = localSorted;
		allCounters[TID] = localDigitCounter;
		allPrefixSum[TID] = localPrefixSum;
	}

	public int[] getNewLocalArray(int TID, int digitToSort) {

		try {
			semaphore.acquire();
			while (semaphore.availablePermits() != 0 && digitToSort >= digitsSorted)
				;
			synchronized (globalPrefixSum) {
				if (digitsSorted == digitToSort) {
					for (int digit = 0; digit < base; ++digit) {
						globalPrefixSum[digit] = 0;
						for (int thread = 0; thread < noThreads; ++thread) {
							globalPrefixSum[digit] += allPrefixSum[thread][digit];
						}
					}
					++digitsSorted;
				}
			}

			semaphore.release();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		/*
		 * Parallel calculation for redistributing the concatenated sorted elements to
		 * each thread.
		 */
		int localStartPoint = TID * elementsPerThread;
		int digit, thread, index, counterSum = 0;

		// Compute the digit that the local thread must start receiving the elements.
		for (digit = 0; digit + 1 < base && globalPrefixSum[digit + 1] <= localStartPoint; ++digit)
			;
		counterSum = globalPrefixSum[digit];

		// Compute the thread from which the local thread must start receiving elements.
		for (thread = 0; thread < noThreads && counterSum + allCounters[thread][digit] <= localStartPoint; ++thread) {
			counterSum += allCounters[thread][digit];
		}

		// Compute the index in the array at the position of [digit] in the [thread]
		// data segment from where the local thread to receive elements.
		index = localStartPoint - counterSum;
		int[] localElements = new int[elementsPerThread];
		// Copy in the local array the elements that need to be sorted at the next step.
		for (int i = 0; i < elementsPerThread; ++i) {
			localElements[i] = allLocalSorted[thread][allPrefixSum[thread][digit] + index++];
			if (index == allCounters[thread][digit]) {
				index = 0;
				++thread; 
				if (thread == noThreads) {
					thread = 0; ++digit;
				} 
				do {
					++thread;
					if (thread >= noThreads) {
						thread = 0;
						++digit;
					}
					if(digit == base) {
						break;
					}
				} while ((i != elementsPerThread - 1) && allCounters[thread][digit] == 0);
			}
		}
		return localElements;

	}

	public static void main(String[] args) {
		int noThreads = 1;
		int base = 4;
		int size = 10;
		int[] toSort;
		toSort = Generator.generateNumbersArray(size, 0, 1000000000);
		RadixSortCounter radix = new RadixSortCounter(noThreads, base);
		long start = System.currentTimeMillis();
		radix.sort(toSort, size);
		long end = System.currentTimeMillis();

		for (int i = 0; i < size; ++i) {
			System.out.print(toSort[i] + " ");
		}

		System.out.println(end - start);
	}

}
