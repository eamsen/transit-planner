// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include <boost/foreach.hpp>
#include <gmock/gmock.h>
#include <vector>
#include <string>

#include "./GtestUtil.h"
#include "../src/Line.h"
#include "../src/DirectConnection.h"
#include "../src/Random.h"

using std::string;
using std::make_pair;
using ::testing::ElementsAre;
using ::testing::Contains;

class DirectConnectionTest : public ::testing::Test {
 public:
  void SetUp() {
    // Trips stops:
    // trip 1: 0   -> 1       -> 2       -> 3
    // times:  100 -> 200|210 -> 300|310 -> 400|410
    // trip 2: 0   -> 1       -> 2       -> 3
    // times:  500 -> 600|610 -> 700|710 -> 800|810
    // trip 3: 0   -> 1         -> 4         -> 3
    // times:  900 -> 1000|1010 -> 1100|1110 -> 1200|1210
    // trip 4: 0   -> 1       -> 5       -> 6
    // times:  100 -> 200|210 -> 300|310 -> 400|410
    // trip 5: 0   -> 1       -> 5       -> 6
    // times:  500 -> 600|610 -> 700|710 -> 800|810
    // trip 6: 0   -> 1       -> 5       -> 6
    // times:  800 -> 810|820 -> 830|840 -> 950|960
    // trip 7: 0   -> 1       -> 5       -> 6
    // times:  800 -> 800|800 -> 800|800 -> 900|910
    // Trip 1
    times.push_back(make_pair(0, 100));
    times.push_back(make_pair(200, 210));
    times.push_back(make_pair(300, 310));
    times.push_back(make_pair(400, 410));
    // Trip 2
    times.push_back(make_pair(0, 500));
    times.push_back(make_pair(600, 610));
    times.push_back(make_pair(700, 710));
    times.push_back(make_pair(800, 810));
    // Trip 3
    times.push_back(make_pair(0, 900));
    times.push_back(make_pair(1000, 1010));
    times.push_back(make_pair(1100, 1110));
    times.push_back(make_pair(1200, 1210));
    // Trip 4
    times.push_back(make_pair(0, 100));
    times.push_back(make_pair(200, 210));
    times.push_back(make_pair(300, 310));
    times.push_back(make_pair(400, 410));
    // Trip 5
    times.push_back(make_pair(0, 500));
    times.push_back(make_pair(600, 610));
    times.push_back(make_pair(700, 710));
    times.push_back(make_pair(800, 810));
    // Trip 6
    times.push_back(make_pair(0, 800));
    times.push_back(make_pair(810, 820));
    times.push_back(make_pair(830, 840));
    times.push_back(make_pair(950, 960));
    // Trip 7
    times.push_back(make_pair(0, 800));
    times.push_back(make_pair(800, 800));
    times.push_back(make_pair(800, 800));
    times.push_back(make_pair(900, 910));
    // Trip 1, line 1
    stops.push_back(0);
    stops.push_back(1);
    stops.push_back(2);
    stops.push_back(3);
    // Trip 2, line 1
    stops.push_back(0);
    stops.push_back(1);
    stops.push_back(2);
    stops.push_back(3);
    // Trip 3, line 2
    stops.push_back(0);
    stops.push_back(1);
    stops.push_back(4);
    stops.push_back(3);
    // Trip 4, line 3
    stops.push_back(0);
    stops.push_back(1);
    stops.push_back(5);
    stops.push_back(6);
    // Trip 5, line 3
    stops.push_back(0);
    stops.push_back(1);
    stops.push_back(5);
    stops.push_back(6);
    // Trip 6, line 3
    stops.push_back(0);
    stops.push_back(1);
    stops.push_back(5);
    stops.push_back(6);
    // Trip 7, line 3
    stops.push_back(0);
    stops.push_back(1);
    stops.push_back(5);
    stops.push_back(6);

    t1.addStop(0, 100, 0);
    t1.addStop(200, 210, 1);
    t1.addStop(300, 310, 2);
    t1.addStop(400, 410, 3);

    t2.addStop(0, 500, 0);
    t2.addStop(600, 610, 1);
    t2.addStop(700, 710, 2);
    t2.addStop(800, 810, 3);

    t3.addStop(0, 900, 0);
    t3.addStop(1000, 1010, 1);
    t3.addStop(1100, 1110, 4);
    t3.addStop(1200, 1210, 3);

    t4.addStop(0, 100, 0);
    t4.addStop(200, 210, 1);
    t4.addStop(300, 310, 5);
    t4.addStop(400, 410, 6);

    t5.addStop(0, 500, 0);
    t5.addStop(600, 610, 1);
    t5.addStop(700, 710, 5);
    t5.addStop(800, 810, 6);

    t6.addStop(0, 800, 0);
    t6.addStop(810, 820, 1);
    t6.addStop(830, 840, 5);
    t6.addStop(950, 960, 6);

    t7.addStop(0, 800, 0);
    t7.addStop(800, 800, 1);
    t7.addStop(800, 800, 5);
    t7.addStop(900, 910, 6);
  }

