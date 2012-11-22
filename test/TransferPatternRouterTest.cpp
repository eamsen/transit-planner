// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include <gmock/gmock.h>
#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <utility>
#include "./GtestUtil.h"
#include "../src/TransferPatternRouter.h"
#include "../src/DirectConnection.h"
#include "../src/Dijkstra.h"
#include "../src/Utilities.h"
#include "../src/GtfsParser.h"
#include "../src/Command.h"

using std::cout;
using std::endl;
using ::testing::ElementsAre;
using ::testing::Contains;

class TransferPatternRouterTest : public ::testing::Test {
 public:
  void SetUp() {
    log.enabled(false);
  }
  void TearDown() {}
  Logger log;
};


/*     (50,0)    (10,1)     (0,0)    (50,0)
 * s1---------n1--------n2--------n3--------t2 @(100,1) <- Transfer Pattern?
 *  \
 *   \ (60,0)
 *    \_______n4________n5________n6________t1 @(90,1)
 *               (10,1)     (0,0)    (20,0)
 *
 * s2_______________________________________t3 @(150,0)
 *                   (150,0)
*/
TEST_F(TransferPatternRouterTest, computeTransferPatterns) {
  TransitNetwork network;
  Stop stopStart("start", "stopName", 100, 100);
  Stop stopInter1("inter1", "stopName2", 1000, 100);
  Stop stopInter2("inter2", "stopName2", 100, 1000);
  Stop stopTarget("target", "stopName3", 1000, 1000);
  network.addStop(stopStart);
  network.addStop(stopInter1);
  network.addStop(stopInter2);
  network.addStop(stopTarget);

  int s1 = network.addTransitNode(network.stopIndex("start"),
                                  Node::DEPARTURE, 0);
  int s2 = network.addTransitNode(network.stopIndex("start"),
                                  Node::DEPARTURE, 0);
  int n1 = network.addTransitNode(network.stopIndex("inter1"),
                                  Node::ARRIVAL, 50*60);
  int n2 = network.addTransitNode(network.stopIndex("inter1"),
                                  Node::TRANSFER, 60*60);
  int n3 = network.addTransitNode(network.stopIndex("inter1"),
                                  Node::DEPARTURE, 60*60);
  int n4 = network.addTransitNode(network.stopIndex("inter2"),
                                  Node::ARRIVAL, 60*60);
  int n5 = network.addTransitNode(network.stopIndex("inter2"),
                                  Node::TRANSFER, 70*60);
  int n6 = network.addTransitNode(network.stopIndex("inter2"),
                                  Node::DEPARTURE, 70*60);
  int t1 = network.addTransitNode(network.stopIndex("target"),
                                  Node::ARRIVAL, 90*60);
  int t2 = network.addTransitNode(network.stopIndex("target"),
                                  Node::ARRIVAL, 100*60);
  int t3 = network.addTransitNode(network.stopIndex("target"),
                                  Node::ARRIVAL, 150*60);

  network.addTransitNode(network.stopIndex("target"), Node::DEPARTURE, 200*60);
  network.addTransitNode(network.stopIndex("target"), Node::DEPARTURE, 200*60);
  network.addTransitNode(network.stopIndex("target"), Node::DEPARTURE, 200*60);

  network.addArc(s1, n1, 50*60, 0);
  network.addArc(s1, n4, 60*60, 0);
  network.addArc(n1, n2, 10*60, 1);
  network.addArc(n2, n3, 0, 0);
  network.addArc(n3, t2, 40*60, 0);
  network.addArc(n4, n5, 10*60, 1);
  network.addArc(n5, n6, 0, 0);
  network.addArc(n6, t1, 20*60, 0);
  network.addArc(s2, t3, 150*60, 0);

  network.preprocess();

  TransferPatternRouter router(network);
  router.logger(&log);
  HubSet hubs;
  TPDB tpdb;
  tpdb.init(network.numStops(), hubs);
  router.transferPatternsDB(tpdb);
  set<vector<int> > patterns =
      router.computeTransferPatterns(network, network.stopIndex("start"), hubs);
  for (auto it = patterns.begin(); it != patterns.end(); ++it)
    tpdb.addPattern(*it);
  for (auto it = patterns.begin(); it != patterns.end(); ++it)
    tpdb.addPattern(*it);
  router.transferPatternsDB(tpdb);

  vector<int> pattern1 = {network.stopIndex("start"),
                          network.stopIndex("inter2"),
                          network.stopIndex("target")};

  vector<int> pattern2 = {network.stopIndex("start"),
                          network.stopIndex("inter1"),
                          network.stopIndex("target")};

  vector<int> pattern3 = {network.stopIndex("start"),
                          network.stopIndex("target")};

  vector<int> pattern4 = {network.stopIndex("start"),
                          network.stopIndex("inter1")};

  vector<int> pattern5 = {network.stopIndex("start"),
                          network.stopIndex("inter2")};

  vector<int> pattern6 = {network.stopIndex("start")};

  set<vector<int> > startTargetPatterns =
      router.generateTransferPatterns(network.stopIndex("start"),
                                      network.stopIndex("target"));
  set<vector<int> > startInter1Patterns =
      router.generateTransferPatterns(network.stopIndex("start"),
                                      network.stopIndex("inter1"));
  set<vector<int> > startInter2Patterns =
      router.generateTransferPatterns(network.stopIndex("start"),
                                      network.stopIndex("inter2"));

  EXPECT_EQ(3, startTargetPatterns.size());
  EXPECT_THAT(startTargetPatterns, Contains(pattern1));
  EXPECT_THAT(startTargetPatterns, Contains(pattern2));
  EXPECT_THAT(startTargetPatterns, Contains(pattern3));
  EXPECT_EQ(1, startInter1Patterns.size());
  EXPECT_THAT(startInter1Patterns, Contains(pattern4));
  EXPECT_EQ(1, startInter2Patterns.size());
  EXPECT_THAT(startInter2Patterns, Contains(pattern5));
}

//       (100,0)      (10,1)       (0,0)         (90,0)
// s2-------------n4----------n5----------n6----------------t2 @(200,1)
//                     i  n  t  e  r  2
//
//
//       (50,0)       (10,1)       (0,0)          (40,0)
// s1-------------n1---------n2-----------n3----------------t1 @(100,1)
//                     i  n  t  e  r  1
//
TEST_F(TransferPatternRouterTest, arrivalLoop_DifferentPatterns) {
  TransitNetwork network;
  Stop stopStart("start", "stopName", 1000, 1000);
  Stop stopInter1("int1", "stopName2", 2000, 2000);
  Stop stopInter2("int2", "stopName2", 3000, 3000);
  Stop stopTarget("target", "stopName3", 4000, 4000);
  network.addStop(stopStart);
  network.addStop(stopInter1);
  network.addStop(stopInter2);
  network.addStop(stopTarget);

  int s2 = network.addTransitNode(network.stopIndex("start"),
                                  Node::DEPARTURE, 0);
  int s1 = network.addTransitNode(network.stopIndex("start"),
                                  Node::DEPARTURE, 50*60);
  int n1 = network.addTransitNode(network.stopIndex("int1"),
                                  Node::ARRIVAL, 100*60);
  int n4 = network.addTransitNode(network.stopIndex("int2"),
                                  Node::ARRIVAL, 100*60);
  int n2 = network.addTransitNode(network.stopIndex("int1"),
                                  Node::TRANSFER, 110*60);
  int n3 = network.addTransitNode(network.stopIndex("int1"),
                                  Node::DEPARTURE, 110*60);
  int n5 = network.addTransitNode(network.stopIndex("int2"),
                                  Node::TRANSFER, 110*60);
  int n6 = network.addTransitNode(network.stopIndex("int2"),
                                  Node::DEPARTURE, 110*60);
  int t1 = network.addTransitNode(network.stopIndex("target"),
                                  Node::ARRIVAL, 150*60);
  int t2 = network.addTransitNode(network.stopIndex("target"),
                                  Node::ARRIVAL, 200*60);

  network.addArc(s1, n1, 50*60, 0);
  network.addArc(n1, n2, 10*60, 1);
  network.addArc(n2, n3, 0, 0);
  network.addArc(n3, t1, 40*60, 0);
  network.addArc(s2, n4, 100*60, 0);
  network.addArc(n4, n5, 10*60, 1);
  network.addArc(n5, n6, 0, 0);
  network.addArc(n6, t2, 90*60, 0);

  network.preprocess();

  TransferPatternRouter router(network);
  router.logger(&log);
  vector<vector<int> > transferPatterns;
  TPDB tpdb;
  router.transferPatternsDB(tpdb);
  router.computeAllTransferPatterns(&tpdb);
  set<vector<int> > patterns;
  patterns = router.generateTransferPatterns(network.stopIndex("start"),
                                             network.stopIndex("target"));

  vector<int> pattern1 = {network.stopIndex("start"), network.stopIndex("int1"),
                          network.stopIndex("target")};

  EXPECT_EQ(1, patterns.size());
  EXPECT_THAT(patterns, Contains(pattern1));
}


