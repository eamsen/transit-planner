// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko

#include "./Statistics.h"
#include <vector>

using std::vector;

// QueryComparei


void QueryCompare::hubs(const HubSet* hubs) {
  _hubs = hubs;
}


int QueryCompare::compare(const vector<QueryResult::Path>& resultDi,
                          const vector<QueryResult::Path>& resultTp) {
  // Invalid query
  if (resultDi.size() == 0) {
    return 0;
  }

  // Get optimal costs of Dijkstra paths
  size_t optimalCost = INT_MAX;
  for (size_t i = 0; i < resultDi.size(); i++)
    if (resultDi[i].first.cost() < optimalCost)
      optimalCost = resultDi[i].first.cost();

  // Remove obscure paths
  vector<QueryResult::Path> validDiResults;
  for (size_t i = 0; i < resultDi.size(); i++)
    if (resultDi[i].first.cost() < 2 * optimalCost)
      validDiResults.push_back(resultDi[i]);

  // Check if Di result is subset of Tp result
  // Therefor every Dijkstra path must match to at least one TP
  bool isSubset = true;
  bool isAlmostSubset = true;
  for (size_t i = 0; i < validDiResults.size(); i++) {
    bool validPath = false;
    bool almostValidPath = false;
    for (size_t j = 0; j < resultTp.size(); j++) {
      int costDiff = static_cast<int>(validDiResults[i].first.cost() / 60 + 0.5)
                   - static_cast<int>(resultTp[j].first.cost() / 60 + 0.5);
      int penaltyDiff = validDiResults[i].first.penalty()
                            - resultTp[j].first.penalty();
      if (costDiff == 0 && penaltyDiff == 0)
        validPath = true;
      if (abs(costDiff) < optimalCost / 60 * 0.2 && penaltyDiff >= -1)
        almostValidPath = true;
    }
    if (!validPath)
      isSubset = false;
    if (!almostValidPath)
      isAlmostSubset = false;
  }
  if (isSubset)
    return 1;
  if (isAlmostSubset)
    return 2;
  // check for failings that are due to the suboptimal paths in the query graph
  if (_hubs) {
    bool validTpPath = true;
    for (size_t i = 0; i < resultTp.size(); i++) {
      if (resultTp[i].first.penalty() > 3) {
        const vector<int>& path = resultTp[i].second;
        bool containsHub = false;
        for (size_t j = 0; j < path.size(); j++) {
          if (contains(*_hubs, path[j])) {
            containsHub = true;
            break;
          }
        }
        if (!containsHub)
          validTpPath = false;
      }
    }
    if (!validTpPath)
      return 4;
  }
  return 3;
}

