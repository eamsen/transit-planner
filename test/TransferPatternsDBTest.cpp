// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include <boost/foreach.hpp>
#include <gmock/gmock.h>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include "./GtestUtil.h"
#include "../src/TransferPatternsDB.h"
#include "../src/Utilities.h"
#include "../src/GtfsParser.h"
#include "../src/TransferPatternRouter.h"

using std::cout;
using std::endl;
using ::testing::ElementsAre;
using ::testing::Contains;

class TransferPatternsDBTest : public ::testing::Test {
 public:
  void SetUp() {
    log.enabled(false);
    A = 0;
    B = 1;
    C = 2;
    D = 3;
    E = 4;
    int a1[4] = {A, B, D, E};
    int a2[2] = {A, E};
    int a3[5] = {A, B, C, D, E};
    int a4[3] = {A, B, E};
    int a5[3] = {A, B, C};
    p1 = vector<int>(a1, a1 + 4);
    p2 = vector<int>(a2, a2 + 2);
    p3 = vector<int>(a3, a3 + 5);
    p4 = vector<int>(a4, a4 + 3);
    p5 = vector<int>(a5, a5 + 3);
  }
  void TearDown() {}
  Logger log;
  vector<int> p1, p2, p3, p4, p5;
  int A, B, C, D, E;
};

// Based on the example shown on Fig. 1 in Bast's paper
TEST_F(TransferPatternsDBTest, basic) {
  HubSet hubs;
  int numStops = 5;
  TPDB db(numStops, hubs);
  const TPG& aGraph = db.graph(A);
  EXPECT_EQ(A, aGraph.depStop());
  db.addPattern(p1);
  log.debug("added p1");
  db.addPattern(p2);
  log.debug("added p2");
  db.addPattern(p3);
  log.debug("added p3");
  db.addPattern(p4);
  log.debug("added p4");
  db.addPattern(p5);
  log.debug("added p5");

  EXPECT_EQ(TPG::INVALID_NODE, aGraph.destNode(A));
  EXPECT_EQ(TPG::INVALID_NODE, aGraph.destNode(B));
  EXPECT_EQ(TPG::INVALID_NODE, aGraph.destNode(D));
  EXPECT_EQ(0, aGraph.destHubs().size());
  EXPECT_EQ(7, aGraph.numNodes());

  // A->...->E
  const int eNode = aGraph.destNode(E);
  EXPECT_NE(-1, eNode);
  const vector<int>& eSuccs = aGraph.successors(eNode);
  set<int> eSuccStops;
  BOOST_FOREACH(int node, eSuccs) {
    eSuccStops.insert(aGraph.stop(node));
  }
  EXPECT_EQ(3, eSuccStops.size());
  // ...->D->E
  EXPECT_THAT(eSuccStops, Contains(D));
  // A->E
  EXPECT_THAT(eSuccStops, Contains(A));
  // A->B->E
  EXPECT_THAT(eSuccStops, Contains(B));

  BOOST_FOREACH(int node, eSuccs) {
    // A->B->E
    if (aGraph.stop(node) == B) {
      EXPECT_EQ(0, aGraph.successors(node).back());
    } else if (aGraph.stop(node) == D) {  // ...->D->E
      EXPECT_TRUE(aGraph.stop(aGraph.successors(node).back()) == C
                  || aGraph.stop(aGraph.successors(node).back()) == B);
    } else {  // A->E
      EXPECT_EQ(A, aGraph.stop(node));
    }
  }
  // A->B->C
  const int cNode = aGraph.destNode(C);
  EXPECT_NE(-1, cNode);
  const vector<int>& cSuccs = aGraph.successors(cNode);
  set<int> cSuccStops;
  BOOST_FOREACH(int s, cSuccs) {
    cSuccStops.insert(aGraph.stop(s));
  }
  EXPECT_EQ(1, cSuccStops.size());
  EXPECT_THAT(cSuccStops, Contains(B));
}

// Some tests taken from former TransferPatternRouterTest

// _____________________________________________________________________________
// transfer patterns:
// [A, B, D, Z]
// [A, C, Z]
// [A, B, E, Z]
//
TEST_F(TransferPatternsDBTest, TransferPatternGraph) {
  vector<int> transferPattern1 = {0, 1, 3, 25};
  vector<int> transferPattern2 = {0, 2, 25};
  vector<int> transferPattern3 = {0, 1, 4, 25};

  TransferPatternsDB db;
  HubSet hubs = {};
  db.init(6, hubs);
  db.addPattern(transferPattern1);
  db.addPattern(transferPattern2);
  db.addPattern(transferPattern3);

  vector<int> successorsOf0 = {};
  vector<int> successorsOf1 = {0};
  vector<int> successorsOf2 = {0};
  vector<int> successorsOf3 = {1};
  vector<int> successorsOf4 = {1};
  vector<int> successorsOf25 = {2, 3, 4};

  const TPG& graph = db.graph(0);
  EXPECT_EQ(0, graph.successors(0).size());
  EXPECT_EQ(1, graph.successors(1).size());
  EXPECT_EQ(1, graph.successors(2).size());
  EXPECT_EQ(3, graph.successors(3).size());  // node 3 is stop 25, notice
  EXPECT_EQ(1, graph.successors(4).size());  // order of appearance
  EXPECT_EQ(1, graph.successors(5).size());

  EXPECT_EQ(graph.successors(2), graph.successors(5));
  EXPECT_EQ(graph.successors(1), graph.successors(4));
}

