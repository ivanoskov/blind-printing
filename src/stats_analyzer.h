#pragma once
#include "console_handler.h"
#include <vector>
#include <string>
#include <chrono>
#include <functional>

struct SessionStats {
    std::string timestamp;
    double cpm;
    double accuracy;
    int errors;
    int total_chars;
    int duration;
    std::string text;
};

class StatsAnalyzer {
public:
    explicit StatsAnalyzer(ConsoleHandler& console);
    
    void displayStats(const std::string& language,
                     double current_cpm,
                     double current_accuracy,
                     int current_errors,
                     int current_chars,
                     std::chrono::seconds current_duration);

private:
    ConsoleHandler& console_;
    std::vector<SessionStats> loadStats(const std::string& language);
    
    void displaySpeedBarChart(const std::vector<SessionStats>& stats, 
                            double current_cpm, 
                            int y_offset);
    
    void displayFullStats(const std::vector<SessionStats>& stats,
                         double current_cpm,
                         double current_accuracy,
                         int current_errors,
                         int current_chars,
                         std::chrono::seconds current_duration);
    
    double calculateAverage(const std::vector<SessionStats>& stats,
                          const std::function<double(const SessionStats&)>& getter);
    std::string formatChange(double current, double average);
}; 