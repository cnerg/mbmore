#include <gtest/gtest.h>

#include "behavior_functions.h"

#include "agent_tests.h"
#include "context.h"
#include "facility_tests.h"

namespace mbmore {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Should return True for even #s and False for odd #s
TEST(Behavior_Functions_Test, TestEveryX) {

  int interval = 2;
  int curr_time = 0;

  bool t0 = EveryXTimestep(curr_time, interval);
  curr_time++;
  bool t1 = EveryXTimestep(curr_time, interval);
  curr_time++;
  bool t2 = EveryXTimestep(curr_time, interval);

  EXPECT_TRUE(t0);
  EXPECT_FALSE(t1);
  EXPECT_EQ(t0, t2);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Should return True about 1/freq times. For freq=2, should have ~equal
// number of True and False, but on a given instance can vary by 15% or
// more. Therefore, give it 3 tries to get within a reasonable tolerance.
TEST(Behavior_Functions_Test, TestEveryRandomX) {

  int freq = 2;
  int rng_seed = -1;

  double tol = 0.05;

  bool good = false;
  int ntries = 0;
  
  while ((good == false) and (ntries < 3)){
    double n_true = 0;
    double n_false = 0;
    for (int i = 0; i < 10000; i++) {
      bool res = EveryRandomXTimestep(freq, rng_seed);
      (res == true) ? (n_true++) : (n_false++);
    }
    
    ((n_true/n_false < 1.0 + tol) && (n_true/n_false > 1.0 - tol)) ?
      good = true : ntries++;
    //    std::cout << "T: " << n_true << "F: "<< n_false << std::endl;
    //    std::cout << "ntries: " << ntries << std::endl;
  }
  EXPECT_TRUE(good);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Should return True about 1/freq times. For freq=2, should have ~equal
// number of True and False, but on a given instance can vary by 15% or
// more. Therefore, give it 3 tries to get within a reasonable tolerance.
TEST(Behavior_Functions_Test, XLikely){

  double prob = 0.5;
  int rng_seed = -1;

  double tol = 0.05;

  bool good = false;
  int ntries = 0;
  
  while ((good == false) and (ntries < 3)){
    double n_true = 0;
    double n_false = 0;
    for (int i = 0; i < 10000; i++) {
      bool res = XLikely(prob, rng_seed);
      (res == true) ? (n_true++) : (n_false++);
    }
    
    ((n_true/n_false < 1.0 + tol) && (n_true/n_false > 1.0 - tol)) ?
      good = true : ntries++;
    std::cout << "T: " << n_true << "F: "<< n_false << std::endl;
    std::cout << "ntries: " << ntries << std::endl;
  }
  EXPECT_TRUE(good);

  // Endpoints Likely =1 and Likely = 0 should be always consistent
  prob = 0.0;
  double n_true = 0;
  double n_false = 0;
  for (int i = 0; i < 1000; i++) {
    bool res = XLikely(prob, rng_seed);
    (res == true) ? (n_true++) : (n_false++);
  }

  EXPECT_EQ(n_true, 0.0);

  prob = 1.0;
  n_true = 0;
  n_false = 0;
  for (int i = 0; i < 1000; i++) {
    bool res = XLikely(prob, rng_seed);
    (res == true) ? (n_true++) : (n_false++);
  }

  EXPECT_EQ(n_false, 0.0);

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Mean and Standard deviation of a Normal Gaussian Distribution should be
// within 5% of the requested value.
TEST(Behavior_Functions_Test, TestNormalDist) {
  double mean = 10;
  double sigma = 1;
  double rng_seed = -1;
  double tol = 0.05;
  
  int array_size = 10000;
  std::vector<double> record(array_size);
  
  double sum = 0;
  for (int i = 0; i < array_size; i++) {
    record[i] = RNG_NormalDist(mean, sigma, rng_seed);
    sum += record[i];
  }

  double mu = sum / record.size();

  double accum = 0.0;
  for (int d = 0; d < record.size(); ++d) {
    accum += (record[d] - mu) * (record[d] - mu);
  };
  double stdev = std::sqrt(accum / (record.size() - 1)); 

  EXPECT_NEAR(mean/mu, 1.0, tol);
  EXPECT_NEAR(stdev/sigma, 1.0, tol);
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Each number in the range from min to max should be selected with equal
// frequency to within tolerance (5%)
TEST(Behavior_Functions_Test, TestRNGInteger) {
  int min = 1;
  int max = 3;
  int rng_seed = -1;

  double tol = 0.05;


  bool big_good = false;
  int ntries = 0;
  
  while ((big_good == false) and (ntries < 3)){
    int array_size = max - min + 1;
    std::vector<double> record(array_size, 0);
    
    for (int i = 0; i < 10000; i++) {
      int res = RNG_Integer(min, max, rng_seed);
      record[res-1]++;
    }
    std::cout << "r1 " << record[0] << "r2 " << record[1] <<
      "r3 " << record[2] <<  std::endl;

    bool good12 = ((record[0]/record[1] < 1.0 + tol) &&
		   (record[0]/record[1] > 1.0 - tol));
    bool good23 = ((record[1]/record[2] < 1.0 + tol) &&
		   (record[1]/record[2] > 1.0 - tol));
    bool good31 = ((record[2]/record[0] < 1.0 + tol) &&
		   (record[2]/record[0] > 1.0 - tol));

    (good12 && good23 && good31) ? big_good = true : ntries++;

    //    std::cout << "12: " << good12 << "   23: "<< good23 << "   31: " << good31 << std::endl;
    //    std::cout << "ntries: " << ntries << std::endl;
  }
  EXPECT_TRUE(big_good);

  }


} // namespace mbmore