// Tests the temporary modification of the network done by the arrival loop.
// For computing the transfer patterns between s0 and s3
// a temporary arc is added between s3n2 and s3n1
// resulting in the patterns {[s0, s1, s3]} without pattern [s0, s2, s3]
//
// If this arc is not temporary, it affects the computations between s0 and s4:
// It's then possible to go from s0 to s4 via s1 and s3
// with only one transfer between at s2. This should not be possible.
//
// Expected result for s0 to s4 should be [s0, s2, s4]
//
//
//       (100,0)     (10,1)    (0,0)     (90,0)     (10,0)     (50,0)
// s0n2----------s2n1------s2n2-----s2n3--------s3n2------s3s3--------s4n1
// @0            @100       @110          @110          @200          @260
//
//
//
//       (50,0)      (10,1)    (0,0)     (40,0)
// s0n1----------s1n1------s1n2-----s1n3--------s3n1
// @50           @100      @110     @110        @150
//
TEST_F(TransferPatternRouterTest, arrivalLoop_StopInterference) {
  TransitNetwork network;
  Stop stop0("s0", "s0", 1000, 1000);
  Stop stop1("s1", "s1", 2000, 2000);
  Stop stop2("s2", "s2", 3000, 3000);
  Stop stop3("s3", "s3", 4000, 4000);
  Stop stop4("s4", "s4", 5000, 5000);
  network.addStop(stop0);
  network.addStop(stop1);
  network.addStop(stop2);
  network.addStop(stop3);
  network.addStop(stop4);

  int s0n2 = network.addTransitNode(network.stopIndex("s0"),
                                    Node::DEPARTURE, 0);
  int s0n1 = network.addTransitNode(network.stopIndex("s0"),
                                    Node::DEPARTURE, 50*60);
  int s1n1 = network.addTransitNode(network.stopIndex("s1"),
                                    Node::ARRIVAL, 100*60);
  int s2n1 = network.addTransitNode(network.stopIndex("s2"),
                                    Node::ARRIVAL, 100*60);
  int s1n2 = network.addTransitNode(network.stopIndex("s1"),
                                    Node::TRANSFER, 110*60);
  int s1n3 = network.addTransitNode(network.stopIndex("s1"),
                                    Node::DEPARTURE, 110*60);
  int s2n2 = network.addTransitNode(network.stopIndex("s2"),
                                    Node::TRANSFER, 110*60);
  int s2n3 = network.addTransitNode(network.stopIndex("s2"),
                                    Node::DEPARTURE, 110*60);
  int s3n1 = network.addTransitNode(network.stopIndex("s3"),
                                    Node::ARRIVAL, 150*60);
  int s3n2 = network.addTransitNode(network.stopIndex("s3"),
                                    Node::ARRIVAL, 200*60);
  int s3n3 = network.addTransitNode(network.stopIndex("s3"),
                                    Node::DEPARTURE, 210*60);
  int s4n1 = network.addTransitNode(network.stopIndex("s4"),
                                    Node::ARRIVAL, 260*60);

  network.addArc(s0n1, s1n1, 50*60, 0);
  network.addArc(s1n1, s1n2, 10*60, 1);
  network.addArc(s1n2, s1n3, 0, 0);
  network.addArc(s1n3, s3n1, 40*60, 0);
  network.addArc(s0n2, s2n1, 100*60, 0);
  network.addArc(s2n1, s2n2, 10*60, 1);
  network.addArc(s2n2, s2n3, 0, 0);
  network.addArc(s2n3, s3n2, 90*60, 0);
  network.addArc(s3n2, s3n3, 10*60, 0);
  network.addArc(s3n3, s4n1, 50*60, 0);

  network.preprocess();

  TransferPatternRouter router(network);
  router.logger(&log);

  TPDB tpdb;
  router.transferPatternsDB(tpdb);
  router.computeAllTransferPatterns(&tpdb);

  vector<int> pattern03 = {network.stopIndex("s0"), network.stopIndex("s1"),
                           network.stopIndex("s3")};
  set<vector<int> > patterns03;
  patterns03 = router.generateTransferPatterns(network.stopIndex("s0"),
                                               network.stopIndex("s3"));

  EXPECT_EQ(1, patterns03.size());
  EXPECT_THAT(patterns03, Contains(pattern03));

  vector<int> pattern04 = {network.stopIndex("s0"), network.stopIndex("s2"),
                           network.stopIndex("s4")};
  set<vector<int> > patterns04;
  patterns04 = router.generateTransferPatterns(network.stopIndex("s0"),
                                               network.stopIndex("s4"));
  EXPECT_EQ(1, patterns04.size());
  EXPECT_THAT(patterns04, Contains(pattern04));
}


//                                              (90,0)
//                                       t4-----------------n3 @(110,2)
//                                    (200,2)               |
//                      ->ArrivalLoop:(200,0)               | (0,0)
//                                                          |
//                                                          n2 (110,2)
//                                                          |
//                                                          | (10,1)
//      (50,0)       (10,1)       (0,0)          (40,0)     |
// s1-------------t1---------t2-----------t3----------------n1 (100,1)
//              (50,0)     (60,1)         (60,1)
//
// Tests, whether the arrival loop algorithm removes suboptimal paths.
TEST_F(TransferPatternRouterTest, arrivalLoop_unnecessaryStop) {
  TransitNetwork network;
  Stop stopStart("start", "stopName", 100, 100);
  Stop stopInter("inter", "stopName2", 1000, 100);
  Stop stopTarget("target", "stopName3", 100, 1000);
  network.addStop(stopStart);
  network.addStop(stopInter);
  network.addStop(stopTarget);

  int s1 = network.addTransitNode(network.stopIndex("start"),
                                  Node::DEPARTURE, 0);
  int t1 = network.addTransitNode(network.stopIndex("target"),
                                  Node::ARRIVAL, 50*60);
  int t2 = network.addTransitNode(network.stopIndex("target"),
                                  Node::TRANSFER, 60*60);
  int t3 = network.addTransitNode(network.stopIndex("target"),
                                  Node::DEPARTURE, 60*60);
  int n1 = network.addTransitNode(network.stopIndex("inter"),
                                  Node::ARRIVAL, 100*60);
  int n2 = network.addTransitNode(network.stopIndex("inter"),
                                  Node::TRANSFER, 110*60);
  int n3 = network.addTransitNode(network.stopIndex("inter"),
                                  Node::DEPARTURE, 110*60);
  int t4 = network.addTransitNode(network.stopIndex("target"),
                                  Node::ARRIVAL, 200*60);

  network.addArc(s1, t1, 50*60, 0);
  network.addArc(t1, t2, 10*60, 1);
  network.addArc(t2, t3, 0, 0);
  network.addArc(t3, n1, 40*60, 0);
  network.addArc(n1, n2, 10*60, 1);
  network.addArc(n2, n3, 0, 0);
  network.addArc(n3, t4, 90*60, 0);

  network.preprocess();

  TransferPatternRouter router(network);
  router.logger(&log);
  HubSet hubs;
  TPDB tpdb;
  tpdb.init(network.numStops(), hubs);
  router.transferPatternsDB(tpdb);
  set<vector<int> > patterns =
      router.computeTransferPatterns(network, network.stopIndex("start"), hubs);
  for (auto it = patterns.begin(); it != patterns.end(); ++it)
    tpdb.addPattern(*it);
  vector<int> pattern1 = {network.stopIndex("start"),
                          network.stopIndex("target")};

  const TPDB& db = *router.transferPatternsDB();
  const TPG& tpGraph = db.graph(network.stopIndex("start"));
  QueryGraph qGraph(tpGraph, network.stopIndex("target"));
  EXPECT_EQ(network.stopIndex("target"),
            qGraph.stopIndex(qGraph.targetNode()));
  EXPECT_EQ(2, qGraph.size());
}

