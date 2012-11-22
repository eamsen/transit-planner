// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include "./Line.h"
#include <cassert>
#include <climits>
#include <set>
#include <string>
#include <vector>
#include "./Utilities.h"

using std::make_pair;

// TripTime

bool TripTime::operator<(const TripTime& rhs) const {
  assert(_times.size());
  return _times[0].first < rhs._times[0].first;
}

bool TripTime::operator==(const TripTime& rhs) const {
  return _times == rhs._times;
}

int64_t TripTime::arr(const int pos) const {
  assert(pos < size());
  return _times[pos].second;
}

int64_t TripTime::dep(const int pos) const {
  assert(pos < size());
  return _times[pos].first;
}

int TripTime::size() const {
  return _times.size();
}

void TripTime::addStopTime(const int64_t arrTime, const int64_t depTime) {
  assert(_times.empty() || _times.back().first <= arrTime);
  // We keep departure times first in the tuple for convient sorting.
  _times.push_back(make_pair(depTime, arrTime));
}

Int64Pair& TripTime::back() {
  return _times.back();
}

string TripTime::str() const {
  string s = "[ ";
  for (auto it = _times.begin(); it != _times.end(); ++it) {
    const Int64Pair& time = *it;
    s += convert<string>(time.second) + "|"
        + convert<string>(time.first) + " ";
  }
  s += "]";
  return s;
}

// Trip

Trip::Trip() {
  _id = "undefined";
}

Trip::Trip(const string& id) {
  _id = id;
}

const string& Trip::id() const {
  return _id;
}

bool Trip::operator==(const Trip& rhs) const {
  return _time == rhs._time && _stops == rhs._stops && _id == rhs._id;
}

int Trip::size() const {
  assert(static_cast<int>(_stops.size()) == _time.size());
  return _stops.size();
}

void Trip::addStop(const int64_t arrTime, const int64_t depTime,
                   const int stop) {
  assert(_stops.empty() || _stops.back() != stop);
  _time.addStopTime(arrTime, depTime);
  _stops.push_back(stop);
}

int Trip::stop(const int i) const {
  assert(i >= 0 && i < static_cast<int>(_stops.size()));
  return _stops[i];
}

const vector<int>& Trip::stops() const {
  return _stops;
}

const TripTime& Trip::time() const {
  return _time;
}

TripTime& Trip::tripTime() {
  return _time;
}

string Trip::str() const {
  string s = "[ ";
  for (auto it = _stops.begin(); it != _stops.end(); ++it) {
    const int stop = *it;
    s += convert<string>(stop) + " ";
  }
  s += "]\n" + _time.str();
  return s;
}

// Line

const int Line::INFINITE = INT_MAX;

int Line::size() const {
  return _stops.size();
}

bool Line::candidate(const Trip& trip) const {
  const int numStops = _stops.size();
  if (numStops && static_cast<int>(numStops) != trip.size()) {
    return false;
  }
  for (int i = 0; i < numStops; ++i) {
    const int stop = _stops[i];
    if (stop != trip.stop(i)) {
      // The trip does not share the same sequence of stations.
      return false;
    } else if (i > 0) {
      const int lineTravelTime = cost(i-1, 0, i) -
                                 nextDeparture(i-1, 0, i);
      const int tripTravelTime = trip.time().arr(i) -
                                 trip.time().dep(i-1);
      if (lineTravelTime != tripTravelTime) {
        // The trip overtakes or gets overtaken by another trip in the line.
        return false;
      }
    }
  }
  return true;
}

bool Line::addTrip(const Trip& trip) {
  if (candidate(trip)) {
    if (_stops.empty()) {
      _stops = trip.stops();
    }
    _tripTimes.insert(trip.time());
    return true;
  }
  return false;
}

const vector<int>& Line::stops() const {
  return _stops;
}

int Line::stop(const int pos) const {
  return _stops[pos];
}

int Line::cost(const int depPos, const int64_t time, const int destPos) const {
  // This might be handled more efficiently with d&c.
  auto it = _tripTimes.begin();
  while (it != _tripTimes.end() && it->dep(depPos) < time) {
    ++it;
  }
  if (it == _tripTimes.end()) {
    return INFINITE;
  }
  return it->arr(destPos) - time;
}


int Line::nextDeparture(const int depPos, const int64_t time,
                        const int destPos) const {
  auto it = _tripTimes.begin();
  while (it != _tripTimes.end() && it->dep(depPos) < time) {
    ++it;
  }
  if (it == _tripTimes.end()) {
    return INFINITE;
  }
  return it->dep(depPos);
}


string Line::str() const {
  string s = "[ ";
  for (auto it = _stops.begin(); it != _stops.end(); ++it) {
    const int stop = *it;
    s += convert<string>(stop) + " ";
  }
  s += "]\n";
  for (auto it = _tripTimes.begin(); it != _tripTimes.end(); ++it) {
    const TripTime& t = *it;
    s += t.str() + "\n";
  }
  return s;
}

// LineFactory
// TODO(jonas): This method is just used by tests. Remove it?
void LineFactory::createTrips(const vector<Int64Pair>& times,
                              const vector<int>& stops,
                              vector<Trip>* trips) {
  assert(times.size() == stops.size());
  assert(trips);
  set<int> lineStops;
  Trip trip;
  const int numTimes = times.size();
  for (int i = 0; i < numTimes; ++i) {
    set<int>::const_iterator it = lineStops.find(stops[i]);
    if (it != lineStops.end()) {
      lineStops.clear();
      trips->push_back(trip);
      trip = Trip();
    }
    trip.addStop(times[i].first, times[i].second, stops[i]);
    lineStops.insert(stops[i]);
  }
  if (trip.size()) {
    trips->push_back(trip);
  }
}

Trip LineFactory::createTrip(const vector<Int64Pair>& times,
                             const vector<int>& stops) {
  assert(times.size() == stops.size());
  Trip trip;
  const int numStops = times.size();
  for (int i = 0; i < numStops; ++i) {
    trip.addStop(times[i].first, times[i].second, stops[i]);
  }
  return trip;
}

vector<Line> LineFactory::createLines(const vector<Trip>& trips) {
  vector<Line> lines;
  for (auto it = trips.begin(), end = trips.end(); it != end; ++it) {
    const Trip& t = *it;
    bool matched = false;
    for (auto it2 = lines.begin(), end2 = lines.end(); it2 != end2; ++it2) {
      Line& l = *it2;
      if (l.addTrip(t)) {
        matched = true;
        break;
      }
    }
    if (!matched) {
      Line line;
      line.addTrip(t);
      lines.push_back(line);
    }
  }
  return lines;
}
