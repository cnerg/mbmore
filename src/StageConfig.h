#ifndef MBMORE_SRC_STAGE_H_
#define MBMORE_SRC_STAGE_H_

#include <string>
#include "CentrifugeConfig.h"

namespace mbmore {
class CascadeConfig;

class StageConfig {
  friend CascadeConfig;

 public:
  // Setup a empty stage
  StageConfig() { ; }
  // Design a ideal stage for a specific feed assay and feed flow
  StageConfig(CentrifugeConfig cent, double f_assay, double feed_flow,
              double precision = 1e-8);
  // Design a ideal stage for a specific feed assay and feed flow
  StageConfig(double f_assay, double feed_flow, double cut_, double DU_,
              double alpha_ = -1, double precision = 1e-8);

  // Build a stage assumming alpha = beta (if cut is not defined, compute the
  // cut
  // to make it so)
  void BuildIdealStg(double f_assay, double precision = 1e-8);
  // Compute the cut to ensure alpha = beta (from dU)
  double CutForIdealStg(double f_assay, double precision = 1e-8);

  // calculate Alpha value using the dU, Cut and the centrifuge feed flow value
  double AlphaByDU();
  
  // calculate Alpha from the feed assay and the product assay;
  double AlphaByProductAssay();

  // Compute Beta value from alpha value, cut and feed assay
  double BetaByAlphaAndCut();
  // recompute Cut value assuming Alpha and Beta fixed
  double CutByAlphaBeta();

  // Compute Product from gamma
  double ProductAssayByGamma(double gamma);

  // Compute Product assay from feed assay and alpha
  double ProductAssay();
  
  // Compute Waste assy from feed assay and beta
  double TailAssay();

  // Return the minimum number of centrifudes required to meed the feed flow
  double MachinesPerStage();
  // Compute the Product feed
  double ProductPerEnrStage();

  // Configuration of all the centrifuges in the stage
  CentrifugeConfig centrifuge;
  // Precision used for the cut calculation defautl 1e-8
  double precision;

  // cut value of the stage
  double cut;
  // dU value of the stage (calculted form the centrifuges config with the cut)
  double DU;
  // Feed to Product enrichment ratio
  double alpha;
  // Feed to Tail enrichment ratio
  double beta;
  // Feed flow (g/s)
  double feed_flow;

  // number of centriges in the stage
  double n_machines;

  // Feed assay
  double feed_assay;
  // Product assay
  double product_assay;
  // Tail assay
  double tail_assay;
};

}  // namespace mbmore

#endif  // MBMORE_SRC_STAGE_H_
