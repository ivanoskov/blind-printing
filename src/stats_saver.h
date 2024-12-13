#pragma once
#include <string>
#include <chrono>

class StatsSaver
{
public:
    StatsSaver();

    void saveResult(const std::string &language,
                    double cpm,
                    double accuracy,
                    int errors,
                    int total_chars,
                    std::chrono::seconds duration,
                    const std::string &text);

private:
    std::string getStatsFilename(const std::string &language);
    void ensureDirectoryExists();
    std::string getCurrentTimestamp();
};