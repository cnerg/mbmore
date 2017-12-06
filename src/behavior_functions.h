#ifndef MBMORE_SRC_BEHAVIOR_FUNCTIONS_H_
#define MBMORE_SRC_BEHAVIOR_FUNCTIONS_H_

#include <string>
#include <vector>
#include "cyclus.h"

namespace mbmore {

bool SortBids(cyclus::Bid<cyclus::Material>* i,
              cyclus::Bid<cyclus::Material>* j); 
// returns true every X interval (ie every 5th timestep)
bool EveryXTimestep(int curr_time, int interval);

// randomly returns true with a frequency X
// (ie returns true ~20 randomly selected timesteps
// out of 100 when frequency = 5 )
//bool EveryRandomXTimestep(int frequency);

bool EveryRandomXTimestep(int frequency, int rng_seed);

// returns True with a defined probability
// (ie. if probability is 0.2 then will return True on average
// 1 in 5 calls).
// 
bool XLikely(double prob, int rng_seed);

// returns a randomly generated number from a
// normal distribution defined by mean and
// sigma (full-width-half-max)
//double RNG_NormalDist(double mean, double sigma);

 
double RNG_NormalDist(double mean, double sigma, int rng_seed);

// returns a randomly chosen discrete number between min and max
// (ie. integer betweeen 1 and 5)

double RNG_Integer(double min, double max, int rng_seed);

// For various types of time varying curves, calculate y for some x
double CalcYVal(std::string function, std::vector<double> constants,
		double x_val);

// Convert probability integrated over n_timesteps (L, N) to a probability (P)
// at single time, by solving for P:  L = 1 - (1-P)^N 
double ProbPerTime(double xval, double n_timesteps);

} // namespace mbmore

#endif  //  MBMORE_SRC_BEHAVIOR_FUNCTIONS_H_
