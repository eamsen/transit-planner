// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
// #include <kdtree++/kdtree.hpp>
#include <string>
#include <utility>
#include <vector>
#include "../src/TransitNetwork.h"
#include "./GtestUtil.h"
#include "../src/GtfsParser.h"
#include "../src/Utilities.h"

using ::testing::ElementsAre;
using ::testing::Contains;

// _____________________________________________________________________________
TEST(TransitNetworkTest, CopyConstructor) {
  TransitNetwork a;

  TransitNetwork b = a;
}

// _____________________________________________________________________________
TEST(TransitNetworkTest, Equals) {
  Node a1 = Node(0, Node::ARRIVAL, 0);
  Node a2 = Node(0, Node::ARRIVAL, 0);
  EXPECT_TRUE(a1 == a2);
}

// _____________________________________________________________________________
TEST(TransitNetworkTest, findFirstNode) {
  TransitNetwork tn;
  Stop stop("s", 0, 0);
  tn.addStop(stop);
  tn.addTransitNode(0, Node::ARRIVAL, 20);
  tn.addTransitNode(0, Node::ARRIVAL, 30);
  tn.addTransitNode(0, Node::ARRIVAL, 40);

  Stop& s = tn.stop(0);
  int result1 = tn.findFirstNode(s, 30);
  EXPECT_EQ(1, result1);
  int result2 = tn.findFirstNode(s, 0);
  EXPECT_EQ(0, result2);
  int result3 = tn.findFirstNode(s, 39);
  EXPECT_EQ(2, result3);
  int result4 = tn.findFirstNode(s, 99);
  EXPECT_EQ(3, result4);
  int result5 = tn.findFirstNode(s, 25);
  EXPECT_EQ(1, result5);

  // add more values
  tn.addTransitNode(0, Node::ARRIVAL, 50);
  tn.addTransitNode(0, Node::ARRIVAL, 60);
  tn.addTransitNode(0, Node::ARRIVAL, 70);
  tn.addTransitNode(0, Node::ARRIVAL, 80);
  tn.addTransitNode(0, Node::ARRIVAL, 90);

  size_t result6 = tn.findFirstNode(s, 30);
  EXPECT_EQ(1, result6);
  size_t result7 = tn.findFirstNode(s, 0);
  EXPECT_EQ(0, result7);
  size_t result8 = tn.findFirstNode(s, 39);
  EXPECT_EQ(2, result8);
  size_t result9 = tn.findFirstNode(s, 99);
  EXPECT_EQ(8, result9);
  size_t result10 = tn.findFirstNode(s, 65);
  EXPECT_EQ(5, result10);

  // examine some special cases: sequence of equal values
  tn._nodes[4] = tn._nodes[5];
  tn._nodes[6] = tn._nodes[5];

  size_t result11 = tn.findFirstNode(s, 69);
  EXPECT_EQ(4, result11);
  size_t result12 = tn.findFirstNode(s, 70);
  EXPECT_EQ(4, result12);
  size_t result13 = tn.findFirstNode(s, 71);
  EXPECT_EQ(7, result13);
}

// _____________________________________________________________________________
TEST(TransitNetworkTest, findStartNodeSeq) {
  GtfsParser parser;
  Logger test_log;
  test_log.target("log/test.log");
  parser.logger(&test_log);
  TransitNetwork tn;
  tn = parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                   "20111128T000000", "20111128T235959");

  // request for start nodes at node "A"
  // should yield the transfer node from TRIP2
  vector<int> indices = tn.findStartNodeSequence(tn.stop(0),
                                            str2time("20111128T000000"));
  EXPECT_THAT(indices, ElementsAre(2));

  // request for start nodes at node "B" from time 0:00:00
  // should yield the transfer node from TRIP2
  indices = tn.findStartNodeSequence(tn.stop(1), str2time("20111128T000000"));
  EXPECT_THAT(indices, ElementsAre(14));

  // request for start nodes at node "B" from time 0:13:00
  // should yield the transfer node from TRIP1
  indices = tn.findStartNodeSequence(tn.stop(1),
                                str2time("20111128T001300"));
  EXPECT_THAT(indices, ElementsAre(5));
}