/* Tests if the arrival loop algorithm ensures transitivity of virtual arcs:
   When the arrival loop algorithm detects a pair of arrival nodes for which it
   is better to wait, this changes the label at the second node. This change
   may affect the succeeding arrival nodes.

Stops: dep ---> dest
                200
Nodes: dep3@10  --->  arr3@210
                100
       dep2@0   --->  arr2@100
                 50
       dep1@40  --->  arr1@90
 */
TEST_F(TransferPatternRouterTest, arrivalLoop_transitivity) {
  TransitNetwork network;
  Stop stopStart("dep", "startName", 100, 100);
  Stop stopTarget("dest", "targetName", 10000, 10000);
  network.addStop(stopStart);
  network.addStop(stopTarget);

  // notice the time ordering
  int dep2 = network.addTransitNode(network.stopIndex("dep"),
                                    Node::DEPARTURE, 0);
  int dep3 = network.addTransitNode(network.stopIndex("dep"),
                                    Node::DEPARTURE, 10*60);
  int dep1 = network.addTransitNode(network.stopIndex("dep"),
                                    Node::DEPARTURE, 40*60);
  int arr1 = network.addTransitNode(network.stopIndex("dest"),
                                    Node::ARRIVAL, 90*60);
  int arr2 = network.addTransitNode(network.stopIndex("dest"),
                                    Node::ARRIVAL, 100*60);
  int arr3 = network.addTransitNode(network.stopIndex("dest"),
                                    Node::ARRIVAL, 210*60);

  network.addArc(dep1, arr1,  50*60, 0);
  network.addArc(dep2, arr2, 100*60, 0);
  network.addArc(dep3, arr3, 200*60, 0);
  network.preprocess();

  // Do a Dijkstra from the stop 'start'.
  Dijkstra dijkstra(network);
  dijkstra.logger(&log);
  dijkstra.maxPenalty(10);
  QueryResult result;
  dijkstra.findShortestPath({dep1, dep2, dep3}, INT_MAX, &result);
  ASSERT_FALSE(result.matrix.at(arr1).begin()->inactive());
  ASSERT_FALSE(result.matrix.at(arr2).begin()->inactive());
  ASSERT_FALSE(result.matrix.at(arr3).begin()->inactive());

  // We want to ensure that the arrival node 'arr3' is reached via waiting from
  // 'arr1' and 'arr2'. If there was no transitivity for the algorithm, this
  // node would not have a waiting arc from 'arr2'.
  TransferPatternRouter router(network);
  router.arrivalLoop(network, &result.matrix, 1/*stopTarget*/);

  EXPECT_FALSE(result.matrix.at(arr1).begin()->inactive());
  EXPECT_TRUE(result.matrix.at(arr2).begin()->inactive());
  EXPECT_TRUE(result.matrix.at(arr3).begin()->inactive());
}


//       (50,0)      (10,1)       (0,0)         (50,0)
// s2-------------n4----------n5----------n6----------------t2 @(110,1)
//                     i  n  t  e  r  1
//
//
//       (50,0)       (10,1)       (0,0)          (40,0)
// s1-------------n1---------n2-----------n3----------------t1 @(100,1)
//                     i  n  t  e  r  1
//
// Tests, whether duplicate patterns are avoided.
TEST_F(TransferPatternRouterTest, computeTransferPatterns_duplicatePatterns) {
  TransitNetwork network;
  Stop stopStart("start", "stopName", 100, 100);
  Stop stopInter1("inter", "stopName2", 200, 200);
  Stop stopTarget("target", "stopName3", 300, 300);
  network.addStop(stopStart);
  network.addStop(stopInter1);
  network.addStop(stopTarget);

  int s1 = network.addTransitNode(network.stopIndex("start"),
                                  Node::DEPARTURE, 0);
  int n1 = network.addTransitNode(network.stopIndex("inter"),
                                  Node::ARRIVAL, 50*60);
  int n2 = network.addTransitNode(network.stopIndex("inter"),
                                  Node::TRANSFER, 60*60);
  int n3 = network.addTransitNode(network.stopIndex("inter"),
                                  Node::DEPARTURE, 60*60);
  int t1 = network.addTransitNode(network.stopIndex("target"),
                                  Node::ARRIVAL, 100*60);
  int s2 = network.addTransitNode(network.stopIndex("start"),
                                  Node::DEPARTURE, 500*60);
  int n4 = network.addTransitNode(network.stopIndex("inter"),
                                  Node::ARRIVAL, 550*60);
  int n5 = network.addTransitNode(network.stopIndex("inter"),
                                  Node::TRANSFER, 560*60);
  int n6 = network.addTransitNode(network.stopIndex("inter"),
                                  Node::DEPARTURE, 560*60);
  int t2 = network.addTransitNode(network.stopIndex("target"),
                                  Node::ARRIVAL, 610*60);
  network.addTransitNode(network.stopIndex("target"), Node::DEPARTURE, 800*60);
  network.addTransitNode(network.stopIndex("target"), Node::DEPARTURE, 800*60);

  network.addArc(s1, n1, 50*60, 0);
  network.addArc(n1, n2, 10*60, 1);
  network.addArc(n2, n3, 0, 0);
  network.addArc(n3, t1, 40*60, 0);
  network.addArc(s2, n4, 50*60, 0);
  network.addArc(n4, n5, 10*60, 1);
  network.addArc(n5, n6, 0, 0);
  network.addArc(n6, t2, 50*60, 0);

  network.preprocess();

  TransferPatternRouter router(network);
  router.logger(&log);
  HubSet hubs;
  TPDB tpdb;
  tpdb.init(network.numStops(), hubs);
  router.transferPatternsDB(tpdb);
  set<vector<int> > patterns =
      router.computeTransferPatterns(network, network.stopIndex("start"), hubs);
  for (auto it = patterns.begin(); it != patterns.end(); ++it)
    tpdb.addPattern(*it);
  vector<int> pattern1 = {network.stopIndex("start"),
                          network.stopIndex("inter"),
                          network.stopIndex("target")};

  set<vector<int> > startTargetPatterns = router.generateTransferPatterns(
      network.stopIndex("start"), network.stopIndex("target"));

  EXPECT_EQ(1, startTargetPatterns.size());
  EXPECT_THAT(startTargetPatterns, Contains(pattern1));
}


/*                                                          t1 @(200)
 *
 *
 *       (50,0)       (10,1)       (0,0)          (40,0)
 * s1-------------n1---------n2-----------n3----------------n4 @(100,1)
 *                     i  n  t  e  r  1                  i n t e r 2
 *
 * Computes transfer patterns to an unconnected node
 */
TEST_F(TransferPatternRouterTest, computeTransferPatterns_noConnection) {
  TransitNetwork network;
  Stop stopStart("start", "stopName", 100, 100);
  Stop stopInter1("inter1", "stopName2", 1000, 1000);
  Stop stopInter2("inter2", "stopName2", 1000, 100);
  Stop stopTarget("target", "stopName3", 10000, 10000);
  network.addStop(stopStart);
  network.addStop(stopInter1);
  network.addStop(stopInter2);
  network.addStop(stopTarget);

  int s1 = network.addTransitNode(network.stopIndex("start"),
                                  Node::DEPARTURE, 0);
  int n1 = network.addTransitNode(network.stopIndex("inter1"),
                                  Node::ARRIVAL, 50*60);
  int n2 = network.addTransitNode(network.stopIndex("inter1"),
                                  Node::TRANSFER, 60*60);
  int n3 = network.addTransitNode(network.stopIndex("inter1"),
                                  Node::DEPARTURE, 60*60);
  int n4 = network.addTransitNode(network.stopIndex("inter2"),
                                  Node::ARRIVAL, 100*60);
  network.addTransitNode(network.stopIndex("target"), Node::ARRIVAL, 200*60);
  network.addTransitNode(network.stopIndex("target"), Node::DEPARTURE, 400*60);

  network.addArc(s1, n1, 50*60, 0);
  network.addArc(n1, n2, 10*60, 1);
  network.addArc(n2, n3, 0, 0);
  network.addArc(n3, n4, 40*60, 0);

  network.preprocess();

  TransferPatternRouter router(network);
  router.logger(&log);
  HubSet hubs;
  TPDB tpdb;
  tpdb.init(network.numStops(), hubs);
  router.transferPatternsDB(tpdb);
  set<vector<int> > patterns =
      router.computeTransferPatterns(network, network.stopIndex("start"), hubs);
  for (auto it = patterns.begin(); it != patterns.end(); ++it)
    tpdb.addPattern(*it);

  vector<int> pattern1 = {network.stopIndex("start"),
                          network.stopIndex("inter1")};

  vector<int> pattern2 = {network.stopIndex("start"),
                          network.stopIndex("inter1"),
                          network.stopIndex("inter2")};

  set<vector<int> > startTargetPatterns = router.generateTransferPatterns(
      network.stopIndex("start"), network.stopIndex("target"));

  set<vector<int> > startInter1Patterns = router.generateTransferPatterns(
      network.stopIndex("start"), network.stopIndex("inter1"));

  set<vector<int> > startInter2Patterns = router.generateTransferPatterns(
      network.stopIndex("start"), network.stopIndex("inter2"));

  EXPECT_EQ(0, startTargetPatterns.size());
  EXPECT_THAT(startInter1Patterns, Contains(pattern1));
  EXPECT_THAT(startInter2Patterns, Contains(pattern2));
}




