// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include <gmock/gmock.h>
#include <iostream>
#include <set>
#include <vector>
#include "./GtestUtil.h"
#include "../src/Dijkstra.h"
#include "../src/Utilities.h"
#include "../src/GtfsParser.h"
#include "../src/TransferPatternRouter.h"

using std::cout;
using std::endl;
using ::testing::ElementsAre;
using ::testing::Contains;

class DijkstraTest : public ::testing::Test {
 public:
  void SetUp() {
    log.enabled(false);
  }
  void TearDown() {}
  Logger log;
};

TEST_F(DijkstraTest, easyTest) {
  TransitNetwork network;
  Stop stop("start", "stopName", 100, 100);
  Stop stop2("target", "stopName2", 10, 10);
  network.addStop(stop);
  network.addStop(stop2);
  const int s = network.addTransitNode(network.stopIndex("start"),
                                       Node::DEPARTURE, 0);
  const int t = network.addTransitNode(network.stopIndex("target"),
                                       Node::ARRIVAL, 100*60);
  stop.addNodeIndex(s);
  stop2.addNodeIndex(t);
  network.addArc(s, t, 100*60);

  network.preprocess();

  Dijkstra dijkstra(network);
  dijkstra.logger(&log);
  vector<int> depNodes;
  depNodes.push_back(s);
  QueryResult result;
  dijkstra.findShortestPath(depNodes, network.stopIndex("target"), &result);
  EXPECT_EQ(100*60, result.optimalCosts());
  EXPECT_EQ(0, result.optimalPenalty());
  EXPECT_EQ(2, result.numSettledLabels);

  LabelVec::const_iterator it = result.destLabels.begin();
  LabelVec::Hnd hnd = *it;
  EXPECT_EQ(100*60, hnd.cost());
  EXPECT_EQ(0, hnd.penalty());
  EXPECT_EQ(network.stopIndex("target"), network.node(hnd.at()).stop());

  hnd = result.matrix.parent(hnd);
  EXPECT_EQ(0, hnd.cost());
  EXPECT_EQ(0, hnd.penalty());
  EXPECT_EQ(network.stopIndex("start"), network.node(hnd.at()).stop());
}

TEST_F(DijkstraTest, directConnection) {
  TransitNetwork network;
  Stop stopStart("start", "stopName", 100, 100);
  Stop stopInter("inter", "stopName2", 200, 200);
  Stop stopTarget("target", "stopName3", 300, 300);
  network.addStop(stopStart);
  network.addStop(stopInter);
  network.addStop(stopTarget);

  int s = network.addTransitNode(network.stopIndex("start"),
                                 Node::DEPARTURE, 0);
  int n1 = network.addTransitNode(network.stopIndex("inter"),
                                  Node::ARRIVAL, 100*60);
  int n2 = network.addTransitNode(network.stopIndex("inter"),
                                  Node::DEPARTURE, 100*60);
  int t = network.addTransitNode(network.stopIndex("target"),
                                 Node::ARRIVAL, 200*60);

  stopTarget.addNodeIndex(t);
  network.addArc(s, n1, 100*60);
  network.addArc(n1, n2, 0);
  network.addArc(n2, t, 100*60);

  network.preprocess();

  Dijkstra dijkstra(network);
  dijkstra.logger(&log);
  vector<int> depNodes;
  depNodes.push_back(s);
  QueryResult result;
  dijkstra.findShortestPath(depNodes, network.stopIndex("target"), &result);

  EXPECT_EQ(200*60, result.optimalCosts());
  EXPECT_EQ(0, result.optimalPenalty());
  EXPECT_EQ(4, result.numSettledLabels);
  EXPECT_EQ(result.destLabels.size(), 1);

  LabelVec::Hnd label = *(result.destLabels.begin());

  EXPECT_EQ(200*60, label.cost());
  EXPECT_EQ(0, label.penalty());
  EXPECT_EQ(Node::ARRIVAL, network.node(label.at()).type());
  EXPECT_EQ(network.stopIndex("target"), network.node(label.at()).stop());

  label = result.matrix.parent(label);
  EXPECT_EQ(100*60, label.cost());
  EXPECT_EQ(0, label.penalty());
  EXPECT_EQ(Node::DEPARTURE, network.node(label.at()).type());
  EXPECT_EQ(network.stopIndex("inter"), network.node(label.at()).stop());

  label = result.matrix.parent(label);
  EXPECT_EQ(100*60, label.cost());
  EXPECT_EQ(0, label.penalty());
  EXPECT_EQ(Node::ARRIVAL, network.node(label.at()).type());
  EXPECT_EQ(network.stopIndex("inter"), network.node(label.at()).stop());

  label = result.matrix.parent(label);
  EXPECT_EQ(0, label.cost());
  EXPECT_EQ(0, label.penalty());
  EXPECT_EQ(Node::DEPARTURE, network.node(label.at()).type());
  EXPECT_EQ(network.stopIndex("start"), network.node(label.at()).stop());
}


