// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include "./GtfsParser_impl.h"
#include <assert.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include "./Line.h"
#include "./TransitNetwork.h"
#include "./Utilities.h"

using std::sort;
using std::ifstream;
using std::ofstream;
using boost::posix_time::from_iso_string;
using boost::gregorian::date;
using boost::gregorian::day_iterator;


GtfsParser::GtfsParser(Logger* log) : _data(new Data()), _log(log) {}


GtfsParser::~GtfsParser() {
  delete _data;
}


string GtfsParser::parseName(const string& gtfsDir) {
  string name = gtfsDir;
  // remove trailing spaces
  while (name.rfind("/") == name.size() - 1)
    name = name.substr(0, name.size() - 1);
  // get the network's name
  name = name.substr(name.rfind("/") + 1);
  return name;
}


string GtfsParser::parseName(const vector<string>& gtfsDirs,
                             const string& startTimeStr,
                             const string& endTimeStr,
                             TransitNetwork* network) {
  assert(gtfsDirs.size());
  // get the base name
  string name = parseName(gtfsDirs[0]);
  // mark it as combined dataset if so
  if (gtfsDirs.size() > 1)
    name = "combined_" + name + "_etc";
  // add the timestamps
  name += "_" + startTimeStr.substr(0, 8) + "_" + endTimeStr.substr(0, 8);
  if (network) network->_name = name;
  return name;
}


TransitNetwork
GtfsParser::createTransitNetwork(const string& gtfsDir,
                                 const string& startTimeStr,
                                 const string& endTimeStr,
                                 vector<Line>* lines) {
  vector<string> dirs = {gtfsDir};
  TransitNetwork network =
      createTransitNetwork(dirs, startTimeStr, endTimeStr, lines);
  return network;
}


TransitNetwork
GtfsParser::createTransitNetwork(const vector<string>& gtfsDirs,
                                 const string& startTimeStr,
                                 const string& endTimeStr,
                                 vector<Line>* lines) {
  TransitNetwork network;
  if (!isValidTimePeriod(startTimeStr, endTimeStr)) {
    if (_log) _log->error("no valid time period specified");
    return network;
  }
  parseName(gtfsDirs, startTimeStr, endTimeStr, &network);

  // Parse the files in all the Gtfs directories.
  vector<Trip> trips;
  for (auto it = gtfsDirs.cbegin(); it != gtfsDirs.end(); ++it) {
    parseGtfs(*it, &network);
    translateLastTripsToNetwork(startTimeStr, endTimeStr, &network, &trips);
  }

  // Create the direct connected Lines.
  LineFactory lf;
  if (lines)
    *lines = lf.createLines(trips);

  // Sort the nodes of each stop by time and generate the arcs between them.
  generateInterTripArcs(&network);

  if (_log) {
    _log->info("constructed TransitNetwork with %d stops, %d nodes and %d arcs",
               network.numStops(), network.numNodes(), network._numArcs);
    if (lines) _log->info("created %d lines with %d trips in total",
                          lines->size(), trips.size());
  }
  return network;
}


void
GtfsParser::parseGtfs(const string& gtfsDirectory, TransitNetwork* network) {
  if (_log) _log->info("parsing GTFS files from " + gtfsDirectory);

  // Collect the set of serviceIds active the current day.
  _data->lastServiceActivity =
      parseCalendarFile(gtfsDirectory + "/calendar.txt");

  // Collect the association between trip_id and service_id.
  _data->lastTrip2Service = parseTripsFile(gtfsDirectory + "/trips.txt");

  // Parse all stops
  parseStopsFile(gtfsDirectory + "/stops.txt", network);

  // Read the frequency of each trip
  _data->lastFrequencies =
      parseFrequenciesFile(gtfsDirectory + "/frequencies.txt");

  // Parse the trips of stop_times.txt, each as a vector of StopBlockElem.
  _data->lastGtfsTrips =
      parseStopTimesFile(gtfsDirectory + "/stop_times.txt", *network);
}


