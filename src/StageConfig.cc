// Implements the CascadeEnrich class
#include "StageConfig.h"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

namespace mbmore {

StageConfig::StageConfig(CentrifugeConfig cent, double f_assay, double feed_,
                         double precision_)
    : centrifuge(cent),
      feed_assay(f_assay),
      feed_flow(feed_),
      precision(precision_) {
  BuildIdealStg(feed_assay, precision);
}
StageConfig::StageConfig(double f_assay, double feed_, double cut_, double DU_,
                         double alpha_, double precision_)
    : feed_assay(f_assay),
      feed_flow(feed_),
      precision(precision_),
      cut(cut_),
      DU(DU_),
      alpha(alpha_) {
  // if alpha is not provided, compute it from the dU
  if (alpha == -1) {
    AlphaByDU();
  }

  BetaByAlphaAndCut();
  ProductAssay();
  TailAssay();
}
double StageConfig::CutForIdealStg(double f_assay, double precision) {
  feed_assay = f_assay;
  double p_cut = cut = 0.1;
  (*this).DU = centrifuge.ComputeDeltaU(cut);
  double p_alpha = AlphaByDU();
  double p_beta = BetaByAlphaAndCut();
  double p_alpha_m_beta = p_alpha - p_beta;
  cut = 0.9;
  (*this).DU = centrifuge.ComputeDeltaU(cut);
  AlphaByDU();
  BetaByAlphaAndCut();
  while (std::abs(alpha - beta) > precision) {
    // a*cut +b =y
    double alpha_m_beta = alpha - beta;
    double a = (p_alpha_m_beta - alpha_m_beta) / (p_cut - cut);
    double b = alpha_m_beta - cut * a;
    // old = new
    p_alpha_m_beta = alpha_m_beta;
    p_cut = cut;
    // targeting alpha_m_beta = 0
    cut = -b / a;
    DU = centrifuge.ComputeDeltaU(cut);
    AlphaByDU();
    BetaByAlphaAndCut();
  }
  return cut;
}

double StageConfig::ProductAssay() {
  double ratio = alpha * feed_assay / (1.0 - feed_assay);
  product_assay = ratio / (1. + ratio);
  return product_assay;
}

double StageConfig::TailAssay() {
  double A = (feed_assay / (1. - feed_assay)) / beta;
  (*this).tail_assay = A / (1. + A);
  return (*this).tail_assay;
}

double StageConfig::AlphaByDU() {
  double feed = centrifuge.feed;
  double M = centrifuge.M;
  (*this).alpha = 1 + std::sqrt((2 * (DU / M) * (1 - cut) / (cut * feed)));
  return (*this).alpha;
}

double StageConfig::AlphaByProductAssay(){
  (*this).alpha = (1 - feed_assay)/feed_assay * product_assay/(1 - product_assay);
  return (*this).alpha;
}

double StageConfig::BetaByAlphaAndCut() {
  double product_assay = ProductAssay();
  double waste_assay = (feed_assay - cut * product_assay) / (1. - cut);

  beta = feed_assay / (1. - feed_assay) * (1. - waste_assay) / waste_assay;
  return beta;
}

double StageConfig::ProductAssayByGamma(double gamma){
  double prod_assay = ( - sqrt( pow( (feed_assay - cut) * gamma, 2)
                              + gamma * ( 2*feed_assay + 2*cut 
                                          - 2*pow(feed_assay, 2)
                                          - 2*pow(cut, 2))
                        + pow(feed_assay + cut -1, 2))
                        + gamma*(feed_assay + cut)
                        - feed_assay - cut + 1 )
                      / (2*cut * (gamma -1));
  
  (*this).product_assay = prod_assay;
  
  return (*this).product_assay;
}



double StageConfig::CutByAlphaBeta() {
  double product_assay = ProductAssay();
  double tail_assay = TailAssay();

  cut = (feed_assay - tail_assay) / (product_assay - tail_assay);
  return cut;
}

void StageConfig::BuildIdealStg(double f_assay, double precision) {
  feed_assay = f_assay;
  if (DU == -1 || alpha == -1) {
    CutForIdealStg(feed_assay, precision);
    DU = centrifuge.ComputeDeltaU(cut);
    AlphaByDU();
    beta = alpha;
    CutByAlphaBeta();
  } else {
    beta = alpha;
    CutByAlphaBeta();
  }
  ProductAssay();
  TailAssay();
}

double StageConfig::MachinesPerStage() {
  n_machines = feed_flow / (2.0 * DU / (pow((alpha - 1.0), 2)));
  return n_machines;
}

double StageConfig::ProductPerEnrStage() {
  return feed_flow * (alpha - 1.0) * feed_assay * (1 - feed_assay) /
         (2 * (product_assay - feed_assay));
}

}  // namespace mbmore
