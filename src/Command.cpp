// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include "./Command.h"
#include <boost/asio.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <omp.h>
#include <cstdarg>
#include <cstring>
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <utility>
#include <fstream>
#include <numeric>
#include <map>
#include "./Utilities.h"
#include "./GtfsParser.h"
#include "./Dijkstra.h"
#include "./Random.h"
#include "./ScenarioGenerator.h"
// #include <google/profiler.h>

using std::string;
using std::ifstream;
using std::ostringstream;
using boost::asio::io_service;
using boost::system::error_code;
using boost::asio::ip::tcp;
using boost::thread;
using boost::function;
using boost::shared_mutex;
using std::accumulate;

shared_mutex networkMutex;
typedef boost::unique_lock<shared_mutex> WriteLock;
typedef boost::shared_lock<shared_mutex> ReadLock;
// boost::mutex networkWriteMutex;

void Command::execute(Server& server,  // NOLINT
                      shared_ptr<tcp::socket> socket,
                      const string& com, const StrStrMap& args, Logger& log) {
  Command* command = NULL;
  if (com == "web") command = new WebCommand();
  else if (com == "select") command = new SelectStop();
  else if (com == "route") command = new FindRoute();
  else if (com == "listnetworks") command = new ListNetworks();
  else if (com == "loadnetwork") command = new LoadNetwork();
  else if (com == "test") command = new Test();
  else if (com == "plotseeds") command = new PlotSeedStops();
  else if (com == "listhubs") command = new ListHubs();
  else if (com == "selectbyid") command = new SelectStopById();
  else if (com == "generatescenario") command = new GenerateScenario();
  else if (com == "geoinfo") command = new GetGeoInfo();
  else if (com == "label") command = new LabelStops();
  if (command) {
    vector<string> msgs = (*command)(server, args, log);
    delete command;
    BOOST_FOREACH(const string& msg, msgs) {
      Command::send(socket, msg, log);
    }
  }
}

void Command::send(shared_ptr<tcp::socket> socket, const string& msg,
                   Logger& log) {
  if (msg.size()) {
    error_code error;
    size_t sent = boost::asio::write(*socket, boost::asio::buffer(msg),
                                     boost::asio::transfer_all(), error);
    if (sent < msg.size()) {
      log.error("Sending error: " + error.message());
    }
  }
}

vector<Query> Command::getRandQueries(int numQueries, int numStops, int seed) {
  vector<Query> queries;
  RandomGen randGen(0, numStops, seed);
  RandomGen randHour(6, 19, seed);
  RandomGen randMin(0, 59, seed);

  for (int i = 0; i < numQueries; ++i) {
    int depIndex = randGen.next();
    int destIndex = randGen.next();
    while (depIndex == destIndex) {
      destIndex = randGen.next();
    }

    stringstream ts;
    ts << "20120501T";

    int hour = randHour.next();
    if (hour < 10)
      ts << "0";
    ts << hour;

    int min = randMin.next();
    if (min < 10)
      ts << "0";
    ts << min << "00";

    Query q;
    q.dep = depIndex;
    q.dest = destIndex;
    q.time = ts.str();
    queries.push_back(q);
  }
  return queries;
}

void Command::dijkstraQuery(const TransitNetwork& network, const HubSet* hubs,
                           const int dep, const int time, const int dest,
                           QueryResult* resultPtr) {
  Dijkstra dijkstra(network);
  dijkstra.hubs(hubs);
  dijkstra.maxPenalty(3);
  dijkstra.maxHubPenalty(3);
  const Stop& depStop = network.stop(dep);
  const vector<int> depNodes = network.findStartNodeSequence(depStop, time);
  dijkstra.startTime(time);
  dijkstra.findShortestPath(depNodes, dest, resultPtr);
}

string WebCommand::contentType(const string& doc) const {
  string type_id = doc.substr(doc.find_last_of(".") + 1);
  string type = "text/html";
  if (type_id == "js") type = "application/javascript";
  else if (type_id == "css") type = "text/css";
  else if (type_id == "ico" || type_id == "png") type = "image/x-icon";
  return type;
}

