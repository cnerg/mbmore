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
  
namespace randomenrichtests {

Composition::Ptr c_nou235() {
  cyclus::CompMap m;
  m[922380000] = 1.0;
  return Composition::CreateFromMass(m);
};
Composition::Ptr c_natu1() {
  cyclus::CompMap m;
  m[922350000] = 0.007;
  m[922380000] = 0.993;
  return Composition::CreateFromMass(m);
};
Composition::Ptr c_natu2() {
  cyclus::CompMap m;
  m[922350000] = 0.01;
  m[922380000] = 0.99;
  return Composition::CreateFromMass(m);
};
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
Composition::Ptr c_heu90() {
  cyclus::CompMap m;
  m[922350000] = 0.90;
  m[922380000] = 0.10;
  return Composition::CreateFromMass(m);
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(RandomEnrichTests, RequestQty) {
  // this tests verifies that requests for input material are fulfilled
  // without providing any extra
  
  std::string config = 
    "   <feed_commod>natu</feed_commod> "
    "   <feed_recipe>natu1</feed_recipe> "
    "   <product_commod>enr_u</product_commod> "
    "   <tails_commod>tails</tails_commod> "
    "   <max_feed_inventory>1.0</max_feed_inventory> "
    "   <tails_assay>0.003</tails_assay> ";

  int simdur = 1;
  cyclus::MockSim sim(cyclus::AgentSpec
		      (":mbmore:RandomEnrich"), config, simdur);
  sim.AddRecipe("natu1", c_natu1());
  
  sim.AddSource("natu")
    .recipe("natu1")
    .Finalize();
  
  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("natu")));
  QueryResult qr = sim.db().Query("Transactions", &conds);
  Material::Ptr m = sim.GetMaterial(qr.GetVal<int>("ResourceId"));
  