void GtfsParser::translateLastTripsToNetwork(const string& startTimeStr,
                                             const string& endTimeStr,
                                             TransitNetwork* network,
                                             vector<Trip>* trips) {
  if (_log)
    _log->info("constructing the TransitNetwork for time period from %s to %s",
               startTimeStr.c_str(), endTimeStr.c_str());
  const ActivityMap& serviceActivity = _data->lastServiceActivity;
  const map<string, string>& trip2Service = _data->lastTrip2Service;
  const FrequencyMap& frequencies = _data->lastFrequencies;
  const vector<Trip>& gtfsTrips   = _data->lastGtfsTrips;

  // Process all the data read: Iterate over the the date period
  date start = from_iso_string(startTimeStr).date();
  date end   = from_iso_string(endTimeStr).date();
  for (day_iterator dayIt = day_iterator(start); dayIt <= end; ++dayIt) {
    const int t0 = 24 * 60 * 60 * (*dayIt - date(1970, 1, 1)).days();
    // Process each block of stops from stop_times.txt
    for (auto iter = gtfsTrips.cbegin(); iter != gtfsTrips.end(); ++iter) {
      const Trip& trip = *iter;
      if (trip.size() > 1) {
        // Skip trips that are not active today
        if (isActive(trip2Service.at(trip.id()), serviceActivity, *dayIt)) {
          // Gernerate all arrival, transfer and departure nodes for this trip
          generateTripNodes(trip, frequencies, t0, network);
          // Collect direct connection data (same trip may go at multiple days)
          if (trips) addTrip(trip, t0, trips);
        }
      }
    }
  }
}


bool GtfsParser::isActive(const string& serviceId,
                          const ActivityMap& activityMap,
                          const date& day) {
  const Activity& activity = activityMap.find(serviceId)->second;
  const int serviceDay   = convert<int>(to_iso_string(day));
  const int serviceStart = activity.get<1>();
  const int serviceEnd   = activity.get<2>();
  // day of week 0 = sunday, 6 = saturday
  int weekday = day.day_of_week().as_number();
  // 0 = monday, 6 = sunday
  weekday -= 1;
  if (weekday == -1) weekday = 6;
  assert(weekday >= 0 && weekday < 7);
  return serviceStart <= serviceDay &&
         serviceDay <= serviceEnd &&
         activity.get<0>()[weekday];
}

vector<int>
GtfsParser::generateStartTimes(const string& tripId,
                               const FrequencyMap& frequencies) const {
  vector<int> tripStartTimes;
  FrequencyMap::const_iterator it = frequencies.find(tripId);
  if (it != frequencies.end()) {
    for (size_t i = 0; i < it->second.size(); i++) {
      const Frequency& f = it->second[i];
      for (int time = f.start; time < f.finish; time += f.frequency) {
        tripStartTimes.push_back(time);
      }
    }
  } else {
    tripStartTimes.push_back(0);
  }
  return tripStartTimes;
}


void GtfsParser::addTrip(const Trip& trip, const int timeOffset,
                         vector<Trip>* trips) const {
  assert(trips);
  LineFactory lf;
  vector<Int64Pair> times;
  vector<int> stops;
  for (int line = 0; line < trip.size(); ++line) {
    times.push_back(make_pair(trip.time().arr(line) + timeOffset,
                              trip.time().dep(line) + timeOffset));
    stops.push_back(trip.stop(line));
  }
  trips->push_back(lf.createTrip(times, stops));
}


