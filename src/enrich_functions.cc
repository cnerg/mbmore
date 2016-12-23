#define _USE_MATH_DEFINES

#include "enrich_functions.h"
#include <ctime> // to make truly random
#include <cstdlib>
#include <iostream>
#include <cmath>

namespace mbmore {

  double D_rho = 2.2e-5; // kg/m/s
  double R = 8.315; // J/K/mol
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: RENAME VARIABLES TO BE MORE INTUITIVE
// CONVERT N stages, N machines to integer-limited
// annotate assumed units, Glaser paper reference, Z_P (What is it?)
// Math Functions to Check: pi (M_PI??), exp??

  double CalcDelU(double v_a, double Z, double d,  double F_m, double T,
		  double cut, double eff, double M, double dM, double x,
		  double L_F) {

    // L_F = 2-4, x = pressure ratio, M = 0.352 kg/mol of UF6, dM = 0.003 kg/mol diff between 235 and 238
    

    double a = d/2.0;  // outer radius

    // withdrawl radius for heavy isotope
    // Glaser 2009 says operationally it ranges from 0.96-0.99
    double r_2 = 0.99 * a; 

    double r_12 = std::sqrt(1.0 - (2.0 * R * T*(log(x)) / M / (pow(v_a,2)))); 
    double r_1 = r_2 * r_12; // withdrawl radius for ligher isotope

    // Glaser eqn 12
    // Vertical location of feed
    double Z_p = Z * (1.0 - cut) * (1.0 + L_F) / (1.0 - cut + L_F);

    //Glaser eqn 3
    double C1 = (2.0 * M_PI * D_rho / (log(r_2 / r_1)));
    double A_p = C1 *(1.0 / F_m) * (cut / ((1.0 + L_F) * (1.0 - cut + L_F)));
    double A_w = C1 * (1.0 / F_m) * ((1.0 - cut)/(L_F * (1.0 - cut + L_F)));
     
    double C_therm = CalcCTherm(v_a, T, dM);

    // defining terms in the Ratz equation
    double r12_sq = pow(r_12,2);
    double C_scale = (pow((r_2 / a), 4)) * (pow((1 - r12_sq),2));
    double bracket1 = (1 + L_F) / cut;
    double exp1 = exp(-1.0 * A_p * Z_p);
    double bracket2 = L_F/(1 - cut);
    double exp2 = exp(-1.0 * A_w * (Z - Z_p));

    // Glaser eqn 10 (Ratz Equation)
    double major_term = 0.5 * cut * (1.0 - cut) * (pow(C_therm, 2)) * C_scale *
      pow(((bracket1 * (1 - exp1)) + (bracket2 * (1 - exp2))),2); // kg/s    
    double del_U = F_m * major_term * eff; // kg/s
    
    return del_U;
  }

  double CalcCTherm(double v_a, double T, double dM) {
    double c_therm = (dM * (pow(v_a,2)))/(2.0 * R * T);
    return c_therm;
  }

  double CalcV(double N_in){
    return (2.0 * N_in - 1.0) * log(N_in / (1.0 - N_in));
  }

  // Avery p 18
  // del_U should have units of moles/sec
  double AlphaBySwu(double del_U, double F_m, double cut, double M){
    double alpha = 1 + std::sqrt((2 * (del_U / M) * (1 - cut) / (cut * F_m)));
    return alpha;
  }

  // per machine
  double NProductByAlpha(double alpha, double Nfm){
    double ratio = (1.0 - Nfm) / (alpha * Nfm);
    return 1.0 / (ratio + 1.0);
  }

  // Avery p 59 (per machine)
  double NWasteByAlpha(double alpha, double Nfm){
    double A = (Nfm / (1 - Nfm)) / alpha;
    return A / (1 + A);
  }

  // Avery ???
  // TODO: CONVERT THIS TO INTEGER NUMBER!!
  std::pair<double,double> StagesPerCascade(double alpha, double Nfc, double Npc, double Nwc){

    using std::pair;

    double epsilon = alpha - 1.0;
    double enrich_inner = (Npc / (1.0 - Npc)) * ((1.0 - Nfc) / Nfc);
    double strip_inner =  (Nfc / (1.0 - Nfc)) * ((1.0 - Nwc) / Nwc);

    double enrich_stages = (1.0 / epsilon) * log(enrich_inner);
    double strip_stages = (1.0 / epsilon) * log(strip_inner);
    std::pair<double, double> stages = std::make_pair(enrich_stages, strip_stages);
    return stages;

  }   

  
} // namespace mbmore