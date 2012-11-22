// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include "./TransferPatternRouter.h"
#include <boost/foreach.hpp>
#include <vector>
#include <string>
#include <algorithm>
#include <set>
#include <map>
#include "./Dijkstra.h"
#include "./Utilities.h"
#include "./Random.h"

using std::cout;
using std::endl;

const int TransferPatternRouter::TIME_LIMIT = 1 * 60 * 60;
const unsigned char TransferPatternRouter::PENALTY_LIMIT = 3;

TransferPatternRouter::TransferPatternRouter(const TransitNetwork& network)
    : _network(network), _tpdb(NULL), _log(&LOG) {
}


void TransferPatternRouter::prepare(const vector<Line>& lines) {
  _connections.init(_network.numStops(), lines);
  _timeCompressedNetwork = _network.createTimeCompressedNetwork();
}


void TransferPatternRouter::computeAllTransferPatterns(TPDB* tpdb) {
  assert(tpdb);
  if (tpdb->numGraphs() == 0) {
    HubSet dummyHubs;
    tpdb->init(_network.numStops(), dummyHubs);
  }
  const int numStops = _network.numStops();
  for (int stop = 0; stop < numStops; ++stop) {
    set<vector<int> > pattern = computeTransferPatterns(_network, stop, _hubs);
    for (auto it = pattern.cbegin(); it != pattern.cend(); ++it)
      tpdb->addPattern(*it);
  }
}


set<vector<int> >
TransferPatternRouter::computeTransferPatterns(const TransitNetwork& network,
                                               const int depStop,
                                               const HubSet& hubs) {
  set<vector<int> > patterns;
  if (hubs.size() && !contains(hubs, depStop)) {
    patterns = computeTransferPatternsToHubs(network, depStop, hubs);
  } else {
    patterns = computeTransferPatternsToAll(network, depStop, hubs);
  }
  return patterns;
}


set<vector<int> >
TransferPatternRouter::computeTransferPatternsToHubs(
    const TransitNetwork& network,
    const int depStop,
    const HubSet& hubs) {
  const vector<int> depNodes = network.getDepNodes(depStop);
  Dijkstra dijkstra(network);
  dijkstra.maxPenalty(PENALTY_LIMIT);
  dijkstra.maxHubPenalty(0);
  dijkstra.hubs(&hubs);
  QueryResult result;
  dijkstra.findShortestPath(depNodes, INT_MAX, &result);

  // Remove inactive labels and collect the stops for which at least one node is
  // settled.
  result.matrix.pruneInactive();
  set<int> settledStops;
  for (auto it = result.matrix.cbegin(), end = result.matrix.cend();
       it != end; ++it) {
    const LabelVec& labels = *it;
    if (labels.size()) {
      const int node = labels.at();
      const int stop = network.node(node).stop();
      settledStops.insert(stop);
    }
  }
  // For all stops which have been reached remove suboptimal labels using the
  // arrival loop algorithm and trace back the paths.
  adjustWalkingCosts(network, &result.matrix);
  for (auto it = settledStops.cbegin(), end = settledStops.cend();
       it != end; ++it) {
    const int stop = *it;
    arrivalLoop(network, &result.matrix, stop);
  }
  return collectTransferPatterns(network, result.matrix, depStop);
}


set<vector<int> >
TransferPatternRouter::computeTransferPatternsToAll(
    const TransitNetwork& network,
    const int depStop,
    const HubSet& hubs) {
  // Do a full dijkstra from the set of nodes of the departure stop
  vector<int> depNodes = network.getDepNodes(depStop);
  Dijkstra dijkstra(network);
  dijkstra.maxPenalty(PENALTY_LIMIT);
  dijkstra.hubs(&hubs);
  dijkstra.maxHubPenalty(0);
  QueryResult result;
  dijkstra.findShortestPath(depNodes, INT_MAX, &result);

  adjustWalkingCosts(network, &result.matrix);
  // Get transfer Patterns to all stops in the network:
  const int numStops = network.numStops();
  for (int targetStop = 0; targetStop < numStops; ++targetStop) {
    if (depStop == targetStop) {
      continue;
    }
    arrivalLoop(network, &result.matrix, targetStop);
  }
  return collectTransferPatterns(network, result.matrix, depStop);
}


