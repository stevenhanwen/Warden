#include <iostream>
#include <libproc.h>
#include <fstream>
#include <string>
#include <vector>


int main(int argc, char* argv[]) {
    long limit = 500; // default limit of 500mb

    // read config file
    std::ifstream config("warden.conf");
    std::vector<std::string> protected_processes;
    if (config.is_open()) {
        std::string line;
        while (std::getline(config, line)) {
            // searches for "limit=" starting at position 0
            // rfind returns the position in which it was found,
            // which will be 0, otherwise returns npos
            if (line.rfind("limit=", 0) == 0) { 
                limit = atol(line.substr(6).c_str());
            }
            if (line.rfind("protect=", 0) == 0) { 
                protected_processes.push_back(line.substr(8)); 
            }
        }
    }

    if (argc > 1) {
        // use the argument if provided and convert to a long 
        limit = atol(argv[1]); 
    }

    int pids[1024];
    int count = proc_listallpids(pids, sizeof(pids));

    for (int i = 0; i < count; i++) {
        struct proc_taskinfo info;
        char name[64];

        proc_name(pids[i], name, sizeof(name));
        int res = proc_pidinfo(pids[i], PROC_PIDTASKINFO, 0, &info, sizeof(info));

        bool is_protected = false; 
        for (auto& p : protected_processes) {
            if (p == name) {
                is_protected = true;
                break;
            }
        }

        if (res > 0 && !is_protected) {
            long mb = info.pti_resident_size / 1024 / 1024;
            if (mb > 0) {
                if (mb > limit) {
                    std::cout << "⚠️  " << name << " is using too lots of memory: " << mb << " MB\n";
                } else {
                    std::cout << "✅ " << name << " — " << mb << " MB\n";
                }
            }
        }
    }
}