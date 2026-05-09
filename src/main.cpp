#include <iostream>
#include <unistd.h>
#include <signal.h>
#include "config.h"
#include "process.h"

int main(int argc, char* argv[]) {
    Config config = load_config("warden.conf");
    long limit = config.limit;
    bool watch_mode = false;
    bool kill_mode = false;

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (std::string(argv[i]) == "--watch") {
                watch_mode = true;
            } else if (std::string(argv[i]) == "--kill") {
                kill_mode = true;
            } else if (atol(argv[i]) > 0) {
                limit = atol(argv[i]);
            }
        }
    }

    if (watch_mode && kill_mode) {
        std::cout << "error: cannot use --watch and --kill together\n";
        return 1;
    }

    do {
        if (watch_mode) {
            system("clear");
            std::cout << "Warden — watching processes (limit: " << limit << "mb)\n";
            std::cout << "─────────────────────────────────────────\n";
        }

        std::vector<Process> processes = scan_processes(config.protected_processes);
        std::vector<std::pair<std::string, std::array<long, 2>>> groups = group_processes(processes);

        for (const auto& app_values : groups) {
            const std::string& app_name = app_values.first;
            const std::array<long, 2>& values = app_values.second;

            if (values[0] > limit) {
                std::cout << "⚠️  " << app_name << " — " << values[0] << " MB (" << values[1] << " processes)\n";
                if (kill_mode) {
                    std::cout << "Are you sure you want to kill " << app_name << "? (y/n): ";
                    char confirm;
                    std::cin >> confirm;
                    if (confirm == 'y') {
                        std::cout << "🔪 killing " << app_name << "\n";
                    } else {
                        std::cout << "skipping " << app_name << "\n";
                    }
                }
            } else {
                std::cout << "✅ " << app_name << " — " << values[0] << " MB (" << values[1] << " processes)\n";
            }
        }

        if (watch_mode) {
            std::cout << "─────────────────────────────────────────\n";
            std::cout << "updating every 3 seconds. press Ctrl+C to stop.\n";
            sleep(3);
        }

    } while (watch_mode);

    return 0;
}