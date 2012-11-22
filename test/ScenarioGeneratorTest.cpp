// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include<set>
#include <string>
#include <vector>
#include "./GtestUtil.h"
#include "../src/ScenarioGenerator.h"
#include "../src/GtfsParser_impl.h"
#include "../src/Line.h"
#include "../src/Random.h"
#include "../src/TransitNetwork.h"
#include "../src/Dijkstra.h"
#include "../src/Utilities.h"


using std::ostringstream;
using ::testing::Contains;

TEST(ScenarioGeneratorTest, extractGtfsDirs) {
  ScenarioParams params;
  params.delayPercentage = 0;
  params.delayMean = 0.00001;

  ScenarioGenerator generator(params);
  string args;
  std::vector<std::string> gtfsDirs;

  args = "./build/ServerMain -i test/data/hawaii";
  gtfsDirs = generator.extractGtfsDirs(args);
  EXPECT_EQ("test/data/hawaii", gtfsDirs.at(0));

  args = "./build/ServerMain -i test/data/hawaii -m 4";
  gtfsDirs = generator.extractGtfsDirs(args);
  EXPECT_EQ("test/data/hawaii", gtfsDirs.at(0));

  args = ("./build/ServerMain -i test/data/new_york/google_transit_subway/ "
          "test/data/new_york/google_transit_manhattan/ "
          "test/data/new_york/google_transit_brooklyn/ "
          "test/data/new_york/google_transit_bronx/");
  gtfsDirs = generator.extractGtfsDirs(args);
  EXPECT_EQ("test/data/new_york/google_transit_subway/", gtfsDirs.at(0));
  EXPECT_EQ("test/data/new_york/google_transit_manhattan/", gtfsDirs.at(1));
  EXPECT_EQ("test/data/new_york/google_transit_brooklyn/", gtfsDirs.at(2));
  EXPECT_EQ("test/data/new_york/google_transit_bronx/", gtfsDirs.at(3));

  args = ("./build/ServerMain -i test/data/new_york/google_transit_subway/ "
          "test/data/new_york/google_transit_manhattan/ "
          "test/data/new_york/google_transit_brooklyn/ "
          "test/data/new_york/google_transit_bronx/ -m 4");
  gtfsDirs = generator.extractGtfsDirs(args);
  EXPECT_EQ("test/data/new_york/google_transit_subway/", gtfsDirs.at(0));
  EXPECT_EQ("test/data/new_york/google_transit_manhattan/", gtfsDirs.at(1));
  EXPECT_EQ("test/data/new_york/google_transit_brooklyn/", gtfsDirs.at(2));
  EXPECT_EQ("test/data/new_york/google_transit_bronx/", gtfsDirs.at(3));
}

TEST(ScenarioGeneratorTest, exponentialDistribution) {
  ExpDistribution distribution1(getSeed(), 1/0.0000001);
  for (int i = 0; i < 100; ++i)
    EXPECT_GT(distribution1.sample(), 0);
  ExpDistribution distribution2(getSeed(), 1/20000.);
  const int n = 100;
  int countSmall = 0;
  for (int i = 0; i < n; ++i)
    if (distribution2.sample() <= 1) countSmall++;
  EXPECT_LT(countSmall, 1);  // This is a random test, it might fail somtimes!

  // Test the pdf using the cdf
  float beta = 1/10.;
  ExpDistribution distribution3(getSeed(), beta);
  const size_t N = 1000000;
  vector<size_t> count(40, 0);
  // draw random samples using the probability densitiy function
  for (size_t i = 0; i < N; ++i) {
    float sample = distribution3.sample();
    size_t index = static_cast<size_t>(sample);
    if (index > count.size() - 1)
      count.back()++;
    else
      count[index]++;
  }
  // compare average values with the cumulative density function
  vector<float> cdf(40 - 1, 0);
  for (size_t i = 0; i < count.size() - 1; ++i)
    cdf[i] = (1 - exp(-beta*(i+1))) - (1 - exp(-beta*(i)));
  for (size_t i = 0; i < cdf.size(); ++i)
    EXPECT_NEAR(count[i] / static_cast<float>(N), cdf[i], 0.001);
}

