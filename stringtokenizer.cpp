#include "stringtokenizer.h"

#include <sstream>

void StringTokenizer::Tokenize(const std::string& stringToSplit, char deliminator, std::vector<std::string>& tokens)
{
    std::stringstream ss(stringToSplit);
    std::string item;
    while(std::getline(ss, item, deliminator)) {
        tokens.push_back(item);
    }
}


std::vector<std::string> StringTokenizer::Tokenize(const std::string& stringToSplit, char deliminator)
{
    std::vector<std::string> tokens;
    Tokenize(stringToSplit, deliminator, tokens);
    return tokens;
}
