// Copyright 2012, Jonas Sternisko
#include <gmock/gmock.h>
#include <time.h>
#include <vector>
#include "../src/TransitNetwork.h"
#include "../src/Random.h"

using testing::Contains;

// _____________________________________________________________________________
TEST(TreeOfStops, randomWalk) {
  srand(time(NULL));
  RandomFloatGen random(0, 1, time(NULL));
  // insert some stops into a tree and test if the random leaf operator works
  Stop s0("A", 0, 0), s1("B", 1, 0), s2("C", 0, 1), s3("D", 1, 1);
  StopTreeNode n0(s0), n1(s1), n2(s2), n3(s3);
  StopTree tree;
  tree.insert(n0);
  tree.insert(n1);
  tree.insert(n2);
  tree.insert(n3);

  vector<Stop> stops;
  stops.push_back(s0);
  stops.push_back(s1);
  stops.push_back(s2);
  stops.push_back(s3);
//   vector<Stop> stops = {s0, s1, s2, s3};
  for (int i = 0; i < 100; i++) {
    StopTreeNode r0 = tree.randomWalk(&random);
    EXPECT_THAT(stops, Contains(*r0.stopPtr));
  }
//   for (auto it = tree.begin(); it != tree.end(); it++) {
//     printf("Stop id: %s\n", it->stopPtr->id().c_str());
//   }


  vector<StopTreeNode> nodes;
  // fill the tree with random values
  for (int i = 0; i < 100; i++) {
    char buf[3];
    snprintf(buf, 3 * sizeof(char), "%d", i);  // NOLINT
    Stop random_stop(buf, rand() / 1000.f, rand() / 1000.f);  // NOLINT
    stops.push_back(random_stop);
  }

  // IMPORTANT: To use pointers on stops, the vector must be fixed. If you add
  // more stops, it might be relocated and pointers get invalid.
  for (uint i = 4; i < stops.size(); i++) {
    StopTreeNode random_node(stops[i]);
    tree.insert(random_node);
  }
//   tree.check_tree();
  tree.optimise();

  // some more Contains-checks
  for (int i = 0; i < 100; i++) {
    StopTreeNode r1 = tree.randomWalk(&random);
    EXPECT_THAT(stops, Contains(*(r1.stopPtr)));
//     printf("%s\n", r1.stopPtr->id().c_str());
  }
}
