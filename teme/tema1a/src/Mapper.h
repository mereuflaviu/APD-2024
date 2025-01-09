#ifndef MAPPER_H
#define MAPPER_H

#include <string>
#include <unordered_map>
#include <set>

class Mapper {
public:
    void processFile(const std::string& filename, int fileId, 
                     std::unordered_map<std::string, std::set<int>>& results);
};

#endif
