#include "kill_utils.h"
#include <gtest/gtest.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

// Private functions for testing purposes
namespace {

pid_t spawn_sleep_child() {
  // Make a full copy of the currently running process.
  // Note: The parent gets back the PID of the child as return value of fork().
  // The child gets back 0 as the return value of fork().
  // Thus this line executes twice in separate processes.
  //
  pid_t pid = fork();
  if (pid == 0) {
    // Stop running test program and become sleep program.
    execl("/bin/sleep", "sleep", "30", static_cast<char *>(nullptr));
    // Force shut down old program if still running test code.
    _exit(127);
  }

  // Original process will return the PID of the sleep process.
  return pid;
}

void cleanup_child(pid_t pid) {
  // Safety check.
  if (pid <= 0) {
    return;
  }

  // Check if process is already finished.
  // If process is still running, waitpid returns 0, otherwise, the PID.
  // Variable status if used to hold information about how process ended.
  int status = 0;
  // WNOHANG flag asks for immediate current situation of process
  pid_t result = waitpid(pid, &status, WNOHANG);

  // If process still running send kill signal to the process.
  if (result == 0) {
    kill(pid, SIGKILL);
    // Passing 0 waits until the process is finished.
    waitpid(pid, &status, 0);
  }
}

} // namespace

TEST(KillUtilsTests, KillProcessGroupKillsOnlyMatchingGroup) {
  pid_t target_1 = spawn_sleep_child();
  pid_t target_2 = spawn_sleep_child();
  pid_t other = spawn_sleep_child();

  // Check PIDS are greater than 0
  ASSERT_GT(target_1, 0);
  ASSERT_GT(target_2, 0);
  ASSERT_GT(other, 0);

  std::vector<Process> processes = {
      {"TargetApp", 10, target_1, ""},
      {"TargetApp", 12, target_2, ""},
      {"OtherApp", 8, other, ""},
  };

  // Should kill the processes with names "TargetApp".
  // Because if path is empty, it just uses the app name.
  kill_process_group("TargetApp", processes);

  int target_status_1 = 0;
  int target_status_2 = 0;
  // Should return PID, because process is done running.
  ASSERT_EQ(waitpid(target_1, &target_status_1, 0), target_1);
  ASSERT_EQ(waitpid(target_2, &target_status_2, 0), target_2);
  // WIFSIGNALED checks if process died from a signal (liked SIGKILL)
  EXPECT_TRUE(WIFSIGNALED(target_status_1));
  EXPECT_TRUE(WIFSIGNALED(target_status_2));

  // Then check if the signal itself was SIGKILL
  EXPECT_EQ(WTERMSIG(target_status_1), SIGKILL);
  EXPECT_EQ(WTERMSIG(target_status_2), SIGKILL);

  // Other should still be running, so clean it up
  int other_status = 0;
  EXPECT_EQ(waitpid(other, &other_status, WNOHANG), 0);
  cleanup_child(other);
}

TEST(KillUtilsTests, KillProcessGroupDoesNothingForMissingGroup) {
  pid_t child = spawn_sleep_child();
  ASSERT_GT(child, 0);

  std::vector<Process> processes = {
      {"SafeApp", 10, child, ""},
  };

  kill_process_group("DoesNotExist", processes);

  int status = 0;
  EXPECT_EQ(waitpid(child, &status, WNOHANG), 0);
  cleanup_child(child);
}
