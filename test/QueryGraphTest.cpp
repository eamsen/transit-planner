// Copyright 2012
#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include "../src/QueryGraph.h"
#include "./GtestUtil.h"
#include "../src/TransferPatternRouter.h"
#include "../src/Utilities.h"

using std::cout;
using std::endl;
// using ::testing::WhenSorted;
using ::testing::ElementsAre;
using ::testing::Contains;
using ::testing::AnyOf;
using ::testing::Eq;

// _____________________________________________________________________________
// Construct the QueryGraph from the DAG presented in the lecture.
TEST(QueryGraphTest, Constructor) {
  int A = 0;
  int B = 1;
  int C = 2;
  int D = 3;
  int E = 4;
  vector<int> p0 = {A, B, D, E};
  vector<int> p1 = {A, B, C, D, E};
  vector<int> p2 = {A, B, E};
  vector<int> p3 = {A, E};
  TPG tpgA(A);
  tpgA.addPattern(p0);
  tpgA.addPattern(p1);
  tpgA.addPattern(p2);
  tpgA.addPattern(p3);

  QueryGraph qgraph(tpgA, E);

  std::map<int, std::set<int> > collect;
  for (uint i = 0; i < qgraph._stops.size(); i++) {
    int stop = qgraph.stopIndex(i);
    collect[stop] = set<int>();
    for (int successorNode: qgraph.successors(i)) {
      collect[stop].insert(qgraph.stopIndex(successorNode));
    }
  }
//   std::cout << qgraph.toString() << std::endl;
  ASSERT_EQ(5, qgraph._stops.size());
  EXPECT_EQ(0, qgraph.successors(qgraph.targetNode()).size());  // E
  EXPECT_THAT(qgraph.successors(qgraph.sourceNode()),
              Contains(qgraph.targetNode()));  // A --> E

  EXPECT_THAT(collect[A], ElementsAre(B, E));
  EXPECT_THAT(collect[B], ElementsAre(C, D, E));
  EXPECT_THAT(collect[C], ElementsAre(D));
  EXPECT_THAT(collect[D], ElementsAre(E));
  EXPECT_THAT(collect[E], ElementsAre());
}

// Some tests taken from former TransferPatternRouterTest

// _____________________________________________________________________________
// transfer patterns:
// [A, B, D, Z]
// [A, C, Z]
// [A, B, E, Z]
//
// MATCHER_P(HasSize, size, "") {
//  return arg.size() == static_cast<size_t>(size); }
TEST(QueryGraphTest, QueryGraph) {
  vector<int> transferPattern1 = {0, 1, 3, 25};
  vector<int> transferPattern2 = {0, 2, 25};
  vector<int> transferPattern3 = {0, 1, 4, 25};

  TransferPatternsDB db;
  set<int> hubs;
  db.init(6, hubs);
  db.addPattern(transferPattern1);
  db.addPattern(transferPattern2);
  db.addPattern(transferPattern3);

//   set<int> successorsOf0 = {1, 2};
//   set<int> successorsOf1 = {3, 4};
//   set<int> successorsOf2 = {25};
//   set<int> successorsOf3 = {25};
//   set<int> successorsOf4 = {25};
  set<int> successorsOf25;

  QueryGraph qg(db.graph(0), 25);
  EXPECT_EQ(2, qg.successors(qg.sourceNode()).size());
  EXPECT_EQ(successorsOf25, qg.successors(qg.targetNode()));
  EXPECT_THAT(qg.successors(2), Eq(qg.successors(3)));
  EXPECT_THAT(qg.successors(3), Eq(qg.successors(4)));
  EXPECT_EQ(2, qg.successors(5).size());
}

// _____________________________________________________________________________
TEST(QueryGraphTest, generatePatterns) {
  int A = 0;
  int B = 1;
  int C = 2;
  int D = 3;
  int E = 4;
  vector<int> p0 = {A, B, D, E};
  vector<int> p1 = {A, B, C, D, E};
  vector<int> p2 = {A, B, E};
  TransferPatternsDB db;
  set<int> hubs;
  db.init(5, hubs);
  db.addPattern(p0);
  db.addPattern(p1);
  db.addPattern(p2);

  QueryGraph qg(db.graph(A), E);

  set<vector<int> > control;
  control.insert(p0);
  control.insert(p1);
  control.insert(p2);

  vector<vector<int> > patterns = qg.generateTransferPatternsRecursive();
  ASSERT_EQ(control, std::set<vector<int> >(patterns.begin(), patterns.end()));

  patterns = qg.generateTransferPatterns();
  ASSERT_EQ(control, std::set<vector<int> >(patterns.begin(), patterns.end()));
}


