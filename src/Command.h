// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#ifndef SRC_COMMAND_H_
#define SRC_COMMAND_H_

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <map>
#include <sstream>
#include "./Logger.h"
#include "./Server.h"
#include "./Statistics.h"

using std::string;
using std::map;
using boost::asio::ip::tcp;
using boost::shared_ptr;

struct Query {
  int dep;
  int dest;
  string time;
};

class Command {
 public:
  virtual ~Command() {}
  virtual vector<string> operator()(Server& server,  // NOLINT
                                    const StrStrMap& args,
                                    Logger& log) = 0;
  static void execute(Server& server,  // NOLINT
                      shared_ptr<tcp::socket> socket, // NOLINT
                      const string& com, const StrStrMap& args,
                      Logger& log);
  static void send(shared_ptr<tcp::socket> socket,
                   const string& msg, Logger& log);
  // Returns a vector of random queries. Dep and destStops are ranging from zero
  // to numStops, times from 0:00:00 to 23:59:00 at 1st of may 2012.
  static vector<Query> getRandQueries(int numQueries, int numStops, int seed);

  // Computes the shortest path between dep and dest stop @ time
  static void dijkstraQuery(const TransitNetwork& network, const HubSet* hubs,
                           const int dep, const int time, const int dest,
                           QueryResult* resultPtr);
};

class WebCommand : public Command {
  vector<string> operator()(Server& server,  // NOLINT
                            const StrStrMap& args, Logger& log);
  string contentType(const string& doc) const;
};

class SelectStop : public Command {
  vector<string> operator()(Server& server,  // NOLINT
                            const StrStrMap& args, Logger& log);
};

class SelectStopById : public Command {
  vector<string> operator()(Server& server,  // NOLINT
                            const StrStrMap& args, Logger& log);
};

class LoadNetwork : public Command {
  vector<string> operator()(Server& server,  // NOLINT
                            const StrStrMap& args, Logger& log);
};

class ListNetworks : public Command {
  vector<string> operator()(Server& server,  // NOLINT
                            const StrStrMap& args, Logger& log);
};

class FindRoute : public Command {
  vector<string> operator()(Server& server,  // NOLINT
                            const StrStrMap& args, Logger& log);
};

class Test : public Command {
  vector<string> operator()(Server& server,  // NOLINT
                            const StrStrMap& args, Logger& serverLog);

  boost::shared_mutex _mutex;
  int _numPathsDi;
  int _numPathsTp;
  int _numReachedDi;
  int _numReachedTp;
  int _numSubset;
  int _numAlmostSubset;
  int _numFailed;
  int _numInvalid;
  int _numTpInvalid;
  Logger* _serverLog;
  Logger* _expLog;
  Server* _server;
};

class GenerateScenario : public Command {
  vector<string> operator()(Server& server,  // NOLINT
                            const StrStrMap& args, Logger& log);
};

class PlotSeedStops : public Command {
  vector<string> operator()(Server& server,  // NOLINT
                            const StrStrMap& args, Logger& log);
};

class ListHubs : public Command {
  vector<string> operator()(Server& server,  // NOLINT
                            const StrStrMap& args, Logger& log);
};

class GetGeoInfo : public Command {
  vector<string> operator()(Server& server,  // NOLINT
                            const StrStrMap& args, Logger& log);
};

class LabelStops : public Command {
  vector<string> operator()(Server& server,  // NOLINT
                            const StrStrMap& args, Logger& log);
};

#endif  // SRC_COMMAND_H_
