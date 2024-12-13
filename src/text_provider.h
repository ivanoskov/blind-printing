#pragma once
#include <string>
#include <vector>

class TextProvider
{
public:
    explicit TextProvider(const std::string &filename);
    std::string getRandomText();
    static std::string getLanguageFromFile(const std::string &filename);

private:
    std::vector<std::string> texts_;
    void loadTexts(const std::string &filename);
};