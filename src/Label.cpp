// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include "./Label.h"
#include <boost/foreach.hpp>
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>
#include "./Utilities.h"

using std::pair;
using std::swap;

// LabelVec

LabelVec::LabelVec()
  : _at(-1), _numUsed(0), _updated(true) {}

LabelVec::LabelVec(const int at, const unsigned char maxPenalty)
  : _at(at), _numUsed(0), _updated(true) {
  _fields.reserve(maxPenalty + 1);
  for (unsigned char i = 0; i <= maxPenalty; ++i) {
    _fields.push_back(Field(_at, i, maxPenalty / 2));
    assert(_fields.at(i).at == _at);
  }
}

int LabelVec::pruneInactive() {
  int pruned = 0;
  for (auto it = _fields.begin(), end = _fields.end(); it != end; ++it) {
    Field& field = *it;
    if (field.used() && field.inactive()) {
      ++pruned;
      field.used(false);
    }
  }
  _numUsed -= pruned;
  _updated = true;
  return pruned;
}

LabelVec::const_iterator LabelVec::begin() const {
  if (_updated) {
    _bakedLabels.clear();
    _bakedLabels.reserve(_numUsed);
    const unsigned char numFields = _fields.size();
    for (unsigned char i = 0; i < numFields; ++i) {
      const Field& field = _fields[i];
      if (field.used()) {
        _bakedLabels.push_back(Hnd(field.cost, i, field.inactive(),
                                   const_cast<LabelVec::Field*>(&field)));
      }
    }
    _updated = false;
  }
  return _bakedLabels.begin();
}

LabelVec::const_iterator LabelVec::end() const {
  if (_updated) {
    _bakedLabels.clear();
    _bakedLabels.reserve(_numUsed);
    const unsigned char numFields = _fields.size();
    for (unsigned char i = 0; i < numFields; ++i) {
      const Field& field = _fields[i];
      if (field.used()) {
        _bakedLabels.push_back(Hnd(field.cost, i, field.inactive(),
                                   const_cast<LabelVec::Field*>(&field)));
      }
    }
    _updated = false;
  }
  return _bakedLabels.end();
}


bool LabelVec::candidate(unsigned int cost, unsigned char penalty) const {
  assert(penalty < _fields.size());
  int pos = penalty;
  while (pos >= 0 && !_fields[pos].used()) {
    --pos;
  }
//   const Field& field = _fields[pos];
  return pos < 0 || cost < _fields[pos].cost;
}

LabelVec::Field* LabelVec::add(const unsigned int cost,
                               const unsigned char penalty,
                               const unsigned char maxPenalty,
                               const bool walk, const bool inactive,
                               LabelVec::Field* parent) {
  assert(penalty < _fields.size());
  assert(_at >= 0);
  _numUsed += !_fields[penalty].used();
  // assert(_fields.at(penalty).at == _at);
  _fields[penalty] = Field(_at, penalty, maxPenalty, cost, walk, inactive,
                           parent);
  assert(_fields[penalty].at == _at);
  size_t pos = penalty + 1;
  while (pos < _fields.size() &&
         (!_fields[pos].used() || cost <= _fields[pos].cost)) {
    _numUsed -= _fields[pos].used();
    _fields[pos].used(false);
    ++pos;
  }
  _updated = true;
  return &_fields[penalty];
}

void LabelVec::add(const LabelVec::Hnd& label, const LabelVec::Hnd& parent) {
  Field* field = add(label.cost(), label.penalty(), label.maxPenalty(),
                     label.walk(),
                     label.inactive(), parent.field());
  field->at = label.at();
}

const LabelVec::Field& LabelVec::field(const unsigned char penalty) const {
  assert(penalty < _fields.size());
  return _fields[penalty];
}

void LabelVec::deactivate(const unsigned int cost,
                          const unsigned char penalty) {
  assert(penalty < _fields.size());
  assert(_at >= 0);
  assert(_fields[penalty].at == _at);

  size_t pos = penalty;
  const size_t numFields = _fields.size();
  while (pos < numFields &&
         (!_fields[pos].used() || cost <= _fields[pos].cost)) {
    // Set inactive:
    _fields[pos].inactive(true);
    ++pos;
  }
  _updated = true;
}


