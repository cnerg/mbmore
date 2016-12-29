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

  // Calculates the product assay after N enriching stages
  double NpcFromNstages(double alpha, double Nfc, double enrich_stages);

  // Calculates the assay of the waste after N stripping stages
  double NwcFromNstages(double alpha, double Nfc, double strip_stages);


  // Number of machines in the enriching stage given the feed flow (Fs)
  // flows do not have required units so long as they are consistent
  // Feed flow of a single machine (in Avery denoted with L)
  // Avery p. 62
  double MachinesPerEnrStage(double alpha, double del_U, double Fs);
  
  // Number of machines in the stripping stage given the feed flow (Fs)
  // flows do not have required units so long as they are consistent
  // Feed flow of a single machine (in Avery denoted with L)
  // Avery p. 62
  double MachinesPerStripStage(double alpha, double del_U, double Fs);


  // Flow of Waste (in same units and feed flow) in each stripping stage
  // F_stage = incoming flow (in Avery denoted with L_r)
  // Avery p. 60
  double WastePerStripStage(double alpha, double Nfs, double Nws, double Fs);

  // Separation potential of the cascade
  double DeltaUCascade(double Npc, double Nwc, double Fc, double Pc);

  // Number of machines in the cascade given the target feed rate and target
  // assays and flow rates
  double MachinesPerCascade(double del_U_machine, double Npc, double Nwc,
			    double Fc, double Pc);

  // Effective separation potential of a single machine when cascade is not
  // being used in optimal configuration, as defined by the non-optimal
  // assays and flow rates
  double DelUByCascadeConfig(double Npc, double Nwc, double Pc, double Wc,
			     double n_cf);
  
  
} // namespace mbmore

#endif  //  MBMORE_SRC_ENRICH_FUNCTIONS_H_
