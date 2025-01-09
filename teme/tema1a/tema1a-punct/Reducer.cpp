#include "Reducer.h"
#include "SortingUtility.h"

// Functia processResults combina rezultatele din mai multi mapperi si le proceseaza
void Reducer::processResults(char startLetter, char endLetter, 
                             const std::vector<std::unordered_map<std::string, std::set<int>>>& mapperData) {
    //map pentru combinarea rezultatelor de la mapperi
    std::unordered_map<std::string, std::set<int>> aggregatedResults;

    // Parcurgem rezultatele mapperilor
    for (const auto& mapperOutput : mapperData) {
        for (const auto& entry : mapperOutput) {
            // Verificam daca prima litera este in intervalul dorit
            char firstChar = std::tolower(entry.first[0]);
            if (firstChar >= startLetter && firstChar <= endLetter) {
                aggregatedResults[entry.first].insert(entry.second.begin(), entry.second.end());
            }
        }
    }

    // Procesam rezultatele pentru fiecare litera din interval
    for (char currentLetter = startLetter; currentLetter <= endLetter; ++currentLetter) {
        auto filtered = SortingUtility::filterByLetter(aggregatedResults, currentLetter);  // filtrare dupa litera
        SortingUtility::sortResults(filtered);  // sortare dupa criterii
        SortingUtility::writeResultsToFile(filtered, std::string(1, currentLetter) + ".txt");  
    }
}