// _____________________________________________________________________________
TEST(TransitNetworkTest, KDTreeTest_nearest) {
  // Demonstrates the usage of libkdtree++ nearest neighbor search.
  StopTree tree;
  Stop s0("A", 1, 0);
  Stop s1("B", 1, 1);
  StopTreeNode n0(s0), n1(s1);
  tree.insert(n0);
  tree.insert(n1);
  tree.optimise();

  // Create a reference node.
  StopTreeNode reference;
  reference.pos[0] = 0;
  reference.pos[1] = 0;
  // Search for nodes in the range 1.0 around the reference.
  std::pair<StopTree::const_iterator, float> result =
      tree.find_nearest(reference);
  Stop debug = *(result.first->stopPtr);
  EXPECT_EQ(s0, *result.first->stopPtr);
}

// _____________________________________________________________________________
TEST(TransitNetworkTest, KDTreeTest_nearest_range) {
  // Demonstrates the usage of libkdtree++ range search
  // Test whether the libkdtree++ handles range delimiter as Euclidean distance
  // (unit circle radius) or as indipendent limit in each dimension.
  StopTree tree;
  Stop s0("A", 1, 0);
  Stop s1("B", 1, 1);
  StopTreeNode n0(s0), n1(s1);
  tree.insert(n0);
  tree.insert(n1);
  tree.optimise();

  // Create a reference node.
  StopTreeNode reference;
  reference.pos[0] = 0;
  reference.pos[1] = 0;
  // Search for nodes in the range 1.0 around the reference.
  vector<StopTreeNode> results;
  tree.find_within_range(reference, 1.0f,
                   std::back_insert_iterator<vector<StopTreeNode> >(results));
  EXPECT_EQ(s1, *(results[0].stopPtr));
  EXPECT_EQ(s0, *(results[1].stopPtr));
  // NOTE: results come unsorted and do not fulfill Euclidean distance (see
  // distance metric template argument for the KDTree)
}

// _____________________________________________________________________________
TEST(TransitNetworkTest, createTimeCompressedNetwork) {
  TransitNetwork tn;
  Stop s0("S1", "Stop1", 0, 0), s1("S2", "Stop2", 0, 0);
  tn.addStop(s0);
  tn.addStop(s1);
  tn.addTransitNode(0, Node::DEPARTURE, 20);
  tn.addTransitNode(0, Node::DEPARTURE, 40);
  tn.addTransitNode(1, Node::ARRIVAL, 50);
  tn.addTransitNode(1, Node::ARRIVAL, 100);
  tn.addArc(0, 2, 30);
  tn.addArc(1, 3, 60);
  ASSERT_EQ(2, tn.stop(0).getNodeIndices().size());
  TransitNetwork compressed = tn.createTimeCompressedNetwork();
  EXPECT_EQ(2, compressed.numNodes());
  EXPECT_EQ(1, compressed.numArcs());
  EXPECT_EQ("[2,1,{(1,30)},{}]", compressed.debugString());

  TransitNetwork tn2;
  GtfsParser parser;
  tn2 = parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                    "20120512T000000", "20120520T000000");
  TransitNetwork compressed2 = tn2.createTimeCompressedNetwork();
  EXPECT_EQ(tn2.numStops(), compressed2.numNodes());
  EXPECT_EQ(compressed2.numNodes(), compressed2.numStops());
  EXPECT_EQ(4, compressed2.numArcs());
  vector<int> stopsUncompressed;
  for (size_t i = 0; i < tn2.numStops(); i++)
    stopsUncompressed.push_back(tn2.stop(i).index());
  vector<int> nodesCompressed;
  for (size_t i = 0; i < compressed2.numNodes(); i++)
    nodesCompressed.push_back(compressed2.node(i).stop());
  EXPECT_EQ(stopsUncompressed, nodesCompressed);

  nodesCompressed.clear();
  for (size_t i = 0; i < compressed2.numNodes(); i++)
    nodesCompressed.push_back(i);
  EXPECT_EQ(stopsUncompressed, nodesCompressed);
}

