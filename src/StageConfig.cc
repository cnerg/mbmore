// Implements the CascadeEnrich class
#include "StageConfig.h"
#include "cyclus.h"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

namespace mbmore {

StageConfig::StageConfig(CentrifugeConfig cent, double f_assay,
                         double precision__, double feed__)
    : centrifuge(cent),
      feed_assay_(f_assay),
      feed_flow_(feed__),
      precision_(precision__) {
  BuildIdealStg();
}
StageConfig::StageConfig(double f_assay, double feed__, double cut__,
                         double DU__, double alpha__, double precision__)
    : centrifuge(),
      feed_assay_(f_assay),
      feed_flow_(feed__),
      precision_(precision__),
      cut_(cut__),
      DU_(DU__),
      alpha_(alpha__) {
  // if alpha is not provided, compute it from the dU
  if (alpha_ == -1) {
    AlphaByDU();
  }

  BetaByAlphaAndCut();
  ProductAssay();
  TailAssay();
}

// Search for cut where alpha = beta by starting at cut = 0.1, 0.9
void StageConfig::CutForIdealStg() {
  // Calculating high and low parameters
  double low_cut = cut_ = 0.1;
  DU_ = centrifuge.ComputeDeltaU(cut_);
  AlphaByDU();
  BetaByAlphaAndCut();
  double low_alpha = alpha_;
  double low_beta = beta_;
  double high_cut = cut_ = 0.9;
  DU_ = centrifuge.ComputeDeltaU(cut_);
  AlphaByDU();
  BetaByAlphaAndCut();
  double high_alpha = alpha_;
  double high_beta = beta_;

  double p_alpha, p_beta, p_cut;
  // Set initial guess to closer cut values
  if (std::abs(low_alpha - low_beta) < std::abs(high_alpha - high_beta)) {
    alpha_ = low_alpha;
    beta_ = low_beta;
    cut_ = low_cut;

    p_alpha = high_alpha;
    p_beta = high_beta;
    p_cut = high_cut;
  } else {
    alpha_ = high_alpha;
    beta_ = high_beta;
    cut_ = high_cut;

    p_alpha = low_alpha;
    p_beta = low_beta;
    p_cut = low_cut;
  }

  double p_alpha_minus_beta = p_alpha - p_beta;
  while (std::abs(alpha_ - beta_) > precision_) {
    // a*cut +b =y
    double alpha_minus_beta = alpha_ - beta_;
    double a = (p_alpha_minus_beta - alpha_minus_beta) / (p_cut - cut_);
    double b = alpha_minus_beta - cut_ * a;
    // old = new
    p_alpha_minus_beta = alpha_minus_beta;
    p_cut = cut_;
    // targeting alpha_minus_beta = 0
    cut_ = -b / a;
    DU_ = centrifuge.ComputeDeltaU(cut_);
    AlphaByDU();
    BetaByAlphaAndCut();
  }
}

void StageConfig::ProductAssay() {
  double ratio = alpha_ * feed_assay_ / (1. - feed_assay_);
  product_assay_ = ratio / (1. + ratio);
}

void StageConfig::TailAssay() {
  double A = (feed_assay_ / (1. - feed_assay_)) / beta_;
  tail_assay_ = A / (1. + A);
}

void StageConfig::AlphaByDU() {
  double M = centrifuge.M;  // UF6 kg/mol
  double M_238 = 0.238;     // U kg/mol
  double ratio_UF6_U = M_238 / M;

  // converting feed from UF6 to U
  double feed = centrifuge.feed * ratio_UF6_U;
  // "Uranium Enrichment By Gas Centrifuge" D.G. Avery & E. Davies pg. 18
  alpha_ = 1. + std::sqrt((2. * DU_ * (1. - cut_) / (cut_ * feed)));
}

void StageConfig::AlphaByProductAssay() {
  alpha_ =
      (1 - feed_assay_) / feed_assay_ * product_assay_ / (1 - product_assay_);
}

void StageConfig::BetaByAlphaAndCut() {
  ProductAssay();
  double waste_assay = (feed_assay_ - cut_ * product_assay_) / (1. - cut_);

  beta_ = feed_assay_ / (1. - feed_assay_) * (1. - waste_assay) / waste_assay;
}

void StageConfig::ProductAssayByGamma(double gamma) {
  product_assay_ = (-sqrt(pow((feed_assay_ - cut_) * gamma, 2) +
                          gamma * (2 * feed_assay_ + 2 * cut_ -
                                   2 * pow(feed_assay_, 2) - 2 * pow(cut_, 2)) +
                          pow(feed_assay_ + cut_ - 1, 2)) +
                    gamma * (feed_assay_ + cut_) - feed_assay_ - cut_ + 1) /
                   (2 * cut_ * (gamma - 1));
}

void StageConfig::CutByAlphaBeta() {
  ProductAssay();
  TailAssay();

  cut_ = (feed_assay_ - tail_assay_) / (product_assay_ - tail_assay_);
}

void StageConfig::BuildIdealStg() {
  if (DU_ == -1 || alpha_ == -1) {
    CutForIdealStg();
    DU_ = centrifuge.ComputeDeltaU(cut_);
    AlphaByDU();
  }

  beta_ = alpha_;
  CutByAlphaBeta();
  ProductAssay();
  TailAssay();
}

void StageConfig::MachinesNeededPerStage(double tolerance) {
  // n_machines: the denominator should be equal to the
  // centrifuge feed flow (centrifuge.feed).

  double M = centrifuge.M;  // UF6 kg/mol
  double M_238 = 0.238;     // U kg/mol
  double ratio_UF6_U = M_238 / M;

  // "Uranium Enrichment By Gas Centrifuge" D.G. Avery & E. Davies pg. 18
  double centrifuge_feed_flow =
      2 * DU_ * ((1 - cut_) / cut_) / pow((alpha_ - 1.), 2.) / ratio_UF6_U;
  double n_exact = this->feed_flow_ / (centrifuge_feed_flow);

  // Adds a machine if fractional amount is needed
  n_machines_ = int(n_exact);
  if (std::abs(n_exact - n_machines_) > tolerance) {
    n_machines_ = int(n_exact) + 1;
  }
}
double StageConfig::SWU() {
  using cyclus::toolkit::Assays;
  using cyclus::toolkit::SwuRequired;
  if (product_assay_ == -1 || feed_assay_ == -1 || tail_assay_ == 1 ||
      feed_flow_ == -1)
    return -1;

  return SwuRequired(feed_flow_,
                     Assays(feed_assay_, product_assay_, tail_assay_));
}

}  // namespace mbmore
