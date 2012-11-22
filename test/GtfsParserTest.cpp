// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include <boost/date_time/posix_time/posix_time.hpp>
#include <google/dense_hash_set>
#include <google/dense_hash_map>
#include <ostream>
#include <map>
#include <string>
#include <vector>
#include "./GtestUtil.h"
#include "../src/GtfsParser_impl.h"
#include "../src/TransitNetwork.h"
#include "../src/Line.h"
#include "../src/Logger.h"
#include "../src/Utilities.h"

using std::vector;
using std::string;
using boost::posix_time::from_iso_string;
using google::dense_hash_set;
using google::dense_hash_map;
using ::testing::ElementsAre;
using ::testing::Contains;
using ::testing::ContainerEq;

// The basic test fixture for these tests.
class GtfsParserTest : public ::testing::Test {
 public:
  GtfsParser parser;
  Logger test_logger;
  GtfsParser::ActivityMap serviceActivity;
  map<string, string> trip2Service;
  dense_hash_map<string, int> stopId2index;
  GtfsParser::FrequencyMap frequencies;
  vector<Stop> stops;
  TransitNetwork simple_network_two_days;

  // Prepares the fixture object. Could be in Constructor as well.
  void SetUp() {
    serviceActivity.set_empty_key("");
//     trip2Service.set_empty_key("");
    stopId2index.set_empty_key("");
    frequencies.set_empty_key("");
    test_logger.target("log/test.log");
    parser.logger(&test_logger);
    simple_network_two_days =
        parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                    "20111216T000000", "20111217T235959");
  }

  // Finishes the usage of the fixture object. Could be in Destructor as well.
  void TearDown() {}
};

// The Tests go below.

// _____________________________________________________________________________
TEST_F(GtfsParserTest, parseName) {
  string test1 = "/home/user/proj/test/data/gtfs_dataset_name";
  string test2 = "/home/user/proj/test/data/gtfs_dataset_name/";
  string test3 = "/home/user/proj/test/data/gtfs_dataset_name///";
  string test4 = "/home/user/proj/test/data////gtfs_dataset_name";

  string name = "gtfs_dataset_name";
  EXPECT_EQ(name, parser.parseName(test1));
  EXPECT_EQ(name, parser.parseName(test2));
  EXPECT_EQ(name, parser.parseName(test3));
  EXPECT_EQ(name, parser.parseName(test4));
}

// _____________________________________________________________________________
TEST_F(GtfsParserTest, calendar_txt) {
  string filename = tmpDir + "CalenderTest.TMP.txt";
  // Write simple calender file.
  FILE* file = fopen(filename.c_str(), "w");
  ASSERT_TRUE(file != NULL);
  fprintf(file,
          "service_id,monday,tuesday,wednesday,thursday,friday,saturday,"
              "sunday,start_date,end_date\n"
          "FULLW,1,1,1,1,1,1,1,20070101,20121231\n"
          "WE,0,0,0,0,0,1,1,20070101,20121231\n");
  fclose(file);

  serviceActivity = parser.parseCalendarFile(filename.c_str());
  ASSERT_TRUE(contains(serviceActivity, string("FULLW")));
  for (size_t i = 0; i < 7; ++i)
    EXPECT_TRUE(serviceActivity["FULLW"].get<0>()[i]);
  EXPECT_EQ(20070101, serviceActivity["FULLW"].get<1>());
  EXPECT_EQ(20121231, serviceActivity["FULLW"].get<2>());

  ASSERT_TRUE(contains(serviceActivity, string("WE")));
  for (size_t i = 0; i < 5; ++i)
    EXPECT_FALSE(serviceActivity["WE"].get<0>()[i]);
}

