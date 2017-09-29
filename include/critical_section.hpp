#ifndef CRITICAL_SECTION_HPP
#define CRITICAL_SECTION_HPP

#include <mutex>

template <class F>
auto critical_section(std::mutex& mx, F f)
  -> decltype(f(std::declval<std::unique_lock<std::mutex>&>())) {
  std::unique_lock<std::mutex> guard{mx};
  return f(guard);
}

template <class F>
auto critical_section(std::mutex& mx, F f) -> decltype(f()) {
  std::unique_lock<std::mutex> guard{mx};
  return f();
}

#endif // CRITICAL_SECTION_HPP