  void TearDown() {}

  vector<Int64Pair> times;
  vector<int> stops;
  Trip t1, t2, t3, t4, t5, t6, t7;
};

TEST_F(DirectConnectionTest, addTrip1) {
  Line l1;
  EXPECT_TRUE(l1.candidate(t1));
  EXPECT_TRUE(l1.addTrip(t1));
  EXPECT_TRUE(l1.candidate(t2));
  EXPECT_TRUE(l1.addTrip(t2));
  EXPECT_FALSE(l1.candidate(t3));
  EXPECT_FALSE(l1.addTrip(t3));
  EXPECT_FALSE(l1.candidate(t4));
  EXPECT_FALSE(l1.addTrip(t4));
  EXPECT_FALSE(l1.candidate(t5));
  EXPECT_FALSE(l1.addTrip(t5));
}

TEST_F(DirectConnectionTest, addTrip2) {
  Line l1;
  EXPECT_TRUE(l1.candidate(t3));
  EXPECT_TRUE(l1.addTrip(t3));
  EXPECT_FALSE(l1.candidate(t1));
  EXPECT_FALSE(l1.addTrip(t1));
  EXPECT_FALSE(l1.candidate(t2));
  EXPECT_FALSE(l1.addTrip(t2));
  EXPECT_FALSE(l1.candidate(t4));
  EXPECT_FALSE(l1.addTrip(t4));
  EXPECT_FALSE(l1.candidate(t5));
  EXPECT_FALSE(l1.addTrip(t5));
}

TEST_F(DirectConnectionTest, addTrip3) {
  Line l1;
  EXPECT_TRUE(l1.candidate(t4));
  EXPECT_TRUE(l1.addTrip(t4));
  EXPECT_TRUE(l1.candidate(t5));  //
  EXPECT_TRUE(l1.addTrip(t5));
  EXPECT_FALSE(l1.candidate(t1));
  EXPECT_FALSE(l1.addTrip(t1));
  EXPECT_FALSE(l1.candidate(t2));
  EXPECT_FALSE(l1.addTrip(t2));
  EXPECT_FALSE(l1.candidate(t3));
  EXPECT_FALSE(l1.addTrip(t3));
}

TEST_F(DirectConnectionTest, TripFactory) {
  vector<Trip> trips;
  LineFactory::createTrips(times, stops, &trips);
  EXPECT_EQ(7, trips.size());
  EXPECT_EQ(t1, trips.at(0));
  EXPECT_EQ(t2, trips.at(1));
  EXPECT_EQ(t3, trips.at(2));
  EXPECT_EQ(t4, trips.at(3));
  EXPECT_EQ(t5, trips.at(4));
  EXPECT_EQ(t6, trips.at(5));
  EXPECT_EQ(t7, trips.at(6));
}

TEST_F(DirectConnectionTest, LineFactory) {
  vector<Trip> trips;
  LineFactory::createTrips(times, stops, &trips);
  vector<Line> lines = LineFactory::createLines(trips);
  EXPECT_EQ(5, lines.size());
  string l1Str = string("[ 0 1 2 3 ]\n[ 0|100 200|210 300|310 400|410 ]\n")
      + "[ 0|500 600|610 700|710 800|810 ]\n";
  EXPECT_EQ(l1Str, lines.at(0).str());
  string l2Str = "[ 0 1 4 3 ]\n[ 0|900 1000|1010 1100|1110 1200|1210 ]\n";
  EXPECT_EQ(l1Str, lines.at(0).str());
  string l3Str = string("[ 0 1 5 6 ]\n[ 0|100 200|210 300|310 400|410 ]\n")
      + "[ 0|500 600|610 700|710 800|810 ]\n";
  EXPECT_EQ(l3Str, lines.at(2).str());
  string l4Str = string("[ 0 1 5 6 ]\n[ 0|800 810|820 830|840 950|960 ]\n");
  EXPECT_EQ(l4Str, lines.at(3).str());
  string l5Str = string("[ 0 1 5 6 ]\n[ 0|800 800|800 800|800 900|910 ]\n");
  EXPECT_EQ(l5Str, lines.at(4).str());
}

