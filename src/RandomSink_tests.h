#ifndef MBMORE_SRC_SINK_TESTS_H_
#define MBMORE_SRC_SINK_TESTS_H_

#include <gtest/gtest.h>

#include "RandomSink.h"

#include "test_context.h"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class RandomSinkTest : public ::testing::Test {
 protected:
  cyclus::TestContext tc_;
  TestFacility* trader;
  mbmore::RandomSink* src_facility;
  std::string commod1_, commod2_, commod3_;
  double capacity_, inv_, qty_;
  int ncommods_;

  virtual void SetUp();
  virtual void TearDown();
  void InitParameters();
  void SetUpRandomSink();
};

#endif  // MBMORE_SRC_SINK_TESTS_H_