vector<string> WebCommand::operator()(Server& server,  // NOLINT
                              const StrStrMap& args, Logger& log) {
  string doc;
  if (!found(args, string("doc"), doc) || doc == "") {
    return vector<string>();
  }
  string data = readFile(doc);
  ostringstream answer;
  string status = "200 OK";
  if (!data.size())
    status = "404 Not Found";
  answer << "HTTP/1.1 " + status
         << "\r\nContent-Length: " << data.size()
         << "\r\nContent-Type: " << contentType(doc)
         << "\r\nConnection: close\r\n\r\n"
         << data;
  return vector<string>(1, answer.str());
}

vector<string> SelectStop::operator()(Server& server,  // NOLINT
                              const StrStrMap& args, Logger& log) {
  ReadLock lock(networkMutex);
  float lat = 0.0f;
  float lon = 0.0f;
  if (!found(args, string("lat"), lat)
      || !found(args, string("lon"), lon)) {
    log.error("search error: position arguments not provided");
    return vector<string>();
  }
  int perf_id = log.beginPerf();
  const Stop* stop = server.network().findNearestStop(lat, lon);
  log.endPerf(perf_id, "findNearestStop");
  if (!stop) {
    log.error("no close stop found around (" + convert<string>(lat) + ", "
              + convert<string>(lon) + ")");
    return vector<string>();
  }
  log.info("closest stop to (" + convert<string>(lat) + ", "
           + convert<string>(lon) + ") is "
           + stop->name() + " (" + stop->id()
           + ") (" + convert<string>(stop->lat()) + ", "
           + convert<string>(stop->lon()) + ")");
  string data = "{\"name\":\"" + stop->name()
      + "\",\"id\":" + convert<string>(stop->index())
      + ",\"lat\":" + convert<string>(stop->lat())
      + ",\"lon\":" + convert<string>(stop->lon())
      + "}";
  ostringstream answer;
  answer << "HTTP/1.1 200 OK"
         << "\r\nContent-Length: " << data.size()
         << "\r\nContent-Type: application/json"
         << "\r\nConnection: close\r\n\r\n"
         << data;

  return vector<string>(1, answer.str());
}

vector<string> SelectStopById::operator()(Server& server,  // NOLINT
                                          const StrStrMap& args,
                                          Logger& log) {
  ReadLock lock(networkMutex);
  int id = -1;
  if (!found(args, string("id"), id)) {
    log.error("search error: position arguments not provided");
    return vector<string>();
  }
  if (id < 0 || id >= static_cast<int>(server.network().numStops())) {
    log.error("stop index out of range");
    return vector<string>();
  }
  const Stop& stop = server.network().stop(id);
  assert(stop.index() == id);
  string data = "{\"name\":\"" + stop.name()
      + "\",\"id\":" + convert<string>(stop.index())
      + ",\"lat\":" + convert<string>(stop.lat())
      + ",\"lon\":" + convert<string>(stop.lon())
      + "}";
  ostringstream answer;
  answer << "HTTP/1.1 200 OK"
         << "\r\nContent-Length: " << data.size()
         << "\r\nContent-Type: application/json"
         << "\r\nConnection: close\r\n\r\n"
         << data;
  return vector<string>(1, answer.str());
}

vector<string> LoadNetwork::operator()(Server& server,  // NOLINT
                               const StrStrMap& args, Logger& log) {
  WriteLock lock(networkMutex);
  string gtfs_path = "";
  int start_time = localTime();
  int end_time = localTime() + kSecondsPerDay * 1;
  if (!found(args, string("path"), gtfs_path)) {
    log.error("load error: some arguments not provided, "\
              "required are path, start_time, end_time");
    return vector<string>();
  }
  if (gtfs_path != server.network().name()) {
    string path = server.dataDir() + "/" + gtfs_path;
    const vector<string> dirs = listDirs(server.dataDir());
    if (find(dirs.begin(), dirs.end(), gtfs_path) == dirs.end()) {
      log.error("load error: path " + path + " not found");
      return vector<string>();
    }
    vector<string> paths = {path};
    server.loadGtfs(paths, start_time, end_time);
    server.network().name(gtfs_path);
  }
  string data = "\"network " + gtfs_path + " loaded\"";
  ostringstream answer;
  answer << "HTTP/1.1 200 OK"
         << "\r\nContent-Length: " << data.size()
         << "\r\nContent-Type: application/json"
         << "\r\nConnection: close\r\n\r\n"
         << data;
  return vector<string>(1, answer.str());
}

