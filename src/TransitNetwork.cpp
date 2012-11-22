// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include "./TransitNetwork.h"
#include <cassert>
#include <limits>
#include <vector>
#include <string>
#include <queue>
#include <utility>
#include <functional>
#include <algorithm>
#include "./Utilities.h"

using std::pair;
using std::priority_queue;


// _____________________________________________________________________________
// NODE METHODS

bool Node::operator==(const Node& other) const {
  return _stop == other._stop && _type == other._type && _time == other._time;
}


string type2Str(const Node::Type type) {
  if (type == Node::ARRIVAL)
    return "ARRIVAL";
  if (type == Node::TRANSFER)
    return "TRANSFER";
  if (type == Node::DEPARTURE)
    return "DEPARTURE";
  if (type == Node::NONE)
    return "NONE";
  else
    return "UNDEFINED!";
}


bool CompareNodesByTime::operator()(const int& a, const int& b) {
  const Node& node1 = _nodes->at(a);
  const Node& node2 = _nodes->at(b);
  if (node1.time() < node2.time())
    return true;
  else if (node1.time() == node2.time() &&
           node1.type() == Node::TRANSFER  && node2.type() == Node::DEPARTURE)
    return true;
  else
    return false;
}


// string Node::debugString() const {
//   return "stop: " + convert<string>(_stop) + "," + type2Str(_type) + "@" +
//          time2str(_time);
// }


// _____________________________________________________________________________
// TRANSITNETWORK METHODS

const int TransitNetwork::TRANSFER_BUFFER = 120;
const float TransitNetwork::MAX_WALKWAY_DIST = 100.0f;


TransitNetwork::TransitNetwork()
    : _numArcs(0) {
  _stopId2indexMap.set_empty_key("");
  reset();
}


TransitNetwork::~TransitNetwork() {
}


TransitNetwork::TransitNetwork(const TransitNetwork& other)
    : _nodes(other._nodes), _adjacencyLists(other._adjacencyLists),
    _numArcs(other._numArcs), _stops(other._stops),
    _stopId2indexMap(other._stopId2indexMap), _name(other._name) {
  _mapOfStops = other._mapOfStops;
  _walkwayLists = other._walkwayLists;
  _geoInfo = other.geoInfo();
}


TransitNetwork& TransitNetwork::operator=(const TransitNetwork& other) {
  _nodes = other._nodes;
  _adjacencyLists = other._adjacencyLists;
  _numArcs = other._numArcs;
  _stops = other._stops;
  _stopId2indexMap = other._stopId2indexMap;
  _name = other._name;
  _mapOfStops = other._mapOfStops;
  _walkwayLists = other._walkwayLists;
  _geoInfo = other.geoInfo();
  return *this;
}


void TransitNetwork::reset() {
  _name = "";
  _nodes.clear();
  _adjacencyLists.clear();
  _numArcs = 0;
  _stops.clear();
  _walkwayLists.clear();
}


void TransitNetwork::validate() const {
  for (size_t n = 0; n < _adjacencyLists.size(); ++n) {
    const Node::Type nodeType = _nodes.at(n).type();
    const vector<Arc>& arcs = _adjacencyLists.at(n);
    for (auto arcIt = arcs.begin(); arcIt != arcs.end(); ++arcIt) {
      const Arc& arc = *arcIt;
      const Node::Type succType = _nodes.at(arc.destination()).type();
      assert((nodeType == Node::TRANSFER &&
              (succType == Node::TRANSFER || succType == Node::DEPARTURE)) ||
             (nodeType == Node::ARRIVAL &&
              (succType == Node::DEPARTURE || succType == Node::TRANSFER)) ||
             (nodeType == Node::DEPARTURE && succType == Node::ARRIVAL));
    }
  }
//  for (int i = 0; i < numStops(); i++) {
//    vector<int> nodes = stop(i).getNodeIndices();
//    for (int j = 0; j < nodes.size()-1; j++) {
//      assert(node(nodes[j]).time() <= node(nodes[j+1]).time());
//    }
//  }
}


void TransitNetwork::preprocess() {
  validate();
  buildKdtreeFromStops();
  // set up the walkway lists if not yet done
  if (_walkwayLists.size() == 0) {
    buildWalkingGraph(MAX_WALKWAY_DIST);
  }
  computeGeoInfo();
}


