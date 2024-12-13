#include "stats_saver.h"
#include <filesystem>
#include <fstream>
#include <ctime>
#include <iomanip>

StatsSaver::StatsSaver()
{
    ensureDirectoryExists();
}

void StatsSaver::saveResult(const std::string &language,
                            double cpm,
                            double accuracy,
                            int errors,
                            int total_chars,
                            std::chrono::seconds duration,
                            const std::string &text)
{
    std::string filename = getStatsFilename(language);
    std::ofstream file(filename, std::ios::app);

    if (file.is_open())
    {
        file << getCurrentTimestamp() << ","
             << cpm << ","
             << accuracy << ","
             << errors << ","
             << total_chars << ","
             << duration.count() << ","
             << "\"" << text << "\"" // Текст в кавычках для CSV
             << std::endl;
    }
}

std::string StatsSaver::getStatsFilename(const std::string &language)
{
    return "stats/" + language + "_results.csv";
}

void StatsSaver::ensureDirectoryExists()
{
    std::filesystem::create_directory("stats");
}

std::string StatsSaver::getCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}