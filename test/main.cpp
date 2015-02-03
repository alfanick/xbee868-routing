#include <iostream>
#include <gtest/gtest.h>

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);

  int gtestResult = RUN_ALL_TESTS();

  std::cout << "Gtest result: " << gtestResult << "\n";
  return gtestResult;
}
