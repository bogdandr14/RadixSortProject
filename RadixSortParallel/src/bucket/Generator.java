package bucket;

import java.util.ArrayList;
import java.util.List;

public class Generator {
	/**
	 * Creates an integer number in a given interval and returns it.
	 * 
	 * @param noElements The number of numbers to be generated.
	 * @param minVal The minimum value of the number.
	 * @param maxVal The maximum value of the number.
	 * @return The integer number created.
	 */
	public static List<Integer> generateNumbersList(int noElements, int minVal, int maxVal) {
		List<Integer> generatedElements = new ArrayList<>();
		for(int i = 0; i<noElements; ++i) {
			generatedElements.add((int)(Math.random() * (maxVal - minVal + 1) + minVal));
		}
		return generatedElements;
	}
	
	/**
	 * Creates an integer number in a given interval and returns it.
	 * 
	 * @param noElements The number of numbers to be generated.
	 * @param minVal The minimum value of the number.
	 * @param maxVal The maximum value of the number.
	 * @return The integer number created.
	 */
	public static int[] generateNumbersArray(int noElements, int minVal, int maxVal) {
		int[] generatedElements = new int[noElements];
		for(int i = 0; i<noElements; ++i) {
			generatedElements[i]=(int)(Math.random() * (maxVal - minVal + 1) + minVal);
		}
		return generatedElements;
	}
}
