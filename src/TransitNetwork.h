// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#ifndef SRC_TRANSITNETWORK_H_
#define SRC_TRANSITNETWORK_H_

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <google/dense_hash_map>
#include <kdtree++/kdtree.hpp>
#include <ostream>
#include <vector>
#include <string>
#include <functional>
#include <queue>
#include "gtest/gtest_prod.h"  // Needed for FRIEND_TEST in this case.
#include "./StopTree.h"
#include "./GeoInfo.h"

// #define CREATE_WALKWAY_STATISTICS

using std::vector;
using std::string;
using std::pair;
using std::make_pair;
using google::dense_hash_map;


// Represents a Node in a transit network.
class Node {
 public:
  enum Type {
    NONE = 0,
    ARRIVAL,
    TRANSFER,
    DEPARTURE,
  };

  // Constructor
  Node(const int stopIndex, const Node::Type& type, int time)
    : _stop(stopIndex), _type(type), _time(time) {}
  // Node comparison
  bool operator==(const Node& other) const;
  int stop() const { return _stop; }
  Type type() const { return _type; }
  int time() const { return _time; }
  // Returns a string representation of the node for debug purposes.
//   string debugString() const;

 private:
  // Constructor
  Node()
    : _stop(-1), _type(NONE), _time(-1) {}
  // serialization / deserialization
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {  //NOLINT
    ar & _stop;
    ar & _time;
    ar & _type;
  }
  friend class boost::serialization::access;

  // stop index
  int _stop;
  Type _type;
  int _time;
};


// Converts an enum type to a string.
string type2Str(const Node::Type type);


// A functor to compare nodes[a] and nodes[b] for sorting, sorts ascending by
// time and transfer nodes before departure nodes. This sophisticated structure
// is used here, because a standard static comparison function cannot access the
// non-static _nodes array.
class CompareNodesByTime : std::binary_function<int, int, bool> {
 public:
  explicit CompareNodesByTime(const vector<Node>* nodes) : _nodes(nodes) {}
  bool operator() (const int& a, const int& b);
  const vector<Node>* _nodes;
};


// An arc to a destination node specified by its id with a certain cost.
class Arc {
 public:
  Arc(const int dest, const unsigned int cost, const unsigned char penalty)
    : _dest(dest), _cost(cost), _penalty(penalty) {}

  bool operator==(const Arc& rhs) const {
    return _dest == rhs._dest && _cost == rhs._cost && _penalty == rhs._penalty;
  }

  int destination() const { return _dest; }
  unsigned int cost() const { return _cost; }
  unsigned char penalty() const { return _penalty; }

 private:
  Arc() {}
  // serialization / deserialization
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {  // NOLINT
    ar & _dest;
    ar & _cost;
    ar & _penalty;
  }
  friend class boost::serialization::access;

  int _dest;
  unsigned int _cost;
  unsigned char _penalty;
};


// The TransitNetwork class. It's a graph.
class TransitNetwork {
 public:
  // The time needed to get off a vehicle and to board another.
  static const int TRANSFER_BUFFER;
  // The farthest distance between stops that can be walked.
  static const float MAX_WALKWAY_DIST;

  // Constructor
  explicit TransitNetwork();

  // Destructor
  ~TransitNetwork();

  // Copy Constructor
  TransitNetwork(const TransitNetwork& other);

  // Assignment Operator
  TransitNetwork& operator=(const TransitNetwork& other);

  // Resets the TransitNetwork, clearing all data.
  void reset();

  // A Validity check for the graph. Asserts on error.
  void validate() const;

  // Performs all actions that have to be done after both parsing and loading
  // a network. I.e. construct stuff that cannot be serialized.
  void preprocess();

  // Creates a compressed, i.e. time independent version of the network: For
  // each stop it has one node and between two nodes there is an arc with cost
  // as the cost of the fastest connection between two arrival and departure of
  // the  corresponding stops in the original network.
  TransitNetwork createTimeCompressedNetwork() const;
  FRIEND_TEST(DijkstraTest, compressedNetwork);

  // Creates a mirrored version of the network: For each arc, there is an arc of
  // opposite direction and equal cost. Used to determine connected components.
  const TransitNetwork mirrored() const;

  // Returns the largest connected component if we consider the network as bi-
  // directional graph. NOTE(jonas): Right now, the component has the same arcs
  // and nodes as the original network. Just the stops are filtered.
  const TransitNetwork largestConnectedComponent() const;
  FRIEND_TEST(TransitNetworkTest, largestConnectedComponent);

  // Sets the network name.
  void name(const string& name);

  // Returns the network name;
  string name() const;