/*     stop1
 *         @170
 *   n2____n1_________________________
 *                                    \
 *                                     \ (50,0)
 *   n3 @0                              \
 *   |                                  |
 *   |                                  |
 *   |  (50,0)                          |
 *   |                                  |
 *   |                                  |    stop3
 *   n4     stop2                       n9 @120
 *   |(10,1)                   (10,0)/  |(0,0)
 *   |                              /   |
 *  n5__n6_________________________n7___n8
 *              (50,0)              (10,1)
 *                                 @110
 */


TEST_F(TransferPatternRouterTest, computeAllTransferPatterns) {
  TransitNetwork network;
  Stop stop1("stop1", "stopName", 100, 100);
  Stop stop2("stop2", "stopName2", 1000, 1000);
  Stop stop3("stop3", "stopName3", 10000, 10000);
  network.addStop(stop1);
  network.addStop(stop2);
  network.addStop(stop3);

  int n3 = network.addTransitNode(network.stopIndex("stop1"),
                                  Node::DEPARTURE, 0);
  int n4 = network.addTransitNode(network.stopIndex("stop2"),
                                  Node::ARRIVAL, 50*60);
  int n5 = network.addTransitNode(network.stopIndex("stop2"),
                                  Node::TRANSFER, 60*60);
  int n6 = network.addTransitNode(network.stopIndex("stop2"),
                                  Node::DEPARTURE, 60*60);
  int n7 = network.addTransitNode(network.stopIndex("stop3"),
                                  Node::ARRIVAL, 110*60);
  int n8 = network.addTransitNode(network.stopIndex("stop3"),
                                  Node::TRANSFER, 120*60);
  int n9 = network.addTransitNode(network.stopIndex("stop3"),
                                  Node::DEPARTURE, 120*60);
  int n1 = network.addTransitNode(network.stopIndex("stop1"),
                                  Node::ARRIVAL, 170*60);
  int n2 = network.addTransitNode(network.stopIndex("stop1"),
                                  Node::TRANSFER, 180*60);

  network.addArc(n1, n2, 10*60, 1);
  network.addArc(n4, n5, 10*60, 1);
  network.addArc(n5, n6, 0, 0);
  network.addArc(n7, n8, 10*60, 1);
  network.addArc(n8, n9, 0, 0);
  network.addArc(n7, n9, 10*60, 0);
  network.addArc(n3, n4, 50*60, 0);
  network.addArc(n6, n7, 50*60, 0);
  network.addArc(n9, n1, 50*60, 0);

  network.preprocess();

  TransferPatternRouter router(network);
  router.logger(&log);
  TPDB tpdb;
  router.transferPatternsDB(tpdb);
  router.computeAllTransferPatterns(&tpdb);

  set<vector<int> > stop12Patterns = router.generateTransferPatterns(
                        network.stopIndex("stop1"), network.stopIndex("stop2"));
  set<vector<int> > stop13Patterns = router.generateTransferPatterns(
                        network.stopIndex("stop1"), network.stopIndex("stop3"));
  set<vector<int> > stop21Patterns = router.generateTransferPatterns(
                        network.stopIndex("stop2"), network.stopIndex("stop1"));
  set<vector<int> > stop23Patterns = router.generateTransferPatterns(
                        network.stopIndex("stop2"), network.stopIndex("stop3"));
  set<vector<int> > stop31Patterns = router.generateTransferPatterns(
                        network.stopIndex("stop3"), network.stopIndex("stop1"));
  set<vector<int> > stop32Patterns = router.generateTransferPatterns(
                        network.stopIndex("stop3"), network.stopIndex("stop2"));

  EXPECT_EQ(1, stop12Patterns.size());
  EXPECT_EQ(1, stop13Patterns.size());
  EXPECT_EQ(1, stop21Patterns.size());
  EXPECT_EQ(1, stop23Patterns.size());
  EXPECT_EQ(1, stop31Patterns.size());
  EXPECT_EQ(0, stop32Patterns.size());
}


TEST_F(TransferPatternRouterTest, Constructor) {
  // Test for the constructor of TransferPatternRouter.
  TransitNetwork network;
  GtfsParser parser;
  vector<Line> lines;
  parser.logger(&log);
  network = parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                        "20120118T000000", "20120120T000000",
                                        &lines);
  network.preprocess();

  TransferPatternRouter router(network);
  router.logger(&log);
  router.prepare(lines);

  int time1 = router._connections.query(network.stopIndex("A"),
                                        str2time("20120118T000000"),
                                        network.stopIndex("C"));
  ASSERT_EQ(30 * 60, time1);

  int time2 = router._connections.query(network.stopIndex("BC"),
                                        str2time("20120118T000000"),
                                        network.stopIndex("C"));
  ASSERT_EQ(30 * 60, time2);

  int time3 = router._connections.query(network.stopIndex("BC"),
                                        str2time("20120118T001000"),
                                        network.stopIndex("C"));
  ASSERT_EQ(20 * 60, time3);

  int time4 = router._connections.query(network.stopIndex("A"),
                                        str2time("20120118T001000"),
                                        network.stopIndex("D"));
  ASSERT_EQ(DirectConnection::INFINITE, time4);
}


TEST_F(TransferPatternRouterTest, getHubs) {
  TransitNetwork network;
  GtfsParser parser;
  parser.logger(&log);

  network =  parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                         "20111128T000000", "20111128T235959");

  network.preprocess();

  TransferPatternRouter router(network);
  router.logger(&log);
  router.prepare({});
  vector<int> seedStops;
  seedStops.push_back(network.stopIndex("A"));
  seedStops.push_back(network.stopIndex("D"));

  vector<IntPair> stopFreqs(network.numStops(), make_pair(0, 0));
  for (size_t i = 0; i < network.numStops(); ++i) {
    stopFreqs[i].first = i;
  }

  for (size_t i = 0; i < seedStops.size(); i++) {
    router.countStopFreq(seedStops[i], &stopFreqs);
  }

  std::sort(stopFreqs.begin(), stopFreqs.end(), sortStopsByImportance());
  EXPECT_EQ(network.stopIndex("BC"), stopFreqs.at(0).first);
  EXPECT_EQ(6, stopFreqs.at(0).second);
  EXPECT_EQ(4, stopFreqs.at(1).second);
  EXPECT_EQ(4, stopFreqs.at(2).second);
  EXPECT_EQ(2, stopFreqs.at(3).second);
  EXPECT_EQ(2, stopFreqs.at(4).second);
}


