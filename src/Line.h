// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#ifndef SRC_LINE_H_
#define SRC_LINE_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/utility.hpp>
#include <set>
#include <string>
#include <vector>
#include <utility>

using std::string;
using std::set;
using std::vector;
using std::pair;

typedef pair<int64_t, int64_t> Int64Pair;

// Time table for a trip.
class TripTime {
 public:
  // Comparison operator used for sorting by first departure time.
  bool operator<(const TripTime& rhs) const;
  bool operator==(const TripTime& rhs) const;

  // Returns the arrival time for given stop position.
  int64_t arr(const int pos) const;

  // Returns the departure time for given stop position.
  int64_t dep(const int pos) const;

  // Returns the number of stop times.
  int size() const;

  // Adds a stop time to the time table.
  void addStopTime(const int64_t arrTime, const int64_t depTime);

  // Returns a write reference to the last element.
  Int64Pair& back();

  // Returns a string representation of the time table.
  string str() const;

 private:
  // departure, arrival time tuples
  vector<Int64Pair> _times;

  // serialization
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {  // NOLINT
    ar & _times;
  }
  friend class boost::serialization::access;
};

// A trip is a sequence of stops without transfers.
// It consists of a time table with corresponding stop indices.
class Trip {
 public:
  explicit Trip();
  explicit Trip(const string& id);

  bool operator==(const Trip& rhs) const;

  // Returns the number of stops on the trip.
  int size() const;

  // Adds a stop to the trip.
  void addStop(const int64_t arrTime, const int64_t depTime, const int stop);

  // Returns stop index at given stop sequence.
  int stop(const int i) const;

  // Returns a const reference to the stop indices.
  const vector<int>& stops() const;

  // Returns a const reference to the trip time table.
  const TripTime& time() const;

  // Returns a write reference to the trip time table.
  TripTime& tripTime();

  // Returns a const reference to the trip id.
  const string& id() const;

  // Returns a string representation of the trip.
  string str() const;

 private:
  string _id;
  TripTime _time;
  // stop indices
  vector<int> _stops;
};

// A line is a collection of trips with the same stop sequence.
class Line {
 public:
  static const int INFINITE;

  // Returns the number of stops on the line's trips.
  int size() const;

  // Returns whether the trip shares the line's stop sequence.
  bool candidate(const Trip& trip) const;

  // Adds a trip to the line, if the trip is suitable.
  // Returns wether the trip was added.
  bool addTrip(const Trip& trip);

  // Returns a const reference to the stop indices.
  const vector<int>& stops() const;

  // Returns the stop index at given sequence position.
  int stop(const int pos) const;

  // Returns the cost to travel from a stop dep to a stop dest starting at time.
  // The total cost is waiting time + travel time in seconds.
  int cost(const int dep, const int64_t time, const int dest) const;

  // Return the next departure time from dep to dest after time.
  int nextDeparture(const int dep, const int64_t time, const int dest) const;

  // Returns a string representation of the line.
  string str() const;

 private:
  set<TripTime> _tripTimes;
  vector<int> _stops;

  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {  // NOLINT
    ar & _tripTimes;
    ar & _stops;
  }
  friend class boost::serialization::access;
};

// Utilities for trips and line construction.
struct LineFactory {
  // Creates trips out of a list of times with corresponding stop indices, adds
  // the trips to an existing collection of trips.
  static void createTrips(const vector<Int64Pair>& times,
                          const vector<int>& stops,
                          vector<Trip>* trips);

  // Creates on trip out of a list
  static Trip createTrip(const vector<Int64Pair>& times,
                         const vector<int>& stops);

  // Creates lines out of a list of trips.
  static vector<Line> createLines(const vector<Trip>& trips);
};

#endif  // SRC_LINE_H_
