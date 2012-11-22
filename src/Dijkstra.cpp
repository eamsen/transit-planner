// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include "./Dijkstra.h"
#include <cassert>
#include <cmath>
#include <vector>
#include <string>
#include <queue>
#include <limits>
#include <utility>
#include <functional>
#include <algorithm>
#include <set>
#include <map>
#include "./DirectConnection.h"
#include "./TransitNetwork.h"
#include "./Utilities.h"

using std::min;
using std::max;
using std::cout;
using std::endl;
using std::pair;
using std::priority_queue;
using std::set;
using std::make_pair;
using std::greater;

// QueryResult

QueryResult::QueryResult() {
  clear();
}

void QueryResult::clear() {
  numSettledLabels = 0;
}

int QueryResult::optimalCosts() const {
  return destLabels.minCost();
}

int QueryResult::optimalPenalty() const {
  return destLabels.minPenalty();
}


set<vector<int>>
QueryResult::transferStops(const TransitNetwork& network) const {
  set<vector<int>> transferStops;
  for (auto it = destLabels.begin(), end = destLabels.end();
       it != end; ++it) {
    transferStops.insert(getTransferPattern(network, *it));
  }
  return transferStops;
}

vector<int>
QueryResult::getTransferPattern(const TransitNetwork& network,
                                LabelVec::Hnd destLabel) const {
  LabelVec::Hnd label = destLabel;
  int destStop = network.node(label.at()).stop();
  LabelVec::Hnd parent = matrix.parent(label);
  vector<int> transfers;
  transfers.push_back(destStop);
  while (label && parent) {
    if ((label.penalty() > parent.penalty())
        && transfers.back() != network.node(label.at()).stop())
      transfers.push_back(network.node(label.at()).stop());
    if (label.walk())
      transfers.push_back(network.node(parent.at()).stop());
    label = parent;
    parent = matrix.parent(parent);
  }
  // Add parent stop in case there is a direct connection from dep to dest.
  if (parent)
    transfers.push_back(network.node(parent.at()).stop());
  else
    transfers.push_back(network.node(label.at()).stop());
  std::reverse(transfers.begin(), transfers.end());
  return transfers;
}

set<QueryResult::ExplicitPath>
QueryResult::traceBackOptimalPaths() const {
  set<ExplicitPath> paths;
  for (auto it = destLabels.begin(), end = destLabels.end();
       it != end; ++it) {
    LabelVec::Hnd label = *it;
    vector<LabelVec::Hnd> path;
    while (label.field() != NULL) {
      path.push_back(label);
      label = matrix.parent(label);
    }
    std::reverse(path.begin(), path.end());
    paths.insert(ExplicitPath(*it, path));
  }
  return paths;
}


vector<QueryResult::Path>
QueryResult::optimalPaths(const TransitNetwork& network, string* log) const {
  vector<Path> finalPaths;

  const set<ExplicitPath> paths = traceBackOptimalPaths();

  for (auto it = paths.begin(), end = paths.end(); it != end; ++it) {
    vector<int> path;
    path.reserve(it->second.size());
    for (auto it2 = it->second.begin(), end2 = it->second.end();
         it2 != end2; ++it2) {
      path.push_back(network.node(it2->at()).stop());
    }
    finalPaths.push_back(Path(it->first, path));
  }
  std::sort(finalPaths.begin(), finalPaths.end(), PathCmp());

  if (log) {
    stringstream ss;
    for (auto it = paths.begin(); it != paths.end(); ++it) {
      const vector<LabelVec::Hnd>& path = it->second;
      for (uint i = 0; i < path.size(); i++) {
        if (i == 0 || i == path.size()-1 || i == path.size()-2 ||
            (i < path.size()-2 && path[i].penalty() < path[i+1].penalty())
            || path[i].walk()) {
          ss << "[" << network.node(path[i].at()).stop()
             << "," << path[i].cost()
             << "," << static_cast<int>(path[i].penalty())
             << "," << path[i].walk();
          const int type = network.node(path[i].at()).type();
          if (type == Node::ARRIVAL)
            ss << ",A]\n";
          else if (type == Node::TRANSFER)
            ss << ",T]\n";
          else if (type == Node::DEPARTURE)
            ss << ",D]\n";
          else
            ss << ",N]\n";
        }
      }
      ss << "\n";
    }
    *log = ss.str();
  }
  return finalPaths;
}


