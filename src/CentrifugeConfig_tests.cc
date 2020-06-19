#include <gtest/gtest.h>

#include "CentrifugeConfig.h"

#include "context.h"

namespace mbmore {

namespace centrifugeconfig_test {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Calculate ideal SWU params of single machine (separation potential delU
// and separation factor alpha)
TEST(CentrifugeConfig_Test, TestSWU) {
  // Fixed for a cascade separating out U235 from U238 in UF6 gas
  double M = 0.352;   // kg/mol UF6
  double dM = 0.003;  // kg/mol U238 - U235
  double x = 1000;    // Pressure ratio (Glaser)

  // General cascade assumptions
  double flow_ratio = 2.0;
  double eff = 1.0;
  double cut = 0.5;

  // Centrifuge params used in Python test code
  // found at CNERG/enrich_calc:mbmore_unit_tests
  // (based on Glaser SGS 2009 paper)
  double v_a = 485;                                           // m/s
  double height = 0.5;                                        // meters
  double diameter = 0.15;                                     // meters
  double feed_m = 15 * 60 * 60 / ((1e3) * 60 * 60 * 1000.0);  // kg/sec
  double temp = 320.0;                                        // Kelvin

  CentrifugeConfig centrifuge(v_a, height, diameter, feed_m, temp, eff, M, dM,
                              x, flow_ratio);
  // dU=8.638345e-08 alpha=1.130517
  double delU = centrifuge.ComputeDeltaU(cut);

  double pycode_U = 8.638345e-08;
  double tol = 1e-9;

  EXPECT_NEAR(delU, pycode_U, tol);
}

}  // namespace centrifugeconfig_test
}  // namespace mbmore
