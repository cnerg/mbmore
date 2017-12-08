#include <gtest/gtest.h>

#include "CascadeConfig.h"

#include "agent_tests.h"
#include "context.h"
#include "facility_tests.h"

namespace mbmore {

// Benchmarked against mbmore_enrich_compare.ipynb
// https://github.com/mbmcgarry/data_analysis/tree/master/enrich_calcs
namespace cascadeconfig_tests {
// Fixed for a cascade separating out U235 from U238 in UF6 gas
const double M = 0.352;   // kg/mol UF6
const double dM = 0.003;  // kg/mol U238 - U235
const double x = 1000;    // Pressure ratio (Glaser)

// General cascade assumptions
const double flow_internal = 2.0;
const double eff = 1.0;
const double cut = 0.5;

// Centrifuge params used in Python test code
// (based on Glaser SGS 2009 paper)
const double v_a = 485;                                           // m/s
const double height = 0.5;                                        // meters
const double diameter = 0.15;                                     // meters
const double feed_m = 15 * 60 * 60 / ((1e3) * 60 * 60 * 1000.0);  // kg/sec
const double temp = 320.0;                                        // Kelvin

// Cascade params used in Python test code (Enrichment_Calculations.ipynb)
const double feed_assay = 0.0071;
const double product_assay = 0.035;
const double waste_assay = 0.001;
const double feed_c = 739 / (30.4 * 24 * 60 * 60);    // kg/month -> kg/sec
const double product_c = 77 / (30.4 * 24 * 60 * 60);  // kg/month -> kg/sec
CentrifugeConfig centrifuge(v_a, height, diameter, feed_m, temp, eff, M, dM, x,
                            flow_internal);
// del U=7.0323281e-08 alpha=1.16321
double delU = centrifuge.ComputeDeltaU(cut);

const double tol_assay = 1e-5;
const double tol_qty = 1e-6;
const double tol_num = 1e-2;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Ideal cascade design, and then using away from ideal design

TEST(Cascade_Test, TestCascade) {
  
  CascadeConfig cascade;
  cascade.centrifuge = centrifuge;
  cascade.BuildIdealCascade(feed_assay, product_assay, waste_assay, 1e-8);
  int pycode_n_enrich_stage = 11;
  int pycode_n_strip_stage = 12;
  //  integer
  int n_stage_enrich = cascade.n_enrich;
  int n_stage_waste = cascade.n_strip;

  EXPECT_EQ(n_stage_enrich, pycode_n_enrich_stage);
  EXPECT_EQ(n_stage_waste, pycode_n_strip_stage);

  // Now test assays when cascade is modified away from ideal design
  // (cascade optimized for natural uranium feed, now use 20% enriched
  double feed_assay_mod = 0.20;
  cascade.DesignCascade(feed_c, 1000000);
  CascadeConfig cascade_non_ideal =
      cascade.Compute_Assay(feed_assay_mod, 1e-31);

  double mod_product_assay =
      cascade_non_ideal.stgs_config[n_stage_enrich - 1].product_assay;
  double mod_waste_assay =
      cascade_non_ideal.stgs_config[-n_stage_waste].product_assay;

  double pycode_mod_product_assay = 0.8189;
  EXPECT_NEAR(mod_product_assay, pycode_mod_product_assay, tol_assay);

  double pycode_mod_waste_assay = 0.11198;
  EXPECT_NEAR(mod_waste_assay, pycode_mod_waste_assay, tol_assay);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// tests the steady state flow rates for a cascade
//
TEST(Cascade_Test, TestCascadeDesign) {
  double fa = 0.10;
  double pa = 0.20;
  double wa = 0.05;

  std::vector<double> pycode_flows = {
      0.00030693, 0.00061387, 0.0009208,  0.00122774, 0.00153467,
      0.00127889, 0.00102311, 0.00076734, 0.00051156, 0.00025578};

  std::vector<int> pycode_machines = {80,  149, 210, 264, 312,
                                      241, 180, 127, 80,  38};

  CascadeConfig cascade(centrifuge, fa, pa, wa, feed_c, 1000000);

  for (int i = 0; i < pycode_flows.size(); i++) {
    EXPECT_NEAR(cascade.stgs_config[i - cascade.n_strip].feed_flow, pycode_flows[i], tol_num);
    int nmach = cascade.stgs_config[i - cascade.n_strip].n_machines;
    EXPECT_EQ(nmach, pycode_machines[i]);
  }

  // not enough machines
  int max_centrifuges = 80;
  cascade.DesignCascade(feed_c, max_centrifuges);
  int py_tot_mach = 80;
  double py_opt_feed = 1.30116169899e-05;

  EXPECT_EQ(py_tot_mach, cascade.FindTotalMachines());
  EXPECT_NEAR(py_opt_feed, cascade.FeedFlow(), tol_qty);

  // more machines than requested capacity
  max_centrifuges = 1000;
  cascade.DesignCascade(feed_c, max_centrifuges);
  py_tot_mach = 999;
  py_opt_feed = 0.0001667;

  EXPECT_EQ(py_tot_mach, cascade.FindTotalMachines());
  EXPECT_NEAR(py_opt_feed, cascade.FeedFlow(), tol_qty);
}

TEST(Cascade_Test, TestUpdateAssay) {
  double fa = 0.10;
  double pa = 0.20;
  double wa = 0.05;

  CascadeConfig cascade(centrifuge,fa, pa, wa, feed_c, 100);
  double product_assay =
      cascade.stgs_config[cascade.n_enrich - 1].product_assay;
  double tail_assay = cascade.stgs_config[-cascade.n_strip].tail_assay;
  double product_flow = cascade.stgs_config[cascade.n_enrich - 1].feed_flow *
                        cascade.stgs_config[cascade.n_enrich - 1].cut;
  double tail_flow = cascade.stgs_config[-cascade.n_strip].feed_flow *
                     (1 - cascade.stgs_config[-cascade.n_strip].cut);

  double feed_from_assay =
      product_flow * (product_assay - tail_assay) / (fa - tail_assay);
  double tail_from_assay =
      product_flow * (product_assay - fa) / (fa - tail_assay);

  EXPECT_NEAR(cascade.FeedFlow(), feed_from_assay, 1e-3);
  EXPECT_NEAR(tail_flow, tail_from_assay, 1e-3);

  fa = 0.2;
  cascade = cascade.Compute_Assay(fa, 1e-17);
  product_assay = cascade.stgs_config[cascade.n_enrich - 1].product_assay;
  tail_assay = cascade.stgs_config[-cascade.n_strip].tail_assay;
  product_flow = cascade.stgs_config[cascade.n_enrich - 1].feed_flow *
                 cascade.stgs_config[cascade.n_enrich - 1].cut;
  tail_flow = cascade.stgs_config[-cascade.n_strip].feed_flow *
              (1 - cascade.stgs_config[-cascade.n_strip].cut);
  feed_from_assay =
      product_flow * (product_assay - tail_assay) / (fa - tail_assay);
  tail_from_assay =
      product_flow * (product_assay - fa) / (fa - tail_assay);

  EXPECT_NEAR(cascade.FeedFlow(), feed_from_assay, 1e-3);
  EXPECT_NEAR(tail_flow, tail_from_assay, 1e-3);
}

}  // namespace cascadeconfig_tests
}  // namespace mbmore