TEST_F(TransferPatternRouterTest, shortestPath) {
  TransitNetwork network;
  GtfsParser parser;
  vector<Line> lines;
  parser.logger(&log);
  network = parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                        "20120118T000000", "20120120T000000",
                                        &lines);
  network.preprocess();

  TransferPatternRouter router(network);
  router.logger(&log);
  router.prepare(lines);
  HubSet hubs;
  TPDB tpdb;
  tpdb.init(network.numStops(), hubs);
  router.transferPatternsDB(tpdb);

  // Compute transferPatterns without Hubs
  for (size_t i = 0; i < network.numStops(); i++) {
    set<vector<int> > patterns = router.computeTransferPatterns(network, i,
                                                                hubs);
    for (auto it = patterns.begin(); it != patterns.end(); ++it)
      tpdb.addPattern(*it);
  }

  // reflexive query
  vector<QueryResult::Path> result0;

  result0 = router.shortestPath(network.stopIndex("BC"),
                                str2time("20120118T000000"),
                                network.stopIndex("BC"));
  EXPECT_EQ(1, result0.size());
  EXPECT_EQ(0, result0.begin()->first.cost());
  vector<int> optimalPath0 = {network.stopIndex("BC")};
  EXPECT_EQ(optimalPath0, result0.begin()->second);

  // other queries
  vector<QueryResult::Path> result = router.shortestPath(network.stopIndex("A"),
                                                    str2time("20120118T000000"),
                                                    network.stopIndex("C"));
  EXPECT_EQ(1, result.size());
  LabelVec::Hnd hnd = (*(result.begin())).first;
  vector<int> path = (*(result.begin())).second;
  vector<int> optimalPath = {network.stopIndex("A"), network.stopIndex("C")};
  EXPECT_EQ(5*60 + 25*60, hnd.cost());
  EXPECT_EQ(optimalPath, path);

  result = router.shortestPath(network.stopIndex("D"),
                               str2time("20120118T000000"),
                               network.stopIndex("C"));

  ASSERT_EQ(1, result.size());
  hnd = (*(result.begin())).first;
  path = (*(result.begin())).second;
  optimalPath = {network.stopIndex("D"), network.stopIndex("BC"),
                 network.stopIndex("C")};
  EXPECT_EQ(1*60 + 29*60, hnd.cost());
  EXPECT_EQ(optimalPath, path);
}


TEST_F(TransferPatternRouterTest, shortestPath_withHubs) {
  TransitNetwork network;
  GtfsParser parser;
  vector<Line> lines;
  parser.logger(&log);
  network = parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                        "20120118T000000", "20120119T000000",
                                        &lines);
  network.preprocess();

  TransferPatternRouter router(network);
  router.logger(&log);
  router.prepare(lines);

  HubSet hubs;
  hubs.insert(network.stopIndex("BC"));
  router.hubs(hubs);
  TPDB tpdb;
  tpdb.init(network.numStops(), hubs);
  router.transferPatternsDB(tpdb);
  router.computeAllTransferPatterns(&tpdb);

  // reflexive query from hub
  vector<QueryResult::Path> result0;
  result0 = router.shortestPath(network.stopIndex("BC"),
                                str2time("20120118T000000"),
                                network.stopIndex("BC"));
  EXPECT_EQ(1, result0.size());
  EXPECT_EQ(0, result0.begin()->first.cost());
  vector<int> optimalPath0 = {network.stopIndex("BC")};
  EXPECT_EQ(optimalPath0, result0.begin()->second);

  // reflexive query from non-hub
  vector<QueryResult::Path> result01;
  result01 = router.shortestPath(network.stopIndex("D"),
                                 str2time("20120118T000000"),
                                 network.stopIndex("D"));
  EXPECT_EQ(1, result01.size());
  EXPECT_EQ(0, result01.begin()->first.cost());
  vector<int> optimalPath01 = {network.stopIndex("D")};
  EXPECT_EQ(optimalPath01, result01.begin()->second);

  // other queries
  vector<QueryResult::Path> result = router.shortestPath(network.stopIndex("A"),
                                            str2time("20120118T000000"),
                                            network.stopIndex("C"));

  EXPECT_EQ(1, result.size());
  LabelVec::Hnd hnd = (*(result.begin())).first;
  vector<int> path = (*(result.begin())).second;
  vector<int> optimalPath = {network.stopIndex("A"), network.stopIndex("C")};
  EXPECT_EQ(5*60 + 25*60, hnd.cost());
  EXPECT_EQ(path, optimalPath);

  result = router.shortestPath(network.stopIndex("D"),
                               str2time("20120118T000000"),
                               network.stopIndex("C"));

  ASSERT_EQ(1, result.size());
  hnd = result.begin()->first;
  path = result.begin()->second;
  vector<int> optimalPath2 = {network.stopIndex("D"), network.stopIndex("BC"),
                              network.stopIndex("C")};
  EXPECT_EQ(1*60 + 29*60, hnd.cost());
  EXPECT_EQ(path, optimalPath2);
}


TEST_F(TransferPatternRouterTest, shortestPath_withHubs_multipleSolutions) {
  TransitNetwork network;
  GtfsParser parser;
  vector<Line> lines;
  parser.logger(&log);
  network = parser.createTransitNetwork("test/data/query-test-case/",
                                        "20120118T000000", "20120119T000000",
                                        &lines);
  network.preprocess();

  TransferPatternRouter router(network);
  router.logger(&log);
  router.prepare(lines);

  HubSet hubs;
  hubs.insert(network.stopIndex("B"));
  router.hubs(hubs);
  TPDB tpdb;
  tpdb.init(network.numStops(), hubs);
  router.transferPatternsDB(tpdb);
  for (size_t i = 0; i < network.numStops(); i++) {
    set<vector<int> > patterns = router.computeTransferPatterns(network, i,
                                                                hubs);
    for (auto it = patterns.begin(); it != patterns.end(); ++it)
      tpdb.addPattern(*it);
  }

  vector<QueryResult::Path> result = router.shortestPath(network.stopIndex("F"),
                                            str2time("20120118T000000"),
                                            network.stopIndex("Z"));

  EXPECT_EQ(2, result.size());
  for (auto it = result.begin(); it != result.end(); it++) {
    LabelVec::Hnd hnd = (*it).first;
    vector<int> optimalPath = (*it).second;

    if (optimalPath.size() == 2) {
      vector<int> pattern1 = {network.stopIndex("F"), network.stopIndex("Z")};
      EXPECT_EQ(5 * 60 + 110 * 60, hnd.cost());
      EXPECT_EQ(0, hnd.penalty());
      EXPECT_EQ(pattern1, optimalPath);
    } else if (optimalPath.size() == 3) {
      vector<int> pattern2 = {network.stopIndex("F"), network.stopIndex("B"),
                          network.stopIndex("Z")};
      EXPECT_EQ(5 * 60 + 90 * 60, hnd.cost());
      EXPECT_EQ(1, hnd.penalty());
      EXPECT_EQ(pattern2, optimalPath);
    } else {
      EXPECT_EQ(true, false);
    }
  }
}


/* Three trains TRIP1, TRIP2, TRIP3 and a walkable way between W and B:
 *
 *    W_____B
 *   /       \
 * F --------> Z
 *
 * TRIP1 F-Z takes 120 minutes and goes at 0:00:00
 * TRIP2 F-W takes 30 minutes and goes at 0:00:00
 * TRIP3 B-Z takes 30 minutes and goes at 1:00:00
 */
TEST_F(TransferPatternRouterTest, dijkstraCompare_walkIntermediateStop) {
  // Setup test scenario:
  TransitNetwork network;
  GtfsParser parser;
  vector<Line> lines;
  parser.logger(&log);
  network = parser.createTransitNetwork("test/data/walk-test-case3/",
                                        "20120118T000000", "20120120T000000",
                                        &lines);
  network.preprocess();

  const int stopF = network.stopIndex("F");
  const int stopW = network.stopIndex("W");
  const int stopB = network.stopIndex("B");
  const int stopZ = network.stopIndex("Z");

  // Setup dijkstra:
  Dijkstra dijkstra(network);
  dijkstra.logger(&log);
  vector<int> depNodes;
  depNodes = network.findStartNodeSequence(network.stop(stopF),
                                           str2time("20120118T000000"));


  QueryResult result;
  dijkstra.startTime(str2time("20120118T000000"));
  dijkstra.findShortestPath(depNodes, stopZ, &result);
  vector<QueryResult::Path> pathsDijkstra = result.optimalPaths(network);

  // Setup transfer patterns:
  TransferPatternRouter router(network);
  router.logger(&log);
  router.prepare(lines);
  TPDB tpdb;
  router.transferPatternsDB(tpdb);
  router.computeAllTransferPatterns(&tpdb);
  // No hubs: All stops should be in the tpg
  const TPG& tpgDepStop = router.transferPatternsDB()->graph(stopF);
  EXPECT_FALSE(TPG::INVALID_NODE == tpgDepStop.destNode(stopW));
  EXPECT_FALSE(TPG::INVALID_NODE == tpgDepStop.destNode(stopB));
  EXPECT_FALSE(TPG::INVALID_NODE == tpgDepStop.destNode(stopZ));

  vector<QueryResult::Path> pathsPattern;
  pathsPattern = router.shortestPath(stopF, str2time("20120118T000000"), stopZ);

  EXPECT_EQ(2, pathsDijkstra.size());
  EXPECT_EQ(pathsDijkstra.size(), pathsPattern.size());
}