/*
                s
                |
                |
                n1
                | \
                n2 n3
                |   \
  n4 -------n5-------n6---------t
                |
                |
                n7

 n1, n2, n3, n5, n6 all belong to one stop
*/
TEST_F(DijkstraTest, transferConnection) {
  TransitNetwork network;
  Stop stopStart("start", "stopName", 100, 100);
  Stop stopTarget("target", "stopName2", 200, 100);
  Stop stopInter("inter", "stopName3", 300, 100);
  Stop stopOther1("other1", "stopName4", 400, 100);
  Stop stopOther2("other2", "stopName5", 500, 100);
  network.addStop(stopStart);
  network.addStop(stopTarget);
  network.addStop(stopInter);
  network.addStop(stopOther1);
  network.addStop(stopOther2);

  int s = network.addTransitNode(network.stopIndex("start"),
                                 Node::DEPARTURE, 0);
  int n4 = network.addTransitNode(network.stopIndex("other1"),
                                  Node::DEPARTURE, 0);
  int n1 = network.addTransitNode(network.stopIndex("inter"),
                                  Node::ARRIVAL, 50*60);
  int n2 = network.addTransitNode(network.stopIndex("inter"),
                                  Node::DEPARTURE, 50*60);
  int n3 = network.addTransitNode(network.stopIndex("inter"),
                                  Node::TRANSFER, 60*60);
  int n5 = network.addTransitNode(network.stopIndex("inter"),
                                  Node::ARRIVAL, 80*60);
  int n6 = network.addTransitNode(network.stopIndex("inter"),
                                  Node::DEPARTURE, 80*60);
  int t = network.addTransitNode(network.stopIndex("target"),
                                 Node::ARRIVAL, 200*60);
  int n7 = network.addTransitNode(network.stopIndex("other2"),
                                  Node::ARRIVAL, 400*60);

  stopTarget.addNodeIndex(t);

  network.addArc(s, n1, 50*60);
  network.addArc(n1, n2, 0);
  network.addArc(n1, n3, 30*60, 1);
  network.addArc(n2, n7, 350*60);
  network.addArc(n3, n6, 0);
  network.addArc(n4, n5, 80*60);
  network.addArc(n5, n6, 0);
  network.addArc(n6, t, 120*60);

  network.preprocess();

  Dijkstra dijkstra(network);
  dijkstra.logger(&log);
  vector<int> depNodes;
  depNodes.push_back(s);
  QueryResult result;
  dijkstra.findShortestPath(depNodes, network.stopIndex("target"), &result);

  EXPECT_EQ(200*60, result.optimalCosts());
  EXPECT_EQ(1, result.optimalPenalty());
  EXPECT_EQ(7, result.numSettledLabels);
  EXPECT_EQ(1, result.destLabels.size());

  LabelVec::Hnd label = *(result.destLabels.begin());
  EXPECT_EQ(200*60, label.cost());
  EXPECT_EQ(1, label.penalty());
  EXPECT_EQ(Node::ARRIVAL, network.node(label.at()).type());
  EXPECT_EQ(network.stopIndex("target"), network.node(label.at()).stop());

  label = result.matrix.parent(label);
  EXPECT_EQ(80*60, label.cost());
  EXPECT_EQ(1, label.penalty());
  EXPECT_EQ(Node::DEPARTURE, network.node(label.at()).type());
  EXPECT_EQ(network.stopIndex("inter"), network.node(label.at()).stop());

  label = result.matrix.parent(label);
  EXPECT_EQ(80*60, label.cost());
  EXPECT_EQ(1, label.penalty());
  EXPECT_EQ(Node::TRANSFER, network.node(label.at()).type());
  EXPECT_EQ(network.stopIndex("inter"), network.node(label.at()).stop());

  label = result.matrix.parent(label);
  EXPECT_EQ(50*60, label.cost());
  EXPECT_EQ(0, label.penalty());
  EXPECT_EQ(Node::ARRIVAL, network.node(label.at()).type());
  EXPECT_EQ(network.stopIndex("inter"), network.node(label.at()).stop());

  label = result.matrix.parent(label);
  EXPECT_EQ(0, label.cost());
  EXPECT_EQ(0, label.penalty());
  EXPECT_EQ(Node::DEPARTURE, network.node(label.at()).type());
  EXPECT_EQ(network.stopIndex("start"), network.node(label.at()).stop());
}


/*
       (100,0)  (0,0)       (100,0)
  s------------n1---n2------------------t @200
                 \(10,1)
                 n3
                  \(0,0)     (20,0)
  n5----------n6---n4-------------------t @130

 n1, n2, n3, n4, n6 belong to one station
*/
TEST_F(DijkstraTest, multipleSolutions) {
  TransitNetwork network;
  QueryResult result;
  Stop stopStart("start", "stopName", 100, 100);
  Stop stopInter("inter", "stopName2", 200, 100);
  Stop stopTarget("target", "stopName3", 300, 100);
  Stop stopOther("other", "stopName4", 400, 100);
  network.addStop(stopStart);
  network.addStop(stopInter);
  network.addStop(stopTarget);
  network.addStop(stopOther);

  int s, n1, n2, n3, n4, n5, n6, t1, t2;
  s = network.addTransitNode(network.stopIndex("start"),
                             Node::DEPARTURE, 0);
  n5 = network.addTransitNode(network.stopIndex("other"),
                              Node::DEPARTURE, 0);
  n1 = network.addTransitNode(network.stopIndex("inter"),
                              Node::ARRIVAL, 100*60);
  n2 = network.addTransitNode(network.stopIndex("inter"),
                              Node::DEPARTURE, 100*60);
  n3 = network.addTransitNode(network.stopIndex("inter"),
                              Node::TRANSFER, 110*60);
  n4 = network.addTransitNode(network.stopIndex("inter"),
                              Node::DEPARTURE, 110*60);
  n6 = network.addTransitNode(network.stopIndex("inter"),
                              Node::ARRIVAL, 110*60);
  t1 = network.addTransitNode(network.stopIndex("target"),
                              Node::ARRIVAL, 130*60);
  t2 = network.addTransitNode(network.stopIndex("target"),
                              Node::ARRIVAL, 200*60);

  network.addArc(s, n1, 100*60);
  network.addArc(n1, n2, 0);
  network.addArc(n1, n3, 10*60, 1);
  network.addArc(n2, t2, 100*60);
  network.addArc(n3, n4, 0);
  network.addArc(n4, t1, 20*60);
  network.addArc(n5, n6, 110*60);
  network.addArc(n6, n4, 0);
  stopTarget.addNodeIndex(t1);
  stopTarget.addNodeIndex(t2);

  network.preprocess();

  Dijkstra dijkstra(network);
  dijkstra.logger(&log);
  vector<int> depNodes;
  depNodes.push_back(s);
  dijkstra.findShortestPath(depNodes, network.stopIndex("target"), &result);
  EXPECT_EQ(130*60, result.optimalCosts());
  EXPECT_EQ(0, result.optimalPenalty());
  EXPECT_EQ(2, result.destLabels.size());

  LabelVec::const_iterator it;
  for (it = result.destLabels.begin();  it != result.destLabels.end(); ++it) {
    LabelVec::Hnd label = *it;
    EXPECT_EQ(network.stopIndex("target"), network.node(label.at()).stop());
    EXPECT_EQ(Node::ARRIVAL, network.node(label.at()).type());
    if (label.cost() == 130*60 && label.penalty() == 1) {
      label = result.matrix.parent(label);
      EXPECT_EQ(network.stopIndex("inter"), network.node(label.at()).stop());
      EXPECT_EQ(Node::DEPARTURE, network.node(label.at()).type());
      label = result.matrix.parent(label);
      EXPECT_EQ(network.stopIndex("inter"), network.node(label.at()).stop());
      EXPECT_EQ(Node::TRANSFER, network.node(label.at()).type());
      label = result.matrix.parent(label);
      EXPECT_EQ(network.stopIndex("inter"), network.node(label.at()).stop());
      EXPECT_EQ(Node::ARRIVAL, network.node(label.at()).type());
      label = result.matrix.parent(label);
      EXPECT_EQ(network.stopIndex("start"), network.node(label.at()).stop());
      EXPECT_EQ(Node::DEPARTURE, network.node(label.at()).type());
    } else if (label.cost() == 200*60 && label.penalty() == 0) {
      label = result.matrix.parent(label);
      EXPECT_EQ(network.stopIndex("inter"), network.node(label.at()).stop());
      EXPECT_EQ(Node::DEPARTURE, network.node(label.at()).type());
      label = result.matrix.parent(label);
      EXPECT_EQ(network.stopIndex("inter"), network.node(label.at()).stop());
      EXPECT_EQ(Node::ARRIVAL, network.node(label.at()).type());
      label = result.matrix.parent(label);
      EXPECT_EQ(network.stopIndex("start"), network.node(label.at()).stop());
      EXPECT_EQ(Node::DEPARTURE, network.node(label.at()).type());
    } else {
      EXPECT_EQ(true, false);
    }
  }
}


