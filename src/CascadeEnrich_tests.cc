#include "CascadeEnrich_tests.h"

#include <gtest/gtest.h>

#include <sstream>

#include "agent_tests.h"
#include "env.h"
#include "facility_tests.h"
#include "infile_tree.h"
#include "resource_helpers.h"
#include "toolkit/mat_query.h"

using cyclus::QueryResult;
using cyclus::Cond;
using cyclus::CompMap;
using cyclus::toolkit::MatQuery;
using pyne::nucname::id;
using cyclus::Material;

namespace mbmore {

namespace cascadenrichtest {

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
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CascadeEnrichTest, RequestQty) {
  // this tests verifies that requests for input material are fulfilled
  // without providing any extra

  std::string config =
      "   <feed_commod>natu</feed_commod> "
      "   <feed_recipe>natu1</feed_recipe> "
      "   <product_commod>enr_u</product_commod> "
      "   <tails_commod>tails</tails_commod> "
      "   <tails_assay>0.003</tails_assay> "
      "   <max_feed_inventory>1000</max_feed_inventory> ";

  int simdur = 1;
  cyclus::MockSim sim(cyclus::AgentSpec(":mbmore:CascadeEnrich"), config,
                      simdur);
  sim.AddRecipe("natu1", cascadenrichtest::c_natu1());

  sim.AddSource("natu").recipe("natu1").Finalize();

  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("natu")));
  QueryResult qr = sim.db().Query("Transactions", &conds);
  Material::Ptr m = sim.GetMaterial(qr.GetVal<int>("ResourceId"));

  // Should be only one transaction into the EF,
  // and it should be exactly 207.8928179411368kg of natu
  EXPECT_EQ(1, qr.rows.size());
  EXPECT_NEAR(1000, m->quantity(), 1e-3)
      << "matched trade provides the wrong quantity of material";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CascadeEnrichTest, CheckFlowConstraint) {
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
      "   <initial_feed>1000</initial_feed> ";

  int simdur = 1;

  cyclus::MockSim sim(cyclus::AgentSpec(":mbmore:CascadeEnrich"), config,
                      simdur);

  sim.AddRecipe("natu1", cascadenrichtest::c_natu1());
  sim.AddRecipe("heu", cascadenrichtest::c_heu());

  sim.AddSink("enr_u").recipe("heu").capacity(10).Finalize();

  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("enr_u")));
  QueryResult qr = sim.db().Query("Transactions", &conds);
  Material::Ptr m = sim.GetMaterial(qr.GetVal<int>("ResourceId"));

  EXPECT_EQ(1.0, qr.rows.size());
  EXPECT_NEAR(10, m->quantity(), 0.1)
      << "traded quantity differ from flow contraints";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CascadeEnrichTest, CheckCapConstraint) {
  // Tests that a request for more material than is available in
  // inventory is partially filled with only the inventory quantity.

  std::string config =
      "   <feed_commod>natu</feed_commod> "
      "   <feed_recipe>natu1</feed_recipe> "
      "   <product_commod>enr_u</product_commod> "
      "   <tails_commod>tails</tails_commod> "
      "   <tails_assay>0.003</tails_assay> "
      "   <initial_feed>200</initial_feed> "
      "   <design_feed_flow>50</design_feed_flow> ";

  int simdur = 1;

  cyclus::MockSim sim(cyclus::AgentSpec(":mbmore:CascadeEnrich"), config,
                      simdur);

  sim.AddRecipe("natu1", cascadenrichtest::c_natu1());
  sim.AddRecipe("heu", cascadenrichtest::c_heu());

  sim.AddSink("enr_u").recipe("heu").capacity(10).Finalize();

  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("enr_u")));
  QueryResult qr = sim.db().Query("Transactions", &conds);
  Material::Ptr m = sim.GetMaterial(qr.GetVal<int>("ResourceId"));

  EXPECT_EQ(1.0, qr.rows.size());
  EXPECT_NEAR(5.831, m->quantity(), 0.01)
      << "traded quantity exceeds capacity constraint";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CascadeEnrichTest, TradeTails) {
  // this tests whether tails are being traded.

  std::string config =
      "   <feed_commod>natu</feed_commod> "
      "   <feed_recipe>natu1</feed_recipe> "
      "   <product_commod>enr_u</product_commod> "
      "   <tails_commod>tails</tails_commod> "
      "   <tails_assay>0.003</tails_assay> ";

  // time 1-source to EF, 2-Enrich, add to tails, 3-tails avail. for trade
  int simdur = 3;
  cyclus::MockSim sim(cyclus::AgentSpec(":mbmore:CascadeEnrich"), config,
                      simdur);
  sim.AddRecipe("natu1", cascadenrichtest::c_natu1());
  sim.AddRecipe("leu", cascadenrichtest::c_leu());

  sim.AddSource("natu").recipe("natu1").capacity(200).Finalize();
  sim.AddSink("enr_u").recipe("leu").Finalize();
  sim.AddSink("tails").Finalize();

  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("tails")));
  QueryResult qr = sim.db().Query("Transactions", &conds);

  // Should be exactly one tails transaction
  EXPECT_EQ(1, qr.rows.size());
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CascadeEnrichTest, TailsQty) {
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
  cyclus::MockSim sim(cyclus::AgentSpec(":mbmore:CascadeEnrich"), config,
                      simdur);
  sim.AddRecipe("natu1", cascadenrichtest::c_natu1());
  sim.AddRecipe("leu", cascadenrichtest::c_leu());

  sim.AddSource("natu").recipe("natu1").capacity(200).Finalize();
  sim.AddSink("enr_u").recipe("leu").capacity(0.5).Finalize();
  sim.AddSink("enr_u").recipe("leu").capacity(0.5).Finalize();
  sim.AddSink("tails").Finalize();

  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("tails")));
  QueryResult qr = sim.db().Query("Transactions", &conds);
  Material::Ptr m = sim.GetMaterial(qr.GetVal<int>("ResourceId"));

  // Should be 2 tails transactions, one from each LEU sink, each 3.787453kg.
  // (1* (0.0403193-0.00271456) / (0.0071-0.00271456) -1)/2. = 3.787453
  EXPECT_EQ(2, qr.rows.size());

  cyclus::SqlStatement::Ptr stmt = sim.db().db().Prepare(
      "SELECT SUM(r.Quantity) FROM Transactions AS t"
      " INNER JOIN Resources AS r ON r.ResourceId = t.ResourceId"
      " WHERE t.Commodity = ?;");

  stmt->BindText(1, "tails");
  stmt->Step();
  EXPECT_NEAR(7.575, stmt->GetDouble(0), 0.01)
      << "Not providing the requested quantity";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CascadeEnrichTest, BidPrefs) {
  // This tests that natu sources are preference-ordered by
  // U235 content

  std::string config =
      "   <feed_commod>natu</feed_commod> "
      "   <feed_recipe>natu2</feed_recipe> "
      "   <product_commod>enr_u</product_commod> "
      "   <tails_commod>tails</tails_commod> "
      "   <tails_assay>0.003</tails_assay> "
      "   <max_feed_inventory>1.0</max_feed_inventory> ";

  int simdur = 1;
  cyclus::MockSim sim(cyclus::AgentSpec(":mbmore:CascadeEnrich"), config,
                      simdur);
  sim.AddRecipe("natu1", cascadenrichtest::c_natu1());
  sim.AddRecipe("natu2", cascadenrichtest::c_natu2());

  sim.AddSource("natu").recipe("natu1").capacity(200).Finalize();
  sim.AddSource("natu").recipe("natu1").capacity(1).Finalize();

  sim.AddSource("natu").recipe("natu2").capacity(2).Finalize();

  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("natu")));
  QueryResult qr = sim.db().Query("Transactions", &conds);

  // should trade only with #2 since it has higher U235
  EXPECT_EQ(1, qr.rows.size());

  Material::Ptr m = sim.GetMaterial(qr.GetVal<int>("ResourceId"));
  CompMap got = m->comp()->mass();
  CompMap want = cascadenrichtest::c_natu2()->mass();
  cyclus::compmath::Normalize(&got);
  cyclus::compmath::Normalize(&want);

  CompMap::iterator it;
  for (it = want.begin(); it != want.end(); ++it) {
    EXPECT_DOUBLE_EQ(it->second, got[it->first])
        << "nuclide qty off: " << pyne::nucname::name(it->first);
  }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CascadeEnrichTest, NoBidPrefs) {
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
  cyclus::MockSim sim(cyclus::AgentSpec(":mbmore:CascadeEnrich"), config,
                      simdur);
  sim.AddRecipe("natu1", cascadenrichtest::c_natu1());
  sim.AddRecipe("natu2", cascadenrichtest::c_natu2());

  sim.AddSource("natu").recipe("natu1").capacity(1).Finalize();

  sim.AddSource("natu").recipe("natu2").capacity(1).Finalize();

  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("natu")));
  QueryResult qr = sim.db().Query("Transactions", &conds);

  // should trade with both to meet its capacity limit
  EXPECT_EQ(2, qr.rows.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CascadeEnrichTest, ZeroU235) {
  // Test that offers of natu with no u235 content are rejected

  std::string config =
      "   <feed_commod>natu</feed_commod> "
      "   <feed_recipe>natu1</feed_recipe> "
      "   <product_commod>enr_u</product_commod> "
      "   <tails_commod>tails</tails_commod> "
      "   <tails_assay>0.003</tails_assay> "
      "   <max_feed_inventory>1.0</max_feed_inventory> ";

  int simdur = 1;
  cyclus::MockSim sim(cyclus::AgentSpec(":mbmore:CascadeEnrich"), config,
                      simdur);
  sim.AddRecipe("no_u235", cascadenrichtest::c_nou235());
  sim.AddRecipe("natu1", cascadenrichtest::c_natu1());

  sim.AddSource("natu").recipe("no_u235").capacity(1).Finalize();

  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("natu")));
  // DB table should be empty since there are no transactions
  EXPECT_THROW(sim.db().Query("Transactions", &conds), std::exception);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CascadeEnrichTest::SetUp() {
  cyclus::Env::SetNucDataPath();
  cyclus::Context* ctx = tc_.get();
  src_facility = new CascadeEnrich(ctx);
  trader = tc_.trader();
  InitParameters();
  SetUpSource();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CascadeEnrichTest::TearDown() { delete src_facility; }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CascadeEnrichTest::InitParameters() {
  cyclus::Context* ctx = tc_.get();

  feed_commod = "incommod";
  product_commod = "outcommod";
  tails_commod = "tailscommod";

  feed_recipe = "recipe";
  feed_assay = 0.0072;

  cyclus::CompMap v;
  v[922350000] = feed_assay;
  v[922380000] = 1 - feed_assay;
  recipe = cyclus::Composition::CreateFromAtom(v);
  ctx->AddRecipe(feed_recipe, recipe);

  tails_assay = 0.002;
  swu_capacity = 100;  //**
  inv_size = 105.5;

  reserves = 105.5;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CascadeEnrichTest::SetUpSource() {
  src_facility->feed_recipe = feed_recipe;
  src_facility->feed_commod = feed_commod;
  src_facility->product_commod = product_commod;
  src_facility->tails_commod = tails_commod;
  src_facility->tails_assay = tails_assay;
  src_facility->max_enrich = max_enrich;
  src_facility->SetMaxInventorySize(inv_size);
  src_facility->initial_feed = reserves;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::Material::Ptr CascadeEnrichTest::GetMat(double qty) {
  return cyclus::Material::CreateUntracked(qty,
                                           tc_.get()->GetRecipe(feed_recipe));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CascadeEnrichTest::DoAddMat(cyclus::Material::Ptr mat) {
  src_facility->AddMat_(mat);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::Material::Ptr CascadeEnrichTest::DoRequest() {
  return src_facility->Request_();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::Material::Ptr CascadeEnrichTest::DoOffer(cyclus::Material::Ptr mat) {
  return src_facility->Offer_(mat);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::Material::Ptr CascadeEnrichTest::DoEnrich(cyclus::Material::Ptr mat,
                                                  double qty) {
  return src_facility->Enrich_(mat, qty);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CascadeEnrichTest, Request) {
  // Tests that quantity in material request is accurate
  double req = inv_size;
  double add = 0;
  cyclus::Material::Ptr mat = DoRequest();
  EXPECT_DOUBLE_EQ(mat->quantity(), req);
  EXPECT_EQ(mat->comp(), tc_.get()->GetRecipe(feed_recipe));

  add = 2 * inv_size / 3;
  req -= add;
  DoAddMat(GetMat(add));
  mat = DoRequest();
  EXPECT_DOUBLE_EQ(mat->quantity(), req);
  EXPECT_EQ(mat->comp(), tc_.get()->GetRecipe(feed_recipe));

  add = inv_size / 3;
  req = 0;
  DoAddMat(GetMat(add));
  mat = DoRequest();
  EXPECT_DOUBLE_EQ(mat->quantity(), req);
  EXPECT_EQ(mat->comp(), tc_.get()->GetRecipe(feed_recipe));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CascadeEnrichTest, ValidReq) {
  // Tests that material requests have U235/(U235+U238) > tails assay
  using cyclus::CompMap;
  using cyclus::Composition;
  using cyclus::Material;

  double qty = 4.5;  // some magic number

  cyclus::CompMap v1;
  v1[922350000] = 1;
  Material::Ptr mat =
      Material::CreateUntracked(qty, Composition::CreateFromAtom(v1));
  EXPECT_FALSE(src_facility->ValidReq(mat));  // u238 = 0

  cyclus::CompMap v2;
  v2[922350000] = tails_assay;
  v2[922380000] = 1 - tails_assay;
  mat = Material::CreateUntracked(qty, Composition::CreateFromAtom(v2));
  // u235 / (u235 + u238) <= tails_assay
  EXPECT_FALSE(src_facility->ValidReq(mat));

  cyclus::CompMap v3;
  v3[922350000] = 1;
  v3[922380000] = 1;
  mat = Material::CreateUntracked(qty, Composition::CreateFromAtom(v3));
  EXPECT_TRUE(src_facility->ValidReq(mat));  // valid
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

}  // namespace cycamore

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::Agent* CascadeEnrichConstructor(cyclus::Context* ctx) {
  return new mbmore::CascadeEnrich(ctx);
}
/*
// required to get functionality in cyclus agent unit tests library
#ifndef CYCLUS_AGENT_TESTS_CONNECTED
int ConnectAgentTests();
static int cyclus_agent_tests_connected = ConnectAgentTests();
#define CYCLUS_AGENT_TESTS_CONNECTED cyclus_agent_tests_connected
#endif  // CYCLUS_AGENT_TESTS_CONNECTED

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
INSTANTIATE_TEST_CASE_P(CascadeEnrichFac, FacilityTests,
                        Values(&CascadeEnrichConstructor));
INSTANTIATE_TEST_CASE_P(CascadeEnrichFac, AgentTests,
                        Values(&CascadeEnrichConstructor));
*/
