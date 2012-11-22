// Copyright 2012: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include "./QueryGraph.h"
#include <boost/foreach.hpp>
#include <assert.h>
#include <algorithm>
#include <map>
#include <queue>
#include <functional>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <vector>
#include "./Label.h"
#include "./TransferPatternsDB.h"
#include "./Utilities.h"

using std::cout;
using std::endl;

const int QueryGraph::INVALID_NODE = -1;


QueryGraph::QueryGraph() {}


QueryGraph::QueryGraph(const TPG& tpgOrigin, const int destStop) {
  const int origStop = tpgOrigin.depStop();
  const int destNode = tpgOrigin.destNode(destStop);

  if (destNode == TPG::INVALID_NODE) {
    // if the source B node is not in the TPG, the QueryGraph(A,B) is empty
    _stops.push_back(origStop);
    _successors.push_back(set<int>());
    _nodeIndex[origStop] = sourceNode();

    _stops.push_back(destStop);
    _successors.push_back(set<int>());
    _nodeIndex[destStop] = targetNode();
  } else {
    // merge the empty query graph with the one
    merge(tpgOrigin, destStop);
    assert(_stops.size() == _successors.size());
  }
}


const set<int>& QueryGraph::successors(const int node) const {
  assert(node >= 0);
  assert(node < static_cast<int>(_stops.size()));
  return _successors[node];
}


int QueryGraph::stopIndex(const int node) const {
  //   printf("_stops.size()=%d node=%d\n", _stops.size(), node);
  assert(node >= 0);
  assert(node < static_cast<int>(_stops.size()));
  return _stops[node];
}


int QueryGraph::nodeIndex(const int stop) const {
//   int index = INVALID_NODE;
  auto it = _nodeIndex.find(stop);
  int index = (it != _nodeIndex.end()) ? it->second : INVALID_NODE;
  /*for (size_t i = 0; i < _stops.size(); i++) {
    if (_stops[i] == stop) {
      index = i;
      break;
    }
  }*/
  return index;
}


const int QueryGraph::targetNode() const {
  if (_stops.size() > 1)
    return 1;
  else
    return INVALID_NODE;
}


bool QueryGraph::empty() const {
  return size() < 2 || _successors[sourceNode()].size() == 0;
}


void QueryGraph::merge(const TPG& tpgOrigin, const int destStop) {
  const int origNode = tpgOrigin.depStop();
  const int origStop = origNode;
  const int destNode = tpgOrigin.destNode(destStop);

  if (destNode == TPG::INVALID_NODE) {
    return;
  }

  vector<int> tpgNodes;
  tpgNodes.push_back(origNode);
  tpgNodes.push_back(destNode);
  vector<int> nodeStopIndices;  // remember the corresponding index in _stops

  int indexOrig = addStop(origStop, set<int>());
  nodeStopIndices.push_back(indexOrig);
  int indexDest = addStop(destStop, set<int>());
  nodeStopIndices.push_back(indexDest);

  map<int, int> qgStopsMapped;
  qgStopsMapped[origStop] = indexOrig;
  qgStopsMapped[destStop] = indexDest;

  // "Assume node B has successor nodes Ci. For each successor, add arcs (Ci, B)
  // to the QueryGraph."
  for (uint i = 1; i < tpgNodes.size(); i++) {
    const int qgIndexOfB = nodeStopIndices[i];
    const vector<int>& tpgSuccessorsOfB = tpgOrigin.successors(tpgNodes[i]);
    for (uint j = 0; j < tpgSuccessorsOfB.size(); j++) {
      int tpgSuccessorNode = tpgSuccessorsOfB[j];
      int qgIndexOfCi = addStop(tpgOrigin.stop(tpgSuccessorNode), {qgIndexOfB});

      tpgNodes.push_back(tpgSuccessorNode);
      nodeStopIndices.push_back(qgIndexOfCi);
    }
  }
  assert(_stops.size() == _successors.size());
}


int QueryGraph::addStop(const int stop, const set<int>& successors) {
  int node = nodeIndex(stop);
  if (node == INVALID_NODE) {
    // stop is not in the query graph
    assert(_stops.size() == _successors.size());
    _stops.push_back(stop);
    _successors.push_back(successors);
    node = _stops.size() - 1;
    _nodeIndex[stop] = node;
  } else {
    // stop is already in the query graph
    for (auto it = successors.begin(); it != successors.end(); it++) {
      assert(*it < static_cast<int>(_stops.size()));
      _successors[node].insert(*it);
    }
  }
  return node;
}