/*
   s1            (200,0)
   @0h------------------------------t @200  = {(200,0), (100,1)}
                                    |
                                    |
                                    | (40,0)
        (50,0)     (10,1)   (0,0)   |
   s2------------n1------n2--------n3
   @100h         @150    @160   @160

 n1, n2, n3 belong to one stop
*/
TEST_F(DijkstraTest, mulitpleStartNodes) {
  TransitNetwork network;
  Stop stopStart("start", "stopName", 100, 100);
  Stop stopInter("inter", "stopName2", 200, 100);
  Stop stopTarget("target", "stopName3", 300, 100);
  network.addStop(stopStart);
  network.addStop(stopInter);
  network.addStop(stopTarget);

  int s1 = network.addTransitNode(network.stopIndex("start"),
                                  Node::DEPARTURE, 0);
  int s2 = network.addTransitNode(network.stopIndex("start"),
                                  Node::DEPARTURE, 100*60);
  int n1 = network.addTransitNode(network.stopIndex("inter"),
                                  Node::ARRIVAL, 150*60);
  int n2 = network.addTransitNode(network.stopIndex("inter"),
                                  Node::TRANSFER, 160*60);
  int n3 = network.addTransitNode(network.stopIndex("inter"),
                                  Node::DEPARTURE, 160*60);
  int t = network.addTransitNode(network.stopIndex("target"),
                                 Node::ARRIVAL, 200*60);

  stopTarget.addNodeIndex(t);
  network.addArc(s1, t, 200*60, 0);
  network.addArc(s2, n1, 50*60, 0);
  network.addArc(n1, n2, 10*60, 1);
  network.addArc(n2, n3, 0, 0);
  network.addArc(n3, t, 40*60, 0);

  network.preprocess();

  Dijkstra dijkstra(network);
  dijkstra.logger(&log);
  vector<int> depNodes;
  depNodes.push_back(s1);
  depNodes.push_back(s2);
  QueryResult result;
  dijkstra.findShortestPath(depNodes, network.stopIndex("target"), &result);
  EXPECT_EQ(100*60, result.optimalCosts());
  EXPECT_EQ(0, result.optimalPenalty());
  EXPECT_EQ(2, result.destLabels.size());

  LabelVec::const_iterator it;
  for (it = result.destLabels.begin();  it != result.destLabels.end(); ++it) {
    LabelVec::Hnd label = *it;
    EXPECT_EQ(network.stopIndex("target"), network.node(label.at()).stop());
    EXPECT_EQ(Node::ARRIVAL, network.node(label.at()).type());
      if (label.cost() == 100*60 && label.penalty() == 1) {
      label = result.matrix.parent(label);
      EXPECT_EQ(network.stopIndex("inter"), network.node(label.at()).stop());
      EXPECT_EQ(Node::DEPARTURE, network.node(label.at()).type());
      label = result.matrix.parent(label);
      EXPECT_EQ(network.stopIndex("inter"), network.node(label.at()).stop());
      EXPECT_EQ(Node::TRANSFER, network.node(label.at()).type());
      label = result.matrix.parent(label);
      EXPECT_EQ(network.stopIndex("inter"), network.node(label.at()).stop());
      EXPECT_EQ(Node::ARRIVAL, network.node(label.at()).type());
      label = result.matrix.parent(label);
      EXPECT_EQ(network.stopIndex("start"), network.node(label.at()).stop());
      EXPECT_EQ(Node::DEPARTURE, network.node(label.at()).type());
    } else if (label.cost() == 200*60 && label.penalty() == 0) {
      label = result.matrix.parent(label);
      EXPECT_EQ(network.stopIndex("start"), network.node(label.at()).stop());
      EXPECT_EQ(Node::DEPARTURE, network.node(label.at()).type());
    } else {
      EXPECT_EQ(true, false);
    }
  }
}


/*
     ________________t1
    /100
   /
  /   50    0    40
 s-------n1---n2-----t2
  \
   \60
    \____n3___n4_____t3
            0    20
*/
TEST_F(DijkstraTest, noneOptimalPaths1) {
  TransitNetwork network;
  Stop stopStart("start", "stopName", 100, 100);
  Stop stopInter1("inter1", "stopName2", 200, 100);
  Stop stopInter2("inter2", "stopName2", 300, 100);
  Stop stopTarget("target", "stopName3", 400, 100);
  network.addStop(stopStart);
  network.addStop(stopInter1);
  network.addStop(stopInter2);
  network.addStop(stopTarget);

  int s = network.addTransitNode(network.stopIndex("start"),
                                 Node::DEPARTURE, 0);
  int n1 = network.addTransitNode(network.stopIndex("inter1"),
                                  Node::ARRIVAL, 50*60);
  int n2 = network.addTransitNode(network.stopIndex("inter1"),
                                  Node::DEPARTURE, 50*60);
  int n3 = network.addTransitNode(network.stopIndex("inter2"),
                                  Node::ARRIVAL, 60*60);
  int n4 = network.addTransitNode(network.stopIndex("inter2"),
                                  Node::DEPARTURE, 60*60);
  int t3 = network.addTransitNode(network.stopIndex("target"),
                                  Node::ARRIVAL, 80*60);
  int t2 = network.addTransitNode(network.stopIndex("target"),
                                  Node::ARRIVAL, 90*60);
  int t1 = network.addTransitNode(network.stopIndex("target"),
                                  Node::ARRIVAL, 100*60);

  stopTarget.addNodeIndex(t1);
  stopTarget.addNodeIndex(t2);
  stopTarget.addNodeIndex(t3);

  network.addArc(s, t1, 100*60, 0);
  network.addArc(s, n1, 50*60, 0);
  network.addArc(s, n3, 60*60, 0);
  network.addArc(n1, n2, 0, 0);
  network.addArc(n3, n4, 0, 0);
  network.addArc(n2, t2, 40*60, 0);
  network.addArc(n4, t3, 20*60, 0);

  network.preprocess();

  Dijkstra dijkstra(network);
  dijkstra.logger(&log);
  vector<int> depNodes;
  depNodes.push_back(s);
  QueryResult result;
  dijkstra.findShortestPath(depNodes, network.stopIndex("target"), &result);
  EXPECT_EQ(80*60, result.optimalCosts());
  EXPECT_EQ(0, result.optimalPenalty());
  EXPECT_EQ(1, result.destLabels.size());

  LabelVec::Hnd label = *(result.destLabels.begin());
  EXPECT_EQ(network.stopIndex("target"), network.node(label.at()).stop());
  EXPECT_EQ(Node::ARRIVAL, network.node(label.at()).type());
  label = result.matrix.parent(label);
  EXPECT_EQ(network.stopIndex("inter2"), network.node(label.at()).stop());
  EXPECT_EQ(Node::DEPARTURE, network.node(label.at()).type());
  label = result.matrix.parent(label);
  EXPECT_EQ(network.stopIndex("inter2"), network.node(label.at()).stop());
  EXPECT_EQ(Node::ARRIVAL, network.node(label.at()).type());
  label = result.matrix.parent(label);
  EXPECT_EQ(network.stopIndex("start"), network.node(label.at()).stop());
  EXPECT_EQ(Node::DEPARTURE, network.node(label.at()).type());
}


