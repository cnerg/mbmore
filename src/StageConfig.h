#ifndef MBMORE_SRC_CASCADE_ENRICH_H_
#define MBMORE_SRC_CASCADE_ENRICH_H_

#include <string>

namespace mbmore {

class CentrifugeConfig : {
 public:
  CentrifugeConfig();
  CentrifugeConfig(double v_a, double h, double d, double feed, double T,
                   double eff, double M, double dM, double x, double i_flow);

  double ComputeDeltaU(double cut);
  double CalcCTherm(double v_a, double temp, double dM);
  double CalcV(double assay);

  // flow_internal = 2-4, x = pressure ratio, M = 0.352 kg/mol of UF6,
  // dM = 0.003 kg/mol diff between 235 and 238
  double M;
  double dM;
  double x;
  double flow_internal;

  double v_a;
  double height;
  double diameter;
  double feed;
  double temp;
  double eff;
};

class CascadeConfig : {
 public:
  CascadeConfig();

  void BuildIdealStg(double feed_assay, double precision = 1e-16);
  double get_cut_for_ideal_stg(double feed_assay, double precision = 1e-16);
  double ComputeAlpha(double DeltaU, double feed_flow, double cut, double M);
  double BetaByAlphaAndCut(double alpha, double feed_assay, double cut);
  double CutByAalphaBeta(double alpha, double beta, double feed_assay);
  double MachinesPerStage(double alpha, double del_U, double stage_feed);

  // Calculates the V(N_x) term for enrichment eqns where N_x is the assay
  // of isotope x

 private:
  CentrifugeConfig centrifuge;

  double cut;
  double DU;
  double alpha;
  double beta;
  double flow;

  double n_machines;

  double feed_assay;
  double product_assay;
  double tail_assay;
};

class CascadeConfig : {
 public:
  void BuildIdealCascade(double f_assay, double p_assay, double w_assay,
                         double precision = 1e-16);

  void CalcFeedFlows();
  void CalcStageFeatures();
  void DesignCascade(double max_feed, int max_centrifuges);
  CascadeConfig Compute_Assay(cascade_config cascade_config, double feed_assay,
                              double precision);

  double Diff_enrichment(CascadeConfig actual_enrichments,
                         CascadeConfig previous_enrichement);

  std::map<int, stg_config> Update_enrichment(cascade_config cascade,
                                              double feed_assay);

  // Number of machines in the cascade given the target feed rate and target
  // assays and flow rates
  // Avery 62
  double MachinesPerCascade(double del_U_machine, double product_assay,
                            double waste_assay, double feed_flow,
                            double product_flow);

  pirivate : centrifuge_config cent_config;
  std::map<int, stg_config> stgs_config;

  int n_enrich = 0;
  int n_strip = 0;
  double feed_flow;
};

}  // namespace mbmore

#endif  // MBMORE_SRC_CASCADE_ENRICH_H_
