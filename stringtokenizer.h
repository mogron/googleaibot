#ifndef STRINGTOKENIZER_H
#define STRINGTOKENIZER_H

#include <vector>
#include <string>

class StringTokenizer
{
public:
    static void Tokenize(const std::string& stringToSplit, char deliminator, std::vector<std::string>& tokens);
    static std::vector<std::string> Tokenize(const std::string& stringToSplit, char deliminator);

private:
    StringTokenizer();
};

#endif // STRINGTOKENIZER_H
