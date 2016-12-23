#ifndef MBMORE_SRC_ENRICH_FUNCTIONS_H_
#define MBMORE_SRC_ENRICH_FUNCTIONS_H_

#include <string>
#include <vector>

namespace mbmore {

  // Calculates the ideal separation energy for a single machine 
  // as defined by the Raetz equation
  // (referenced in Glaser, Science and Global Security 2009)
  double CalcDelU(double v_a, double Z, double d,  double F_m, double T,
		  double cut, double eff, double M, double dM, double x,
		   double L_F);

  // Calculates the exponent for the energy distribution using ideal gas law
  // (component of multiple other equations)
  double CalcCTherm(double v_a, double T, double dM);

  // Calculates the V(N_x) term for enrichment eqns where N_x is the assay
  // of isotope x
  double CalcV(double N_in);

  // Calculates the separations factor given the ideal separation energy of a
  // single machine
  double AlphaBySwu(double del_U, double F_m, double cut, double M);

  // Calculates the assay of the product given the assay
  // of the feed and the theoretical separation factor of the machine
  double NProductByAlpha(double alpha, double Nfm);

  // Calculates the assay of the waste given the assay
  // of the feed and the theoretical separation factor of the machine
  double NWasteByAlpha(double alpha, double Nfm);



  // Calculates the number of stages needed in a cascade given the separation
  // potential of a single centrifuge and the material assays
  std::pair<double, double>
    StagesPerCascade(double alpha, double Nfc, double Npc, double Nwc);

  
  
} // namespace mbmore

#endif  //  MBMORE_SRC_ENRICH_FUNCTIONS_H_
