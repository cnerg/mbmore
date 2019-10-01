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
// found in enrich_calcs repo listed above.
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

  StageConfig stage(cur_f_assay, feed_m, cut, delU, cur_alpha, 1e-16);
  double cal_prod_assay = stage.ProductAssay();

  // N_prime = alpha*R / ( 1+alpha*R)
  double target_prod_assay = 0.009773;
  double tol = 1e-6;

  EXPECT_NEAR(cal_prod_assay, target_prod_assay, tol);

  double n_stages = 5;
  double target_w_assay = 0.004227;
  double cur_beta = stage.BetaByAlphaAndCut();
  double cal_w_assay = stage.TailAssay();

  EXPECT_NEAR(cal_w_assay, target_w_assay, tol);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Calculate ideal SWU params of single machinefeed_assay (separation potential delU
// and separation factor alpha)
TEST(StageConfig_Test, TestSWU) {
  double pycode_U = 7.03232816847e-08;
  double tol = 1e-9;

  StageConfig stage(feed_assay, feed_m, cut, delU, -1, 1e-16);

  double pycode_alpha = 1.16321;
  double tol_alpha = 1e-2;
  EXPECT_NEAR(stage.alpha, pycode_alpha, tol_alpha);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Calculate the product assay for an ideal stage configuration.
TEST(StageConfig_Test, TestIdealStage) {
  StageConfig stage_ideal(feed_assay, feed_m, cut, -1, -1, 1e-16);

  stage_ideal.BuildIdealStg(feed_assay,1e-3);

  double pycode_alpha = 1.16321;
  double tol_alpha = 1e-2;

  double pycode_cut = 0.5;
  double tol_cut = 1e-3;

  double pycode_U = 7.03232816847e-08;
  double tol_DU = 1e-9;

  EXPECT_NEAR(stage_ideal.alpha, pycode_alpha, tol_alpha);
  EXPECT_NEAR(stage_ideal.cut, pycode_cut, tol_cut);
  EXPECT_NEAR(stage_ideal.DU, pycode_U, tol_DU);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Determine the output of the first enrich/strip stage of a cascade
// based on the design params for the cascade
TEST(StageConfig_Test, TestStages) {
  //StageConfig stage(feed_assay, feed_c, cut, delU, -1, 1e-16);
  StageConfig stage(centrifuge, feed_assay, feed_c, 1e-16);

  stage.AlphaByDU();

  double product_assay_s = stage.ProductAssay();
  double n_mach_e = stage.MachinesNeededPerStage();

  double pycode_product_assay_s = 0.0082492;
  double pycode_n_mach_e = 53.287;

  EXPECT_NEAR(n_mach_e, pycode_n_mach_e, tol_num);
  EXPECT_NEAR(product_assay_s, pycode_product_assay_s, tol_assay);
}

}  // namespace enrichfunctiontests
}  // namespace mbmore
