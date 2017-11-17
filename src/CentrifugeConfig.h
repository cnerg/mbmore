#ifndef MBMORE_SRC_CASCADE_ENRICH_H_
#define MBMORE_SRC_CASCADE_ENRICH_H_

#include <string>

namespace mbmore {

class CentrifugeConfig : {
 public:
  CentrifugeConfig();
  CentrifugeConfig(double v_a, double h, double d, double feed, double T,
                   double eff, double M, double dM, double x, double i_flow);

  double ComputeDeltaU(double cut);
  double CalcCTherm(double v_a, double temp, double dM);
  double CalcV(double assay);

  // flow_internal = 2-4, x = pressure ratio, M = 0.352 kg/mol of UF6,
  // dM = 0.003 kg/mol diff between 235 and 238
  double M;
  double dM;
  double x;
  double flow_internal;

  double v_a;
  double height;
  double diameter;
  double feed;
  double temp;
  double eff;
};


}  // namespace mbmore

#endif  // MBMORE_SRC_CASCADE_ENRICH_H_
