#define _USE_MATH_DEFINES

#include "enrich_functions.h"
#include <ctime> // to make truly random
#include <cstdlib>
#include <iostream>
#include <cmath>

namespace mbmore {

  double D_rho = 2.2e-5; // kg/m/s
  double gas_const = 8.314; // J/K/mol
  double M_238 = 0.238; //kg/mol
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO:
// annotate assumed units, Glaser paper reference

  double CalcDelU(double v_a, double height, double diameter,  double feed,
		  double temp, double cut, double eff, double M_mol, double dM,
		  double x, double flow_internal) {

    // Inputs that are effectively constants:
    // flow_internal = 2-4, x = pressure ratio, M = 0.352 kg/mol of UF6,
    // dM = 0.003 kg/mol diff between 235 and 238

    double a = diameter / 2.0;  // outer radius

    // withdrawl radius for heavy isotope
    // Glaser 2009 says operationally it ranges from 0.96-0.99
    double r_2 = 0.99 * a; 

    double r_12 = std::sqrt(1.0 - (2.0 * gas_const * temp*(log(x)) / M_mol /
				   (pow(v_a, 2)))); 
    double r_1 = r_2 * r_12; // withdrawl radius for ligher isotope

    // Glaser eqn 12
    // Vertical location of feed
    double Z_p = height * (1.0 - cut) * (1.0 + flow_internal) /
      (1.0 - cut + flow_internal);

    //Glaser eqn 3
    // To make consistent with Glaser results:
    //    double C1 = (2.0 * M_PI * (D_rho*M_238/M_mol) / (log(r_2 / r_1)));
    double C1 = (2.0 * M_PI * D_rho / (log(r_2 / r_1)));
    double A_p = C1 *(1.0 / feed) *
      (cut / ((1.0 + flow_internal) * (1.0 - cut + flow_internal)));
    double A_w = C1 * (1.0 / feed) *
      ((1.0 - cut)/(flow_internal * (1.0 - cut + flow_internal)));
     
    double C_therm = CalcCTherm(v_a, temp, dM);

    // defining terms in the Ratz equation
    double r12_sq = pow(r_12, 2);
    double C_scale = (pow((r_2 / a), 4)) * (pow((1 - r12_sq), 2));
    double bracket1 = (1 + flow_internal) / cut;
    double exp1 = exp(-1.0 * A_p * Z_p);
    double bracket2 = flow_internal/(1 - cut);
    double exp2 = exp(-1.0 * A_w * (height - Z_p));

    // Glaser eqn 10 (Ratz Equation)
    double major_term = 0.5 * cut * (1.0 - cut) * (pow(C_therm, 2)) * C_scale *
      pow(((bracket1 * (1 - exp1)) + (bracket2 * (1 - exp2))), 2); // kg/s    
    double del_U = feed * major_term * eff; // kg/s
    
    return del_U;
  }

  double CalcCTherm(double v_a, double temp, double dM) {
    double c_therm = (dM * (pow(v_a, 2)))/(2.0 * gas_const * temp);
    return c_therm;
  }

  double CalcV(double assay){
    return (2.0 * assay - 1.0) * log(assay / (1.0 - assay));
  }

  double AlphaBySwu(double del_U, double feed, double cut, double M){
    double alpha = 1 + std::sqrt((2 * (del_U / M) * (1 - cut) / (cut * feed)));
    return alpha;
  }

  // per machine
  double ProductAssayByAlpha(double alpha, double feed_assay){
    // Possibly incorrect is commented out ? 
    //    double ratio = (1.0 - feed_assay) / (alpha * feed_assay);
    //    return 1.0 / (ratio + 1.0);
    double ratio = alpha * feed_assay/(1.0 - feed_assay);
    return ratio / (1 + ratio);
  }

