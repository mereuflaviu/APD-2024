#include "QueueManager.h"


// primeste ca parametru un pointer la un mutex care este utilizat pentru a asigura sincronizarea accesului la coada
QueueManager::QueueManager(pthread_mutex_t* mutex) : mutex(mutex) {}

// Functia getNextFile extrage urmatorul fisier din coada 
bool QueueManager::getNextFile(std::queue<std::pair<std::string, int>>& queue,
                               std::pair<std::string, int>& fileInfo) {
    // Blocheaza mutexul pentru a preveni accesul simultan la coada
    pthread_mutex_lock(mutex);

    // Verificam daca coada este goala
    if (queue.empty()) {
        // Daca coada este goala, eliberam mutexul si returnam false
        pthread_mutex_unlock(mutex);
        return false;
    }

    // Extragem elementul din fata cozii si il stocam in fileInfo
    fileInfo = queue.front();
    queue.pop();  

    // Eliberam mutexul 
    pthread_mutex_unlock(mutex);

    
    return true;
}
