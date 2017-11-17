// Implements the CascadeEnrich class
#include "CascadeConfig.h"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <limits>
#include <sstream>
#include <vector>

namespace mbmore {

  CascadeConfig::CascadeConfig() {}
  
  CascadeConfig(double f_assay, double p_assay, double t_assay, double max_feed_flow,
                int max_centrifuge){
    
    double feed_assay = f_assay;
    double product_assay = p_assay;
    double tail_assay = t_assay;
    
    double feed_flow = max_feed_flow;
    int n_centrifuge = max_centrifuge;
    
    BuildIdealCascade(feed_assay, product_assay, tail_assay);
    DesignCascade(max_feed_flow, max_centrifuge) 
  
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Calculate steady-state flow rates into each stage
// Linear system of equations in form AX = B, where A is nxn square matrix
// of linear equations for the flow rates of each stage and B are the external
// feeds for the stage. External feed is zero for all stages accept cascade
// feed stage (F_0) stages start with last strip stage [-2, -1, 0, 1, 2]
//  http://www.physics.utah.edu/~detar/phys6720/handouts/lapack.html
//
void CasacdeConfig::CalcFeedFlows() {
  // This is the Max # of stages in  It cannot be passed in due to
  // how memory is allocated and so must be hardcoded. It's been chosen
  // to be much larger than it should ever need to be

  // total number of stages
  int n_stages = n_enrich + n_strip;
  int max_stages = n_stages;
  if (n_stages > max_stages) {
    std::cout << "To many stages in the  can't calculated the "
                 "thoerritical flows..."
              << std::endl;
    exit(1);
  }
  double feed_assay = stgs_config[0].feed_assay;
  double product_assay =
      stgs_config[enrich_stgs - 1].product_assay;
  double tail_assay = stgs_config[-stripping_stgs].tail_assay;

  double product_flow =
      feed_flow * (feed_assay - tail_assay) / (product_assay - tail_assay);
  double tail_flow =
      product_flow * (product_assay - feed_assay) / (feed_assay - tail_assay);

  // Build Array with pointers
  double flow_eqns[max_stages][max_stages];
  double flows[1][max_stages];

  // build matrix of equations in this pattern
  // [[ -1, 1-cut,    0,     0,      0]       [[0]
  //  [cut,    -1, 1-cut,    0,      0]        [0]
  //  [  0,   cut,    -1, 1-cut,     0]  * X = [-1*feed]
  //  [  0,     0,   cut,    -1, 1-cut]        [0]
  //  [  0,     0,     0,   cut,    -1]]       [0]]
  //
  for (int row_idx = 0; row_idx < max_stages; row_idx++) {
    // fill the array with zeros, then update individual elements as nonzero
    flows[0][row_idx] = 0;
    for (int fill_idx = 0; fill_idx < max_stages; fill_idx++) {
      flow_eqns[fill_idx][row_idx] = 0;
    }
    // Required do to the artificial 'Max Stages' defn. Only calculate
    // non-zero matrix elements where stages really exist.
    if (row_idx < n_stages) {
      int stg_i = row_idx - n_strip;
      int col_idx = n_strip + stg_i;
      flow_eqns[col_idx][row_idx] = -1.;
      if (row_idx != 0) {
        flow_eqns[col_idx - 1][row_idx] = stgs_config[stg_i - 1].cut;
      }
      if (row_idx != n_stages - 1) {
        flow_eqns[col_idx + 1][row_idx] =
            (1 - stgs_config[stg_i + 1].cut);
      }

      // Add the external feed for the cascade
      if (stg_i == 0) {
        flows[0][row_idx] = -1. * feed_flow;
      }
      /*if (stg_i == -n_strip){
        flows[0][row_idx] = 1. * tail_flow;
      }
      if (stg_i == n_enrich-1){
        flows[0][row_idx] = 1. * product_flow;
      }*/
    }
  }

  // LAPACK solver variables
  int nrhs = 1;          // 1 column solution
  int lda = max_stages;  // must be >= MAX(1,N)
  int ldb = max_stages;  // must be >= MAX(1,N)
  int ipiv[max_stages];
  int info;

  // Solve the linear system
  dgesv_(&n_stages, &nrhs, &flow_eqns[0][0], &lda, ipiv, &flows[0][0], &ldb,
         &info);

  // Check for success
  if (info != 0) {
    std::cerr << "LAPACK linear solver dgesv returned error " << info << "\n";
  }

  for (int i = 0; i < n_stages; i++) {
    int stg_i = i - n_strip;
    stgs_config[stg_i].flow = flows[0][i];
  }
}

void CascadeConfig::BuildIdealCascade(double feed_assay, double product_assay,
                                     double waste_assay,
                                     double precision) {
  map<int, StageConfig> ideal_stgs;
  int ideal_enrich_stage = 0;
  int ideal_strip_stage = 0;

  // Initialisation of Feeding stage (I == 0)
  StageConfig stg = BuildIdealStg(feed_assay, centrifuge, -1, -1, precision);
  int stg_i = 0;
  ideal_stgs[stg_i] = stg;
  double ref_alpha = ideal_stgs[0].alpha;
  double ref_du = ideal_stgs[0].DU;
  // Calculate number of enriching stages
  while (stg.product_assay < product_assay) {
    stg = BuildIdealStg(stg.product_assay, ref_du, ref_alpha,
                        precision);
    stg_i++;
    ideal_stgs[stg_i] = stg;
  }
  n_enrich = stg_i + 1;
  // reset
  stg_i = 0;
  stg = ideal_stgs[stg_i];
  // Calculate number of stripping stages
  while (stg.tail_assay > waste_assay) {
    stg = BuildIdealStg(stg.tail_assay, centrifuge, ref_du, ref_alpha,
                        precision);
    stg_i--;
    ideal_stgs[stg_i] = stg;
  }
  n_strip = -stg_i;

  stgs_config = ideal_stgs;

}



void DesignCascade(double max_feed, int max_centrifuges) {
  // Determine the ideal steady-state feed flows for this cascade design given
  // the maximum potential design feed rate
  feed_flow = max_feed;
  CalcFeedFlows();
  CalcStageFeatures(max_feed_cascade);

  // Do design parameters require more centrifuges than what is available?
  int machines_needed = FindTotalMachines();
  double max_feed_from_machine = max_feed;
  while (machines_needed > max_centrifuges) {
    double scaling_ratio = (double)machines_needed / (double)max_centrifuges;
    max_feed_from_machine = max_feed_from_machine / scaling_ratio;
    feed_flow = max_feed_from_machine;
    CalcFeedFlows();
    CalcStageFeatures();
    machines_needed = FindTotalMachines();
  }
  
  n_machines = machines_needed;
  return max_feed_cascade;
}








}  // namespace mbmore
