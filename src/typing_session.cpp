#include "typing_session.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include "stats_saver.h"
#include "stats_analyzer.h"

TypingSession::TypingSession(TextProvider &provider, ConsoleHandler &console, const std::string &language)
    : textProvider_(provider), console_(console), language_(language) {}

void TypingSession::start()
{
    while (true)
    {
        text_ = textProvider_.getRandomText();
        std::wstring wtext;

        // Конвертируем текст в wide string один раз
        wchar_t *wstr = new wchar_t[text_.length() * 4 + 1];
        size_t len = mbstowcs(wstr, text_.c_str(), text_.length() * 4);
        if (len != static_cast<size_t>(-1))
        {
            wtext = std::wstring(wstr, len);
        }
        delete[] wstr;

        if (wtext.empty())
        {
            console_.displayTextCentered("Ошибка преобразования текста", 0);
            return;
        }

        int errors = 0;
        int totalChars = wtext.length();

        console_.clearScreen();
        console_.displayTextCentered("=== Typing Trainer ===", -5);
        console_.displayTextCentered(text_, 0);
        console_.displayTextCentered("Нажмите любую клавишу для начала или ESC для выхода...", 5);

        // Определяем начальную раскладку по первой букве текста
        bool is_russian = false;
        for (wchar_t c : wtext)
        {
            if (isRussianLetter(c))
            {
                is_russian = true;
                break;
            }
            else if (isEnglishLetter(c))
            {
                is_russian = false;
                break;
            }
        }

        wint_t ch = console_.getChar();
        if (ch == 27 || ch == 'q' || ch == 'Q')
        {
            break;
        }

        console_.clearScreen();
        auto startTime = std::chrono::steady_clock::now();
        size_t currentPos = 0;

        // Получаем размеры экрана и вычисляем позицию текста
        auto [height, width] = console_.getScreenSize();
        int text_y = height / 2;
        int text_x = (width - wtext.length()) / 2;

        // Проверяем, была ли первая буква правильной
        if (static_cast<wchar_t>(ch) == wtext[0])
        {
            console_.moveCursor(text_y, text_x);
            console_.setColor(ConsoleHandler::COLOR_TYPED);
            console_.displayText(wcharToUtf8(wtext[0]));
            currentPos = 1;
        }

        // Отображаем текст и клавиатуру после начала
        console_.moveCursor(text_y, text_x + currentPos);
        console_.setColor(ConsoleHandler::COLOR_UNTYPED);
        console_.displayText(text_.substr(currentPos));
        displayKeyboard(wtext[currentPos], is_russian);

        while (currentPos < wtext.length())
        {
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
                input == static_cast<wint_t>('Q'))
            {
                return;
            }

            // Сравниваем символы с приведением типов
            if (static_cast<wchar_t>(input) == current_wchar)
            {
                // При правильном вводе
                console_.moveCursor(text_y, text_x + currentPos);
                console_.setColor(ConsoleHandler::COLOR_TYPED);
                console_.displayText(current_char);
                currentPos++;

                // Подсвечиваем следующий символ
                if (currentPos < wtext.length())
                {
                    console_.moveCursor(text_y, text_x + currentPos);
                    console_.setColor(ConsoleHandler::COLOR_CURRENT);
                    console_.displayText(wcharToUtf8(wtext[currentPos]));
                }
            }
            else
            {
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
            displayKeyboard(wtext[currentPos], is_russian);
        }

        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

        displayStats(errors, totalChars, duration);

        console_.displayTextCentered("Нажмите ENTER для продолжения или ESC/Q для выхода...", 5);
        wint_t choice;
        do
        {
            choice = console_.getChar();
        } while (choice != '\n' && choice != 27 && choice != 'q' && choice != 'Q');

        if (choice == 27)
        {
            break;
        }
    }
}

void TypingSession::displayRealtimeStats(int errors, int totalChars,
                                         std::chrono::steady_clock::time_point startTime,
                                         size_t currentPos)
{
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

    auto [height, width] = console_.getScreenSize();
    int stats_y = height - 6;

    // Очищаем всю строку перед выводом новой статистики
    console_.moveCursor(stats_y, 0);
    console_.setColor(ConsoleHandler::COLOR_UNTYPED);
    console_.displayText(std::string(width, ' '));

    // Выводим новую статистику
    console_.displayTextCentered(stats, height - 6);
    console_.resetColor();
}

void TypingSession::displayErrorChar(int y, int x, char expected)
{
    console_.moveCursor(y, x);
    console_.setColor(ConsoleHandler::COLOR_ERROR);
    console_.displayText(std::string(1, expected), false);
    console_.resetColor();
}

double TypingSession::calculateCurrentCPM(int chars, const std::chrono::steady_clock::time_point &startTime)
{
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
    double minutes = duration.count() / 60.0;
    if (minutes < 0.0001)
        return 0.0;
    return chars / minutes;
}

void TypingSession::displayStats(int errors, int totalChars, std::chrono::seconds duration)
{
    double cpm = calculateCPM(totalChars, duration);
    double accuracy = calculateAccuracy(errors, totalChars);
    
    // Сохраняем результаты (только здесь!)
    StatsSaver stats_saver;
    stats_saver.saveResult(
        language_,
        cpm,
        accuracy,
        errors,
        totalChars,
        duration,
        text_
    );
    
    // Показываем текущую статистику
    console_.clearScreen();
    std::vector<std::string> stats = {
        "Результаты:",
        "Скорость: " + std::to_string(static_cast<int>(cpm)) + " символов в минуту",
        "Точность: " + std::to_string(static_cast<int>(accuracy)) + "%",
        "Ошибки: " + std::to_string(errors),
        "Время: " + std::to_string(duration.count()) + " секунд"
    };
    
    int startY = -15;  // Увеличиваем отступ сверху
    for (const auto& line : stats) {
        console_.displayTextCentered(line, startY++);
    }
    
    // Отображаем историю и сравнительную статистику
    StatsAnalyzer analyzer(console_);
    analyzer.displayStats(
        language_,
        cpm,
        accuracy,
        errors,
        totalChars,
        duration
    );
}

