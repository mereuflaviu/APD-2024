#ifndef WORDPROCESSOR_H
#define WORDPROCESSOR_H

#include <string>

class WordProcessor {
public:
    static std::string normalizeWord(const std::string& word);

private:
    static bool isLetter(char c);
    static char toLower(char c);
};

#endif
