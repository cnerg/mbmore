#include <gtest/gtest.h>

#include "cyclus.h"


#include "agent_tests.h"
#include "context.h"
#include "facility_tests.h"

namespace mbmore {


namespace InteractRegionTests {
  /*
  TEST(InteractRegionTests, DeployProto) {

  std::string config = 
    "<pursuit_weights>"
    "    <item><factor>Enrich</factor>  <weight>0.5</weight> </item>"
    "    <item><factor>Conflict</factor><weight>0.5</weight> </item>"
    "    </pursuit_weights>"
    "<likely_converter>  "
    "    <item>"
    "      <phase>Pursuit</phase>"
    "      <function>"
    "   	<name>power</name>"
    "	        <params>"
    "	            <val>3</val>"
    "	        </params>"
    "      </function>"
    "    </item>"
    "</likely_converter>"
    "<p_conflict_relations>"
    "	  <item>"
    "	    <primary_state>StateA</primary_state>"
    "	    <pair_state>"
    "	      <item>"
    "		<name>StateB</name>"
    "		<relation>1</relation>"
    "	      </item>"
    "	    </pair_state>"
    "	  </item>"
    "	  <item>"
    "	    <primary_state>StateB</primary_state>"
    "	    <pair_state>"
    "	      <item>"
    "		<name>StateA</name>"
    "		<relation>1</relation>"
    "	      </item>"
    "	    </pair_state>"
    "	  </item>"
    "</p_conflict_relations>";

  int simdur = 1;
  cyclus::MockSim sim(cyclus::AgentSpec(":mbmore:StateInst"), config, simdur);

  
  }
  */
  /*
  TEST(InteractRegionTests, DeployProto) {
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




} // namespace InteractRegionTests
} // namespace mbmore