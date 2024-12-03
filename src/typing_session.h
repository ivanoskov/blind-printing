#pragma once
#include "text_provider.h"
#include "console_handler.h"
#include <chrono>
#include <string>

class TypingSession {
public:
    TypingSession(TextProvider& provider, ConsoleHandler& console);
    void start();

private:
    TextProvider& textProvider_;
    ConsoleHandler& console_;
    
    void displayRealtimeStats(int errors, int totalChars, 
                            std::chrono::steady_clock::time_point startTime,
                            size_t currentPos);
    void displayErrorChar(int y, int x, char expected);
    void displayStats(int errors, int totalChars, 
                     std::chrono::seconds duration);
    double calculateWPM(int totalChars, std::chrono::seconds duration);
    double calculateAccuracy(int errors, int totalChars);
    double calculateCurrentWPM(int chars, const std::chrono::steady_clock::time_point& startTime);
    std::string wcharToUtf8(wchar_t wc);
}; 