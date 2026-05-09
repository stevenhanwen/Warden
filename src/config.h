#pragma once
#include <string>
#include <vector>

struct Config {
    long limit;
    std::vector<std::string> protected_processes;
};

// Takes a file .conf file path and reads for memory limits and protected processes.
Config load_config(const std::string& path);