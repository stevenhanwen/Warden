#include <iostream>
#include <unistd.h>
#include <signal.h>
#include "config.h"
#include "process.h"
#include <ncurses.h>
#include <chrono>



int main(int argc, char* argv[]) {
    initscr();
    noecho(); // doesnt print keypresses
    cbreak(); // makes keypresses instant without needing enter
    keypad(stdscr, true); // allows for arrow keys
    timeout(100); // Controls how long getch() waits for a keypress before giving up and returning -1.
    Config config = load_config("warden.conf");
    long limit = config.limit;

    int selected = 0;  // currently selected item

    // Initial scan before loop
    auto last_refresh = std::chrono::steady_clock::now();
    auto groups = group_processes(scan_processes(config.protected_processes));

    while (true) {
        // Only scan every 5 seconds instead of every press of a key
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_refresh).count();
        if (elapsed >= 5) {
            groups = group_processes(scan_processes(config.protected_processes));
            last_refresh = now;
        }

        clear(); // wipe the canvas

        std::string line(COLS, '-');
        mvprintw(0, 0, "Warden - Select a Process:");
        mvprintw(1, 0, "%s", line.c_str());

        int app_index = 0; 
        for (const auto& app_values : groups) {
            const std::string& app_name = app_values.first;
            const std::array<long, 2>& values = app_values.second;

            std::string line_text = app_name + " " + 
                                    std::to_string(values[0]) + " MB (" + 
                                    std::to_string(values[1]) + " processes)";

            
            if (values[0] > limit) {
                line_text += " Warning, process group over configured limit";
            }

            if (app_index == selected) {
                attron(A_REVERSE);
                mvprintw(app_index + 2, 0, line_text.c_str());
                attroff(A_REVERSE);
            } else {
                mvprintw(app_index + 2, 0, line_text.c_str());
            }

            app_index++; 
        }

        mvprintw(groups.size() + 3, 0, "%s", line.c_str());
        mvprintw(groups.size() + 4, 0, "[up/down] navigate  [q] quit");

        refresh(); // Prints to canvas

        int key = getch();
        if (key == 'q') break;
        if (key == KEY_UP && selected > 0) selected--;
        if (key == KEY_DOWN && selected < (int)groups.size() - 1) selected++;
    }

    endwin();
    return 0;
}