void GtfsParser::generateTripNodes(const Trip& trip,
                                   const FrequencyMap& frequencies,
                                   const int timeOffset,
                                   TransitNetwork* network) const {
  const TripTime& times = trip.time();
//   int firstArrivalTime = times.arr(0);

  // If a trip has a frequency we make the absolute times from
  // stop_times.txt relative to the first arrival time. If a trip has no
  // frequency we take the absolute time stamp.
  vector<int> tripStartTimes = generateStartTimes(trip.id(), frequencies);

  for (size_t i = 0; i < tripStartTimes.size(); i++) {
    const int tripStartTime = tripStartTimes[i];
    // Remember departure node index from the previous stop
    size_t prevDepartureIndex = 0;
    // add nodes for each pair of arrival and departure in the stop block
    for (int j = 0; j < trip.size(); j++) {
      int stopIndex   = trip.stop(j);
      int arrival     = times.arr(j);
      int departure   = times.dep(j);
      int waitingTime = departure - arrival;
      assert(waitingTime >= 0);
      // Make the times relative if we have a frequency
      if (contains(frequencies, trip.id())) {
        arrival   += tripStartTime/* - firstArrivalTime*/;
        departure += tripStartTime/* - firstArrivalTime*/;
      }
      // Add the offset for the current day
      arrival   += timeOffset;
      departure += timeOffset;
      // add the current arrival node
      int arrivalIndex = network->addTransitNode(stopIndex, Node::ARRIVAL,
                                                 arrival);
      // add the travel arc from departure@LastStop -> arrival@CurrentStop
      if (j > 0) {
        int deltaT  = times.arr(j) - times.dep(j-1);
        network->addArc(prevDepartureIndex, arrivalIndex, deltaT);
        assert(network->node(prevDepartureIndex).stop() !=
               network->node(arrivalIndex).stop());
      }

      // add the current departure node
      int departureIndex = network->addTransitNode(stopIndex, Node::DEPARTURE,
                                                   departure);
      // add the waiting arc at from arrival@T to departure@T
      network->addArc(arrivalIndex, departureIndex, waitingTime);

      // add the current transfer node
      int transferIndex = network->addTransitNode(stopIndex, Node::TRANSFER,
                                     arrival + TransitNetwork::TRANSFER_BUFFER);
      // add the transfer arc from arrival@T to transfer@T with PENALTY of 1
      network->addArc(arrivalIndex, transferIndex,
                      TransitNetwork::TRANSFER_BUFFER, 1);

      prevDepartureIndex = departureIndex;
    }
  }
}


void GtfsParser::generateInterTripArcs(TransitNetwork* network) const {
  const vector<Node>& nodes = network->_nodes;
  for (size_t i = 0; i < network->numStops(); i++) {
    Stop& stop = network->stop(i);
    // sort the vector of node indices according to the time of the node
    sort(stop.nodesBegin(), stop.nodesEnd(), CompareNodesByTime(&nodes));
    // for each transfer node, we add arcs to all subsequent departure nodes
    // until the next transfer node, to which we add an arc as well
    for (int k = 0; k < stop.numNodes(); k++) {
      int currNodeIndex = stop.nodeIndex(k);
      if (nodes[currNodeIndex].type() == Node::TRANSFER) {
        for (int j = k + 1; j < stop.numNodes(); j++) {
          int nextNodeIndex = stop.nodeIndex(j);
          int waitTime = nodes[nextNodeIndex].time() -
                         nodes[currNodeIndex].time();
          assert(waitTime >= 0);
          if (nodes[nextNodeIndex].type() == Node::DEPARTURE) {
            network->addArc(currNodeIndex, nextNodeIndex, waitTime);
          } else if (nodes[nextNodeIndex].type() == Node::TRANSFER) {
            network->addArc(currNodeIndex, nextNodeIndex, waitTime);
            break;
          }
        }
      }
    }
  }
}


int GtfsParser::removeInterTripArcs(TransitNetwork* network) const {
  int removed = 0;
  for (size_t i = 0; i < network->_nodes.size(); ++i) {
    if (network->_nodes[i].type() == Node::TRANSFER) {
      removed += network->_adjacencyLists[i].size();
      network->_adjacencyLists[i] = vector<Arc>();
    }
  }
  network->_numArcs -= removed;
  return removed;
}


