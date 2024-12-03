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