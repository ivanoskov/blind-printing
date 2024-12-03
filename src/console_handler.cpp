#include "console_handler.h"
#include <ncurses.h>
#include <clocale>
#include <cstring>

ConsoleHandler::ConsoleHandler() {
    initializeConsole();
}

ConsoleHandler::~ConsoleHandler() {
    restoreConsole();
}

void ConsoleHandler::initializeConsole() {
    std::setlocale(LC_ALL, "");
    std::setlocale(LC_CTYPE, "en_US.UTF-8");
    
    initscr();              
    raw();                  
    keypad(stdscr, TRUE);   
    noecho();              
    start_color();         
    curs_set(0);          
    
    getmaxyx(stdscr, screen_height_, screen_width_);
    
    use_default_colors();
    
    init_pair(COLOR_PAIR_DEFAULT, COLOR_WHITE, -1);
    init_pair(COLOR_PAIR_TYPED, COLOR_GREEN, -1);
    init_pair(COLOR_PAIR_CURRENT, COLOR_WHITE, -1);
    init_pair(COLOR_PAIR_ERROR, COLOR_RED, -1);
    init_pair(COLOR_PAIR_UNTYPED, 8, -1);
    
    attron(COLOR_PAIR(COLOR_PAIR_DEFAULT));
    refresh();
}

void ConsoleHandler::restoreConsole() {
    endwin();
}

void ConsoleHandler::clearScreen() {
    clear();
    refresh();
}

void ConsoleHandler::displayText(const std::string& text, bool /* highlight */) {
    // Преобразуем строку в широкие символы для корректного отображения UTF-8
    wchar_t* wstr = new wchar_t[text.length() * 2 + 1];
    mbstowcs(wstr, text.c_str(), text.length() * 2);
    addwstr(wstr);
    delete[] wstr;
    refresh();
}

void ConsoleHandler::displayTextCentered(const std::string& text, int y_offset) {
    // Преобразуем строку в широкие символы
    wchar_t* wstr = new wchar_t[text.length() * 2 + 1];
    size_t len = mbstowcs(wstr, text.c_str(), text.length() * 2);
    
    // Получаем реальную длину строки в терминале
    int display_width = wcswidth(wstr, len);
    if (display_width < 0) display_width = len; // Если не удалось определить ширину
    
    int x = (screen_width_ - display_width) / 2;
    if (x < 0) x = 0;
    
    int y = (screen_height_ / 2) + y_offset;
    if (y < 0) y = 0;
    if (y >= screen_height_) y = screen_height_ - 1;
    
    move(y, x);
    addwstr(wstr);
    delete[] wstr;
    refresh();
}

wint_t ConsoleHandler::getChar() {
    wint_t ch;
    get_wch(&ch);
    return ch;
}

void ConsoleHandler::setColor(int color) {
    attroff(A_COLOR); // Сбрасываем текущий цвет
    
    switch(color) {
        case COLOR_TYPED:
            attron(COLOR_PAIR(COLOR_PAIR_TYPED));
            break;
        case COLOR_CURRENT:
            attron(COLOR_PAIR(COLOR_PAIR_CURRENT) | A_BOLD);
            break;
        case COLOR_ERROR:
            attron(COLOR_PAIR(COLOR_PAIR_ERROR) | A_BOLD);
            break;
        case COLOR_UNTYPED:
            attron(COLOR_PAIR(COLOR_PAIR_UNTYPED));
            break;
        default:
            attron(COLOR_PAIR(COLOR_PAIR_DEFAULT));
    }
    refresh();
}

void ConsoleHandler::resetColor() {
    attroff(A_COLOR | A_BOLD);
    attron(COLOR_PAIR(COLOR_PAIR_DEFAULT));
    refresh();
}

std::pair<int, int> ConsoleHandler::getScreenSize() {
    return {screen_height_, screen_width_};
}

void ConsoleHandler::moveCursor(int y, int x) {
    move(y, x);
    refresh();
} 