TransitNetwork TransitNetwork::createTimeCompressedNetwork() const {
  TransitNetwork compressed;
  // For each stop in the original network, add a stop and with one node.
  for (size_t i = 0; i < _stops.size(); ++i) {
    const Stop& orig = _stops[i];
    Stop s = Stop(orig.id(), orig.name(), orig.lat(), orig.lon());
    compressed.addStop(s);
    const int nodeIndex = compressed.addTransitNode(i, Node::NONE, 0);
    compressed.stop(i).addNodeIndex(nodeIndex);
  }
//   compressed.preprocess();  NOTE: not needed if walking arcs are included
  // For each stop: Iterate over adjacent arcs and collect the lowest cost to
  // each other stop.
  for (size_t i = 0; i < _stops.size(); ++i) {
    const vector<int>& nodes = _stops[i].getNodeIndices();
    dense_hash_map<int, unsigned int> minCosts;
    minCosts.set_empty_key(-1);
    for (auto nodeIter = nodes.begin(); nodeIter != nodes.end(); ++nodeIter) {
      for (auto arcIter = _adjacencyLists[*nodeIter].begin();
           arcIter != _adjacencyLists[*nodeIter].end(); ++arcIter) {
        const Arc& arc = *arcIter;
        const uint stopB = static_cast<uint>(_nodes[arc.destination()].stop());
        if (i != stopB) {
          auto result = minCosts.find(stopB);
          if (result == minCosts.end() || arc.cost() < minCosts[stopB])
            minCosts[stopB] = arc.cost();
        }
      }
    }
    // Add arcs with minimum cost to the compressed network.
    for (auto it = minCosts.begin(); it != minCosts.end(); ++it)
      compressed.addArc(i, it->first, it->second);
  }
  // Add walking arcs to the network
  for (size_t i = 0; i < _walkwayLists.size(); ++i) {
    for (size_t j = 0; j < _walkwayLists[i].size(); ++j) {
      const Arc& walkingArc = _walkwayLists[i][j];
      const int cost = walkingArc.cost() < static_cast<int>(TRANSFER_BUFFER) ?
                       TRANSFER_BUFFER : walkingArc.cost();
      compressed.addArc(i, walkingArc.destination(), cost);
    }
  }
  return compressed;
}


// The intension of this method is to remove isolated stops and separated parts
// from the transit network. Therefore a time-compressed network is created with
// all arcs mirrored. On this network, the largest connected component is found.
// Finally, the stops of this component are translated into stops of the
// original network including all node indices.
const TransitNetwork TransitNetwork::largestConnectedComponent() const {
  // determine node indices of the largest connected component
  TransitNetwork bidirect = createTimeCompressedNetwork().mirrored();
  vector<bool> visitedMarks(bidirect.numNodes(), false);
  vector<size_t> largestComponentNodes;
  for (size_t i = 0; i < bidirect.numNodes(); ++i) {
    if (!visitedMarks[i]) {
      vector<size_t> component = bidirect.connectedComponentNodes(i);
      for (auto node = component.cbegin(); node != component.end(); ++node)
        visitedMarks[*node] = true;
      if (component.size() > largestComponentNodes.size())
        component.swap(largestComponentNodes);
    }
  }
  // build the corresponding network: add all stops of the largest component's
  // nodes and all arcs between such stops
  TransitNetwork lcc = *this;
  lcc._stops.clear();
  lcc._stopId2indexMap.clear();
  vector<bool> inserted(bidirect.numStops(), false);
  for (auto node = largestComponentNodes.cbegin();
       node != largestComponentNodes.end();
       ++node) {
    const size_t stopIndex = bidirect.node(*node).stop();  // index from bidirec
    if (!inserted[stopIndex]) {
      Stop s = stop(stopIndex);  // copy from the original network
      lcc.addStop(s);
      inserted[stopIndex] = true;
    }
  }
  lcc._walkwayLists.clear();
  lcc.preprocess();
  return lcc;
}


const TransitNetwork TransitNetwork::mirrored() const {
  TransitNetwork mirrored = *this;
  for (size_t i = 0; i < _adjacencyLists.size(); ++i) {
    for (auto arc = adjacencyList(i).cbegin(); arc != adjacencyList(i).end();
         ++arc) {
      mirrored.addArc(arc->destination(), i, arc->cost());
    }
  }
  return mirrored;
}