/* Two trains TRIP1, TRIP2 and a walkable way between B and Z:
 *
 *    ___B___
 *   /       \
 * F --------> Z
 *
 * TRIP1 F-Z takes 120 minutes and goes at 0:05:00
 * TRIP2 F-B takes 30 minutes and goes at 0:05:00
 *
 */
TEST_F(TransferPatternRouterTest, dijkstraCompare_walkLastStop) {
  // Setup test scenario:
  TransitNetwork network;
  GtfsParser parser;
  vector<Line> lines;
  parser.logger(&log);
  network = parser.createTransitNetwork("test/data/walk-test-case2/",
                                        "20120118T000000", "20120118T230000",
                                        &lines);
  network.preprocess();

  const int stopF = network.stopIndex("F");
  const int stopB = network.stopIndex("B");
  const int stopZ = network.stopIndex("Z");

  // Setup dijkstra:
  Dijkstra dijkstra(network);
  dijkstra.logger(&log);
  vector<int> depNodes;
  depNodes = network.findStartNodeSequence(network.stop(stopF),
                                           str2time("20120118T000000"));

  QueryResult result;
  dijkstra.startTime(str2time("20120118T000000"));
  dijkstra.findShortestPath(depNodes, stopZ, &result);
  vector<QueryResult::Path> pathsDijkstra = result.optimalPaths(network);

  // Setup transfer patterns:
  TransferPatternRouter router(network);
  router.logger(&log);
  router.prepare(lines);
  TPDB tpdb;
  router.transferPatternsDB(tpdb);
  router.computeAllTransferPatterns(&tpdb);

  // No hubs: All stops should be in the tpg
  const TPG& tpgDepStop = router._tpdb->graph(stopF);
  EXPECT_FALSE(TPG::INVALID_NODE == tpgDepStop.destNode(stopB));
  EXPECT_FALSE(TPG::INVALID_NODE == tpgDepStop.destNode(stopZ));
//   cout << tpgDepStop.debugString() << endl;
  EXPECT_EQ(4, tpgDepStop.numNodes());

  vector<QueryResult::Path> pathsPatterns;
  pathsPatterns = router.shortestPath(stopF, str2time("20120118T000000"),
                                      stopZ);

  EXPECT_EQ(2, pathsDijkstra.size());
  EXPECT_EQ(pathsDijkstra.size(), pathsPatterns.size());
}


/* Three trains TRIP1, TRIP2, TRIP3
 *
 * O ---> F ---> B ---> Z
 *
 * TRIP1 O-F takes 45 minutes and goes at 0:05:00
 * TRIP2 F-B takes 60 minutes and goes at 1:05:00
 * TRIP3 B-Z takes 30 minutes and goes at 2:20:00
 *
 * Total time: 0:05:00 - 2:50:00 = 2:45:00 = 165 min
 */
TEST_F(TransferPatternRouterTest, dijkstraCompare_timeTest_waitingForBegin) {
  // Setup test scenario:
  TransitNetwork network;
  GtfsParser parser;
  vector<Line> lines;
  parser.logger(&log);
  network = parser.createTransitNetwork("test/data/time-test-case1/",
                                        "20120118T000000", "20120120T000000",
                                        &lines);
  network.preprocess();

  const int stopO = network.stopIndex("O");
  const int stopZ = network.stopIndex("Z");

  // Setup dijkstra:
  Dijkstra dijkstra(network);
  dijkstra.logger(&log);
  vector<int> depNodes;
  depNodes = network.findStartNodeSequence(network.stop(stopO),
                                           str2time("20120118T000000"));

  QueryResult result;
  dijkstra.startTime(str2time("20120118T000000"));
  dijkstra.findShortestPath(depNodes, stopZ, &result);
  vector<QueryResult::Path> pathsDijkstra = result.optimalPaths(network);

  // Setup transfer patterns:
  TransferPatternRouter router(network);
  router.logger(&log);
  router.prepare(lines);
  TPDB tpdb;
  router.transferPatternsDB(tpdb);
  router.computeAllTransferPatterns(&tpdb);

  vector<QueryResult::Path> pathsPatterns;
  pathsPatterns = router.shortestPath(stopO,
                                      str2time("20120118T000000"), stopZ);
  ASSERT_EQ(1, pathsDijkstra.size());
  ASSERT_EQ(1, pathsPatterns.size());

  const int costDijkstra = pathsDijkstra.begin()->first.cost();
  const int costPatterns = pathsPatterns.begin()->first.cost();

  ASSERT_EQ(pathsDijkstra.size(), pathsPatterns.size());
  ASSERT_EQ(5 * 60 + 165 * 60, costDijkstra);
  ASSERT_EQ(costDijkstra, costPatterns);
  ASSERT_EQ(2, pathsDijkstra.begin()->first.penalty());
  ASSERT_EQ(2, pathsPatterns.begin()->first.penalty());
}

/* A minimal transit network with three trains TRIP1, TRIP2 and TRIP3:
 *
 *
 * F ----B----> Z
 *
 *
 * TRIP1 F-B takes 45 minutes and goes at 0:05:00
 * TRIP2 B-Z takes 30 minutes and goes at 1:05:00
 */
TEST_F(TransferPatternRouterTest, dijkstraCompare_transferTest) {
  // Setup test scenario:
  TransitNetwork network;
  GtfsParser parser;
  vector<Line> lines;
  parser.logger(&log);
  network = parser.createTransitNetwork("test/data/transfer-test-case/",
                                        "20120118T000000", "20120119T000000",
                                        &lines);
  network.preprocess();

  const int stopF = network.stopIndex("F");
  const int stopZ = network.stopIndex("Z");

  // Setup dijkstra:
  Dijkstra dijkstra(network);
  dijkstra.logger(&log);

  QueryResult result;
  vector<int> depNodes = network.findStartNodeSequence(network.stop(stopF),
                                                   str2time("20120118T000000"));
  dijkstra.startTime(str2time("20120118T000000"));
  dijkstra.findShortestPath(depNodes, stopZ, &result);
  vector<QueryResult::Path> pathsDijkstra = result.optimalPaths(network);

  // Setup transfer patterns:
  TransferPatternRouter router(network);
  router.logger(&log);
  router.prepare(lines);
  TPDB tpdb;
  router.transferPatternsDB(tpdb);
  router.computeAllTransferPatterns(&tpdb);

  vector<QueryResult::Path> pathsPatterns;
  pathsPatterns = router.shortestPath(stopF,
                                      str2time("20120118T000000"), stopZ);

  ASSERT_EQ(1, pathsDijkstra.size());
  const int costDijkstra = pathsDijkstra.begin()->first.cost();

  EXPECT_EQ(90*60, costDijkstra);
  EXPECT_EQ(1, pathsDijkstra.begin()->first.penalty());

  ASSERT_EQ(pathsDijkstra.size(), pathsPatterns.size());
  const int costPatterns = pathsPatterns.begin()->first.cost();
  EXPECT_EQ(costDijkstra, costPatterns);
  EXPECT_EQ(1, pathsPatterns.begin()->first.penalty());
}


