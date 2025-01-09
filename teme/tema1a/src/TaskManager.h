#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <vector>
#include <string>
#include <queue>
#include <unordered_map>
#include <set>
#include <pthread.h>
#include "Mapper.h"
#include "Reducer.h"

struct ThreadData {
    int id;
    std::queue<std::pair<std::string, int>>* fileQueue;
    std::vector<std::unordered_map<std::string, std::set<int>>>* mapperResults;
    pthread_mutex_t* queueMutex;
    int numReducers;
};

class TaskManager {
public:
    TaskManager(int mappers, int reducers, const std::string& inputFile);
    void execute();

private:
    int numMappers;
    int numReducers;
    std::string inputFile;
    std::queue<std::pair<std::string, int>> fileQueue;
    std::vector<std::unordered_map<std::string, std::set<int>>> mapperResults;
    pthread_mutex_t queueMutex;

    void readInput();
    static void* runMapper(void* arg);
    static void* runReducer(void* arg);
};

#endif
