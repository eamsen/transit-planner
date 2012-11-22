// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#ifndef SRC_UTILITIES_H_
#define SRC_UTILITIES_H_

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <set>

// For formatting ASCII output.
#define BOLD     "\033[1m"
#define BLACK    "\033[30m"
#define RED      "\033[31m"
#define GREEN    "\033[32m"
#define BROWN    "\033[33m"
#define BLUE     "\033[34m"
#define RESET    "\033[0m"


// Use global Logger LOG instead.
/*#define PARSER     GREEN << "[Parser    ] " << RESET
#define PRECOMP    GREEN << "[Prepare   ] " << RESET
#define EXPERIMENT GREEN << "[Experiment] " << RESET
#define RESULTS    GREEN << "[Results   ] " << RESET*/

using std::set;
using std::map;
using std::vector;
using std::pair;
using std::string;
using std::stringstream;

typedef map<string, string> StrStrMap;
typedef pair<int, int> IntPair;

const int kSecondsPerDay = 86400;

// Converts variable a from its type A to type B.
template<typename B, typename A>
    B convert(const A& a) {
  B b;
  stringstream ss;
  ss << a;
  ss >> b;
  return b;
}

// Map search with inline type conversion.
// Returns whether the key was found in the map.
template<typename A, typename B, typename C>
    bool found(const map<A, B>& m, const A& key, C& value) {
  const typename map<A, B>::const_iterator iter = m.find(key);
  const bool foundit = iter != m.end();
  if (foundit) {
    value = convert<C>(iter->second);
  }
  return foundit;
}

// Initialises a map with given value for given key iff key is not contained
// in the map.
// Returns whether true if the map was initialised with the value for the key
// and false if the key was already contained within the map.
template<typename K, typename V>
bool safeInit(map<K, V>& c, const K& k, const V& v) {
  if (c.find(k) == c.end()) {
    c[k] = v;
    return true;
  }
  return false;
}

// Inserts a value into the container found in the map at given key.
// If there is no container in the map at that key, it creates new container
// at given key and inserts the value into it.
template<typename K, typename V, class Container>
void safeInsert(map<K, Container>& c, const K& k, const V& v) {
  typename map<K, Container>::iterator it = c.find(k);
  if (it == c.end()) {
    Container& value = (c[k] = Container());
    value.insert(v);
  } else {
    it->second.insert(v);
  }
}

template<class T, class Container>
bool contains(const Container& c, const T& key) {
  return c.find(key) != c.end();
}

template<class T>
bool contains(const vector<T>& c, const T& item) {
  for (auto it = c.begin(); it != c.end(); ++it) {
    if (*it == item) {
      return true;
    }
  }
  return false;
}

// Returns a formatted time string.
string formatPerfTime(const double s);

// Retuns all directories listed under path.
vector<string> listDirs(const string& path);

// Returns whether the file exists.
bool fileExists(const string& path);

// Returns the file size.
int fileSize(const string& path);

// Reads the whole file into a string.
// Remark: do not use it for big files.
string readFile(const string& path);

// checks whether a string is a valid time string to be converted with str2time
bool isValidTimeString(const string& str);

// Converts a timestring of format yyyymmddThhmmss into seconds since 1970.
int64_t str2time(const string& str);

// Converts a number of seconds into the iso format string yyyymmddThhmmss.
string time2str(const int64_t time);

// Gets the weekday for a date given by seconds since 1.1.1970
string getWeekday(const int64_t time);

// Considers TIME as a date specified by seconds since 1970. Gets the offset in
// seconds between 0:00:00 at this date and 1.1.1970.
int64_t getDateOffsetSeconds(const int64_t time);

// Returns the local time in seconds since 1970.
int64_t localTime();

// Returns a seed (usually localtime) for random number generation.
int64_t getSeed();

// Returns the first of may localTime at 0:00.
int64_t firstOfMay();

// Computes and returns the great circle distance between two positions on the
// globe in meters.
float greatCircleDistance(const float latitude1, const float longitude1,
                          const float latitude2, const float longitude2);

// Writes a set of transfer patterns to stdout.
void printTransferPatterns(const set<vector<int> >& patterns);

// Splits a string at whitespaces.
vector<string> splitString(const string& content);

#endif  // SRC_UTILITIES_H_
