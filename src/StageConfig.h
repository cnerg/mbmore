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

  void BuildIdealStg(double f_assay, double precision = 1e-16);
  double CutForIdealStg(double f_assay, double precision = 1e-16);

  double AlphaByDU();

  double BetaByAlphaAndCut();
  double CutByAalphaBeta();

  double ProductAssay();
  double TailAssay();

  double MachinesPerStage();
  double ProductPerEnrStage();

  // Calculates the V(N_x) term for enrichment eqns where N_x is the assay
  // of isotope x

  CentrifugeConfig centrifuge;

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
