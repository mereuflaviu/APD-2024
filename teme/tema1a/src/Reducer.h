#ifndef REDUCER_H
#define REDUCER_H

#include <vector>
#include <string>
#include <unordered_map>
#include <set>

class Reducer {
public:
    void processResults(char startLetter, char endLetter, 
                        const std::vector<std::unordered_map<std::string, std::set<int>>>& mapperResults);
};

#endif