int LabelVec::size() const {
  return _numUsed;
}

int LabelVec::minCost() const {
  unsigned int m = INT_MAX;
  for (auto it = _fields.begin(), end = _fields.end(); it != end; ++it) {
    const Field& field = *it;
    if (field.used()) {
      m = min(m, field.cost);
    }
  }
  return m;
}

int LabelVec::minPenalty() const {
  size_t pen = 0;
  const size_t numFields = _fields.size();
  while (pen < numFields && !_fields[pen].used()) {
    ++pen;
  }
  return pen < numFields ? pen : INT_MAX;
}

int LabelVec::at() const {
  return _at;
}

// LabelMatrix

void LabelMatrix::resize(const int numNodes, const unsigned char maxPenalty) {
  _matrix.clear();
  _matrix.reserve(numNodes);
  for (int i = 0; i < numNodes; ++i) {
    _matrix.push_back(LabelVec(i, maxPenalty));
  }
}

bool LabelMatrix::candidate(const int at, const unsigned int cost,
                            const unsigned char penalty) const {
  assert(at >= 0 && at < size());
  return _matrix[at].candidate(cost, penalty);
}

bool LabelMatrix::contains(const int at, const unsigned char penalty) const {
  assert(at >= 0 && at < size());
  return _matrix[at].field(penalty).used();
}

bool LabelMatrix::closed(const int at, const unsigned char penalty) const {
  assert(at >= 0 && at < size());
  return _matrix[at].field(penalty).closed();
}

LabelMatrix::Hnd LabelMatrix::add(const int at, const unsigned int cost,
                                  const unsigned char penalty,
                                  const unsigned char maxPenalty) {
  return add(at, cost, penalty, maxPenalty, false, false, Hnd());
}

LabelMatrix::Hnd LabelMatrix::add(const int at, const unsigned int cost,
                                  const unsigned char penalty,
                                  const unsigned char maxPenalty,
                                  const bool walk,
                                  const bool inactive, const Hnd& parent) {
  assert(at < size());
  LabelVec::Field* field = _matrix[at].add(cost, penalty, maxPenalty,
                                           walk, inactive,
                                           parent.field());
  return Hnd(cost, penalty, inactive, field);
}

void LabelMatrix::deactivate(const int at, const unsigned int cost,
                                         const unsigned char penalty) {
  assert(at < size());
  _matrix[at].deactivate(cost, penalty);
}


LabelMatrix::Hnd LabelMatrix::parent(const LabelMatrix::Hnd& succ) const {
  assert(succ.field());
  LabelVec::Field* parent = succ.field()->parent;
  // assert(!parent || parent->used());
  const unsigned int cost = parent ? parent->cost : 0u;
  const unsigned char penalty = parent ? parent->penalty : 0u;
  const bool inactive = parent ? parent->inactive() : false;
  return Hnd(cost, penalty, inactive, parent);
}

int LabelMatrix::pruneInactive() {
  int pruned = 0;
  for (auto it = _matrix.begin(), end = _matrix.end(); it != end; ++it) {
    pruned += it->pruneInactive();
  }
  return pruned;
}

const LabelVec& LabelMatrix::at(const int at) const {
  assert(at >= 0 && at < size());
  return _matrix[at];
}

LabelVec& LabelMatrix::at(const int at) {
  assert(at >= 0 && at < size());
  return _matrix[at];
}

LabelMatrix::const_iterator LabelMatrix::cbegin() const {
  return _matrix.cbegin();
}

LabelMatrix::const_iterator LabelMatrix::cend() const {
  return _matrix.cend();
}

int LabelMatrix::size() const {
  return _matrix.size();
}

int LabelMatrix::numLabels() const {
  int numLabels = 0;
  for (auto it = _matrix.begin(), end = _matrix.end(); it != end; ++it) {
    numLabels += it->size();
  }
  return numLabels;
}