// _____________________________________________________________________________
// Construct the QG from the DAG presented in the lecture via adding stops.
TEST(QueryGraphTest, addStop) {
  int A = 0;
  int B = 1;
  int C = 2;
  int D = 3;
  int E = 4;
  vector<int> p0 = {A, B, D, E};
  vector<int> p1 = {A, B, C, D, E};
  vector<int> p2 = {A, B, E};
  vector<int> p3 = {A, E};
  TPG tpgA(A);
  tpgA.addPattern(p2);
  tpgA.addPattern(p3);

  QueryGraph qgraph(tpgA, E);

  // add stop D and C to the network
  qgraph.addStop(D, {qgraph.nodeIndex(E)});
  qgraph.addStop(C, {qgraph.nodeIndex(D)});

  // set ars from predecessors of D and C
  qgraph.addStop(B, {qgraph.nodeIndex(D), qgraph.nodeIndex(C)});

  std::map<int, std::set<int> > collect;
  for (uint i = 0; i < qgraph._stops.size(); i++) {
    int stop = qgraph.stopIndex(i);
    collect[stop] = set<int>();
    for (int successorNode: qgraph.successors(i)) {
      collect[stop].insert(qgraph.stopIndex(successorNode));
    }
  }

  ASSERT_EQ(5, qgraph._stops.size());
  EXPECT_EQ(0, qgraph.successors(qgraph.targetNode()).size());  // E
  EXPECT_THAT(qgraph.successors(qgraph.sourceNode()),
              Contains(qgraph.targetNode()));  // A --> E

  EXPECT_THAT(collect[A], ElementsAre(B, E));
  EXPECT_THAT(collect[B], ElementsAre(C, D, E));
  EXPECT_THAT(collect[C], ElementsAre(D));
  EXPECT_THAT(collect[D], ElementsAre(E));
  EXPECT_THAT(collect[E], ElementsAre());
}

// _____________________________________________________________________________
// Construct the QG from the DAG presented in the lecture via merging.
TEST(QueryGraphTest, merge) {
  int A = 0;
  int B = 1;
  int C = 2;
  int D = 3;
  int E = 4;
  vector<int> p0 = {B, D, E};
  vector<int> p1 = {B, C, D, E};
  vector<int> p2 = {A, B, E};
  vector<int> p3 = {A, E};

  TPG tpgA(A);
  tpgA.addPattern(p2);
  tpgA.addPattern(p3);

  QueryGraph qgraph(tpgA, E);

  TPG tpgB(B);
  tpgB.addPattern(p0);
  tpgB.addPattern(p1);

  qgraph.merge(tpgB, E);

  std::map<int, std::set<int> > collect;
  for (uint i = 0; i < qgraph._stops.size(); i++) {
    int stop = qgraph.stopIndex(i);
    collect[stop] = set<int>();
    for (int successorNode: qgraph.successors(i)) {
      collect[stop].insert(qgraph.stopIndex(successorNode));
    }
  }

  ASSERT_EQ(5, qgraph._stops.size());
  EXPECT_EQ(0, qgraph.successors(qgraph.targetNode()).size());  // E
  EXPECT_THAT(qgraph.successors(qgraph.sourceNode()),
              Contains(qgraph.targetNode()));  // A --> E

  EXPECT_THAT(collect[A], ElementsAre(B, E));
  EXPECT_THAT(collect[B], ElementsAre(C, D, E));
  EXPECT_THAT(collect[C], ElementsAre(D));
  EXPECT_THAT(collect[D], ElementsAre(E));
  EXPECT_THAT(collect[E], ElementsAre());
}

// _____________________________________________________________________________
/* Consider three TPGraphs:
 * A -->  B --> C     C --> E --> F   D --> B --> F
 *  \    /     /                       \--> E -->/
 *   ->D/ ----/
 *
 * Take D and C as hubs. Construct the final QueryGraph from (A -> C), (A -> D),
 * (C -> F), (D -> F)
 */
