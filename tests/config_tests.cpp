#include "config.h"
#include <gtest/gtest.h>

// Demonstrate some basic assertions.
TEST(ConfigTests, ReadingEmptyConfig) {
  Config empty_config = load_config(TEST_FIXTURE_DIR "/empty_config.conf");

  EXPECT_EQ(empty_config.limit, 500);
  EXPECT_EQ(empty_config.protected_processes.size(), 0);
}

TEST(ConfigTests, ReadingNoLimitConfig) {
  Config no_limit_config = load_config(TEST_FIXTURE_DIR "/no_limit_config.conf");

  // Check for default limit
  EXPECT_EQ(no_limit_config.limit, 500);

  const auto &protected_processes = no_limit_config.protected_processes;

  // Hard code the expected protected processes
  EXPECT_EQ(protected_processes.at(0), "Finder");
  EXPECT_EQ(protected_processes.at(1), "Terminal");
  EXPECT_EQ(protected_processes.at(2), "kernel_task");
  EXPECT_EQ(protected_processes.at(3), "launchd");
  EXPECT_EQ(protected_processes.at(4), "kextd");
}

TEST(ConfigTests, ReadingRegConfig) {
  Config reg_config = load_config(TEST_FIXTURE_DIR "/reg_config.conf");

  // Check for default limit
  EXPECT_EQ(reg_config.limit, 100);

  const auto &protected_processes = reg_config.protected_processes;

  // Hard code the expected protected processes
  EXPECT_EQ(protected_processes.at(0), "Finder");
  EXPECT_EQ(protected_processes.at(1), "Terminal");
  EXPECT_EQ(protected_processes.at(2), "kernel_task");
  EXPECT_EQ(protected_processes.at(3), "launchd");
  EXPECT_EQ(protected_processes.at(4), "kextd");
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
