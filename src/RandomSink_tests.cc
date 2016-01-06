#include <gtest/gtest.h>

#include "cyclus.h"

using cyclus::QueryResult;
using cyclus::Cond;
using cyclus::CompMap;
using cyclus::toolkit::MatQuery;
using pyne::nucname::id;
using cyclus::Material;
using cyclus::Composition;

namespace mbmore {
  /*
    rng_seed: cannot be tested
    avg_qty: for normal dist (already tested in behavior fns)
    sigma: for normal dist (already tested in behavior fns)
  */

Composition::Ptr c_leu() {
  cyclus::CompMap m;
  m[922350000] = 0.04;
  m[922380000] = 0.96;
  return Composition::CreateFromMass(m);
};
Composition::Ptr c_heu() {
  cyclus::CompMap m;
  m[922350000] = 0.20;
  m[922380000] = 0.80;
  return Composition::CreateFromMass(m);
};
  
namespace randomsinktests {
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(RandomSinkTests, TestSocial) {
  // Tests that when social_behav = Every then trading occurs only every X
  // timestep (social_behav & behav_interval)

  std::string config = 
    "   <in_commods><val>leu</val></in_commods> "
    "   <recipe_name>leu</recipe_name> "
    "	<social_behav>Every</social_behav> "
    "  	<behav_interval>2</behav_interval> ";

  int simdur = 4;
  cyclus::MockSim sim(cyclus::AgentSpec
		      (":mbmore:RandomSink"), config, simdur);
  sim.AddRecipe("leu", c_leu());
  
  sim.AddSource("leu")
    .capacity(1)
    .recipe("leu")
    .Finalize();
  
  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("leu")));
  QueryResult qr = sim.db().Query("Transactions", &conds);
  
  // Should be only two transactions into the sink
  EXPECT_EQ(2.0, qr.rows.size());
  
}
  /*

    -- test that Reference == Random if interval is 1
     (social_behav: "None, Every, Random, Reference")
     
     -- swu capacitated trades should go to only higher pref facility
     ( user_pref: change default preference for requests )

     -- t_trade: time before which no trades occur

    */


  
} // namespace randomsinktests
} // namespace mbmore
