#ifndef SORTINGUTILITY_H
#define SORTINGUTILITY_H

#include <string>
#include <set>
#include <vector>
#include <unordered_map>

class SortingUtility {
public:
    static std::vector<std::pair<std::string, std::set<int>>> filterByLetter(
        const std::unordered_map<std::string, std::set<int>>& results, char letter);

    static void sortResults(std::vector<std::pair<std::string, std::set<int>>>& results);

    static void writeResultsToFile(
        const std::vector<std::pair<std::string, std::set<int>>>& results, const std::string& filename);
};

#endif