// _____________________________________________________________________________
TEST_F(TransferPatternsDBTest, constructParsedQueryGraphTest) {
  TransitNetwork network;
  GtfsParser parser;
  parser.logger(&log);
  network = parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                        "20120118T000000", "20120120T000000");
  network.preprocess();

  TransferPatternRouter router(network);
  router.logger(&log);
  TPDB tpdb;
  router.transferPatternsDB(tpdb);
  router.computeAllTransferPatterns(&tpdb);

  int indexA = network.stopIndex("A");
  int indexB = network.stopIndex("BC");
  int indexC = network.stopIndex("C");
  int indexD = network.stopIndex("D");
  int indexE = network.stopIndex("E");

  vector<int> patternAB = {indexA, indexB};
  vector<int> patternAC = {indexA, indexC};
  vector<int> patternAE = {indexA, indexB, indexE};

  vector<int> patternBC = {indexB, indexC};
  vector<int> patternBE = {indexB, indexE};

  vector<int> patternDB = {indexD, indexB};
  vector<int> patternDC = {indexD, indexB, indexC};
  vector<int> patternDE = {indexD, indexE};

  const TransferPatternsDB& tpdb2 = *router.transferPatternsDB();

  EXPECT_NE(TPG::INVALID_NODE, tpdb2.graph(indexA).destNode(indexB));
  EXPECT_NE(TPG::INVALID_NODE, tpdb2.graph(indexA).destNode(indexC));
  EXPECT_EQ(TPG::INVALID_NODE, tpdb2.graph(indexA).destNode(indexD));
  EXPECT_NE(TPG::INVALID_NODE, tpdb2.graph(indexA).destNode(indexE));

//   EXPECT_THAT(router.transferPatterns(indexA, indexB), Contains(patternAB));
//   EXPECT_THAT(router.transferPatterns(indexA, indexC), Contains(patternAC));
//   EXPECT_THAT(router.transferPatterns(indexA, indexE), Contains(patternAE));

  EXPECT_EQ(TPG::INVALID_NODE, tpdb2.graph(indexB).destNode(indexA));
  EXPECT_NE(TPG::INVALID_NODE, tpdb2.graph(indexB).destNode(indexC));
  EXPECT_EQ(TPG::INVALID_NODE, tpdb2.graph(indexB).destNode(indexD));
  EXPECT_NE(TPG::INVALID_NODE, tpdb2.graph(indexB).destNode(indexE));

//   EXPECT_THAT(router.transferPatterns(indexB, indexC), Contains(patternBC));
//   EXPECT_THAT(router.transferPatterns(indexB, indexE), Contains(patternBE));

//   EXPECT_EQ(0, router.transferPatterns(indexC, indexA));
//   EXPECT_EQ(0, router.transferPatterns(indexC, indexB));
//   EXPECT_EQ(0, router.transferPatterns(indexC, indexD));
//   EXPECT_EQ(0, router.transferPatterns(indexC, indexE));

  EXPECT_EQ(TPG::INVALID_NODE, tpdb2.graph(indexD).destNode(indexA));
  EXPECT_NE(TPG::INVALID_NODE, tpdb2.graph(indexD).destNode(indexB));
  EXPECT_NE(TPG::INVALID_NODE, tpdb2.graph(indexD).destNode(indexC));
  EXPECT_NE(TPG::INVALID_NODE, tpdb2.graph(indexD).destNode(indexE));

//   EXPECT_THAT(router.transferPatterns(indexD, indexB), Contains(patternDB));
//   EXPECT_THAT(router.transferPatterns(indexD, indexC), Contains(patternDC));
//   EXPECT_THAT(router.transferPatterns(indexD, indexE), Contains(patternDE));

  EXPECT_EQ(TPG::INVALID_NODE, tpdb2.graph(indexE).destNode(indexA));
  EXPECT_EQ(TPG::INVALID_NODE, tpdb2.graph(indexE).destNode(indexB));
  EXPECT_EQ(TPG::INVALID_NODE, tpdb2.graph(indexE).destNode(indexC));
  EXPECT_EQ(TPG::INVALID_NODE, tpdb2.graph(indexE).destNode(indexD));
}

// _____________________________________________________________________________
TEST_F(TransferPatternsDBTest, addTwoPatterns) {
  int A = 0;
  int B = 1;
  int C = 2;
  vector<int> p0;
  p0.push_back(A);
  p0.push_back(B);
  p0.push_back(C);
  vector<int> p1;
  p1.push_back(A);
  p1.push_back(C);

  TPG tpgA(A);
  tpgA.addPattern(p0);
  tpgA.addPattern(p1);

  const int destNodeC = tpgA.destNode(C);
  EXPECT_EQ(2, tpgA.successors(destNodeC).size());
}

// _____________________________________________________________________________
TEST_F(TransferPatternsDBTest, addPatternCorrectness) {
  int A(0), B(1), C(2);
  TPG tpgStopA(A);
  tpgStopA.addPattern({A, B, C});
  EXPECT_EQ(TPG::INVALID_NODE, tpgStopA.destNode(A));
  EXPECT_EQ(TPG::INVALID_NODE, tpgStopA.destNode(B));
  EXPECT_NE(TPG::INVALID_NODE, tpgStopA.destNode(C));

  set<int> hubs;
  TPDB db(3, hubs);
  db.addPattern({A, B, C});
  TPG dbGraphA = db.graph(A);
  EXPECT_EQ(TPG::INVALID_NODE, dbGraphA.destNode(A));
  EXPECT_EQ(TPG::INVALID_NODE, dbGraphA.destNode(B));
  EXPECT_NE(TPG::INVALID_NODE, dbGraphA.destNode(C));
}