map<string, int> GtfsParser::parseFields(const string& filename) const {
  CsvParser parser;
  parser.openFile(filename);
  map<string, int> field_map;
  for (size_t i = 0; i < parser.getNumColumns(); ++i) {
    field_map[parser.getItem(i)] = i;
  }
  parser.closeFile();
  return field_map;
}


GtfsParser::ActivityMap GtfsParser::parseCalendarFile(const string& filename) {
  ActivityMap serviceIds;
  serviceIds.set_empty_key("");
  map<string, int> field_map = parseFields(filename);
  CsvParser parser;

  vector<int> dayIndices;
  dayIndices.push_back(field_map["monday"]);
  dayIndices.push_back(field_map["tuesday"]);
  dayIndices.push_back(field_map["wednesday"]);
  dayIndices.push_back(field_map["thursday"]);
  dayIndices.push_back(field_map["friday"]);
  dayIndices.push_back(field_map["saturday"]);
  dayIndices.push_back(field_map["sunday"]);
  assert(dayIndices.size() == 7);

  parser.openFile(filename);
  while (not parser.eof()) {
    parser.readNextLine();
    if (parser.getNumColumns() > 0) {
      string serviceId = parser.getItem(field_map["service_id"]);
      int startDate = convert<int>(parser.getItem(field_map["start_date"]));
      int endDate = convert<int>(parser.getItem(field_map["end_date"]));
      Activity activity(0, startDate, endDate);
      for (size_t i = 0; i < 7; ++i)
        activity.get<0>()[i] = convert<bool>(parser.getItem(dayIndices[i]));
      serviceIds[serviceId] = activity;
    }
  }
  parser.closeFile();
  return serviceIds;
}


map<string, string> GtfsParser::parseTripsFile(const string& filename) {
  map<string, string> tripsAndService;
  map<string, int> field_map = parseFields(filename);
  CsvParser parser;
  parser.openFile(filename);
  while (not parser.eof()) {
    parser.readNextLine();
    if (parser.getNumColumns() > 0) {
      const string tripId    = parser.getItem(field_map["trip_id"]);
      const string serviceId = parser.getItem(field_map["service_id"]);
      assert(!contains(tripsAndService, tripId));
      tripsAndService[tripId] = serviceId;
    }
  }
  parser.closeFile();
  return tripsAndService;
}


void GtfsParser::parseStopsFile(const string& filename,
                                TransitNetwork* network) {
  // map row indices from the header of the file
  map<string, int> field_map = parseFields(filename);
  const int stop_id_index = field_map["stop_id"];
  const int stop_name_index = field_map["stop_name"];
  const int stop_lat_index = field_map["stop_lat"];
  const int stop_lon_index = field_map["stop_lon"];

  CsvParser parser;
  parser.openFile(filename);
  while (not parser.eof()) {
    parser.readNextLine();
    if (parser.getNumColumns() > 0) {
      Stop s = Stop(parser.getItem(stop_id_index),
                    parser.getItem(stop_name_index),
                    convert<float>(parser.getItem(stop_lat_index)),
                    convert<float>(parser.getItem(stop_lon_index)));
      // Just add stops not contained in the network.
      const int stopIndex = network->stopIndex(s.id());
      if (stopIndex == -1) {
        network->addStop(s);
      } else {
        if (s == network->stop(stopIndex)) {
        } else {
          std::stringstream ss;
          ss << "old: " << network->stop(stopIndex) << std::endl
             << "new: " << s;
          _log->info("Stop id already in network but data differs:\n%s",
                     ss.str().c_str());
        }
//      assert(s == network->stop(stopIndex) &&
//           "Error parsing: Stop-ID already in network but stop data differs");
      }
    }
  }
  parser.closeFile();
}


