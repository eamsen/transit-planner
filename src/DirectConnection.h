// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#ifndef SRC_DIRECTCONNECTION_H_
#define SRC_DIRECTCONNECTION_H_

#include <gtest/gtest_prod.h>
#include <boost/serialization/access.hpp>
#include <set>
#include <vector>
#include <string>
#include "./Line.h"

using std::set;
using std::vector;
using std::string;

// A line incidence for some stop.
struct Incidence {
  Incidence()
  : line(-1), pos(-1) {}

  Incidence(const int line, const int pos)
  : line(line), pos(pos) {}

  // Comparison operator used for sorting by line index.
  bool operator<(const Incidence& rhs) const {
    return line < rhs.line;
  }

  // Equality operator used for set intersection by line index.
  bool operator==(const Incidence& rhs) const {
    return line == rhs.line;
  }

  // The line index.
  int line;

  // The position of the stop within the line's sequence.
  int pos;
};

// This data structure computes direct-connection queries efficiently.
class DirectConnection {
 public:
  // Used for unreachable connection costs.
  static const int INFINITE;

  DirectConnection();

  // Prepares the data structure for lines with given total number of stops.
  explicit DirectConnection(const int numStops);

  // Initialises the data structure with the given lines.
  explicit DirectConnection(const int numStops, const vector<Line>& lines);

  // Initialises the data structure with the given lines.
  void init(const int numStops, const vector<Line>& lines);

  // Adds a line for efficient direct-connection queries.
  void addLine(const Line& line);

  // Computes the optimal costs between the given stops at the given time.
  // Returns the total time difference between the arrival time and the query
  // time if a direct connection exists and INFINITE otherwise.
  int query(const int dep, const int64_t time, const int dest) const;

  // Returns the next possible start time on the connection from dep to dest
  // after time.
  int nextDepartureTime(const int dep, const int64_t time, const int dest)
  const;

  // Returns a string representation of the direct connection structure.
  string str() const;

  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {  // NOLINT
    ar & _incidents;
    ar & _lines;
  }
  friend class boost::serialization::access;

 private:
  vector<set<Incidence> > _incidents;
  vector<Line> _lines;
  FRIEND_TEST(DirectConnectionTest, LineFactory_doubleStop);
};

#endif  // SRC_DIRECTCONNECTION_H_