vector<size_t> TransitNetwork::connectedComponentNodes(size_t startNode) const {
  assert(startNode < numNodes());
  vector<bool> retrieved(numNodes(), false)/*, expanded(retrieved)*/;
  vector<size_t> component;
  component.push_back(startNode);
  retrieved[startNode] = true;
  size_t index = 0;
  while (index < component.size()) {
//     if (!expanded[component[index]]) {
      const vector<Arc>& arcs = adjacencyList(component[index]);
      for (auto arc = arcs.cbegin(); arc != arcs.end(); ++arc) {
        const size_t dest = arc->destination();
        if (!retrieved[dest]) {
          component.push_back(dest);
          retrieved[dest] = true;
        }
      }
//       expanded[component[index]] = true;
//     }
    ++index;
  }
  return component;
}


void TransitNetwork::name(const string& name) {
  _name = name;
}

string TransitNetwork::name() const {
  return _name;
}


int TransitNetwork::addTransitNode(const int stopIndex,
                                   const Node::Type& type,
                                   int time) {
  // add the node, initialize its adjacency list and return its index in _nodes
  assert(_nodes.size() == _adjacencyLists.size());
  assert(stopIndex >= 0 && stopIndex < static_cast<int>(_stops.size()));
  const size_t index = _nodes.size();
  _nodes.push_back(Node(stopIndex, type, time));
  _adjacencyLists.push_back(vector<Arc>());
  stop(stopIndex).addNodeIndex(index);
  return index;
}


void TransitNetwork::addStop(Stop& stop) {  // NOLINT
  assert(_stopId2indexMap.find(stop.id()) == _stopId2indexMap.end());
  _stopId2indexMap[stop.id()] = _stops.size();
  stop.index(_stops.size());
  _stops.push_back(stop);
}


void TransitNetwork::addArc(int source, int target, int cost) {
  addArc(source, target, cost, 0);
}


void TransitNetwork::addArc(int source, int target, int cost, int penalty) {
  assert(cost >= 0);
  assert(penalty >= 0);
  assert(_adjacencyLists.size() == _nodes.size());
  assert(source < static_cast<int>(_nodes.size()));
  assert(target < static_cast<int>(_nodes.size()));
  _adjacencyLists[source].push_back(Arc(target, cost, penalty));
  _numArcs++;
}


const Stop* TransitNetwork::findNearestStop(const float lat,
                                            const float lon) const {
  const Stop* stop = NULL;
  StopTreeNode reference;
  reference.pos[0] = lat;
  reference.pos[1] = lon;
  std::pair<StopTree::const_iterator, float> found =
      _mapOfStops.find_nearest(reference);
  assert(found.first != _mapOfStops.end());
  stop = found.first->stopPtr;
  return stop;
}


vector<int> TransitNetwork::findStartNodeSequence(const Stop& stop,
                                                  const int time) const {
  const int numStopNodes = stop.numNodes();
  vector<int> nodes;
  nodes.reserve(numStopNodes * 0.66f);
  const int startIndex = findFirstNode(stop, time);
  if (startIndex >= 0) {
    for (int i = startIndex; i < numStopNodes; ++i) {
      const int nodeIndex = stop.nodeIndex(i);
      const int type = _nodes[nodeIndex].type();
      if (type == Node::DEPARTURE) {
          nodes.push_back(nodeIndex);
      } else if (type == Node::TRANSFER) {
          nodes.push_back(nodeIndex);
          break;
      }
    }
  }
  return nodes;
}


const vector<int> TransitNetwork::getDepNodes(const int stopIndex) const {
  const vector<int> nodeIndices = _stops[stopIndex].getNodeIndices();
  vector<int> depNodes;
  depNodes.reserve(nodeIndices.size() / 2);
  for (auto it = nodeIndices.begin(), end = nodeIndices.end();
       it != end; ++it) {
    if (_nodes[*it].type() == Node::DEPARTURE)
      depNodes.push_back(*it);
  }
  return depNodes;
}