vector<QueryResult::Path>
QueryResult::optimalPaths(const QueryGraph& graph, string* log) const {
  // Translate each path in the query graph into a path of stops in the transit
  // network.
  vector<Path> finalPaths;
  set<ExplicitPath> paths = traceBackOptimalPaths();

  for (auto path = paths.begin(), end = paths.end(); path != end; ++path) {
    vector<int> finalPath;
    finalPath.reserve(path->second.size());
    for (uint i = 0; i < path->second.size(); i++) {
      finalPath.push_back(graph.stopIndex(path->second[i].at()));
    }
    finalPaths.push_back(Path(path->first, finalPath));
  }
  std::sort(finalPaths.begin(), finalPaths.end(), PathCmp());

  if (log) {
    stringstream ss;
    for (auto it = paths.begin(); it != paths.end(); it++) {
      vector<LabelVec::Hnd> path = it->second;
      for (uint i = 0; i < path.size(); i++) {
        ss << "[" << graph.stopIndex(path[i].at())
           << "," << path[i].cost()
           << "," << static_cast<int>(path[i].penalty())
           << "," << path[i].walk() << "]\n";
      }
      ss << "\n";
    }
    *log += ss.str();
  }

  return finalPaths;
}


// Dijkstra

Dijkstra::Dijkstra(const TransitNetwork& network)
  : _network(network), _log(&LOG), _hubs(NULL),
    _maxPenalty(3), _maxHubPenalty(3), _maxCost(INT_MAX), _startTime(0) {}


void Dijkstra::logger(const Logger* log) {
  assert(log);
  _log = log;
}


void Dijkstra::findShortestPath(const vector<int>& depNodes,
                                const int destStop,
                                QueryResult* resultPtr) const {
  QueryResult& result = *resultPtr;
  result.clear();
  result.destLabels = LabelVec(destStop, _maxPenalty + _maxHubPenalty);
  result.matrix.resize(_network.numNodes(), _maxPenalty + _maxHubPenalty);
  if (depNodes.empty()) {
    return;
  }
  PriorityQueue queue;
  // init the queue with departure nodes
  int numOpened = 0;
  int numInactive = 0;
  for (auto it = depNodes.begin(), end = depNodes.end(); it != end; ++it) {
    const int node = *it;
    assert(result.matrix.candidate(node, 0, 0));
    LabelMatrix::Hnd label;
    if (_startTime) {
      int waitTime = _network.node(node).time() - _startTime;
      assert(waitTime >= 0);
      label = result.matrix.add(node, waitTime, 0, _maxPenalty);
    } else {
      label = result.matrix.add(node, 0, 0, _maxPenalty);
    }
    ++numOpened;
    queue.push(label);
    assert(label.at() == node);
    // If a full Dijkstra is started from a hub, expand walking arcs for the
    // departure nodes
    const int depStop = _network.node(label.at()).stop();
    if (destStop == INT_MAX && _hubs && contains(*_hubs, depStop)) {
      expandWalkNode(label, destStop, &queue, &result, &numOpened,
                     &numInactive);
    }
  }
  while ((destStop != INT_MAX && queue.size()) ||
         static_cast<int>(queue.size()) > numInactive) {
    ++result.numSettledLabels;
    LabelMatrix::Hnd label = queue.top();
    queue.pop();
    numInactive -= label.inactive();
    assert(numInactive >= 0);
    if (!label.closed()) {
      --numOpened;
      assert(numOpened >= 0);
      const int node = label.at();
      const int stop = _network.node(node).stop();

      // debug
//       printf("Settling node at stop %d with (%d, %d), maxPenalty %d\n", stop,
//              label.cost(), label.penalty(), label.maxPenalty());

      label.closed(true);
      if (stop == destStop) {
        // assert(result.destLabels.candidate(label.cost(), label.penalty()));
        if (result.destLabels.candidate(label.cost(), label.penalty())) {
          const LabelVec::Hnd parent = result.matrix.parent(label);
          result.destLabels.add(label, parent);
        }
      } else {
        expandNode(label, &queue, &result, &numOpened, &numInactive);
        if (_network.node(node).type() == Node::ARRIVAL) {
          expandWalkNode(label, destStop, &queue, &result,
                         &numOpened, &numInactive);
        }
      }
    }
  }
  assert(static_cast<int>(queue.size()) == numInactive);
}

inline
void Dijkstra::expandNode(const LabelMatrix::Hnd& label,
                         PriorityQueue* queue,
                         QueryResult* result,
                         int* numOpened, int* numInactive) const {
  const vector<Arc>& adj = _network.adjacencyList(label.at());
  for (auto it = adj.begin(), end = adj.end(); it != end; ++it) {
    const Arc& arc = *it;
    addSuccessor(label, arc.cost(), arc.penalty(), false,
                 arc.destination(), queue, result,
                 numOpened, numInactive);
  }
}

