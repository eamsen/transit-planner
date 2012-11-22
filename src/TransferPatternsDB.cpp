// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include "./TransferPatternsDB.h"
#include <boost/foreach.hpp>
#include <cassert>
#include <cstring>  // for size_t
#include <vector>
#include <map>
#include <set>
#include <string>
#include "./Utilities.h"


// TransferPatternsGraph

const int TPG::INVALID_NODE = -1;
const set<int> TPG::_emptySet = set<int>();
const HubSet* TPG::_hubs = NULL;

TPG::TransferPatternsGraph(const TPG& rhs)
  : _nodes(rhs._nodes), _successors(rhs._successors),
    _destHubs(rhs._destHubs), _destMap(rhs._destMap),
    _prefixMap(rhs._prefixMap) {}

TPG::TransferPatternsGraph(const int depStop)
  : _nodes(1, depStop), _successors(1, vector<int>()) {
  _hubs = NULL;
}

TPG::TransferPatternsGraph(const int depStop, const HubSet& hubs)
  : _nodes(1, depStop), _successors(1, vector<int>()) {
  _hubs = &hubs;
}

int TPG::numNodes() const {
  return _nodes.size();
}

int TPG::depStop() const {
  assert(_nodes.size());
  return _nodes[0];
}

int TPG::destNode(const int stop) const {
  auto const it = _destMap.find(stop);
  if (it == _destMap.end()) {
      return INVALID_NODE;
  }
  return it->second;
}

const set<int>& TPG::destHubs() const {
  return _destHubs;
}

int TPG::stop(const int node) const {
  assert(node >= 0 && static_cast<size_t>(node) < _nodes.size());
  return _nodes[node];
}

const vector<int>& TPG::successors(const int node) const {
  assert(node >= 0 && static_cast<size_t>(node) < _nodes.size());
  return _successors[node];
}

void TPG::addPattern(const vector<int>& stops) {
  assert(stops.size() > 1);
  assert(stops[0] == _nodes[0]);
  // connect prefix nodes
  int succ = 0;
  for (size_t s = 1; s < stops.size() - 1; ++s) {
    succ = addPrefixNode(stops[s], succ);
  }
  // connect destination node
  addDestNode(stops.back(), succ);
}

int TPG::addPrefixNode(const int stop, const int successor) {
  assert(stop >= 0);
  assert(successor >= 0 && static_cast<size_t>(successor) < _nodes.size());
  int prefix = findProperPrefix(stop, successor);
  if (prefix == INVALID_NODE) {
    _successors.push_back(vector<int>(1, successor));
    _nodes.push_back(stop);
    safeInsert(_prefixMap, stop, _nodes.size() - 1);
    prefix = _nodes.size() - 1;
  }
  return prefix;
}

int TPG::addDestNode(const int stop, const int successor) {
  assert(stop >= 0);
  assert(successor >= 0 && static_cast<size_t>(successor) < _nodes.size());
  int dest = destNode(stop);
  if (dest == INVALID_NODE) {
    dest = _nodes.size();
    _destMap[stop] = dest;
    _successors.push_back(vector<int>(1, successor));
    _nodes.push_back(stop);
    if (_hubs && _hubs->find(stop) != _hubs->end()) {
      _destHubs.insert(stop);
    }
  } else if (!contains(_successors[dest], successor)) {
    _successors[dest].push_back(successor);
  }
  return dest;
}


int TPG::findProperPrefix(const int stop, const int successor) const {
  const set<int>& nodes = prefixNodes(stop);
  for (auto it = nodes.begin(), end = nodes.end(); it != end; ++it) {
    assert(successors(*it).size() == 1);
    if (successors(*it).back() == successor) {
      return *it;
    }
  }
  return INVALID_NODE;
}

const set<int>& TPG::prefixNodes(const int stop) const {
  auto const it = _prefixMap.find(stop);
  if (it == _prefixMap.end()) {
    return _emptySet;
  } else {
    return it->second;
  }
}

void TPG::finalise() {
  _prefixMap.clear();
  // making sure the memory is freed
  map<int, set<int> >().swap(_prefixMap);
}

TPG& TPG::operator=(const TPG& rhs) {
  _nodes = rhs._nodes;
  _successors = rhs._successors;
  _destHubs = rhs._destHubs;
  _destMap = rhs._destMap;
  _prefixMap = rhs._prefixMap;
  return *this;
}

void TPG::swap(TPG& rhs) {
  _nodes.swap(rhs._nodes);
  _successors.swap(rhs._successors);
  _destHubs.swap(rhs._destHubs);
  _destMap.swap(rhs._destMap);
}

bool TPG::operator==(const TPG& rhs) const {
  return _destHubs == rhs.destHubs() && _nodes == rhs._nodes &&
         _successors == rhs._successors;
}

string TPG::debugString() const {
  std::stringstream ss;
  for (uint i = 0; i < _nodes.size(); i++) {
    ss << i << ":(stop " << stop(i) << "):{";
    BOOST_FOREACH(int successor, _successors[i]) {
      ss << successor << ",";
    }
    ss << "}\n";
  }
  ss << "DestNodes:";
  for (auto it = _destMap.begin(); it != _destMap.end(); ++it) {
    ss << " " << it->second;
  }
  ss << "\n";
  return ss.str();
}


// TransferPatternsDB

TPDB::TransferPatternsDB(const TPDB& rhs)
  : _graphs(rhs._graphs) {}

TPDB::TransferPatternsDB() {}

TPDB::TransferPatternsDB(const int numStops, const HubSet& hubs) {
  assert(numStops);  // do not initialise with empty network
  init(numStops, hubs);
}

void TPDB::init(const int numStops, const HubSet& hubs) {
  // do not re-initialise the database
  assert(_graphs.empty());
  _graphs.reserve(numStops);
  for (int s = 0; s < numStops; ++s) {
    _graphs.push_back(TPG(s, hubs));
  }
}

TPDB& TPDB::operator+=(TPDB& other) {  // NOLINT
  assert(numGraphs() == other.numGraphs());
  // Swaps with each of the other's graphs where this TPDB has no own graph.
  for (size_t i = 0; i < numGraphs(); ++i) {
    TPG& otherGraph = other.getGraph(i);
    if (_graphs[i].numNodes() > 1) {
      assert(otherGraph.numNodes() == 1);
    } else {
      if (otherGraph.numNodes() > 1) {
        _graphs[i].swap(otherGraph);
      }
    }
  }
  return *this;
}


const TPG& TPDB::graph(const int depStop) const {
  assert(depStop >= 0 && static_cast<size_t>(depStop) < _graphs.size());
  return _graphs[depStop];
}

TPG& TPDB::getGraph(const int depStop) {
  return _graphs[depStop];
}


void TPDB::addPattern(const vector<int>& stops) {
  assert(stops.size() > 1);
  assert(stops[0] >= 0);
  assert(static_cast<size_t>(stops[0]) < _graphs.size());
  _graphs[stops[0]].addPattern(stops);
}

size_t TPDB::numGraphs() const {
  return _graphs.size();
}

void TPDB::finalise(const int depStop) {
  assert(depStop >= 0 && static_cast<size_t>(depStop) < _graphs.size());
  _graphs[depStop].finalise();
}

TPDB& TPDB::operator=(const TPDB& rhs) {
  _graphs = rhs._graphs;
  return *this;
}

bool TPDB::operator==(const TPDB& rhs) const {
  return _graphs == rhs._graphs;
}
