// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#ifndef SRC_LOGGER_H_
#define SRC_LOGGER_H_

#include <boost/thread/shared_mutex.hpp>
#include <string>
#include <fstream>
#include <iostream>
#include <map>
#include "./Clock.h"

using std::string;
using std::ofstream;
using std::ostream;
using std::map;

class Logger {
 public:
  Logger();
  Logger(const Logger& other);
  ~Logger();

  Logger& operator=(const Logger& other);

  // Returns wether the logging is enabled.
  bool enabled() const;

  // Sets the logging state.
  void enabled(const bool state);

  // Sets the logger target file.
  // Empty ("") path redirects output to stdout.
  void target(const std::string& path);

  // Resets the logger target to stdout.
  void reset();

  // Shortens a message text to the maximum buffer size.
  string shorten(const string& text) const;

  // Logs a debug message.
  void debug(const std::string& text) const;
  void debug(const char* format, ...) const;

  // Logs a runtime info message.
  void info(const std::string& text) const;
  void info(const char* format, ...) const;

  // Logs a runtime error message.
  void error(const std::string& text) const;
  void error(const char* format, ...) const;

  // Returns a timer id used for performance measurements.
  // Starts the timer.
  int beginPerf();

  // Logs a performance message. The elapsed time since the call to
  // beginPerformance is measured for the given timer id.
  // Setting iter will additionally log the average time per iteration.
  // Returns the total elapsed time in seconds.
  double endPerf(const int id, const string& text = "", const int iter = 1);

  // Returns a timer id used for progress measurements.
  // Starts the timer.
  int beginProg();

  // Logs a progress message.
  // Returns the estimated time to complete in seconds.
  double prog(const int id, const int finished, const int total,
              const string& text, const int numWorkers = 1);

  // Logs the final progress message.
  // Returns the total elapsed time in seconds.
  double endProg(const int id, const string& text);

  // Returns the maximum length of a message. Depends on the buffer size for
  // vsnprintf.
  size_t maxMessageLength() const;

 private:
  string path_;
  ostream* _stream;
  map<int, base::Clock> timers_;
  int timer_counter_;
  bool enabled_;
  boost::shared_mutex _mutex;
  static const size_t BUFFER_SIZE;
};

// The global instance of the logger.
static Logger LOG;

#endif  // SRC_LOGGER_H_
