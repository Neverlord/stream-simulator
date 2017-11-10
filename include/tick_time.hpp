#ifndef TICK_TIME_HPP
#define TICK_TIME_HPP

#include <chrono>

/// A timestamp value for the simulated clock.
using tick_time = int;

/// A duration type for tick time.
using tick_duration = int;

/// The resolution of a single tick in realtime.
using tick_resolution = std::chrono::microseconds;

/// Retrns a resolution in tick time as seconds.
constexpr double to_seconds(tick_duration x) {
  return std::chrono::duration<double>{tick_resolution{x}}.count();
};

#endif // TICK_TIME_HPP