TEST(ScenarioGeneratorTest, selectNRandomIndices) {
  RandomFloatGen random(0., 1.);

  // Test that the selected indices are of the right number and range.
  EXPECT_EQ(4, ScenarioGenerator::selectNRandomIndices(42, 4, &random).size());
  EXPECT_EQ(0, ScenarioGenerator::selectNRandomIndices(42, 0, &random).size());
  set<size_t> full =
      ScenarioGenerator::selectNRandomIndices(42, 42, &random);
  EXPECT_EQ(42, full.size());
  for (size_t val: full) {
    EXPECT_LT(val, 42);
    EXPECT_GE(val, 0);
  }

  // Test that the selected indices are distributed equally.
  vector<size_t> count(40, 0);
  const size_t n = 500000;
  for (size_t i = 0; i < n; ++i) {
    set<size_t> selected =
        ScenarioGenerator::selectNRandomIndices(40, 10, &random);
    for (size_t elem: selected) {
      count[elem]++;
    }
  }
  for (size_t val: count) {
    EXPECT_NEAR(static_cast<float>(val) / n, 0.25, 0.002);
  }
}

TEST(ScenarioGeneratorTest, delayTrips) {
  LineFactory lf;
  vector<Int64Pair> times = {Int64Pair(0, 5), Int64Pair(15, 20),
                             Int64Pair(30, 35)};
  vector<int> stop = {0, 1, 2};
  Trip t1 = lf.createTrip(times, stop);
  ScenarioParams params;
  params.delayPercentage = 0;
  params.delayMean = 0.00001;
  ASSERT_TRUE(params.valid());

  ScenarioGenerator sg(params);
  ExpDistribution distribution1(9999, 1/0.0000001);  // mean of 0 seconds delay
  Trip delayed = sg.delayTrip(t1, t1.size() + 1, 0);
  EXPECT_EQ(t1, delayed);
  EXPECT_EQ(t1.stops(), delayed.stops());

  delayed = sg.delayTrip(t1, 0, 0);
  EXPECT_EQ(t1, delayed);
  EXPECT_EQ(t1.stops(), delayed.stops());
  EXPECT_EQ(t1, delayed);

  ExpDistribution distribution2(9999, 1./1800.);  // mean of 1800 seconds delay

  int delay = floor(distribution2.sample());
  delayed = sg.delayTrip(t1, 0, delay);
  EXPECT_FALSE(delayed == t1);
  EXPECT_GT(delayed.time().arr(0), t1.time().arr(0));
  EXPECT_GT(delayed.time().dep(0), t1.time().dep(0));
  EXPECT_GT(delayed.time().arr(1), t1.time().arr(1));
  EXPECT_GT(delayed.time().dep(1), t1.time().dep(1));
  EXPECT_GT(delayed.time().arr(2), t1.time().arr(2));
  EXPECT_GT(delayed.time().dep(2), t1.time().dep(2));
  EXPECT_EQ(t1.stops(), delayed.stops());

  delay = floor(distribution2.sample());
  delayed = sg.delayTrip(t1, 1, delay);
  EXPECT_EQ(delayed.time().arr(0), t1.time().arr(0));
  EXPECT_EQ(delayed.time().dep(0), t1.time().dep(0));
  EXPECT_GT(delayed.time().arr(1), t1.time().arr(1));
  EXPECT_GT(delayed.time().dep(1), t1.time().dep(1));
  EXPECT_GT(delayed.time().arr(2), t1.time().arr(2));
  EXPECT_GT(delayed.time().dep(2), t1.time().dep(2));
  EXPECT_EQ(t1.stops(), delayed.stops());

  params.delayPercentage = 100;
  params.delayMean = 0.00001;
  sg = ScenarioGenerator(params);
  vector<Trip> trips = {t1};
  vector<Trip> delayedTrips = sg.delayTrips(trips);
  EXPECT_EQ(trips.size(), delayedTrips.size());
  EXPECT_EQ(trips, delayedTrips);

  params.delayPercentage = 100;
  params.delayMean = 20.;
  sg = ScenarioGenerator(params);
  vector<Trip> trips2 = {t1, t1, t1};
  vector<Trip> delayedTrips2 = sg.delayTrips(trips2);
  EXPECT_EQ(trips2.size(), delayedTrips2.size());
  EXPECT_EQ(3, delayedTrips2.size());
  for (size_t i = 0; i < delayedTrips2.size(); ++i)
    EXPECT_EQ(t1.size(), delayedTrips2[i].size());
  EXPECT_NE(trips2, delayedTrips2);
  EXPECT_EQ(trips2[0].stops(), delayedTrips2[0].stops());
  EXPECT_EQ(trips2[1].stops(), delayedTrips2[1].stops());
  EXPECT_EQ(trips2[2].stops(), delayedTrips2[2].stops());
  bool delayReached = false;
  for (int i = 0; i < trips2[0].size(); ++i) {
    if (!delayReached) {
      if (trips2[0].time().arr(i) < delayedTrips2[0].time().arr(i)) {
        EXPECT_LT(trips2[0].time().dep(i), delayedTrips2[0].time().dep(i));
        delayReached = true;
      } else {
        EXPECT_EQ(trips2[0].time().arr(i), delayedTrips2[0].time().arr(i));
        EXPECT_EQ(trips2[0].time().dep(i), delayedTrips2[0].time().dep(i));
      }
    } else {
      EXPECT_LT(trips2[0].time().arr(i), delayedTrips2[0].time().arr(i));
      EXPECT_LT(trips2[0].time().dep(i), delayedTrips2[0].time().dep(i));
    }
  }
}

