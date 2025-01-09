#include "WordProcessor.h" 
#include <cctype>          

// Functia normalizeWord normalizeaza un cuvant prin eliminarea caracterelor (., etc) si transforma literele in litere mici
std::string WordProcessor::normalizeWord(const std::string& word) {
    std::string normalized; // string pentru normalizare

    // parcurgem fiecare caracter din cuvantul de intrare
    for (char c : word) {
        if (isLetter(c)) { // Verificam daca caracterul este o litera 
            normalized += toLower(c); // Daca este litera, o transformam in litera mica 
        }
    }

    return normalized; 
}

// Functia `isLetter` verifica daca un caracter este o litera.
bool WordProcessor::isLetter(char c) {
    // verifica daca un caracter este o litera
    return std::isalpha(static_cast<unsigned char>(c));
}

// functia toLower transforma un caracter in litera mica.
char WordProcessor::toLower(char c) {
    // transforma un caracter in litera mica.
    return std::tolower(static_cast<unsigned char>(c));
}