// _____________________________________________________________________________
TEST(TransitNetworkTest, computeGeoInfo) {
  GtfsParser parser;
  TransitNetwork network =
     parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                 "20120601T000000", "20120601T000001");
  network.computeGeoInfo();
  ASSERT_TRUE(true);
  EXPECT_FLOAT_EQ(-10, network.geoInfo().latMin);
  EXPECT_FLOAT_EQ(-10, network.geoInfo().lonMin);
  EXPECT_FLOAT_EQ(10, network.geoInfo().latMax);
  EXPECT_FLOAT_EQ(10, network.geoInfo().lonMax);
}

// _____________________________________________________________________________
TEST(TransitNetworkTest, largestConnectedComponent) {
  GtfsParser parser;
  const vector<string> gtfs = {"test/data/simple-parser-test-case/",
                               "test/data/time-test-case1/"};
  TransitNetwork compare = parser.createTransitNetwork(gtfs[0],
                                          "20120618T000000", "20120619T000001");
  compare.preprocess();
  // At first test the connected component search.
  EXPECT_EQ(4,
      compare.createTimeCompressedNetwork().connectedComponentNodes(0).size());
  EXPECT_EQ(3,
      compare.createTimeCompressedNetwork().connectedComponentNodes(1).size());
  EXPECT_EQ(1,
      compare.createTimeCompressedNetwork().connectedComponentNodes(2).size());

  // Now test the behaviour of mirrored().
  TransitNetwork mirrored = compare.mirrored();
  EXPECT_EQ(compare.numStops(), mirrored.numStops());
  EXPECT_EQ(compare.numNodes(), mirrored.numNodes());
  EXPECT_EQ(2*compare.numArcs(), mirrored.numArcs());
  TransitNetwork mirroredTimeIndependent =
      compare.createTimeCompressedNetwork().mirrored();
  EXPECT_EQ(5, mirroredTimeIndependent.connectedComponentNodes(0).size());
  EXPECT_EQ(5, mirroredTimeIndependent.connectedComponentNodes(1).size());
  EXPECT_EQ(5, mirroredTimeIndependent.connectedComponentNodes(2).size());

  // Finally test largetConnectedComponent().
  TransitNetwork network = parser.createTransitNetwork(gtfs,
                                          "20120618T000000", "20120619T000001");
  network.preprocess();
  EXPECT_EQ(9, network.numStops());
  EXPECT_LT(compare.numNodes(), network.numNodes());
  EXPECT_LT(compare.numArcs(), network.numArcs());
  TransitNetwork compressed = network.createTimeCompressedNetwork().mirrored();
  EXPECT_EQ(2*3+2*4, compressed.numArcs());
  EXPECT_EQ(4 + 5, compressed.numStops());
  EXPECT_EQ(4 + 5, compressed.numNodes());

  TransitNetwork largestComponent = network.largestConnectedComponent();
  // NOTE(jonas): see note in header file
//   EXPECT_EQ(compare.numNodes(), largestComponent.numNodes());
//   EXPECT_EQ(compare.numArcs(), largestComponent.numArcs());
  EXPECT_EQ(5, largestComponent.numStops());
  for (const Stop& stop: compare._stops)
    EXPECT_THAT(largestComponent._stops, Contains(stop));
  EXPECT_EQ(compare.walkwayList(1), largestComponent.walkwayList(1));
  EXPECT_EQ(compare._walkwayLists, largestComponent._walkwayLists);
}
