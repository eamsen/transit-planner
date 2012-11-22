// Copyright 2011, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Hannah Bast <bast>.

#include <assert.h>
#include <algorithm>
#include <string>
#include <vector>
#include "./CsvParser.h"

using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::remove;

// ____________________________________________________________________________
void CsvParser::openFile(string fileName) {
  _fileStream.open(fileName.c_str());
  assert(_fileStream.good());
  readNextLine();
  assert(_currentItems.size() > 0);
}


// ____________________________________________________________________________
void CsvParser::readNextLine() {
  assert(_fileStream.good());
  getline(_fileStream, _currentLine);
  // remove new line characters
  _currentLine.erase(remove(_currentLine.begin(), _currentLine.end(), '\r'),
                     _currentLine.end());
  _currentLine.erase(remove(_currentLine.begin(), _currentLine.end(), '\n'),
                     _currentLine.end());
  size_t pos = 0;
  _currentItems.clear();
  if (_fileStream.eof() && _currentLine.empty()) return;
  // size_t i = 0;
  while (pos != string::npos) {
    size_t lastPos = pos;
    pos = _currentLine.find(',', pos);
    if (pos != string::npos) _currentLine[pos++] = 0;
    _currentItems.push_back(_currentLine.c_str() + lastPos);
    // assert(i < _currentItems.size());
    // _currentItems[i++] = _currentLine.c_str() + lastPos;
  }
}

// ____________________________________________________________________________
const char* CsvParser::getItem(size_t i) {
  assert(i < _currentItems.size());
  return _currentItems[i];
}

// ____________________________________________________________________________
void CsvParser::closeFile() {
  assert(_fileStream.is_open());
  _fileStream.close();
}
