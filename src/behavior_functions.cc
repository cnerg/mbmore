#include "behavior_functions.h"
#include <ctime> // to make truly random
#include <cstdlib>
#include <iostream>
#include <cmath>


bool seeded;
namespace mbmore {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EveryXTimestep(int curr_time, int interval) {
  // true when there is no remainder, so it is the Xth timestep
  return curr_time % interval == 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EveryRandomXTimestep(int frequency, int rng_seed) {
  //TODO: Doesn't work for a frequency of 1
  if (frequency == 0) {
    return false;
  }

  if (!seeded) {
    if (rng_seed == -1) {
      srand(time(0));    // seed random
    }
    else {
      srand(rng_seed);   // user-defined fixed seed
    }
    seeded = true;
  }

  // Because this relies on integer rounding, it fails for a frequency of
  // 1 because the midpoint rounds to zero.
  double midpoint;
  (frequency == 1) ? (midpoint = 1) : (midpoint = frequency / 2);
    
  // The interwebs say that rand is not truly random.
  //  tRan = rand() % frequency;
  double cur_rand = rand();
  int tRan = 1 + (cur_rand*(1.0/(RAND_MAX+1.0))) * frequency;
  //  int tRan = 1 + uniform_deviate_(rand()) * frequency;
  //  std::cout << "tRan: " << tRan << " midpoint " << midpoint << std::endl;
  
  if (tRan == midpoint) {
    return true;
  } else {
   return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Returns true for this instance with a particular likelihood of getting a
// True over all instances.

  bool XLikely(double prob, int rng_seed) {

    /*
  if (prob == 0) {
    return false;
  }
    */
    
  if (!seeded) {
    if (rng_seed == -1) {
      srand(time(0));    // seed random
    }
    else {
      srand(rng_seed);   // user-defined fixed seed
    }
    seeded = true;
  }

  double cur_rand = rand();
  double tRan = (cur_rand/RAND_MAX);

  if (tRan <= prob) {
    return true;
  } else {
   return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
bool EveryRandomXTimestep(int frequency) {
  bool time_seed = 0 ;
  return EveryRandomXTimestep(frequency, time_seed);
}
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Use Box-Muller algorithm to make a random number sampled from
// a normal distribution

double RNG_NormalDist(double mean, double sigma, int rng_seed) {

  if (sigma == 0 ) {
    return mean ;
  }

  static double n2 = 0.0;
  static int n2_cached = 0;

  double result ;
  double x, y, r;
  double rand1, rand2;

  if (!seeded) {
    if (rng_seed == -1) {
      srand(time(0)); // if seeding on time
    }
    else {
      srand(rng_seed);  //use fixed seed for reproducibility
    }
    seeded = true;
  }
  
  do {
    rand1 = rand();
    rand2 = rand();
    x = 2.0*rand1/RAND_MAX - 1;
    y = 2.0*rand2/RAND_MAX - 1;
    r = x*x + y*y;
    //    std::cout << rand1/RAND_MAX << "  " << rand2/RAND_MAX  << std::endl;
  } while (r == 0.0 || r > 1.0);
  
  double d = std::sqrt(-2.0*log(r)/r);
  double n1 = x*d;
  n2 = y*d;
  
  /*
  if (!n2_cached) {
    n2_cached = 1;
    return n1*sigma + mean;
  }
  else {
    n2_cached = 0 ;
    return n2*sigma + mean;
  }
  */
  //  std::cout << "NormalDist: " << n1*sigma + mean  << std::endl;
  return n1*sigma + mean;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Randomly choose a discrete number between min and max
// (ie. integer betweeen 1 and 5)

double RNG_Integer(double min, double max, int rng_seed) {

  if (!seeded) {
    if (rng_seed == -1) {
      srand(time(0)); // if seeding on time
    }
    else {
      srand(rng_seed);  //use fixed seed for reproducibility
    }
    seeded = true;
  }

  int tRan = min + (rand()*(1.0/(RAND_MAX+1.0))) * max;

  return tRan;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
double RNG_NormalDist(double mean, double sigma) {
  bool time_seed = 0;
  return RNG_NormalDist(mean, sigma, time_seed);
}
*/
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
// Internal function to make a better random number
double uniform_deviate_ ( int seed ){
  return seed * ( 1.0 / ( RAND_MAX + 1.0 ) );
}
*/


} // namespace mbmore