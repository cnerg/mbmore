#ifndef MBMORE_SRC_ENRICH_FUNCTIONS_H_
#define MBMORE_SRC_ENRICH_FUNCTIONS_H_

#include <string>
#include <vector>

namespace mbmore {

// LAPACK solver for system of linear equations
extern "C" {
     void dgesv_(int *n, int *nrhs,  double *a,  int  *lda,  
           int *ipivot, double *b, int *ldb, int *info) ;
}

  
  // Calculates the ideal separation energy for a single machine 
  // as defined by the Raetz equation
  // (referenced in Glaser, Science and Global Security 2009)
  double CalcDelU(double v_a, double height, double diameter,  double feed,
		  double temp, double cut, double eff, double M, double dM,
		  double x, double flow_internal);

  // Calculates the exponent for the energy distribution using ideal gas law
  // (component of multiple other equations)
  double CalcCTherm(double v_a, double temp, double dM);

  // Calculates the V(N_x) term for enrichment eqns where N_x is the assay
  // of isotope x
  double CalcV(double assay);

  // Calculates the separations factor given the ideal separation energy of a
  // single machine (del_U has units of moles/sec)
  // Avery p 18
  double AlphaBySwu(double del_U, double feed, double cut, double M);

  // Calculates the assay of the product given the assay
  // of the feed and the theoretical separation factor of the machine
  // Avery p 57
  double ProductAssayByAlpha(double alpha, double feed_assay);

  // Calculates the assay of the waste given the assay
  // of the feed and the theoretical separation factor of the machine
  // Avery p 59 (per machine)
  double WasteAssayByAlpha(double alpha, double feed_assay);

  // Calculates the number of stages needed in a cascade given the separation
  // potential of a single centrifuge and the material assays
  std::pair<int, int>
    FindNStages(double alpha, double feed_assay, double product_assay,
		     double Nwc);

  // Calculates the product assay after N enriching stages
  double ProductAssayFromNStages(double alpha, double feed_assay,
			    double enrich_stages);

  // Calculates the assay of the waste after N stripping stages
  double WasteAssayFromNStages(double alpha, double feed_assay,
			       double strip_stages);

  // Number of machines in a stage (either enrich or strip)
  // given the feed flow (stage_feed)
  // flows do not have required units so long as they are consistent
  // Feed flow of a single machine (in Avery denoted with L)
  // Avery p. 62
  double MachinesPerStage(double alpha, double del_U, double stage_feed);
  
  // Flow of Waste (in same units and feed flow) in each stripping stage
  // F_stage = incoming flow (in Avery denoted with L_r)
  // Avery p. 60
  double ProductPerEnrStage(double alpha, double feed_assay,
			     double product_assay, double stage_feed);

    // Flow of Waste (in same units and feed flow) in each stripping stage
  // F_stage = incoming flow (in Avery denoted with L_r)
  // Avery p. 60
  double WastePerStripStage(double alpha, double feed_assay, double waste_assay,
			    double stage_feed);

  // Separation potential of the cascade
  // ???
  double DeltaUCascade(double product_assay, double waste_assay,
		       double feed_flow, double Pc);

  // Number of machines in the cascade given the target feed rate and target
  // assays and flow rates
  // Avery 62
  double MachinesPerCascade(double del_U_machine, double product_assay,
			    double waste_assay, double feed_flow,
			    double product_flow);

  // Effective separation potential of a single machine when cascade is not
  // being used in optimal configuration, as defined by the non-optimal
  // assays and flow rates of the cascade
  // ????
  double DelUByCascadeConfig(double product_assay, double waste_assay,
			     double product_flow, double waste_flow,
			     double feed_assay);

  // Solves system of linear eqns to determine steady state flow rates
  // in each stage of cascade
  //  std::vector<double> CalcFeedFlows(std::pair<double, double> n_st,
  //  double** CalcFeedFlows(std::pair<double, double> n_st,
  //  void CalcFeedFlows(std::pair<double, double> n_st,
  std::vector<double> CalcFeedFlows(std::pair<double, double> n_st,
				    double cascade_feed, double cut);

  
} // namespace mbmore

#endif  //  MBMORE_SRC_ENRICH_FUNCTIONS_H_
