// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#ifndef SRC_RANDOM_H_
#define SRC_RANDOM_H_

#include <boost/random/linear_congruential.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/generator_iterator.hpp>

using boost::minstd_rand;
using boost::uniform_int;
using boost::uniform_real;

// Generator for random int sequences.
class RandomGen {
 public:
  typedef boost::variate_generator<minstd_rand&, uniform_int<> >  Generator;

  // Construct a random generator with given min, max and seed.
  RandomGen(const int minValue, const int maxValue, const int seed);

  // Construct a random generator with given min, max and arbitrary seed.
  RandomGen(const int minValue, const int maxValue);

  // Returns the next "random" number in sequence.
  int next();

  // Returns the seed used.
  int seed() const;

 private:
  int _seed;
  minstd_rand _base;
  uniform_int<> _dist;
  Generator _gen;
};


// Generator for random float sequences.
class RandomFloatGen {
  typedef boost::variate_generator<minstd_rand&, uniform_real<> >  Generator;
 public:
  // Construct a random generator for numbers in [min, max) with seed.
  RandomFloatGen(const float min, const float max, const int seed);

  // Construct a random generator for numbers in [min, max) with automatic seed.
  RandomFloatGen(const float min, const float max);

  // Returns the next "random" number in sequence.
  float next();
  // Returns the seed used.
  int seed() const;

 private:
  int _seed;
  minstd_rand _base;
  uniform_real<> _distribution;
  Generator _generator;
};


// Generator for random samples from the exponential distribution with mean 1/b.
// p(x) = b * exp(-bx)
class ExpDistribution : public RandomFloatGen {
 public:
  ExpDistribution(const int seed, const float beta);
  float sample();
 private:
  float _beta;
};

#endif  // SRC_RANDOM_H_
