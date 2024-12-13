#pragma once
#include "console_handler.h"
#include <vector>
#include <string>

class MenuHandler
{
public:
    explicit MenuHandler(ConsoleHandler &console);

    // Возвращает выбранный файл с текстами
    std::string showLanguageMenu();

private:
    ConsoleHandler &console_;
    std::vector<std::pair<std::string, std::string>> menu_items_; // <Отображаемое имя, путь к файлу>

    void loadAvailableLanguages();
    void displayMenu(size_t selected_index);
    void highlightItem(const std::string &item, bool is_selected, int y_offset);
    std::string getDisplayName(const std::string &filename);
};