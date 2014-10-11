/* Copyright 2014 Peter Goodman, all rights reserved. */

#include <chrono>
#include <ctime>

__thread std::chrono::time_point<std::chrono::system_clock> start, end;

extern "C" void StartClock(void) {
  start = std::chrono::system_clock::now();
}

extern "C" void EndClock(void) {
  end = std::chrono::system_clock::now();

}

extern "C" double GetElapsedTime(void) {
  std::chrono::duration<double> elapsed_seconds = end-start;
  return elapsed_seconds.count();
}
