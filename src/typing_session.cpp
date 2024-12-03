#include "typing_session.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>

TypingSession::TypingSession(TextProvider& provider, ConsoleHandler& console)
    : textProvider_(provider), console_(console) {}

void TypingSession::start() {
    while (true) {
        std::string text = textProvider_.getRandomText();
        std::wstring wtext;
        
        // Конвертируем текст в wide string один раз
        wchar_t* wstr = new wchar_t[text.length() * 4 + 1];
        size_t len = mbstowcs(wstr, text.c_str(), text.length() * 4);
        if (len != static_cast<size_t>(-1)) {
            wtext = std::wstring(wstr, len);
        }
        delete[] wstr;
        
        if (wtext.empty()) {
            console_.displayTextCentered("Ошибка преобразования текста", 0);
            return;
        }
        
        int errors = 0;
        int totalChars = wtext.length();
        
        console_.clearScreen();
        console_.displayTextCentered("=== Typing Trainer ===", -5);
        console_.displayTextCentered(text, 0);
        console_.displayTextCentered("Нажмите любую клавишу для начала или ESC для выхода...", 5);
        
        // Определяем начальную раскладку по первой букве текста
        bool is_russian = false;
        for (wchar_t c : wtext) {
            if (isRussianLetter(c)) {
                is_russian = true;
                break;
            } else if (isEnglishLetter(c)) {
                is_russian = false;
                break;
            }
        }
        
        // Отображаем клавиатуру до начала ввода
        displayKeyboard(wtext[0], is_russian);
        
        wint_t ch = console_.getChar();
        if (ch == 27 || ch == 'q' || ch == 'Q') {
            break;
        }
        
        console_.clearScreen();
        auto startTime = std::chrono::steady_clock::now();
        size_t currentPos = 0;
        
        // Получаем размеры экрана и вычисляем позицию текста
        auto [height, width] = console_.getScreenSize();
        int text_y = height / 2;
        int text_x = (width - wtext.length()) / 2;
        
        // Отображаем текст только один раз в начале
        if (currentPos == 0) {
            console_.moveCursor(text_y, text_x);
            console_.setColor(ConsoleHandler::COLOR_UNTYPED);
            console_.displayText(text);
        }
        
        while (currentPos < wtext.length()) {
            console_.moveCursor(text_y, text_x + currentPos);
            console_.setColor(ConsoleHandler::COLOR_CURRENT);
            
            // Получаем текущий символ как wide char
            wchar_t current_wchar = wtext[currentPos];
            std::string current_char = wcharToUtf8(current_wchar);
            console_.displayText(current_char);
            
            wint_t input = console_.getChar();
            
            // Проверяем специальные клавиши
            if (input == static_cast<wint_t>(27) || // ESC
                input == static_cast<wint_t>('q') || 
                input == static_cast<wint_t>('Q')) {
                return;
            }
            
            // Сравниваем символы с приведением типов
            if (static_cast<wchar_t>(input) == current_wchar) {
                // При правильном вводе
                console_.moveCursor(text_y, text_x + currentPos);
                console_.setColor(ConsoleHandler::COLOR_TYPED);
                console_.displayText(current_char);
                currentPos++;
                
                // Подсвечиваем следующий символ
                if (currentPos < wtext.length()) {
                    console_.moveCursor(text_y, text_x + currentPos);
                    console_.setColor(ConsoleHandler::COLOR_CURRENT);
                    console_.displayText(wcharToUtf8(wtext[currentPos]));
                }
            } else {
                // При ошибке подсвечиваем текущий символ красным
                console_.moveCursor(text_y, text_x + currentPos);
                console_.setColor(ConsoleHandler::COLOR_ERROR);
                console_.displayText(current_char);
                
                // Небольшая пауза для отображения ошибки
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                // Возвращаем подсветку текущего символа
                console_.moveCursor(text_y, text_x + currentPos);
                console_.setColor(ConsoleHandler::COLOR_CURRENT);
                console_.displayText(current_char);
                
                errors++;
            }
            
            displayRealtimeStats(errors, totalChars, startTime, currentPos);
            // Передаем текущую раскладку в displayKeyboard
            displayKeyboard(wtext[currentPos], is_russian);
        }
        
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
        
        displayStats(errors, totalChars, duration);
        
        console_.displayTextCentered("Нажмите ENTER для продолжения или ESC/Q для выхода...", 5);
        wint_t choice;
        do {
            choice = console_.getChar();
        } while (choice != '\n' && choice != 27 && choice != 'q' && choice != 'Q');
        
        if (choice == 27 || choice == 'q' || choice == 'Q') {
            break;
        }
    }
}

