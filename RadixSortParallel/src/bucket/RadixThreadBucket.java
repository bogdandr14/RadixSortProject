package bucket;
import java.util.ArrayList;
import java.util.List;

public class RadixThreadBucket extends Thread {
	private int TID;
	private int base;
	private int noDigits;
	List<Integer>[] buckets;

	private List<Integer> localElements;
	private RadixSortBucket radixSort;

	public RadixThreadBucket(int TID, int base, int noDigits, List<Integer> localElements, RadixSortBucket radixSort) {
		this.TID = TID;
		this.base = base;
		this.noDigits = noDigits;
		this.localElements = localElements;
		this.radixSort = radixSort;

		buckets = new ArrayList[base];
		for (int i = 0; i < base; ++i) {
			buckets[i] = new ArrayList<Integer>();
		}
	}

	public void setNoDigits(int noDigits) {
		this.noDigits = noDigits;
	}

	@Override
	public void run() {
		for (int digitPos = 1, digit = 0; digit < noDigits; ++digit, digitPos *= base) {
			for (int i = 0; i < base; ++i) {
				buckets[i].clear();
			}
			
			final int digitAux = digitPos;
			localElements.forEach((element)->{
				buckets[(element / digitAux) % base].add(element);
			});
			
			localElements.clear();
			localElements.addAll(radixSort.computeToSort(buckets, TID, digit));

		}
	}

}
