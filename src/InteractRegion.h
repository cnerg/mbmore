#ifndef MBMORE_SRC_INTERACT_REGION_H_
#define MBMORE_SRC_INTERACT_REGION_H_

#include "cyclus.h"

namespace mbmore {

/// @class Region
///
/// The Region class is the abstract class/interface used by all
/// region agents
///
/// This is all that is known externally about Regions

class InteractRegion
  : public cyclus::Region {
  friend class InteractRegionTests;
 public:
  /// Default constructor for InteractRegion Class
  InteractRegion(cyclus::Context* ctx);

  /// InteractRegions should not be indestructible.
  virtual ~InteractRegion();

  #pragma cyclus

  #pragma cyclus note {"doc": "A region that governs a scenario in which " \
                              "there is growth in demand for a commodity. "}
  
  virtual void Tick() {}

  virtual void Tock() {}

  // perform actions required when entering the simulation
  virtual void Build(cyclus::Agent* parent);

  // shares the pursuit and acquisition equation weighting information
  // with the child institutions
  std::map<std::string, double> GetWeights(std::string eqn_type);

  // Uses the pursuit or acquire likelihood conversion equation to determine the
  // likeliness of pursuit and acquire on a 0-1 scale for the requested timestep
  std::map<std::string, double> GetLikely(double eqn_val);
  
  /// every agent should be able to print a verbose description
  virtual std::string str();

 private:
  #pragma cyclus var {							\
    "alias": ["pursuit_weights", "factor", "weight"],			\
    "doc": "Weighting for Pursuit Factors "				\
    " Total weight should add to 1 (otherwise it will be normalized)",	\
    }
  std::map<std::string, double> p_wts ;

  #pragma cyclus var {      \
  "alias": ["Likely", "phase", ["function","name", ["params","val"]]],	\
    "doc": "Relational Equation to convert from Pursuit or acquire score to a " \
    " Likelihood of occuring"						\
    "Beginning with pursuit score between 0 to 10, equation converts to a " \
    "likelihood between 0 to 1. Function option is (Power,A) in the "	\
    "form (x over 10) to the A power"					\
    "The required Phases are Pursuit and Acquire",			\
    }
 std::map<std::string, std::pair<std::string, std::vector<double> > > likely ;
 

  
}; //cyclus::Region

}  // namespace mbmore
#endif  // MBMORE_SRC_INTERACT_REGION_H_
