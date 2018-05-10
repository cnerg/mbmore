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

CascadeConfig::CascadeConfig()
    : n_enrich(0),
      n_strip(0),
      n_machines(0),
      feed_flow(0),
      feed_assay(0),
      design_product_assay(0),
      design_tail_assay(0) {}

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
  ScaleCascade(max_feed_flow, max_centrifuge);
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
        std::map<int, StageConfig>::iterator it = stgs_config.find(stg_i - 1);
        if (it != stgs_config.end()) {
          flow_eqns[col_idx - 1][row_idx] = it->second.cut;
        }
      }
      if (row_idx != n_stages - 1) {
        std::map<int, StageConfig>::iterator it = stgs_config.find(stg_i + 1);
        if (it != stgs_config.end()) {
          flow_eqns[col_idx + 1][row_idx] = (1 - it->second.cut);
        }
      }
      // Add the external feed for the cascade
      if (stg_i == 0) {
        flows[0][row_idx] = -1. * feed_flow;
      }
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

  int i = 0;
  std::map<int, StageConfig>::iterator it;
  for (it = stgs_config.begin(); it != stgs_config.end(); it++) {
    it->second.feed_flow = flows[0][i];
    i++;
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
    ideal_stgs.insert(std::make_pair(stg_i, stg));
  }
  n_enrich = stg_i + 1;
  // reset
  stg_i = 0;
  stg = ideal_stgs[stg_i];
  // Calculate number of stripping stages
  while (stg.tail_assay > waste_assay) {
    stg.BuildIdealStg(stg.tail_assay, precision);
    stg_i--;
    ideal_stgs.insert(std::make_pair(stg_i, stg));
  }
  n_strip = -stg_i;
  stgs_config = ideal_stgs;
}

void CascadeConfig::PopulateStages() {
  double machine_tol = 0.01;
  int n_stages = n_enrich + n_strip;

  for (int i = 0; i < n_stages; i++) {
    int curr_stage = i - n_strip;
    std::map<int, StageConfig>::iterator it = stgs_config.find(curr_stage);
    if (it == stgs_config.end()) {
      std::cout << "Bad Stage number" << std::endl;
      exit(1);
    }
    double n_mach_exact = it->second.MachinesPerStage();
    // unless the ideal number of machines is Very close to an integer value,
    // round up to next integer to preserve steady-state flow balance
    int n_mach = (int)n_mach_exact;
    if (std::abs(n_mach_exact - n_mach) > machine_tol) {
      n_mach = int(n_mach_exact) + 1;
    }
    it->second.n_machines = n_mach;
  }
}

int CascadeConfig::FindTotalMachines() {
  int machines = 0;
  std::map<int, StageConfig>::iterator it;
  for (it = stgs_config.begin(); it != stgs_config.end(); it++) {
    machines += it->second.n_machines;
  }
  return machines;
}

void CascadeConfig::ScaleCascade(double max_feed, int max_centrifuges) {
  // Determine the ideal steady-state feed flows for this cascade design given
  // the maximum potential design feed rate
  feed_flow = max_feed;
  CalcFeedFlows();
  PopulateStages();

  // Do design parameters require more centrifuges than what is available?
  int machines_needed = FindTotalMachines();
  double max_feed_from_machine = max_feed;
  while (machines_needed > max_centrifuges) {
    double scaling_ratio = (double)machines_needed / (double)max_centrifuges;
    max_feed_from_machine = max_feed_from_machine / scaling_ratio;
    feed_flow = max_feed_from_machine;
    CalcFeedFlows();
    PopulateStages();
    machines_needed = FindTotalMachines();
  }
  n_machines = machines_needed;
}

CascadeConfig CascadeConfig::ModelMissUsedCascade(double f_assay,
                                                  int modeling_opt,
                                                  double precision) {
  CascadeConfig miss_used_cascade = (*this);
  miss_used_cascade.PropagateAssay(f_assay);

  switch (modeling_opt) {
    default:
      miss_used_cascade.ComputeAssayByAlpha(f_assay, precision);
      break;

    case 1:
      miss_used_cascade.UpdateCut();
      miss_used_cascade.UpdateFlow();
      break;
    
    case 2:
      miss_used_cascade.ComputeAssayByGamma(f_assay, precision);
      break;
  }
  return miss_used_cascade;
}

void CascadeConfig::UpdateFlow() {
  // recompute the flow according to the new cuts  
  (*this).CalcFeedFlows();
  
  double ratio = 1;
  std::map<int, StageConfig>::iterator it;
  for (it = (*this).stgs_config.begin(); it != (*this).stgs_config.end();
       it++) {
    std::map<int, StageConfig>::iterator it_real =
        (*this).stgs_config.find(it->first);
    double max_stg_flow =
        it_real->second.n_machines * it_real->second.centrifuge.feed;
    double stg_flow_ratio = it->second.feed_flow / max_stg_flow;
    if (ratio < stg_flow_ratio) {
      ratio = stg_flow_ratio;
    }
  }
  for (it = (*this).stgs_config.begin(); it != (*this).stgs_config.end();
       it++) {
    it->second.feed_flow *= 1. / ratio;
  }
  (*this).feed_flow *= 1. / ratio;
}

void CascadeConfig::UpdateCut() {
  std::map<int, StageConfig>::iterator it;
  for (it = (*this).stgs_config.begin(); it != (*this).stgs_config.end();
       it++) {
    it->second.CutByAlphaBeta();
  }
}

