// Copyright 2011, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Hannah Bast <bast>.

#ifndef SRC_CSVPARSER_H_
#define SRC_CSVPARSER_H_

#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <vector>

using std::string;
using std::vector;

// Class for processing the CSV files from GTFS.
class CsvParser {
 public:
  // Opens file and reads first line with table headers.
  void openFile(string fileName);

  // Returns true iff same function of underlying stream returns true.
  bool eof() const { return _fileStream.eof(); }

  // Read next line.
  void readNextLine();
  FRIEND_TEST(CsvParserTest, readNextLine);

  // Get i-th column from current line. Prerequisite: i < _numColumns.
  const char* getItem(size_t i);

  // Close file. Prerequisite: file must have been opened with openFile before,
  // assert failure otherwise.
  void closeFile();

  // Get the number of columns. Will be zero before openFile has been called.
  size_t getNumColumns() const { return _currentItems.size(); }

 private:
  // The handle to the file.
  std::ifstream _fileStream;

  // Current line (pointers returned by readNextLine will refer to parts of it.
  string _currentLine;

  // Pointers to the items in the current line.
  vector<const char*> _currentItems;
};

#endif  // SRC_CSVPARSER_H_