int TransitNetwork::stopIndex(const std::string& id) const {
  dense_hash_map<string, int>::const_iterator it = _stopId2indexMap.find(id);
  return it != _stopId2indexMap.end() ? it->second : -1;
}

const GeoInfo& TransitNetwork::geoInfo() const {
  return _geoInfo;
}

string TransitNetwork::debugString() const {
  std::ostringstream oss;

  oss << "[" << _nodes.size() << "," << _numArcs;
  if (_adjacencyLists.size()) oss << ",";
  for (size_t i = 0; i < _adjacencyLists.size(); i++) {
    const vector<Arc>& arcs = _adjacencyLists[i];
    oss << "{";
    for (size_t j = 0; j < arcs.size(); j++) {
      oss << "(" << arcs[j].destination() << "," << arcs[j].cost() << ")";
    }
    oss << "}";
    if (i < _adjacencyLists.size() - 1) oss << ",";
  }
  oss << "]";
  return oss.str();
}

string TransitNetwork::debugStringOfWalkingGraph() const {
  std::ostringstream oss;
  oss << "[";
  for (size_t i = 0; i < _walkwayLists.size(); ++i) {
    const vector<Arc>& arcs = _walkwayLists[i];
    oss << "{";
    for (size_t j = 0; j < arcs.size(); j++) {
      oss << "(" << arcs[j].destination() << "," << arcs[j].cost() << ")";
    }
    oss << "}";
    if (i < _walkwayLists.size() - 1) oss << ",";
  }
  oss << "]";
  return oss.str();
}



int TransitNetwork::findFirstNode(const Stop& stop, const int ptime) const {
  // Perform a binary search on the sorted sequence of indices.
  int lowerLimit = 0;
  int upperLimit = stop.numNodes();
  int size = stop.numNodes();
  int center = -1;
  while (size > 1) {
    center = lowerLimit + size / 2;
    const int centerTime = _nodes[stop.nodeIndex(center)].time();
    if (ptime < centerTime) {
      upperLimit = center;
    } else if (ptime > centerTime) {
      lowerLimit = center;
    } else {  // ptime == centerTime
      lowerLimit = center;
      break;
    }
    size = upperLimit - lowerLimit;
  }
  center = lowerLimit;
  if (stop.numNodes()) {
    // Go to the left to the first index that corresponds to a node
    // with time less than PTIME or to the first position of INDICES.
    // Important for ranges of equal value.
    while (_nodes[stop.nodeIndex(center)].time() >= ptime && center > 0)
      center--;
    // We want the index of the first node with time >= PTIME
    // so increase CENTER.
    if (_nodes[stop.nodeIndex(center)].time() < ptime)
      center++;
  }
  return center;
}


void TransitNetwork::buildKdtreeFromStops() {
  _mapOfStops.clear();
  for (size_t i = 0; i < _stops.size(); i++) {
    _mapOfStops.insert(StopTreeNode(_stops[i]));
  }
  _mapOfStops.optimize();
}