vector<string> ListNetworks::operator()(Server& server,  // NOLINT
                               const StrStrMap& args, Logger& log) {
  const vector<string> dirs = listDirs(server.dataDir());
  ostringstream data;
  data << "\"";
  for (vector<string>::const_iterator it = dirs.begin();
       it != dirs.end();
       ++it) {
    data << *it << " ";
  }
  data << "\"";
  ostringstream answer;
  answer << "HTTP/1.1 200 OK"
         << "\r\nContent-Length: " << data.str().size()
         << "\r\nContent-Type: application/json"
         << "\r\nConnection: close\r\n\r\n"
         << data.str();
  return vector<string>(1, answer.str());
}

vector<string> FindRoute::operator()(Server& server,  // NOLINT
                             const StrStrMap& args, Logger& log) {
  ReadLock lock(networkMutex);
  int dep = -1;
  int dest = -1;
  string depTime = "";
  bool useTransferPatterns = false;
  if (!found(args, string("from"), dep)
      || !found(args, string("to"), dest)
      || !found(args, string("at"), depTime)
      || !found(args, string("tp"), useTransferPatterns)) {
    log.error("find route error: arguments not provided");
    return vector<string>();
  }
  ostringstream data;
  data << "{\"id\":" << dest << ",\"labels\":[";

  if (useTransferPatterns) {
    assert(server.router().transferPatternsDB());
    if (server.router().transferPatternsDB()->numGraphs() == 0) {
      log.error("finding shortest path via transfer patterns failed");
      useTransferPatterns = false;
    } else {
      // string logStr = "";
      vector<QueryResult::Path> resultTP;
      resultTP = server.router().shortestPath(dep, str2time(depTime), dest
                                              /*, &logStr*/);
      // log.debug("TP Paths\n%s", logStr.c_str());
      // get paths and costs
      ostringstream pathData;
      int labelIndex = 0;
      for (auto it = resultTP.begin(); it != resultTP.end(); ++it) {
        LabelVec::Hnd pathcost = it->first;
        const vector<int>& pathvec = it->second;
        log.info("TP: Found path with cost %d and penalty %d",
                 pathcost.cost(), pathcost.penalty());

        if (it != resultTP.begin()) {
          data << ",";
        }
        data << "[" << convert<string>(static_cast<int>((it->first).cost()))
             << "," << convert<string>(static_cast<int>((it->first).penalty()))
             << "]";

        int lastStopIndex = -1;
        for (size_t i = 0; i < pathvec.size(); i++) {
          const Stop& stop = server.network().stop(pathvec[i]);
          if (stop.index() != lastStopIndex) {
            lastStopIndex = stop.index();
            if (pathData.str().size()) {
              pathData << ",";
            }
            pathData << "{\"id\":" << stop.index()
               << ",\"lat\":" << stop.lat()
               << ",\"lon\":" << stop.lon()
               << ",\"label\":" << labelIndex
               << "}";
          }
        }
        ++labelIndex;
      }
      data << "],\"stops\":[" << pathData.str()
           << "],\"tp\":" << static_cast<int>(useTransferPatterns) << "}";
    }
  }
  if (!useTransferPatterns) {
    QueryResult result;
    const TransitNetwork& network = server.scenarioSet() ? server.scenario()
                                                         : server.network();
    const HubSet* hubs = &server.router().hubs();
    dijkstraQuery(network, hubs, dep, str2time(depTime), dest, &result);
    ostringstream path;

    // debug
    // string logStr = "";
    // result.optimalPaths(network, &logStr);
    // log.debug("Dijkstra paths:\n%s\n", logStr.c_str());

    int labelIndex = 0;
    for (auto it = result.destLabels.begin();
         it != result.destLabels.end();
         ++it) {
      LabelVec::Hnd label = *it;
      log.info("DI: Found path with (%d,%d)", label.cost(), label.penalty());

      if (it != result.destLabels.begin())
        data << ",";
      data << "[" << convert<string>(label.cost())
           << "," << convert<string>(static_cast<int>(label.penalty())) << "]";

      int lastStopIndex = -1;
      while (label) {
        const Node& node = network.node(label.at());
        const Stop& stop = network.stop(node.stop());

        // Get path description for web interface:
        if (stop.index() != lastStopIndex) {
          lastStopIndex = stop.index();
          if (path.str().size())
            path << ",";

          path << "{\"id\":" << stop.index()
               << ",\"lat\":" << stop.lat()
               << ",\"lon\":" << stop.lon()
               << ",\"cost\":" << label.cost()
               << ",\"penalty\":" << static_cast<int>(label.penalty())
               << ",\"label\":" << labelIndex << "}";
        }
        label = result.matrix.parent(label);
      }
      ++labelIndex;
    }
    data << "],\"stops\":[" << path.str()
         << "],\"tp\":" << static_cast<int>(useTransferPatterns) << "}";
  }

  ostringstream answer;
  answer << "HTTP/1.1 200 OK"
         << "\r\nContent-Length: " << data.str().size()
         << "\r\nContent-Type: application/json"
         << "\r\nConnection: close\r\n\r\n"
         << data.str();
  return vector<string>(1, answer.str());
}


