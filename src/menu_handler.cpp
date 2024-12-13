#include "menu_handler.h"
#include <filesystem>
#include <algorithm>

MenuHandler::MenuHandler(ConsoleHandler &console) : console_(console)
{
    loadAvailableLanguages();
}

void MenuHandler::loadAvailableLanguages()
{
    menu_items_.clear();

    // Сканируем директорию data
    for (const auto &entry : std::filesystem::directory_iterator("data"))
    {
        if (entry.path().extension() == ".txt")
        {
            std::string filepath = entry.path().string();
            std::string display_name = getDisplayName(entry.path().stem().string());
            menu_items_.push_back({display_name, filepath});
        }
    }

    // Сортируем языки по алфавиту
    std::sort(menu_items_.begin(), menu_items_.end());

    if (menu_items_.empty())
    {
        throw std::runtime_error("No text files found in data directory");
    }
}

std::string MenuHandler::getDisplayName(const std::string &filename)
{
    // Первая буква заглавная, остальные строчные
    std::string display = filename;
    if (!display.empty())
    {
        display[0] = std::toupper(display[0]);
        for (size_t i = 1; i < display.length(); ++i)
        {
            display[i] = std::tolower(display[i]);
        }
    }
    return display;
}

std::string MenuHandler::showLanguageMenu()
{
    size_t selected = 0;

    while (true)
    {
        console_.clearScreen();
        console_.displayTextCentered("=== Typing Trainer ===", -5);
        console_.displayTextCentered("Choose language:", -2);

        displayMenu(selected);

        wint_t key = console_.getChar();
        switch (key)
        {
        case KEY_UP:
        case 'k':
            if (selected > 0)
                selected--;
            break;
        case KEY_DOWN:
        case 'j':
            if (selected < menu_items_.size() - 1)
                selected++;
            break;
        case '\n':
        case ' ':
            return menu_items_[selected].second;
        case 27: // ESC
            return "";
        }
    }
}

void MenuHandler::displayMenu(size_t selected_index)
{
    for (size_t i = 0; i < menu_items_.size(); ++i)
    {
        highlightItem(menu_items_[i].first, i == selected_index, i);
    }
}

void MenuHandler::highlightItem(const std::string &item, bool is_selected, int y_offset)
{
    console_.setColor(is_selected ? ConsoleHandler::COLOR_CURRENT : ConsoleHandler::COLOR_UNTYPED);
    console_.displayTextCentered(
        (is_selected ? "> " : "  ") + item + (is_selected ? " <" : "  "),
        y_offset);
    console_.resetColor();
}