// Implements the CascadeEnrich class
#include "StageConfig.h"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <limits>
#include <sstream>
#include <vector>

namespace mbmore {

  StageConfig::StageConfig() {}

double StageConfig::CutForIdealStg(double f_assay, double precision) {
  feed_assay = f_assay;
  double p_cut = 0.01;
  StageConfig p_stg = (*this);
  p_stg.centrifuge.ComputeDeltaU(p_cut);

  double p_alpha = p_stg.AlphaByDU();
  double p_beta = p_stg.BetaByAlphaAndCut();
  double p_alpha_m_beta = p_alpha - p_beta;

  cut = 0.99;
  DU = centrifuge.ComputeDeltaU(cut);
  AlphaByDU();
  BetaByAlphaAndCut();
  while (std::abs(alpha - beta) > precision) {
    // a*cut +b =y
    double alpha_m_beta = alpha - beta;
    double a = (p_alpha_m_beta - alpha_m_beta) / (p_cut - cut);
    double b = alpha_m_beta - cut * a;

    // old = new
    p_stg = (*this);
    p_alpha_m_beta = alpha_m_beta;

    // targeting alpha_m_beta = 0
    cut = -b / a;
    DU = centrifuge.ComputeDeltaU(cut);
    alpha = AlphaByDU();
    beta = BetaByAlphaAndCut();
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
  tail_assay = A / (1. + A);
  return tail_assay;
}

double StageConfig::AlphaByDU() {
  double feed = centrifuge.feed;
  double M = centrifuge.M;
  alpha = 1 + std::sqrt((2 * (DU / M) * (1 - cut) / (cut * feed)));
  return alpha;
}

double StageConfig::BetaByAlphaAndCut() {
  double product_assay = ProductAssay();
  double waste_assay = (feed_assay - cut * product_assay) / (1. - cut);

  beta = feed_assay / (1. - feed_assay) * (1. - waste_assay) / waste_assay;
  return beta;
}

double StageConfig::CutByAalphaBeta() {
  double product_assay = ProductAssay();
  double tail_assay = TailAssay();

  cut = (feed_assay - tail_assay) / (product_assay - tail_assay);
  return cut;
}

void StageConfig::BuildIdealStg(double f_assay, double precision) {
  feed_assay = f_assay;

  if (DU == -1 || alpha == -1) {
    cut = CutForIdealStg(feed_assay, precision);
    DU = centrifuge.ComputeDeltaU(cut);
    alpha = AlphaByDU();
    beta = BetaByAlphaAndCut();
  } else {
    beta = alpha;
    cut = CutByAalphaBeta();
  }
  product_assay = ProductAssay();
  tail_assay = TailAssay();
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
