// Copyright 2012: Eugen Sawin, Philip Stahl, Jonas Sternisko
#ifndef SRC_SCENARIOGENERATOR_H_
#define SRC_SCENARIOGENERATOR_H_

#include <set>
#include <string>
#include <vector>
#include "gtest/gtest_prod.h"  // Needed for FRIEND_TEST in this case.
#include "./TransitNetwork.h"

class RandomGen;
class ExpDistribution;
class TransitNetwork;
class Trip;
class Line;
class Logger;

struct ScenarioParams {
  ScenarioParams();
  // Checks if the parameter set is valid.
  bool valid() const;
  // Returns a string representation of the parameterset.
  const std::string str() const;
  // Percentage of trips delayed.
  int delayPercentage;
  // Mean (mu) of the exponential distribution used to generate delay seconds.
  float delayMean;
};


// A Generator for TransitNetwork of one day with random delay in trips.
class ScenarioGenerator {
 public:
  explicit ScenarioGenerator(const vector<ScenarioParams>& params);
  explicit ScenarioGenerator(const ScenarioParams& params);
  // Generate and return a delayed version of the network.
  const TransitNetwork gen(const std::string& networkName);
  const TransitNetwork& generatedNetwork() const;
  // Returns the last generated dc-lines
  const std::vector<Line>& generatedLines() const;
  FRIEND_TEST(ScenarioGeneratorTest, gen);
  const vector<ScenarioParams>& params() const;
  // Selects N random indices in the range 0 ... SIZE-1 using a random number
  // generator with range [0...1]
  static std::set<size_t>
  selectNRandomIndices(int size, int N, RandomFloatGen* random);
 private:
  const std::vector<std::string> extractGtfsDirs(const std::string& args) const;
  FRIEND_TEST(ScenarioGeneratorTest, extractGtfsDirs);
  const std::vector<Trip> delayTrips(const std::vector<Trip>& cTrips,
                                     const Logger* log = NULL) const;
  const Trip delayTrip(const Trip& trip, const int index,
                       const int delay) const;
  FRIEND_TEST(ScenarioGeneratorTest, delayTrips);
  // Tests if all scenario parameters are valid.
  bool validParams() const;
  FRIEND_TEST(ScenarioGeneratorTest, delayTrips_multipleScenarios);

  std::vector<ScenarioParams> _params;
  TransitNetwork _lastGeneratedTN;
  std::vector<Line> _lastGeneratedLines;
};

#endif  // SRC_SCENARIOGENERATOR_H_
