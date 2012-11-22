// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include "./DirectConnection.h"
#include <cassert>
#include <climits>
#include <vector>
#include <string>
#include <set>
#include <algorithm>
#include "./Utilities.h"

using std::set_intersection;
using std::min;

const int DirectConnection::INFINITE = INT_MAX;

DirectConnection::DirectConnection() {}

DirectConnection::DirectConnection(const int numStops)
    : _incidents(numStops) {}

DirectConnection::DirectConnection(const int numStops,
                                   const vector<Line>& lines) {
  init(numStops, lines);
}

void DirectConnection::init(const int numStops, const vector<Line>& lines) {
  _incidents.clear();
  _incidents.resize(numStops);
  for (auto it = lines.begin(), end = lines.end(); it != end; ++it) {
    addLine(*it);
  }
}

void DirectConnection::addLine(const Line& line) {
  const int numInc = line.size();
  for (int i = 0; i < numInc; ++i) {
    assert(line.stop(i) < static_cast<int>(_incidents.size()));
    _incidents[line.stop(i)].insert(Incidence(_lines.size(), i));
  }
  _lines.push_back(line);
}

int DirectConnection::query(const int dep, const int64_t time,
                            const int dest) const {
  assert(dep < static_cast<int>(_incidents.size()));
  assert(dest < static_cast<int>(_incidents.size()));
  const set<Incidence>& depSet  = _incidents[dep];
  const set<Incidence>& destSet = _incidents[dest];
  vector<Incidence> inter(min(depSet.size(), destSet.size()));
  auto const end = set_intersection(depSet.begin(), depSet.end(),
                                    destSet.begin(), destSet.end(),
                                    inter.begin());
  int cost = INFINITE;
  for (auto it = inter.begin(); it != end; ++it) {
    const Incidence& depInc  = *depSet.find(*it);
    const Incidence& destInc = *destSet.find(*it);
    if (depInc.pos < destInc.pos) {
      cost = min(cost,
                 _lines.at(depInc.line).cost(depInc.pos, time, destInc.pos));
    }
  }
  return cost;
}


int DirectConnection::nextDepartureTime(const int dep, const int64_t time,
                                        const int dest) const {
  assert(dep < static_cast<int>(_incidents.size()));
  assert(dest < static_cast<int>(_incidents.size()));
  const set<Incidence>& depSet  = _incidents[dep];
  const set<Incidence>& destSet = _incidents[dest];
  vector<Incidence> inter(min(depSet.size(), destSet.size()));
  auto const end = set_intersection(depSet.begin(), depSet.end(),
                                    destSet.begin(), destSet.end(),
                                    inter.begin());
  int nextDepartureTime = INFINITE;
  for (auto it = inter.begin(); it != end; ++it) {
    const Incidence& depInc = *depSet.find(*it);
    const Incidence& destInc = *destSet.find(*it);
    if (depInc.pos < destInc.pos) {
      nextDepartureTime =
       min(nextDepartureTime,
           _lines[depInc.line].nextDeparture(depInc.pos, time, destInc.pos));
    }
  }
  return nextDepartureTime;
}


string DirectConnection::str() const {
  string s;
  for (auto it = _lines.begin(); it != _lines.end(); ++it) {
    const Line& l = *it;
    s += l.str();
  }
  s += "stop_id: [ line_id:line_pos ... ]\n";
  for (size_t i = 0; i < _incidents.size(); ++i) {
    s += convert<string>(i) + ": [ ";
    for (auto it = _incidents.at(i).begin();
         it != _incidents.at(i).end();
         ++it) {
      const Incidence& inc = *it;
      s += convert<string>(inc.line) + ":" + convert<string>(inc.pos) + " ";
    }
    s += "]\n";
  }
  return s;
}
