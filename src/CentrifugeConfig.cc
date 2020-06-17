// Implements the CascadeEnrich class
#include "CentrifugeConfig.h"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <limits>
#include <sstream>
#include <vector>

/*
@article{alexander_glaser_characteristics_2008,
        title = {Characteristics of the {Gas} {Centrifuge} for {Uranium}
{Enrichment} and {Their} {Relevance} for {Nuclear} {Weapon} {Proliferation}},
        issn = {1547-7800},
        doi = {10.1080/08929880802335998},
        number = {16},
        journal = {Science and Global Security},
        author = {Alexander Glaser},
        year = {2008},
        pages = {1--25},
        annote = {From Tamara},
        file =
{2008aglaser_sgsvol16.pdf:/Users/mouginot/Zotero/storage/UAGA49FS/2008aglaser_sgsvol16.pdf:application/pdf}
}
*/
namespace mbmore {

CentrifugeConfig::CentrifugeConfig() {
  // Standard Values for a P1-type centrifuges
  v_a = 320;
  height = 1.8;
  diameter = 0.10;
  feed = 15. / 1000. / 1000.;
  temp = 320;

  eff = 1.0;
  x = 1000;
  flow_ratio = 2.0;

  // default UF6 0.352 kg/mol
  M = 0.352;

  // default 0.003kg/mol (u235/U238)
  dM = 0.003;
  // SEE GLASER P2
  D_rho = 2.2e-5;     // kg/m/s
  gas_const = 8.314;  // J/K/mol
  M_238 = 0.238;      // kg/mol
}

CentrifugeConfig::CentrifugeConfig(double v_a_, double h_, double d_,
                                   double feed_, double T_, double eff_,
                                   double M_, double dM_, double x_,
                                   double flow_) {
  v_a = v_a_;
  height = h_;
  diameter = d_;
  feed = feed_;
  temp = T_;

  eff = eff_;
  x = x_;
  flow_ratio = flow_;

  M = M_;
  dM = dM_;
  D_rho = 2.2e-5;     // kg/m/s
  gas_const = 8.314;  // J/K/mol
  M_238 = 0.238;      // kg/mol
}

double CentrifugeConfig::ComputeDeltaU(double cut) {
  // Inputs that are effectively constants:

  double a = diameter / 2.0;  // outer radius

  // withdrawal radius for heavy isotope
  // Glaser 2009 says operationally it ranges from 0.96-0.99
  double r2 = 0.975 * a;

  // Glaser: if v_a < 380 then r1_over_r2 = cte = 0.534
  double r1_over_r2 = 0;
  if (v_a > 380) {
    r1_over_r2 =
        std::sqrt(1.0 - (2.0 * gas_const * temp * log(x)) / M / (v_a * v_a));
  } else {
    r1_over_r2 = 0.534;
  }
  double r1 = r2 * r1_over_r2;  // withdrawal radius for lighter isotope

  // Glaser eqn 12
  // Vertical location of feed
  double Z_p =
      height * (1.0 - cut) * (1.0 + flow_ratio) / (1.0 - cut + flow_ratio);

  // Glaser eqn 3
  // Converting from molecular mass to atomic mass (assuming U238)
  // Warning: This assumption is only valid at low enrichment
  // TODO: EXPLICITLY CALCULATE ATOMIC MASS GIVEN FEED ASSAY

  // converting feed from UF6 to U
  double ratio_UF6_U = M_238 / M;
  double feed_U = feed * ratio_UF6_U;

  double C1 = (2.0 * M_PI * D_rho * ratio_UF6_U / (log(r2 / r1)));
  double A_p = C1 * (1.0 / feed_U) *
               (cut / ((1.0 + flow_ratio) * (1.0 - cut + flow_ratio)));
  double A_w = C1 * (1.0 / feed_U) *
               ((1.0 - cut) / (flow_ratio * (1.0 - cut + flow_ratio)));

  double C_therm = CalcCTherm(v_a, temp, dM);

  // defining terms in the Ratz equation
  double r1_over_r2_sq = pow(r1_over_r2, 2);
  double C_scale = pow((r2 / a), 4) * pow(1 - r1_over_r2_sq, 2);
  double bracket1 = (1 + flow_ratio) / cut;
  double exp1 = exp(-1.0 * A_p * Z_p);
  double bracket2 = flow_ratio / (1 - cut);
  double exp2 = exp(-1.0 * A_w * (height - Z_p));

  // Glaser eqn 10 (Ratz Equation)
  double major_term =
      0.5 * cut * (1.0 - cut) * (C_therm * C_therm) * C_scale *
      pow(((bracket1 * (1 - exp1)) + (bracket2 * (1 - exp2))), 2);  // kg/s
  double del_U = feed_U * major_term;  //* eff; // kg/s

  return del_U;
}

double CentrifugeConfig::CalcCTherm(double v_a, double temp, double dM) {
  return dM * (v_a * v_a) / (2.0 * gas_const * temp);
}

double CentrifugeConfig::CalcV(double assay) {
  return (2.0 * assay - 1.0) * log(assay / (1.0 - assay));
}

}  // namespace mbmore
