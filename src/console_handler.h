#pragma once
#include <string>
#define _XOPEN_SOURCE_EXTENDED 1
#include <ncurses.h>

class ConsoleHandler
{
public:
    // Определения типов текста
    static const int COLOR_TYPED = 1;   // Набранный текст
    static const int COLOR_CURRENT = 2; // Текущий символ
    static const int COLOR_ERROR = 3;   // Ошибки
    static const int COLOR_UNTYPED = 4; // Ненабранный текст

private:
    // Цветовые пары для ncurses
    static const int COLOR_PAIR_DEFAULT = 1;
    static const int COLOR_PAIR_TYPED = 2;
    static const int COLOR_PAIR_CURRENT = 3;
    static const int COLOR_PAIR_ERROR = 4;
    static const int COLOR_PAIR_UNTYPED = 5;

public:
    ConsoleHandler();
    ~ConsoleHandler();

    void clearScreen();
    void displayText(const std::string &text, bool highlight = false);
    void displayTextCentered(const std::string &text, int y_offset = 0);
    wint_t getChar();
    void setColor(int color);
    void resetColor();
    std::pair<int, int> getScreenSize();
    void moveCursor(int y, int x);

private:
    void initializeConsole();
    void restoreConsole();
    int screen_height_;
    int screen_width_;
};