/*
 s1_________________
 @0    100          \
                     \
      50     0    40  \
 s2-------n1---n2------t @100
 @10                  /
                     /
 s3_______n3___n4___/
 @20  60     0      20
*/
TEST_F(DijkstraTest, noneOptimalPaths2) {
  TransitNetwork network;

  Stop stopStart("start", "stopName", 100, 100);
  Stop stopInter1("inter1", "stopName2", 200, 100);
  Stop stopInter2("inter2", "stopName2", 300, 100);
  Stop stopTarget("target", "stopName3", 400, 100);

  network.addStop(stopStart);
  network.addStop(stopInter1);
  network.addStop(stopInter2);
  network.addStop(stopTarget);

  int s1 = network.addTransitNode(network.stopIndex("start"),
                                  Node::DEPARTURE, 0);
  int s2 = network.addTransitNode(network.stopIndex("start"),
                                  Node::DEPARTURE, 10*60);
  int s3 = network.addTransitNode(network.stopIndex("start"),
                                  Node::DEPARTURE, 20*60);
  int n1 = network.addTransitNode(network.stopIndex("inter1"),
                                  Node::ARRIVAL, 60*60);
  int n2 = network.addTransitNode(network.stopIndex("inter1"),
                                  Node::DEPARTURE, 60*60);
  int n3 = network.addTransitNode(network.stopIndex("inter2"),
                                  Node::ARRIVAL, 80*60);
  int n4 = network.addTransitNode(network.stopIndex("inter2"),
                                  Node::DEPARTURE, 80*60);
  int t = network.addTransitNode(network.stopIndex("target"),
                                 Node::ARRIVAL, 100*60);

  stopTarget.addNodeIndex(t);

  network.addArc(s1, t, 100*60, 0);
  network.addArc(s2, n1, 50*60, 0);
  network.addArc(s3, n3, 60*60, 0);
  network.addArc(n1, n2, 0, 0);
  network.addArc(n3, n4, 0, 0);
  network.addArc(n2, t, 40*60, 0);
  network.addArc(n4, t, 20*60, 0);

  network.preprocess();

  Dijkstra dijkstra(network);
  dijkstra.logger(&log);
  vector<int> depNodes;
  depNodes.push_back(s1);
  depNodes.push_back(s2);
  depNodes.push_back(s3);

  QueryResult result;
  dijkstra.findShortestPath(depNodes, network.stopIndex("target"), &result);

  EXPECT_EQ(80*60, result.optimalCosts());
  EXPECT_EQ(0, result.optimalPenalty());
  EXPECT_EQ(1, result.destLabels.size());

  LabelVec::Hnd label = *(result.destLabels.begin());
  EXPECT_EQ(network.stopIndex("target"), network.node(label.at()).stop());
  EXPECT_EQ(Node::ARRIVAL, network.node(label.at()).type());
  label = result.matrix.parent(label);
  EXPECT_EQ(network.stopIndex("inter2"), network.node(label.at()).stop());
  EXPECT_EQ(Node::DEPARTURE, network.node(label.at()).type());
  label = result.matrix.parent(label);
  EXPECT_EQ(network.stopIndex("inter2"), network.node(label.at()).stop());
  EXPECT_EQ(Node::ARRIVAL, network.node(label.at()).type());
  label = result.matrix.parent(label);
  EXPECT_EQ(network.stopIndex("start"), network.node(label.at()).stop());
  EXPECT_EQ(Node::DEPARTURE, network.node(label.at()).type());
}