void TypingSession::displayRealtimeStats(int errors, int totalChars, 
                                       std::chrono::steady_clock::time_point startTime,
                                       size_t currentPos) {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
    
    double current_cpm = calculateCurrentCPM(currentPos, startTime);
    double accuracy = calculateAccuracy(errors, currentPos > 0 ? currentPos : 1);
    int progress = static_cast<int>((currentPos * 100.0) / totalChars);
    
    std::string stats = 
        "Скорость: " + std::to_string(current_cpm).substr(0, std::to_string(current_cpm).find(".") + 2) + " сим/мин | " +
        "Точность: " + std::to_string(accuracy).substr(0, std::to_string(accuracy).find(".") + 2) + "% | " +
        "Ошибки: " + std::to_string(errors) + " | " +
        "Прогресс: " + std::to_string(progress) + "%";
    
    console_.displayTextCentered(stats, 5);
}

void TypingSession::displayErrorChar(int y, int x, char expected) {
    console_.moveCursor(y, x);
    console_.setColor(ConsoleHandler::COLOR_ERROR);
    console_.displayText(std::string(1, expected), false);
    console_.resetColor();
}

double TypingSession::calculateCurrentCPM(int chars, const std::chrono::steady_clock::time_point& startTime) {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
    double minutes = duration.count() / 60.0;
    if (minutes < 0.0001) return 0.0;
    return chars / minutes;
}

void TypingSession::displayStats(int errors, int totalChars, std::chrono::seconds duration) {
    console_.clearScreen();
    
    double cpm = calculateCPM(totalChars, duration);
    double accuracy = calculateAccuracy(errors, totalChars);
    
    std::vector<std::string> stats = {
        "Результаты:",
        "Скорость: " + std::to_string(cpm).substr(0, std::to_string(cpm).find(".") + 2) + " символов в минуту",
        "Точность: " + std::to_string(accuracy).substr(0, std::to_string(accuracy).find(".") + 2) + "%",
        "Ошибки: " + std::to_string(errors),
        "Время: " + std::to_string(duration.count()) + " секунд"
    };
    
    // Отображаем статистику с одинаковым отступом
    const int startY = -4;
    for (size_t i = 0; i < stats.size(); ++i) {
        console_.displayTextCentered(stats[i], startY + i);
    }
}

double TypingSession::calculateCPM(int totalChars, std::chrono::seconds duration) {
    double minutes = duration.count() / 60.0;
    if (minutes < 0.0001) return 0.0;
    return totalChars / minutes;
}

double TypingSession::calculateAccuracy(int errors, int totalChars) {
    return 100.0 * (1.0 - static_cast<double>(errors) / totalChars);
}

// Добавляем вспомогательную функцию для конвертации wchar_t в UTF-8 string
std::string TypingSession::wcharToUtf8(wchar_t wc) {
    char buf[MB_CUR_MAX + 1];
    int len = wctomb(buf, wc);
    if (len > 0) {
        return std::string(buf, len);
    }
    return std::string(1, '?');
}

// Добавляем вспомогательные функции для определения типа символа
bool TypingSession::isRussianLetter(wchar_t c) {
    return (c >= L'А' && c <= L'я') || c == L'Ё' || c == L'ё';
}