TEST(QueryGraphTest, mergeGraphs) {
  int A(0), B(1), C(2), D(3), E(4), F(5);

  TPDB db;
  set<int> hubs;
  db.init(6, hubs);
  // tpgs for first graph
  db.addPattern({A, B, C});
  db.addPattern({A, B, D, C});
  db.addPattern({A, D, C});
  db.addPattern({A, B, D});
  db.addPattern({A, D});

  // tpgs for second graph
  db.addPattern({C, E, F});

  // tpgs for third graph
  db.addPattern({D, B, F});
  db.addPattern({D, E, F});

  QueryGraph qg(db.graph(A), C);
  qg.merge(db.graph(A), D);
  // test sth.
  qg.merge(db.graph(C), F);
  // test sth.
  qg.merge(db.graph(D), F);
  EXPECT_EQ(6, qg.size());
//   EXPECT_THAT(qg._stops, WhenSorted(ElementsAre(A, B, C, D, E, F)));
  std::sort(qg._stops.begin(), qg._stops.end());
  EXPECT_THAT(qg._stops, (ElementsAre(A, B, C, D, E, F)));
}

// _____________________________________________________________________________
TEST(QueryGraphTest, findPattern_and_containsPattern) {
  int A(5), B(4), C(3), D(2), E(1)/*, F(0)*/;

  TPDB db;
  set<int> hubs;
  db.init(6, hubs);
  db.addPattern({A, B, C});
  db.addPattern({A, B, D, C});

  // For the beginning, test some invalid constructions:
  QueryGraph graphInvalid0(db.graph(B), E);
  QueryGraph graph0 = graphInvalid0.findPattern({A, B, C});
  EXPECT_TRUE(graph0.empty());
  EXPECT_FALSE(graphInvalid0.containsPattern({A, B, C}));

  QueryGraph graphInvalid1(db.graph(A), B);
  QueryGraph graph1 = graphInvalid1.findPattern({A, B});
  EXPECT_TRUE(graph1.empty());
  EXPECT_FALSE(graphInvalid1.containsPattern({A, B}));


  // Now take a good QueryGraph as base:
  QueryGraph graphAC(db.graph(A), C);
  ASSERT_EQ(4, graphAC.size());

  // Try a pattern not contained in the graph.
  const vector<int> pattern0 = {111, 222};
  QueryGraph graphPattern0 = graphAC.findPattern(pattern0);
  EXPECT_TRUE(graphPattern0.empty());
  EXPECT_FALSE(graphAC.containsPattern(pattern0));

  // Try a pattern partially contained in the graph.
  const vector<int> pattern1 = {A, B, 999};
  QueryGraph graphPattern1 = graphAC.findPattern(pattern1);
  EXPECT_TRUE(graphPattern1.empty());
  EXPECT_FALSE(graphAC.containsPattern(pattern1));

  // Try another pattern
  const vector<int> pattern2 = {A};
  QueryGraph graphPattern2 = graphAC.findPattern(pattern2);
  EXPECT_TRUE(graphPattern2.empty());
  EXPECT_FALSE(graphAC.containsPattern(pattern2));

  // Try one more pattern (empty pattern)
  const vector<int> pattern3 = {};
  QueryGraph graphPattern3 = graphAC.findPattern(pattern3);
  EXPECT_TRUE(graphPattern3.empty());
  EXPECT_FALSE(graphAC.containsPattern(pattern3));

  // Lets try some contained patterns:
  const vector<int> pattern4 = {A, B, C};
  QueryGraph graphPattern4 = graphAC.findPattern(pattern4);
  EXPECT_EQ(3, graphPattern4.size());
  stringstream ss;
  ss << A << ":{" << B << ",}" << endl
     << C << ":{}" << endl
     << B << ":{" << C << ",}" << endl;
  EXPECT_EQ(ss.str(), graphPattern4.debugString());
  EXPECT_TRUE(graphAC.containsPattern(pattern4));

  const vector<int> pattern5 = {A, B, D, C};
  QueryGraph graphPattern5 = graphAC.findPattern(pattern5);
  EXPECT_EQ(4, graphPattern5.size());
  stringstream ss2;
  ss2 << A << ":{" << B << ",}" << endl
      << C << ":{}" << endl
      << B << ":{" << D << ",}" << endl
      << D << ":{" << C << ",}" << endl;
     EXPECT_EQ(ss2.str(), graphPattern5.debugString());
  EXPECT_TRUE(graphAC.containsPattern(pattern5));

  // Finally, try a too long pattern.
  const vector<int> pattern6 = {A, B, D, C, C};
  QueryGraph graphPattern6 = graphAC.findPattern(pattern6);
  EXPECT_TRUE(graphPattern6.empty());
  EXPECT_FALSE(graphAC.containsPattern(pattern6));
}
