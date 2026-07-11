#include "process.h"
#include <cstddef>
#include <gtest/gtest.h>
#include <string>

TEST(TestTrimProcessName, TestNoTrim) {
  std::string app_name = "Warden";
  std::string trimmed_warden = trim_process_name(app_name);
  EXPECT_EQ(trimmed_warden, "Warden");

  std::string empty = "";
  std::string trimmed_empty = trim_process_name(empty);
  EXPECT_EQ(trimmed_empty, "");
}

TEST(TestTrimProcessName, TestRegularTrim) {
  std::string name_with_space = "Code Helper (Renderer)";
  std::string trimmed_name = trim_process_name(name_with_space);
  EXPECT_EQ(trimmed_name, "Code Helper");

  std::string name_without_space = "Obsidian (GPU)";
  trimmed_name = trim_process_name(name_without_space);
  EXPECT_EQ(trimmed_name, "Obsidian");
}

TEST(TestSearchProcessGroup, TestEmpty) {
  ProcessGroupVec empty;
  std::string empty_string = "";
  ProcessGroupVec result = search_processes(empty_string, empty);
  EXPECT_EQ(result, empty);
}

TEST(TestSearchProcessGroup, TestRegularSearch) {
  ProcessGroupVec app_groups;
  app_groups.push_back({"Claude", {100, 5}});
  app_groups.push_back({"Google", {50, 3}});
  app_groups.push_back({"GitHub", {200, 7}});
  app_groups.push_back({"Neovim", {25, 1}});

  std::string search_term = "Cl";
  ProcessGroupVec result = search_processes(search_term, app_groups);
  ProcessGroupVec expected = {{"Claude", {100, 5}}};
}
