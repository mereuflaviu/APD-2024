#include "Mapper.h"
#include "WordProcessor.h"
#include <fstream>
#include <iostream>

// Functia processFile proceseaza un fisier dat pentru a genera un map intre cuvintele normalizate si ID-ul fisierului
void Mapper::processFile(const std::string& filename, int fileId, 
                         std::unordered_map<std::string, std::set<int>>& results) {
    // Deschidem fisierul pentru citire
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {  // Verificare
        std::cerr << "Unable to open file: " << filename << "\n";  //  eroare daca fisierul nu poate fi deschis
        return;
    }

    // variabila pentru stocarea fiecarui cuvant din fisier
    std::string currentWord;

    // Citim fiecare cuvant din fisier
    while (inputFile >> currentWord) {
        // Normalizam cuvantul folosind clasa WordProcessor
        auto cleanWord = WordProcessor::normalizeWord(currentWord);
        if (!cleanWord.empty()) {  
            // Daca cuvantul nu exista in map il adaugam
            if (results.find(cleanWord) == results.end()) {
                results[cleanWord] = {fileId};  // Adaugam cuvantul impreuna cu ID-ul fisierulu
            } else {
                results[cleanWord].insert(fileId);  // Adaugam ID-ul fisierului la lista existenta
            }
        }
    }
    inputFile.close();  
}
