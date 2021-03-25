#include<random>
#include<time.h>
#include<fstream>
#include<iostream>

/**
 * Here are generated the input datasets of random numbers by providing the
 * number of elements, minimum and maximum values of the numbers generated.
 */

 /*
 *Generates a random number in the given range [min, max].
 * min - minimum possible value of the number generated
 * max - maximum possible value of the number generated
 */
int randomNumberInRange(long long min, long long max) {
    long long interval = max - min + 1;
    long long currentPossibleMaximum = RAND_MAX;
    long long generatedNumber = rand();
    while (currentPossibleMaximum < interval) {
        generatedNumber = generatedNumber * rand() + rand();
        currentPossibleMaximum = currentPossibleMaximum * RAND_MAX + RAND_MAX;
    }
    return (int)(generatedNumber % interval) + min;
}

/*
* Generated the requested number of numbers in the given range and stores
* them in the input file of the project.
* min - minimum possible value of the number generated
* max - maximum possible value of the number generated
* noNumbers - the number of numbers to be generated
*/
void generateInputNumbers(int min, int max, int noNumbers) {
    std::ofstream fout("../Inputs/input.txt");
    srand(time(0));
    fout << noNumbers << "\n";
    std::cout << "Started generating numbers\n";
    for (int i = 0; i < noNumbers; ++i) {
        fout << randomNumberInRange(min, max) << "\n";
        if (i % 10000000 == 0) {
            std::cout << "Generated so far: " << i << "\n";
        }
    }
    std::cout << "\nFinished generating numbers\n";
    fout.close();
}
