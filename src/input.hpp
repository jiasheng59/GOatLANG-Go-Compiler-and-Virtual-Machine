#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

std::string input(const std::string& filename) {
    std::ifstream input_file(filename);
    if (!input_file) {
        std::cerr << "Error: Unable to open input file." << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << "const char prog[] =\n";

    std::string line;
    while (std::getline(input_file, line)) {
        buffer << line;
    }
    buffer << ";";

    return buffer.str();
}
