#pragma once
#include <string>
#include <vector>

class TextProvider {
public:
    explicit TextProvider(const std::string& filename);
    std::string getRandomText(size_t minLength = 50);

private:
    std::vector<std::string> texts_;
    void loadTexts(const std::string& filename);
}; 