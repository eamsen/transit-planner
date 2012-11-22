// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/local_time/local_date_time.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include "./Utilities.h"

using std::string;
using std::ifstream;
using std::ios;
using boost::posix_time::ptime;
using boost::posix_time::seconds;
using boost::posix_time::to_iso_string;
using boost::gregorian::date;
using boost::posix_time::time_duration;
using boost::posix_time::from_iso_string;
using boost::posix_time::second_clock;
namespace fs = boost::filesystem;

string formatPerfTime(const double s) {
  string t;
  if (s >= 3600.0) {
    t = convert<string>(s / 3600.0) + "h";
  } else if (s >= 60.0) {
    t = convert<string>(s / 60.0) + "min";
  } else if (s >= 1.0) {
    t = convert<string>(s) + "s";
  } else if (s >= 0.001) {
    t = convert<string>(s * 1000.0) + "ms";
  } else {
    t = convert<string>(s * 1000000.0) + "µs";
  }
  return t;
}

vector<string> listDirs(const string& path) {
  vector<string> dirs;
  fs::path p(path);
  for (fs::directory_iterator dir_it(p);
       dir_it != fs::directory_iterator();
       ++dir_it) {
    if (fs::is_directory(dir_it->status())) {
#ifdef COMPILE_AT_PANAREA
      string dir = dir_it->path().filename();
#else
      string dir = dir_it->path().filename().string();
#endif
      if (dir.at(0) != '.') {  // primitive filtering
        dirs.push_back(dir);
      }
    }
  }
  return dirs;
}

bool fileExists(const string& path) {
  ifstream is(path.c_str());
  if (is.is_open()) {
    is.close();
    return true;
  }
  return false;
}

int fileSize(const string& path) {
  ifstream is(path.c_str(), ifstream::binary);
  int size = 0;
  if (is.is_open()) {
    is.seekg(0, ios::end);
    size = is.tellg();
    is.close();
  }
  return size;
}

string readFile(const string& path) {
  const int size = fileSize(path);
  string data = "";
  if (size) {
    char* buffer = new char[size];
    ifstream is(path.c_str(), ifstream::binary);
    is.read(buffer, size);
    is.close();
    data.assign(buffer, size);
    delete[] buffer;
  }
  return data;
}


bool isValidTimeString(const string& str) {
  try {
    str2time(str);
  } catch(...) {
    return false;
  }
  return true;
}


int64_t str2time(const string& str) {
  static const ptime epoch(date(1970, 1, 1));
  const ptime string_time = from_iso_string(str);
  const time_duration diff = string_time - epoch;
  return diff.total_seconds();
}


string time2str(const int64_t time) {
  static const ptime epoch(date(1970, 1, 1));
  const ptime t(epoch + seconds(time));
  return to_iso_string(t);
}


string getWeekday(const int64_t time) {
  const ptime t(date(1970, 1, 1), seconds(time));
  boost::gregorian::date d = t.date();
  boost::gregorian::greg_weekday wd = d.day_of_week();
  return wd.as_long_string();
}


int64_t getDateOffsetSeconds(const int64_t time) {
  const ptime t(date(1970, 1, 1), seconds(time));
  boost::gregorian::date_duration dd = t.date() - date(1970, 1, 1);
  return dd.days() * 24 * 60 * 60;
}


int64_t localTime() {
  static const ptime epoch(date(1970, 1, 1));
  const time_duration diff = second_clock::local_time() - epoch;
  return diff.total_seconds();
}


int64_t getSeed() {
  return localTime();
}


int64_t firstOfMay() {
  return 1335830400;
}


float greatCircleDistance(const float latitude1, const float longitude1,
                          const float latitude2, const float longitude2) {
  const float r = 6371000.f;
  const float deg_2_rad = M_PI / 180.f;
  const float deltaLat = (latitude2 - latitude1) * deg_2_rad;
  const float deltaLon = (longitude2 - longitude1) * deg_2_rad;
  const float a = sin(deltaLat / 2) * sin(deltaLat / 2) +
                  cos(latitude1  * deg_2_rad) * cos(latitude2 * deg_2_rad) *
                  sin(deltaLon / 2) * sin(deltaLon / 2);
  return  r * 2 * atan2(sqrt(a), sqrt(1 - a));
}


void printTransferPatterns(const set<vector<int> >& transferPatterns) {
  for (auto it = transferPatterns.begin(); it != transferPatterns.end(); it++) {
    std::cout << "[";
    for (uint i = 0; i < it->size(); i++)
      std::cout << it->at(i) << " ";
    std::cout << "]" << std::endl;
  }
}


vector<string> splitString(const string& content) {
  static const string kWhitespace = " \n\t\r";
  vector<string> items;
  size_t pos = content.find_first_not_of(kWhitespace);
  while (pos != string::npos) {
    size_t end = content.find_first_of(kWhitespace, pos);
    if (end == string::npos) {
      // Last item found.
      items.push_back(content.substr(pos));
    } else {
      // Item found.
      items.push_back(content.substr(pos, end - pos));
    }
    pos = content.find_first_not_of(kWhitespace, end);
  }
  return items;
}
