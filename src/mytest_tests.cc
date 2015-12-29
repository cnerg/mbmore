#include <gtest/gtest.h>

#include "mytest.h"

#include "agent_tests.h"
#include "context.h"
#include "facility_tests.h"

using mbmore::mytest;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class mytestTest : public ::testing::Test {
 protected:
  cyclus::TestContext tc;
  mytest* facility;

  virtual void SetUp() {
    facility = new mytest(tc.get());
  }

  virtual void TearDown() {
    delete facility;
  }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(mytestTest, InitialState) {
  // Test things about the initial state of the facility here
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(mytestTest, Print) {
  EXPECT_NO_THROW(std::string s = facility->str());
  // Test mytest specific aspects of the print method here
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(mytestTest, Tick) {
  ASSERT_NO_THROW(facility->Tick());
  // Test mytest specific behaviors of the Tick function here
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(mytestTest, Tock) {
  EXPECT_NO_THROW(facility->Tock());
  // Test mytest specific behaviors of the Tock function here
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Do Not Touch! Below section required for connection with Cyclus
cyclus::Agent* mytestConstructor(cyclus::Context* ctx) {
  return new mytest(ctx);
}
// Required to get functionality in cyclus agent unit tests library
#ifndef CYCLUS_AGENT_TESTS_CONNECTED
int ConnectAgentTests();
static int cyclus_agent_tests_connected = ConnectAgentTests();
#define CYCLUS_AGENT_TESTS_CONNECTED cyclus_agent_tests_connected
#endif  // CYCLUS_AGENT_TESTS_CONNECTED
INSTANTIATE_TEST_CASE_P(mytest, FacilityTests,
                        ::testing::Values(&mytestConstructor));
INSTANTIATE_TEST_CASE_P(mytest, AgentTests,
                        ::testing::Values(&mytestConstructor));
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