inline
void Dijkstra::expandWalkNode(const LabelMatrix::Hnd& label,
                             const int destStop,
                             PriorityQueue* queue,
                             QueryResult* result,
                             int* numOpened, int* numInactive) const {
  const int node = label.at();
  const int stop = _network.node(node).stop();
  const vector<Arc>& walkArcs = _network.walkwayList(stop);
  for (auto arc = walkArcs.begin(), end = walkArcs.end(); arc != end; ++arc) {
    const int walkStopIndex = arc->destination();
    assert(stop != walkStopIndex);
    const Stop& walkStop = _network.stop(walkStopIndex);
    int time = _network.node(node).time();
    time += arc->cost() + TransitNetwork::TRANSFER_BUFFER;
    // For the target do not consider the transfer buffer for the start node seq
    if (walkStopIndex == destStop) {
      time -= TransitNetwork::TRANSFER_BUFFER;
    }

    vector<int> nodes = _network.findStartNodeSequence(walkStop, time);
    for (auto it2 = nodes.begin(), end = nodes.end(); it2 != end; ++it2) {
      const int walkNode = *it2;
      unsigned int cost = _network.node(walkNode).time() -
                          _network.node(node).time();
      // When reaching the target, use the actual time of travel
      if (walkStopIndex == destStop) { cost = arc->cost(); }
      const unsigned char penalty = arc->penalty();
      addSuccessor(label, cost, penalty, true, walkNode, queue, result,
                   numOpened, numInactive);
    }
  }
}

inline
void Dijkstra::addSuccessor(const LabelMatrix::Hnd& parentLabel,
                           const unsigned int arcCost,
                           const unsigned char arcPenalty,
                           const bool walk,
                           const int succNode,
                           PriorityQueue* queue,
                           QueryResult* result,
                           int* numOpened, int* numInactive) const {
  const uint32_t cost = parentLabel.cost() + arcCost;
  const uint8_t penalty = parentLabel.penalty() + arcPenalty;
  uint8_t maxPenalty = parentLabel.maxPenalty();

  if (penalty <= maxPenalty &&
      cost <= _maxCost &&
      result->destLabels.candidate(cost, penalty) &&
      result->matrix.candidate(succNode, cost, penalty)) {
    const Node::Type succType = _network.node(succNode).type();
    const bool hub = isHub(parentLabel.at());
    const bool succHub = isHub(succNode);
    const bool nowInactive = (hub || (succHub && walk)) &&
                             (walk || succType == Node::TRANSFER);
    if (nowInactive && !parentLabel.inactive()) {
      maxPenalty = max(maxPenalty,
                       static_cast<uint8_t>(penalty + _maxHubPenalty));
    }
    const bool inactive = parentLabel.inactive() || nowInactive;
    const bool oldContained = result->matrix.contains(succNode, penalty);
    const bool oldClosed = result->matrix.closed(succNode, penalty);
    LabelMatrix::Hnd label = result->matrix.add(succNode, cost, penalty,
                                                maxPenalty, walk,
                                                inactive, parentLabel);
    assert(!label.closed());
    queue->push(label);
    *numOpened += !oldContained || oldClosed;
    *numInactive += inactive;
  }
}

inline
bool Dijkstra::isHub(const int node) const {
  const int stop = _network.node(node).stop();
  return _hubs != NULL && contains(*_hubs, stop);
}

// walk or transfer at a hub

void Dijkstra::maxPenalty(const unsigned char pen) {
  _maxPenalty = pen;
}

void Dijkstra::maxHubPenalty(const unsigned char pen) {
  _maxHubPenalty = pen;
}

unsigned char Dijkstra::maxPenalty() const {
  return _maxPenalty;
}

unsigned char Dijkstra::maxHubPenalty() const {
  return _maxHubPenalty;
}

void Dijkstra::maxCost(const unsigned int cost) {
  _maxCost = cost;
}

inline
unsigned int Dijkstra::maxCost() const {
  return _maxCost;
}

void Dijkstra::hubs(const HubSet* hubs) {
  _hubs = hubs;
}

inline
const HubSet* Dijkstra::hubs() const {
  return _hubs;
}

void Dijkstra::startTime(const int startTime) {
  _startTime = startTime;
}

inline
const int Dijkstra::startTime() const {
  return _startTime;
}

// QuerySearch

QuerySearch::QuerySearch(const QueryGraph& graph, const TransitNetwork& network)
    : _graph(graph), _network(network), _maxPenalty(6),
      _log(&LOG) {}

