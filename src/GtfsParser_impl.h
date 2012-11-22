// Copyright 2012: Eugen Sawin, Philip Stahl, Jonas Sternisko
#ifndef SRC_GTFSPARSER_IMPL_H_
#define SRC_GTFSPARSER_IMPL_H_

#include "./GtfsParser.h"
#include <map>
#include <string>
#include <vector>

// A datatype for a transportation frequency.
struct GtfsParser::Frequency {
  Frequency() : start(-1), finish(-1), frequency(-1) {}
  Frequency(int start, int finish, int frequency)
  : start(start), finish(finish), frequency(frequency) {}
  // starting time in seconds from 0:00:00
  int start;
  // finish time in seconds from 0:00:00
  int finish;
  // the frequency in seconds
  int frequency;
};

// GtfsParser's internal stored data.
struct GtfsParser::Data {
  ActivityMap  lastServiceActivity;
  map<string, string> lastTrip2Service;
  FrequencyMap lastFrequencies;
  vector<Trip> lastGtfsTrips;
};

#endif  // SRC_GTFSPARSER_IMPL_H_
