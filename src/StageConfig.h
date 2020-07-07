#ifndef MBMORE_SRC_STAGE_H_
#define MBMORE_SRC_STAGE_H_

#include <string>
#include "CentrifugeConfig.h"

namespace mbmore {
class CascadeConfig;

class StageConfig {

  private:

    // Compute the cut to ensure alpha = beta (from dU)
    void CutForIdealStg();

    // Precision used for the cut calculation defautl 1e-8
    double precision_ = 1e-8;

    // cut value of the stage
    double cut_;
    // dU value of the stage (calculated form the centrifuges config with the cut)
    double DU_;
    // Feed to Product enrichment ratio
    double alpha_;
    // Feed to Tail enrichment ratio
    double beta_;
    // Feed flow (g/s)
    double feed_flow_;

    // number of centrifuges in the stage
    int n_machines_ = -1;

    // Feed assay
    double feed_assay_;
    // Product assay
    double product_assay_;
    // Tail assay
    double tail_assay_;

  public:
    // Setup a empty stage
    StageConfig() { ; }
    // Design an ideal stage for a specific feed assay and feed flow
    StageConfig(CentrifugeConfig cent, double f_assay, double feed_flow,
                double precision = 1e-8);
    // Design an ideal stage for a specific feed assay and feed flow
    StageConfig(double f_assay, double feed_flow, double cut_, double DU_,
                double alpha_ = -1, double precision = 1e-8);

    // Build a stage assuming alpha = beta
    // (if cut is not defined, compute the cut to make it so)
    void BuildIdealStg();

    // calculate Alpha value using the dU, Cut and the centrifuge feed flow value
    void AlphaByDU();

    // calculate Alpha from the feed assay and the product assay;
    void AlphaByProductAssay();

    // Compute Beta value from alpha value, cut and feed assay
    void BetaByAlphaAndCut();
    // recompute Cut value assuming Alpha and Beta fixed
    void CutByAlphaBeta();

    // Compute Product from gamma
    void ProductAssayByGamma(double gamma);

    // Compute Product assay from feed assay and alpha
    void ProductAssay();
    // Compute Waste assay from feed assay and beta
    void TailAssay();

    // Return the minimum number of centrifuges required to meet the feed flow
    void MachinesNeededPerStage(double tolerance = 0.01);

    // Configuration of all the centrifuges in the stage
    // Default centrifuge initialized
    CentrifugeConfig centrifuge;

    // Setter methods
    void precision(double p) {precision_ = p;}
    void cut(double c) {cut_ = c;}
    void DU(double du) {DU_ = du;}
    void alpha(double a) {alpha_ = a;}
    void beta(double b) {beta_ = b;}
    void feed_flow(double f) {feed_flow_ = f;}
    void feed_assay(double fa) {feed_assay_ = fa;}

    // Getter methods
    double precision() {return precision_;}
    double cut() {return cut_;}
    double DU() {return DU_;}
    double alpha() {return alpha_;}
    double beta() {return beta_;}
    double feed_flow() {return feed_flow_;}
    int n_machines() {return n_machines_;}
    double feed_assay() {return feed_assay_;}
    double product_assay() {return product_assay_;}
    double tail_assay() {return tail_assay_;}
};

}  // namespace mbmore

#endif  // MBMORE_SRC_STAGE_H_