// _____________________________________________________________________________
TEST_F(GtfsParserTest, trips_txt) {
  // Write simple trips file.
  string filename = tmpDir + "TripsTest.TMP.txt";
  FILE* file = fopen(filename.c_str(), "w");
  ASSERT_TRUE(file != NULL);
  fprintf(file,
          "route_id,service_id,trip_id,trip_headsign,direction_id,block_id,"
              "shape_id\n"
          "AB,FULLW,AB1,to Bullfrog,0,1,\n"
          "AB,FULLW,AB2,to Airport,1,2,\n"
          "STBA,FULLW,STBA,Shuttle,,,\n"
          "CITY,FULLW,CITY1,,0,,\n"
          "CITY,FULLW,CITY2,,1,,\n"
          "BFC,FULLW,BFC1,to Furnace Creek Resort,0,1,\n"
          "BFC,FULLW,BFC2,to Bullfrog,1,2,\n"
          "AAMV,WE,AAMV1,to Amargosa Valley,0,,\n"
          "AAMV,WE,AAMV2,to Airport,1,,\n"
          "AAMV,WE,AAMV3,to Amargosa Valley,0,,\n"
          "AAMV,WE,AAMV4,to Airport,1,,\n");
  fclose(file);


  trip2Service = parser.parseTripsFile(filename.c_str());
  EXPECT_EQ("FULLW", trip2Service["AB1"]);
  EXPECT_EQ("FULLW", trip2Service["AB2"]);
  EXPECT_EQ("FULLW", trip2Service["STBA"]);
  EXPECT_EQ("FULLW", trip2Service["CITY1"]);
  EXPECT_EQ("FULLW", trip2Service["CITY2"]);
  EXPECT_EQ("FULLW", trip2Service["BFC1"]);
  EXPECT_EQ("FULLW", trip2Service["BFC2"]);
  EXPECT_EQ("WE", trip2Service["AAMV1"]);
}

// _____________________________________________________________________________
TEST_F(GtfsParserTest, isActive) {
  serviceActivity = parser.parseCalendarFile(tmpDir + "CalenderTest.TMP.txt");
  trip2Service = parser.parseTripsFile(tmpDir + "TripsTest.TMP.txt");

  boost::gregorian::date date1 = from_iso_string("20120602T000000").date();
  boost::gregorian::date date2 = from_iso_string("20120530T000000").date();
  boost::gregorian::date date3 = from_iso_string("19900101T000000").date();
  boost::gregorian::date date4 = from_iso_string("20200101T000000").date();
  EXPECT_TRUE(parser.isActive(trip2Service["AB1"], serviceActivity, date1));
  EXPECT_TRUE(parser.isActive(trip2Service["AAMV1"], serviceActivity, date1));
  EXPECT_FALSE(parser.isActive(trip2Service["AAMV1"], serviceActivity, date2));
  EXPECT_FALSE(parser.isActive(trip2Service["AAMV1"], serviceActivity, date3));
  EXPECT_FALSE(parser.isActive(trip2Service["AAMV1"], serviceActivity, date4));
}

// _____________________________________________________________________________
TEST_F(GtfsParserTest, stops_txt) {
  TransitNetwork network;
  string filename = tmpDir + "StopsTest.TMP.txt";
  std::ofstream file(filename.c_str());
  ASSERT_TRUE(file.is_open());
  file << "stop_id,stop_name,stop_desc,stop_lat,stop_lon,zone_id,stop_url\n"
       << "StationA,North Ave / N A Ave (Demo),,36.914944,-116.761472,,\n"
       << "StationB,Doing Ave / D Ave N (Demo),,36.909489,-116.768242,,\n"
       << "StationD,Blabla,,36.905697,-116.76218,,\n"
       << "StationE,E Main St / S Irving St (Demo),,36.905697,-116.76218,,\n";
  file.close();
  parser.parseStopsFile(filename.c_str(), &network);
  //   EXPECT_TRUE(contains(network.stopId()2indexMap, string("StationA")));
//   EXPECT_TRUE(contains(network.stopId()2indexMap, string("EMSI")));
  const Stop& s = network.stop(network.stopIndex("StationA"));
  EXPECT_EQ("North Ave / N A Ave (Demo)", s.name());
  EXPECT_NEAR(-116.761472, s.lon(), 0.001);
  EXPECT_NEAR(36.914944, s.lat(), 0.001);
}

