// Implements the CascadeEnrich class
#include "CascadeConfig.h"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <limits>
#include <sstream>
#include <vector>

namespace mbmore {

CentrifugeConfig::CentrifugeConfig() {
  double v_a = 485;
  double height = 0.5;
  double diameter = 0.15;
  double feed = 15. / 1000. / 1000.;
  double temp = 320;

  double eff = 1.0;
  double M = 0.352;
  double dM = 0.003;
  double x = 1000;
  double flow_internal = 2.0;
}

double CentrifugeConfig::ComputeDeltaU(double cut) {
  // Inputs that are effectively constants:

  double a = diameter / 2.0;  // outer radius

  // withdrawl radius for heavy isotope
  // Glaser 2009 says operationally it ranges from 0.96-0.99
  double r_2 = 0.975 * a;

  double r_12 = std::sqrt(
      1.0 - (2.0 * gas_const * temp * (log(x)) / M_mol / (pow(v_a, 2))));
  double r_1 = r_2 * r_12;  // withdrawl radius for ligher isotope

  // Glaser eqn 12
  // Vertical location of feed
  double Z_p = height * (1.0 - cut) * (1.0 + flow_internal) /
               (1.0 - cut + flow_internal);

  // Glaser eqn 3
  // Converting from molecular mass to atomic mass (assuming U238)
  // Warning: This assumption is only valid at low enrichment
  // TODO: EXPLICITLY CALCULATE ATOMIC MASS GIVEN FEED ASSAY
  double C1 = (2.0 * M_PI * (D_rho * M_238 / M_mol) / (log(r_2 / r_1)));
  //  double C1 = (2.0 * M_PI * D_rho / (log(r_2 / r_1)));
  double A_p = C1 * (1.0 / feed) *
               (cut / ((1.0 + flow_internal) * (1.0 - cut + flow_internal)));
  double A_w = C1 * (1.0 / feed) *
               ((1.0 - cut) / (flow_internal * (1.0 - cut + flow_internal)));

  double C_therm = CalcCTherm(v_a, temp, dM);

  // defining terms in the Ratz equation
  double r12_sq = pow(r_12, 2);
  double C_scale = (pow((r_2 / a), 4)) * (pow((1 - r12_sq), 2));
  double bracket1 = (1 + flow_internal) / cut;
  double exp1 = exp(-1.0 * A_p * Z_p);
  double bracket2 = flow_internal / (1 - cut);
  double exp2 = exp(-1.0 * A_w * (height - Z_p));

  // Glaser eqn 10 (Ratz Equation)
  double major_term =
      0.5 * cut * (1.0 - cut) * (pow(C_therm, 2)) * C_scale *
      pow(((bracket1 * (1 - exp1)) + (bracket2 * (1 - exp2))), 2);  // kg/s
  double del_U = feed * major_term * eff;                           // kg/s

  return del_U;
}

double CentrifugeConfig::CalcCTherm(double v_a, double temp, double dM) {
  double c_therm = (dM * (pow(v_a, 2))) / (2.0 * gas_const * temp);
  return c_therm;
}

double CentrifugeConfig::CalcV(double assay) {
  return (2.0 * assay - 1.0) * log(assay / (1.0 - assay));
}


}  // namespace mbmore
