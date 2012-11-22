// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include "./Random.h"
#include <cmath>
#include "./Utilities.h"

RandomGen::RandomGen(const int minValue, const int maxValue, const int seed)
    : _seed(seed), _base(_seed), _dist(minValue, maxValue),
      _gen(_base, _dist) {}

RandomGen::RandomGen(const int minValue, const int maxValue)
    : _seed(getSeed()), _base(_seed), _dist(minValue, maxValue),
      _gen(_base, _dist) {}

int RandomGen::next() {
  return _gen();
}

int RandomGen::seed() const {
  return _seed;
}


RandomFloatGen::RandomFloatGen(const float min, const float max, const int seed)
    : _seed(seed), _base(_seed), _distribution(min, max),
      _generator(_base, _distribution) {}

RandomFloatGen::RandomFloatGen(const float min, const float max)
    : _seed(getSeed()), _base(_seed), _distribution(min, max),
      _generator(_base, _distribution) {}

float RandomFloatGen::next() {
  return _generator();
}

int RandomFloatGen::seed() const {
  return _seed;
}


ExpDistribution::ExpDistribution(const int seed, const float beta)
    : RandomFloatGen(0., 1., seed), _beta(beta) {
  assert(_beta != 0);
}

float ExpDistribution::sample() {
  float u;
  do { u = next(); }
  while (u == 0.);
  float sample = -std::log(u) / _beta;
  assert(sample >= 0);
  return sample;
}