TEST(ScenarioGeneratorTest, delayTrips_multipleScenarios) {
  vector<Int64Pair> times = {Int64Pair(0, 5), Int64Pair(15, 20),
  Int64Pair(30, 35)};
  vector<int> stop = {0, 1, 2};
  Trip t1 = LineFactory::createTrip(times, stop);
  vector<Trip> trips(20, t1);

  ScenarioParams params1, params2;
  params1.delayPercentage = 0;
  params1.delayMean = 45.;
  params2.delayPercentage = 10;
  params2.delayMean = 20.;
  vector<ScenarioParams> par = {params1, params2};

  ScenarioGenerator sg(par);
  ASSERT_TRUE(sg.validParams());
  vector<Trip> delayed = sg.delayTrips(trips);
  // ensure that the original trip is still contained at least once
  ASSERT_EQ(20, delayed.size());
  EXPECT_THAT(delayed, Contains(t1));

  // now change all
  params1.delayPercentage = 90;
  vector<ScenarioParams> par2 = {params1, params2};
  ScenarioGenerator sg2(par2);
  ASSERT_TRUE(sg2.validParams());
  vector<Trip> delayed2 = sg2.delayTrips(trips);
  ASSERT_EQ(20, delayed2.size());
  ASSERT_EQ(20, trips.size());
  for (size_t i = 0; i < trips.size(); ++i) {
    ASSERT_EQ(trips[i].size(), delayed2[i].size());
    bool delayReached = false;
    for (int i = 0; i < trips[0].size(); ++i) {
      if (!delayReached) {
        if (trips[0].time().arr(i) < delayed2[0].time().arr(i)) {
          EXPECT_LT(trips[0].time().dep(i), delayed2[0].time().dep(i));
          delayReached = true;
        } else {
          EXPECT_EQ(trips[0].time().arr(i), delayed2[0].time().arr(i));
          EXPECT_EQ(trips[0].time().dep(i), delayed2[0].time().dep(i));
        }
      } else {
        EXPECT_LT(trips[0].time().arr(i), delayed2[0].time().arr(i));
        EXPECT_LT(trips[0].time().dep(i), delayed2[0].time().dep(i));
      }
    }
  }
}