bool QueryGraph::containsPattern(const vector<int>& stops) const {
  if (empty() || stops.size() < 2 || _stops[sourceNode()] != stops[0] ||
    _stops[targetNode()] != stops.back())
    return false;
  int curr = nodeIndex(stops[0]), next;
  if (curr == INVALID_NODE)
    return false;
  for (size_t i = 0; i < stops.size() - 1; ++i) {
    next = nodeIndex(stops[i+1]);
    if (next == INVALID_NODE || !contains(_successors[curr], next))
      return false;
    curr = next;
  }
  return true;
}


const QueryGraph QueryGraph::findPattern(const vector<int>& stops) const {
  QueryGraph graph;
  if (!empty() && stops.size() > 1 && _stops[sourceNode()] == stops[0] &&
      _stops[targetNode()] == stops.back()) {
    graph._stops.push_back(_stops[sourceNode()]);
    graph._successors.push_back(set<int>());
    graph._nodeIndex[_stops[sourceNode()]] = 0;
    graph._stops.push_back(_stops[targetNode()]);
    graph._successors.push_back(set<int>());
    graph._nodeIndex[_stops[targetNode()]] = 1;
    int curr = nodeIndex(stops[0]), next;
    /*if (curr == INVALID_NODE)
      return QueryGraph();*/
    for (size_t i = 0; i < stops.size() - 1; ++i) {
      next = nodeIndex(stops[i+1]);
      if (next != INVALID_NODE && contains(_successors[curr], next)) {
        int nextInNew = graph.addStop(stops[i+1], set<int>());
        const set<int> successors = {nextInNew};
        graph.addStop(stops[i], successors);
      } else  {
        return QueryGraph();
      }
      curr = next;
    }
  }
  return graph;
}


string QueryGraph::debugString() const {
  std::stringstream ss;
  for (uint i = 0; i < _stops.size(); i++) {
    ss << _stops[i] << ":{";
    BOOST_FOREACH(int successor, successors(i)) {
      ss << _stops[successor] << ",";
    }
    ss << "}\n";
  }
  return ss.str();
}


void QueryGraph::recurse(const int node, vector<int> stops,
                         vector<vector<int> >* patterns) const {
  stops.push_back(_stops[node]);
  if (node == targetNode()) {
    patterns->push_back(stops);
  } else {
    set<int>::const_iterator it;
    for (it = _successors[node].begin(); it != _successors[node].end(); ++it) {
      recurse(*it, stops, patterns);
    }
  }
}


const size_t QueryGraph::countArcs() const {
  size_t numArcs = 0;
  for (size_t i = 0; i < _successors.size(); i++)
    numArcs += _successors[i].size();
  return numArcs;
}


const vector<vector<int> > QueryGraph::generateTransferPatternsRecursive()
const {
  vector<vector<int> > patterns;
  vector<int> stops;
  recurse(sourceNode(), stops, &patterns);
  return patterns;
}


const vector<vector<int> > QueryGraph::generateTransferPatterns() const {
  // perfom iterative DFS
  vector<vector<int> > queryGraphPaths;
  vector<int> path = {sourceNode()};
  std::queue<vector<int> > tmpPaths;
  tmpPaths.push(path);
  while (tmpPaths.size()) {
    path = tmpPaths.front();
    tmpPaths.pop();
    const set<int>& succs = successors(path.back());
    for (auto succIt = succs.cbegin(); succIt != succs.cend(); ++succIt) {
      vector<int> extendedPath = path;
      extendedPath.push_back(*succIt);
      if (*succIt == targetNode()) {
        queryGraphPaths.push_back(extendedPath);
      } else {
        tmpPaths.push(extendedPath);
      }
    }
  }

  // translate paths into pattern of node ids
  vector<vector<int> > patterns;
  for (auto it = queryGraphPaths.cbegin(); it != queryGraphPaths.cend(); ++it) {
    vector<int> translatedPath;
    for (auto node = it->cbegin(); node != it->cend(); ++node) {
      translatedPath.push_back(stopIndex(*node));
    }
    patterns.push_back(translatedPath);
  }
  return patterns;
}

