// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#ifndef SRC_TRANSFERPATTERNSDB_H_
#define SRC_TRANSFERPATTERNSDB_H_

#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/vector.hpp>
#include <vector>
#include <map>
#include <set>
#include <string>
#include "./HubSet.h"

using std::vector;
using std::map;
using std::set;

// Directed acyclic graph holding the transfer patterns in reversed direction
// - from destination stops to the departure stop.
class TransferPatternsGraph {
 public:
  static const int INVALID_NODE;

  // Pointer to the hubs set.
  static const HubSet* _hubs;

  // Copy constructor.
  TransferPatternsGraph(const TransferPatternsGraph& rhs);

  // Initialises the graph with the departure stop without hubs.
  explicit TransferPatternsGraph(const int depStop);

  // Initialises the graph with the departure stop and hubs.
  TransferPatternsGraph(const int depStop, const HubSet& hubs);

  // Returns the number of nodes within the graph including departure,
  // destination and prefix nodes.
  int numNodes() const;

  // Returns the departure stop index.
  int depStop() const;

  // Returns the destination node index for given stop if available and
  // INVALID_NODE otherwise.
  int destNode(const int stop) const;

  // Returns a const reference to the set of all hubs which occur as
  // destination nodes within the graph.
  const set<int>& destHubs() const;

  // Returns the stop index for a given graph node.
  int stop(const int node) const;

  // Returns a const reference to all successor node indices for a given node.
  const vector<int>& successors(const int node) const;

  // Adds nodes and connections according to the given transfer pattern.
  void addPattern(const vector<int>& stops);

  // Cleares cache required for efficient graph construction.
  // Use this after construction to increase query-time efficiency.
  void finalise();

  // Assignment operator.
  TransferPatternsGraph& operator=(const TransferPatternsGraph& rhs);

  // Swaps the non-construction-content of two graphs.
  void swap(TransferPatternsGraph& rhs);

  // Equality of TransferPatternGraphs. Used for testing.
  bool operator==(const TransferPatternsGraph& rhs) const;

  // Returns a debug string of the TPG.
  std::string debugString() const;

 private:
  static const set<int> _emptySet;

  // Connects a prefix node for given stop to its successor node.
  // Adds a new prefix node on demand.
  // Returns the index of the prefix node.
  int addPrefixNode(const int stop, const int successor);

  // Connects a destination node for given stop to its successor node.
  // Adds a new destination node on demand.
  // Returns the index of the destination node.
  int addDestNode(const int stop, const int successor);

  // Returns the first node of a prefix, which conforms to the given connection
  // stop -> successor if available and INVALID_NODE rhswise.
  // E.g. in graph {E->D->C->B->A, E->A, E->D->B->A}
  // C->B->A and B->A are the only proper prefixes.
  int findProperPrefix(const int stop, const int successor) const;

  // Returns the prefix nodes for given stop.
  const set<int>& prefixNodes(const int stop) const;

  vector<int> _nodes;
  vector<vector<int> > _successors;  // TODO(jonas): could be vector<set<int> >
  set<int> _destHubs;
  map<int, int> _destMap;

  // Used only during graph construction; cleared on finalising.
  map<int, set<int> > _prefixMap;

  // Default Constructor needed for serialization.
  TransferPatternsGraph() {}

  // Serialization.
  template<class Archive>
  void serialize(Archive& ar, const uint version) {  // NOLINT
    ar & _nodes;
    ar & _destHubs;
    ar & _successors;
    ar & _destMap;
  }
  friend class boost::serialization::access;
};
typedef TransferPatternsGraph TPG;


// The transfer patterns database holds transfer patterns graphs for all stops.
class TransferPatternsDB {
 public:
  // Copy constructor.
  TransferPatternsDB(const TransferPatternsDB& rhs);

  // Default construction without initialisation.
  TransferPatternsDB();

  // Initialises the database given the total number of stops and the hubs.
  TransferPatternsDB(const int numStops, const HubSet& hubs);

  // Initialises the database given the total number of stops and the hubs.
  void init(const int numStops, const HubSet& hubs);

  // Addition operator used for reduction of parallel results. Asserts both TPDB
  // have disjunct set of graphs, meaning they have the same number of graphs
  // BUT if one has a non-empty graph for a stop the corresponding graph of the
  // other is empty.
  TransferPatternsDB& operator+=(TransferPatternsDB& other);  // NOLINT

  // Returns a const reference to the transfer patterns graph for given
  // departure stop.
  const TPG& graph(const int depStop) const;
  // Returns a write reference to the graph used for swapping.
  TPG& getGraph(const int depStop);

  // Adds a transfer pattern to the database.
  // Remark: must be initialised before calling.
  void addPattern(const vector<int>& stops);

  // Returns the number of graphs in the database, which equals the number of
  // stops as provided by the initialisation.
  size_t numGraphs() const;

  // Cleares cache required for efficient graph construction.
  // Use this after the graph construction for the given departure stop
  // to increase query-time efficiency.
  // Remark: finalising during construction will increase the memory footprint
  // of the final graph.
  void finalise(const int depStop);

  // Assignment operator.
  TransferPatternsDB& operator=(const TransferPatternsDB& rhs);

  // TPDB comparison. Used for testing.
  bool operator==(const TransferPatternsDB& rhs) const;

 private:
  vector<TPG> _graphs;

  // Serialization.
  template<class Archive>
  void serialize(Archive& ar, const uint version) {  // NOLINT
    ar & _graphs;
  }
  friend class boost::serialization::access;
};
typedef TransferPatternsDB TPDB;


#endif  // SRC_TRANSFERPATTERNSDB_H_