TEST(ScenarioGeneratorTest, gen) {
  GtfsParser parser;
  TransitNetwork network =
      parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                  "20120501T000000", "20120501T000001");
  network.preprocess();

  ofstream f("local/simple-parser-test-case.info");
  f << "./build/ServerMain -i test/data/simple-parser-test-case -m 4"
    << std::endl;
  f.close();

  ScenarioParams params;
  params.delayPercentage = 100;
  params.delayMean = 180;

  ScenarioGenerator generator(params);

  string networkName = "simple-parser-test-case";
  TransitNetwork scenario = generator.gen(networkName);

  EXPECT_EQ(network.numStops(), scenario.numStops());
  EXPECT_EQ(network.numNodes(), scenario.numNodes());
  EXPECT_EQ(network.numArcs(), scenario.numArcs());
  // count delayed nodes: should be at least the 6 nodes of the two last stops
  int count = 0;
  for (size_t i = 0; i < network.numNodes(); i++) {
    EXPECT_LE(network.node(i).time(), scenario.node(i).time());
    if (network.node(i).time() < scenario.node(i).time())
      count++;
  }
  EXPECT_LE(6, count);

  int tStart = str2time("20120501T000000");
  Dijkstra compare(network);
  QueryResult res2;
  vector<int> start2 =
      network.findStartNodeSequence(network.stop(network.stopIndex("A")),
                                    tStart);
  compare.startTime(tStart);
  compare.findShortestPath(start2, network.stopIndex("C"), &res2);
  ASSERT_EQ(30*60, res2.destLabels.begin()->cost());

  Dijkstra dijkstra(scenario);
  QueryResult res;
  vector<int> start1 =
      scenario.findStartNodeSequence(scenario.stop(scenario.stopIndex("A")),
                                     tStart);
  dijkstra.startTime(tStart);
  dijkstra.findShortestPath(start1, scenario.stopIndex("C"), &res);

  EXPECT_LT(res2.optimalCosts(), res.optimalCosts());
}

TEST(ScenarioGeneratorTest, gen_multipleNetworks) {
  GtfsParser parser;
  vector<string> gtfs = {"test/data/simple-parser-test-case/",
                         "test/data/time-test-case1"};
  TransitNetwork network =
      parser.createTransitNetwork(gtfs, "20120501T000000", "20120501T000001");
  network.preprocess();

  ofstream f("local/simple-parser-test-case.info");
  f << "./build/ServerMain -i test/data/simple-parser-test-case"
       " test/data/time-test-case1 -m 4" << std::endl;
  f.close();

  ScenarioParams params;
  params.delayPercentage = 100;
  params.delayMean = 180;

  ScenarioGenerator generator(params);

  string networkName = "simple-parser-test-case";
  TransitNetwork scenario = generator.gen(networkName);

  EXPECT_EQ(network.numStops(), scenario.numStops());
  EXPECT_EQ(network.numNodes(), scenario.numNodes());
  EXPECT_EQ(network.numArcs(), scenario.numArcs());
  // count delayed nodes: should be at least the 3 nodes of the three last stops
  int count = 0;
  for (size_t i = 0; i < network.numNodes(); i++) {
    EXPECT_LE(network.node(i).time(), scenario.node(i).time());
    if (network.node(i).time() < scenario.node(i).time())
      count++;
  }
  EXPECT_LT(9, count);

  int tStart = str2time("20120501T000000");
  Dijkstra compare(network);
  QueryResult compare1;
  vector<int> start2 =
      network.findStartNodeSequence(network.stop(network.stopIndex("A")),
                                                 tStart);
  compare.startTime(tStart);
  compare.findShortestPath(start2, network.stopIndex("C"), &compare1);
  ASSERT_EQ(30*60, compare1.destLabels.begin()->cost());

  // Check for delay in the first part of the network
  Dijkstra dijkstra(scenario);
  QueryResult result1;
  vector<int> start1 =
      scenario.findStartNodeSequence(scenario.stop(scenario.stopIndex("A")),
                                     tStart);
  dijkstra.startTime(tStart);
  dijkstra.findShortestPath(start1, scenario.stopIndex("C"), &result1);
  EXPECT_LT(compare1.optimalCosts(), result1.optimalCosts());

  // Check for delay in the second part of the network
  QueryResult compare2, result2;
  compare.startTime(tStart);
  vector<int> startO =
      network.findStartNodeSequence(network.stop(network.stopIndex("O")),
                                    tStart);
  compare.findShortestPath(startO, scenario.stopIndex("Z"), &compare2);
  vector<int> start4 =
      scenario.findStartNodeSequence(scenario.stop(scenario.stopIndex("O")),
                                     tStart);
  dijkstra.startTime(tStart);
  dijkstra.findShortestPath(start4, scenario.stopIndex("Z"), &result2);
  EXPECT_LT(compare2.optimalCosts(), result2.optimalCosts());
}