set<vector<int> >
TransferPatternRouter::generateTransferPatterns(const int orig, const int dest)
const {
  assert(_tpdb);
  QueryGraph queryGraph(_tpdb->graph(orig), dest);
  vector<vector<int> > patterns = queryGraph.generateTransferPatterns();
  return set<vector<int> >(patterns.begin(), patterns.end());
}


set<vector<int> >
TransferPatternRouter::generateAllTransferPatterns(const int orig) const {
  set<vector<int> > allPatterns;
  const int numNodes = _network.numNodes();
  for (int i = 0; i < numNodes; ++i) {
    const set<vector<int> > queryPatterns = generateTransferPatterns(orig, i);
    for (auto it = queryPatterns.cbegin(); it != queryPatterns.cend(); ++it)
      allPatterns.insert(*it);
  }
  return allPatterns;
}


void TransferPatternRouter::adjustWalkingCosts(const TransitNetwork& network,
                                               LabelMatrix* matrix) {
  for (int j = 0; j < matrix->size(); j++) {
    if (network.node(j).type() == Node::TRANSFER ||
        network.node(j).type() == Node::DEPARTURE) {
      for (auto it = matrix->at(j).begin(); it != matrix->at(j).end(); it++) {
        if (it->walk()) {
          assert(it->at() == j);
          const Node& node = network.node(j);
          const Node& parentNode = network.node((matrix->parent(*it)).at());
          int stopIndex = node.stop();
          int parentStopIndex = parentNode.stop();
          // Get matching arc between stop and parent stop:
          int arcCost = INT_MAX;
          const vector<Arc>& walkArcs = network.walkwayList(parentStopIndex);
          for (size_t i = 0; i < walkArcs.size(); i++)
            if (walkArcs[i].destination() == stopIndex)
              arcCost = walkArcs[i].cost();
          assert(arcCost != INT_MAX);

          // Adjust walking costs:
          int cost = it->cost() - (node.time() - parentNode.time()) + arcCost;
          matrix->add(it->at(), cost, it->penalty(), it->maxPenalty(),
                      it->walk(),
                      it->inactive(), matrix->parent(*it));
        }
      }
    }
  }
}