double TypingSession::calculateCPM(int totalChars, std::chrono::seconds duration)
{
    double minutes = duration.count() / 60.0;
    if (minutes < 0.0001)
        return 0.0;
    return totalChars / minutes;
}

double TypingSession::calculateAccuracy(int errors, int totalChars)
{
    int limited_errors = std::min(errors, totalChars);
    return 100.0 * (1.0 - static_cast<double>(limited_errors) / totalChars);
}

// Добавляем вспомогательную функцию для конвертации wchar_t в UTF-8 string
std::string TypingSession::wcharToUtf8(wchar_t wc)
{
    char buf[MB_CUR_MAX + 1];
    int len = wctomb(buf, wc);
    if (len > 0)
    {
        return std::string(buf, len);
    }
    return std::string(1, '?');
}

// Добавляем вспомогательные функции для определения типа символа
bool TypingSession::isRussianLetter(wchar_t c)
{
    return (c >= L'А' && c <= L'я') || c == L'Ё' || c == L'ё';
}

bool TypingSession::isEnglishLetter(wchar_t c)
{
    return (c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z');
}

void TypingSession::displayKeyboard(wchar_t currentChar, bool is_russian)
{
    // Определяем раскладки с фиксированными позициями
    const std::vector<std::pair<std::string, int>> ru_row1 = {
        {"Й", 3}, {"Ц", 6}, {"У", 9}, {"К", 12}, {"Е", 15}, {"Н", 18}, {"Г", 21}, {"Ш", 24}, {"Щ", 27}, {"З", 30}, {"Х", 33}, {"Ъ", 36}};
    const std::vector<std::pair<std::string, int>> ru_row2 = {
        {"Ф", 5}, {"Ы", 8}, {"В", 11}, {"А", 14}, {"П", 17}, {"Р", 20}, {"О", 23}, {"Л", 26}, {"Д", 29}, {"Ж", 32}, {"Э", 35}};
    const std::vector<std::pair<std::string, int>> ru_row3 = {
        {"Я", 7}, {"Ч", 10}, {"С", 13}, {"М", 16}, {"И", 19}, {"Т", 22}, {"Ь", 25}, {"Б", 28}, {"Ю", 31}, {".", 34}};

    const std::vector<std::pair<std::string, int>> en_row1 = {
        {"Q", 3}, {"W", 6}, {"E", 9}, {"R", 12}, {"T", 15}, {"Y", 18}, {"U", 21}, {"I", 24}, {"O", 27}, {"P", 30}, {"[", 33}, {"]", 36}};
    const std::vector<std::pair<std::string, int>> en_row2 = {
        {"A", 5}, {"S", 8}, {"D", 11}, {"F", 14}, {"G", 17}, {"H", 20}, {"J", 23}, {"K", 26}, {"L", 29}, {";", 32}, {"'", 35}};
    const std::vector<std::pair<std::string, int>> en_row3 = {
        {"Z", 7}, {"X", 10}, {"C", 13}, {"V", 16}, {"B", 19}, {"N", 22}, {"M", 25}, {",", 28}, {".", 31}, {"/", 34}};

    auto &row1 = is_russian ? ru_row1 : en_row1;
    auto &row2 = is_russian ? ru_row2 : en_row2;
    auto &row3 = is_russian ? ru_row3 : en_row3;

    // Получаем позицию для отображения клавиатуры
    auto [height, width] = console_.getScreenSize();
    int keyboard_y = height - 10;

    // Увеличиваем ширину рамки
    int frame_width = 44;
    int start_x = (width - frame_width) / 2;

    // Рисуем рамку со скругленными углами
    console_.setColor(ConsoleHandler::COLOR_UNTYPED);
    console_.moveCursor(keyboard_y, start_x);
    console_.displayText("╭──────────────────────────────────────────╮");

    for (int i = 1; i < 4; i++)
    {
        console_.moveCursor(keyboard_y + i, start_x);
        console_.displayText("│");
        console_.moveCursor(keyboard_y + i, start_x + frame_width - 1);
        console_.displayText("│");
    }

    console_.moveCursor(keyboard_y + 4, start_x);
    console_.displayText("╰──────────────────────────────────────────╯");

    // Функция для отрисовки ряда клавиш
    auto drawRow = [&](const std::vector<std::pair<std::string, int>> &row, int y)
    {
        for (const auto &[key, x] : row)
        {
            console_.moveCursor(y, start_x + x + 2);
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

    for (const auto &rows : {row1, row2, row3})
    {
        for (const auto &[key, x] : rows)
        {
            if (key == current)
            {
                int y = keyboard_y + (rows == row1 ? 1 : rows == row2 ? 2
                                                                      : 3);
                console_.moveCursor(y, start_x + x + 2);
                console_.setColor(ConsoleHandler::COLOR_CURRENT);
                console_.displayText(key);
                break;
            }
        }
    }

    console_.resetColor();
}