TEST_F(TransferPatternRouterTest, dijkstraCompare_directFastConnection) {
  // Setup test scenario:
  TransitNetwork network;
  GtfsParser parser;
  vector<Line> lines;
  parser.logger(&log);
  network = parser.createTransitNetwork(
      "test/data/direct-connection-test-case/",
      "20120118T000000", "20120119T000000", &lines);
  network.preprocess();

  const int stopF = network.stopIndex("F");
  const int stopZ = network.stopIndex("Z");

  // Setup dijkstra:
  Dijkstra dijkstra(network);
  dijkstra.logger(&log);

  QueryResult result;
  vector<int> depNodes = network.findStartNodeSequence(network.stop(stopF),
                                                   str2time("20120118T000000"));
  dijkstra.startTime(str2time("20120118T000000"));
  dijkstra.findShortestPath(depNodes, stopZ, &result);
  vector<QueryResult::Path> pathsDijkstra = result.optimalPaths(network);

  // Setup transfer patterns:
  TransferPatternRouter router(network);
  router.logger(&log);
  router.prepare(lines);
  TPDB tpdb;
  router.transferPatternsDB(tpdb);
  router.computeAllTransferPatterns(&tpdb);

  vector<QueryResult::Path> pathsPatterns;
  pathsPatterns = router.shortestPath(stopF,
                                      str2time("20120118T000000"), stopZ);

  ASSERT_EQ(1, pathsDijkstra.size());
  const int costDijkstra = pathsDijkstra.begin()->first.cost();

  EXPECT_EQ(5 * 60 + 45, costDijkstra);
  EXPECT_EQ(0, pathsDijkstra.begin()->first.penalty());

  ASSERT_EQ(pathsDijkstra.size(), pathsPatterns.size());
  const int costPatterns = pathsPatterns.begin()->first.cost();
  EXPECT_EQ(costDijkstra, costPatterns);
  EXPECT_EQ(0, pathsPatterns.begin()->first.penalty());
}


/*
 *                20
 *              E-------F
 *             /20
 *     120    /
 * A --------B          C
 *  \                   |
 *   \20                |
 *    D                 G
 *
 * Dep: A; Dest: C
 *
 * Its possible to walk
 * from D to B
 * from B to C
 * from F to C
 *
 * The following problem exists (Jonas):
 * Two routes reach C by walking: ABC and ADBEFC. C has just one node-triple. In
 * the last step a walking arc is relaxed. Both routes add a label to the
 * transfer node which is assigned the cost difference between the timestamp of
 * the starting node at A (t0) and the timestamp of the final transfer node.
 * Thus they have the same cost although the time of travel is different. Due to
 * a higher number of transfers, the label of the second route is dropped and no
 * according transfer pattern can be found during precomputation.
 * Instead the reference Dijkstra finds two routes because it checks for C being
 * the target stop and adds corrected labels to the destination nodes.
 *
 * THE OLD DESCRIPTION FOLLOWS BECAUSE THERE MIGHT BE ANOTHER PROBLEM(jonas):
 * The test describes the following problem:
 * In the first expansion labels (20,0) for arr@D and (120,0) for arr@B are
 * pushed in the PQ. In the second step the label at arr@D is expanded and the
 * label (20+walk,1) for tra@B is pushed in the PQ. While the Dijkstra continues
 * the path A->B will not be continued because the the expansion of arr@B to
 * tra@B with a label candidate (120+transferBuffer,1) is 'blocked' by that
 * first label at the transfer node of B having equal penalty and less cost.
 * During backtracking from C ... // TODO(jonas+Philip)
 * That is why patterns does not find the path A,B,C because
 * the label (120,1) is not expanded.
 *
 * The correct result should be:
 *  Path 1: A-B-C
 *  Path 2: A-D-B-E-F-C
 *
 * The problem is another one:
 */
TEST_F(TransferPatternRouterTest, walkTestCase4) {
  //   TEST_NOT_IMPLEMENTED
  TransitNetwork network;
  GtfsParser parser;
  vector<Line> lines;
  parser.logger(&log);
  network = parser.createTransitNetwork("test/data/walk-test-case4/",
                                        "20120118T000000", "20120118T230000",
                                        &lines);
  network.preprocess();
//   cout << network.debugStringOfWalkingGraph() << endl;

  Stop& sA = network.stop(network.stopIndex("A"));
  Stop& sB = network.stop(network.stopIndex("B"));
  Stop& sC = network.stop(network.stopIndex("C"));
  Stop& sD = network.stop(network.stopIndex("D"));
  Stop& sE = network.stop(network.stopIndex("E"));
  Stop& sF = network.stop(network.stopIndex("F"));

  int max = network.MAX_WALKWAY_DIST;
  EXPECT_LT(max, greatCircleDistance(sA.lat(), sA.lon(), sB.lat(), sB.lon()));
  EXPECT_LT(max, greatCircleDistance(sA.lat(), sA.lon(), sC.lat(), sC.lon()));
  EXPECT_LT(max, greatCircleDistance(sA.lat(), sA.lon(), sD.lat(), sD.lon()));
  EXPECT_LT(max, greatCircleDistance(sA.lat(), sA.lon(), sE.lat(), sE.lon()));
  EXPECT_LT(max, greatCircleDistance(sA.lat(), sA.lon(), sF.lat(), sF.lon()));

  EXPECT_GT(max, greatCircleDistance(sB.lat(), sB.lon(), sC.lat(), sC.lon()));
  EXPECT_GT(max, greatCircleDistance(sB.lat(), sB.lon(), sD.lat(), sD.lon()));
  EXPECT_LT(max, greatCircleDistance(sB.lat(), sB.lon(), sE.lat(), sE.lon()));
  EXPECT_LT(max, greatCircleDistance(sB.lat(), sB.lon(), sF.lat(), sF.lon()));

  EXPECT_LT(max, greatCircleDistance(sC.lat(), sC.lon(), sD.lat(), sD.lon()));
  EXPECT_LT(max, greatCircleDistance(sC.lat(), sC.lon(), sE.lat(), sE.lon()));
  EXPECT_GT(max, greatCircleDistance(sC.lat(), sC.lon(), sF.lat(), sF.lon()));

  EXPECT_LT(max, greatCircleDistance(sD.lat(), sD.lon(), sE.lat(), sE.lon()));
  EXPECT_LT(max, greatCircleDistance(sD.lat(), sD.lon(), sF.lat(), sF.lon()));

  EXPECT_LT(max, greatCircleDistance(sE.lat(), sE.lon(), sF.lat(), sF.lon()));

  // Setup dijkstra:
  Dijkstra dijkstra(network);
  dijkstra.logger(&log);

  QueryResult result;
  vector<int> depNodes;
  depNodes = network.findStartNodeSequence(network.stop(network.stopIndex("A")),
                                           str2time("20120118T000000"));
  dijkstra.startTime(str2time("20120118T000000"));
  dijkstra.findShortestPath(depNodes, network.stopIndex("C"), &result);
  EXPECT_EQ(2, result.destLabels.size());
  vector<QueryResult::Path> pathsDijkstra = result.optimalPaths(network);
  EXPECT_EQ(2, pathsDijkstra.size());

  // Setup patterns:
  TransferPatternRouter router(network);
  router.logger(&log);
  router.prepare(lines);
  TPDB tpdb;
  router.transferPatternsDB(tpdb);
  router.computeAllTransferPatterns(&tpdb);
  vector<QueryResult::Path> pathsPatterns;
  pathsPatterns = router.shortestPath(network.stopIndex("A"),
                                      str2time("20120118T000000"),
                                      network.stopIndex("C"));

  EXPECT_EQ(pathsDijkstra.size(), pathsPatterns.size());
}


/* A transit network with five trains TRIP1, TRIP2, TRIP3, TRIP4 and TRIP5
 * and walkable ways between B-C and C-D
 *
 * A --> B --> C --> D ---> E
 *
 * TRIP1 A-B takes 20 minutes and goes at 0:00:00
 * TRIP2 B-C takes 20 minutes and goes at 0:30:00
 * TRIP3 C-D takes 50 minutes and goes at 1:00:00
 * TRIP4 D-E takes 10 minutes and goes at 1:20:00
 * TRIP5 D-E takes 10 minutes and goes at 2:20:00
 *
 * We have the following problem:
 * It is better to walk the way C-D than the way B-C, but the query search
 * first expands the walkway B-C. That blocks the expansion of the walkway C-D,
 * because label (1684,1) replaces label (3300,1) (or marks it as closed?).
 *
 * If we expand also all closed labels remaining in the priority queue,
 * then label (3300,1,walk=0) is changed to (3300,1,walk=1) which is strange!
 *
 *
 * Expansion of all labels not closed:
 *
 * pop (1500,0,0)@ 1
 * - expand (3300,1,0)@ 2
 * - expand (1684,1,1)@ 2
 * pop (1684,1,1)@ 2
 * - expand (6900,1,0)@ 3
 * pop (6900,1,0)@ 3
 * - expand (9300,2,0)@ 4
 *
 *
 * Expansion of all labels, inclusive closed ones:
 *
 * pop (1500,0,0)@ 1
 *  - expand (3300,1,0)@ 2
 *  - expand (3300,1,1)@ 2
 * pop (1684,1,1)@ 2
 *  - expand (6900,1,0)@ 3
 * pop (3300,1,1)@ 2
 * pop (6900,1,0)@ 3
 *  - expand (9300,2,0)@ 4
 *
 */
