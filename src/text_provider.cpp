#include "text_provider.h"
#include <fstream>
#include <random>
#include <stdexcept>

TextProvider::TextProvider(const std::string& filename) {
    loadTexts(filename);
}

void TextProvider::loadTexts(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Не удалось открыть файл с текстами");
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            texts_.push_back(line);
        }
    }

    if (texts_.empty()) {
        throw std::runtime_error("Файл с текстами пуст");
    }
}

std::string TextProvider::getRandomText(size_t minLength) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, texts_.size() - 1);

    return texts_[dis(gen)];
} 