void TransferPatternRouter::arrivalLoop(const TransitNetwork& network,
                                        LabelMatrix* matrix,
                                        const int stop) {
  const vector<int>& stopNodes = network.stop(stop).getNodeIndices();
  const int numStopNodes = stopNodes.size();
  // collect arrival nodes and transfer nodes with walk label
  vector<int> stopArrivalNodes;
  stopArrivalNodes.reserve(numStopNodes / 3 * 2);

  for (int i = 0; i < numStopNodes; i++) {
    if (network.node(stopNodes[i]).type() == Node::ARRIVAL) {
      stopArrivalNodes.push_back(stopNodes[i]);
    } else {  // for TRANSFER and DEPARTURE
      bool containsWalkNode = false;
      for (auto it = matrix->at(stopNodes[i]).begin();
           it != matrix->at(stopNodes[i]).end();
           it++) {
        if (it->walk())
          containsWalkNode = true;
      }
      if (containsWalkNode)
        stopArrivalNodes.push_back(stopNodes[i]);
    }
  }
  // create a local copy of the nodes' label vectors
  const int numStopArrivalNodes = stopArrivalNodes.size();
  vector<LabelVec> tmpMatrix;
  tmpMatrix.reserve(numStopArrivalNodes);
  for (auto node = stopArrivalNodes.cbegin(), end = stopArrivalNodes.cend();
       node != end; ++node) {
    if (network.node(*node).type() == Node::ARRIVAL) {
      tmpMatrix.push_back(matrix->at(*node));
    } else {  // for TRANSFER and DEPARTURE
      LabelVec tmpVector(*node, 12/*maxPenalty*/);
      for (auto label = matrix->at(*node).begin();
           label != matrix->at(*node).end();
           label++) {
        if (label->walk()) {
          tmpVector.add(*label, matrix->parent(*label));
        }
      }
      assert(tmpVector.size() > 0);
      tmpMatrix.push_back(tmpVector);
    }
  }
  assert(tmpMatrix.size() == stopArrivalNodes.size());

  // arrival loop algorithm
  if (numStopArrivalNodes) {
    for (int i = 0; i < numStopArrivalNodes - 1; ++i) {
      const int currNode = stopArrivalNodes[i];
      const int nextNode = stopArrivalNodes[i+1];
      const int timeDiff = network.node(nextNode).time() -
                           network.node(currNode).time();
      assert(timeDiff >= 0);

      const LabelVec& nodeLabels = tmpMatrix[i];
      for (auto labelIt = nodeLabels.begin(), end2 = nodeLabels.end();
           labelIt != end2; ++labelIt) {
        const LabelVec::Hnd& label = *labelIt;
        const int alternativeCost = label.cost() + timeDiff;

        // Deactivate all labels at nextNode which are strictly worse than the
        // current label of currNode plus the time difference.
        if (matrix->candidate(nextNode, alternativeCost, label.penalty())) {
          tmpMatrix[i+1].add(alternativeCost, label.penalty(),
                             label.maxPenalty(),
                             false, false, NULL);
          // deactivate the corresponding labels in the original vector
          matrix->deactivate(nextNode, alternativeCost, label.penalty());
        }
      }
    }
  }
}


set<vector<int> >
TransferPatternRouter::collectTransferPatterns(const TransitNetwork& network,
                                               const LabelMatrix& matrix,
                                               const int depStop) {
  set<vector<int> > patterns;
  for (auto it = matrix.cbegin(), end = matrix.cend(); it != end; ++it) {
    const LabelVec& labels = *it;
    for (auto it2 = labels.begin(), end2 = labels.end(); it2 != end2; ++it2) {
      vector<int> pattern;
      LabelVec::Hnd label = *it2;
      const Node& firstNode = network.node(label.at());
      const Node::Type nodeType = firstNode.type();
      if (!label.inactive()
          && (nodeType == Node::ARRIVAL ||
              (label.walk() && nodeType == Node::TRANSFER) ||
              (label.walk() && nodeType == Node::DEPARTURE))
          && firstNode.stop() != depStop) {
        assert(label);
        LabelVec::Hnd parent = matrix.parent(label);
        assert(parent);
        pattern.push_back(firstNode.stop());
        while (label && parent && label.penalty() > 0) {
          const int stop = network.node(label.at()).stop();
          if (label.penalty() > parent.penalty()) {
            if (pattern.back() != stop) {
              pattern.push_back(stop);
            }
            const int parentStop = network.node(parent.at()).stop();
            if (label.walk() && pattern.back() != parentStop) {
              pattern.push_back(parentStop);
            }
          }
          label = parent;
          parent = matrix.parent(label);
        }
        if (pattern.back() != depStop) {
          pattern.push_back(depStop);
        }
        reverse(pattern.begin(), pattern.end());
        assert(pattern[0] == depStop);
        patterns.insert(pattern);
      }
    }
  }
  return patterns;
}


const QueryGraph TransferPatternRouter::queryGraph(int depStop, int destStop)
const {
  assert(_tpdb);
  // Get the transfer pattern graph and construct the query graph from it.
  const TPG& tpgDepStop = _tpdb->graph(depStop);
  QueryGraph queryGraph(tpgDepStop, destStop);
  // Expand the query graph by the query graphs between departure, destination
  // and hubs.
  const set<int>& depStopHubs = tpgDepStop.destHubs();
  for (auto it = depStopHubs.cbegin(), end = depStopHubs.cend();
       it != end; ++it) {
    const int hub = *it;
    if (_tpdb->graph(depStop).destNode(hub) != TPG::INVALID_NODE) {
      queryGraph.merge(_tpdb->graph(depStop), hub);
      queryGraph.merge(_tpdb->graph(hub), destStop);
    }
  }
  return queryGraph;
}