// _____________________________________________________________________________
TEST(QuerySearchTest, walkOrder) {
  TransitNetwork network;
  GtfsParser parser;
  vector<Line> lines;
  parser.logger(&LOG);
  LOG.enabled(false);
  network = parser.createTransitNetwork("test/data/walk-test-case5/",
                                        "20120118T000000", "20120118T230000",
                                        &lines);
  network.preprocess();

  Stop& sA = network.stop(network.stopIndex("A"));
  Stop& sB = network.stop(network.stopIndex("B"));
  Stop& sC = network.stop(network.stopIndex("C"));
  Stop& sD = network.stop(network.stopIndex("D"));

  int max = network.MAX_WALKWAY_DIST;
  EXPECT_LT(max, greatCircleDistance(sA.lat(), sA.lon(), sB.lat(), sB.lon()));
  EXPECT_LT(max, greatCircleDistance(sA.lat(), sA.lon(), sC.lat(), sC.lon()));
  EXPECT_LT(max, greatCircleDistance(sA.lat(), sA.lon(), sD.lat(), sD.lon()));
  EXPECT_GT(max, greatCircleDistance(sB.lat(), sB.lon(), sC.lat(), sC.lon()));
  EXPECT_LT(max, greatCircleDistance(sB.lat(), sB.lon(), sD.lat(), sD.lon()));
  EXPECT_GT(max, greatCircleDistance(sC.lat(), sC.lon(), sD.lat(), sD.lon()));

  // Setup dijkstra:
  Dijkstra dijkstra(network);

  QueryResult result;
  vector<int> depNodes;
  depNodes = network.findStartNodeSequence(network.stop(network.stopIndex("A")),
                                           str2time("20120118T000000"));
  dijkstra.startTime(str2time("20120118T000000"));
  dijkstra.findShortestPath(depNodes, network.stopIndex("E"), &result);
  EXPECT_EQ(1, result.destLabels.size());
  vector<QueryResult::Path> pathsDijkstra = result.optimalPaths(network);
  ASSERT_EQ(1, pathsDijkstra.size());

  // Setup patterns:
  TransferPatternRouter router(network);
  router.logger(&LOG);
  router.prepare(lines);
  TPDB tpdb;
  router.transferPatternsDB(tpdb);
  router.computeAllTransferPatterns(&tpdb);

  vector<QueryResult::Path> pathsPatterns;
  pathsPatterns = router.shortestPath(network.stopIndex("A"),
                                      str2time("20120118T000000"),
                                      network.stopIndex("E"));
  ASSERT_EQ(pathsDijkstra.size(), pathsPatterns.size());
  EXPECT_EQ(pathsDijkstra[0].first.cost(), pathsPatterns[0].first.cost());
  EXPECT_EQ(pathsDijkstra[0].first.penalty(), pathsPatterns[0].first.penalty());
}


/* Another problem of our implementation:
 * We do not trace back patterns from departure nodes in the precomputation.
 * This shows up when a path via walking arrives after the last transfer node
 * but still before the last departure node.
 * This problem has rather small impact, since for each stop there is just one
 * such node and it is rather unprobable that a random query arrives at that
 * stop at a bad time.
 * A solution would be to include departure nodes in the pattern backtracking,
 * but this would increase precompuation time and requires special treatment in
 * the arrival loop algorithm.
 */
TEST_F(TransferPatternRouterTest, walkingProblem2) {
  TransitNetwork network;
  GtfsParser parser;
  vector<Line> lines;
  parser.logger(&log);
  network = parser.createTransitNetwork("test/data/walk-test-case6/",
                                        "20120118T000000", "20120118T230000",
                                        &lines);
  network.preprocess();

  int A = network.stopIndex("A");
  int C = network.stopIndex("C");
  int D = network.stopIndex("D");
  int E = network.stopIndex("E");

  ASSERT_TRUE(network.numNodes());
  ASSERT_TRUE(network.numArcs());
  ASSERT_TRUE(network.walkwayList(2).size());

  TransferPatternRouter router(network);
  router.prepare(lines);
  TPDB tpdb;
  router.transferPatternsDB(tpdb);
  router.computeAllTransferPatterns(&tpdb);

  // there is a TP (A C D E) that walks from C to D using the departure node
  QueryGraph qgAE(tpdb.graph(A), E);
  EXPECT_EQ(1, qgAE.generateTransferPatterns().size());

  // But up until D TPs do not find a pattern during precomputation
  QueryGraph qgAD(tpdb.graph(A), D);
  EXPECT_EQ(2, qgAD.generateTransferPatterns().size());
  EXPECT_THAT(qgAD.generateTransferPatterns(), Contains({A, D}));
  EXPECT_THAT(qgAD.generateTransferPatterns(), Contains({A, C, D}));

  // ... and so the search for a connection fails
  vector<QueryResult::Path> pathsTP =
      router.shortestPath(A, str2time("20120118T001000"), D);
  EXPECT_EQ(1, pathsTP.size());

  // ... while Dijkstra finds a path to the departure node
  Dijkstra dijkstra(network);
  QueryResult result;
  vector<int> depNodes =
      network.findStartNodeSequence(network.stop(A),
                                    str2time("20120118T001000"));
  dijkstra.startTime(str2time("20120118T001000"));
  dijkstra.findShortestPath(depNodes, D, &result);
  EXPECT_EQ(1, result.destLabels.size());
  vector<QueryResult::Path> pathsDijkstra = result.optimalPaths(network);
  ASSERT_EQ(1, pathsDijkstra.size());
  ASSERT_EQ(1, pathsTP.size());
  EXPECT_EQ(pathsDijkstra[0].first.cost(), pathsTP[0].first.cost());
}


/* Another systematic problem:
 * If a path without a hub is better than a path with a hub, a suboptimal or no
 * path will be found by Dijkstra:
 * From C, there is a outgoing line that starts at a time such that it can be
 * reached from A-T2-T5-C before the first A-T1-Hub-C. Neither Dijkstra nor TP
 * can find a route A-->B without using the hub. But the Dijkstra will not find
 * the first route via 'Hub' because its label at C is strictly less than the
 * label from the route A-T2-T5-C.
 *
 */
TEST_F(TransferPatternRouterTest, structuralProblem) {
  TransitNetwork network;
  GtfsParser parser;
  vector<Line> lines;
  parser.logger(&log);
  network = parser.createTransitNetwork("test/data/query-test-case2/",
                                        "20120805T000000", "20120805T230000",
                                        &lines);
  network.preprocess();

  const int A = network.stopIndex("A");
  const int B = network.stopIndex("B");
  const int Hub = network.stopIndex("Hub");
  HubSet hubs = {Hub};

  TransferPatternRouter router(network);
  router.hubs(hubs);
  router.prepare(lines);
  TPDB tpdb;
  tpdb.init(network.numStops(), hubs);
  router.transferPatternsDB(tpdb);
  router.computeAllTransferPatterns(&tpdb);

  // make sure 'B' is not in the TPG of 'A' but 'Hub' is and 'B' can be reached
  ASSERT_EQ(TPG::INVALID_NODE, tpdb.graph(A).destNode(B));
  ASSERT_NE(TPG::INVALID_NODE, tpdb.graph(A).destNode(Hub));
  ASSERT_NE(TPG::INVALID_NODE, tpdb.graph(Hub).destNode(B));

  vector<QueryResult::Path> pathsTP =
      router.shortestPath(A, str2time("20120805T110000"), B);
  ASSERT_EQ(1, pathsTP.size());

  QueryResult result;
  Command::dijkstraQuery(network, &hubs, A, str2time("20120805T110000"), B,
                         &result);
  vector<QueryResult::Path> pathsDijkstra = result.optimalPaths(network);

  // We would expect the Dijkstra to find the same path as the TP routing
  ASSERT_EQ(1, pathsDijkstra.size());
  ASSERT_EQ(pathsDijkstra[0].first.cost(), pathsTP[0].first.cost());
}