/*

 s1    (50,0)   (0,0)            (150,0)
 @0----------n1------n2----------------------t1 @200
             @50     @50


         (15,1)   (0,0)          (35,0)
       n3------n4--------n5-------------t2 @50
       @50      @65       @65

 n4 is reachable from n1 by walk with costs of 7
*/
TEST_F(DijkstraTest, changeTransferByWalking) {
  TransitNetwork network;
  Stop stopStart("start", "stopName", 10, 10);
  Stop stopInter1("inter1", "stopName2", 100, 100);
  Stop stopInter2("inter2", "stopName2", 100.0001, 100);
  Stop stopTarget("target", "stopName3", 2000, 2000);
  network.addStop(stopStart);
  network.addStop(stopInter1);
  network.addStop(stopInter2);
  network.addStop(stopTarget);

  int s1 = network.addTransitNode(network.stopIndex("start"),
                                  Node::DEPARTURE, 0);
  int n1 = network.addTransitNode(network.stopIndex("inter1"),
                                  Node::ARRIVAL, 50*60);
  int n2 = network.addTransitNode(network.stopIndex("inter1"),
                                  Node::DEPARTURE, 50*60);
  int n3 = network.addTransitNode(network.stopIndex("inter2"),
                                  Node::ARRIVAL, 50*60);
  int n4 = network.addTransitNode(network.stopIndex("inter2"),
                                  Node::TRANSFER, 65*60);
  int n5 = network.addTransitNode(network.stopIndex("inter2"),
                                  Node::DEPARTURE, 65*60);
  int t2 = network.addTransitNode(network.stopIndex("target"),
                                  Node::ARRIVAL, 100*60);
  int t1 = network.addTransitNode(network.stopIndex("target"),
                                  Node::ARRIVAL, 200*60);

  network.stop(network.stopIndex("start")).addNodeIndex(s1);
  network.stop(network.stopIndex("inter1")).addNodeIndex(n1);
  network.stop(network.stopIndex("inter1")).addNodeIndex(n2);
  network.stop(network.stopIndex("inter2")).addNodeIndex(n3);
  network.stop(network.stopIndex("inter2")).addNodeIndex(n4);
  network.stop(network.stopIndex("inter2")).addNodeIndex(n5);
  network.stop(network.stopIndex("target")).addNodeIndex(t1);
  network.stop(network.stopIndex("target")).addNodeIndex(t2);

  network.addArc(s1, n1, 50*60, 0);
  network.addArc(n1, n2, 0, 0);
  network.addArc(n2, t1, 150*60, 0);
  network.addArc(n3, n4, 15*60, 1);
  network.addArc(n4, n5, 0, 0);
  network.addArc(n5, t2, 35*60, 0);

  network.preprocess();

  Dijkstra dijkstra(network);
  dijkstra.logger(&log);
  vector<int> depNodes;
  depNodes.push_back(s1);
  QueryResult result;
  dijkstra.findShortestPath(depNodes, network.stopIndex("target"), &result);

  EXPECT_EQ(100*60, result.optimalCosts());
  EXPECT_EQ(0, result.optimalPenalty());
  EXPECT_EQ(2, result.destLabels.size());

  LabelVec::const_iterator it;
  for (it = result.destLabels.begin();  it != result.destLabels.end(); ++it) {
    LabelVec::Hnd label = *it;
    if (label.cost() == 100*60 && label.penalty() == 1) {
      EXPECT_EQ(network.stopIndex("target"), network.node(label.at()).stop());
      label = result.matrix.parent(label);
      EXPECT_EQ(network.stopIndex("inter2"), network.node(label.at()).stop());
      label = result.matrix.parent(label);
      EXPECT_EQ(network.stopIndex("inter2"), network.node(label.at()).stop());
      label = result.matrix.parent(label);
      EXPECT_EQ(network.stopIndex("inter1"), network.node(label.at()).stop());
      label = result.matrix.parent(label);
      EXPECT_EQ(network.stopIndex("start"), network.node(label.at()).stop());
    } else if (label.cost() == 200*60 && label.penalty() == 0) {
      EXPECT_EQ(network.stopIndex("target"), network.node(label.at()).stop());
      label = result.matrix.parent(label);
      EXPECT_EQ(network.stopIndex("inter1"), network.node(label.at()).stop());
      label = result.matrix.parent(label);
      EXPECT_EQ(network.stopIndex("inter1"), network.node(label.at()).stop());
      label = result.matrix.parent(label);
      EXPECT_EQ(network.stopIndex("start"), network.node(label.at()).stop());
    } else {
      EXPECT_EQ(true, false);
    }
  }
}

/*   ________________t1
    /30
   /
  /   10    0    10
 s-------n1---n2-----t2
  \
   \60
    \____n3___n4_____n5_________n6
            0    60   |    60
                      |
                      | 120
                      |
                      n7
*/
TEST_F(DijkstraTest, fullDijkstra) {
  TransitNetwork network;
  Stop stopStart("start", "stopName", 100, 100);
  Stop stopInter1("inter1", "stopName2", 200, 100);
  Stop stopInter2("inter2", "stopName2", 300, 100);
  Stop stopTarget("target", "stopName3", 400, 100);
  Stop stopEnd("end", "stopName4", 100, 500);
  network.addStop(stopStart);
  network.addStop(stopInter1);
  network.addStop(stopInter2);
  network.addStop(stopTarget);
  network.addStop(stopEnd);

  int s = network.addTransitNode(network.stopIndex("start"),
                                 Node::DEPARTURE, 0);
  int n1 = network.addTransitNode(network.stopIndex("inter1"),
                                  Node::ARRIVAL, 10*60);
  int n2 = network.addTransitNode(network.stopIndex("inter1"),
                                  Node::DEPARTURE, 10*60);
  int t2 = network.addTransitNode(network.stopIndex("target"),
                                  Node::ARRIVAL, 20*60);
  int t1 = network.addTransitNode(network.stopIndex("target"),
                                  Node::ARRIVAL, 30*60);
  int n3 = network.addTransitNode(network.stopIndex("inter2"),
                                  Node::ARRIVAL, 60*60);
  int n4 = network.addTransitNode(network.stopIndex("inter2"),
                                  Node::DEPARTURE, 60*60);
  int n5 = network.addTransitNode(network.stopIndex("end"),
                                  Node::ARRIVAL, 120*60);
  int n6 = network.addTransitNode(network.stopIndex("end"),
                                  Node::DEPARTURE, 180*60);
  int n7 = network.addTransitNode(network.stopIndex("end"),
                                  Node::TRANSFER, 240*60);

  network.stop(network.stopIndex("target")).addNodeIndex(t1);
  network.stop(network.stopIndex("target")).addNodeIndex(t2);

  network.addArc(s, t1, 30*60, 0);
  network.addArc(s, n1, 10*60, 0);
  network.addArc(s, n3, 60*60, 0);
  network.addArc(n1, n2, 0, 0);
  network.addArc(n3, n4, 0, 0);
  network.addArc(n2, t2, 10*60, 0);
  network.addArc(n4, n5, 60*60, 0);
  network.addArc(n5, n6, 60*60, 0);
  network.addArc(n5, n7, 120*60, 0);

  network.preprocess();

  Dijkstra dijkstra(network);
  dijkstra.logger(&log);
  vector<int> depNodes;
  depNodes.push_back(s);
  QueryResult result;
  dijkstra.findShortestPath(depNodes, INT_MAX, &result);

  EXPECT_EQ(10, result.numSettledLabels);

  EXPECT_EQ(0, (*(result.matrix.at(s).begin())).cost());
  EXPECT_EQ(0, (*(result.matrix.at(s).begin())).penalty());
  EXPECT_EQ(10*60, (*(result.matrix.at(n1).begin())).cost());
  EXPECT_EQ(0, (*(result.matrix.at(n1).begin())).penalty());
  EXPECT_EQ(10*60, (*(result.matrix.at(n2).begin())).cost());
  EXPECT_EQ(0, (*(result.matrix.at(n2).begin())).penalty());
  EXPECT_EQ(60*60, (*(result.matrix.at(n3).begin())).cost());
  EXPECT_EQ(0, (*(result.matrix.at(n3).begin())).penalty());
  EXPECT_EQ(60*60, (*(result.matrix.at(n4).begin())).cost());
  EXPECT_EQ(0, (*(result.matrix.at(n4).begin())).penalty());
  EXPECT_EQ(120*60, (*(result.matrix.at(n5).begin())).cost());
  EXPECT_EQ(0, (*(result.matrix.at(n5).begin())).penalty());
  EXPECT_EQ(180*60, (*(result.matrix.at(n6).begin())).cost());
  EXPECT_EQ(0, (*(result.matrix.at(n6).begin())).penalty());
  EXPECT_EQ(240*60, (*(result.matrix.at(n7).begin())).cost());
  EXPECT_EQ(0, (*(result.matrix.at(n7).begin())).penalty());
  EXPECT_EQ(30*60, (*(result.matrix.at(t1).begin())).cost());
  EXPECT_EQ(0, (*(result.matrix.at(t1).begin())).penalty());
  EXPECT_EQ(20*60, (*(result.matrix.at(t2).begin())).cost());
  EXPECT_EQ(0, (*(result.matrix.at(t2).begin())).penalty());
}