// have a cyclic line
// NOTICE: This tests shows a structural problem of our implementation. It
// cannot handle lines with multiple incidents at the same stop.
TEST_F(DirectConnectionTest, LineFactory_doubleStop) {
  //   TEST_NOT_IMPLEMENTED  // <-- uncomment to disable this failing test
  vector<Int64Pair> times;
  vector<int> stops = {0, 1, 2, 3, 0, 4, 0};
  times.push_back(make_pair(100, 100));
  times.push_back(make_pair(200, 250));
  times.push_back(make_pair(350, 400));
  times.push_back(make_pair(480, 500));
  times.push_back(make_pair(550, 600));
  times.push_back(make_pair(680, 700));
  times.push_back(make_pair(780, 780));

  vector<Trip> trips;
  trips.push_back(LineFactory::createTrip(times, stops));

  // test the direct connection queries
  vector<Line> lines = LineFactory::createLines(trips);
  // NOTICE: This test can be used later on when we handle cyclic trips.
  ASSERT_EQ(1, lines.size());
  std::string str = string("[ 0 1 2 3 0 4 0 ]\n") +
                "[ 100|100 200|250 350|400 480|500 550|600 680|700 780|780 ]\n";
  EXPECT_STREQ(str.c_str(), lines[0].str().c_str());

  // NOTICE: In natural scenarios there are always more stops than stops on
  // a line, but in the tests this may result in errors if a line holds more
  // stops due to cycles. So instead of
  // DirectConnection dc(5);  because |{0, 1, 2, 3, 4}| = 5  we use
  DirectConnection dc(stops.size());
  for (const Line& l: lines) {
    dc.addLine(l);
  }

  ASSERT_EQ(3, dc._incidents[0].size());
  ASSERT_EQ(1, dc._incidents[1].size());
  ASSERT_EQ(1, dc._incidents[2].size());
  ASSERT_EQ(1, dc._incidents[3].size());


//   EXPECT_EQ(0, dc.query(0, 100, 0));
  EXPECT_EQ(300, dc.query(1, 250, 0));
  EXPECT_EQ(80, dc.query(0, 600, 4));
  EXPECT_EQ(100, dc.query(4, 680, 0));
}

TEST_F(DirectConnectionTest, query) {
  vector<Trip> trips;
  LineFactory::createTrips(times, stops, &trips);
  vector<Line> lines = LineFactory::createLines(trips);
  DirectConnection dc(7);
  BOOST_FOREACH(const Line& l, lines) {
    dc.addLine(l);
  }
  // Trips stops:
  // trip 1: 0   -> 1       -> 2       -> 3
  // times:  100 -> 200|210 -> 300|310 -> 400|410
  // trip 2: 0   -> 1       -> 2       -> 3
  // times:  500 -> 600|610 -> 700|710 -> 800|810
  // trip 3: 0   -> 1         -> 4         -> 3
  // times:  900 -> 1000|1010 -> 1100|1110 -> 1200|1210
  // trip 4: 0   -> 1       -> 5       -> 6
  // times:  100 -> 200|210 -> 300|310 -> 400|410
  // trip 5: 0   -> 1       -> 5       -> 6
  // times:  500 -> 600|610 -> 700|710 -> 800|810
  EXPECT_EQ(250, dc.query(0, 50, 2));
  EXPECT_EQ(350, dc.query(0, 50, 3));
  EXPECT_EQ(690, dc.query(0, 110, 3));
  EXPECT_EQ(650, dc.query(0, 550, 3));
  EXPECT_EQ(400, dc.query(1, 0, 6));
  EXPECT_EQ(580, dc.query(1, 220, 6));
  EXPECT_EQ(DirectConnection::INFINITE, dc.query(3, 0, 0));
  EXPECT_EQ(DirectConnection::INFINITE, dc.query(4, 0, 6));
  EXPECT_EQ(DirectConnection::INFINITE, dc.query(1, 1200, 3));
}


