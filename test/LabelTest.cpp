// Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
#include <gmock/gmock.h>
#include <iostream>
#include "./GtestUtil.h"
#include "../src/Label.h"

using std::cout;
using std::endl;
using ::testing::ElementsAre;
using ::testing::Contains;
using ::testing::Not;
// using ::testing::WhenSortedBy;

class LabelTest : public ::testing::Test {
 public:
  void SetUp() {}
  void TearDown() {}
};

// _____________________________________________________________________________
TEST_F(LabelTest, LabelVec_Hnd) {
  unsigned int cost1 = 12323;
  unsigned char penalty1 = 12;
  LabelVec::Hnd l1(cost1, penalty1, false, NULL);
  EXPECT_EQ(cost1, l1.cost());
  EXPECT_EQ(penalty1, l1.penalty());

  unsigned int cost2 = 12323;
  unsigned char penalty2 = 13;
  LabelVec::Hnd l2(cost2, penalty2, false, NULL);
  EXPECT_EQ(cost2, l2.cost());
  EXPECT_EQ(penalty2, l2.penalty());

  EXPECT_TRUE(l1 < l2);
  EXPECT_TRUE(l2 > l1);

  unsigned int cost3 = 12324;
  unsigned char penalty3 = 12;
  LabelVec::Hnd l3(cost3, penalty3, false, NULL);
  EXPECT_EQ(cost3, l3.cost());
  EXPECT_EQ(penalty3, l3.penalty());

  EXPECT_TRUE(l1 < l3);
  EXPECT_TRUE(l3 > l1);
  EXPECT_TRUE(l2 < l3);
}

// _____________________________________________________________________________
TEST_F(LabelTest, LabelVec_candidate) {
  int at = 0;
  int maxPenalty = 20;
  LabelVec vec(at, maxPenalty);
  EXPECT_TRUE(vec.candidate(10, 1));

  vec.add(10, 1, maxPenalty, false, false, NULL);
  EXPECT_FALSE(vec.candidate(10, 1));
  EXPECT_FALSE(vec.candidate(10, 2));
  EXPECT_FALSE(vec.candidate(11, 1));
  EXPECT_FALSE(vec.candidate(11, 2));
  EXPECT_TRUE(vec.candidate(9, 0));
  EXPECT_TRUE(vec.candidate(9, 1));
  EXPECT_TRUE(vec.candidate(9, 2));
  EXPECT_TRUE(vec.candidate(10, 0));
  EXPECT_TRUE(vec.candidate(11, 0));

  vec.add(11, 0, maxPenalty, false, false, NULL);
  EXPECT_FALSE(vec.candidate(10, 1));
  EXPECT_FALSE(vec.candidate(10, 2));
  EXPECT_FALSE(vec.candidate(11, 0));
  EXPECT_FALSE(vec.candidate(11, 1));
  EXPECT_FALSE(vec.candidate(11, 2));
  EXPECT_TRUE(vec.candidate(9, 0));
  EXPECT_TRUE(vec.candidate(9, 1));
  EXPECT_TRUE(vec.candidate(9, 2));
  EXPECT_TRUE(vec.candidate(10, 0));

  vec.add(0, 0, maxPenalty, false, false, NULL);
  EXPECT_FALSE(vec.candidate(10, 1));
  EXPECT_FALSE(vec.candidate(10, 2));
  EXPECT_FALSE(vec.candidate(11, 0));
  EXPECT_FALSE(vec.candidate(11, 1));
  EXPECT_FALSE(vec.candidate(11, 2));
  EXPECT_FALSE(vec.candidate(9, 0));
  EXPECT_FALSE(vec.candidate(9, 1));
  EXPECT_FALSE(vec.candidate(9, 2));
  EXPECT_FALSE(vec.candidate(10, 0));
}

// _____________________________________________________________________________
TEST_F(LabelTest, LabelVec_add) {
  int at = 0;
  int maxPenalty = 20;
  LabelVec vec(at, maxPenalty);

  EXPECT_TRUE(vec.candidate(10, 10));
  vec.add(10, 10, maxPenalty, false, false, NULL);
  // (10, 10)
  EXPECT_EQ(1, vec.size());

  EXPECT_TRUE(vec.candidate(10, 9));
  vec.add(10, 9, maxPenalty, false, false, NULL);
  // (10, 9)
  EXPECT_EQ(1, vec.size());

  EXPECT_TRUE(vec.candidate(10, 8));
  vec.add(10, 8, maxPenalty, false, false, NULL);
  // (10, 8)
  EXPECT_EQ(1, vec.size());

  EXPECT_TRUE(vec.candidate(11, 7));
  vec.add(11, 7, maxPenalty, false, false, NULL);
  // (10, 8), (11, 7)
  EXPECT_EQ(2, vec.size());

  EXPECT_TRUE(vec.candidate(12, 6));
  vec.add(12, 6, maxPenalty, false, false, NULL);
  // (10, 8), (11, 7), (12, 6)
  EXPECT_EQ(3, vec.size());

  EXPECT_TRUE(vec.candidate(13, 5));
  vec.add(13, 5, maxPenalty, false, false, NULL);
  // (10, 8), (11, 7), (12, 6), (13, 5)
  EXPECT_EQ(4, vec.size());

  EXPECT_TRUE(vec.candidate(10, 7));
  vec.add(10, 7, maxPenalty, false, false, NULL);
  // (12, 6), (13, 5), (10, 7)
  EXPECT_EQ(3, vec.size());

  EXPECT_TRUE(vec.candidate(0, 0));
  vec.add(0, 0, maxPenalty, false, false, NULL);
  // (0, 0)
  EXPECT_EQ(1, vec.size());
}

