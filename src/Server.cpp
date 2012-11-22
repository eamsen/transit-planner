// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include "./Server.h"
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/serialization/map.hpp>
#include <omp.h>
// #include <google/profiler.h>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include "./Utilities.h"
#include "./GtfsParser.h"
#include "./Command.h"
#include "./Random.h"

using std::string;
using std::stringstream;
using std::ostringstream;
using std::ifstream;
using std::vector;
using std::map;
using std::copy;
using boost::asio::io_service;
using boost::system::error_code;
using boost::asio::ip::tcp;
using boost::thread_group;
using boost::shared_ptr;
using boost::bind;

typedef boost::unique_lock<boost::shared_mutex> WriteLock;
typedef boost::shared_lock<boost::shared_mutex> ReadLock;


Worker::Worker(const function<void(void)>& func, Server* server)
    : _func(func), _server(server) {}

void Worker::operator()() {
  _func();
  _server->releaseWorker();
}


Server::Server(const int port, const string& dataDir,
               const string& workDir, const string& logPath)
    : _router(_network), _scenarioSet(false), _port(port), _dataDir(dataDir),
      _workDir(workDir), _maxWorkers(1), _activeWorkers(0) {
  _log.target(logPath);
  _router.logger(&_log);
//   _router.hubs().set_empty_key(-1);
}

void Server::maxWorkers(const int n) {
  WriteLock(_maxWorkerMutex);
  _maxWorkers = std::max(n, 1);
}

int Server::maxWorkers() const {
  ReadLock(_maxWorkerMutex);
  return _maxWorkers;
}

void Server::reserveWorker() {
  while (true) {
    boost::this_thread::sleep(boost::posix_time::milliseconds(3));
    {
      WriteLock(_activeWorkerMutex);
      if (_activeWorkers < maxWorkers()) {
        ++_activeWorkers;
        return;
      }
    }
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
  }
}

void Server::releaseWorker() {
  WriteLock(_activeWorkerMutex);
  --_activeWorkers;
}

void Server::joinWorkers(const int numBusy) {
  while (true) {
    {
      ReadLock(_activeWorkerMutex);
      if (_activeWorkers <= numBusy) {
        return;
      }
    }
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
  }
}

TransitNetwork& Server::network() {
  return _network;
}

TransitNetwork& Server::scenario() {
  return _scenario;
}

void Server::scenario(const TransitNetwork& scenario) {
  _scenario = scenario;
  _scenarioSet = true;
}

bool Server::scenarioSet() {
  return _scenarioSet;
}

void Server::scenarioSet(bool value) {
  _scenarioSet = value;
}

TransferPatternRouter& Server::router() {
  return _router;
}

string Server::dataDir() const {
  return _dataDir;
}

string Server::workDir() const {
  return _workDir;
}

int Server::port() const {
  return _port;
}

void Server::loadGtfs(const string& path,
                      const int startTime, const int endTime) {
  const vector<string> gtfsDirs = {path};
  loadGtfs(gtfsDirs, startTime, endTime);
}

void Server::loadGtfs(const vector<string>& paths,
                      const int startTime, const int endTime) {
  assert(paths.size());
  const string startStr = time2str(startTime);
  const string endStr   = time2str(endTime);
  GtfsParser parser;
  parser.logger(&_log);
  bool loaded = false;
  string serialFile;
  vector<Line> lines;
  bool serialization = false;
  if (serialization) {
    serialFile = parser.parseName(paths, startStr, endStr);
    serialFile = "local/" + serialFile + "_network.serialized";
    loaded = parser.load(serialFile, &_network);
  }
  if (!loaded) {
    const int perf_id = _log.beginPerf();
    _network = parser.createTransitNetwork(paths, startStr, endStr, &lines);
    _log.endPerf(perf_id, "GtfsParser::parse() on ");
    for (auto it = paths.begin(); it != paths.end(); ++it)
      _log.info(" --> %s", it->c_str());
    const int perf_id2 = _log.beginPerf();
    _network.preprocess();
    _log.endPerf(perf_id2, "TransitNetwork::preprocess()");
//     const int perf_id3 = _log.beginPerf();
//     const size_t numStopsOld = _network.numStops();
//     _network = _network.largestConnectedComponent();
//     _log.endPerf(perf_id3, "TransitNetwork::largestConnectedComponent()");
//     _log.info("Largest connected component has %i of %i stops.",
//               _network.numStops(), numStopsOld);
  }
  _router.prepare(lines);
  if (serialization && !loaded) {
    parser.save(_network, serialFile);
  }
}


void Server::precompute() {
  precomputeHubs();
  precomputeTransferPatterns();
}


