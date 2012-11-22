// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#ifndef SRC_STATISTICS_H_
#define SRC_STATISTICS_H_

#include <vector>
#include "./Dijkstra.h"

using std::vector;

class QueryCompare {
 public:
  // Constructor
  QueryCompare() : _hubs(NULL) {}
  // Sets the hub set used for path evaluation.
  void hubs(const HubSet* hubs);
  // Compares two set of of paths with each other. ResultDijkstra should be a
  // a subset of resultTp. Returns the type of the query compare:
  // 0: dijkstra result is empty
  // 1: dijkstra is a true subset of Tp
  // 2: dijkstra is almost a subset of Tp (paths costs vary not more than 10%)
  // 3: dijkstra is not a subset of Tp
  // 4: Tp found a path without a hub that has larger penalty than the penalty
  //    limit between some station and a hub. Caused by suboptimal query graphs.
  int compare(const vector<QueryResult::Path>& resultDi,
              const vector<QueryResult::Path>& resultTp);
 private:
  const HubSet* _hubs;
};

#endif  // SRC_STATISTICS_H_