GtfsParser::FrequencyMap
GtfsParser::parseFrequenciesFile(const string& filename) {
  FrequencyMap frequencies;
  frequencies.set_empty_key("");
  // Check if the optional frequencies.txt exists
  std::ifstream fstream;
  fstream.open(filename.c_str());
  if (!fstream.good()) {
    if (_log) _log->info("no frequencies.txt found");
    return frequencies;
  }
  // TODO(jonas): Support frequencies. Currently it causes a problem with
  // generation of scenarios, because later in the parser the 'firstArrivalTime'
  // is considered if there are frequencies. So for now, we don't us them.
  if (_log) _log->error("frequencies.txt found -- NOT SUPPORTED!");
  return frequencies;

  // map row indices from the header of the file
  map<string, int> field_map = parseFields(filename);
  const int trip_id_index      = field_map["trip_id"];
  const int start_time_index   = field_map["start_time"];
  const int end_time_index     = field_map["end_time"];
  const int headway_secs_index = field_map["headway_secs"];

  CsvParser parser;
  parser.openFile(filename);
  while (not parser.eof()) {
    parser.readNextLine();
    if (parser.getNumColumns() > 0) {
      const string tripId = parser.getItem(trip_id_index);
      const int startTime = gtfsTimeStr2Sec(parser.getItem(start_time_index));
      const int endTime   = gtfsTimeStr2Sec(parser.getItem(end_time_index));
      const int headwaySecs = convert<int>(parser.getItem(headway_secs_index));
      // frequency is a map from <string> to <pointer to vector>
      if (!contains(frequencies, tripId))
        frequencies[tripId] = vector<Frequency>();
      frequencies[tripId].push_back(Frequency(startTime, endTime, headwaySecs));
    }
  }
  parser.closeFile();
  return frequencies;
}


vector<Trip> GtfsParser::parseStopTimesFile(const string& filename,
                                            const TransitNetwork& network) {
  // map row indices from the header of the file
  map<string, int> field_map = parseFields(filename);
  const int trip_id_index        = field_map["trip_id"];
  const int arrival_time_index   = field_map["arrival_time"];
  const int departure_time_index = field_map["departure_time"];
  const int stop_id_index        = field_map["stop_id"];

  vector<Trip> trips;
  CsvParser parser;
  parser.openFile(filename);
  parser.readNextLine();

  while (!parser.eof()) {
    if (parser.getNumColumns() == 0)
      continue;
    // collect a block from stop_times.txt, reduce times to time differences
    Trip trip(parser.getItem(trip_id_index));
    addStopToTrip(parser.getItem(arrival_time_index),
                  parser.getItem(departure_time_index),
                  network.stopIndex(parser.getItem(stop_id_index)), &trip);
    parser.readNextLine();
    while (!parser.eof() && parser.getItem(trip_id_index) == trip.id()) {
      if (parser.getNumColumns() > 0) {
        const string stopIndexStr = parser.getItem(stop_id_index);
        if (network.stopIndex(stopIndexStr) != trip.stops().back()) {
          addStopToTrip(parser.getItem(arrival_time_index),
                        parser.getItem(departure_time_index),
                        network.stopIndex(stopIndexStr), &trip);
        } else {
          // update the departure time of the last stop
          int departure = gtfsTimeStr2Sec(parser.getItem(departure_time_index));
          trip.tripTime().back().first = departure;
        }
      }
      parser.readNextLine();
    }
    trips.push_back(trip);
  }
  parser.closeFile();

  return trips;
}


inline void
GtfsParser::addStopToTrip(const string& arrTimeStr, const string& depTimeStr,
                          const int stopIndex, Trip* trip) {
  assert(trip);
  // If no arrival or departure time is given, take the one from the preceding
  // stop. See: http://code.google.com/intl/de-DE/transit/spec/transit_ ...
  // feed_specification.html#stop_times_txt___Field_Definitions
  int arrival, departure;
  if (arrTimeStr == "")
    arrival = trip->time().arr(trip->time().size() - 1);
  else
    arrival = gtfsTimeStr2Sec(arrTimeStr);
  if (depTimeStr == "")
    departure = trip->time().dep(trip->time().size() - 1);
  else
    departure = gtfsTimeStr2Sec(depTimeStr);
  trip->addStop(arrival, departure, stopIndex);
}


