// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include "./Logger.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cstdarg>
#include <string>
#include "./Utilities.h"
#include "./Clock.h"

using std::cout;
using boost::posix_time::ptime;
using boost::posix_time::second_clock;
using base::Clock;

typedef boost::unique_lock<boost::shared_mutex> WriteLock;
typedef boost::shared_lock<boost::shared_mutex> ReadLock;

const size_t Logger::BUFFER_SIZE = 512;

Logger::Logger()
    : path_(""), _stream(NULL), timer_counter_(0) , enabled_(true) {
  target("");
}

Logger::Logger(const Logger& other)
    : path_(""), _stream(NULL), timers_(other.timers_),
      timer_counter_(other.timer_counter_), enabled_(other.enabled_) {
  target(other.path_);
}

Logger::~Logger() {
  reset();
}

Logger& Logger::operator=(const Logger& other) {
  timers_ = other.timers_;
  timer_counter_ = other.timer_counter_;
  enabled_ = other.enabled_;
  target(other.path_);
  return *this;
}

bool Logger::enabled() const {
  return enabled_;
}

void Logger::enabled(const bool state) {
  enabled_ = state;
}

void Logger::reset() {
  if (path_ != "") {
    static_cast<ofstream*>(_stream)->close();
    delete _stream;
  }
}

void Logger::target(const string& path) {
  reset();
  path_ = path;
  if (path_ == "") {
    _stream = &cout;
  } else {
    _stream = new ofstream(path_.c_str(), std::ios::app);
    if (!static_cast<ofstream*>(_stream)->is_open()) {
      target("");
      error("unable to open " + path + " for logging");
    }
  }
}

string Logger::shorten(const string& text) const {
  string tex;
  if (text.size() > maxMessageLength()) {
    tex = text.substr(0, maxMessageLength() - 1);
  } else {
    tex = text;
  }
  return tex;
}

void Logger::debug(const char* format, ...) const {
#ifndef NDEBUG
  if (!enabled()) {
    return;
  }
  char buffer[BUFFER_SIZE];  // TODO(sawine): unsafe, what about longer strings?
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);
  ptime now = second_clock::local_time();
  *_stream << "[debug@" << now << "] " << buffer << "\n";
  _stream->flush();
#endif
}

void Logger::debug(const string& text) const {
#ifndef NDEBUG
  debug(shorten(text).c_str());
#endif
}

void Logger::info(const char* format, ...) const {
  if (!enabled()) {
    return;
  }
  char buffer[BUFFER_SIZE];
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);
  ptime now = second_clock::local_time();
  // *_stream << BROWN << "[ info@" << now << "] " << buffer << RESET << "\n";
  *_stream << "[ info@" << now << "] " << buffer << "\n";
  _stream->flush();
}

void Logger::info(const string& text) const {
  string shortened = shorten(text);
  info(shortened.c_str());
}

void Logger::error(const char* format, ...) const {
  if (!enabled()) {
    return;
  }
  char buffer[BUFFER_SIZE];
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);
  ptime now = second_clock::local_time();
  // *_stream << RED << BOLD << "[error@" << now << "] "
  // << buffer << RESET <<"\n";
  *_stream << "[error@" << now << "] " << buffer << "\n";
  _stream->flush();
}

void Logger::error(const string& text) const {
  error(shorten(text).c_str());
}

int Logger::beginPerf() {
  WriteLock(_mutex);
  const int id = timer_counter_++;
  timers_[id] = Clock();
  return id;
}

double Logger::endPerf(const int id, const string& text, const int iter) {
  double t = 0.0;
  {
    ReadLock(_mutex);
    t = (Clock() - timers_[id]) * Clock::kSecInMicro;
  }
  {
    WriteLock(_mutex);
    timers_.erase(id);
  }
  if (!enabled()) {
    return t;
  }
  if (text != "") {
    ptime now = second_clock::local_time();
    *_stream << "[ perf@" << now << "] ["
            << formatPerfTime(t);
    if (iter > 1) {
      *_stream << " | " << formatPerfTime(t / iter);
    }
    *_stream << "] " << text << "\n";
    _stream->flush();
  }
  return t;
}

int Logger::beginProg() {
  return beginPerf();
}

double Logger::prog(const int id, const int finished, const int total,
                    const string& text, const int numWorkers) {
  ptime now = second_clock::local_time();
  double t = 0.0;
  {
    ReadLock(_mutex);
    t = (Clock() - timers_[id]) * Clock::kSecInMicro;
  }
  double etc = t / finished * (total - finished) / numWorkers;
  if (!enabled()) {
    return etc;
  }
  *_stream << "[ prog@" << now << "] ["
           << finished << "/" << total
           << " | etc "
           << formatPerfTime(etc)
           << "] " << text << "\n";
  _stream->flush();
  return etc;
}

double Logger::endProg(const int id, const string& text) {
  return endPerf(id, text, 1);
}


size_t Logger::maxMessageLength() const {
  return BUFFER_SIZE;
}
