#ifndef MBMORE_SRC_CENTRIFUGE_H_
#define MBMORE_SRC_CENTRIFUGE_H_

#include <string>

namespace mbmore {

class CascadeConfig;
class CentrifugeConfig {
  friend CascadeConfig;

 public:
  CentrifugeConfig();
  CentrifugeConfig(double v_a, double h, double d, double feed, double T,
                   double eff, double M, double dM, double x, double i_flow);

  double ComputeDeltaU(double cut); // compute the solution of the Raetz equation using all the paramters valus and the provided cut value

  double dM; // molar mass diff between the 2 components of the gaz default 0.003kg/mol (u235/U238)
  double M;  // gaz molar mass, default UF6 0.352 kg/mol
  double x;  // pressure ration -> drive r1/r2 ratio
  double flow_internal; // countercurrent-to-feed ratios k range between 2 and 4

  double v_a; // peripheral velocities
  double height; // centrifugre height
  double diameter; // centrifuge diameter
  double feed; // feed flow rate
  double temp; // average gaz temperature
  double eff; // efficiency

 private:
  double D_rho;      // kg/m/s
  double gas_const;  // J/K/mol
  double M_238;      // kg/mol

  // two method to compute some part of the solution to the Raetz equation
  double CalcCTherm(double v_a, double temp, double dM);
  double CalcV(double assay);
};

}  // namespace mbmore

#endif  // MBMORE_SRC_CENTRIFUGE_H_
