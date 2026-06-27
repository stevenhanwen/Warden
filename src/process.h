#pragma once
#include <array>
#include <map>
#include <string>
#include <vector>

struct Process {
  std::string name;
  long mb;
  int pid;
  std::string exe_path;
};

struct AppGroup {
  std::string name;
  long total_mb;
  int num_processes;
};

// Returns a vector of processes sorted
std::vector<Process>
scan_processes(const std::vector<std::string> &protected_processes);

// Returns the group name for a process (used for grouping and killing)
// The root app of all the underlying processes.
std::string
group_name_for_process(const Process &process);

std::vector<std::pair<std::string, std::array<long, 2>>>
group_processes(const std::vector<Process> &processes);

// A function that takes in the vector of groups processes
// and searches for the any processes that begins with the substring
// RETURNS: vector of pair<std::string, std::array<long, 2>>
std::vector<std::pair<std::string, std::array<long, 2>>>
search_processes(std::string &search,
                 std::vector<std::pair<std::string, std::array<long, 2>>> &groups);
