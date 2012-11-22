// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#ifndef SRC_GTFSPARSER_H_
#define SRC_GTFSPARSER_H_

#include <boost/tuple/tuple.hpp>
#include <google/dense_hash_set>
#include <google/dense_hash_map>
#include <bitset>
#include <map>
#include <string>
#include <vector>
#include "./CsvParser.h"
#include "./Logger.h"


using std::string;
using std::vector;
using std::map;
using google::dense_hash_set;
using google::dense_hash_map;

class Line;
class Trip;
class TransitNetwork;


class GtfsParser {
 public:
  // Forward declaration for private data types.
  class Frequency;
  class Data;
  // Map for frequencies.
  typedef dense_hash_map<string, vector<Frequency> > FrequencyMap;
  // A tuple of weekday activity as bitset, start time and end time.
  typedef boost::tuple<std::bitset<7>, int, int> Activity;
  // A mapping indicating service activity for each weekday.
  typedef dense_hash_map<string, Activity> ActivityMap;

  // Constructor
  explicit GtfsParser(Logger* log = NULL);
  // Null assignment operator (prohibits assignment)
  GtfsParser& operator=(const GtfsParser& other);
  // Destructor
  ~GtfsParser();

  string parseName(const string& gtfsDir);
  // Formats the name of a network from its location and the time period.
  string parseName(const vector<string>& gtfsDirs,
                 const string& startTimeStr, const string& endTimeStr,
                 TransitNetwork* network = NULL);

  // Create the transit network from the GTFS files in 'gtfsDirectory'.
  TransitNetwork createTransitNetwork(const string& gtfsDir,
                                      const string& startTimeStr,
                                      const string& endTimeStr,
                                      vector<Line>* lines = NULL);
  // Create the transit network from the GTFS files (!) in each directory.
  TransitNetwork createTransitNetwork(const vector<string>& gtfsDirs,
                                      const string& startTimeStr,
                                      const string& endTimeStr,
                                      vector<Line>* lines = NULL);

  // Reads the Gtfs files in a directory.
  void parseGtfs(const string& gtfsDirectory, TransitNetwork* network);

  // Creates the nodes and arcs for each trip using the last read gtfs data.
  // Optionally transforms the gtfs trips into dc-trips by argument 'trips'.
  void
  translateLastTripsToNetwork(const string& startTimeStr,
                              const string& endTimeStr, TransitNetwork* network,
                              vector<Trip>* trips = NULL);

  // Removes all arcs starting at transit nodes. Returns the number of deleted.
  int removeInterTripArcs(TransitNetwork* network) const;

  // Serializes transit network to a binary file.
  bool save(const TransitNetwork& network, const string& filename);
  // Deserializes transit network from binary file. Returns false if not exists.
  bool load(const string& filename, TransitNetwork* network);

  // Set the logger to an external logger.
  void logger(Logger* const log);

  // Accesses the private data.
  const GtfsParser::Data& data() const;

 private:
  // Check whether a service is active on a certain day.
  bool isActive(const string& serviceId, const ActivityMap& activityMap,
                const boost::gregorian::date& day);
  FRIEND_TEST(GtfsParserTest, isActive);

  // Generates the vector of starting times of a trip using frequencies.txt's
  // content.
  vector<int> generateStartTimes(const string& tripId,
                                 const FrequencyMap& frequencies) const;

  // Adds a trip for a trip of stop_times.txt converting its relative time
  // stamps to absolute times.
  void addTrip(const Trip& block, const int timeOffset,
               vector<Trip>* trips) const;

  // Generates arrival, departure and transfer node for each stop of a trip
  // (which is given as a block of stop_times.txt)
  void generateTripNodes(const Trip& trip, const FrequencyMap& frequencies,
                         const int timeOffset, TransitNetwork* network) const;

  // Sorts the nodes for each stop by time, add waiting arcs between transit
  // nodes, and boarding arcs between transit and departure nodes.
  void generateInterTripArcs(TransitNetwork* network) const;

  // Returns a field name -> column index map of given CSV file.
  map<string, int> parseFields(const string& filename) const;

  // Parses the GTFS calendar file and create a mapping from service id to a
  // bitset describing the active days and two integers for start and end of
  // the service period.
  GtfsParser::ActivityMap parseCalendarFile(const string& filename);
  FRIEND_TEST(GtfsParserTest, calendar_txt);

  // Parses the GTFS trips file. Returns a map from trip_id to service_id.
  map<string, string> parseTripsFile(const string& filename);
  FRIEND_TEST(GtfsParserTest, trips_txt);

  // Parses the GTFS stops file.
  void parseStopsFile(const string& filename, TransitNetwork* network);
  FRIEND_TEST(GtfsParserTest, stops_txt);

  // Parses the GTFS frequencies file.
  FrequencyMap parseFrequenciesFile(const string& filename);
  FRIEND_TEST(GtfsParserTest, frequencies_txt);

  // Parses the GTFS stop-times file.
  vector<Trip> parseStopTimesFile(const string& filename,
                                  const TransitNetwork& network);
  FRIEND_TEST(GtfsParserTest, stop_times_txt);

  // Adds the data from one line of the stop-times file to a vector of stops.
  void addStopToTrip(const string& arrTimeStr, const string& depTimeStr,
                     const int stopIndex, Trip* trip);

  // Converts a string of format "" into the number of seconds from midnight.
  int gtfsTimeStr2Sec(const string& timesStr);
  FRIEND_TEST(GtfsParserTest, timeStringToSeconds);

  // Checks, whether two time stamps define a valid time period.
  static bool isValidTimePeriod(const string& startTimeStr,
                                const string& endTimeStr);
  FRIEND_TEST(GtfsParserTest, isValidTimePeriod);

  Data* _data;
  Logger* _log;
  friend class ScenarioGenerator;
};

#endif  // SRC_GTFSPARSER_H_
