#include "TaskManager.h"
#include "QueueManager.h"
#include <cmath>
#include <iostream>
#include <fstream>

// initializare managerul de taskuri
TaskManager::TaskManager(int mappers, int reducers, const std::string& inputFile)
    : numMappers(mappers), numReducers(reducers), inputFile(inputFile), queueMutex(PTHREAD_MUTEX_INITIALIZER) {}

// functie de executie
void TaskManager::execute() {
    readInput();  // citim fisierele de intrare

    // lansez thread-urile mapper
    std::vector<pthread_t> mapperThreads(numMappers);
    std::vector<ThreadData> mapperData(numMappers);

    for (int i = 0; i < numMappers; ++i) {
        mapperData[i] = {i, &fileQueue, &mapperResults, &queueMutex, numReducers};
        pthread_create(&mapperThreads[i], nullptr, runMapper, &mapperData[i]);
    }

    // astept ca thread-urile mapper sa termine
    for (int i = 0; i < numMappers; ++i) {
        pthread_join(mapperThreads[i], nullptr);
    }

    // lansez thread-urile reducer
    std::vector<pthread_t> reducerThreads(numReducers);
    std::vector<ThreadData> reducerData(numReducers);

    for (int i = 0; i < numReducers; ++i) {
        reducerData[i] = {i, nullptr, &mapperResults, nullptr, numReducers};
        pthread_create(&reducerThreads[i], nullptr, runReducer, &reducerData[i]);
    }

    // astept ca thread-urile reducer sa termine
    for (int i = 0; i < numReducers; ++i) {
        pthread_join(reducerThreads[i], nullptr);
    }
}

// citesc datele din fisierul de intrare
void TaskManager::readInput() {
    std::ifstream infile(inputFile);
    if (!infile.is_open()) {
        std::cerr << "Error opening input file\n";
        return;
    }

    int numFiles;
    infile >> numFiles;

    for (int i = 1; i <= numFiles; ++i) {
        std::string fileName;
        infile >> fileName;
        fileQueue.push({fileName, i});  // adaug fisierul si ID-ul in coada
    }

    mapperResults.resize(numMappers);  
    infile.close();
}

// Functie pentru executia unui thread mapper
void* TaskManager::runMapper(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    Mapper mapper;
    QueueManager queueManager(data->queueMutex);

    while (true) {
        std::pair<std::string, int> fileInfo;
        if (!queueManager.getNextFile(*data->fileQueue, fileInfo)) {
            break;  // Iesim daca nu mai sunt fisiere.
        }
        mapper.processFile(fileInfo.first, fileInfo.second, (*data->mapperResults)[data->id]);
    }

    return nullptr;
}

// Functie pentru executia unui thread reducer.
void* TaskManager::runReducer(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    Reducer reducer;

    int lettersPerReducer = std::ceil(26.0 / data->numReducers);  // Calculam intervalul de litere.
    char startLetter = 'a' + data->id * lettersPerReducer;
    char endLetter = std::min((char)('a' + (data->id + 1) * lettersPerReducer - 1), 'z');

    reducer.processResults(startLetter, endLetter, *data->mapperResults);

    return nullptr;
}
