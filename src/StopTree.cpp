// Copyright 2012: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include <boost/random/uniform_real.hpp>
#include <ctime>
#include <string>
#include "./StopTree.h"
#include "./Utilities.h"
#include "./Random.h"


using boost::minstd_rand;
using boost::uniform_real;

const float Stop::kInvalidPos = 999.0f;

// _____________________________________________________________________________
// STOP METHODS

Stop::Stop()
    : _lat(kInvalidPos),
      _lon(kInvalidPos) {}

Stop::Stop(const string& id, float lat, float lon)
      : _stopId(id), _stopName(""), _lat(lat), _lon(lon) {}

Stop::Stop(const string& id, const string& name, float lat, float lon)
      : _stopId(id), _stopName(name),  _lat(lat), _lon(lon) {}

bool Stop::operator==(const Stop& other) const {
  return /*_nodeIndices == other._nodeIndices &&*/
         _stopId == other._stopId &&
         _stopName == other._stopName &&
         _lat == other._lat &&
         _lon == other._lon;
}

std::ostream& operator<<(std::ostream& s, const Stop& stop) {
  s << stop.id() << "," << stop.name() << "," << stop.lat() << ","
    << stop.lon();
  return s;
}


// _____________________________________________________________________________
// STOPTREE METHODS

const StopTree::value_type& StopTree::randomWalk(RandomFloatGen* random) const {
  // for probabilities
  int level = 0;
  int covered = 0;
  float N = size() * 2.f;  // "* 2.f" is a hack: Since the KDTree is no perfect
                           // binary tree completely filled at each level, the
                           // expected stop depth of the search is to low.
  const _Node<value_type>* current = _M_get_root();
  const value_type* elem = NULL;
  while (current != NULL) {
    elem = &_S_value(current);
    // compute probability to descent further, such that all nodes of the tree
    // are choosen with approximate equal probability
    covered += pow(2, level);
    float pDescent = 1 - (covered / N);
    float randomFloat = random->next();
//     printf("%f, %f\n", pDescent, randomFloat);
    if (randomFloat <= pDescent || pDescent < 0) {
      if (random->next() < 0.5f) {
        current = _S_left(current);
      } else {
        current = _S_right(current);
      }
      level++;
    } else {
//       break; or use (optimizes better):
      current = NULL;
    }
  }
  assert(elem);
//   printf("\n");
  return *elem;
}