std::string labelsToString(const vector<QueryResult::Path>& paths) {
  std::stringstream ss;
  ss << "{";
  for (size_t i = 0; i < paths.size(); ++i) {
    ss << "(" << paths[i].first.cost() << ","
       << static_cast<int>(paths[i].first.penalty()) << ")";
    if (i < paths.size() - 1)
      ss << ",";
  }
  ss << "}";
  return ss.str();
}


vector<string> Test::operator()(Server& server,  // NOLINT
                                const StrStrMap& args,
                                Logger& serverLog) {
  ReadLock lock(networkMutex);
  int numTests = 0;
  bool useTransferPatterns = false;

  if (!found(args, string("num"), numTests)
      || !found(args, string("tp"), useTransferPatterns)) {
    serverLog.error("test error: arguments not provided");
    return vector<string>();
  }

  int seed = getSeed();
  found(args, string("seed"), seed);

  _numPathsDi = 0;
  _numPathsTp = 0;
  _numReachedDi = 0;
  _numReachedTp = 0;
  _numSubset = 0;
  _numAlmostSubset = 0;
  _numInvalid = 0;
  _numFailed = 0;
  _numTpInvalid = 0;

  const vector<Query> queries = getRandQueries(numTests,
                                         server.network().numStops() - 1, seed);
  const size_t nQueries = queries.size();
  assert(static_cast<int>(nQueries) == numTests);
  ostringstream logText;
  logText << numTests << " samples; " << seed << " seed; ";

  // Init Dijkstra with a specified scenario or the same network as patterns.
  const TransitNetwork& network = server.scenarioSet() ? server.scenario()
                                                       : server.network();
  // Use an extra logger for experiments
  Logger expLog;
  const int64_t expTime = localTime();
  expLog.target("log/experiments/"+ network.name() + "_"
                + time2str(expTime) + ".log");
  expLog.info("type,dep,dest,time,#DI,labelDI,#TP,labelTP,"
              "timeDI(ms),timeTP(ms),sizeQG,route");

  _serverLog = &serverLog;
  _expLog = &expLog;
  _server = &server;

  vector<vector<QueryResult::Path> > tpResults;
  tpResults.resize(nQueries);
  vector<double> secondsTP;
  secondsTP.resize(nQueries);
  vector<size_t> queryGraphSizes;
  queryGraphSizes.resize(nQueries);
  int progress = 0;
  for (size_t i = 0; i < nQueries; ++i) {
    const Query& query = queries[i];
    const int perfId = _serverLog->beginPerf();
    tpResults[i] = server.router().shortestPath(query.dep, str2time(query.time),
                                                query.dest);
    secondsTP[i] = _serverLog->endPerf(perfId);
    queryGraphSizes[i] =
        server.router().queryGraph(query.dep, query.dest).countArcs();
    ++progress;
    if ((progress + 1) % 10 == 0)
      serverLog.info("%i of %i TP queries done.", progress, nQueries);
  }

  // ProfilerStart("cpu_profile.bla");
  // Compare Results
  progress = 0;
  int numPathsDi, numReachedDi, numPathsTp, numReachedTp, numInvalid, numSubset,
      numAlmostSubset, numFailed, numTpInvalid;
  numPathsDi = numReachedDi = numPathsTp = numReachedTp = numInvalid =
      numSubset = numAlmostSubset = numFailed = numTpInvalid = 0;
  const int nThreads = server.maxWorkers() > omp_get_max_threads() ?
                       omp_get_max_threads() : server.maxWorkers();
  omp_set_num_threads(nThreads);
  #pragma omp parallel reduction(+:numPathsDi, numReachedDi, numPathsTp, \
                                  numReachedTp, numInvalid, numSubset, \
                                  numAlmostSubset, numFailed, numTpInvalid)
  {  // NOLINT
  numPathsDi = numReachedDi = numPathsTp = numReachedTp = numInvalid =
      numSubset = numAlmostSubset = numFailed = numTpInvalid = 0;
  Logger logger;
  const HubSet& hubs = server.router().hubs();
  #pragma omp for
  for (size_t i = 0; i < nQueries; ++i) {
    const Query& query = queries[i];
    QueryResult dijkstraResult;
    const int perfId = logger.beginPerf();
    Command::dijkstraQuery(network, &hubs, query.dep, str2time(query.time),
                           query.dest, &dijkstraResult);
    const double secondsDijkstra = logger.endPerf(perfId);

    numPathsDi += dijkstraResult.destLabels.size();
    numReachedDi += !!dijkstraResult.destLabels.size();
    numPathsTp += tpResults[i].size();
    numReachedTp += !!tpResults[i].size();

    vector<QueryResult::Path> dijkstraPaths =
        dijkstraResult.optimalPaths(network);
    QueryCompare comparator;
    comparator.hubs(&hubs);
    int queryType = comparator.compare(dijkstraPaths, tpResults[i]);

    if (queryType == 0)
      ++numInvalid;
    else if (queryType == 1)
      ++numSubset;
    else if (queryType == 2)
      ++numAlmostSubset;
    else if (queryType == 3)
      ++numFailed;
    else if (queryType == 4)
      ++numTpInvalid;

    stringstream queryStream;
    queryStream << "type" << queryType << ","
                << query.dep << ","
                << query.dest << ","
                << query.time << ","
                << dijkstraPaths.size() << ","
                << labelsToString(dijkstraPaths) << ","
                << tpResults[i].size() << ","
                << labelsToString(tpResults[i]) << ","
                << 1000.*secondsDijkstra << ","
                << 1000.*secondsTP[i] << ","
                << queryGraphSizes[i]
                << ","
                << "/route " << query.dep
                << " 01.05.2012 "
                << query.time.substr(9, 2) << ":"
                << query.time.substr(11, 2) << ":00"
                << " " << query.dest;
    const string queryString = queryStream.str();

    #pragma omp atomic
    ++progress;

    #pragma omp critical(experiment_log)
    {  // NOLINT
    expLog.info(queryString);
    if ((progress + 1) % 10 == 0)
      serverLog.info("%i of %i DI queries done.", progress, queries.size());
    }
  }
  }  // pragma omp parallel
  // ProfilerStop();
  _numPathsDi = numPathsDi;
  _numReachedDi = numReachedDi;
  _numPathsTp = numPathsTp;
  _numReachedTp = numReachedTp;
  _numInvalid = numInvalid;
  _numTpInvalid = numTpInvalid;
  _numSubset = numSubset;
  _numAlmostSubset = numAlmostSubset;
  _numFailed = numFailed;

  // Output in console and logger
  logText << "Dijkstra"
          << (server.scenarioSet() ? "on modified network: " : ": ")
          << _numReachedDi * 100 / numTests << "% reached, "
          << "[" << _numPathsDi << " | " << 1.0f * _numPathsDi / numTests << "]"
          << " paths found; "
          << "TP: " << _numReachedTp * 100 / numTests << "% reached, "
          << "[" << _numPathsTp << " | " << 1.0f * _numPathsTp / numTests << "]"
          << " paths found; "
          << "Empty queries: " << _numInvalid << "; "
          << "OK: " << _numSubset << "; "
          << "Almost OK: " << _numAlmostSubset << "; "
          << "Failed: " << _numFailed << "; "
          << "Long path without hub: " << _numTpInvalid << ";";
  serverLog.info(logText.str());
  Logger overview;
  overview.target("log/experiments/" + network.name() + "_" +
                  time2str(expTime) + ".overview");
  overview.info(logText.str());

  ostringstream data;
  data << "\"" << logText.str() << "\"";

  ostringstream answer;
  answer << "HTTP/1.1 200 OK"
         << "\r\nContent-Length: " << data.str().size()
         << "\r\nContent-Type: application/json"
         << "\r\nConnection: close\r\n\r\n"
         << data.str();

  return vector<string>(1, answer.str());
}


