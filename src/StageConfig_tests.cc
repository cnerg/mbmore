#include <gtest/gtest.h>

#include "StageConfig.h"

#include "agent_tests.h"
#include "context.h"
#include "facility_tests.h"

namespace mbmore {

// Benchmarked using a regression test (expected values calculated manually)
// Calculations can be found at CNERG/enrich_calc:mbmore_unit_tests
namespace stageconfig_test {
// Fixed for a cascade separating out U235 from U238 in UF6 gas
double M = 0.352;   // kg/mol UF6
double dM = 0.003;  // kg/mol U238 - U235
double x = 1000;    // Pressure ratio (Glaser)

// General cascade assumptions
double flow_ratio = 2.0;
double eff = 1.0;
double cut = 0.5;

// Centrifgue parameters based on Glaser SGS 2009 paper
double v_a = 485;                                           // m/s
double height = 0.5;                                        // meters
double diameter = 0.15;                                     // meters
double feed_m = 15 * 60 * 60 / ((1e3) * 60 * 60 * 1000.0);  // kg/sec
double temp = 320.0;                                        // Kelvin

// Cascade params used in in calculating expected values
const double feed_assay = 0.0071;
const double prod_assay = 0.035;
const double waste_assay = 0.001;
const double feed_c = 739 / (30.4 * 24 * 60 * 60);    // kg/month -> kg/sec
const double product_c = 77 / (30.4 * 24 * 60 * 60);  // kg/month -> kg/sec
CentrifugeConfig centrifuge(v_a, height, diameter, feed_m, temp, eff, M, dM, x,
                            flow_ratio);

// del U=8.638345e-08 alpha=1.130517
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
  stage.ProductAssay();

  // N_prime = alpha*R / ( 1+alpha*R)
  double target_prod_assay = 0.009773;
  double tol = 1e-6;

  EXPECT_NEAR(stage.product_assay(), target_prod_assay, tol);

  double n_stages = 5;
  double target_w_assay = 0.004227;
  stage.BetaByAlphaAndCut();
  stage.TailAssay();

  EXPECT_NEAR(stage.tail_assay(), target_w_assay, tol);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Calculate ideal SWU params of single machinefeed_assay
// (separation potential delU and separation factor alpha)
TEST(StageConfig_Test, TestSWU) {
  double expected_U = 8.638345e-08;
  double tol = 1e-9;

  StageConfig stage(feed_assay, feed_m, cut, delU, -1, 1e-16);

  double expected_alpha = 1.130517;
  double tol_alpha = 1e-2;
  EXPECT_NEAR(stage.alpha(), expected_alpha, tol_alpha);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Calculate the product assay for an ideal stage configuration.
TEST(StageConfig_Test, TestIdeal) {
  StageConfig stage_ideal(feed_assay, feed_m, cut, -1, -1, 1e-16);

  // Only setting precision for building ideal stage
  stage_ideal.precision(1e-3);
  stage_ideal.BuildIdealStg();
  stage_ideal.precision(1e-16);

  double expected_alpha = 1.130517;
  double tol_alpha = 1e-2;

  // These expected values are found by regression test
  // and are working as intended as of committing this line.
  double expected_cut = 0.4685952;
  double tol_cut = 1e-3;
  double expected_U = 8.281007e-08;
  double tol_DU = 1e-9;

  EXPECT_NEAR(stage_ideal.alpha(), expected_alpha, tol_alpha);
  EXPECT_NEAR(stage_ideal.cut(), expected_cut, tol_cut);
  EXPECT_NEAR(stage_ideal.DU(), expected_U, tol_DU);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(StageConfig_Test, ProductAssayByGamma) {
  double gamma = 1.3798316056650026;
  double target_product_assay = 0.00822;
  double theta_ = 0.46040372309;
  double feed_assay_ = 0.007;
  StageConfig stage(feed_assay_, feed_c, theta_, delU, -1, 1e-16);
  stage.ProductAssayByGamma(gamma);

  EXPECT_NEAR(target_product_assay,stage.product_assay(), 1e-5);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(StageConfig_Test, AlphaByProductAssay) {
  double gamma = 1.3798316056650026;
  double target_product_assay = 0.00821;
  double theta_ = 0.46040372309;
  double feed_assay_ = 0.007;
  StageConfig stage(feed_assay_, feed_c, theta_, delU, -1, 1e-16);
  stage.feed_assay(0.1);
  stage.product_assay(0.3);
  double alpha_ = 0.3/(1-0.3)*(1-0.1)/0.1;
  stage.AlphaByProductAssay();

  EXPECT_NEAR(alpha_,stage.alpha(), 1e-5);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Determine the output of the first enrich/strip stage of a cascade
// based on the design params for the cascade
TEST(StageConfig_Test, TestStages) {
  StageConfig stage(feed_assay, feed_c, cut, delU, -1, 1e-16);

  stage.ProductAssay();
  stage.MachinesNeededPerStage();

  double expected_product_assay_s = 0.0080192;

  // Calculated using equations from 2009 Glaser paper
  int expected_n_mach_e = ceil(feed_c/feed_m);

  EXPECT_NEAR(stage.n_machines(), expected_n_mach_e, tol_num);
  EXPECT_NEAR(stage.product_assay(), expected_product_assay_s, tol_assay);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Verify that, for a P1-type centrifuge, results are in agreement
// with those presented by Glaser's paper.
TEST(StageConfig_Test, AlphaBeta) {
  // P1-type parameters
  v_a = 320; // m/s
  eff = 0.564;
  x = 1000.;
  flow_ratio = 2.0;
  temp = 320; // K
  M = 0.352;   // kg/mol UF6
  dM = 0.003;  // kg/mol U238 - U235
  cut = 0.5;
  double Z = 1.8; // m
  double d = 0.10; // m
  double feed = 12.6 / 1000. / 1000.; // mg/s -> kg/s
  double f_assay = 0.00720;

  CentrifugeConfig cent(v_a, Z, d, feed, temp, eff, M, dM, x, flow_ratio);

  double delU = cent.ComputeDeltaU(cut);

  // Glaser's paper value is delU_e = 2.5 / (365.25 * 24 * 60 * 60)
  // The answer below is apprx. 6% different.
  // WIP to reconcile the difference.
  double delU_e = 8.40508e-8;
  EXPECT_NEAR(delU, delU_e, 1e-8);

  StageConfig stg(f_assay, feed, cut, delU);

  double ab_e = 1.29;
  double tol_ab = 0.01;
  EXPECT_NEAR(stg.alpha() * stg.beta(), ab_e, tol_ab);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Verify that the centrifuge feed flow can be reconstructed if alpha
// is not provided.
TEST(StageConfig_Test, FeedFlow) {
  StageConfig stage(feed_assay, feed_c, cut, delU, -1, 1e-16);

  double M = centrifuge.M;  // UF6 kg/mol
  double M_238 = 0.238;     // U kg/mol
  double ratio_UF6_U = M_238 / M;

  // "Uranium Enrichment By Gas Centrifuge" D.G. Avery & E. Davies pg. 18
  double centrifuge_feed_flow =
      2 * stage.DU() * ((1 - stage.cut()) / stage.cut())
      / pow((stage.alpha() - 1.), 2.) / ratio_UF6_U;

  EXPECT_NEAR(centrifuge_feed_flow,stage.centrifuge.feed,1e-6);
}

}  // namespace enrichfunctiontests
}  // namespace mbmore
