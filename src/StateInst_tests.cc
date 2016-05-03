#include <gtest/gtest.h>

#include "cyclus.h"

namespace mbmore {


namespace StateInstTests {
  /*
  TEST(StateInstTests, DeployProto) {
  std::string config = 
     "<declared_protos> <val>source</val> </declared_protos>"
     "<secret_protos> <val>secret_sink</val> </secret_protos>"
     ;

  int simdur = 6;
  cyclus::MockSim sim(cyclus::AgentSpec(":mbmore:StateInst"), config, simdur);

  sim.AddSource("source")
    .capacity(1)
    .Finalize();

  sim.AddSink("secret_sink")
    .capacity(1)
    .Finalize();

  int id = sim.Run();

  cyclus::SqlStatement::Ptr stmt = sim.db().db().Prepare(
							 //      "SELECT COUNT(*) FROM AgentEntry WHERE Prototype = 'foobar' AND EnterTime = 1;"
      "SELECT COUNT(*) FROM AgentEntry WHERE Prototype = 'secret_sink';"
      );
  stmt->Step();
  EXPECT_EQ(1, stmt->GetInt(0));

  stmt = sim.db().db().Prepare(
      "SELECT COUNT(*) FROM AgentEntry WHERE Prototype = 'secret_sink' AND EnterTime = 4;"
      );
  stmt->Step();
  EXPECT_EQ(1, stmt->GetInt(0));
}
  */




} // namespace StateInstTests
} // namespace mbmore