vector<string> GenerateScenario::operator()(Server& server,  // NOLINT
                                           const StrStrMap& args, Logger& log) {
  ReadLock lock(networkMutex);
  ostringstream data;
  ostringstream answer;

  int numParams = 0;
  if (!found(args, string("numparams"), numParams)) {
    log.error("generate scenario error: arguments not provided");
    return vector<string>();
  }

  vector<int> delayPercentages(numParams, 0);
  vector<int> delayMeans(numParams, 1);
  vector<ScenarioParams> params;
  for (int i = 0; i < numParams; ++i) {
    const string percentParam = "percent" + convert<string>(i);
    const string meanParam = "mean" + convert<string>(i);

    if (!found(args, percentParam, delayPercentages[i])
        || !found(args, meanParam, delayMeans[i])) {
      log.error("generate scenario error: arguments not provided");
      return vector<string>();
    }
    ScenarioParams param;
    param.delayPercentage = delayPercentages[i];
    param.delayMean = delayMeans[i];
    if (!param.valid()) {
      string str = "Error: ScenarioGenerator arguments invalid: " + param.str();
      log.error(str);
      data << "\" " << str << "\"";
    } else {
      params.push_back(param);
      log.debug(param.str());
    }
  }

  log.info("Generate new scenario on transit network: "
            + server.network().name());

  ScenarioGenerator generator(params);
  server.scenario(generator.gen(server.network().name()));
  server.router().prepare(generator.generatedLines());

  data << "\" Scenario loaded with ";
  for (size_t i = 0; i < params.size(); ++i) {
    data << params[i].delayPercentage << "% delayed trips with mean delay "
         << params[i].delayMean;
    if (i < params.size() - 1)
      data << ", ";
  }
  data << " on " << server.network().name() << "\"";

  answer << "HTTP/1.1 200 OK"
         << "\r\nContent-Length: " << data.str().size()
         << "\r\nContent-Type: application/json"
         << "\r\nConnection: close\r\n\r\n"
         << data.str();

  return vector<string>(1, answer.str());
}