vector<QueryResult::Path>
TransferPatternRouter::shortestPath(const int depStop, const int time,
                                    const int destStop, string* log) {
  QueryGraph graph = queryGraph(depStop, destStop);
  // Search for the optimal paths.
  QuerySearch querySearch(graph, _network);
  QueryResult result;
  querySearch.findOptimalPaths(time, _connections, &result);
  // Backtrack each optimal path in the QueryGraph and translate them into
  // sequences of stops in the TransitNetwork, sort paths by cost and penalty.
  vector<QueryResult::Path> optimalPaths = result.optimalPaths(graph, log);
  return optimalPaths;
}


void TransferPatternRouter::countStopFreq(const int seedStop,
                                          vector<IntPair>* stopFreqs) const {
  assert(stopFreqs);
  assert(stopFreqs->size() == static_cast<size_t>(_network.numStops()));
  // Do a full time limited Dijkstra from the current seed stop
  Dijkstra dijkstra(_timeCompressedNetwork);
  // dijkstra.logger(_log);
  // dijkstra.maxCost(TIME_LIMIT);
  // dijkstra.maxPenalty(PENALTY_LIMIT);
  QueryResult result;
  dijkstra.findShortestPath({seedStop}, INT_MAX, &result);
  // Traceback all labels to the departure Stop and count +1 for the stop of
  // every visited node.
  for (auto it = result.matrix.cbegin(), end = result.matrix.cend();
       it != end; ++it) {
    const LabelVec& labels = *it;
    for (auto it2 = labels.begin(), end2 = labels.end(); it2 != end2; ++it2) {
      LabelVec::Hnd label = *it2;
      while (label) {
        // in the compressed network we have stop == node
        const int stopIndex = label.at();
        stopFreqs->at(stopIndex).second++;
        label = result.matrix.parent(label);
      }
    }
  }
}


set<int> TransferPatternRouter::findBasicHubs(int numHubs) {
  const int numStops = _network.numStops();
  vector<pair<int, int> > stops;
  stops.reserve(numStops);
  for (int i = 0; i < numStops; ++i) {
    stops.push_back(make_pair(i, _network.stop(i).numNodes()));
  }

  std::sort(stops.begin(), stops.end(), sortStopsByImportance());

  set<int> hubs;
  for (int i = 0; i < numHubs; ++i) {
    hubs.insert(stops[i].first);
  }
  _log->info("%d stops with the most nodes have been choosen as hubs.",
             hubs.size());
  return hubs;
}


string TransferPatternRouter::printPatterns(const set<vector<int> >& patterns) {
  stringstream patternStream;
  for (auto it = patterns.cbegin(); it != patterns.cend(); it++) {
    vector<int> pattern = *it;
    patternStream << "[";
    for (size_t i = 0; i < pattern.size()-1; i++) {
      patternStream << pattern[i] << ", ";
    }
    patternStream << pattern[pattern.size()-1] << "]\n";
  }
  return patternStream.str();
}


void TransferPatternRouter::logger(Logger* log) {
  if (log) {
    _log = log;
  }
}


const HubSet& TransferPatternRouter::hubs() const {
  return _hubs;
}


void TransferPatternRouter::hubs(const HubSet& hubs) {
  _hubs = hubs;
}


const TPDB* TransferPatternRouter::transferPatternsDB() const {
  return _tpdb;
}


void TransferPatternRouter::transferPatternsDB(const TPDB& db) {
  _tpdb = &db;
}

const DirectConnection& TransferPatternRouter::directConnection() {
  return _connections;
}
