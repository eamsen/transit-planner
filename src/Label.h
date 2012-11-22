// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#ifndef SRC_LABEL_H_
#define SRC_LABEL_H_

#include <boost/serialization/access.hpp>
#include <climits>
#include <cassert>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <bitset>

using std::set;
using std::string;
using std::vector;
using std::bitset;
using std::min;


class LabelVec {
 public:
  struct Field {
    Field(const int at, const unsigned char penalty,
          const unsigned char maxPenalty)
      : at(at), penalty(penalty), maxPenalty(maxPenalty), cost(0),
        parent(NULL) {}

    Field(const int at, const unsigned char penalty,
          const unsigned char maxPenalty, const unsigned int cost,
          const bool walk, const bool inactive, Field* parent)
      : at(at), penalty(penalty), maxPenalty(maxPenalty), cost(cost),
        parent(parent) {
      used(true);
      this->walk(walk);
      this->inactive(inactive);
    }

    bool used() const { return _misc.test(0); }
    bool closed() const { return _misc.test(1); }
    bool inactive() const { return _misc.test(2); }
    bool walk() const { return _misc.test(3); }

    void used(bool value) { _misc.set(0, value); }
    void closed(bool value) { _misc.set(1, value); }
    void inactive(bool value) { _misc.set(2, value); }
    void walk(bool value) { _misc.set(3, value); }

    int at;
    unsigned char penalty;
    unsigned char maxPenalty;
    unsigned int cost;
    Field* parent;
    bitset<4> _misc;  // 0:used 1:closed 2:inactive 3:walk
  };

  // A label proxy interfacing with the internal structures of  LabelVec.
  class Hnd {
   public:
    struct Comp {
      bool operator()(const Hnd& lhs, const Hnd& rhs) const {
        return rhs < lhs;
      }
    };

    Hnd(const Hnd& rhs)
      : _values(rhs._values),
        _field(rhs._field),
        _inactive(rhs._inactive) {
      assert(at() == rhs.at());
    }

    Hnd() : _values(0), _field(NULL), _inactive(false) {}

    Hnd(unsigned int cost, unsigned char penalty, bool inactive,
        LabelVec::Field* field)
      : _values((cost << 8) | penalty), _field(field),
        _inactive(inactive) {}

    Hnd& operator=(const Hnd& rhs) {
      if (this == &rhs) {
        return *this;
      }
      _values = rhs._values;
      _field = rhs._field;
      _inactive = rhs._inactive;
      return *this;
    }

    bool operator>(const Hnd& rhs) const {
      return _values > rhs._values;
    }

    bool operator<(const Hnd& rhs) const {
      return _values < rhs._values;
    }

    // Implicit bool conversion showing whether the label is valid.
    operator bool() const { return valid(); }

    // Returns whether the label is valid. This is used for terminating
    // tracebacks of optimal paths.
    bool valid() const { return _field; }

    unsigned int cost() const { return _values >> 8; }
    unsigned char penalty() const { return _values & 0xff; }
    unsigned char maxPenalty() const { return _field->maxPenalty; }
    int at() const { return _field->at; }
    bool closed() const { return _field->closed(); }
    bool inactive() const { return _inactive; }
    bool walk() const { return _field->walk(); }

    void inactive(bool value) { _inactive = value; }
    void closed(bool value) { _field->closed(value); }

    Field* field() const { return _field; }

   private:
    unsigned int _values;
    Field* _field;
    bool _inactive;
  };

  typedef vector<Hnd>::const_iterator const_iterator;

  LabelVec();
  LabelVec(const int at, const unsigned char maxPenalty);

  const_iterator begin() const;

  const_iterator end() const;

  // Returns whether the cost, penalty pair is optimal within the vector.
  bool candidate(const unsigned int cost, const unsigned char penalty) const;

  // Adds a new label, possibly overwritting the old label with same penalty.
  Field* add(const unsigned int cost, const unsigned char penalty,
             const unsigned char maxPenalty,
             const bool walk, const bool inactive, Field* parent);

  void add(const Hnd& label, const Hnd& parent);

  // Returns const reference to the field at given penalty.
  const Field& field(const unsigned char penalty) const;

  // Removes all inactive labels.
  int pruneInactive();

  // Returns the vector size, which equals to the max penalty + 1.
  int size() const;

  // Returns the minimum cost value of all labels.
  int minCost() const;

  // Returns the minimum penalty value of all labels.
  int minPenalty() const;

  // Returns the node index at which this labels are connected.
  int at() const;

  // Sets the inactive value to true
  // for all labels which are worse the given cost and penalty
  void deactivate(const unsigned int cost, const unsigned char penalty);

 private:
  int _at;
  int _numUsed;
  vector<Field> _fields;
  mutable bool _updated;
  mutable vector<Hnd> _bakedLabels;
};

// Holds a label vector for each node and ways to operate on them.
class LabelMatrix {
 public:
  typedef LabelVec::Hnd Hnd;
  typedef vector<LabelVec>::const_iterator const_iterator;

  // Resizes the matrix given the number of nodes and max penalty.
  // Resizing deletes all previous labels.
  void resize(const int numNodes, const unsigned char maxPenalty);

  // Returns whether given cost, penalty pair is optimal for given node id.
  bool candidate(const int at, unsigned int cost, unsigned char penalty) const;

  // Returns whether there is a label with given penalty for given node id.
  bool contains(const int at, unsigned char penalty) const;

  // Returns whether the label at given node id and penalty is set closed.
  bool closed(const int at, unsigned char penalty) const;

  // This is a specialisation for adding labels without parent labels.
  Hnd add(const int at, const unsigned int cost, const unsigned char penalty,
          const unsigned char maxPenalty);

  // Adds a successor label for given node id with given cost and penalty.
  // Returns a label proxy for the newly created label data.
  Hnd add(const int at, const unsigned int cost, const unsigned char penalty,
          const unsigned char maxPenalty, const bool walk, const bool inactive,
          const Hnd& parent);

  // Removes all inactive labels.
  // Returns the number of labels removed.
  int pruneInactive();

  // Returns the parent label proxy for given successor label.
  // If no parent is available it returns an invalid label.
  Hnd parent(const Hnd& succ) const;

  // Returns a const reference to the label vector for given node index.
  const LabelVec& at(const int at) const;

  // Returns a reference to the label vector for given node index.
  LabelVec& at(const int at);

  // Sets the inactive value to true for all labels at the given node
  // which are worse the given cost and penalty. Used by arrival loop.
  void deactivate(const int at, const unsigned int cost,
                                const unsigned char penalty);


  const_iterator cbegin() const;
  const_iterator cend() const;

  // Returns the size of the matrix, which equals to the number of label vectors
  // it serves.
  int size() const;

  int numLabels() const;

 private:
  vector<LabelVec> _matrix;
};

#endif  // SRC_LABEL_H_