vector<string> PlotSeedStops::operator()(Server& server,  // NOLINT
                             const StrStrMap& args, Logger& log) {
  const TransitNetwork& network = server.network();
  int numSeeds = -1;
  if (!found(args, string("seeds"), numSeeds)) {
    log.error("plot seeds error: arguments not provided");
    return vector<string>();
  }
  RandomFloatGen random(0.f, 1.f, localTime());
  vector<string> answers;
  string header = string("HTTP/1.1 200 OK\r\nContent-Type: application/json")
      + "\r\nConnection: close\r\n\r\n"
      + "{\"seeds\": [";
  answers.push_back(header);
  for (int i = 0; i < numSeeds; ++i) {
    const string id = network.stopTree().randomWalk(&random).stopPtr->id();
    const int index = network.stopIndex(id);
    const Stop& stop = network.stop(index);
    ostringstream data;
    if (i > 0) {
      data << ",";
    }
    data << "{\"id\":\"" << stop.id() << "\","
         << "\"name\":\"" << stop.name() << "\","
         << "\"lat\":" << stop.lat() << ",\"lon\":" << stop.lon()
         << "}";
    answers.push_back(data.str());
  }
  answers.push_back("]}");
  return answers;
}

vector<string> ListHubs::operator()(Server& server,  // NOLINT
                                    const StrStrMap& args,
                                    Logger& log) {
  vector<string> answers;
  string header = string("HTTP/1.1 200 OK\r\nContent-Type: application/json")
      + "\r\nConnection: close\r\n\r\n"
      + "{\"hubs\": [";
  answers.push_back(header);
  const TransitNetwork& network = server.network();
  const HubSet& hubs = server.router().hubs();
  HubSet::const_iterator it;
  for (it = hubs.begin(); it != hubs.end(); ++it) {
    const Stop& stop = network.stop(*it);
    ostringstream data;
    if (it != hubs.begin()) {
      data << ",";
    }
    data << "{\"id\":\"" << stop.id() << "\","
         << "\"name\":\"" << stop.name() << "\","
         << "\"lat\":" << stop.lat() << ",\"lon\":" << stop.lon()
         << "}";
    answers.push_back(data.str());
  }
  answers.push_back("]}");
  return answers;
}

