// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko

// This file collects basic includes for all tests we write as well as some
// utility functions.

#ifndef TEST_GTESTUTIL_H_
#define TEST_GTESTUTIL_H_

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <cstdio>


using std::string;

// If you create temporary output files for testing, please store them in
// 'data/tmp/' in order to keep the other directories clean. Otherwise you are
// welcome to have your files removed, e.g. by the Fixture's TearDown() method.
const string tmpDir = "data/tmp/"; //NOLINT


// A macro to mark a test as not yet implemented.
#define TEST_NOT_IMPLEMENTED printf("TEST NOT IMPLEMENTED.\n"); \
                             return;

#endif  // TEST_GTESTUTIL_H_
