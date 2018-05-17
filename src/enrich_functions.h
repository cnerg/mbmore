#ifndef MBMORE_SRC_ENRICH_FUNCTIONS_H_
#define MBMORE_SRC_ENRICH_FUNCTIONS_H_

#include <string>
#include <vector>

#include "cyclus.h"

namespace mbmore {

// LAPACK solver for system of linear equations
extern "C" {
     void dgesv_(int *n, int *nrhs,  double *a,  int  *lda,  
           int *ipivot, double *b, int *ldb, int *info) ;
}
  


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


  // Determines the number of machines and product in each stage based
  // on the steady-state flows defined for the cascade.
  std::vector<std::pair<int, double>> CalcStageFeatures(double feed_assay,
						    double alpha, double del_U,
						    double cut,
						    std::pair<int, int> n_st,
						    std::vector<double> feed_flow);


  std::pair<int,double> DesignCascade( double design_feed, double design_alpha,
				       double design_delU, double cut,
				       int max_centrifuges,
				       std::pair<int,int> n_stages);

  
} // namespace mbmore

#endif  //  MBMORE_SRC_ENRICH_FUNCTIONS_H_
