// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include <gmock/gmock.h>
#include <ostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include "../src/Statistics.h"

using std::vector;
using std::map;
using std::set;


TEST(StatisticsTest, compareTest) {
  vector<int> path = {1, 2, 3};
  int maxPenalty = 12;
  LabelMatrix matrix;
  matrix.resize(1, 16);
  LabelVec::Hnd label1 = matrix.add(0, 60*100, 1, maxPenalty);
  LabelVec::Hnd label2 = matrix.add(0, 60*40, 2, maxPenalty);
  LabelVec::Hnd label3 = matrix.add(0, 60*200, 0, maxPenalty);
  LabelVec::Hnd label4 = matrix.add(0, 60*90, 1, maxPenalty);
  LabelVec::Hnd label5 = matrix.add(0, 60*204, 0, maxPenalty);

  vector<QueryResult::Path> paths1;
  paths1.push_back(QueryResult::Path(label1, path));
  paths1.push_back(QueryResult::Path(label2, path));
  vector<QueryResult::Path> paths2;
  paths2.push_back(QueryResult::Path(label3, path));
  paths2.push_back(QueryResult::Path(label4, path));
  vector<QueryResult::Path> paths3;
  paths3.push_back(QueryResult::Path(label3, path));
  vector<QueryResult::Path> paths4;
  vector<QueryResult::Path> paths5;
  paths5.push_back(QueryResult::Path(label5, path));

  QueryCompare qc;
  EXPECT_EQ(3, qc.compare(paths1, paths2));
  EXPECT_EQ(3, qc.compare(paths2, paths3));
  EXPECT_EQ(1, qc.compare(paths3, paths2));
  EXPECT_EQ(2, qc.compare(paths3, paths5));
  EXPECT_EQ(3, qc.compare(paths5, paths4));
  EXPECT_EQ(0, qc.compare(paths4, paths5));
}
