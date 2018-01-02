#ifndef MBMORE_SRC_STAGE_H_
#define MBMORE_SRC_STAGE_H_

#include <string>
#include "CentrifugeConfig.h"

namespace mbmore {
class CascadeConfig;

class StageConfig {
  friend CascadeConfig;

 public:
  StageConfig() {;}
  StageConfig(double f_assay, double feed_flow, double precision, double cut = -1, double DU = -1, double alpha = -1); 

  void BuildIdealStg(double f_assay, double precision = 1e-8);
  double CutForIdealStg(double f_assay, double precision = 1e-8);

  double AlphaByDU();

  double BetaByAlphaAndCut();
  double CutByAlphaBeta();

  double ProductAssay();
  double TailAssay();

  double MachinesPerStage();
  double ProductPerEnrStage();

  // Calculates the V(N_x) term for enrichment eqns where N_x is the assay
  // of isotope x

  CentrifugeConfig centrifuge;
  double precision;

  double cut;
  double DU;
  double alpha;
  double beta;
  double feed_flow;
  
  double n_machines;

  double feed_assay;
  double product_assay;
  double tail_assay;
};

}  // namespace mbmore

#endif  // MBMORE_SRC_STAGE_H_
