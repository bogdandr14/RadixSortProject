package bucket;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Semaphore;

public class RadixSortBucket extends Thread {
	private int noThreads = 1;
	private int base;
	private volatile Semaphore semaphore;
	private volatile List<Integer>[][] allBuckets;

	private volatile List<Integer> toSort;
	private volatile int digitsSorted;
	private int noDigits;
	private int elementsPerThread;
	private int addedZeros = 0;

	public RadixSortBucket(int noThreads, int base) {
		this.noThreads = noThreads;
		this.base = base;

		semaphore = new Semaphore(noThreads);
		allBuckets = new ArrayList[noThreads][base];
		for (int i = 0; i < noThreads; ++i) {
			for (int j = 0; j < base; ++j) {
				allBuckets[i][j] = new ArrayList<Integer>();
			}
		}
	}

	public List<Integer> sort(List<Integer> input) {
		toSort = input;
		digitsSorted = 0;
		addedZeros = (noThreads - (toSort.size() % noThreads)) % noThreads;
		for (int i = 0; i < addedZeros; ++i) {
			toSort.add(0);
		}
		elementsPerThread = toSort.size() / noThreads;
		int maxVal = 0;
		for (int i = 0; i < input.size(); ++i) {
			int element = input.get(i);
			maxVal = maxVal < element ? element : maxVal;
		}
		while (maxVal != 0) {
			++noDigits;
			maxVal /= base;
		}

		for (int TID = 0; TID < noThreads; ++TID) {
			List<Integer> localSort = new ArrayList<Integer>();
			localSort.addAll(toSort.subList(TID * elementsPerThread, (TID + 1) * elementsPerThread));
			new RadixThreadBucket(TID, base, noDigits, localSort, this).start();
		}

		while (digitsSorted != noDigits)
			;
		return toSort.subList(addedZeros, toSort.size());
	}

	public List<Integer> computeToSort(List<Integer>[] buckets, int TID, int digitToSort) {
		allBuckets[TID] = buckets;

		try {
			semaphore.acquire();
			while (semaphore.availablePermits() != 0 && digitToSort >= digitsSorted)
				;
			synchronized (toSort) {
				if (digitsSorted == digitToSort) {
					toSort.clear();
					for (int digit = 0; digit < base; ++digit) {
						for (int thread = 0; thread < noThreads; ++thread) {
							if (allBuckets[thread][digit].size() != 0) {
								toSort.addAll(allBuckets[thread][digit]);
							}
						}
					}
					++digitsSorted;
				}
			}
			semaphore.release();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}

		return toSort.subList(TID * elementsPerThread, (TID + 1) * elementsPerThread);
	}

	public static void main(String[] args) {
		int noThreads = 32;
		int base = 256;
		List<Integer> toSort = new ArrayList<Integer>();
		toSort = Generator.generateNumbersList(100000, 0, 1000000000);
		RadixSortBucket radix = new RadixSortBucket(noThreads, base);
		long start = System.currentTimeMillis();
		toSort = radix.sort(toSort);
		long end = System.currentTimeMillis();
		System.out.println(end-start);
	}
}