void
QuerySearch::findOptimalPaths(const int startTime, const DirectConnection& dc,
                              QueryResult* resultPtr) const {
  QueryResult& result = *resultPtr;
  result.clear();
  result.destLabels = LabelVec(_graph.targetNode(), _maxPenalty);
  result.matrix.resize(_graph.size(), _maxPenalty);

  Dijkstra::PriorityQueue queue;
  LabelMatrix::Hnd label = result.matrix.add(_graph.sourceNode(), 0, 0,
                                             _maxPenalty);
  // Handle reflexive queries.
  if (_graph.stopIndex(_graph.sourceNode()) ==
      _graph.stopIndex(_graph.targetNode())) {
    label.closed(true);
    result.destLabels.add(label, result.matrix.parent(label));
    return;
  }
  assert(label);

  // First expansion of stops: It's not possible to walk unless from a hub.
  const set<int>& succs = _graph.successors(_graph.sourceNode());
  for (auto it = succs.begin(), end = succs.end(); it != end; ++it) {
    int succNode = *it;
    int succTime = dc.query(_graph.stopIndex(_graph.sourceNode()), startTime,
                            _graph.stopIndex(succNode));

    if (succTime != DirectConnection::INFINITE) {
      LabelMatrix::Hnd succLabel;
      succLabel = result.matrix.add(succNode, succTime, 0, _maxPenalty,
                                    false, false, label);
      queue.push(succLabel);
    }
  }

  while (!queue.empty()) {
    LabelMatrix::Hnd label = queue.top();
    queue.pop();
    const int node = label.at();
    const int stop = _graph.stopIndex(node);
    int time = label.cost();
    int penalty = label.penalty();

    if (!label.closed()) {
      label.closed(true);
      if (node == _graph.targetNode() &&
          result.destLabels.candidate(label.cost(), label.penalty())) {
        // assert(!label.field()->parent || label.field()->parent->used());

        const LabelVec::Hnd parent = result.matrix.parent(label);
        result.destLabels.add(label, parent);
      } else {
        const set<int>& succs = _graph.successors(node);
        for (auto it = succs.begin(), end = succs.end(); it != end; ++it) {
          int succNode = *it;
          int succStop = _graph.stopIndex(succNode);
          assert(succStop != stop);

          // Get the Direct Connection time for the node to expand
          int queryTime = startTime + time;
          if (!label.walk()) { queryTime += TransitNetwork::TRANSFER_BUFFER; }
          int travelTime = dc.query(stop, queryTime, succStop);
          int succTime = time + travelTime;
          if (!label.walk()) { succTime += TransitNetwork::TRANSFER_BUFFER; }
          bool validSuccTime = travelTime != DirectConnection::INFINITE;

          // Check if it's possible to walk from stop to succStop, if current
          // label is not the start stop and it has not been walked in the
          // previous step, expand additional walk label
          bool walked = false;
          int succPenalty = penalty;
          if (!label.walk()) {
            ++succPenalty;
            int walkSuccTime = std::numeric_limits<int>::max();
            const vector<Arc>& walkingArcs = _network.walkway(stop, succStop);
            if (walkingArcs.size()) {
              walkSuccTime = time + walkingArcs[0].cost();
              if (succNode != _graph.targetNode()) {
                walkSuccTime += TransitNetwork::TRANSFER_BUFFER;
                int earliestDep = std::numeric_limits<int>::max();
                const set<int>& walkSuccs = _graph.successors(succNode);
                for (auto it = walkSuccs.begin(); it != walkSuccs.end(); ++it) {
                  int nextDep = dc.nextDepartureTime(_graph.stopIndex(succNode),
                                                     startTime + walkSuccTime,
                                                     _graph.stopIndex(*it));
                  if (nextDep < earliestDep)
                    earliestDep = nextDep;
                }
                walkSuccTime = earliestDep - startTime;
              }
              if (walkSuccTime < std::numeric_limits<int>::max()
                  && (!validSuccTime
                      || (validSuccTime && walkSuccTime <= succTime))) {
                walked = true;
                validSuccTime = true;
                succTime = walkSuccTime;
              }
            }
          }

          if (validSuccTime
              && succPenalty <= _maxPenalty
              && result.destLabels.candidate(succTime, succPenalty)
              && result.matrix.candidate(succNode, succTime, succPenalty)) {
            queue.push(result.matrix.add(succNode, succTime, succPenalty,
                                         _maxPenalty, walked, false, label));
          }
        }
      }
    }
  }
}
