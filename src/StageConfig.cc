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

// Search for cut where alpha = beta by starting at cut = 0.1, 0.9
double StageConfig::CutForIdealStg(double f_assay, double precision) {
  feed_assay = f_assay;

  // Calculating high and low parameters
  double p_cut = cut = 0.1;
  (*this).DU = centrifuge.ComputeDeltaU(cut);
  double low_alpha = AlphaByDU();
  double low_beta = BetaByAlphaAndCut();
  double high_cut = cut = 0.9;
  (*this).DU = centrifuge.ComputeDeltaU(cut);
  double high_alpha = AlphaByDU();
  double high_beta = BetaByAlphaAndCut();

  // Set initial guess to closer cut values
  if (std::abs(low_alpha - low_beta) < std::abs(high_alpha - high_beta)) {
    alpha = low_alpha;
    beta = low_beta;
    cut = low_cut;

    double p_alpha = high_alpha;
    double p_beta = high_beta;
    double p_cut = high_cut;
  } else {
    alpha = high_alpha;
    beta = high_beta;
    cut = high_cut;

    double p_alpha = low_alpha;
    double p_beta = low_beta;
    double p_cut = low_cut;
  }

  double p_alpha_minus_beta = p_alpha - p_beta;
  while (std::abs(alpha - beta) > precision) {
    // a*cut +b =y
    double alpha_minus_beta = alpha - beta;
    double a = (p_alpha_minus_beta - alpha_minus_beta) / (p_cut - cut);
    double b = alpha_minus_beta - cut * a;
    // old = new
    p_alpha_minus_beta = alpha_minus_beta;
    p_cut = cut;
    // targeting alpha_minus_beta = 0
    cut = -b / a;
    DU = centrifuge.ComputeDeltaU(cut);
    AlphaByDU();
    BetaByAlphaAndCut();
  }
  return cut;
}

double StageConfig::ProductAssay() {
  double ratio = alpha * feed_assay / (1. - feed_assay);
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
  // "Uranium Enrichment By Gas Centrifuge" D.G. Avery & E. Davies pg. 18
  alpha = 1. + std::sqrt((2. * (DU / M) * (1. - cut) / (cut * feed)));
  return alpha;
}

double StageConfig::BetaByAlphaAndCut() {
  double product_assay = ProductAssay();
  double waste_assay = (feed_assay - cut * product_assay) / (1. - cut);

  beta = feed_assay / (1. - feed_assay) * (1. - waste_assay) / waste_assay;
  return beta;
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
  }

  beta = alpha;
  CutByAlphaBeta();
  ProductAssay();
  TailAssay();
}

double StageConfig::MachinesNeededPerStage() {
  // n_machines: the denominator should be equal to the
  // centrifuge feed flow (centrifuge.feed).

  // "Uranium Enrichment By Gas Centrifuge" D.G. Avery & E. Davies pg. 18
  cfeed_flow = (2 * DU / M) * ((1 - cut) / cut) / pow((alpha - 1.), 2.);
  //n_machines = (feed_flow / ((2 * DU / M) * ((1 - cut) / cut) / pow((alpha - 1.), 2.)));
  n_machines = std::round(feed_flow / cfeed_flow);
  return n_machines;
}

}  // namespace mbmore