bool TypingSession::isEnglishLetter(wchar_t c) {
    return (c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z');
}

void TypingSession::displayKeyboard(wchar_t currentChar, bool is_russian) {
    // Определяем раскладки с фиксированными позициями
    const std::vector<std::pair<std::string, int>> ru_row1 = {
        {"Й",2}, {"Ц",6}, {"У",10}, {"К",14}, {"Е",18}, {"Н",22}, {"Г",26}, {"Ш",30}, {"Щ",34}, {"З",38}, {"Х",42}, {"Ъ",46}
    };
    const std::vector<std::pair<std::string, int>> ru_row2 = {
        {"Ф",4}, {"Ы",8}, {"В",12}, {"А",16}, {"П",20}, {"Р",24}, {"О",28}, {"Л",32}, {"Д",36}, {"Ж",40}, {"Э",44}
    };
    const std::vector<std::pair<std::string, int>> ru_row3 = {
        {"Я",6}, {"Ч",10}, {"С",14}, {"М",18}, {"И",22}, {"Т",26}, {"Ь",30}, {"Б",34}, {"Ю",38}
    };
    
    const std::vector<std::pair<std::string, int>> en_row1 = {
        {"Q",2}, {"W",6}, {"E",10}, {"R",14}, {"T",18}, {"Y",22}, {"U",26}, {"I",30}, {"O",34}, {"P",38}, {"[",42}, {"]",46}
    };
    const std::vector<std::pair<std::string, int>> en_row2 = {
        {"A",4}, {"S",8}, {"D",12}, {"F",16}, {"G",20}, {"H",24}, {"J",28}, {"K",32}, {"L",36}, {";",40}, {"'",44}
    };
    const std::vector<std::pair<std::string, int>> en_row3 = {
        {"Z",6}, {"X",10}, {"C",14}, {"V",18}, {"B",22}, {"N",26}, {"M",30}, {",",34}, {".",38}
    };
    
    auto& row1 = is_russian ? ru_row1 : en_row1;
    auto& row2 = is_russian ? ru_row2 : en_row2;
    auto& row3 = is_russian ? ru_row3 : en_row3;
    
    // Получаем позицию для отображения клавиатуры
    auto [height, width] = console_.getScreenSize();
    int keyboard_y = height - 8;
    int start_x = (width - 50) / 2;
    
    // Рисуем рамку со скругленными углами
    console_.setColor(ConsoleHandler::COLOR_UNTYPED);
    console_.moveCursor(keyboard_y, start_x);
    console_.displayText("╭────────────────────────────────────────────────╮");
    
    for (int i = 1; i < 4; i++) {
        console_.moveCursor(keyboard_y + i, start_x);
        console_.displayText("│");
        console_.moveCursor(keyboard_y + i, start_x + 49);
        console_.displayText("│");
    }
    
    console_.moveCursor(keyboard_y + 4, start_x);
    console_.displayText("╰────────────────────────────────────────────────╯");
    
    // Функция для отрисовки ряда клавиш
    auto drawRow = [&](const std::vector<std::pair<std::string, int>>& row, int y) {
        for (const auto& [key, x] : row) {
            console_.moveCursor(y, start_x + x);
            console_.displayText(key);
        }
    };
    
    // Отрисовываем все ряды клавиш
    console_.setColor(ConsoleHandler::COLOR_UNTYPED);
    drawRow(row1, keyboard_y + 1);
    drawRow(row2, keyboard_y + 2);
    drawRow(row3, keyboard_y + 3);
    
    // Подсвечиваем текущую клавишу
    std::string current = wcharToUtf8(std::towupper(currentChar));
    for (const auto& rows : {row1, row2, row3}) {
        for (const auto& [key, x] : rows) {
            if (key == current) {
                int y = keyboard_y + (rows == row1 ? 1 : rows == row2 ? 2 : 3);
                console_.moveCursor(y, start_x + x);
                console_.setColor(ConsoleHandler::COLOR_CURRENT);
                console_.displayText(key);
                break;
            }
        }
    }
    
    // Отображаем индикатор раскладки
    std::string layout_indicator = is_russian ? "[RU]" : "[EN]";
    console_.setColor(ConsoleHandler::COLOR_UNTYPED);
    console_.moveCursor(keyboard_y + 5, (width - layout_indicator.length()) / 2);
    console_.displayText(layout_indicator);
    
    console_.resetColor();
} 