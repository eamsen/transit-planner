// Copyright 2011-12: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "./Server.h"
#include "./Utilities.h"

namespace po = boost::program_options;
using std::string;
using std::vector;
using std::cout;
using std::endl;

const int kPortDef = 8080;
const string kDataDirDef = "data";  // NOLINT
const string kWorkDirDef = "web";  // NOLINT
const string kLogPathDef = "log/server.log";  // NOLINT

bool parseArgs(int argc, char* argv[],
               int& port, string& workDir, string& dataDir,
               string& initDirs, string& logPath,
               int& maxThreads);

int main(int argc, char* argv[]) {
  int port = kPortDef;
  string workDir = kWorkDirDef;
  string dataDir = kDataDirDef;
  string initDirs;
  string logPath = kLogPathDef;
  int maxThreads = 1;
  if (!parseArgs(argc, argv, port, workDir, dataDir, initDirs, logPath,
                 maxThreads)) {
    return 1;
  }
  Server server(port, dataDir, workDir, logPath);
  server.maxWorkers(maxThreads);
  vector<string> dirVec = splitString(initDirs);
  for (auto it = dirVec.begin(); it != dirVec.end(); ++it) { it->append("/"); }
  server.loadGtfs(dirVec, firstOfMay(), firstOfMay() + kSecondsPerDay * 1 - 9);
  if (dirVec.size()) {
    server.precompute();
  }
  // write a file with network information
  std::ofstream ofs("local/" + server.network().name() + ".info");
  for (int i = 0; i < argc; ++i) { ofs << argv[i] << " "; }
  ofs << endl;
  server.run();
  return 0;
}

bool parseArgs(int argc, char* argv[],
               int& port, string& workDir, string& dataDir,
               string& initDirs, string& logPath, int& maxThreads) {
  po::options_description args("Server options");
  args.add_options()
      ("help,h", "show help")
      ("port,p", po::value<int>(&port)->default_value(kPortDef), "port")
      ("httpdir,w",
       po::value<string>(&workDir)->default_value(kWorkDirDef),
       "http directory")
      ("datadir,d",
       po::value<string>(&dataDir)->default_value(kDataDirDef),
       "GTFS data directory")
      ("initdata,i",
       po::value<string>(&initDirs)->default_value(""),
       "initialise with this GTFS data directories, loads 1 day only")
      ("logfile,l", po::value<string>(&logPath)->default_value(kLogPathDef),
       "log file path")
      ("maxWorkers,m",
       po::value<int>(&maxThreads)->default_value(1),
       "maximum worker threads");
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, args), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << args << endl;
    return false;
  }
  return true;
}