  // Should be only one transaction into the EF,
  // and it should be exactly 1kg of natu
  EXPECT_EQ(1.0, qr.rows.size());
  EXPECT_NEAR(1.0, m->quantity(), 1e-10) <<
    "matched trade provides the wrong quantity of material";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(RandomEnrichTests, CheckSWUConstraint) {
  // Tests that request for enrichment that exceeds the SWU constraint
  // fulfilled only up to the available SWU.
  // Also confirms that initial_feed flag works.
  // 388 SWU = 10kg 80% enriched HEU from 486kg feed matl

  std::string config = 
    "   <feed_commod>natu</feed_commod> "
    "   <feed_recipe>natu1</feed_recipe> "
    "   <product_commod>enr_u</product_commod> "
    "   <tails_commod>tails</tails_commod> "
    "   <tails_assay>0.003</tails_assay> "
    "   <initial_feed>1000</initial_feed> "
    "   <swu_capacity>195</swu_capacity> ";

  int simdur = 1;
  
  cyclus::MockSim sim(cyclus::AgentSpec
		      (":mbmore:RandomEnrich"), config, simdur);
   
  sim.AddRecipe("natu1", c_natu1());
  sim.AddRecipe("heu", c_heu());
  
  sim.AddSink("enr_u")
    .recipe("heu")
    .capacity(10)
    .Finalize();
  
  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("enr_u")));
  QueryResult qr = sim.db().Query("Transactions", &conds);
  Material::Ptr m = sim.GetMaterial(qr.GetVal<int>("ResourceId"));

  EXPECT_EQ(1.0, qr.rows.size());
  EXPECT_NEAR(5.0, m->quantity(), 0.1) <<
    "traded quantity exceeds SWU constraint";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(RandomEnrichTests, CheckCapConstraint) {
  // Tests that a request for more material than is available in
  // inventory is partially filled with only the inventory quantity.

  std::string config = 
    "   <feed_commod>natu</feed_commod> "
    "   <feed_recipe>natu1</feed_recipe> "
    "   <product_commod>enr_u</product_commod> "
    "   <tails_commod>tails</tails_commod> "
    "   <tails_assay>0.003</tails_assay> "
    "   <initial_feed>243</initial_feed> ";

  int simdur = 1;

  cyclus::MockSim sim(cyclus::AgentSpec
		      (":mbmore:RandomEnrich"), config, simdur);


  sim.AddRecipe("natu1", c_natu1());
  sim.AddRecipe("heu", c_heu());
   
  sim.AddSink("enr_u")
    .recipe("heu")
    .capacity(10)
    .Finalize();
  
  int id = sim.Run();
  
  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("enr_u")));
  QueryResult qr = sim.db().Query("Transactions", &conds);
  Material::Ptr m = sim.GetMaterial(qr.GetVal<int>("ResourceId"));

  EXPECT_EQ(1.0, qr.rows.size());
  EXPECT_NEAR(5.0, m->quantity(), 0.01) <<
    "traded quantity exceeds capacity constraint";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(RandomEnrichTests, RequestEnrich) {
  // this tests verifies that requests for output material exceeding
  // the maximum allowed enrichment are not fulfilled.
  
  std::string config = 
    "   <feed_commod>natu</feed_commod> "
    "   <feed_recipe>natu1</feed_recipe> "
    "   <product_commod>enr_u</product_commod> "
    "   <tails_commod>tails</tails_commod> "
    "   <tails_assay>0.003</tails_assay> "
    "   <max_enrich>0.20</max_enrich> ";

  int simdur = 2;
  cyclus::MockSim sim(cyclus::AgentSpec
		      (":mbmore:RandomEnrich"), config, simdur);
  sim.AddRecipe("natu1", c_natu1());
  sim.AddRecipe("leu", c_leu());
  sim.AddRecipe("heu", c_heu());
  
  sim.AddSource("natu")
    .recipe("natu1")
    .Finalize();
  sim.AddSink("enr_u")
    .recipe("leu")
    .capacity(1.0)
    .Finalize();
  sim.AddSink("enr_u")
    .recipe("heu")
    .Finalize();
  
  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("enr_u")));
  QueryResult qr = sim.db().Query("Transactions", &conds);
  Material::Ptr m = sim.GetMaterial(qr.GetVal<int>("ResourceId"));
   
  // Should be only one transaction out of the EF,
  // and it should be 1kg of LEU
  EXPECT_EQ(1.0, qr.rows.size());
  EXPECT_NEAR(1.0, m->quantity(), 0.01) <<
    "Not providing the requested quantity" ;

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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(RandomEnrichTests, TradeTails) {
  // this tests whether tails are being traded.

  std::string config = 
    "   <feed_commod>natu</feed_commod> "
    "   <feed_recipe>natu1</feed_recipe> "
    "   <product_commod>enr_u</product_commod> "
    "   <tails_commod>tails</tails_commod> "
    "   <tails_assay>0.003</tails_assay> ";

  // time 1-source to EF, 2-Enrich, add to tails, 3-tails avail. for trade
  int simdur = 3;
  cyclus::MockSim sim(cyclus::AgentSpec
		      (":mbmore:RandomEnrich"), config, simdur);
  sim.AddRecipe("natu1", c_natu1());
  sim.AddRecipe("leu", c_leu());
  
  sim.AddSource("natu")
    .recipe("natu1")
    .Finalize();
  sim.AddSink("enr_u")
    .recipe("leu")
    .Finalize();
   sim.AddSink("tails")
    .Finalize();
  
  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("tails")));
  QueryResult qr = sim.db().Query("Transactions", &conds);

  // Should be exactly one tails transaction
  EXPECT_EQ(1, qr.rows.size());
  
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  TEST(RandomEnrichTests, TailsQty) {
  // this tests whether tails are being traded at correct quantity when
  // requested amount is larger than qty in a single tails-buffer element

  std::string config = 
    "   <feed_commod>natu</feed_commod> "
    "   <feed_recipe>natu1</feed_recipe> "
    "   <product_commod>enr_u</product_commod> "
    "   <tails_commod>tails</tails_commod> "
    "   <tails_assay>0.003</tails_assay> ";

  // time 1-source to EF, 2-Enrich, add to tails, 3-tails avail. for trade
  int simdur = 3;
  cyclus::MockSim sim(cyclus::AgentSpec
		      (":mbmore:RandomEnrich"), config, simdur);
  sim.AddRecipe("natu1", c_natu1());
  sim.AddRecipe("leu", c_leu());
  
  sim.AddSource("natu")
    .recipe("natu1")
    .Finalize();
  sim.AddSink("enr_u")
    .recipe("leu")
    .capacity(0.5)
    .Finalize();
  sim.AddSink("enr_u")
    .recipe("leu")
    .capacity(0.5)
    .Finalize();
  sim.AddSink("tails")
    .Finalize();

  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("tails")));
  QueryResult qr = sim.db().Query("Transactions", &conds);
  Material::Ptr m = sim.GetMaterial(qr.GetVal<int>("ResourceId"));
  
  // Should be 2 tails transactions, one from each LEU sink, each 4.084kg.
  EXPECT_EQ(2, qr.rows.size());

  cyclus::SqlStatement::Ptr stmt = sim.db().db().Prepare(
      "SELECT SUM(r.Quantity) FROM Transactions AS t"
      " INNER JOIN Resources AS r ON r.ResourceId = t.ResourceId"
      " WHERE t.Commodity = ?;"
      );

  stmt->BindText(1, "tails");
  stmt->Step();
  EXPECT_NEAR(8.168,stmt->GetDouble(0), 0.01) <<
    "Not providing the requested quantity" ;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(RandomEnrichTests, BidPrefs) {
  // This tests that natu sources are preference-ordered by
  // U235 content

  std::string config = 
    "   <feed_commod>natu</feed_commod> "
    "   <feed_recipe>natu1</feed_recipe> "
    "   <product_commod>enr_u</product_commod> "
    "   <tails_commod>tails</tails_commod> "
    "   <tails_assay>0.003</tails_assay> "
    "   <max_feed_inventory>1.0</max_feed_inventory> ";

  int simdur = 1;
  cyclus::MockSim sim(cyclus::AgentSpec
		      (":mbmore:RandomEnrich"), config, simdur);
  sim.AddRecipe("natu1", c_natu1());
  sim.AddRecipe("natu2", c_natu2());

  sim.AddSource("natu")
    .recipe("natu1")
    .capacity(1)
    .Finalize();

  sim.AddSource("natu")
    .recipe("natu2")
    .capacity(1)
    .Finalize();
  
  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("natu")));
  QueryResult qr = sim.db().Query("Transactions", &conds);

  // should trade only with #2 since it has higher U235
  EXPECT_EQ(1, qr.rows.size());
  
  Material::Ptr m = sim.GetMaterial(qr.GetVal<int>("ResourceId"));
  CompMap got = m->comp()->mass();
  CompMap want = c_natu2()->mass();
  cyclus::compmath::Normalize(&got);
  cyclus::compmath::Normalize(&want);

  CompMap::iterator it;
  for (it = want.begin(); it != want.end(); ++it) {
    EXPECT_DOUBLE_EQ(it->second, got[it->first]) <<
      "nuclide qty off: " << pyne::nucname::name(it->first);
  }
  
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  TEST(RandomEnrichTests, NoBidPrefs) {
  // This tests that preference-ordering for sources
  // turns off correctly if flag is used

  std::string config = 
    "   <feed_commod>natu</feed_commod> "
    "   <feed_recipe>natu1</feed_recipe> "
    "   <product_commod>enr_u</product_commod> "
    "   <tails_commod>tails</tails_commod> "
    "   <tails_assay>0.003</tails_assay> "
    "   <max_feed_inventory>2.0</max_feed_inventory> " 
    "   <order_prefs>0</order_prefs> ";

  int simdur = 1;
  cyclus::MockSim sim(cyclus::AgentSpec
		      (":mbmore:RandomEnrich"), config, simdur);
  sim.AddRecipe("natu1", c_natu1());
  sim.AddRecipe("natu2", c_natu2());

  sim.AddSource("natu")
    .recipe("natu1")
    .capacity(1)
    .Finalize();

  sim.AddSource("natu")
    .recipe("natu2")
    .capacity(1)
    .Finalize();
  
  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("natu")));
  QueryResult qr = sim.db().Query("Transactions", &conds);

  // should trade with both to meet its capacity limit
  EXPECT_EQ(2, qr.rows.size());
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(RandomEnrichTests, ZeroU235) {
  // Test that offers of natu with no u235 content are rejected

  std::string config = 
    "   <feed_commod>natu</feed_commod> "
    "   <feed_recipe>natu1</feed_recipe> "
    "   <product_commod>enr_u</product_commod> "
    "   <tails_commod>tails</tails_commod> "
    "   <tails_assay>0.003</tails_assay> "
    "   <max_feed_inventory>1.0</max_feed_inventory> ";

  int simdur = 1;
  cyclus::MockSim sim(cyclus::AgentSpec
		      (":mbmore:RandomEnrich"), config, simdur);
  sim.AddRecipe("no_u235", c_nou235());
  sim.AddRecipe("natu1", c_natu1());

  sim.AddSource("natu")
    .recipe("no_u235")
    .capacity(1)
    .Finalize();

  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("natu")));
  // DB table should be empty since there are no transactions
  EXPECT_THROW(sim.db().Query("Transactions", &conds),
	       std::exception);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(RandomEnrichTests, TestEvery) {
  // Tests that Enrichment only supplies material on the correct interval
  // when 'social_behav = Every'

  std::string config = 
    "   <feed_commod>natu</feed_commod> "
    "   <feed_recipe>natu1</feed_recipe> "
    "   <product_commod>leu</product_commod> "
    "   <tails_commod>tails</tails_commod> "
    "   <tails_assay>0.003</tails_assay> "
    "	<social_behav>Every</social_behav> "
    "  	<behav_interval>2</behav_interval> ";

  int simdur = 5;
  cyclus::MockSim sim(cyclus::AgentSpec
		      (":mbmore:RandomEnrich"), config, simdur);
  sim.AddRecipe("natu1", c_natu1());
  sim.AddRecipe("leu", c_leu());

  sim.AddSource("natu")
    .recipe("natu1")
    .capacity(1)
    .Finalize();

  
  sim.AddSink("leu")
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(RandomEnrichTests, TestTailsAssay) {
  // When tails has a truncated normal distribution (tails_sigma),
  // then each timestep should have a different tails assay, but always
  // within the range tails-sigma <= val <= tails+sigma

  std::string config = 
    "   <feed_commod>natu</feed_commod> "
    "   <feed_recipe>natu1</feed_recipe> "
    "   <product_commod>enr_u</product_commod> "
    "   <tails_commod>tails</tails_commod> "
    "   <tails_assay>0.002</tails_assay> "
    "   <sigma_tails>0.001</sigma_tails> ";

  // time 1-source to EF, 2-Enrich, add to tails, 3-tails avail. for trade
  int simdur = 4;
  cyclus::MockSim sim(cyclus::AgentSpec
		      (":mbmore:RandomEnrich"), config, simdur);
  sim.AddRecipe("natu1", c_natu1());
  sim.AddRecipe("leu", c_leu());
  
  sim.AddSource("natu")
    .recipe("natu1")
    .Finalize();
  sim.AddSink("enr_u")
    .recipe("leu")
    .capacity(1.0)
    .Finalize();
   sim.AddSink("tails")
    .Finalize();
  
  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("tails")));
  QueryResult qr = sim.db().Query("Transactions", &conds);
  int n_trans = qr.rows.size();
  EXPECT_EQ(2, n_trans) << "expected 2 transactions, got " << n_trans;

  int res_id = qr.GetVal<int>("ResourceId", 0);
  cyclus::Material::Ptr m0 = sim.GetMaterial(res_id);

  res_id = qr.GetVal<int>("ResourceId", 1);
  cyclus::Material::Ptr m1 = sim.GetMaterial(res_id);

  cyclus::toolkit::MatQuery mq0(m0);
  cyclus::toolkit::MatQuery mq1(m1);

  double t0 = mq0.mass(922350000)/(mq0.mass(922350000) + mq0.mass(922380000));
  double t1 = mq1.mass(922350000)/(mq1.mass(922350000) + mq1.mass(922380000));

  
  // Two tails values should be different from one another
  EXPECT_GT(std::abs(t0 - t1), 0.0);

 //And they should be no more than 2xsigma_tails different
  EXPECT_LE(std::abs(t0 - t1), 0.002);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(RandomEnrichTests, TestInspectionNegatives) {
  // Inspections occur with an average frequency (50%),
  // False_Neg: swipe rate should have 50% false negative rate when HEU is
  // present
  
  std::string config = 
    "   <feed_commod>natu</feed_commod> "
    "   <feed_recipe>natu1</feed_recipe> "
    "   <product_commod>enr_u</product_commod> "
    "   <tails_commod>tails</tails_commod> "
    "   <tails_assay>0.002</tails_assay> "
    "   <social_behav>Every</social_behav> "
    "   <behav_interval>1</behav_interval> "
    "   <inspect_freq>2</inspect_freq> "
    "   <n_swipes>10</n_swipes> "
    "   <false_pos>0</false_pos> "
    "   <false_neg>0.5</false_neg> ";

  // time 1-source to EF, 2-Enrich to sink
  int simdur = 100;
  cyclus::MockSim sim(cyclus::AgentSpec
		      (":mbmore:RandomEnrich"), config, simdur);
  sim.AddRecipe("natu1", c_natu1());
  sim.AddRecipe("enr_u", c_heu90());
  
  sim.AddSource("natu")
    .recipe("natu1")
    .capacity(1.0)
    .Finalize();
  sim.AddSink("enr_u")
    .recipe("enr_u")
    .Finalize();
  
  int id = sim.Run();
  std::vector<Cond> conds;
  conds.push_back(Cond("SampleLoc", "==", std::string("Cascade")));
  QueryResult qr = sim.db().Query("Inspections", &conds);
  int n_inspect = qr.rows.size();

  double eps = 0.1;
  double inspect_ratio = double(n_inspect)/double(simdur);
  EXPECT_NEAR(inspect_ratio, 0.5, eps) <<
    "expected 50 inspections, got " << n_inspect;

  // start counting fraction of positive swipes once HEU has been officially
  // detected.
  int n_heu_detected = 0;
  double swipe_tot = 0;
  for (int it=0; it < n_inspect; it++){
    double posfrac = qr.GetVal<double>("PosSwipeFrac", it);
    swipe_tot += posfrac;
    if (posfrac > 0) { n_heu_detected++; } 
  }
  double pos_rate = swipe_tot/double(n_heu_detected);
  EXPECT_NEAR(pos_rate, 0.5, eps);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(RandomEnrichTests, TestInspectionPositives) {
  // If no HEU is actually in the facility, the inspections should still
  // falsely measure the presence of HEU with some liklihood (50%)

 std::string config = 
    "   <feed_commod>natu</feed_commod> "
    "   <feed_recipe>natu1</feed_recipe> "
    "   <product_commod>enr_u</product_commod> "
    "   <tails_commod>tails</tails_commod> "
    "   <tails_assay>0.002</tails_assay> "
    "   <social_behav>Every</social_behav> "
    "   <behav_interval>1</behav_interval> "
    "   <inspect_freq>2</inspect_freq> "
    "   <n_swipes>10</n_swipes> "
    "   <false_pos>0.5</false_pos> "
    "   <false_neg>0</false_neg> ";

  // time 1-source to EF, 2-Enrich to sink
  int simdur = 100;
  cyclus::MockSim sim(cyclus::AgentSpec
		      (":mbmore:RandomEnrich"), config, simdur);
  sim.AddRecipe("natu1", c_natu1());
  sim.AddRecipe("enr_u", c_leu()); // this is 4% enriched, still "LEU"
  
  sim.AddSource("natu")
    .recipe("natu1")
    .capacity(1.0)
    .Finalize();
  sim.AddSink("enr_u")
    .recipe("enr_u")
    .Finalize();
  
  int id = sim.Run();
  std::vector<Cond> conds;
  conds.push_back(Cond("SampleLoc", "==", std::string("Cascade")));
  QueryResult qr = sim.db().Query("Inspections", &conds);
  int n_inspect = qr.rows.size();

  double eps = 0.05;

  double swipe_tot = 0;
  for (int it=0; it < n_inspect; it++){
    double posfrac = qr.GetVal<double>("PosSwipeFrac", it);
    swipe_tot += posfrac;
  }
  double pos_rate = swipe_tot/double(n_inspect);
  EXPECT_NEAR(pos_rate, 0.5, eps);

  } 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  TEST(RandomEnrichTests, TestHeuShipQty) {
    // Even though inspections are set to occur every timestep, HEU is not
    // 'present' until timestep 4 because that is the first time it's removed
    // from the cascade to be shipped.

 std::string config = 
    "   <feed_commod>natu</feed_commod> "
    "   <feed_recipe>natu1</feed_recipe> "
    "   <product_commod>enr_u</product_commod> "
    "   <tails_commod>tails</tails_commod> "
    "   <tails_assay>0.002</tails_assay> "
    "   <heu_ship_qty>0.015</heu_ship_qty> "
    "   <inspect_freq>1</inspect_freq> "
    "   <n_swipes>10</n_swipes> " ;

  // time 1-source to EF, 2-Enrich to sink 0.0056kg,
  // 3- Enrich to sink 0.011kg, 4 - Enrich to sink 0.016kg > heu_ship_qty
  int simdur = 4;
  cyclus::MockSim sim(cyclus::AgentSpec
		      (":mbmore:RandomEnrich"), config, simdur);
  sim.AddRecipe("natu1", c_natu1());
  sim.AddRecipe("enr_u", c_heu90()); 
  
  sim.AddSource("natu")
    .recipe("natu1")
    .capacity(1.0)
    .Finalize();
  sim.AddSink("enr_u")
    .recipe("enr_u")
    .Finalize();
  
  int id = sim.Run();
  std::vector<Cond> conds;
  conds.push_back(Cond("SampleLoc", "==", std::string("Cascade")));
  QueryResult qr = sim.db().Query("Inspections", &conds);
  int n_inspect = qr.rows.size();

  double swipe_tot=0.0;
  for (int it=0; it < n_inspect; it++){
    swipe_tot += qr.GetVal<double>("PosSwipeFrac", it);
  }
  EXPECT_EQ(swipe_tot, 1);
  
  } 

} // namespace randomenrichtests
} // namespace mbmore