void Server::precomputeHubs() {
  // ProfilerStart("log/hubs.perf");
  assert(_router.hubs().size() == 0);
  HubSet hubs;
  if (loadHubs(&hubs)) {
    _router.hubs(hubs);
    _log.info("Loaded %d hub stations.", _router.hubs().size());
  } else {
    // (stopIndex, frequency) pairs
    vector<IntPair> stopFreqs(_network.numStops(), make_pair(0, 0));
    for (size_t i = 0; i < _network.numStops(); ++i) {
      stopFreqs[i].first = i;
    }
    RandomFloatGen random(0, 1, getSeed());
    const size_t numSeedStops = _network.numStops() * 0.01 + 1;
    int progId = _log.beginProg();
    for (size_t i = 0; i < numSeedStops; ++i) {
      string id = _network.stopTree().randomWalk(&random).stopPtr->id();
      int seedStop = _network.stopIndex(id);
      _router.countStopFreq(seedStop, &stopFreqs);
      _log.info("Dijkstra for hub selection from stop %s.", id.c_str());
      _log.prog(progId, i, numSeedStops, "finding hubs");
    }
    std::sort(stopFreqs.begin(), stopFreqs.end(), sortStopsByImportance());
    _log.info("most frequent stop is %i: %i", stopFreqs.at(0).first,
              stopFreqs.at(0).second);
    _log.info("least frequent stop is %i: %i", stopFreqs.back().first,
              stopFreqs.back().second);
    const int numHubs = ceil(_network.numStops() * 0.01);
    for (int i = 0; i < numHubs; ++i) {
      hubs.insert(stopFreqs.at(i).first);
    }
    _router.hubs(hubs);
    _log.endProg(progId, "found all hubs.");
    saveHubs(_router.hubs());
  }
  // ProfilerStop();
}

void Server::precomputeTransferPatterns() {
  // ProfilerStart("log/tp.perf");
  if (loadTransferPatternsDB(&_tpdb)) {
    _log.info("Loaded transfer patterns.");
  } else {
    if (_tpdb.numGraphs() == 0) {
      _tpdb.init(_network.numStops(), _router.hubs());
    }

    const int progId = _log.beginProg();
    const size_t numStops = _network.numStops();
    const int nThreads = _maxWorkers > omp_get_max_threads() ?
                         omp_get_max_threads() : _maxWorkers;
    omp_set_num_threads(nThreads);
    int progress = 0;
    #pragma omp parallel
    {  // NOLINT
    const TransitNetwork network = _network;
    const HubSet hubs = _router.hubs();
    TPDB tpdb(network.numStops(), hubs);
    #pragma omp for schedule(dynamic, 3)
    for (size_t stop = 0; stop < numStops; ++stop) {
      const set<vector<int> > patterns =
          TransferPatternRouter::computeTransferPatterns(network, stop, hubs);
      for (auto it = patterns.cbegin(), end = patterns.cend(); it != end; ++it)
        tpdb.addPattern(*it);
      tpdb.finalise(stop);  // clears construction cache

      #pragma omp critical(progress_message)
      if (progress < 100 || progress % (numStops / 100) == 0)
        _log.prog(progId, progress + 1, numStops,
                  "computing transfer patterns (mt)", omp_get_num_threads());
      #pragma omp atomic
      ++progress;
    }
    #pragma omp critical(reduction)
    {  // NOLINT
      // combine and clear the local TPDB
      _tpdb += tpdb;
      tpdb = TransferPatternsDB();
    }
    }  // pragma omp parallel

    saveTransferPatternsDB(_tpdb);
  }
  _router.transferPatternsDB(_tpdb);
  // ProfilerStop();
}

string Server::retrieveQuery(const string& request) {
  const string pre_query = "GET /";
  const string post_query = " HTTP";
  const size_t pre_offset = pre_query.size();
  const size_t post_offset = post_query.size();
  size_t pos = request.find(post_query);
  pos = pos == string::npos ? pos : pos - post_offset;
  string query = request.substr(pre_offset, pos);
  for (size_t i = 0; i < query.size(); ++i) {
    query[i] = isspace(query[i]) ? ' ' : query[i];
  }
  return query;
}

string Server::retrieveCommand(const string& query) {
  size_t pos = query.find("?");
  if (pos == string::npos) {
    return "web";
  } else {
    return query.substr(0, pos);
  }
}

StrStrMap Server::retrieveArgs(const string& query, const string& workDir) {
  StrStrMap m;
  size_t pos = query.find("?");
  if (pos == string::npos) {
    if (query.size())
      m["doc"] = workDir + "/" + query;
    else
      m["doc"] = workDir + "/index.html";
  } else {
    string q = query.substr(pos + 1);
    while (q.size()) {
      pos = q.find_first_of("& \r\n");
      if (pos == string::npos) {
        pos = q.size();
      }
      string arg = q.substr(0, pos);
      size_t pos2 = arg.find("=");
      string key = arg.substr(0, pos2);
      string value = arg.substr(pos2 + 1);
      m[key] = value;
      if (pos < q.size()) {
        q = q.substr(pos + 1);
      } else {
        break;
      }
    }
  }
  return m;
}

