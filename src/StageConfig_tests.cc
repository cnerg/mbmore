#include <gtest/gtest.h>

#include "StageConfig.h"

#include "agent_tests.h"
#include "context.h"
#include "facility_tests.h"

namespace mbmore {

// Benchmarked against mbmore_enrich_compare.ipynb
// https://github.com/mbmcgarry/data_analysis/tree/master/enrich_calcs
namespace stageconfig_test {
// Fixed for a cascade separating out U235 from U238 in UF6 gas
// Fixed for a cascade separating out U235 from U238 in UF6 gas
double M = 0.352;   // kg/mol UF6
double dM = 0.003;  // kg/mol U238 - U235
double x = 1000;    // Pressure ratio (Glaser)

// General cascade assumptions
double flow_internal = 2.0;
double eff = 1.0;
double cut = 0.5;

// Centrifuge params used in Python test code
// (based on Glaser SGS 2009 paper)
double v_a = 485;                                           // m/s
double height = 0.5;                                        // meters
double diameter = 0.15;                                     // meters
double feed_m = 15 * 60 * 60 / ((1e3) * 60 * 60 * 1000.0);  // kg/sec
double temp = 320.0;                                        // Kelvin

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
// Find product assay from separation factor alpha
TEST(StageConfig_Test, TestAssays) {
  double cur_alpha = 1.4;
  double cur_f_assay = 0.007;

  StageConfig stage(cur_f_assay, feed_m, 1e-16, cut, delU, cur_alpha);
  double cpp_assay = stage.ProductAssay();

  double pycode_assay = 0.009772636;
  double tol = 1e-6;

  EXPECT_NEAR(cpp_assay, pycode_assay, tol);

  double n_stages = 5;
  double pycode_w_assay = 0.030762976143215673;
  double cur_beta = stage.BetaByAlphaAndCut();
  double cpp_w_assay = stage.TailAssay();

  EXPECT_NEAR(cpp_w_assay, pycode_w_assay, tol);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Calculate ideal SWU params of single machine (separation potential delU
// and separation factor alpha)
TEST(StageConfig_Test, TestSWU) {
  double pycode_U = 7.03232816847e-08;
  double tol = 1e-9;
  
  StageConfig stage(feed_assay, feed_m, 1e-16, cut, delU, -1);
  
  double pycode_alpha = 1.16321;
  double tol_alpha = 1e-2;
  EXPECT_NEAR(stage.alpha, pycode_alpha, tol_alpha);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Determine the output of the first enrich/strip stage of a cascade
// based on the design params for the cascade
TEST(StageConfig_Test, TestStages) {
  StageConfig stage(feed_assay, feed_c, 1e-16, cut, delU, -1);
  
  double product_assay_s = stage.ProductAssay();
  double n_mach_e = stage.MachinesPerStage();
  double product_s = stage.ProductPerEnrStage();

  double enrich_waste = feed_c - product_s;
  double enrich_waste_assay =
      (feed_c * feed_assay - product_s * product_assay_s) / enrich_waste;

  double pycode_product_assay_s = 0.0082492;
  double pycode_n_mach_e = 53.287;
  double pycode_product_s = 0.0001408;

  EXPECT_NEAR(n_mach_e, pycode_n_mach_e, tol_num);
  EXPECT_NEAR(product_assay_s, pycode_product_assay_s, tol_assay);
  EXPECT_NEAR(product_s, pycode_product_s, tol_qty);

  stage = StageConfig(feed_assay, enrich_waste, 1e-16, cut, delU, -1);
  double n_mach_w = stage.MachinesPerStage();
  double strip_waste_assay = stage.TailAssay();

  double pycode_n_mach_w = 26.6127;
  double pycode_waste_assay_s = 0.005117;

  EXPECT_NEAR(n_mach_w, pycode_n_mach_w, tol_num);
  EXPECT_NEAR(strip_waste_assay, pycode_waste_assay_s, tol_assay);
}


}  // namespace enrichfunctiontests
}  // namespace mbmore
