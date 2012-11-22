// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#ifndef SRC_SERVER_H_
#define SRC_SERVER_H_

#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <string>
#include <vector>
#include <map>
#include <set>
#include "./Logger.h"
#include "./TransferPatternRouter.h"

using std::string;
using std::set;
using boost::asio::ip::tcp;
using boost::shared_ptr;
using boost::thread;
using boost::function;

class Server;
class TransitNetwork;

class Worker {
 public:
  Worker(const function<void(void)>& func, Server* server);
  void operator()();
 private:
  function<void(void)> _func;
  Server* _server;
};

class Server {
 public:
  Server(const int port, const string& dataDir,
         const string& workDir, const string& logPath);
  void loadGtfs(const string& path,
                const int startTime, const int endTime);
  void loadGtfs(const vector<string>& paths,
                const int startTime, const int endTime);
  void precompute();
  void precomputeHubs();
  void precomputeTransferPatterns();
  void run();
  TransitNetwork& network();
  TransitNetwork& scenario();
  void scenario(const TransitNetwork& scenario);
  bool scenarioSet();
  void scenarioSet(bool value);
  TransferPatternRouter& router();
  int maxWorkers() const;
  void maxWorkers(const int n);
  string dataDir() const;
  string workDir() const;
  int port() const;
  static string retrieveQuery(const string& request);
  static string retrieveCommand(const string& query);
  static StrStrMap retrieveArgs(const string& query, const string& workDir);

  void reserveWorker();
  void joinWorkers(const int numBusy = 0);

 private:
  void handleRequest(shared_ptr<tcp::socket> socket, Logger log);
  void releaseWorker();

  bool loadHubs(HubSet* hubs);
  void saveHubs(const HubSet& hubs);
  bool loadTransferPatternsDB(TransferPatternsDB* tpdb);
  void saveTransferPatternsDB(const TransferPatternsDB& tpdb);
  FRIEND_TEST(ServerTest, hubAndTPDBSerialization);

  friend class Worker;

  TransitNetwork _network;
  TransitNetwork _scenario;
  TransferPatternRouter _router;
  TransferPatternsDB _tpdb;
  bool _scenarioSet;

  int _port;
  string _dataDir;
  string _workDir;
  Logger _log;
  int _maxWorkers;
  int _activeWorkers;
  boost::shared_mutex _maxWorkerMutex;
  boost::shared_mutex _activeWorkerMutex;
};


#endif  // SRC_SERVER_H_
