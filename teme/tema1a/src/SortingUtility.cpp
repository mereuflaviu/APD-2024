#include "SortingUtility.h"
#include <algorithm>
#include <fstream>
#include <iostream>

// Functia filterByLetter filtreaza cuvintele care incep cu o anumita litera
std::vector<std::pair<std::string, std::set<int>>> SortingUtility::filterByLetter(
    const std::unordered_map<std::string, std::set<int>>& inputResults, char letter) {
    // Vector pentru stocarea rezultatelor filtrate
    std::vector<std::pair<std::string, std::set<int>>> matches;

    //parcurgem fiecare cuvant si verificam prima litera
    for (const auto& result : inputResults) {
        if (std::tolower(result.first[0]) == letter) {
            matches.emplace_back(result);  // Adaugam cuvantul in rezultatele filtrate.
        }
    }
    return matches;  
}

// Functia sortResults sorteaza rezultatele filtrate dupa anumite criterii
void SortingUtility::sortResults(std::vector<std::pair<std::string, std::set<int>>>& results) {
    //sortam vectorul
    std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) {
        // sortez dupa dimensiunea setului (descrescator)
        if (a.second.size() != b.second.size()) {
            return a.second.size() > b.second.size();
        }
        // daca dimensiunea este egala sortam alfabetic
        return a.first < b.first;
    });
}

// Functia writeResultsToFile scrie rezultatele intr-un fisier
void SortingUtility::writeResultsToFile(const std::vector<std::pair<std::string, std::set<int>>>& results,
                                        const std::string& fileName) {
    // deschidem fisierul pentru scriere
    std::ofstream outputFile(fileName);
    if (!outputFile.is_open()) {  // verificare
        std::cerr << "Cannot open file: " << fileName << "\n"; 
        return;
    }

    // Scriem fiecare cuvant si ID-urile asociate
    for (const auto& [word, fileIds] : results) {
        outputFile << word << ":[";
        for (auto it = fileIds.begin(); it != fileIds.end(); ++it) {
            outputFile << *it;  // Scriem ID-ul fisierului
            if (std::next(it) != fileIds.end()) {  // Adaugam un spatiu intre ID-uri
                outputFile << " ";
            }
        }
        outputFile << "]\n"; 
    }
    outputFile.close(); 
}
