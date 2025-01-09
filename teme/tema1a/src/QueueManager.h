#ifndef QUEUEMANAGER_H
#define QUEUEMANAGER_H

#include <queue>
#include <string>
#include <pthread.h>

class QueueManager {
public:
    explicit QueueManager(pthread_mutex_t* mutex);
    bool getNextFile(std::queue<std::pair<std::string, int>>& queue, std::pair<std::string, int>& fileInfo);

private:
    pthread_mutex_t* mutex;
};

#endif