// _____________________________________________________________________________
TEST_F(LabelTest, LabelVec_minCost_minPenalty) {
  int at = 0;
  int maxPenalty = 20;
  LabelVec vec(at, maxPenalty);

  EXPECT_EQ(INT_MAX, vec.minCost());
  EXPECT_EQ(INT_MAX, vec.minPenalty());
  vec.add(10, 10, maxPenalty, false, false, NULL);
  EXPECT_EQ(10, vec.minCost());
  EXPECT_EQ(10, vec.minPenalty());

  vec.add(11, 9, maxPenalty, false, false, NULL);
  EXPECT_EQ(10, vec.minCost());
  EXPECT_EQ(9, vec.minPenalty());

  vec.add(12, 8, maxPenalty, false, false, NULL);
  EXPECT_EQ(10, vec.minCost());
  EXPECT_EQ(8, vec.minPenalty());

  vec.add(13, 7, maxPenalty, false, false, NULL);
  EXPECT_EQ(10, vec.minCost());
  EXPECT_EQ(7, vec.minPenalty());

  vec.add(12, 7, maxPenalty, false, false, NULL);
  EXPECT_EQ(10, vec.minCost());
  EXPECT_EQ(7, vec.minPenalty());

  vec.add(12, 6, maxPenalty, false, false, NULL);
  EXPECT_EQ(10, vec.minCost());
  EXPECT_EQ(6, vec.minPenalty());

  vec.add(9, 10, maxPenalty, false, false, NULL);
  EXPECT_EQ(9, vec.minCost());
  EXPECT_EQ(6, vec.minPenalty());

  vec.add(0, 0, maxPenalty, false, false, NULL);
  EXPECT_EQ(0, vec.minCost());
  EXPECT_EQ(0, vec.minPenalty());
}

// _____________________________________________________________________________
TEST_F(LabelTest, LabelMatrix_candidate_add) {
  int numNodes = 4;
  int maxPenalty = 10;
  LabelMatrix matrix;
  matrix.resize(numNodes, maxPenalty);

  // 0:
  // 1:
  // 2:
  EXPECT_TRUE(matrix.candidate(0, 10, 10));
  LabelMatrix::Hnd l1 = matrix.add(0, 10, 10, maxPenalty);
  EXPECT_EQ(10, l1.cost());
  EXPECT_EQ(10, l1.penalty());
  // 0: (10, 10)
  // 1:
  // 2:
  EXPECT_TRUE(matrix.candidate(1, 10, 10));
  LabelMatrix::Hnd l2 = matrix.add(1, 10, 10, maxPenalty);
  EXPECT_EQ(10, l2.cost());
  EXPECT_EQ(10, l2.penalty());
  // 0: (10, 10)
  // 1: (10, 10)
  // 2:
  EXPECT_TRUE(matrix.candidate(2, 10, 10));
  LabelMatrix::Hnd l3 = matrix.add(2, 11, 10, maxPenalty);
  EXPECT_EQ(11, l3.cost());
  EXPECT_EQ(10, l3.penalty());
  // 0: (10, 10)
  // 1: (10, 10)
  // 2: (11, 10)
  EXPECT_FALSE(matrix.candidate(0, 10, 10));
  EXPECT_FALSE(matrix.candidate(1, 10, 10));
  EXPECT_TRUE(matrix.candidate(2, 10, 10));
  LabelMatrix::Hnd l4 = matrix.add(2, 10, 10, maxPenalty);
  EXPECT_EQ(10, l4.cost());
  EXPECT_EQ(10, l4.penalty());
  // 0: (10, 10)
  // 1: (10, 10)
  // 2: (10, 10)
  EXPECT_TRUE(matrix.candidate(0, 11, 9));
  EXPECT_TRUE(matrix.candidate(1, 11, 9));
  EXPECT_TRUE(matrix.candidate(2, 11, 9));
  LabelMatrix::Hnd l5 = matrix.add(0, 11, 9, maxPenalty);
  EXPECT_EQ(11, l5.cost());
  EXPECT_EQ(9, l5.penalty());
  // 0: (10, 10), (11, 9)
  // 1: (10, 10)
  // 2: (10, 10)
  EXPECT_FALSE(matrix.candidate(0, 11, 9));
  EXPECT_TRUE(matrix.candidate(1, 11, 9));
  EXPECT_TRUE(matrix.candidate(2, 11, 9));
}

// _____________________________________________________________________________
TEST_F(LabelTest, LabelMatrix_Hnd_copy) {
  int numNodes = 4;
  int maxPenalty = 10;
  LabelMatrix matrix;
  matrix.resize(numNodes, maxPenalty);

  LabelMatrix::Hnd l1 = matrix.add(0, 10, 10, maxPenalty);
  EXPECT_EQ(0, l1.at());
  EXPECT_EQ(10, l1.cost());
  EXPECT_EQ(10, l1.penalty());
  LabelMatrix::Hnd l1c = l1;
  EXPECT_EQ(0, l1c.at());
  EXPECT_EQ(10, l1c.cost());
  EXPECT_EQ(10, l1c.penalty());
}
