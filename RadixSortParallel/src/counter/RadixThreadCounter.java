package counter;

import java.util.ArrayList;
import java.util.List;

import bucket.RadixSortBucket;

public class RadixThreadCounter extends Thread {
	private int TID;
	private int base;
	private int noDigits;
	private int[] localDigitCounter;

	private int[] localElements;
	private RadixSortCounter radixSort;
	private int noElements;

	public RadixThreadCounter(int TID, int base, int noDigits, int[] toSort, int noElements,
			RadixSortCounter radixSort) {
		this.TID = TID;
		this.base = base;
		this.noDigits = noDigits;
		localElements = new int[noElements];
		for (int i = 0; i < noElements; ++i) {
			localElements[i] = toSort[i + TID * noElements];
		}
		this.noElements = noElements;
		this.radixSort = radixSort;

		localDigitCounter = new int[base];
	}

	public void setNoDigits(int noDigits) {
		this.noDigits = noDigits;
	}

	@Override
	public void run() {
		int[] localSorted = new int[noElements];
		System.out.println(noElements);
		for (int digitPos = 1, digit = 0; digit < noDigits; ++digit, digitPos *= base) {
			int[] localPrefixSum = new int[base], sortingPrefixSum = new int[base];
			for (int i = 0; i < base; ++i) {
				localPrefixSum[i] = sortingPrefixSum[i] = localDigitCounter[i] = 0;

			}
			for (int i = 0; i < noElements; ++i) {
				++localDigitCounter[(localElements[i] / digitPos) % base];
			}

			for (int i = 0; i < base - 1; ++i) {
				sortingPrefixSum[i] = localPrefixSum[i + 1] = localPrefixSum[i] + localDigitCounter[i];
				System.out.print(" digit "+ i + ": " + localDigitCounter[i]);
			}
			System.out.println(" digit "+ (base-1) + ": " + localDigitCounter[base-1]);
			sortingPrefixSum[base - 1] = localPrefixSum[base - 1] + localDigitCounter[base - 1];

			for (int i = noElements - 1; i >= 0; --i) {
				localSorted[--sortingPrefixSum[(localElements[i] / digitPos) % base]] = localElements[i];
			}
			radixSort.computed(localSorted, localDigitCounter, localPrefixSum, TID);
			localElements = radixSort.getNewLocalArray(TID, digit);

		}
	}

}