int GtfsParser::gtfsTimeStr2Sec(const string& timesStr) {
  string timeString = timesStr;

  // Check that the string contains ":"
  if (timeString.find(":") == string::npos) {
    return INT_MAX;
  }

  // Remove the quotes if they are present
  if (timeString.at(0) == '"')
    timeString = timeString.substr(1);
  if (timeString.at(timeString.length() - 1) == '"')
    timeString = timeString.substr(0, timeString.length() - 1);

  char* p;
  char* cStr = strtok_r(const_cast<char*>(timeString.c_str()), ":", &p);
  int seconds = 0;
  for (int i = 0; cStr != NULL; i++) {
    if (i == 0)  // hours
      seconds += atoi(cStr) * 3600;
    else if (i == 1)  // minutes
      seconds += atoi(cStr) * 60;
    else if (i == 2)  // seconds
      seconds += atoi(cStr);
    cStr = strtok_r(NULL, ":", &p);
  }
  return seconds;
}


bool GtfsParser::save(const TransitNetwork& network, const string& filename) {
  ofstream ofs(filename.c_str());
  if (!ofs.good()) {
    if (_log)
      _log->error("GtfsParser::save(): File '%s' could not be created.",
                  filename.c_str());
    return false;
  }
  int perf_id = 0;
  if (_log) {
    perf_id = _log->beginPerf();
    _log->info("serializing TransitNetwork to binary file '%s'",
                filename.c_str());
  }
  boost::archive::binary_oarchive oa(ofs);
  oa << network;
  // workaround for dense_hash_map: extract all <key,value> pairs
  vector<string> tmp_stop_ids;
  vector<int> tmp_indices;
  for (auto it = network._stopId2indexMap.begin();
       it != network._stopId2indexMap.end();
       ++it) {
    tmp_stop_ids.push_back(it->first);
    tmp_indices.push_back(it->second);
  }
  oa << tmp_stop_ids;
  oa << tmp_indices;

  if (_log)  _log->endPerf(perf_id, "GtfsParser::save()");
  return true;
}


bool GtfsParser::load(const string& filename, TransitNetwork* network) {
  ifstream ifs(filename.c_str());
  if (!ifs.good()) {
    if (_log)
      _log->error("GtfsParser::load(): File '%s' could not be read.",
                  filename.c_str());
    return false;
  }
  const int perf_id = _log->beginPerf();
  if (_log)
    _log->info("deserializing saved TransitNetwork from binary file '%s'",
                filename.c_str());
  boost::archive::binary_iarchive ia(ifs);
  ia >> *network;
  // workaround for dense_hash_map
  vector<string> tmp_stop_ids;
  vector<int> tmp_indices;
  ia >> tmp_stop_ids;
  ia >> tmp_indices;
  assert(tmp_stop_ids.size() == tmp_indices.size());
  for (size_t i = 0; i < tmp_stop_ids.size(); i++)
    network->_stopId2indexMap[tmp_stop_ids[i]] = tmp_indices[i];
  // Reconstruct the Kdtree. Cannot be (de)serialized.
  network->preprocess();
  if (_log)
    _log->endPerf(perf_id, "GtfsParser::load()");
  return true;
}


bool GtfsParser::isValidTimePeriod(const string& startTimeStr,
                                   const string& endTimeStr) {
  if (!isValidTimeString(startTimeStr) || !isValidTimeString(endTimeStr))
    return false;
  int deltaT = str2time(endTimeStr) - str2time(startTimeStr);
  if (deltaT <= 0) {
    return false;
  }
  return true;
}


void GtfsParser::logger(Logger* const log) {
  if (log) {
    _log = log;
  }
}


const GtfsParser::Data& GtfsParser::data() const {
  return *_data;
}