TEST_F(DirectConnectionTest, nextDepartureTime) {
  vector<Trip> trips;
  LineFactory::createTrips(times, stops, &trips);
  vector<Line> lines = LineFactory::createLines(trips);
  DirectConnection dc(7);
  BOOST_FOREACH(const Line& l, lines) {
    dc.addLine(l);
  }
  // Trips stops:
  // trip 1: 0   -> 1       -> 2       -> 3
  // times:  100 -> 200|210 -> 300|310 -> 400|410
  // trip 2: 0   -> 1       -> 2       -> 3
  // times:  500 -> 600|610 -> 700|710 -> 800|810
  // trip 3: 0   -> 1         -> 4         -> 3
  // times:  900 -> 1000|1010 -> 1100|1110 -> 1200|1210
  // trip 4: 0   -> 1       -> 5       -> 6
  // times:  100 -> 200|210 -> 300|310 -> 400|410
  // trip 5: 0   -> 1       -> 5       -> 6
  // times:  500 -> 600|610 -> 700|710 -> 800|810
  EXPECT_EQ(100, dc.nextDepartureTime(0, 50, 2));
  EXPECT_EQ(100, dc.nextDepartureTime(0, 50, 3));
  EXPECT_EQ(500, dc.nextDepartureTime(0, 110, 3));
  EXPECT_EQ(900, dc.nextDepartureTime(0, 550, 3));
  EXPECT_EQ(210, dc.nextDepartureTime(1, 0, 6));
  EXPECT_EQ(610, dc.nextDepartureTime(1, 220, 6));
  EXPECT_EQ(DirectConnection::INFINITE, dc.nextDepartureTime(3, 0, 0));
  EXPECT_EQ(DirectConnection::INFINITE, dc.nextDepartureTime(4, 0, 6));
  EXPECT_EQ(DirectConnection::INFINITE, dc.nextDepartureTime(1, 1200, 3));
}

TEST_F(DirectConnectionTest, zeroConnections) {
  vector<Trip> trips;
  LineFactory::createTrips(times, stops, &trips);
  vector<Line> lines = LineFactory::createLines(trips);
  DirectConnection dc(7);
  BOOST_FOREACH(const Line& l, lines) {
    dc.addLine(l);
  }
  // Trips stops:
  // trip 1: 0   -> 1       -> 2       -> 3
  // times:  100 -> 200|210 -> 300|310 -> 400|410
  // trip 2: 0   -> 1       -> 2       -> 3
  // times:  500 -> 600|610 -> 700|710 -> 800|810
  // trip 3: 0   -> 1         -> 4         -> 3
  // times:  900 -> 1000|1010 -> 1100|1110 -> 1200|1210
  // trip 4: 0   -> 1       -> 5       -> 6
  // times:  100 -> 200|210 -> 300|310 -> 400|410
  // trip 5: 0   -> 1       -> 5       -> 6
  // times:  500 -> 600|610 -> 700|710 -> 800|810
  // trip 6: 0   -> 1       -> 5       -> 6
  // times:  800 -> 810|820 -> 830|840 -> 950|960
  // trip 7: 0   -> 1       -> 5       -> 6
  // times:  800 -> 800|800 -> 800|800 -> 900|910
  EXPECT_EQ(0, dc.query(0, 800, 5));
  EXPECT_EQ(0, dc.query(0, 800, 1));
  EXPECT_EQ(100, dc.query(0, 700, 5));
  EXPECT_EQ(100, dc.query(0, 800, 6));
  EXPECT_EQ(DirectConnection::INFINITE, dc.query(0, 810, 5));
}

TEST_F(DirectConnectionTest, queryPerf1M) {
  vector<Trip> trips;
  LineFactory::createTrips(times, stops, &trips);
  vector<Line> lines = LineFactory::createLines(trips);
  DirectConnection dc(7, lines);
  RandomGen timeGen(0, 1000);
  RandomGen stopGen(0, 6);
  for (int i = 0; i < 1000000; ++i) {
    dc.query(stopGen.next(), timeGen.next(), stopGen.next());
  }
}
