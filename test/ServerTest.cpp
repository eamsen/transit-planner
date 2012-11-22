// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include <gmock/gmock.h>
#include <ostream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <cmath>
#include "./GtestUtil.h"
#include "../src/Server.h"
#include "../src/Utilities.h"

using std::string;
using std::pair;
using std::set;
using ::testing::ElementsAre;
using ::testing::Contains;

class ServerTest : public ::testing::Test {
 public:
  void SetUp() {}
  void TearDown() {}
};

TEST(ServerTest, parseQueryTest1) {
  string request = "GET /favicon.ico HTTP/1.1\r\nHost: localhost:8080\r\nConnection: keep-alive\r\nAccept: */*\r\nUser-Agent: Mozilla/5.0 (X11; Linux i686) AppleWebKit/535.1 (KHTML, like Gecko) Ubuntu/11.10 Chromium/14.0.835.202 Chrome/14.0.835.202 Safari/535.1\r\nAccept-Encoding: gzip,deflate,sdch\r\nAccept-Language: en-US,en;q=0.8\r\nAccept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3";  // NOLINT
  string query = Server::retrieveQuery(request);
  string command = Server::retrieveCommand(query);
  StrStrMap args = Server::retrieveArgs(query, "dummy/dir");
  EXPECT_EQ("favicon.ico", query);
  EXPECT_EQ(command, "web");
  EXPECT_THAT(args, ElementsAre(pair<string, string>("doc",
                                                     "dummy/dir/favicon.ico")));
}

TEST(ServerTest, parseQueryTest2) {
  const float lat = 49.234123f;
  const float lon = 32.3214245f;
  string request = "GET /select?lat=" + convert<string>(lat)
      + "&lon=" + convert<string>(lon)
      + " HTTP/1.1\r\nHost: localhost:8080\r\n...";
  string query = Server::retrieveQuery(request);
  string command = Server::retrieveCommand(query);
  StrStrMap args = Server::retrieveArgs(query, "dummy/dir");
  EXPECT_EQ("select?lat=" + convert<string>(lat)
            + "&lon=" + convert<string>(lon), query);
  EXPECT_EQ(command, "select");
  EXPECT_THAT(args, ElementsAre(pair<string, string>("lat",
                                                     convert<string>(lat)),
                                pair<string, string>("lon",
                                                     convert<string>(lon))));
}

TEST(ServerTest, hubAndTPDBSerialization) {
  string testset = "test/data/simple-parser-test-case/";
  Server s1(8081, "data", "web", "log/server.test.log");
  s1._log.enabled(false);
  s1.loadGtfs(testset, firstOfMay(), firstOfMay() + kSecondsPerDay);
  s1.precomputeHubs();  // this internally serializes the hubs

  HubSet loadedHubs;
  ASSERT_TRUE(s1.loadHubs(&loadedHubs));
  ASSERT_EQ(s1.router().hubs(), loadedHubs);
  s1.precomputeTransferPatterns();
  TPDB tpdb;
  ASSERT_TRUE(s1.loadTransferPatternsDB(&tpdb));
  ASSERT_EQ(*s1.router().transferPatternsDB(), tpdb);

  Server s2(8082, "data", "web", "log/server.test.log");
  s2._log.enabled(false);
  s2.loadGtfs(testset, firstOfMay() + 1, firstOfMay() + kSecondsPerDay + 1);
  HubSet hubs;
  ASSERT_TRUE(s2.loadHubs(&hubs));
  s2.router().hubs(hubs);
  ASSERT_EQ(s1.router().hubs(), s2.router().hubs());
  TPDB tpdb2;
  ASSERT_TRUE(s2.loadTransferPatternsDB(&tpdb2));
  s2.router().transferPatternsDB(tpdb2);
  ASSERT_EQ(*s1.router().transferPatternsDB(),
            *s2.router().transferPatternsDB());
  // Makefile deletes serialized hubs after testing
}
