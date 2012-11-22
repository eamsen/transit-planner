// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#ifndef SRC_DIJKSTRA_H_
#define SRC_DIJKSTRA_H_

// #include <google/dense_hash_map>
#include <vector>
#include <string>
#include <functional>
#include <queue>
#include <map>
#include <set>
#include "./HubSet.h"
#include "./Label.h"
#include "./Logger.h"
#include "./QueryGraph.h"
#include "./Utilities.h"

using std::vector;
using std::string;
using std::make_pair;
using std::map;
using std::priority_queue;
using std::greater;
// using google::dense_hash_map;

class TransitNetwork;

// Stores results of a shortest path query.
class QueryResult {
 public:
  // (label, stop ids) pair depicting a path.
  typedef std::pair<LabelVec::Hnd, vector<int> > Path;
  // comparator struct
  struct PathCmp {
    bool operator() (const Path& path1, const Path& path2) const {
      return path1.first.penalty() < path2.first.penalty();
    }
  };

  // describes a path by all it's labels
  typedef pair<LabelVec::Hnd, vector<LabelVec::Hnd> > ExplicitPath;

  QueryResult();

  // The costs of the optimal paths.
  LabelVec destLabels;

  // The costs of all nodes
  LabelMatrix matrix;

  // Number of settled labels.
  size_t numSettledLabels;

  // Clears all contents.
  void clear();

  // Returns the lowest cost from the optimal paths.
  int optimalCosts() const;

  // Returns the lowest penalty from the optimal paths.
  int optimalPenalty() const;

  // Returns all optimal paths using node ids. The two latter sort the paths by
  // their penalty value.
  set<ExplicitPath> traceBackOptimalPaths() const;

  // Path elements are node ids in the transit network
  vector<Path> optimalPaths(const TransitNetwork& network,
                            string* log = NULL) const;

  // Path elements are stop ids in the query graph
  vector<Path> optimalPaths(const QueryGraph& graph, string* log = NULL) const;

  // Returns for each optimal path a vector of its transfer stops.
  // Used for debugging / printing the optimal paths.
  set<vector<int> > transferStops(const TransitNetwork& network) const;

  // Returns the transfer pattern stops for the path ending with destLabel in
  // the given transit network.
  vector<int> getTransferPattern(const TransitNetwork& network,
                                 LabelVec::Hnd destLabel) const;

 private:
  QueryResult(const QueryResult& other) {}
  QueryResult& operator=(const QueryResult& other) { return *this; }
};

// The Dijkstra class
class Dijkstra {
 public:
  typedef priority_queue<LabelMatrix::Hnd, vector<LabelMatrix::Hnd>,
                         LabelMatrix::Hnd::Comp> PriorityQueue;

  // Constructor
  explicit Dijkstra(const TransitNetwork& network);

  void findShortestPath(const vector<int>& depNodes, const int destStop,
                        QueryResult* result) const;

  // Set the logger to an external logger.
  void logger(const Logger* log);

  // Sets the maximum penalty considered during search.
  void maxPenalty(const unsigned char pen);
  // Returns the maximum penalty considered during search.
  unsigned char maxPenalty() const;

  // Sets the maximum penalty from hubs considered during search.
  void maxHubPenalty(const unsigned char pen);
  // Returns the maximum penalty from hubs considered during search.
  unsigned char maxHubPenalty() const;

  // Sets the maximum cost in seconds considered during search.
  void maxCost(const unsigned int cost);

  // Returns the maximum cost in seconds considered durchin search.
  unsigned int maxCost() const;

  // Sets the set of hubs.
  void hubs(const HubSet* hubs);

  // Returns a const pointer to the set hubs.
  const HubSet* hubs() const;

  // Sets the start time of the shortest path search on a network.
  void startTime(const int startTime);

  // Returns the set start time of the shortest path search.
  const int startTime() const;

 private:
  // Expands given node with its walkable successor nodes.
  void expandWalkNode(const LabelMatrix::Hnd& label,
                      const int destStop,
                      PriorityQueue* queue,
                      QueryResult* result,
                      int* numOpened, int* numInactive) const;

  // Expands given node using the given arc label.
  void expandNode(const LabelMatrix::Hnd& label,
                  PriorityQueue* queue,
                  QueryResult* result,
                  int* numOpened, int* numInactive) const;

  // Adds a successor label of given parent only if the resulting label is a
  // candidate for an optimal path and does not violate the maximum penalty and
  // maximum cost setting.
  void addSuccessor(const LabelMatrix::Hnd& parentLabel,
                    const unsigned int cost,
                    const unsigned char penalty, const bool walk,
                    const int succNode, PriorityQueue* queue,
                    QueryResult* result,
                    int* numOpened, int* numInactive) const;

  bool isHub(const int node) const;
  const TransitNetwork& _network;
  const Logger* _log;
  const HubSet* _hubs;
  unsigned char _maxPenalty;
  unsigned char _maxHubPenalty;
  unsigned int _maxCost;
  unsigned int _startTime;
};


class QuerySearch {
 public:
  // Constructor
  explicit QuerySearch(const QueryGraph& queryGraph,
                       const TransitNetwork& network);

  // Perform a shortest path Dijkstra search starting at time. Uses dc for
  // direct connection queries.
  void findOptimalPaths(const int startTime, const DirectConnection& dc,
                        QueryResult* resultPtr) const;
  void logger(Logger* log) { _log = log; }
 private:
  const QueryGraph& _graph;
  const TransitNetwork& _network;
  const int _maxPenalty;
  const Logger* _log;
};
#endif  // SRC_DIJKSTRA_H_
