#include "config.h"
#include "kill_utils.h"
#include "process.h"
#include <chrono>
#include <ncurses.h>
#include <unistd.h>

int main() {
  initscr();
  noecho();             // doesnt print keypresses
  cbreak();             // makes keypresses instant without needing enter
  keypad(stdscr, true); // allows for arrow keys
  timeout(100); // Controls how long getch() waits for a keypress before giving up and returning -1.
  Config config = load_config("warden.conf");
  long limit = config.limit;

  int selected = 0; // currently selected item

  // Initial scan before loop
  auto last_refresh = std::chrono::steady_clock::now();
  auto groups = group_processes(scan_processes(config.protected_processes));

  while (true) {
    // Only scan the processes every 5 seconds instead of every press of a key
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_refresh).count();

    if (elapsed >= 5) {
      groups = group_processes(scan_processes(config.protected_processes));
      last_refresh = now;
    }

    clear(); // wipe the canvas

    // calculate available rows
    const int HEADER_ROWS = 3;
    const int FOOTER_ROWS = 4;
    int available_rows = LINES - HEADER_ROWS - FOOTER_ROWS;

    // calculate scroll offset
    // Only scroll when selected moves outside the visible window
    static int scroll_offset = 0;

    if (selected < scroll_offset) {
      scroll_offset = selected;
    } else if (selected >= scroll_offset + available_rows) {
      scroll_offset = selected - available_rows + 1;
    }

    std::string line(COLS, '-');
    mvprintw(0, 0, "Number of Running Procceses: %d", static_cast<int>(groups.size()));
    mvprintw(1, 0, "Warden - Select a Process:");
    mvprintw(2, 0, "%s", line.c_str());

    int app_index = 0;

    for (const auto &app_values : groups) {

      // This would skip indices of the groups until reaches scroll_offset

      if (app_index < scroll_offset) {
        app_index++;
        continue;
      }
      // stop if we've filled the screen
      // EX: 7th app would be the last to be shown if 6 available rows and scroll_offset = 1

      if (app_index - scroll_offset >= available_rows) {
        break;
      }

      const std::string &app_name = app_values.name;
      const long &total_mb = app_values.total_mb;
      const int &num_processes = app_values.num_processes;

      std::string line_text = app_name + " " + std::to_string(total_mb) + " MB (" +
                              std::to_string(num_processes) + " processes)";

      if (total_mb > limit) {
        line_text += " Warning, process group over configured limit";
      }

      // EX: app_index is 1 (second app) and scroll_offset = 1
      // 1 - 1 + 2 = 2
      int display_row = app_index - scroll_offset + HEADER_ROWS;

      if (app_index == selected) {
        attron(A_REVERSE);
        mvprintw(display_row, 0, "-> %s", line_text.c_str());
        attroff(A_REVERSE);
        mvprintw(LINES - 3, 0, "Currently Selected Process: %s", app_name.c_str());
      } else {
        mvprintw(display_row, 0, line_text.c_str());
      }
      app_index++;
    }

    mvprintw(LINES - FOOTER_ROWS, 0, "%s", line.c_str());

    mvprintw(LINES - 2, 0, "%s", line.c_str());
    mvprintw(LINES - 1, 0, "[up/down] navigate  [k] kill  [q] quit");

    refresh(); // Prints to canvas

    int key = getch();

    if (key == 'q') {
      break;
    }
    if (key == KEY_UP && selected > 0) {
      selected--;
    }
    if (key == KEY_DOWN && selected < (int)groups.size() - 1) {
      selected++;
    }

    // Confirmation prompt for kill action

    if (key == 'k') {
      // Show confirmation prompt
      mvprintw(LINES - 3, 0, "Are you sure you want to kill this group? (y/n) ");
      refresh();
      // Set getch to blocking mode
      timeout(-1);
      int confirm = getch();
      // Restore timeout to 100ms
      timeout(100);

      mvprintw(LINES - 3, 0, "%*s", COLS, ""); // Overwrite old text with spaces
      if (confirm == 'y' || confirm == 'Y') {
        kill_process_group(groups[selected].name, scan_processes(config.protected_processes));
        mvprintw(LINES - 3, 0, "Process group killed!");
      } else {
        mvprintw(LINES - 3, 0, "Kill cancelled.");
      }
      refresh();
      // Wait briefly so user can see the result
      napms(1000);
    }
  }

  endwin();
  return 0;
}
