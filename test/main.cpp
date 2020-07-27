#include "timer.h"

#include <chrono>
#include <iostream>

int main(void) {
  std::cout << "Starting..." << std::endl;

  Timer timer([]() {
    auto now = std::chrono::system_clock::now();
    std::cout << "Tick " << std::chrono::system_clock::to_time_t(now)
              << std::endl;
  });
  timer.SetTime(std::chrono::seconds(1), std::chrono::seconds(1));

  Timer::Dispatch();

  return 0;
}
