#include "TaskManager.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <num_mappers> <num_reducers> <input_file>\n";
        return 1;
    }

    int numMappers = std::stoi(argv[1]);
    int numReducers = std::stoi(argv[2]);
    std::string inputFile = argv[3];

    TaskManager taskManager(numMappers, numReducers, inputFile);
    taskManager.execute();

    return 0;
}
