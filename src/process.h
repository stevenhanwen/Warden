#pragma once
#include <array>
#include <string>
#include <vector>

using ProcessGroup = std::vector<std::pair<std::string, std::array<long, 2>>>;

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
// Excludes processes which are protected, so they cannot be deleted.
std::vector<Process> scan_processes(const std::vector<std::string> &protected_processes);

// Returns the group name for a process (used for grouping and killing)
// The root app of all the underlying processes.
std::string group_name_for_process(const Process &process);

// Groups processes by their app name and sums their memory
// usage and counts the number of processes in each group.
// Returns a vector of pairs where the first element is the app name and the
// second element is an array [total_mb, num_processes], sorted by total_mb in
// descending order.
ProcessGroup group_processes(const std::vector<Process> &processes);

// A function that takes in the vector of groups processes
// and searches for any processes that begins with the substring
// RETURNS: vector of pair<std::string, std::array<long, 2>>
// in sorted order such that the earlier the search string appears
// (the index of position), the earlier in the vector it will be.
ProcessGroup search_processes(std::string &search,
                              std::vector<std::pair<std::string, std::array<long, 2>>> &groups);
