// Copyright 2012: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include "./ScenarioGenerator.h"
#include <set>
#include <string>
#include <vector>
#include "./Random.h"
#include "./TransitNetwork.h"
#include "./GtfsParser_impl.h"
#include "./Logger.h"
#include "./Line.h"
#include "./Utilities.h"


ScenarioParams::ScenarioParams() : delayPercentage(-1), delayMean(-1) {}


bool ScenarioParams::valid() const {
  return delayPercentage >= 0 && delayPercentage <= 100 && delayMean > 0 &&
         1. / delayMean > 0;
}

const string ScenarioParams::str() const {
  return "[ScenarioParams: ratio=" + convert<string>(delayPercentage) +
         "%, mu=" + convert<string>(delayMean) + "]";
}

ScenarioGenerator::ScenarioGenerator(const ScenarioParams& param)
  : _params(vector<ScenarioParams>(1, param)) {
  assert(validParams());
}

ScenarioGenerator::ScenarioGenerator(const vector<ScenarioParams>& params)
    : _params(params) {
  assert(validParams());
}

const vector<string>
ScenarioGenerator::extractGtfsDirs(const string& args) const {
  int start = args.find("-i") + 2;
  int end   = args.find(" -", start);
  const vector<string> vec = splitString(args.substr(start, end - start));
  return vec;
}

const TransitNetwork
ScenarioGenerator::gen(const string& networkName) {
  TransitNetwork network;
  vector<Trip> trips;
  const string info = "local/" + networkName + ".info";
  stringstream ss;
  ss << networkName << "_modified";
  for (auto it = _params.begin(); it != _params.end(); ++it)
    ss << "_" << it->str();
  std::ifstream ifs(info);
  if (validParams() && ifs.good()) {
    GtfsParser parser(&LOG);
    LOG.target("log/LOG.log");
    Logger l;
    l.target("log/experiments/" + ss.str() + "_" +
             time2str(localTime()) + ".scenario");
    l.info("=== Scenario Generation Log ===");
    l.info("Name: %s", ss.str().c_str());
    const string time = time2str(firstOfMay());
    string args;
    while (ifs.good()) {
      string tmp;
      ifs >> tmp;
      args.append(tmp + " ");
    }
    const vector<string> dirs = extractGtfsDirs(args);
    network.name(networkName);
    for (auto dir = dirs.cbegin(); dir != dirs.end(); ++dir) {
      // parse the original networks' gtfs data using the .info file
      parser.parseGtfs(*dir, &network);
      // delay trips according to the parameters
      parser._data->lastGtfsTrips = delayTrips(parser._data->lastGtfsTrips, &l);
      // create the delayed network
      parser.translateLastTripsToNetwork(time, time, &network, &trips);
    }
    parser.generateInterTripArcs(&network);
    network.preprocess();
  } else {
    LOG.error("or .info file not found: %s", info.c_str());
  }
  network.name(ss.str());
  _lastGeneratedTN = network;
  _lastGeneratedLines = LineFactory::createLines(trips);
  return network;
}

const vector<Trip>
ScenarioGenerator::delayTrips(const vector<Trip>& cTrips,
                              const Logger* log) const {
  assert(validParams());
  if (cTrips.size() <= 1)
    return cTrips;

  // select some % of the trips for each parameter setting
  vector<Trip> trips = cTrips;  // it's a copy, but that's ok
  vector<Trip> delayed, remaining;
  set<size_t> selected;
  RandomFloatGen random(0., 1., getSeed());
  for (auto params = _params.begin(); params != _params.end(); ++params) {
    const int nRequired = cTrips.size() * params->delayPercentage / 100.f;
    selected = selectNRandomIndices(trips.size(), nRequired, &random);
    // delay selected
    ExpDistribution distr(getSeed(), 1. / params->delayMean);
    for (size_t i = 0; i < trips.size(); ++i) {
      const Trip& trip = trips[i];
      if (contains(selected, i)) {
        const int index = floor(random.next() * (trip.size() - 1) + 0.5);
        const int delay = floor(distr.sample());
        delayed.push_back(delayTrip(trip, index, delay));
        if (log) {
          log->info("delayed trip %s from stop %i of %i with %i seconds",
                    trip.id().c_str(), index+1, trip.size(), delay);
        }
      } else {
        remaining.push_back(trips[i]);
      }
    }
    trips.swap(remaining);
    remaining.clear();
    selected.clear();
  }
  // add the remaining unchanged trips
  for (auto trip = trips.begin(); trip != trips.end(); ++trip) {
    delayed.push_back(*trip);
    if (log) log->info("keep trip %s unchanged", trip->id().c_str());
  }

  assert(delayed.size() == cTrips.size());
  return delayed;
}

const Trip ScenarioGenerator::delayTrip(const Trip& trip, const int index,
                                        int delay) const {
  if (index >= trip.size())
    return trip;
  Trip delayedTrip(trip.id());
  for (int i = 0; i < index; ++i)
    delayedTrip.addStop(trip.time().arr(i), trip.time().dep(i), trip.stop(i));
  for (int i = index; i < trip.size(); ++i)
    delayedTrip.addStop(trip.time().arr(i) + delay, trip.time().dep(i) + delay,
                        trip.stop(i));
  return delayedTrip;
}


// Implementation follows "http://stackoverflow.com/questions/48087/select-a-
// random-n-elements-from-listt-in-c-sharp"
set<size_t> ScenarioGenerator::selectNRandomIndices(int size, int N,
                                                    RandomFloatGen* random) {
  assert(size >= 0);
  assert(N >= 0);
  set<size_t> selected;
  for (int i = 0; i < size; ++i) {
    float threshold = (N - selected.size()) / static_cast<float>(size - i);
    if (random->next() < threshold) {
      selected.insert(i);
    }
  }
  assert(selected.size() == static_cast<size_t>(N));
  return selected;
}

const TransitNetwork& ScenarioGenerator::generatedNetwork() const {
  return _lastGeneratedTN;
}

const vector<Line>& ScenarioGenerator::generatedLines() const {
  return _lastGeneratedLines;
}

const vector<ScenarioParams>& ScenarioGenerator::params() const {
  return _params;
}

bool ScenarioGenerator::validParams() const {
  bool valid = _params.size();
  int sum = 0;
  for (auto it = _params.cbegin(); it != _params.end(); ++it) {
    if (!it->valid()) {
      LOG.error("Invalid generation parameters %s", it->str().c_str());
      valid = false;
    }
    sum += it->delayPercentage;
  }
  if (sum > 100) {
    LOG.error("ScenarioParameter percentage sums up to more than 100%%: %i%%",
              sum);
    valid = false;
  }
  return valid;
}