/* TIME LIMIT = 50. Nodes n5, n6, n7, t2 should not be expanded
     ________________t1
    /30
   /
  /   20    0    40
 s-------n1---n2-----t2
  \
   \10
    \____n3___n4_____n5_________n6
            0    60   |    10
                      |
                      | 50
                      |
                      n7
*/
TEST_F(DijkstraTest, timeLimitedDijkstra) {
  TransitNetwork network;
  Stop stop1("stop1", "stop1", 100, 100);
  Stop stop2("stop2", "stop2", 1000, 1000);
  Stop stop3("stop3", "stop3", 10000, 10000);
  Stop stop4("stop4", "stop4", 100000, 100000);
  Stop stop5("stop5", "stop5", 1000000, 1000000);
  network.addStop(stop1);
  network.addStop(stop2);
  network.addStop(stop3);
  network.addStop(stop4);
  network.addStop(stop5);

  int s = network.addTransitNode(network.stopIndex("stop1"),
                                 Node::DEPARTURE, 0);
  int n1 = network.addTransitNode(network.stopIndex("stop2"),
                                  Node::ARRIVAL, 10*60);
  int n2 = network.addTransitNode(network.stopIndex("stop2"),
                                  Node::DEPARTURE, 10*60);
  int t2 = network.addTransitNode(network.stopIndex("stop4"),
                                  Node::ARRIVAL, 20*60);
  int t1 = network.addTransitNode(network.stopIndex("stop4"),
                                  Node::ARRIVAL, 30*60);
  int n3 = network.addTransitNode(network.stopIndex("stop3"),
                                  Node::ARRIVAL, 60*60);
  int n4 = network.addTransitNode(network.stopIndex("stop3"),
                                  Node::DEPARTURE, 60*60);
  int n5 = network.addTransitNode(network.stopIndex("stop5"),
                                  Node::ARRIVAL, 120*60);
  int n6 = network.addTransitNode(network.stopIndex("stop5"),
                                  Node::DEPARTURE, 180*60);
  int n7 = network.addTransitNode(network.stopIndex("stop5"),
                                  Node::TRANSFER, 240*60);

  network.stop(network.stopIndex("stop1")).addNodeIndex(s);

  network.stop(network.stopIndex("stop2")).addNodeIndex(n1);
  network.stop(network.stopIndex("stop2")).addNodeIndex(n2);

  network.stop(network.stopIndex("stop3")).addNodeIndex(n3);
  network.stop(network.stopIndex("stop3")).addNodeIndex(n3);

  network.stop(network.stopIndex("stop4")).addNodeIndex(t1);
  network.stop(network.stopIndex("stop4")).addNodeIndex(t2);

  network.stop(network.stopIndex("stop5")).addNodeIndex(n5);
  network.stop(network.stopIndex("stop5")).addNodeIndex(n6);
  network.stop(network.stopIndex("stop5")).addNodeIndex(n7);

  network.addArc(s, t1, 30*60, 0);
  network.addArc(s, n1, 20*60, 0);
  network.addArc(s, n3, 10*60, 0);
  network.addArc(n1, n2, 0, 0);
  network.addArc(n3, n4, 0, 0);
  network.addArc(n2, t2, 40*60, 0);
  network.addArc(n4, n5, 60*60, 0);
  network.addArc(n5, n6, 10*60, 0);
  network.addArc(n5, n7, 50*60, 0);

  network.preprocess();

  Dijkstra dijkstra(network);
  dijkstra.logger(&log);
  vector<int> depNodes;
  depNodes.push_back(s);
  QueryResult result;
  dijkstra.maxCost(50*60);
  dijkstra.findShortestPath(depNodes, INT_MAX, &result);

  // TIME LIMIT = 50. Nodes n5m n6, n7 and t2 should not be expanded
  EXPECT_LE(0, result.matrix.at(s).size());
  EXPECT_LE(0, result.matrix.at(n1).size());
  EXPECT_LE(0, result.matrix.at(n2).size());
  EXPECT_LE(0, result.matrix.at(n3).size());
  EXPECT_LE(0, result.matrix.at(n4).size());
  EXPECT_EQ(0, result.matrix.at(n5).size());
  EXPECT_EQ(0, result.matrix.at(n6).size());
  EXPECT_EQ(0, result.matrix.at(n7).size());
  EXPECT_EQ(0, result.matrix.at(t2).size());
}


// _____________________________________________________________________________
TEST(QuerySearchTest, dijkstraTest) {
  TransitNetwork network;
  GtfsParser parser;
  vector<Line> lines;
  parser.logger(&LOG);
  LOG.enabled(false);
  network = parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                        "20120118T000000", "20120119T000000",
                                        &lines);
  network.preprocess();

  TransferPatternRouter router(network);
  router.logger(&LOG);
  router.prepare(lines);
  TPDB tpdb;
  router.transferPatternsDB(tpdb);
  router.computeAllTransferPatterns(&tpdb);

  vector<QueryResult::Path> paths = router.shortestPath(network.stopIndex("A"),
                                           str2time("20120118T000000"),
                                           network.stopIndex("C"));

  vector<int> pattern1 = {network.stopIndex("A"), network.stopIndex("C")};

  ASSERT_EQ(1, paths.size());
  const LabelVec::Hnd label = (*(paths.begin())).first;
  EXPECT_EQ(5 * 60 + 25 * 60, label.cost());
  EXPECT_EQ(0, label.penalty());
  EXPECT_EQ(pattern1, (*(paths.begin())).second);

  paths = router.shortestPath(network.stopIndex("D"),
                              str2time("20120118T000000"),
                              network.stopIndex("C"));
  vector<int> pattern2 = {network.stopIndex("D"), network.stopIndex("BC"),
                          network.stopIndex("C")};

  ASSERT_EQ(1, paths.size());
  const LabelVec::Hnd label2 = (*(paths.begin())).first;
  EXPECT_EQ(1 * 60 + 29*60, label2.cost());
  EXPECT_EQ(1, label2.penalty());
  EXPECT_EQ(pattern2, (*(paths.begin())).second);

  // No connection:
  paths = router.shortestPath(network.stopIndex("A"),
                              str2time("20120118T000000"),
                              network.stopIndex("D"));
  EXPECT_EQ(0, paths.size());
}


