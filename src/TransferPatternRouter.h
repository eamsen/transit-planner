// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#ifndef SRC_TRANSFERPATTERNROUTER_H_
#define SRC_TRANSFERPATTERNROUTER_H_

#include <boost/serialization/access.hpp>
#include <google/dense_hash_set>
#include <vector>
#include <string>
#include <map>
#include <set>
#include "./Dijkstra.h"
#include "./DirectConnection.h"
#include "./Label.h"
#include "./Logger.h"
#include "./TransferPatternsDB.h"
#include "./TransitNetwork.h"
#include "./QueryGraph.h"
#include "./HubSet.h"

using std::vector;
using std::string;
using std::map;
using std::set;
using google::dense_hash_set;

// Used for sorting a vector of pairs <stop index, importance of stop>
struct sortStopsByImportance {
  bool operator()(const std::pair<int, int>& left,
                  const std::pair<int, int>& right) {
    return left.second > right.second;
  }
};


// The TransferPatternRouter class
class TransferPatternRouter {
 public:
  static const int TIME_LIMIT;
  static const unsigned char PENALTY_LIMIT;

  // Constructor
  explicit TransferPatternRouter(const TransitNetwork& network);
  FRIEND_TEST(TransferPatternRouterTest, Constructor);

  // Initializes the direct connection data structure and creates a time-inde-
  // pendent network used for hub selection.
  void prepare(const vector<Line>& lines);

  static
  set<vector<int> > computeTransferPatterns(const TransitNetwork& network,
                                            const int depStop,
                                            const HubSet& hubs);
  // Function solely used for testing.
  void computeAllTransferPatterns(TPDB* tpdb);

  // Computes all transferPatterns between the given departure Stop and the
  // given Hubs. PLUS: To all other stops in the network which can't be reached
  // over a hub
  static
  set<vector<int> > computeTransferPatternsToHubs(const TransitNetwork& network,
                                                  const int depStop,
                                                  const HubSet& hubs);

  // Computes the transfer patterns from the departure stop to any other stop.
  static
  set<vector<int> > computeTransferPatternsToAll(const TransitNetwork& network,
                                                 const int depStop,
                                                 const HubSet& hubs);

  // Constructs the QueryGraph from one stop to another, maybe empty. Uses hubs.
  const QueryGraph queryGraph(int depStop, int destStop) const;

  // Searches the shortest path between two stops starting at a certain time.
  vector<QueryResult::Path> shortestPath(const int startStop, const int time,
                                         const int targetStop,
                                         string* log = NULL);
  FRIEND_TEST(TransferPatternRouterTest, dijkstraCompare_walkFirstStop);
  FRIEND_TEST(TransferPatternRouterTest, dijkstraCompare_walkIntermediateStop);
  FRIEND_TEST(TransferPatternRouterTest, dijkstraCompare_walkLastStop);

  // Computes the hubs in the network by taking the number of nodes of each
  // stop into account. The stop wich has got the most nodes, is a hub.
  set<int> findBasicHubs(int numHubs);

  // Increases the counter for all stops which are on the optimal paths
  // from the seed stop.
  // stopFreqs is a vector of (stop index, frequency) pairs.
  void countStopFreq(const int seedStop, vector<IntPair>* stopFreqs) const;

  // Set the logger to an external logger.
  void logger(Logger* log);

  // Get the set of hubs.
  const HubSet& hubs() const;

  // Set the hubs.
  void hubs(const HubSet& hubs);

  // Get the transfer pattern db.
  const TPDB* transferPatternsDB() const;

  // Set the constant transfer pattern db.
  void transferPatternsDB(const TPDB& db);

  // Generates the set of transfer patterns between origin and destination using
  // the transfer patterns db. Used for debuging.
  set<vector<int> > generateTransferPatterns(const int orig, const int dest)
  const;
  // Generates all transfer patterns from the stop 'orig'. Used for debugging.
  set<vector<int> > generateAllTransferPatterns(const int orig) const;

  // Returns a string of the given patterns. Used for debugging and statistics.
  string printPatterns(const set<vector<int> >& patterns);

  const DirectConnection& directConnection();

 private:
  // Adds for every label at a transfer or departure node with walk == true a
  // new label to the matrix with costs = costs of the parent label + costs of
  // the walking arc between the stop of the parent label and the stop of the
  // label.
  static void adjustWalkingCosts(const TransitNetwork& network,
                                 LabelMatrix* matrix);
  // Performs the arrival loop algorithm. This method filters the vector
  // QueryResult.costs for costs of arrival nodes at the stop. For the labels of
  // these nodes, suboptimal paths are replaced by 'waiting from better paths'.
  // Returns remaining label sets as a vector of size = #arrival nodes at stop.
  static void arrivalLoop(const TransitNetwork& network, LabelMatrix* matrix,
                          const int stop);
  FRIEND_TEST(TransferPatternRouterTest, arrivalLoop_transitivity);

  // Computes all transfer patterns via backtracking of Labels in the matrix.
  static set<vector<int> >
  collectTransferPatterns(const TransitNetwork& network,
                          const LabelMatrix& matrix, const int depStop);

  // The network.
  const TransitNetwork& _network;
  // A compressed form of the network used to determine hubs.
  TransitNetwork _timeCompressedNetwork;

  // The search structure for direct connection queries.
  DirectConnection _connections;

  // Stores for every pair of stops a list of all transferPatterns from the
  // first stop to the second
  const TransferPatternsDB* _tpdb;

  // The hub stop indices (important stations).
  HubSet _hubs;

  Logger* _log;
};
typedef TransferPatternRouter TPR;

#endif  // SRC_TRANSFERPATTERNROUTER_H_