// _____________________________________________________________________________
TEST_F(GtfsParserTest, timeStringToSeconds) {
  string timeStr = "06:14:23";
  size_t time = parser.gtfsTimeStr2Sec(timeStr);
  EXPECT_EQ(6 * 60 * 60 + 14 * 60 + 23, time);
}

// _____________________________________________________________________________
TEST_F(GtfsParserTest, frequencies_txt) {
  // Write simple frequency file.
  string filename = tmpDir + "FrequenciesTest.TMP.txt";
  FILE* file = fopen(filename.c_str(), "w");
  ASSERT_TRUE(file != NULL);
  fprintf(file,
          "trip_id,start_time,end_time,headway_secs\n"
          "STBA,6:00:00,22:00:00,1800\n"
          "CITY1,6:00:00,7:59:59,1800\n"
          "CITY2,6:00:00,7:59:59,1800\n"
          "CITY1,8:00:00,9:59:59,600\n"
          "CITY2,8:00:00,9:59:59,600\n"
          "CITY1,10:00:00,15:59:59,1800\n"
          "CITY2,10:00:00,15:59:59,1800\n"
          "CITY1,16:00:00,18:59:59,600\n"
          "CITY2,16:00:00,18:59:59,600\n"
          "CITY1,19:00:00,22:00:00,1800\n"
          "CITY2,19:00:00,22:00:00,1800\n");
  fclose(file);
  // TODO(jonas): enable this test when we support frequencies again
  TEST_NOT_IMPLEMENTED
  frequencies = parser.parseFrequenciesFile(filename.c_str());
  EXPECT_EQ(1800, frequencies["STBA"][0].frequency);
  EXPECT_EQ(21600, frequencies["STBA"][0].start);
  EXPECT_EQ(79200, frequencies["STBA"][0].finish);

  EXPECT_EQ(600, frequencies["CITY1"][1].frequency);
  EXPECT_EQ(28800, frequencies["CITY1"][1].start);
  EXPECT_EQ(35999, frequencies["CITY1"][1].finish);

  EXPECT_EQ(1800, frequencies["CITY2"][4].frequency);
  EXPECT_EQ(68400, frequencies["CITY2"][4].start);
  EXPECT_EQ(79200, frequencies["CITY2"][4].finish);
  TEST_NOT_IMPLEMENTED
  // TODO(jonas): Test generateStartTimes();
}

// _____________________________________________________________________________
TEST_F(GtfsParserTest, stop_times_txt) {
  string filename = tmpDir + "StopTimesTest.TMP.txt";
  // Write a stop_times file
  std::ofstream file(filename.c_str());
  file << "trip_id,arrival_time,departure_time,stop_id,stop_sequence,"
       << "stop_headsign,pickuptype(),drop_off_time,shape_dist_traveled\n"
       << "TRIP1,0:00:00,0:05:00,StationA,1,,,,\n"
       << "TRIP1,0:15:00,0:20:00,StationB,1,,,,\n"
       << "TRIP2,0:00:00,0:02:00,StationD,1,,,,\n"
       << "TRIP2,0:10:00,0:12:00,StationB,1,,,,\n"
       << "TRIP2,0:20:00,0:22:00,StationE,1,,,,\n";
  file.close();

  TransitNetwork network;
  parser.parseStopsFile(tmpDir + "StopsTest.TMP.txt", &network);
  vector<Trip> trips = parser.parseStopTimesFile(filename, network);
  EXPECT_EQ(2, trips.size());
  EXPECT_EQ(2, trips[0].size());
  EXPECT_EQ(3, trips[1].size());
  Trip t1("TRIP2");
  t1.addStop(0, 2*60, network.stopIndex("StationD"));
  t1.addStop(10*60, 12*60, network.stopIndex("StationB"));
  t1.addStop(20*60, 22*60, network.stopIndex("StationE"));
  EXPECT_EQ(trips[1].time(), t1.time());
  EXPECT_EQ(trips[1].stops(), t1.stops());
  EXPECT_EQ(trips[1].id(), "TRIP2");
}