  // Returns the number of nodes.
  size_t numNodes() const;

  // Returns the number of Arcs.
  size_t numArcs() const;

  // Returns the number of stops.
  size_t numStops() const;

  // Returns node reference for given node index.
  inline const Node& node(const size_t node) const { return _nodes.at(node); }

  // Returns a reference to the vector of adjacency lists.
  const vector<vector<Arc> >& adjacencyLists() const;

  // Returns a reference to the adjacency list of given node.
  const vector<Arc>& adjacencyList(const size_t node) const;

  // Returns stop reference for given stop index.
  const Stop& stop(const size_t stop) const;

  // Returns stop reference for given stop index.  TODO(sawine): unsafe.
  Stop& stop(const size_t stop);

  // Accesses the walking graph
  const vector<vector<Arc> >& walkingGraph() const;

  // Returns a reference to the i-th walkwayList of the walking graph. Its
  // elements are the arcs starting at stop i.
  const vector<Arc>& walkwayList(const int i) const;

  // Get the walking arc between two stops. Result is empty, when there is no
  // such arc and contains the single arc otherwise.
  vector<Arc> walkway(const int stopFrom, const int stopTo) const;

  // Returns a reference to the KDTree of stops.
  const StopTree& stopTree() const;

  // Adds a new node, returns its index in the _nodes vector.
  int addTransitNode(const int stopIndex, const Node::Type& type,
                     int time);

  // Adds a stop to the network.
  void addStop(Stop& stop);  // NOLINT

  // Adds an arc to target with cost to the adjacency list of source. Handicap
  // is set to 0 if not specified.
  void addArc(int source, int target, int cost);
  void addArc(int source, int target, int cost, int penalty);

  // Returns the nearest stop to the given coordinates.
  // Returns NULL if no suitable stop could be found.
  const Stop* findNearestStop(const float lat, const float lon) const;

  // Returns suitable start nodes for given stop and time.
  vector<int> findStartNodeSequence(const Stop& stop, const int ptime) const;

  // Returns all dep Nodes of the given stop
  const vector<int> getDepNodes(const int stopIndex) const;

  // Returns the index of stop given by stop id.
  // Returns -1 if stop id is not known.
  int stopIndex(const string& id) const;

  const GeoInfo& geoInfo() const;

  // Returns a string representation for debug purposes.
  string debugString() const;
  // Returns a string representation of the walking graph for debug purposes.
  string debugStringOfWalkingGraph() const;

 private:
  // For a certain STOP returns the position in stop.nodeIndices i for the first
  // node after TIME.
  int findFirstNode(const Stop& stop, const int time) const;
  FRIEND_TEST(TransitNetworkTest, findFirstNode);

  // Constructs the kdtree from the vector of stops.
  void buildKdtreeFromStops();

  // Constructs the graph of walking arcs between stops. Retrieves the neighbors
  // within a certain distance (in meters) for all stops and adds arcs to those
  // with costs according to the manhattan distance.
  void buildWalkingGraph(const float dist);
  FRIEND_TEST(GtfsParserTest, walkingGraphConstruction);
  FRIEND_TEST(GtfsParserTest, walkingGraphNonReflexive);

  // Computes geometric information about the network: center, min, max
  void computeGeoInfo();
  FRIEND_TEST(TransitNetworkTest, computeGeoInfo);

  // Performs breadth first search remembering visited nodes. Used on mirrored
  // time-compressed networks to determine connected components.
  vector<size_t> connectedComponentNodes(size_t startNode) const;

  vector<Node> _nodes;
  vector<vector<Arc> > _adjacencyLists;
  size_t _numArcs;

  vector<Stop> _stops;
  // Walking arcs between stops.
  vector<vector<Arc> > _walkwayLists;
  // A (2-)Kdtree to locate the nearest stop to a certain lat-lon-coordinate.
  StopTree _mapOfStops;
  // Geometric information on the network.
  GeoInfo _geoInfo;

  dense_hash_map<string, int> _stopId2indexMap;
  string _name;

  // serialization / deserialization: add new members to the archive using '&'
  // cannot serialize the kd tree, reconstruct it after deserialization
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {  // NOLINT
    ar & _nodes;
    ar & _adjacencyLists;
    ar & _numArcs;
    ar & _stops;
    ar & _walkwayLists;
//     ar & _stopId2indexMap;  // <-- not serializeable
    ar & _name;
  }
  FRIEND_TEST(GtfsParserTest, serialization);
  friend class boost::serialization::access;
  friend class GtfsParser;
};


#endif  // SRC_TRANSITNETWORK_H_
