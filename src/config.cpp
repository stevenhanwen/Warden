#include "config.h"
#include <fstream>

Config load_config(const std::string &path) {
  Config config;
  config.limit = 500; // default limit of 500mb

  // read config file
  std::ifstream file(path);
  if (file.is_open()) {
    std::string line;
    while (std::getline(file, line)) {
      // searches for "limit=" starting at position 0
      // rfind returns the position in which it was found,
      // which will be 0, otherwise returns npos
      if (line.rfind("limit=", 0) == 0) {
        config.limit = atol(line.substr(6).c_str());
      }
      if (line.rfind("protect=", 0) == 0) {
        config.protected_processes.push_back(line.substr(8));
      }
    }
  }
  return config;
}
