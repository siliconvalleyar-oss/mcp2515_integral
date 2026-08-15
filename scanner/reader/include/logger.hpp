#pragma once
#include <fstream>
#include <memory>
#include <string>

class Logger {
private:
    std::unique_ptr<std::ofstream> file;
    std::string filename;
public:
    Logger();
    ~Logger();
    bool open();
    void log(const std::string& key, const std::string& value);
    void log(const std::string& key, int value);
    void log(const std::string& key, float value);
    void log(const std::string& key, double value);
    void close();
};
