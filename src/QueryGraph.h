// Copyright 2012: Eugen Sawin, Philip Stahl, Jonas Sternisko
#ifndef SRC_QUERYGRAPH_H_
#define SRC_QUERYGRAPH_H_

#include <queue>
#include <set>
#include <string>
#include <vector>
#include <utility>
#include <map>
#include "gtest/gtest_prod.h"  // Needed for FRIEND_TEST in this case.

using std::set;
using std::string;
using std::vector;

class DirectConnection;
class TransferPatternsGraph;
typedef TransferPatternsGraph TPG;
class LabelVec;

typedef std::pair<int, int> IntPair;
// Label and nodes of all optimal paths.
typedef std::map<IntPair, vector<int> > SearchResult;


// Represents a QueryGraph for the Query(A,B)
class QueryGraph {
 public:
  static const int INVALID_NODE;

  // Constructs a QueryGraph for query (TPG(stop A), stop B).
  QueryGraph(const TPG& tpgOrigin, const int destStop);
  FRIEND_TEST(QueryGraphTest, Constructor);

  // Returns the list of successors of a node.
  const set<int>& successors(const int node) const;

  // Get the stop index of a node.
  int stopIndex(const int node) const;

  // Get the node Index of a stop. Returns INVALID_NODE if the stop is not in
  // the graph.
  int nodeIndex(const int stop) const;

  // Get source and target nodes.
  const int sourceNode() const { return 0; }
  const int targetNode() const;

  // Get the size of the query graph (the number of nodes)
  const size_t size() const { return _stops.size(); }

  // Get the number of arcs in the query graph
  const size_t countArcs() const;

  // Returns true, if there is no outgoing arc from the origin node.
  bool empty() const;

  // Merge the query graph with a second query graph specified by the given tpg
  void merge(const TPG& tpgOrigin, const int destStop);
  FRIEND_TEST(QueryGraphTest, merge);
  FRIEND_TEST(QueryGraphTest, mergeGraphs);

  // Adds the given stop to the query graph
  int addStop(const int stop, const set<int>& successors);
  FRIEND_TEST(QueryGraphTest, addStop);

  // Checks if the query graph contains a transfer pattern.
  bool containsPattern(const vector<int>& stops) const;
  // Checks if the query graph contains a transfer pattern and returns the
  // the corresponding QueryGraph. Returns the empty query graph otherwise.
  const QueryGraph findPattern(const vector<int>& stops) const;

  // Debug: Creates a string representation of the graph.
  string debugString() const;
  // Debug: Generates all transfer patterns represented by the graph.
  const vector<vector<int> > generateTransferPatternsRecursive() const;
  // Debug: Generates all transfer patterns represented by the graph (iterative)
  const vector<vector<int> > generateTransferPatterns() const;

 private:
  // Standard constructor is private.
  QueryGraph();
  // Recursively walks the graph until the destination node. Stores the
  // sequences of nodes traversed (transfer patterns).
  void recurse(const int node, vector<int> stops,
               vector<vector<int> >* patterns) const;

  // Stops in the QueryGraph. Origin is at position 0, destination at 1.
  vector<int> _stops;
  // Map from stop index to node index;
  std::map<int, int> _nodeIndex;

  // List of succeeding nodes for each node in the QueryGraph. == AdjacencyLists
  vector<set<int> > _successors;
};


#endif  // SRC_QUERYGRAPH_H_