// _____________________________________________________________________________
/* A minimal transit network with two buses TRIP1 and TRIP2:
 *       E
 *       ^
 *       |
 * A --> B --> C
 *       ^
 *       |
 *       D
 * TRIP1 A-B-C takes 10 minutes between two stops and waits for 5 minutes at
 * each stop.
 * TRIP2 D-B-E takes 8 minutes ... and 2 minutes ...
 * Each trip goes exactly one times a day.
 */
TEST_F(GtfsParserTest, parseFromGtfsFiles) {
  TransitNetwork network =
      parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                  "20111128T000000", "20111128T235959");

  int tOffset = getDateOffsetSeconds(str2time("20111128T000000"));

  ASSERT_EQ(18, network.numNodes());
  ASSERT_EQ(network.numNodes(), network.adjacencyLists().size());
  ASSERT_EQ(23, network.numArcs());
  // Contains
  //  - node with station B, type transit, time 0:12 ?
  //  - has this node an arc to node with station B,
  //    type transfer and time 0:17 ?
  bool containsB = false;
  bool existsArcBB = false;

  // Exists node with station C, type departure, time = 0:35 ?
  bool containsC = false;

  // Old test style, do not copy this. Try to use readable gmock expectations!
  for (size_t i = 0; i < network.numNodes(); i++) {
    if (network.node(i).stop() == 1 &&
        network.node(i).type() == Node::TRANSFER &&
        network.node(i).time() == 12 * 60 + tOffset) {
      containsB = true;
      for (size_t j = 0; j < network.adjacencyList(i).size(); j++) {
        size_t headNodeId = network.adjacencyList(i)[j].destination();
        if (network.node(headNodeId).stop() == 1 &&
            network.node(headNodeId).type() == Node::TRANSFER &&
            network.node(headNodeId).time() == 17 * 60 + tOffset) {
          existsArcBB = true;
        }
      }
    }
    if (network.node(i).stop() == 2 &&
        network.node(i).type() == Node::DEPARTURE &&
        network.node(i).time() == 35 * 60 + tOffset) {
      containsC = true;
    }
  }

  EXPECT_EQ(true, containsB);
  EXPECT_EQ(true, existsArcBB);
  EXPECT_EQ(true, containsC);

  // Check for connecting arc between TRIP2 and TRIP1 at stop B
  // Search node indices:
  int from = -1, to = -1;
  for (size_t i = 0; i < network.numNodes(); i++) {
    const Node& n = network.node(i);
    if (n.stop() == 1 && n.time() < 15*60 + tOffset &&
        n.type() == Node::TRANSFER)
      from = i;
    if (n.stop() == 1 && n.time() > 15*60 + tOffset &&
        n.type() == Node::TRANSFER)
      to = i;
  }
  ASSERT_NE(-1, from);
  ASSERT_NE(-1, to);
  Arc connectingArc(to, 5 * 60, 0);
  // Check if the arc is right:
  ASSERT_THAT(network.adjacencyList(from), Contains(connectingArc));
}

// _____________________________________________________________________________
TEST_F(GtfsParserTest, removeInterTripArcs) {
  TransitNetwork network =
      parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                  "20120501T000000", "20120501T230000");
  ASSERT_EQ(18, network.numNodes());
  ASSERT_EQ(23, network.numArcs());
  parser.removeInterTripArcs(&network);
  EXPECT_EQ(18, network.numNodes());
  EXPECT_EQ(16, network.numArcs());
}

