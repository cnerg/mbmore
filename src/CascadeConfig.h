#ifndef MBMORE_SRC_CASCADE_H_
#define MBMORE_SRC_CASCADE_H_

#include <map>
#include <string>

#include "CentrifugeConfig.h"
#include "StageConfig.h"

namespace mbmore {
// LAPACK solver for system of linear equations
extern "C" {
void dgesv_(int *n, int *nrhs, double *a, int *lda, int *ipivot, double *b,
            int *ldb, int *info);
}

class CascadeConfig {
 public:
  CascadeConfig() ;
  CascadeConfig(CentrifugeConfig centrifuge, double f_assay, double p_assay,
                double t_assay, double max_feed_flow, int max_centrifuge,
                double precision = 1e-8);
  // Build a full cascade such as all stage follow alpha = beta = const. Get
  // alpha/beta value from feeding stage. From the design feed/product/assay
  void BuildIdealCascade(double f_assay, double p_assay, double w_assay,
                         double precision = 1e-8);
  // Get the total number of machine in the Cascade
  int FindTotalMachines();

  // Solve the flow matrix from the stages cuts
  void CalcFeedFlows();
  // DO something ?!
  void CalcStageFeatures();
  // Scale the Casacde to meet the limitation in max feed or max centrifuges
  void DesignCascade(double max_feed, int max_centrifuges);

  // Compute the response of the cascade to a non ideal feed assay
  CascadeConfig Compute_Assay(double feed_assay, double precision, bool u_cut = false);

  double FeedFlow() { return feed_flow; }
  // Configuration of the centrifuges in the stages
  CentrifugeConfig centrifuge;
  // Map of all the stage configuration
  std::map<int, StageConfig> stgs_config;
  // number of enrich stages
  int n_enrich;
  // number of stripping stages
  int n_strip;

 private:
  // total number of machine in the Cascade
  int n_machines;
  // real feed flow (constrained by the cascade design/total number of
  // machine/max feed flow
  double feed_flow;

  //design feed assay
  double feed_assay;
  //design product assay
  double design_product_assay;
  //design tail assay
  double design_tail_assay;
  
  // Method to check the assays different between 2 cascades
  double Diff_enrichment(CascadeConfig actual_enrichments,
                         CascadeConfig previous_enrichement);

  // method computing one iteration, of the algorithm used to get the response
  // to non ideal feed assay 
  std::map<int, StageConfig> Update_enrichment(CascadeConfig cascade,
                                               double feed_assay, bool u_cut = false);
};

}  // namespace mbmore

#endif  // MBMORE_SRC_CASCADE_H_
