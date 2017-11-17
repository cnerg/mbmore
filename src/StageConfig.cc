// Implements the CascadeEnrich class
#include "CascadeConfig.h"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <limits>
#include <sstream>
#include <vector>

namespace mbmore {


double StageConfig::ProductAssayByAlpha(double alpha, double feed_assay) {
  double ratio = alpha * feed_assay / (1.0 - feed_assay);
  return ratio / (1. + ratio);
}

double StgeConfig::TailAssayByBeta(double beta, double feed_assay) {
  double A = (feed_assay / (1. - feed_assay)) / beta;
  return A / (1. + A);
}

double StageConfig::AlphaBySwu(double del_U, double cut) {
  double feed = centrifuge.feed;
  double M = centrifuge.M;
  double alpha = 1 + std::sqrt((2 * (del_U / M) * (1 - cut) / (cut * feed)));
  return alpha;
}

double StageConfig::BetaByAlphaAndCut(double alpha, double feed_assay, double cut) {
  double product_assay = ProductAssayByAlpha(alpha, feed_assay);
  double waste_assay = (feed_assay - cut * product_assay) / (1. - cut);

  return feed_assay / (1. - feed_assay) * (1. - waste_assay) / waste_assay;
}

double StageConfig::CutByAalphaBeta(double alpha, double beta, double feed_assay) {
  double product_assay = ProductAssayByAlpha(alpha, feed_assay);
  double tail_assay = TailAssayByBeta(beta, feed_assay);

  return (feed_assay - tail_assay) / (product_assay - tail_assay);
}

void StageConfig::BuildIdealStg(double f_assay, double precision) {
  feed_assay = f_assay;
  
  if (du == -1 || alpha == -1 ) {
    cut = get_cut_for_ideal_stg(centrifuge, stg.feed_assay, precision);
    deltaU = centrifuge.CalcDelU(cut);
    alpha = AlphaBySwu(deltaU, cut);
    beta = BetaByAlphaAndCut(stg.alpha, stg.feed_assay, stg.cut);
  } else {
    beta = alpha;
    cut = CutByAalphaBeta(alpha, alpha, feed_assay);
  }
  product_assay = ProductAssayByAlpha(stg.alpha, stg.feed_assay);
  tail_assay = TailAssayByBeta(stg.beta, stg.feed_assay);

  return stg;
}


}  // namespace mbmore
