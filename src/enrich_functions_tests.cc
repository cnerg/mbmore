#include <gtest/gtest.h>

#include "enrich_functions.h"

#include "agent_tests.h"
#include "context.h"
#include "facility_tests.h"

namespace mbmore {
  
  namespace enrichfunctiontests {
    // Fixed for a cascade separating out U235 from U238 in UF6 gas
    const double M = 0.352; // kg/mol UF6
    const double dM = 0.003; // kg/mol U238 - U235
    const double x = 1000;  // Pressure ratio (Glaser)
    
    // General cascade assumptions
    const double flow_internal = 2.0 ;  
    const double eff = 1.0;  
    const double cut = 0.5;  
    
    // Centrifuge params used in Python test code
    // (based on Glaser SGS 2009 paper)
    const double v_a = 485; // m/s
    const double height = 0.5; // meters
    const double diameter = 0.15; // meters
    const double feed_m = 15 * 60 * 60 / ((1e3) * 60 * 60 * 1000.0); // kg/sec
    const double temp = 320.0 ; //Kelvin
    
    // Cascade params used in Python test code (Enrichment_Calculations.ipynb)
    const double feed_assay = 0.0071;
    const double product_assay = 0.035;
    const double waste_assay = 0.001;
    const double feed_c = 739 / (30.4 * 24 * 60 * 60); // kg/month -> kg/sec
    const double product_c = 77 / (30.4 * 24 * 60 * 60); // kg/month -> kg/sec

    double delU = CalcDelU(v_a, height, diameter, feed_m, temp, cut, eff,
			   M, dM, x, flow_internal);
    
    double alpha = AlphaBySwu(delU, feed_m, cut, M);

    const double tol_assay = 1e-5;
    const double tol_qty = 1e-6;
    const double tol_num = 1e-2;
   
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Calculate ideal SWU params of single machine (separation potential delU
// and separation factor alpha)
TEST(Enrich_Functions_Test, TestSWU) {

  double pycode_U = 1.35805875245e-07;
  double tol = 1e-9;
  
  EXPECT_NEAR(delU, pycode_U, tol);

  double pycode_alpha = 1.2268;
  double tol_alpha = 1e-2;
  EXPECT_NEAR(alpha, pycode_alpha, tol_alpha);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Ideal cascade design, and then using away from ideal design
TEST(Enrich_Functions_Test, TestCascade) {
  double n_machines = MachinesPerCascade(delU, product_assay,
					 waste_assay, feed_c, product_c);

  double pycode_n_mach = 13458.399;
  EXPECT_NEAR(n_machines, pycode_n_mach, tol_num);
  
  std::pair<double, double> n_stages = StagesPerCascade(alpha, feed_assay,
							product_assay,
							waste_assay);

  double pycode_n_enrich_stage = 7.159;
  double pycode_n_strip_stage = 8.669;
  EXPECT_NEAR(n_stages.first, pycode_n_enrich_stage, tol_num);
  EXPECT_NEAR(n_stages.second, pycode_n_strip_stage, tol_num);

  // Now test assays when cascade is modified away from ideal design
  // (cascade optimized for natural uranium feed, now use 20% enriched
  double feed_assay_mod = 0.20;

  int n_stage_enrich = (int) n_stages.first + 1;  // Round up to next integer
  int n_stage_waste = (int) n_stages.second + 1;  // Round up to next integer

  double mod_product_assay = ProductAssayFromNStages(alpha, feed_assay_mod,
						     n_stage_enrich);
  double mod_waste_assay = WasteAssayFromNStages(alpha, feed_assay_mod,
						 n_stage_waste);

  double pycode_mod_product_assay = 0.605435;
  double pycode_mod_waste_assay = 0.031445;
  EXPECT_NEAR(mod_product_assay, pycode_mod_product_assay, tol_assay);
  EXPECT_NEAR(mod_waste_assay, pycode_mod_waste_assay, tol_assay);
  
  }
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Determine the output of the first enrich/strip stage of a cascade
  // based on the design params for the cascade
  TEST(Enrich_Functions_Test, TestStages) {
    double product_assay_s = NProductByAlpha(alpha, feed_assay);
    double n_mach_e = MachinesPerStage(alpha, delU, feed_c);
    double product_s = ProductPerEnrStage(alpha, feed_assay,
					product_assay_s, feed_c);

    double enrich_waste = feed_c - product_s;
    double enrich_waste_assay = (feed_c * feed_assay - product_s *
				 product_assay_s)/enrich_waste;

    double pycode_product_assay_s = 0.008696;
    double pycode_n_mach_e = 53.287;
    double pycode_product_s = 0.0001409;

    EXPECT_NEAR(n_mach_e, pycode_n_mach_e, tol_num);
    EXPECT_NEAR(product_assay_s, pycode_product_assay_s, tol_assay);
    EXPECT_NEAR(product_s, pycode_product_s, tol_qty);

    double n_mach_w = MachinesPerStage(alpha, delU, enrich_waste);
    double strip_waste_assay = NWasteByAlpha(alpha, enrich_waste_assay);
    double strip_waste = WastePerStripStage(alpha, enrich_waste_assay,
					    strip_waste_assay, enrich_waste);

    double pycode_n_mach_w = 26.6007;
    double pycode_waste_assay_s = 0.00448653;
    double pycode_waste_s = 8.60660553717e-05;

    EXPECT_NEAR(n_mach_w, pycode_n_mach_w, tol_num);
    EXPECT_NEAR(strip_waste_assay, pycode_waste_assay_s, tol_assay);
    EXPECT_NEAR(strip_waste, pycode_waste_s, tol_qty);
    
  }

    
  } // namespace enrichfunctiontests
} // namespace mbmore