// _____________________________________________________________________________
TEST_F(GtfsParserTest, loadSaveNonexistingFile) {
  TransitNetwork network;
  parser.load("", &network);
  parser.save(network, "");

  // This test ensures that reading and writing from/to non-existing files does
  // not kill the running program.
  ASSERT_TRUE(true);
}

// _____________________________________________________________________________
TEST_F(GtfsParserTest, serialization) {
  // parse and serialize a network
  TransitNetwork network =
      parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                  "20111128T000000", "20111128T235959");
  network.preprocess();
  // get the distance between B and D to compute a non-empty walking graph
  const Stop& B = network.stop(1);
  const Stop& D = network.stop(3);
  float dist = greatCircleDistance(B.lat(), B.lon(), D.lat(), D.lon());
  network.buildWalkingGraph(dist * 1.01);  // overwrite existing walking graph
  parser.save(network, "local/serializedNetwork.TMP.bin");

  const Stop* s1 = network.findNearestStop(0, 0);

  // deserialize the binary and check for equality
  TransitNetwork loadedNetwork;
  parser.load("local/serializedNetwork.TMP.bin", &loadedNetwork);
  const Stop* s2 = loadedNetwork.findNearestStop(0, 0);

  ASSERT_THAT(loadedNetwork._nodes, ContainerEq(network._nodes));
  // ASSERT_THAT(loadedNetwork.stops_, ContainerEq(network.stops_));
  ASSERT_THAT(loadedNetwork._adjacencyLists,
              ContainerEq(network._adjacencyLists));
  ASSERT_EQ(*s1, *s2);
  ASSERT_THAT(loadedNetwork._walkwayLists, ContainerEq(network._walkwayLists));
}

// _____________________________________________________________________________
TEST_F(GtfsParserTest, nearestStop) {
  TransitNetwork network =
      parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                  "20111128T000000", "20111128T235959");
  network.preprocess();

  // a position in the west of the world, should return the 'West'-Stop which
  // has three nodes (arrival, transfer, departure)
  const Stop* s = network.findNearestStop(0.1, -25.0);
  EXPECT_EQ("West", s->name());
  EXPECT_EQ("A", s->id());
  EXPECT_EQ(0.0f, s->lat());
  EXPECT_EQ(-10.0f, s->lon());
  EXPECT_THAT(s->getNodeIndices(), Contains(0));
  EXPECT_THAT(s->getNodeIndices(), Contains(1));
  EXPECT_THAT(s->getNodeIndices(), Contains(2));

  // a position between multiple nodes, should return the 'Central'-Stop which
  // has three nodes (arrival, transfer, departure) that are different from the
  // nodes of the first Stop
  const Stop* s2 = network.findNearestStop(4.9, 4.9);
  EXPECT_EQ("Center Station", s2->name());
  EXPECT_EQ("BC", s2->id());
  EXPECT_EQ(0.0f, s2->lat());
  EXPECT_EQ(0.0f, s2->lon());
  EXPECT_THAT(s2->getNodeIndices(), Contains(3));
  EXPECT_THAT(s2->getNodeIndices(), Contains(4));
  EXPECT_THAT(s2->getNodeIndices(), Contains(5));
}

// _____________________________________________________________________________
TEST_F(GtfsParserTest, findNearestStop) {
  TransitNetwork network =
      parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                  "20111128T000000", "20111128T235959");
  network.preprocess();

  const Stop* s = network.findNearestStop(0.1, -25.0);
  EXPECT_EQ("West", s->name());
  EXPECT_EQ("A", s->id());
  EXPECT_EQ(0.0f, s->lat());
  EXPECT_EQ(-10.0f, s->lon());

  const Stop* s2 = network.findNearestStop(4.9, 4.9);
  EXPECT_EQ("Center Station", s2->name());
  EXPECT_EQ("BC", s2->id());
  EXPECT_EQ(0.0f, s2->lat());
  EXPECT_EQ(0.0f, s2->lon());
}

