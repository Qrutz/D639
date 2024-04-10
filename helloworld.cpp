#include <iostream>
#include "PrimeChecker.hpp"

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        int number = std::stoi(argv[1]);
        PrimeChecke!!r pc;
        std::cout << "Helsing, Alexander; " << number << " is a prime number? " << pc.isPrime(number) << std::endl;
    }
    return 0;
}
