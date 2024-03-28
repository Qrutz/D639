#include <iostream>

// function to print the hello world
int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Hello, World!" << std::endl;
    } else {
        std::cout << "Hello, " << argv[1] << "!" << std::endl;
    }
    return 0;
}