void Server::handleRequest(shared_ptr<tcp::socket> socket, Logger log) {
  // ProfilerStart("/home/sowa/local_workspace/pje/log/server.perf");
  Server* server = this;
  vector<char> buffer(1024*1024);
  error_code error;
  socket->read_some(boost::asio::buffer(buffer), error);
  string request(buffer.size(), 0);
  copy(buffer.begin(), buffer.end(), request.begin());
  string remoteIp = socket->remote_endpoint().address().to_string();
  log.info("received request from " + remoteIp);
  string query = server->retrieveQuery(request);
  log.debug("query: \"" + query + "\"");
  string command = server->retrieveCommand(query);
  StrStrMap args = server->retrieveArgs(query, server->workDir());
  string argsString = "{";
  for (StrStrMap::const_iterator i = args.begin();
       i != args.end();
       ++i) {
    argsString += i->first + ": " + i->second + ", ";
  }
  argsString = argsString.substr(0, argsString.size() - 2);
  argsString += "}";
  log.info("executing command <" + command + "> with args " + argsString);
  int tries = 10;
  while (tries) {
    try {
      Command::execute(*server, socket, command, args, log);
      tries = 0;
    } catch(const std::bad_alloc&) {
      --tries;
      if (tries) {
        _log.error("not enough memory to complete task: retrying");
        boost::this_thread::sleep(boost::posix_time::seconds(3));
      } else {
        _log.error("not enough memory to complete task: giving up");
      }
    }
  }
  socket->close();
  // ProfilerStop();
}

void Server::run() {
  _log.info("listening at port " + convert<string>(port()));
  // _log.enabled(false);
  io_service ioService;
  tcp::acceptor acceptor(ioService, tcp::endpoint(tcp::v4(), port()));
  bool quit = false;
  while (!quit) {
    shared_ptr<tcp::socket> socket(new tcp::socket(ioService));
    acceptor.accept(*socket);
    reserveWorker();
    Worker worker(bind(&Server::handleRequest, this, socket, _log), this);
    thread workerThread(worker);
  }
}


bool Server::loadHubs(HubSet* hubs) {
  assert(hubs->size() == 0);
  string serialFilename = "local/" + _network.name() + "_hubs.serialized";
  ifstream ifs(serialFilename);
  if (ifs.good()) {
    boost::archive::binary_iarchive ia(ifs);
    set<int> tmp;
    ia >> tmp;
    for (auto it = tmp.begin(); it != tmp.end(); ++it) {
      hubs->insert(*it);
    }
//     ia >> *hubs;
    return true;
  } else {
    return false;
  }
}


void Server::saveHubs(const HubSet& hubs) {
  assert(hubs.size() != 0);
  string serialFilename = "local/" + _network.name() + "_hubs.serialized";
  ofstream ofs(serialFilename);
  if (ofs.good()) {
    boost::archive::binary_oarchive oa(ofs);
    // workaround
    set<int> tmp(hubs.begin(), hubs.end());
    oa << tmp;
//     oa << hubs;
    _log.info("Saved hubs stations to '%s'.", serialFilename.c_str());
  } else {
    _log.error("%s: file could not be opened for writing",
               serialFilename.c_str());
  }
}


bool Server::loadTransferPatternsDB(TransferPatternsDB* tpdb) {
  assert(tpdb->numGraphs() == 0);
  string serialFilename = "local/" + _network.name() + "_TPDB.serialized";
  _log.info("Trying to load TPDB from '%s'", serialFilename.c_str());
  ifstream ifs(serialFilename);
  if (ifs.good()) {
    boost::archive::binary_iarchive ia(ifs);
    ia >> *tpdb;
    return true;
  } else {
    return false;
  }
}


void Server::saveTransferPatternsDB(const TransferPatternsDB& tpdb) {
  assert(tpdb.numGraphs() != 0);
  string serialFilename = "local/" + _network.name() + "_TPDB.serialized";
  _log.info("Serializing...");
  ofstream ofs(serialFilename);
  if (ofs.good()) {
    boost::archive::binary_oarchive oa(ofs);
    oa << tpdb;
    _log.info("Saved TransferPatternDB to '%s'.", serialFilename.c_str());
  } else {
    _log.error("%s: file could not be opened for writing",
               serialFilename.c_str());
  }
  ofs.close();
}
