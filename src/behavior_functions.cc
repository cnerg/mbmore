#include "behavior_functions.h"
#include <ctime> // to make truly random
#include <cstdlib>
#include <iostream>
#include <cmath>

bool seeded;
namespace mbmore {
bool SortBids(cyclus::Bid<cyclus::Material>* i,
              cyclus::Bid<cyclus::Material>* j) {
  cyclus::Material::Ptr mat_i = i->offer();
  cyclus::Material::Ptr mat_j = j->offer();

  cyclus::toolkit::MatQuery mq_i(mat_i);
  cyclus::toolkit::MatQuery mq_j(mat_j);

  return ((mq_i.mass(922350000) / mq_i.qty()) <=
          (mq_j.mass(922350000) / mq_j.qty()));
}


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
// For various types of x_val varying curves, calculate y for some x
// Constants = [y_int, (slope or y_final), (t_change)]
  double CalcYVal(std::string function, std::vector<double> constants,
		  double x_val) {

    double curr_y;
    
    if (function == "Constant" || function == "constant"){
      if (constants.size() < 1) {
	throw "incorrect number of equation parameters";
      } else {
	curr_y = constants[0];
      }
    } else if (function == "Linear" || function == "linear"){
      if (constants.size() < 2) {
	throw "incorrect number of equation parameters";
      } else {
	curr_y = constants[0] + constants[1]*x_val;
      }
    } else if (function == "Power" || function == "power"){
      // If powerlaw has only one constant, then that is the power (A)
      // Bx^A  and B is assumed to be 1.
      double c_b = 1;
      if (constants.size() < 1) {
	throw "incorrect number of equation parameters";
      } else if (constants.size() == 2) {
	c_b = constants[1];
      } 
      curr_y = c_b*(pow( x_val, constants[0]));
    } else if (function == "Bounded_Power" || function == "bounded_power"){
      // Must be defined with all vals below
      // (Bx^A)+C, [D,E]
      // Where D is lower bound and E is upper bound. y for any x vals < D is
      // set to zero, y for any x vals > E is set to E
      if (constants.size() != 5) {
	throw "incorrect number of equation parameters";
      } else {
	double c_b = constants[1];
	double c_a = constants[0];
	double c_c = constants[2];
	double c_d = constants[3];
	double c_e = constants[4];
	if (x_val < c_d){
	  curr_y = 0;
	}
	else if (x_val > c_e) {
	  curr_y = c_c + (c_b*(pow(c_e, c_a)));
	}
	else {
	  curr_y = c_c + (c_b*(pow(x_val, c_a)));
	}
      }
    } else if (function == "Step" || function == "step"){
      if (constants.size() < 3) {
	throw "incorrect number of equation parameters";
      } else {
	if (x_val < constants[2]){
	  curr_y = constants[0];
	}
	else {
	  curr_y = constants[1];
	}
      }
    } else {
      throw "Function choices are constant, linear, step, power";
    }
  
  return curr_y;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Determines probability of an event at a single timestep given the
// likelihood integrated over n_timesteps
double ProbPerTime(double xval, double n_timesteps){
  if ((xval >= 1) || (xval < 0)){
    throw "Xval must be positive and less than 1";
  }
  return 1 - (pow((1.0 - xval), (1.0/n_timesteps)));
}

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