void TransitNetwork::buildWalkingGraph(const float dist) {
  const int numStops_ = numStops();
  assert(numStops_);
  assert(!_mapOfStops.empty());
  assert(dist >= 0);
  _walkwayLists.clear();
  _walkwayLists.resize(numStops_);

  for (int i = 0; i < numStops_; ++i) {
    // collect all neighbors
    StopTreeNode reference(_stops[i]);
    vector<StopTreeNode> results;
    _mapOfStops.find_within_range(reference, dist,
                   std::back_insert_iterator<vector<StopTreeNode> >(results));

    const int numResults = results.size();
    // add arcs to all neighbors, not to the stop itself
    for (int j = 0; j < numResults; ++j) {
      const Stop& head = *(results[j].stopPtr);
      if (head == _stops[i]) {
        continue;
      } else {
        // compute the costs
        float dlat = _stops[i].lat() - head.lat();
        float dlon = _stops[i].lon() - head.lon();
        dlat = dlat * dlat;
        dlon = dlon * dlon;
        float d = greatCircleDistance(_stops[i].lat(), _stops[i].lon(),
                                      head.lat(), head.lon());
        if (d <= dist) {  // takes care for the kdtree shortcoming
          int cost = d / (5.f * 1000.f / 60.f / 60.f);  // speed is 5km/h
          int penalty = 1;
          _walkwayLists[i].push_back(Arc(head.index(), cost, penalty));
        }
      }
    }
  }

  #ifdef CREATE_WALKWAY_STATISTICS
  // TODO(sawine): move this into its own function, don't clutter up functions.
  // create a histogram for the walkway list size
  size_t max = 1;
  for (size_t i = 0; i < _walkwayLists.size(); i++)
    if (max < walkwayList(i).size())
      max = walkwayList(i).size();
    // TODO(sawine): use scope brackets, unclear if bug or not.
    vector<int> histogram(max + 1, 0);
  int sum = 0;
  for (size_t i = 0; i < _walkwayLists.size(); i++) {
    histogram[walkwayList(i).size()]++;
    sum += walkwayList(i).size();
  }

  char buf[256];
  snprintf(buf, 256 * sizeof(buf),
           "plot/%s-walkway-size-histogram_distance=%.0f.txt",
           _name.c_str(), maxWalkwayDistance);
  FILE* f = fopen(buf, "w");
  if (f) {
    fprintf(f, "# histogram of walkway list sizes\n");
    fprintf(f, "# number of stops: %d\n", numStops());
    fprintf(f, "# average number of walking arcs per stop %.2f\n",
            convert<float>(sum) / numStops());
    fprintf(f, "# histogram:\n# size count\n");
    for (size_t i = 0; i < histogram.size(); i++)
      fprintf(f, "%zu %d\n", i, histogram[i]);
    fclose(f);
    printf("Wrote walkway size histogram data for '%s' data set to %s.\n",
           _name.c_str(), buf);
  } else {
    printf("Could not open '%s'.\n", buf);
  }
  #endif
}


void TransitNetwork::computeGeoInfo() {
  const float kE = 0.00001f;
  const float maxFloat = std::numeric_limits<float>::max();
  _geoInfo.latMin = maxFloat;
  _geoInfo.latMax = -1.0f * maxFloat;
  _geoInfo.lonMin = maxFloat;
  _geoInfo.lonMax = -1.0f * maxFloat;
  for (size_t i = 0; i < _stops.size(); ++i) {
    const float lat = _stops[i].lat();
    const float lon = _stops[i].lon();
    assert(fabs(lat - Stop::kInvalidPos) > kE);
    assert(fabs(lon - Stop::kInvalidPos) > kE);
    _geoInfo.latMin = std::min(_geoInfo.latMin, lat);
    _geoInfo.latMax = std::max(_geoInfo.latMax, lat);
    _geoInfo.lonMin = std::min(_geoInfo.lonMin, lon);
    _geoInfo.lonMax = std::max(_geoInfo.lonMax, lon);
  }
}


const vector<vector<Arc> >& TransitNetwork::walkingGraph() const {
  return _walkwayLists;
}


const vector<Arc>& TransitNetwork::walkwayList(const int stop) const {
  assert(stop >= 0 && static_cast<size_t>(stop) < _walkwayLists.size());
  return _walkwayLists.at(stop);
}


vector<Arc>
TransitNetwork::walkway(const int stopFrom, const int stopTo) const {
  vector<Arc> result;
  for (auto it = walkwayList(stopFrom).begin(),
       end = walkwayList(stopFrom).end();
       it != end; ++it) {
    if (it->destination() == stopTo) {
      result.push_back(*it);
      break;
    }
  }
  return result;
}

const vector<vector<Arc> >& TransitNetwork::adjacencyLists() const {
  return _adjacencyLists;
}

const vector<Arc>& TransitNetwork::adjacencyList(const size_t node) const {
  assert(node >= 0 && static_cast<size_t>(node) < _adjacencyLists.size());
  return _adjacencyLists.at(node);
}

size_t TransitNetwork::numNodes() const {
  return _nodes.size();
}

size_t TransitNetwork::numArcs() const {
  return _numArcs;
}

size_t TransitNetwork::numStops() const {
  return _stops.size();
}

const Stop& TransitNetwork::stop(const size_t stop) const {
  assert(stop < numStops());
  return _stops.at(stop);
}

Stop& TransitNetwork::stop(const size_t stop) {
  assert(stop < numStops());
  return _stops.at(stop);
}

const StopTree& TransitNetwork::stopTree() const {
  return _mapOfStops;
}