void CascadeConfig::PropagateAssay(double f_assay) {
  // Initialiase Feeding stage
  std::map<int, StageConfig>::iterator it = (*this).stgs_config.find(0);
  it->second.feed_assay = f_assay;
  it->second.ProductAssay();
  it->second.TailAssay();

  // Propagate initialisation to all stages
  for (int i = 0; i < (*this).n_enrich; i++) {
    it = (*this).stgs_config.find(i);
    std::map<int, StageConfig>::iterator it_feed;

    // Enrich stage -> feed = Product from Previous Stage
    it_feed = (*this).stgs_config.find(it->first - 1);
    if (it->first > 0) {
      if (it_feed != (*this).stgs_config.end()) {
        it->second.feed_assay = it_feed->second.product_assay;
      }
      // Update Product and Tail assay from feed assay
      it->second.ProductAssay();
      it->second.TailAssay();
    }
  }
  for (int i = 1; i <= (*this).n_strip; i++) {
    it = (*this).stgs_config.find(-i);
    std::map<int, StageConfig>::iterator it_feed;

    // Striping stage -> feed = tails from Next Stage
    it_feed = (*this).stgs_config.find(it->first + 1);
    if (it->first < 0) {
      if (it_feed != (*this).stgs_config.end()) {
        it->second.feed_assay = it_feed->second.tail_assay;
      }
      // Update Product and Tail assay from feed assay
      it->second.ProductAssay();
      it->second.TailAssay();
    }
  }
}

void CascadeConfig::ComputeAssayByAlpha(double f_assay, double precision) {
  CascadeConfig previous_cascade;
  while (DeltaEnrichment((*this), previous_cascade) > precision) {
    previous_cascade = (*this);
    (*this).IterrateEnrichment(f_assay);
    (*this).UpdateByAlpha();
  }
}

void CascadeConfig::ComputeAssayByGamma(double f_assay, double precision) {
  CascadeConfig previous_cascade;
  while (DeltaEnrichment((*this), previous_cascade) > precision) {
    previous_cascade = (*this);
    (*this).IterrateEnrichment(f_assay);
    (*this).UpdateByGamma();
  }
}

double CascadeConfig::DeltaEnrichment(CascadeConfig a_enrichments,
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
    std::map<int, StageConfig>::iterator it2 =
        p_enrichments.stgs_config.find(it->first);
    if (it2 != p_enrichments.stgs_config.end()) {
      square_feed_diff +=
          pow(it->second.feed_assay - it2->second.feed_assay, 2);
      square_product_diff +=
          pow(it->second.product_assay - it2->second.product_assay, 2);
      square_waste_diff +=
          pow(it->second.tail_assay - it2->second.tail_assay, 2);
    }
  }
  return square_feed_diff + square_product_diff + square_waste_diff;
}

void CascadeConfig::IterrateEnrichment(double f_assay) {
  CascadeConfig updated_enrichment = (*this);

  // mixing variables
  double down_assay = 0;
  double up_assay = 0;
  double down_flow = 0;
  double up_flow = 0;
  double stg_feed_flow = 0;
  std::map<int, StageConfig>::iterator it;

  // Get the Flow and Assay quantity
  for (it = (*this).stgs_config.begin(); it != (*this).stgs_config.end();
       it++) {
    int i = it->first;
    std::map<int, StageConfig>::iterator it_up =
        (*this).stgs_config.find(i + 1);
    std::map<int, StageConfig>::iterator it_down =
        (*this).stgs_config.find(i - 1);
    down_assay = 0;
    up_assay = 0;
    down_flow = 0;
    up_flow = 0;

    if (it_down != (*this).stgs_config.end()) {
      down_assay = it_down->second.product_assay;
      down_flow = it_down->second.feed_flow * it_down->second.cut;
    }
    if (it_up != (*this).stgs_config.end()) {
      up_assay = it_up->second.tail_assay;
      up_flow = it_up->second.feed_flow * (1 - it_up->second.cut);
    }

    // Mix the Product and the Tail to have the correct Feed Assay
    double stg_f_assay =
        (down_assay * down_flow + up_assay * up_flow) / (down_flow + up_flow);
    if (i == 0) {  // add Feed flow in the entry stage
      stg_f_assay = (down_assay * down_flow + up_assay * up_flow +
                     f_assay * (*this).feed_flow) /
                    (down_flow + up_flow + (*this).feed_flow);
      stg_feed_flow = down_flow + up_flow + (*this).feed_flow;
    }

    std::map<int, StageConfig>::iterator it_new =
        updated_enrichment.stgs_config.find(i);

    // Update Stage feed assay
    it_new->second.feed_assay = stg_f_assay;
  }

  (*this).stgs_config = updated_enrichment.stgs_config;
}

void CascadeConfig::UpdateByAlpha(){
  std::map<int, StageConfig>::iterator it;
  for (it = (*this).stgs_config.begin(); it != (*this).stgs_config.end();
       it++) {
    // Update Beta values (from feed) -- Alpha & Cut are cte
    it->second.BetaByAlphaAndCut();
    // Recompute Product Assay and Tail Assay
    it->second.ProductAssay();
    it->second.TailAssay();
  }
}

void CascadeConfig::UpdateByGamma(){
  std::map<int, StageConfig>::iterator it;
  for (it = (*this).stgs_config.begin(); it != (*this).stgs_config.end();
       it++) {
    double gamma = it->second.alpha * it->second.beta;
    // Recompute Product Assay
    it->second.ProductAssayByGamma(gamma);
    // Alpha by Product assay
    it->second.AlphaByProductAssay();
    // Beta and tail assay...
    it->second.BetaByAlphaAndCut();
    it->second.TailAssay();
  }
}

}  // namespace mbmore