  double WasteAssayByAlpha(double alpha, double feed_assay){
    double A = (feed_assay / (1 - feed_assay)) / alpha;
    return A / (1 + A);
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

std::pair<int, int> FindNStages(double alpha, double feed_assay,
					     double product_assay,
					     double waste_assay){
    using std::pair;

    int ideal_enrich_stage = 0;
    int ideal_strip_stage = 0;
    double stage_feed_assay = feed_assay;
    double stage_product_assay = feed_assay;
    double stage_waste_assay = feed_assay;  //start w/waste of 1st enrich stage

    // Calculate number of enriching stages
    while (stage_product_assay < product_assay){
      stage_product_assay = ProductAssayByAlpha(alpha, stage_feed_assay);
      if (ideal_enrich_stage == 0){
	stage_waste_assay = WasteAssayByAlpha(alpha, stage_feed_assay);
      }
      ideal_enrich_stage +=1;
      stage_feed_assay = stage_product_assay;
    }
    // Calculate number of stripping stages
    stage_feed_assay = stage_waste_assay;
    while (stage_waste_assay > waste_assay){
      stage_waste_assay = WasteAssayByAlpha(alpha, stage_feed_assay);
      ideal_strip_stage += 1;
      stage_feed_assay = stage_waste_assay;
    }
    
    std::pair<int, int> stages = std::make_pair(ideal_enrich_stage,
						ideal_strip_stage);
    return stages;
    
  }
  
  double ProductAssayFromNStages(double alpha, double feed_assay,
			    double enrich_stages){
    double A = (feed_assay / (1 - feed_assay)) *
      exp(enrich_stages * (alpha - 1.0));
    double product_assay = A / (1 + A);
    return product_assay;
  }
  
  double WasteAssayFromNStages(double alpha, double feed_assay,
			       double strip_stages){
    return 1/(1 + ((1 - feed_assay) / feed_assay) *
	      exp(strip_stages * (alpha - 1.0)));
  }

  double MachinesPerStage(double alpha, double del_U, double stage_feed){
    return stage_feed / (2.0 * del_U / (pow((alpha - 1.0), 2)));
  }

  double ProductPerEnrStage(double alpha, double feed_assay,
			    double product_assay, double stage_feed){
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
  
  double DeltaUCascade(double product_assay, double waste_assay,
		       double feed_flow, double product_flow){
    double Vpc = CalcV(product_assay);
    double Vwc = CalcV(waste_assay);
    return product_flow * Vpc + (feed_flow - product_flow) * Vwc;
  }
  
  double MachinesPerCascade(double del_U_machine, double product_assay,
			    double waste_assay, double feed_flow,
			    double product_flow){
    double U_cascade = DeltaUCascade(product_assay, waste_assay, feed_flow,
				     product_flow);
    return U_cascade / del_U_machine;
  }

  double DelUByCascadeConfig(double product_assay, double waste_assay,
			     double product_flow, double waste_flow,
			     double feed_assay){
    double U_cascade = DeltaUCascade(product_assay, waste_assay, product_flow,
				     waste_flow);
    return U_cascade / feed_assay;
  }


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Calculate steady-state flow rates into each stage
/*  std::vector<double> CalcFeedFlows(std::pair<double, double> n_stages,
				    double cascade_feed, double cut){

    int n_enrich = n_stages.first;
    int n_strip = n_stages.second;
    int n_stages = n_stages.first + n_stages.second;

    std::vector <double> eqn_answers;
    for (int ind = 0; ind < n_stages; ind++){
      int i = ind - n_strip;
      int position = n_stages_strip + i;

      eqn = np.zeros(n_stages)

	  eqn[position] = -1
        if (position != 0):
            eqn[position - 1] = cut
        if (position != n_stages - 1):
            eqn[position + 1] = (1-cut)
        if (position == 0):
            eqn_array = eqn
        else:
            eqn_array = np.vstack((eqn_array,eqn))
        if (i == 0):
            eqn_answers[position] = -1*cascade_feed

    return np.linalg.solve(eqn_array, eqn_answers)


    
  }
*/
  
} // namespace mbmore
