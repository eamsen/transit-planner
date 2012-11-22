// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include <gmock/gmock.h>
#include <ostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <cmath>
#include <ctime>
#include "./GtestUtil.h"
#include "../src/Utilities.h"

using std::vector;
using std::map;
using std::set;
using std::string;
using ::testing::ElementsAre;
using ::testing::Contains;

// DON'T USE FIXTURES IF YOU DO NOT NEED THEM
// class UtilitiesTest : public ::testing::Test {
//  public:
//   void SetUp() {}
//   void TearDown() {}
// };

// _____________________________________________________________________________
TEST(UtilitiesTest, foundConvertTest) {
  const float e = 0.000001f;
  map<string, string> m;
  string k1 = "a";
  string k2 = "b";
  string k3 = "c";
  string k4 = "d";
  m[k1] = "554";
  m[k2] = "3.14";
  m[k3] = "ronin";
  int a = 0;
  EXPECT_TRUE(found(m, k1, a));
  EXPECT_EQ(554, a);
  EXPECT_FALSE(found(m, k4, a));
  EXPECT_EQ(554, a);
  float b = 0.0f;
  EXPECT_TRUE(found(m, k2, b));
  EXPECT_TRUE(fabs(3.14 - b) < e);
  EXPECT_FALSE(found(m, k4, b));
  EXPECT_TRUE(fabs(3.14 - b) < e);
  string c = "";
  EXPECT_TRUE(found(m, k3, c));
  EXPECT_EQ("ronin", c);
  EXPECT_FALSE(found(m, k4, c));
  EXPECT_EQ("ronin", c);
}

// _____________________________________________________________________________
TEST(UtilitiesTest, str2timeTest1) {
  // 2011-12-24 21:22:00
  string time_str =  "20111224T212200";
  int time = str2time(time_str);
  EXPECT_EQ(time_str, time2str(time));
}

// _____________________________________________________________________________
TEST(UtilitiesTest, str2timeTest2) {
  // Perform a check for the current time.
  int now = getSeed();
  string now_str = time2str(now);
  EXPECT_EQ(now, str2time(now_str));
}

// _____________________________________________________________________________
TEST(UtilitiesTest, isValidTimeStringTest) {
  EXPECT_TRUE(isValidTimeString("20111128T203100"));
  EXPECT_TRUE(isValidTimeString("20111128T83100"));
  EXPECT_FALSE(isValidTimeString("This is no time string."));
}

// _____________________________________________________________________________
TEST(UtilitiesTest, getWeekdayTest) {
  EXPECT_EQ("Monday", getWeekday(str2time("20111128T120000")));
  EXPECT_EQ("Tuesday", getWeekday(str2time("20111128T263000")));
}

// _____________________________________________________________________________
TEST(UtilitiesTest, getDateOffsetSecondsTest) {
  EXPECT_EQ(0, getDateOffsetSeconds(str2time("19700101T120000")));
  EXPECT_EQ(24*60*60, getDateOffsetSeconds(str2time("19700102T120000")));
}

// _____________________________________________________________________________
TEST(UtilitiesTest, formatPerfTime) {
  double s = 12.12;
  EXPECT_EQ("12.12s", formatPerfTime(s));
  s = 0.1544;
  EXPECT_EQ("154.4ms", formatPerfTime(s));
  s = 0.000051;
  EXPECT_EQ("51µs", formatPerfTime(s));
}

// _____________________________________________________________________________
TEST(UtilitiesTest, listDirs) {
  vector<string> dirs = listDirs(".");
  set<string> dir_set(dirs.begin(), dirs.end());
  set<string> expected_set = {"build", "data", "src", "test", "log", "web",
                              "plot"};
//   EXPECT_THAT(expected_set, Contains(expected_set));
}

// _____________________________________________________________________________
TEST(UtilitiesTest, greatCircleDistance) {
  int d0 = greatCircleDistance(0, 0, 0, 0);
  EXPECT_EQ(0, d0);
  // The distance between two points with latitude different by 1 and equal
  // longitude is about 111km.
  int d1 = greatCircleDistance(0, 0, 1.0, 0);
  EXPECT_NEAR(111000, d1, 500);
}
