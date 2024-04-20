#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    std::ifstream input_file(argv[1]);
    if (!input_file) {
        std::cerr << "Error: Unable to open input file." << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << "const char prog[] =\n";

    std::string line;
    while (std::getline(input_file, line)) {
        buffer << "    \"" << line << "\\n\"\n";
    }

    buffer << ";";

    std::cout << buffer.str() << std::endl;

    return 0;
}
