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

Composition::Ptr c_leu() {
  cyclus::CompMap m;
  m[922350000] = 0.04;
  m[922380000] = 0.96;
  return Composition::CreateFromMass(m);
};
Composition::Ptr c_leu2() {
  cyclus::CompMap m;
  m[922350000] = 0.05;
  m[922380000] = 0.95;
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
TEST(RandomSinkTests, TestEvery) {
  //  Tests that Every returns with the correct frequency

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
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(RandomSinkTests, TestUserPref) {
  //    -- swu capacitated trades should go to only higher pref facility
  //     ( user_pref: change default preference for requests )
 

  std::string config = 
    "   <in_commods><val>fuel</val></in_commods> "
    "   <recipe_name>leu</recipe_name> "
    "   <user_pref>5</user_pref>";


  int simdur = 1;
  cyclus::MockSim sim(cyclus::AgentSpec
		      (":mbmore:RandomSink"), config, simdur);
  sim.AddRecipe("leu", c_leu());
  sim.AddRecipe("leu2", c_leu2());
  
  sim.AddSource("fuel")
    .capacity(1)
    .Finalize();

  // Use a regular Sink that should have a low preference
  std::string prototype = sim.AddSink("leu")
    .recipe("leu2")
    .Finalize();

  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("fuel")));
  QueryResult qr = sim.db().Query("Transactions", &conds);

  // should trade only with RandomSink because it has higher preference
  EXPECT_EQ(1, qr.rows.size());
  
  Material::Ptr m = sim.GetMaterial(qr.GetVal<int>("ResourceId"));
  CompMap got = m->comp()->mass();
  CompMap want = c_leu()->mass();
  cyclus::compmath::Normalize(&got);
  cyclus::compmath::Normalize(&want);

  CompMap::iterator it;
  for (it = want.begin(); it != want.end(); ++it) {
    EXPECT_DOUBLE_EQ(it->second, got[it->first]) <<
      "nuclide qty off: " << pyne::nucname::name(it->first);
  }
  
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(RandomSinkTests, TestTradeTime) {
  // Tests that simulation does not trade before t_trade begins
  // (t_trade starts at time step 0, so six timesteps span 0-5)

  std::string config = 
    "   <in_commods><val>leu</val></in_commods> "
    "   <recipe_name>leu</recipe_name> "
    "   <t_trade>4</t_trade>";

  int simdur = 6;
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
  
  // Should be only three transactions into the sink
  EXPECT_EQ(2.0, qr.rows.size());
  
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  /*
    rng_seed: cannot be tested
    avg_qty: for normal dist (already tested in behavior fns)
    sigma: for normal dist (already tested in behavior fns)
  */

//TEST(RandomSinkTests, TestReference) {
  // Not sure how to test the Reference Flag:
  // When Reference is on, then normal distribution should be the same as
  // when Reference is off but quantity is zero.

  
} // namespace randomsinktests
} // namespace mbmore
