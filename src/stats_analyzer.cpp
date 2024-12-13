#include "stats_analyzer.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>

StatsAnalyzer::StatsAnalyzer(ConsoleHandler& console) : console_(console) {}

void StatsAnalyzer::displayStats(const std::string& language,
                               double current_cpm,
                               double current_accuracy,
                               int current_errors,
                               int current_chars,
                               std::chrono::seconds current_duration) {
    auto stats = loadStats(language);
    
    console_.clearScreen();
    console_.displayTextCentered("=== Результаты ===", -12);
    
    // Отображаем столбчатую диаграмму скорости с большим отступом
    displaySpeedBarChart(stats, current_cpm, -4);
    
    // Отображаем полную статистику внизу
    displayFullStats(stats, current_cpm, current_accuracy, current_errors, 
                    current_chars, current_duration);
}

std::vector<SessionStats> StatsAnalyzer::loadStats(const std::string& language) {
    std::vector<SessionStats> stats;
    std::string filename = "stats/" + language + "_results.csv";
    std::ifstream file(filename);
    
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        SessionStats stat;
        
        std::string field;
        std::getline(ss, stat.timestamp, ',');
        std::getline(ss, field, ','); stat.cpm = std::stod(field);
        std::getline(ss, field, ','); stat.accuracy = std::stod(field);
        std::getline(ss, field, ','); stat.errors = std::stoi(field);
        std::getline(ss, field, ','); stat.total_chars = std::stoi(field);
        std::getline(ss, field, ','); stat.duration = std::stoi(field);
        std::getline(ss, stat.text);
        
        stats.push_back(stat);
    }
    
    return stats;
}

void StatsAnalyzer::displaySpeedBarChart(const std::vector<SessionStats>& stats, 
                                       double current_cpm, int y_offset) {
    auto [height, width] = console_.getScreenSize();
    int chart_width = width / 2;
    int chart_height = 8;  // Уменьшаем высоту графика
    
    // Берем последние N результатов для отображения
    const int max_bars = chart_width;  // Каждый столбец шириной 1 символ
    int start_idx = std::max(0, static_cast<int>(stats.size()) - max_bars);
    std::vector<double> speeds;
    for (size_t i = start_idx; i < stats.size(); ++i) {
        speeds.push_back(stats[i].cpm);
    }
    speeds.push_back(current_cpm);
    
    // Находим максимальную скорость для масштабирования
    double max_speed = *std::max_element(speeds.begin(), speeds.end());
    
    // Отображаем заголовок
    console_.displayTextCentered("История скорости печати", y_offset);
    console_.displayTextCentered("(последние " + std::to_string(speeds.size()) + " сессий, макс. " + 
                                std::to_string(static_cast<int>(max_speed)) + " сим/мин)", y_offset + 1);
    
    // Рисуем столбцы
    int start_x = (width - speeds.size()) / 2;
    for (size_t i = 0; i < speeds.size(); ++i) {
        int bar_height = static_cast<int>((speeds[i] / max_speed) * chart_height);
        
        // Рисуем столбец
        for (int h = 0; h < bar_height; ++h) {
            console_.moveCursor(height/2 + y_offset + chart_height - h, start_x + i);
            console_.setColor(i == speeds.size()-1 ? ConsoleHandler::COLOR_CURRENT 
                                                 : ConsoleHandler::COLOR_TYPED);
            console_.displayText("█");
        }
    }
}

void StatsAnalyzer::displayFullStats(const std::vector<SessionStats>& stats,
                                   double current_cpm,
                                   double current_accuracy,
                                   int current_errors,
                                   int current_chars,
                                   std::chrono::seconds current_duration) {
    auto avg_cpm = calculateAverage(stats, [](const SessionStats& s) { return s.cpm; });
    auto avg_accuracy = calculateAverage(stats, [](const SessionStats& s) { return s.accuracy; });
    auto avg_errors = calculateAverage(stats, [](const SessionStats& s) { return static_cast<double>(s.errors); });
    
    std::vector<std::string> stat_lines = {
        "Текущий результат:",
        "  Скорость: " + std::to_string(static_cast<int>(current_cpm)) + " сим/мин " + 
            formatChange(current_cpm, avg_cpm),
        "  Точность: " + std::to_string(static_cast<int>(current_accuracy)) + "% " + 
            formatChange(current_accuracy, avg_accuracy),
        "  Ошибки: " + std::to_string(current_errors) + " " + 
            formatChange(-current_errors, -avg_errors),
        "  Символов: " + std::to_string(current_chars),
        "  Время: " + std::to_string(current_duration.count()) + " сек",
        "",
        "Средние показатели:",
        "  Скорость: " + std::to_string(static_cast<int>(avg_cpm)) + " сим/мин",
        "  Точность: " + std::to_string(static_cast<int>(avg_accuracy)) + "%",
        "  Ошибки: " + std::to_string(static_cast<int>(avg_errors)),
        "  Всего сессий: " + std::to_string(stats.size() + 1)
    };
    
    auto [height, width] = console_.getScreenSize();
    int start_y = height/2 + 8;  // Увеличиваем отступ снизу
    
    for (size_t i = 0; i < stat_lines.size(); ++i) {
        console_.setColor(ConsoleHandler::COLOR_UNTYPED);
        console_.displayTextCentered(stat_lines[i], start_y + i);
    }
    
    // Добавляем сообщение о продолжении с большим отступом
    console_.displayTextCentered("Нажмите ENTER для продолжения или ESC/Q для выхода...", start_y + stat_lines.size() + 2);
}

std::string StatsAnalyzer::formatChange(double current, double average) {
    if (average == 0) return "";
    
    double change = ((current - average) / average) * 100;
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1);
    
    if (change > 0) {
        ss << "(+" << change << "%)";
        return "\033[32m" + ss.str() + "\033[0m";  // Зеленый цвет для улучшения
    } else if (change < 0) {
        ss << "(" << change << "%)";
        return "\033[31m" + ss.str() + "\033[0m";  // Красный цвет для ухудшения
    }
    
    return "";
}

double StatsAnalyzer::calculateAverage(const std::vector<SessionStats>& stats,
                                     const std::function<double(const SessionStats&)>& getter) {
    if (stats.empty()) return 0.0;
    
    double sum = 0.0;
    for (const auto& stat : stats) {
        sum += getter(stat);
    }
    return sum / stats.size();
} 