vector<string> GetGeoInfo::operator()(Server& server,  // NOLINT
                                      const StrStrMap& args,
                                      Logger& log) {
  vector<string> answers;
  string header = string("HTTP/1.1 200 OK\r\nContent-Type: application/json")
      + "\r\nConnection: close\r\n\r\n";
  answers.push_back(header);
  const GeoInfo& geo = server.network().geoInfo();
  ostringstream data;
  data << "{\"min_lat\":" << geo.latMin
       << ",\"max_lat\":" << geo.latMax
       << ",\"min_lon\":" << geo.lonMin
       << ",\"max_lon\":" << geo.lonMax
       << "}";
  answers.push_back(data.str());
  return answers;
}

vector<string> LabelStops::operator()(Server& server,  // NOLINT
                                      const StrStrMap& args, Logger& log) {
  vector<string> answers;
  string header = string("HTTP/1.1 200 OK\r\nContent-Type: application/json")
                  + "\r\nConnection: close\r\n\r\n"
                  + "{\"labels\": [";
  answers.push_back(header);
  int count = -1;
  if (!found(args, string("count"), count)) {}
  const TransitNetwork& network = server.network();
  for (int i = 0; i < count; ++i) {
    size_t stopId = -1;
    string key = "stopid" + convert<string>(i);
    if (!found(args, key, stopId) || stopId >= network.numStops()) {
      log.info("Did not found stop index %d", stopId);
    } else {
      const Stop& stop = network.stop(stopId);
      ostringstream data;
      if (i > 0) { data << ","; }
      data << "{\"index\":" << stopId << ","
           <<  "\"id\":\"" << stop.id() << "\","
           <<  "\"name\":\"" << stop.name() << "\","
           <<  "\"lat\":" << stop.lat() << ",\"lon\":" << stop.lon()
           << "}";
      answers.push_back(data.str());
    }
  }
  answers.push_back("]}");
//   for (string& answer: answers)
//     std::cout << answer << std::endl;
  return answers;
}