// _____________________________________________________________________________
TEST_F(GtfsParserTest, isValidTimePeriod) {
  ASSERT_TRUE(parser.isValidTimePeriod("20111128T000000", "20111128T235959"));
  ASSERT_TRUE(parser.isValidTimePeriod("20111128T000000", "20151128T235959"));
  ASSERT_FALSE(parser.isValidTimePeriod("20111128T000000", "19901128T235959"));
  ASSERT_FALSE(parser.isValidTimePeriod("NASENRING", "BAUMKUCHEN"));
}

// _____________________________________________________________________________
TEST_F(GtfsParserTest, parseMultipleDays) {
  // Parses the simple test network for multiple days and checks the connections
  // check if there is an arc from A,transfer@friday to A,transfer@saturday.
  TransitNetwork network =
      parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                  "20111216T000000", "20111217T235959");
//   std::cout << network.debugString() << std::endl;
  // find the node index and the node index of the successor
  int fromIndex(-1), toIndex(-1);
  Node from(0, Node::TRANSFER, str2time("20111216T000200"));
  Node   to(0, Node::TRANSFER, str2time("20111217T000200"));
  for (size_t i = 0; i < network.numNodes(); i++) {
    if (network.node(i) == from)
      fromIndex = i;
    if (network.node(i) == to)
      toIndex = i;
  }
  ASSERT_NE(-1, fromIndex);
  ASSERT_NE(-1, toIndex);
  // check for the arc
  int deltaT = to.time() - from.time();
  EXPECT_THAT(network.adjacencyList(fromIndex),
              Contains(Arc(toIndex, deltaT, 0)));
  EXPECT_EQ(36, network.numNodes());
  EXPECT_EQ(51, network.numArcs());

  // Check if the node indices for each stop are ordered ascending according to
  // the timestamp of the node.
  const vector<int> nodeIndicesStopA = network.stop(0).getNodeIndices();
  ASSERT_EQ(6, nodeIndicesStopA.size());
  for (size_t i = 0; i < nodeIndicesStopA.size() - 1; i++) {
    EXPECT_TRUE(network.node(nodeIndicesStopA[i]).time() <=
                network.node(nodeIndicesStopA[i+1]).time());
  }

  const vector<int> nodeIndicesStopB = network.stop(1).getNodeIndices();
  ASSERT_EQ(12, nodeIndicesStopB.size());
  for (size_t i = 0; i < nodeIndicesStopB.size() - 1; i++) {
    EXPECT_TRUE(network.node(nodeIndicesStopB[i]).time() <=
                network.node(nodeIndicesStopB[i+1]).time());
  }
}

// _____________________________________________________________________________
TEST_F(GtfsParserTest, parseMultipleDaysWithGap) {
  // Parse a period where there is a service gap of one day.
  TransitNetwork network =
      parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                  "20120602T000000", "20120604T235959");

  EXPECT_EQ(2 * 18, network.numNodes());
  EXPECT_TRUE(2 * 23 < network.numArcs() && 3 * 23 > network.numArcs());
}

// _____________________________________________________________________________
TEST_F(GtfsParserTest, parseMultipleNetworks) {
  // First of all test whether we can do this at all.
  vector<string> gtfsDirs = {"test/data/simple-parser-test-case/",
                             "test/data/walk-test-case1/"};
  TransitNetwork compare1 = parser.createTransitNetwork(gtfsDirs[0],
                                                        "20120531T000000",
                                                        "20120531T235959");
  TransitNetwork compare2 = parser.createTransitNetwork(gtfsDirs[1],
                                                        "20120531T000000",
                                                        "20120531T235959");

  TransitNetwork combined1 = parser.createTransitNetwork(gtfsDirs,
                                                         "20120531T000000",
                                                         "20120531T235959");
  EXPECT_EQ(compare1.numNodes() + compare2.numNodes(), combined1.numNodes());
  EXPECT_EQ(compare1.numArcs() + compare2.numArcs(), combined1.numArcs());

  // Now check if two networks with overlapping stops merge properly.
  vector<string> gtfsDirs2 = {"test/data/simple-parser-test-case/",
                              "test/data/simple-parser-test-case/"};
  TransitNetwork combined2 = parser.createTransitNetwork(gtfsDirs2,
                                                         "20120531T000000",
                                                         "20120531T235959");
  EXPECT_EQ(2 * compare1.numNodes(), combined2.numNodes());
  EXPECT_EQ(2 * compare1.numArcs() + 5, combined2.numArcs());
  EXPECT_EQ(compare1.numStops(), combined2.numStops());
}

