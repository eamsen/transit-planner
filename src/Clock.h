// Copyright 2012 Eugen Sawin <sawine@me73.com>
#ifndef SRC_CLOCK_H_
#define SRC_CLOCK_H_

#include <cstdint>
#include <ctime>
#include <string>
#include <sstream>

namespace base {

class Clock {
 public:
  typedef int64_t Diff;

  enum Type {
    kProcessClock = CLOCK_PROCESS_CPUTIME_ID,
    kThreadClock = CLOCK_THREAD_CPUTIME_ID
  };

  static const Diff kSecInMin = 60;
  static const Diff kMilliInSec = 1000;
  static const Diff kMicroInMilli = 1000;
  static const Diff kNanoInMicro = 1000;
  static const Diff kMicroInSec = kMilliInSec * kMicroInMilli;
  static const Diff kMicroInMin = kMicroInSec * kSecInMin;
  static constexpr double kMilliInMicro = 1.0 / kMicroInMilli;
  static constexpr double kMicroInNano = 1.0 / kNanoInMicro;
  static constexpr double kSecInMicro = 1.0 / kMicroInSec;
  static constexpr double kMinInMicro = 1.0 / kMicroInMin;

  Clock() {
    clock_gettime(kProcessClock, &time_);
  }

  explicit Clock(const Type type) {
    clock_gettime(type, &time_);
  }

  Diff operator-(const Clock& rhs) const {
    return (time_.tv_sec - rhs.time_.tv_sec) * kMicroInSec +
           (time_.tv_nsec - rhs.time_.tv_nsec) * kMicroInNano;
  }

  // Returns the time duration between given times in microseconds.
  static Diff Duration(const Clock& beg, const Clock& end) {
    return end - beg;
  }

  // Returns the string representation of the given time difference.
  static std::string DiffStr(const Diff& diff) {
    std::stringstream ss;
    ss.setf(std::ios::fixed, std::ios::floatfield);
    ss.precision(2);
    if (diff >= kMicroInMin) {
      const double min = diff * kMinInMicro;
      ss << min << "min";
    } else if (diff >= kMicroInSec) {
      const double sec = diff * kSecInMicro;
      ss << sec << "s";
    } else if (diff >= kMicroInMilli) {
      const double milli = diff * kMilliInMicro;
      ss << milli << "ms";
    } else {
      ss << diff << "µs";
    }
    return ss.str();
  }

  // Returns the system time resolution.
  // Remark: Usually returns 0µs (1ns), this is however a bad promise and does
  // not reflect the (dynamic) underlying clock event resolution.
  static Diff Resolution() {
    timespec res;
    clock_getres(CLOCK_MONOTONIC, &res);
    return res.tv_sec * kMicroInSec + res.tv_nsec * kMicroInNano;
  }


 private:
  timespec time_;
};

}  // namespace base

#endif  // SRC_CLOCK_H_

