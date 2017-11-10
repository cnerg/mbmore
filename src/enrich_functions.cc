#define _USE_MATH_DEFINES

#include <cmath>
#include <cstdlib>
#include <ctime>  // to make truly random
#include <iostream>
#include <iterator>
#include "cyclus.h"
#include "enrich_functions.h"

namespace mbmore {

double D_rho = 2.2e-5;     // kg/m/s
double gas_const = 8.314;  // J/K/mol
double M_238 = 0.238;      // kg/mol

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO:
// annotate assumed units, Glaser paper reference

double CalcDelU(double cut, centrifuges_config cent_config) {
  // Inputs that are effectively constants:
  // flow_internal = 2-4, x = pressure ratio, M = 0.352 kg/mol of UF6,
  // dM = 0.003 kg/mol diff between 235 and 238

  double v_a = cent_condig.v_a;
  double height = cent_condig.height;
  double diameter = cent_condig.diameter;
  double feed = cent_condig.feed;
  double temp = cent_condig.temp;
  double eff = cent_condig.eff;
  double M = cent_condig.M;
  double dM = cent_condig.dM;
  double x = cent_condig.x;
  double flow_internal = cent_condig.flow_internal;

  double a = diameter / 2.0;  // outer radius

  // withdrawl radius for heavy isotope
  // Glaser 2009 says operationally it ranges from 0.96-0.99
  double r_2 = 0.975 * a;

  double r_12 = std::sqrt(
      1.0 - (2.0 * gas_const * temp * (log(x)) / M_mol / (pow(v_a, 2))));
  double r_1 = r_2 * r_12;  // withdrawl radius for ligher isotope

  // Glaser eqn 12
  // Vertical location of feed
  double Z_p = height * (1.0 - cut) * (1.0 + flow_internal) /
               (1.0 - cut + flow_internal);

  // Glaser eqn 3
  // Converting from molecular mass to atomic mass (assuming U238)
  // Warning: This assumption is only valid at low enrichment
  // TODO: EXPLICITLY CALCULATE ATOMIC MASS GIVEN FEED ASSAY
  double C1 = (2.0 * M_PI * (D_rho * M_238 / M_mol) / (log(r_2 / r_1)));
  //  double C1 = (2.0 * M_PI * D_rho / (log(r_2 / r_1)));
  double A_p = C1 * (1.0 / feed) *
               (cut / ((1.0 + flow_internal) * (1.0 - cut + flow_internal)));
  double A_w = C1 * (1.0 / feed) *
               ((1.0 - cut) / (flow_internal * (1.0 - cut + flow_internal)));

  double C_therm = CalcCTherm(v_a, temp, dM);

  // defining terms in the Ratz equation
  double r12_sq = pow(r_12, 2);
  double C_scale = (pow((r_2 / a), 4)) * (pow((1 - r12_sq), 2));
  double bracket1 = (1 + flow_internal) / cut;
  double exp1 = exp(-1.0 * A_p * Z_p);
  double bracket2 = flow_internal / (1 - cut);
  double exp2 = exp(-1.0 * A_w * (height - Z_p));

  // Glaser eqn 10 (Ratz Equation)
  double major_term =
      0.5 * cut * (1.0 - cut) * (pow(C_therm, 2)) * C_scale *
      pow(((bracket1 * (1 - exp1)) + (bracket2 * (1 - exp2))), 2);  // kg/s
  double del_U = feed * major_term * eff;                           // kg/s

  return del_U;
}
double CalcCTherm(double v_a, double temp, double dM) {
  double c_therm = (dM * (pow(v_a, 2))) / (2.0 * gas_const * temp);
  return c_therm;
}

double CalcV(double assay) {
  return (2.0 * assay - 1.0) * log(assay / (1.0 - assay));
}

double AlphaBySwu(double del_U, double feed, double cut, double M) {
  double alpha = 1 + std::sqrt((2 * (del_U / M) * (1 - cut) / (cut * feed)));
  return alpha;
}

double BetaByAlphaAndCut(double alpha, double feed_assay, double cut) {
  double product_assay = ProductAssayByAlpha(alpha, feed_assay);
  double waste_assay = (feed_assay - cut * product_assay) / (1. - cut);

  return feed_assay / (1. - feed_assay) * (1. - waste_assay) / waste_assay;
}

// per machine
double ProductAssayByAlpha(double alpha, double feed_assay) {
  // Possibly incorrect is commented out ?
  //    double ratio = (1.0 - feed_assay) / (alpha * feed_assay);
  //    return 1.0 / (ratio + 1.0);
  double ratio = alpha * feed_assay / (1.0 - feed_assay);
  return ratio / (1. + ratio);

  // return alpha / ( alpha - 1 + 1 / feed_assay );
}

double WasteAssayByBeta(double beta, double feed_assay) {
  double A = (feed_assay / (1. - feed_assay)) / beta;
  return A / (1. + A);
}

// This equation can only be used in the limit where the separation factor
// (alpha) is very close to one, which is not true for modern gas centrifuges
// DO NOT USE THIS EQUATION!!!
// Avery p.59
/*
std::pair<double, double> StagesPerCascade(double alpha, double feed_assay,
                                           double product_assay,
                                           double waste_assay){
  using std::pair;

  double epsilon = alpha - 1.0;
  double enrich_inner = (product_assay / (1.0 - product_assay)) *
    ((1.0 - feed_assay) / feed_assay);
  double strip_inner =  (feed_assay / (1.0 - feed_assay)) *
    ((1.0 - waste_assay) / waste_assay);

  double enrich_stages = (1.0 / epsilon) * log(enrich_inner);
  double strip_stages = (1.0 / epsilon) * log(strip_inner);
  std::pair<double, double> stages = std::make_pair(enrich_stages,
                                                    strip_stages);
  return stages;

}
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Determine number of stages required to reach ideal cascade product assay
// (requires integer number of stages, so output may exceed target assay)

stg_config BuildIdealStg(double feed_assay, centrifuges_config cent_config, double precision){

  stg_config stg;
  stg.feed_assay = feed_assay;
  stg.cut = get_cut_for_ideal_stg(cent_config, stg.feed_assay, precision);  
  stg.DU = CalcDelU(stg.cut, cent_config);
 
  // Ideal Case Alpha := Beta !!
  stg.alpha = AlphaBySwu(stg.DU, stg.feed_assay);
  stg.beta = BetaByAlphaAndCut(stg.alpha, stg.feed_assay, stg.cut);
  
  stg.product_assay = ProductAssayByAlpha(stg.alpha, stg.feed_assay);
  stg.waste_assay = WasteAssayByBeta(stg.beta, stg.feed_assay);

 return stg;
}


cascade_config FindNStages(double feed_assay, double product_assay,
                                double waste_assay, centrifuges_config cent_config, double precision) {
  using std::pair;
  cascade_config stgs_cut_and_assay;
  int ideal_enrich_stage = 0;
  int ideal_strip_stage = 0;

  // Initialisation of Feeding stage (I == 0)
  stg_config stg = BuildIdealStg(feed_assay, cent_config, precision);
  int stg_i = 0;
  stgs_cut_and_assay.stgs_config[stg_i] = stg;

  // Calculate number of enriching stages
  while (this_stg.product_assay < product_assay) {
    stg = BuildIdealStg(stg.product_assay, cent_config, precision);
    stg_i++;
    stgs_cut_and_assay.stgs_config[stg_i] = stg;
  }
  stgs_cut_and_assay.enrich_stgs = stg_i+1;
  
  // reset
  stg_i = 0;
  stg = stgs_cut_and_assay.stgs_config[stg_i];
  // Calculate number of stripping stages
  while (stg.waste_assay > waste_assay) {
    stg = BuildIdealStg(stg.taiL_assay, cent_config, precision);
    stg_i--;
    stgs_cut_and_assay.stgs_config[stg_i] = stg;
  }
  stgs_cut_and_assay.strpping_stgs = -stg_i;
  
  return stgs_cut_and_assay;
}


double ProductAssayFromNStages(double alpha, double beta, double feed_assay,
                               double stages) {
  if (stages == 0) {
    return ProductAssayByAlpha(alpha, feed_assay);
  } else if (stages < 0) {
    double stg_feed_assay =
        WasteAssayFromNStages(alpha, beta, feed_assay, stages + 1);
    return ProductAssayByAlpha(alpha, stg_feed_assay);
  } else if (stages > 0) {
    double stg_feed_assay =
        ProductAssayFromNStages(alpha, beta, feed_assay, stages - 1);
    return ProductAssayByAlpha(alpha, stg_feed_assay);
  }
}

double WasteAssayFromNStages(double alpha, double beta, double feed_assay,
                             double stages) {
  if (stages == 0) {
    return WasteAssayByBeta(beta, feed_assay);
  } else if (stages < 0) {
    double stg_feed_assay =
        WasteAssayFromNStages(alpha, beta, feed_assay, stages + 1);
    return WasteAssayByBeta(beta, stg_feed_assay);
  } else if (stages > 0) {
    double stg_feed_assay =
        ProductAssayFromNStages(alpha, beta, feed_assay, stages - 1);
    return WasteAssayByBeta(beta, stg_feed_assay);
  }
}

double MachinesPerStage(double alpha, double del_U, double stage_feed) {
  return stage_feed / (2.0 * del_U / (pow((alpha - 1.0), 2)));
}

double ProductPerEnrStage(double alpha, double feed_assay, double product_assay,
                          double stage_feed) {
  return stage_feed * (alpha - 1.0) * feed_assay * (1 - feed_assay) /
         (2 * (product_assay - feed_assay));
}

// 14-Feb-2017
// THIS EQN PRODUCES THE WRONG RESULT FOR SOME REASON.
// DONT KNOW WHAT THE PROBLEM IS THOUGH
/*
double WastePerStripStage(double alpha, double feed_assay, double waste_assay,
                          double stage_feed){
  return stage_feed * (alpha - 1.0) * feed_assay * (1 - feed_assay) /
    (2 * (feed_assay - waste_assay));
}
*/

double DeltaUCascade(double product_assay, double waste_assay, double feed_flow,
                     double product_flow) {
  double Vpc = CalcV(product_assay);
  double Vwc = CalcV(waste_assay);
  return product_flow * Vpc + (feed_flow - product_flow) * Vwc;
}

double MachinesPerCascade(double del_U_machine, double product_assay,
                          double waste_assay, double feed_flow,
                          double product_flow) {
  double U_cascade =
      DeltaUCascade(product_assay, waste_assay, feed_flow, product_flow);
  return U_cascade / del_U_machine;
}

double DelUByCascadeConfig(double product_assay, double waste_assay,
                           double product_flow, double waste_flow,
                           double feed_assay) {
  double U_cascade =
      DeltaUCascade(product_assay, waste_assay, product_flow, waste_flow);
  return U_cascade / feed_assay;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Calculate steady-state flow rates into each cascade stage
// Linear system of equations in form AX = B, where A is nxn square matrix
// of linear equations for the flow rates of each stage and B are the external
// feeds for the stage. External feed is zero for all stages accept cascade
// feed stage (F_0) stages start with last strip stage [-2, -1, 0, 1, 2]
//  http://www.physics.utah.edu/~detar/phys6720/handouts/lapack.html
//
std::vector<double> CalcFeedFlows(std::pair<int, int> n_st, double cascade_feed,
                                  double cut, double feed_assay,
                                  double product_assay, double tails_assay) {
  // This is the Max # of stages in cascade. It cannot be passed in due to
  // how memory is allocated and so must be hardcoded. It's been chosen
  // to be much larger than it should ever need to be
  int max_stages = 100;

  // Number of enrich stages
  int n_enrich = n_st.first;
  // NUmber of stripping stages
  int n_strip = n_st.second;
  // total number of stages
  int n_stages = n_st.first + n_st.second;
  if (n_stages > max_stages) {
    std::cout << "To many stages in the cascade, can't calculated the "
                 "thoerritical flows..."
              << std::endl;
    exit(1);
  }
  double product_flow =
      cascade_feed * (feed_assay - tails_assay) / (product_assay - tails_assay);
  double tails_flow = cascade_feed - product_flow;
  // std::cout << "product_flow "<< product_flow << std::endl;
  // std::cout << "waste_flow "<< tails_flow << std::endl;
  // LAPACK takes the external flow feeds as B, and then returns a modified
  // version of the same array now representing the solution flow rates.

  // Build Array with pointers
  double flow_eqns[max_stages][max_stages];
  double flows[1][max_stages];

  // build matrix of equations in this pattern
  // [[ -1, 1-cut,    0,     0,      0]       [[0]
  //  [cut,    -1, 1-cut,    0,      0]        [0]
  //  [  0,   cut,    -1, 1-cut,     0]  * X = [-1*cascade_feed]
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
      int i = row_idx - n_strip;
      int col_idx = n_strip + i;
      flow_eqns[col_idx][row_idx] = -1;
      if (col_idx != 0) {
        flow_eqns[col_idx - 1][row_idx] = cut;
      }
      if (col_idx != n_stages - 1) {
        flow_eqns[col_idx + 1][row_idx] = (1 - cut);
      }

      // Add the external feed for the cascade
      if (i == 0) {
        flows[0][row_idx] = -1 * cascade_feed;
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

  std::vector<double> final_flows;
  for (int i = 0; i < n_stages; i++) {
    final_flows.push_back(flows[0][i]);
  }
  return final_flows;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Determine number of machines in each stage of the cascade, and total
// output flow from each stage
std::vector<std::pair<int, double>> CalcStageFeatures(
    double feed_assay, double alpha, double del_U, double cut,
    std::pair<int, int> n_st, std::vector<double> feed_flow) {
  double machine_tol = 0.01;
  double beta = BetaByAlphaAndCut(alpha, feed_assay, cut);
  int n_enrich = n_st.first;
  int n_strip = n_st.second;
  int n_stages = n_st.first + n_st.second;

  std::vector<std::pair<int, double>> stage_info;

  //  int n_centrifuge = 0;
  double stage_feed_assay = feed_assay;
  double strip_feed_assay;

  for (int i = 0; i < n_enrich; i++) {
    int curr_stage = i + n_strip;
    double stage_feed = feed_flow[curr_stage];
    double n_mach_exact = MachinesPerStage(alpha, del_U, stage_feed);
    // unless the ideal number of machines is Very close to an integer value,
    // round up to next integer to preserve steady-state flow balance
    int n_mach = (int)n_mach_exact;
    if (std::abs(n_mach_exact - n_mach) > machine_tol) {
      n_mach = int(n_mach_exact) + 1;
    }
    double stage_product = stage_feed * cut;
    std::pair<int, double> curr_info = std::make_pair(n_mach, stage_product);
    stage_info.push_back(curr_info);

    // waste assay from first enriching stage becomes feed assay for first
    // stripping stage
    if (i == 0) {
      strip_feed_assay = WasteAssayByBeta(beta, stage_feed_assay);
    }
    // reset feed assay for next stage to product assay from this stage
    stage_feed_assay = ProductAssayByAlpha(alpha, stage_feed_assay);
  }

  stage_feed_assay = strip_feed_assay;
  for (int i = n_strip - 1; i >= 0; --i) {
    int curr_stage = i - n_strip;

    double stage_feed = feed_flow[i];
    double n_mach_exact = MachinesPerStage(alpha, del_U, stage_feed);
    // unless the ideal number of machines is Very close to an integer value,
    // round up to next integer to preserve steady-state flow balance
    int n_mach = (int)n_mach_exact;
    if (std::abs(n_mach_exact - n_mach) > machine_tol) {
      n_mach = int(n_mach_exact) + 1;
    }
    double stage_product = stage_feed * cut;
    std::pair<int, double> curr_info = std::make_pair(n_mach, stage_product);
    stage_info.insert(stage_info.begin(), curr_info);

    // reset feed assay for next stage to waste assay from this stage
    stage_feed_assay = WasteAssayByBeta(beta, stage_feed_assay);
  }

  return stage_info;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Determine total number of machines in the cascade from machines per stage
int FindTotalMachines(std::vector<std::pair<int, double>> stage_info) {
  int machines_needed = 0;
  std::vector<std::pair<int, double>>::const_iterator it;
  for (it = stage_info.begin(); it != stage_info.end(); it++) {
    machines_needed += it->first;
  }
  return machines_needed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::pair<int, double> DesignCascade(double design_feed, double design_alpha,
                                     double design_delU, double cut,
                                     int max_centrifuges,
                                     std::pair<int, int> n_stages,
                                     double feed_assay, double product_assay,
                                     double tail_assay) {
  // Determine the ideal steady-state feed flows for this cascade design given
  // the maximum potential design feed rate
  std::vector<double> feed_flows = CalcFeedFlows(
      n_stages, design_feed, cut, feed_assay, product_assay, tail_assay);

  std::vector<std::pair<int, double>> stage_info = CalcStageFeatures(
      design_feed, design_alpha, design_delU, cut, n_stages, feed_flows);

  // Do design parameters require more centrifuges than what is available?
  int machines_needed = FindTotalMachines(stage_info);

  double optimal_feed = design_feed;

  bool pos_inc = false;
  bool neg_inc = false;
  bool optimal_number = false;
  double step = 1.05;
  double step_size = 1.0;

  double curr_feed = design_feed;
  int max_tries = 10000;
  int ntries = 0;

  // If there are not enough centrifuges for optimal design
  if (machines_needed < max_centrifuges) {
    pos_inc = true;
    step_size = step;
  } else if (machines_needed > max_centrifuges) {
    neg_inc = true;
    step_size = 1.0 / step;
  } else {
    optimal_number = true;
  }

  while ((optimal_number == false) && (ntries < max_tries)) {
    ntries += 1;
    double last_feed = curr_feed;
    curr_feed *= step_size;
    feed_flows = CalcFeedFlows(n_stages, curr_feed, cut, feed_assay,
                               product_assay, tail_assay);
    stage_info = CalcStageFeatures(curr_feed, design_alpha, design_delU, cut,
                                   n_stages, feed_flows);
    machines_needed = FindTotalMachines(stage_info);
    std::pair<int, double> last_stage = stage_info.back();
    std::cout << "# in last stage " << last_stage.first << std::endl;
    // If cannot converge on a cascade with allowable number of centrifuges
    if (ntries >= max_tries) {
      throw cyclus::ValueError(
          "Could not design a cascade using the max allowed machines");
    }
    // If the last stage of the cascade has zero centrifuges then there are
    // not enough to achieve the target enrichment
    else if (last_stage.first < 1) {
      throw cyclus::ValueError(
          "Not enough available centrifuges to achieve target enrichment "
          "level");
    }
    // If optimal design is finally found
    else if ((neg_inc == true) and (machines_needed <= max_centrifuges)) {
      optimal_feed = curr_feed;
      optimal_number = true;
    } else if ((pos_inc == true) and (machines_needed > max_centrifuges)) {
      optimal_feed = last_feed;
      optimal_number = true;
    }
    feed_flows = CalcFeedFlows(n_stages, optimal_feed, cut, feed_assay,
                               product_assay, tail_assay);
    stage_info = CalcStageFeatures(optimal_feed, design_alpha, design_delU, cut,
                                   n_stages, feed_flows);
    machines_needed = FindTotalMachines(stage_info);
  }
  // Otherwise if there are enough centrifuges to process more than the
  // requested amount of material

  std::pair<int, double> cascade_info =
      std::make_pair(machines_needed, optimal_feed);
  return cascade_info;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SortBids(cyclus::Bid<cyclus::Material>* i,
              cyclus::Bid<cyclus::Material>* j) {
  cyclus::Material::Ptr mat_i = i->offer();
  cyclus::Material::Ptr mat_j = j->offer();

  cyclus::toolkit::MatQuery mq_i(mat_i);
  cyclus::toolkit::MatQuery mq_j(mat_j);

  return ((mq_i.mass(922350000) / mq_i.qty()) <=
          (mq_j.mass(922350000) / mq_j.qty()));
}

cascade_config Compute_Assay( cascade_config cascade_config, double feed_assay, double precision) {
  
  
  cascade_config actual_enrichments = cascade_config;
  cascade_config previous_enrichments;
  int i = 0;
  while (Diff_enrichment(actual_enrichments, previous_enrichments) >
         precision) {
    previous_enrichments = actual_enrichments;
    actual_enrichments =
        Update_enrichment(cascade_config, feed_assay);
    i++;
  }
  return actual_enrichments;
}

double Diff_enrichment(cascade_config a_enrichments,
                       cascade_config p_enrichments) {
  if (p_enrichments.feed.size() == 0) {
    return 100.;
  }
  double square_feed_diff = 0;
  double square_product_diff = 0;
  double square_waste_diff = 0;
  srd::map< int, stg_config >::iterator it;
  for (it = a_enrichments.stg_config.begin(); it != a_enrichments.stg_config.end(); it++) {
    int i = it->first; 
    square_feed_diff +=
        pow(a_enrichments.stg_config[i].feed_assay - p_enrichment.stg_config[i].feed_assay, 2);
    square_product_diff +=
        pow(a_enrichments.stg_config[i].product_assay - p_enrichment.stg_config[i].product_assay, 2);
    square_waste_diff +=
        pow(a_enrichments.stg_config[i].tail_assay - p_enrichment.stg_config[i].tail_assay, 2);
  }
  return square_feed_diff + square_product_diff + square_waste_diff;
}

cascade_config Update_enrichment( cascade_config cascade_config, double feed_assay ) {
  cascade_config updated_enrichment = cascade_config;
  
  double down_assay = 0;
  double up_assay = 0;
  double down_flow = 0;
  double up_flow = 0;
  
   double stg_feed_flow = 0;
  srd::map< int, stg_config >::iterator it;
  for (it = cascade_config.stg_config.begin(); it != cascade_config.stg_config.end(); it++) {
    int i = it->first; 
    if (i == cascade_config.strpping_stgs) {  // last strip stage
      down_assay = 0.;
      down_flow = 0.;
      up_assay = cascade_config.stg_config[i + 1].tail_assay;
      up_flow = cascade_config.stg_config[i + 1].flow*(1-cascade_config.stg_config[1+1].cut);
      stg_feed_flow = up_flow;
    } else if (i < cascade_config.enrich_stgs ) {
      down_assay = actual_enrichments.product[i - 1];
      down_flow = cascade_features[i - 1].second;
      up_assay = cascade_config.stg_config[i + 1].tail_assay;
      up_flow = cascade_config.stg_config[i + 1].flow*(1-cascade_config.stg_config[1+1].cut);
      stg_feed_flow = down_flow + up_flow;
    } else {  // last enrichment stage
      down_assay = cascade_config.stg_config[i - 1].product_assay;
      down_flow = cascade_config.stg_config[i - 1].flow*cascade_config.stg_config[1 -1].cut;
      up_assay = 0.;
      up_flow = 0.;
      stg_feed_flow = down_flow;
    }

    double stg_feed_assay =
        (down_assay * down_flow + up_assay * up_flow) / (down_flow + up_flow);
    if (i == n_stages.second) {  // add Feed flow in the entry stage
      stg_feed_assay = (down_assay * down_flow + up_assay * up_flow +
                        feed_assay * feed_flow) /
                       (down_flow + up_flow + cascade_config.feed_flow);
      stg_feed_flow = down_flow + up_flow + cascade_config.feed_flow;
    }
    updated_enrichment.cascade_config[i].beta = BetaByAlphaAndCut(updated_enrichment.cascade_config[i].alpha, stg_feed_assay, updated_enrichment.cascade_config[i].cut);
    double stg_product_assay = ProductAssayByAlpha(updated_enrichment.cascade_config[i].alpha, stg_feed_assay);
    double stg_waste_assay = WasteAssayByBeta(updated_enrichment.cascade_config[i].beta, stg_feed_assay);
    updated_enrichment.cascade_config[i].feed_assay = stg_feed_assay;
    updated_enrichment.cascade_config[i].product_assay = stg_product_assay;
    updated_enrichment.cascade_config[i].tail_assay = stg_tail_assay;
  }

  return updated_enrichment;
}

double get_cut_for_ideal_stg(centrifuges_config cent_config, double feed_assay,
                             double precision) {
  double p_cut = 0.25;
  double p_DU = CalcDelU(p_cut, cent_config);
  double p_alpha = AlphaBySwu(p_DU, feed, p_cut, M);
  double p_beta = BetaByAlphaAndCut(p_alpha, feed, p_cut);
  double p_alpha_m_beta = p_alpha - p_beta;

  double cut = 0.575;
  double DU = CalcDelU(cut, cent_config);
  double alpha = AlphaBySwu(DU, feed, cut, M);
  double beta = BetaByAlphaAndCut(alpha, feed, cut);

  while( abs(alpha - beta) > precision){
    // a*cut +b =y
    double alpha_m_beta = alpha - beta;

    double a = (p_alpha_m_beta - alpha_m_beta) / (p_cut - cut);
    double b = alpha_m_beta - cut * a;
    p_cut = cut;
    p_alpha = alpha;
    p_beta = beta;
    p_alpha_m_beta = alpha_m_beta;
    // targeting alpha_m_beta = 0
    cut = -b / a;
    DU = CalcDelU(cut, cent_config);
    alpha = AlphaBySwu(DU, feed, cut, M);
    beta = BetaByAlphaAndCut(alpha, feed, cut);
  }
  return cut;
}

}  // namespace mbmore