// _____________________________________________________________________________
TEST_F(GtfsParserTest, KDTreeTest_buildKdtreeFromStops) {
  TransitNetwork network =
      parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                  "20111216T000000", "20111217T235959");
  // Tests KDTree with a collection of stops.
  StopTree tree;
  for (size_t i = 0; i < network.numStops(); i++) {
    tree.insert(StopTreeNode(network.stop(i)));
  }
  tree.optimise();

  // Create a reference node.
  StopTreeNode reference;
  reference.pos[0] = 0;
  reference.pos[1] = 0;
  // Search for nodes in the range 10.0 around the reference.
  vector<StopTreeNode> results;
  tree.find_within_range(reference, 10.0f,
                   std::back_insert_iterator<vector<StopTreeNode> >(results));
  vector<Stop> resultNodes;
  for (size_t i = 0; i < results.size(); i++) {
    resultNodes.push_back(*(results[i].stopPtr));
  }
//   EXPECT_EQ(3, resultNodes.size());
  EXPECT_THAT(resultNodes, Contains(Stop("A", "West", 0.0, -10.0)));
  EXPECT_THAT(resultNodes, Contains(Stop("BC", "Center Station", 0.0, 0.0)));
  EXPECT_THAT(resultNodes, Contains(Stop("D", "South", -10.0, 0.0)));
}

// _____________________________________________________________________________
TEST_F(GtfsParserTest, walkingGraphConstruction) {
  // get the distance between B and D to compute a non-empty walking graph
  const Stop& B = simple_network_two_days.stop(1);
  const Stop& D = simple_network_two_days.stop(3);
  float dist = greatCircleDistance(B.lat(), B.lon(), D.lat(), D.lon());  // m
  int   time = dist / (5000. / 60. / 60.);  // sec

  simple_network_two_days.buildKdtreeFromStops();
  // set up the walking graph
  simple_network_two_days.buildWalkingGraph(dist * 1.01);

  ASSERT_EQ(simple_network_two_days._walkwayLists.size(),
            simple_network_two_days.numStops());
  EXPECT_THAT(simple_network_two_days.walkwayList(1),
              Contains(Arc(3, time, 1)));
  EXPECT_THAT(simple_network_two_days.walkwayList(3),
              Contains(Arc(1, time, 1)));
}

// _____________________________________________________________________________
TEST_F(GtfsParserTest, walkingGraphNonReflexive) {
  // ENSURE THAT THE WALKING GRAPH DOES NOT CONTAIN REFLEXIVE ARCS V -> V
  simple_network_two_days.buildKdtreeFromStops();
  simple_network_two_days.buildWalkingGraph(10);

  ASSERT_EQ(simple_network_two_days._walkwayLists.size(),
            simple_network_two_days.numStops());
  EXPECT_EQ(0, simple_network_two_days.walkwayList(0).size());
  EXPECT_EQ(0, simple_network_two_days.walkwayList(1).size());
  EXPECT_EQ(0, simple_network_two_days.walkwayList(2).size());
  EXPECT_EQ(0, simple_network_two_days.walkwayList(3).size());
  EXPECT_EQ(0, simple_network_two_days.walkwayList(4).size());
}
