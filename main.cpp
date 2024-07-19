#include <iostream>
#include <string>




int main(int argc, char *argv[]) {
    std::string path = ".";
    if (argc > 1) { path = argv[1]; }

    std::cout << "Reads 'filematch.txt' and outputs all files from directory that matches description or nothing if no 'filematch.txt' is found." << std::endl;
    return 0;
}