// _____________________________________________________________________________
TEST(QuerySearchTest, multipleSolutions) {
  TransitNetwork network;
  GtfsParser parser;
  vector<Line> lines;
  parser.logger(&LOG);
  LOG.enabled(false);
  network = parser.createTransitNetwork("test/data/query-test-case/",
                                        "20120118T000000", "20120119T000000",
                                        &lines);
  network.preprocess();

  TransferPatternRouter router(network);
  router.logger(&LOG);
  router.prepare(lines);
  TPDB tpdb;
  router.transferPatternsDB(tpdb);
  router.computeAllTransferPatterns(&tpdb);

  vector<QueryResult::Path> paths = router.shortestPath(network.stopIndex("F"),
                                                    str2time("20120118T000000"),
                                                    network.stopIndex("Z"));

  vector<int> pattern1 = {network.stopIndex("F"), network.stopIndex("Z")};
  vector<int> pattern2 = {network.stopIndex("F"), network.stopIndex("B"),
                          network.stopIndex("Z")};

  EXPECT_EQ(2, paths.size());

  /*LabelVec::Hnd hnd1(110*60, 0, NULL);
  QueryResult::Path expected1 = {hnd1, pattern1};
  LabelVec::Hnd hnd2(90*60, 1, NULL);
  QueryResult::Path expected2 = {hnd2, pattern2};
  EXPECT_THAT(paths, Contains(expected1));
  EXPECT_THAT(paths, Contains(expected2));*/

  for (auto it = paths.begin(); it != paths.end(); it++) {
    const LabelVec::Hnd hnd = (*it).first;
    const vector<int> path = (*it).second;

    if (hnd.cost() == 5 * 60 + 110 * 60 && hnd.penalty() == 0) {
      EXPECT_EQ(pattern1, path);
    } else if (hnd.cost() == 5 * 60 + 90 * 60 && hnd.penalty() == 1) {
      EXPECT_EQ(pattern2, path);
    } else {
      printf("Cost: %d Penalty: %d\n", hnd.cost(), hnd.penalty());
      EXPECT_EQ(true, false);
    }
  }
}


// _____________________________________________________________________________
TEST(QuerySearchTest, walkInPattern) {
  TransitNetwork network;
  GtfsParser parser;
  vector<Line> lines;
  parser.logger(&LOG);
  LOG.enabled(false);
  network = parser.createTransitNetwork("test/data/walk-test-case3/",
                                        "20120118T000000", "20120119T000000",
                                        &lines);
  network.preprocess();

  TransferPatternRouter router(network);
  router.logger(&LOG);
  router.prepare(lines);
  TPDB tpdb;
  router.transferPatternsDB(tpdb);
  router.computeAllTransferPatterns(&tpdb);


  vector<QueryResult::Path> paths = router.shortestPath(network.stopIndex("F"),
                                                    str2time("20120118T000000"),
                                                    network.stopIndex("Z"));

  vector<int> pattern1 = {network.stopIndex("F"), network.stopIndex("Z")};
  vector<int> pattern2 = {network.stopIndex("F"), network.stopIndex("W"),
                          network.stopIndex("B"), network.stopIndex("Z")};

  EXPECT_EQ(2, paths.size());

  /*
  for (auto it = paths.begin(); it != paths.end(); it++) {
    const LabelVec::Hnd hnd = (*it).first;
    const vector<int> path = (*it).second;

    if (hnd.cost() == 115 * 60 && hnd.penalty() == 0) {
      EXPECT_EQ(pattern1, path);
    } else if (hnd.cost() == 95 * 60 && hnd.penalty() == 1) {
      EXPECT_EQ(pattern2, path);
    } else {
      EXPECT_EQ(true, false);
    }
  }
  */
}

/*

s1 ---> s2 --> h ---> s3 ---> s4

*/
TEST_F(DijkstraTest, limitedSearchToHubs) {
  TransitNetwork network;
  Stop stop1("s1", "s1", 100, 100);
  Stop stop2("s2", "s2", 100, 1000);
  Stop stop3("s3", "s3", 1000, 1000);
  Stop stop4("s4", "s4", 10000, 1000);
  Stop stopHub("h", "h", 1000, 100);
  network.addStop(stop1);
  network.addStop(stop2);
  network.addStop(stop3);
  network.addStop(stop4);
  network.addStop(stopHub);

  int n1a = network.addTransitNode(network.stopIndex("s1"),
                                   Node::ARRIVAL, 0);
  int n1d = network.addTransitNode(network.stopIndex("s1"),
                                   Node::DEPARTURE, 10*60);
  int n2a = network.addTransitNode(network.stopIndex("s2"),
                                   Node::ARRIVAL, 100*60);
  int n2d = network.addTransitNode(network.stopIndex("s2"),
                                   Node::DEPARTURE, 110*60);
  int nha = network.addTransitNode(network.stopIndex("h"),
                                   Node::ARRIVAL, 200*60);
  int nht = network.addTransitNode(network.stopIndex("h"),
                                   Node::TRANSFER, 200*60);
  int nhd = network.addTransitNode(network.stopIndex("h"),
                                   Node::DEPARTURE, 210*60);
  int n3a = network.addTransitNode(network.stopIndex("s3"),
                                   Node::ARRIVAL, 300*60);
  int n3d = network.addTransitNode(network.stopIndex("s3"),
                                   Node::DEPARTURE, 310*60);
  int n4a = network.addTransitNode(network.stopIndex("s4"),
                                   Node::ARRIVAL, 400*60);
  int n4d = network.addTransitNode(network.stopIndex("s4"),
                                   Node::DEPARTURE, 410*60);

  network.addArc(n1a, n1d, 10*60, 0);
  network.addArc(n1d, n2a, 100*60, 0);
  network.addArc(n2a, n2d, 10*60, 0);
  network.addArc(n2d, nha, 100*60, 0);
  network.addArc(nha, nht, 0, 0);
  network.addArc(nht, nhd, 10*60, 0);
  network.addArc(nhd, n3a, 100*60, 0);
  network.addArc(n3a, n3d, 10*60, 0);
  network.addArc(n3d, n4a, 100*60, 0);
  network.addArc(n4a, n4d, 10*60, 0);

  network.preprocess();

  HubSet hubs;
  hubs.insert(network.stopIndex("h"));

  vector<int> depNodes;
  depNodes.push_back(n1d);

  Dijkstra dijkstra(network);
  dijkstra.logger(&LOG);
  QueryResult result;
  dijkstra.maxPenalty(3);
  dijkstra.hubs(&hubs);
  dijkstra.findShortestPath(depNodes, INT_MAX, &result);

  set<int> expectedStops;
  expectedStops.insert(network.stopIndex("s1"));
  expectedStops.insert(network.stopIndex("s2"));
  expectedStops.insert(network.stopIndex("h"));
  set<int> settledStops;
  for (int i = 0; i < result.matrix.size(); ++i) {
    if (result.matrix.at(i).size() != 0) {
      settledStops.insert(network.node(i).stop());
    }
  }
  EXPECT_EQ(expectedStops, settledStops);
  EXPECT_EQ(0, result.matrix.at(n4a).size());
  EXPECT_EQ(0, result.matrix.at(n4d).size());
}


