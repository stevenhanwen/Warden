#pragma once

#include "process.h"
#include <string>
#include <vector>

// Kills all processes in the given group name (base name, as used in group_processes)
void kill_process_group(const std::string &group_name, const std::vector<Process> &processes);
