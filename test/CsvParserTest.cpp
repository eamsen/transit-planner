// Copyright 2011, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Hannah Bast <bast>.

#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <vector>
#include "./GtestUtil.h"
#include "../src/CsvParser.h"

using std::cout;
using std::endl;
using std::vector;

// _____________________________________________________________________________
TEST(CsvParserTest, readNextLine) {
  // Write small test CSV file.
  string testFileName = tmpDir + "/CsvParserTest.TMP.csv";
  std::ofstream file;
  file.open(testFileName.c_str());
  file << "H1,H2,H3" << endl
       << "1,2,3" << endl
       << "4,," << endl
       << ",5," << endl
       << ",,6" << endl
       << ",," << endl;
  file.close();

  // Now see if readNextLine does the right thing.
  CsvParser cp;
  ASSERT_EQ(0, cp.getNumColumns());
  cp.openFile(testFileName.c_str());
  ASSERT_EQ(3, cp.getNumColumns());
  ASSERT_EQ("H1", string(cp.getItem(0)));
  ASSERT_EQ("H2", string(cp.getItem(1)));
  ASSERT_EQ("H3", string(cp.getItem(2)));
  ASSERT_FALSE(cp.eof());
  cp.readNextLine();
  ASSERT_FALSE(cp.eof());
  ASSERT_EQ(3, cp.getNumColumns());
  ASSERT_EQ("1", string(cp.getItem(0)));
  ASSERT_EQ("2", string(cp.getItem(1)));
  ASSERT_EQ("3", string(cp.getItem(2)));
  cp.readNextLine();
  ASSERT_FALSE(cp.eof());
  ASSERT_EQ(3, cp.getNumColumns());
  ASSERT_EQ("4", string(cp.getItem(0)));
  ASSERT_EQ("", string(cp.getItem(1)));
  ASSERT_EQ("", string(cp.getItem(2)));
  cp.readNextLine();
  ASSERT_FALSE(cp.eof());
  ASSERT_EQ(3, cp.getNumColumns());
  ASSERT_EQ("", string(cp.getItem(0)));
  ASSERT_EQ("5", string(cp.getItem(1)));
  ASSERT_EQ("", string(cp.getItem(2)));
  cp.readNextLine();
  ASSERT_FALSE(cp.eof());
  ASSERT_EQ(3, cp.getNumColumns());
  ASSERT_EQ("", string(cp.getItem(0)));
  ASSERT_EQ("", string(cp.getItem(1)));
  ASSERT_EQ("6", string(cp.getItem(2)));
  cp.readNextLine();
  ASSERT_FALSE(cp.eof());
  ASSERT_EQ(3, cp.getNumColumns());
  ASSERT_EQ("", string(cp.getItem(0)));
  ASSERT_EQ("", string(cp.getItem(1)));
  ASSERT_EQ("", string(cp.getItem(2)));
  cp.readNextLine();
  ASSERT_TRUE(cp.eof());
  ASSERT_EQ(0, cp.getNumColumns());
  cp.closeFile();
}
