#pragma once
#include <string>
#include <vector>
#include <array>
#include <map>

struct Process {
    std::string name;
    long mb;
    int pid;
};

struct AppGroup {
    std::string name;
    long total_mb;
    int num_processes;
};

std::vector<Process> scan_processes(const std::vector<std::string>& protected_processes);
std::vector<std::pair<std::string, std::array<long, 2>>> group_processes(const std::vector<Process>& processes);