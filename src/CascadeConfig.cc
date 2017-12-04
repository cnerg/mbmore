// Implements the CascadeEnrich class
#include "CascadeConfig.h"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <vector>

namespace mbmore {

CascadeConfig::CascadeConfig(CentrifugeConfig centrifuge_, double f_assay,
                             double p_assay, double t_assay,
                             double max_feed_flow, int max_centrifuge,
                             double precision) {
  centrifuge = centrifuge_;
  feed_assay = f_assay;
  design_product_assay = p_assay;
  design_tail_assay = t_assay;

  feed_flow = max_feed_flow;
  n_machines = max_centrifuge;

  BuildIdealCascade(f_assay, p_assay, t_assay, precision);
  DesignCascade(max_feed_flow, max_centrifuge);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Calculate steady-state flow rates into each stage
// Linear system of equations in form AX = B, where A is nxn square matrix
// of linear equations for the flow rates of each stage and B are the external
// feeds for the stage. External feed is zero for all stages accept cascade
// feed stage (F_0) stages start with last strip stage [-2, -1, 0, 1, 2]
//  http://www.physics.utah.edu/~detar/phys6720/handouts/lapack.html
//
void CascadeConfig::CalcFeedFlows() {
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
  double f_assay = stgs_config[0].feed_assay;
  double product_assay = stgs_config[n_enrich - 1].product_assay;
  double tail_assay = stgs_config[-n_strip].tail_assay;

  double product_flow =
      feed_flow * (f_assay - tail_assay) / (product_assay - tail_assay);
  double tail_flow =
      product_flow * (product_assay - f_assay) / (f_assay - tail_assay);

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
        flow_eqns[col_idx + 1][row_idx] = (1 - stgs_config[stg_i + 1].cut);
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
    stgs_config[stg_i].feed_flow = flows[0][i];
  }
}

void CascadeConfig::BuildIdealCascade(double f_assay, double product_assay,
                                      double waste_assay, double precision) {
  std::map<int, StageConfig> ideal_stgs;
  int ideal_enrich_stage = 0;
  int ideal_strip_stage = 0;

  // Initialisation of Feeding stage (I == 0)
  StageConfig stg;
  stg.alpha = -1;
  stg.DU = -1;
  stg.centrifuge = centrifuge;
  stg.BuildIdealStg(f_assay, precision);
  int stg_i = 0;
  ideal_stgs[stg_i] = stg;
  double ref_alpha = ideal_stgs[0].alpha;
  double ref_du = ideal_stgs[0].DU;
  // Calculate number of enriching stages

  while (stg.product_assay < product_assay) {
    stg.BuildIdealStg(stg.product_assay, precision);
    stg_i++;
    ideal_stgs[stg_i] = stg;
  }
  n_enrich = stg_i + 1;
  // reset
  stg_i = 0;
  stg = ideal_stgs[stg_i];
  // Calculate number of stripping stages
  while (stg.tail_assay > waste_assay) {
    stg.BuildIdealStg(stg.tail_assay, precision);
    stg_i--;
    ideal_stgs[stg_i] = stg;
  }
  n_strip = -stg_i;

  stgs_config = ideal_stgs;
}
void CascadeConfig::CalcStageFeatures() {
  double machine_tol = 0.01;
  int n_enrich = n_enrich;
  int n_strip = n_strip;
  int n_stages = n_enrich + n_strip;

  for (int i = 0; i < n_stages; i++) {
    int curr_stage = i - n_strip;
    double stage_feed = stgs_config[curr_stage].feed_flow;
    double alpha = stgs_config[curr_stage].alpha;
    double DU = stgs_config[curr_stage].DU;
    double n_mach_exact = stgs_config[curr_stage].MachinesPerStage();
    // unless the ideal number of machines is Very close to an integer value,
    // round up to next integer to preserve steady-state flow balance
    int n_mach = (int)n_mach_exact;
    if (std::abs(n_mach_exact - n_mach) > machine_tol) {
      n_mach = int(n_mach_exact) + 1;
    }
    stgs_config[curr_stage].n_machines = n_mach;
  }
}

int CascadeConfig::FindTotalMachines() {
  int machines = 0;
  for (int i = -n_strip; i < n_enrich; i++) {
    machines += stgs_config[i].n_machines;
  }
  return machines;
}

void CascadeConfig::DesignCascade(double max_feed, int max_centrifuges) {
  // Determine the ideal steady-state feed flows for this cascade design given
  // the maximum potential design feed rate
  feed_flow = max_feed;
  CalcFeedFlows();
  CalcStageFeatures();

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
}

CascadeConfig CascadeConfig::Compute_Assay(double f_assay, double precision) {
  CascadeConfig cascade = (*this);
  // Initialiase Feeding stage
  cascade.stgs_config[0].feed_assay = f_assay;
  cascade.stgs_config[0].ProductAssay();
  cascade.stgs_config[0].TailAssay();
  // Propagate initialisation to enriching stages
  for (int i = 1; i < cascade.n_enrich; i++) {
    cascade.stgs_config[i].feed_assay =
        cascade.stgs_config[i - 1].product_assay;
    cascade.stgs_config[i].ProductAssay();
    cascade.stgs_config[i].TailAssay();
  }
  // then to stripping stages
  for (int i = 1; i < cascade.n_strip; i++) {
    cascade.stgs_config[-i].feed_assay = cascade.stgs_config[-i + 1].tail_assay;
    cascade.stgs_config[-i].ProductAssay();
    cascade.stgs_config[-i].TailAssay();
  }

  // Looping to get the equilibrium
  CascadeConfig actual_enrichments = cascade;
  CascadeConfig previous_enrichments;

  while (Diff_enrichment(actual_enrichments, previous_enrichments) >
         precision) {
    previous_enrichments = actual_enrichments;
    actual_enrichments.stgs_config = Update_enrichment(cascade, f_assay);
  }
  return actual_enrichments;
}

double CascadeConfig::Diff_enrichment(CascadeConfig a_enrichments,
                                      CascadeConfig p_enrichments) {
  if (p_enrichments.n_enrich == 0) {
    return 100.;
  }
  double square_feed_diff = 0;
  double square_product_diff = 0;
  double square_waste_diff = 0;
  std::map<int, StageConfig>::iterator it;
  for (it = a_enrichments.stgs_config.begin();
       it != a_enrichments.stgs_config.end(); it++) {
    int i = it->first;
    square_feed_diff += pow(a_enrichments.stgs_config[i].feed_assay -
                                p_enrichments.stgs_config[i].feed_assay,
                            2);
    square_product_diff += pow(a_enrichments.stgs_config[i].product_assay -
                                   p_enrichments.stgs_config[i].product_assay,
                               2);
    square_waste_diff += pow(a_enrichments.stgs_config[i].tail_assay -
                                 p_enrichments.stgs_config[i].tail_assay,
                             2);
  }
  return square_feed_diff + square_product_diff + square_waste_diff;
}

std::map<int, StageConfig> CascadeConfig::Update_enrichment(
    CascadeConfig cascade, double f_assay) {
  CascadeConfig updated_enrichment = cascade;

  // mixing variables
  double down_assay = 0;
  double up_assay = 0;
  double down_flow = 0;
  double up_flow = 0;
  double stg_feed_flow = 0;
  std::map<int, StageConfig>::iterator it;

  // Get the Flow and Assay quantity
  for (it = cascade.stgs_config.begin(); it != cascade.stgs_config.end();
       it++) {
    int i = it->first;
    if (i == cascade.n_strip) {  // last strip stage
      down_assay = 0.;
      down_flow = 0.;
      up_assay = cascade.stgs_config[i + 1].tail_assay;
      up_flow = cascade.stgs_config[i + 1].feed_flow *
                (1 - cascade.stgs_config[1 + 1].cut);
      stg_feed_flow = up_flow;
    } else if (i < cascade.n_enrich - 1) {
      down_assay = cascade.stgs_config[i - 1].product_assay;
      down_flow =
          cascade.stgs_config[i - 1].feed_flow * cascade.stgs_config[1 + 1].cut;
      up_assay = cascade.stgs_config[i + 1].tail_assay;
      up_flow = cascade.stgs_config[i + 1].feed_flow *
                (1 - cascade.stgs_config[1 + 1].cut);
      stg_feed_flow = down_flow + up_flow;
    } else {  // last enrichment stage
      down_assay = cascade.stgs_config[i - 1].product_assay;
      down_flow =
          cascade.stgs_config[i - 1].feed_flow * cascade.stgs_config[1 - 1].cut;
      up_assay = 0.;
      up_flow = 0.;
      stg_feed_flow = down_flow;
    }

    // Mix the Product and the Tail to have the correct Feed Assay
    double stg_f_assay =
        (down_assay * down_flow + up_assay * up_flow) / (down_flow + up_flow);
    if (i == 0) {  // add Feed flow in the entry stage
      stg_f_assay = (down_assay * down_flow + up_assay * up_flow +
                     f_assay * cascade.feed_flow) /
                    (down_flow + up_flow + cascade.feed_flow);
      stg_feed_flow = down_flow + up_flow + cascade.feed_flow;
    }

    // Update Stage feed assay
    updated_enrichment.stgs_config[i].feed_assay = stg_f_assay;
    // Update Beta values (from feed) -- Alpha & Cut are cte
    updated_enrichment.stgs_config[i].BetaByAlphaAndCut();
    // Recompute Product Assay and Tail Assay
    updated_enrichment.stgs_config[i].ProductAssay();
    updated_enrichment.stgs_config[i].TailAssay();
  }

  return updated_enrichment.stgs_config;
}
}  // namespace mbmore
