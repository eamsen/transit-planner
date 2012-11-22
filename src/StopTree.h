// Copyright 2012: Eugen Sawin, Philip Stahl, Jonas Sternisko
#ifndef SRC_STOPTREE_H_
#define SRC_STOPTREE_H_

#include <boost/serialization/access.hpp>
#include <kdtree++/kdtree.hpp>
#include <string>
#include <vector>

class RandomFloatGen;

using std::string;
using std::vector;

// A class for a transit stop, e.g. 'Haltestelle Runzmattenweg'.
class Stop {
 public:
  static const float kInvalidPos;

  // Constructor (standard, short, full)
  Stop();
  Stop(const string& id, float lat, float lon);
  Stop(const string& id, const string& name, float lat, float lon);
  string id() const { return _stopId; }
  string name() const { return _stopName; }
  float lat() const { return _lat; }
  float lon() const { return _lon; }
  int index() const { return _index; }
  void index(const int i) { _index = i; }
  const vector<int>& getNodeIndices() const { return _nodeIndices; }
  void addNodeIndex(const int id) { _nodeIndices.push_back(id); }
  int numNodes() const { return _nodeIndices.size(); }
  // Returns the index of the Stop's ith node.
  int nodeIndex(const int i) const { return _nodeIndices.at(i); }
  vector<int>::iterator nodesBegin() { return _nodeIndices.begin(); }
  vector<int>::iterator nodesEnd() { return _nodeIndices.end(); }
  // Compare operator.
  bool operator==(const Stop& other) const;

 private:
  string _stopId;
  string _stopName;
  float _lat;
  float _lon;
  // vector of all indices of transit nodes for this stop
  vector<int> _nodeIndices;
  int _index;

  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {  // NOLINT
    ar & _stopId;
    ar & _stopName;
    ar & _lat;
    ar & _lon;
    ar & _nodeIndices;
    ar & _index;
  }
  friend class boost::serialization::access;
};


std::ostream& operator<<(std::ostream& s, const Stop& stop);  // NOLINT


// A node in the kdtree with a position and a pointer to a Node.
class StopTreeNode {
 public:
  typedef float value_type;

  StopTreeNode() { stopPtr = NULL; }
  explicit StopTreeNode(const Stop& stop) {
    stopPtr = &stop;
    pos[0] = stopPtr->lat();
    pos[1] = stopPtr->lon();
  }
  value_type operator[](size_t i) const {
    return pos[i];
  }
  /*float distance(const StopKDTreeNode& other) {
    // NOTE: Here one could use the Great Circle distance, or the Eucleadean
    // distance but internally the max over all dimensions is used.
    float x = pos[0] - other.pos[0];
    float y = pos[1] - other.pos[1];
    return sqrt(x * x + y * y);
  }*/
  value_type pos[2];
  const Stop* stopPtr;
};


using KDTree::_Node;
// Extension to a 2-d tree in order to implement random descent in the tree.
class StopTree : public KDTree::KDTree<2, StopTreeNode> {
 public:
  // Performs a descent starting at the node. Returns a random node meaning that
  // the direction and depth of the walk depent are choosen probabilistically.
  const StopTree::value_type& randomWalk(RandomFloatGen* random) const;
};


#endif  // SRC_STOPTREE_H_