/*

s1---------s2------>s5
 \         |
  \        |
   s3------s4

*/
TEST_F(DijkstraTest, compressedNetwork) {
  TransitNetwork network;
  Stop stop1("stop1", "stopName1", 100, 100);
  Stop stop2("stop2", "stopName2", 1000, 100);
  Stop stop3("stop3", "stopName3", 100, 1000);
  Stop stop4("stop4", "stopName4", 10000, 100);
  Stop stop5("stop5", "stopName5", 100, 10000);
  network.addStop(stop1);
  network.addStop(stop2);
  network.addStop(stop3);
  network.addStop(stop4);
  network.addStop(stop5);

  int n1 = network.addTransitNode(network.stopIndex("stop1"), Node::NONE, 0);
  int n2 = network.addTransitNode(network.stopIndex("stop2"), Node::NONE, 0);
  int n3 = network.addTransitNode(network.stopIndex("stop3"), Node::NONE, 0);
  int n4 = network.addTransitNode(network.stopIndex("stop4"), Node::NONE, 0);
  int n5 = network.addTransitNode(network.stopIndex("stop5"), Node::NONE, 0);
  stop1.addNodeIndex(n1);
  stop2.addNodeIndex(n2);
  stop3.addNodeIndex(n3);
  stop4.addNodeIndex(n4);
  stop5.addNodeIndex(n5);

  network.addArc(n1, n2, 50*60);
  network.addArc(n1, n3, 10*60);
  network.addArc(n2, n4, 60*60);
  network.addArc(n2, n5, 70*60);
  network.addArc(n3, n4, 200*60);

  network.buildKdtreeFromStops();
  network.buildWalkingGraph(TransitNetwork::MAX_WALKWAY_DIST);

  Dijkstra dijkstra(network);
  dijkstra.logger(&log);
  vector<int> depNodes;
  depNodes.push_back(n1);
  QueryResult result;
  dijkstra.findShortestPath(depNodes, INT_MAX, &result);

  EXPECT_EQ(1, result.matrix.at(n1).size());
  EXPECT_EQ(1, result.matrix.at(n2).size());
  EXPECT_EQ(1, result.matrix.at(n3).size());
  EXPECT_EQ(1, result.matrix.at(n4).size());
  EXPECT_EQ(1, result.matrix.at(n5).size());
}

TEST_F(DijkstraTest, getTransferPatterns) {
  TransitNetwork network;
  GtfsParser parser;
  vector<Line> lines;
  parser.logger(&LOG);
  LOG.enabled(false);
  network = parser.createTransitNetwork("test/data/simple-parser-test-case/",
                                  "20120118T000000", "20120118T230000", &lines);
  network.preprocess();

  Dijkstra dijkstra(network);
  QueryResult result;
  vector<int> depNodes;
  vector<int> shortestPathStops;
  vector<int> expectedStops;

  // Query A -> C
  depNodes = network.findStartNodeSequence(network.stop(network.stopIndex("A")),
                                           str2time("20120118T000000"));
  dijkstra.findShortestPath(depNodes, network.stopIndex("C"), &result);
  ASSERT_EQ(1, result.destLabels.size());
  shortestPathStops = result.getTransferPattern(network,
                                                *(result.destLabels.begin()));
  expectedStops = {network.stopIndex("A"), network.stopIndex("C")};
  EXPECT_EQ(expectedStops, shortestPathStops);

  // Query A -> D
  depNodes = network.findStartNodeSequence(network.stop(network.stopIndex("D")),
                                           str2time("20120118T000000"));
  dijkstra.findShortestPath(depNodes, network.stopIndex("C"), &result);
  ASSERT_EQ(1, result.destLabels.size());
  shortestPathStops = result.getTransferPattern(network,
                                                *(result.destLabels.begin()));
  expectedStops = {network.stopIndex("D"), network.stopIndex("BC"),
                   network.stopIndex("C")};
  EXPECT_EQ(expectedStops, shortestPathStops);

  // TODO(Philip): Wrong test case here
  // Test case for walking between to intermediate stops on the path
  network = parser.createTransitNetwork("test/data/walk-test-case2/",
                                  "20120118T000000", "20120118T230000", &lines);
  network.preprocess();

  Dijkstra dijkstra2(network);
  depNodes = network.findStartNodeSequence(network.stop(network.stopIndex("F")),
                                           str2time("20120118T000000"));
  dijkstra2.findShortestPath(depNodes, network.stopIndex("Z"), &result);

  ASSERT_EQ(2, result.destLabels.size());
  auto it = result.destLabels.begin();

  shortestPathStops = result.getTransferPattern(network, *it);
  expectedStops = {network.stopIndex("F"), network.stopIndex("Z")};
  EXPECT_EQ(expectedStops, shortestPathStops);
  it++;
  shortestPathStops = result.getTransferPattern(network, *it);
  expectedStops = {network.stopIndex("F"), network.stopIndex("B"),
                   network.stopIndex("Z")};
  EXPECT_EQ(expectedStops, shortestPathStops);

  // Test case for walking to the dest stop at the end of the path
  network = parser.createTransitNetwork("test/data/walk-test-case3/",
                                  "20120118T000000", "20120118T230000", &lines);
  network.preprocess();

  Dijkstra dijkstra3(network);
  depNodes = network.findStartNodeSequence(network.stop(network.stopIndex("F")),
                                           str2time("20120118T000000"));
  dijkstra3.findShortestPath(depNodes, network.stopIndex("Z"), &result);

  ASSERT_EQ(2, result.destLabels.size());
  it = result.destLabels.begin();

  shortestPathStops = result.getTransferPattern(network, *it);
  expectedStops = {network.stopIndex("F"), network.stopIndex("Z")};
  EXPECT_EQ(expectedStops, shortestPathStops);
  it++;
  shortestPathStops = result.getTransferPattern(network, *it);
  expectedStops = {network.stopIndex("F"), network.stopIndex("W"),
                   network.stopIndex("B"), network.stopIndex("Z")};
  EXPECT_EQ(expectedStops, shortestPathStops);
}
