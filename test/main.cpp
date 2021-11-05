#include "gtest/gtest.h"

namespace {

TEST(Scanner, Placeholder) {
  EXPECT_EQ(1, true);
}

} // namespace

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}