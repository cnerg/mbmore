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

  #pragma cyclus note {"doc": "A super-region that defines relationships " \
                              "between StateInst agents and acts as a Context "\
                              "to track information needed by multiple " \
                              "prototypes."}

  virtual void Tick();

  virtual void Tock() {}

  // perform actions required when entering the simulation
  virtual void Build(cyclus::Agent* parent);

  /// enter the simulation and register any children present
  //  virtual void EnterNotify();

  // shares the pursuit and acquisition equation weighting information
  // with the child institutions
  std::map<std::string, double> GetWeights(std::string eqn_type);

  // Determines # of states in the simulation. If only one state then
  // Interactive Factors (such as conflict) are not calculated.
  int GetNStates();
  
  // Uses the pursuit or acquire likelihood conversion equation to determine the
  // likeliness of pursuit and acquire on a 0-1 scale for the requested timestep
  double GetLikely(std::string phase, double eqn_val);


  // Determines which factors are defined for this sim
  std::map<std::string, bool> DefinedFactors(std::string eqn_type);

  // Returns a map of regularly used factors and bool to indicate whether
  // the are defined in this sim.
  std::map<std::string, bool> GetDefinedFactors(std::string eqn_type);

  // Returns the main list of all factors to be recorded in database
  std::vector<std::string>& GetMainFactors();

  // Tracks weapons status of each state (0 = not pursuing, 2 = pursuing,
  // 3 = acquired) by updating the sim_weapon_status map
  virtual void UpdateWeaponStatus(std::string proto, int new_weapon_status);

  // Determines Conflict score for each state based on its net
  // relationships with other states and both states' weapon status
  double GetConflictScore(std::string eqn_type, std::string prototype);

  // Builds a string to use as key for map that defines the conflict score
  // for a pair states based on their ally/neutral/enemy relationship
  std::string BuildRelationString(int statusA, int statusB, int relation);

  // Changes conflict relationship from initial value to final value at the
  // specified time
  virtual void ChangeConflictReln(std::string eqn_type,
				    std::string this_state,
				    std::string other_state, int new_val);

  // Records conflict value at beginning of simulation and any time the conflict
  // relation changes
  virtual void RecordConflictReln(std::string eqn_type,
				    std::string this_state,
				    std::string other_state, int new_val);


  // Initialize the map that defines how state relationships map to conflict
  // score
  virtual void BuildScoreMatrix();

  /// every agent should be able to print a verbose description
  virtual std::string str();

 private:

#pragma cyclus var {				\
  "default": 0,						    \
  "tooltip": "Are Conflict and Isolation relationships symmetric?" ,    \
  "doc": "If True then State A and State B have mutually agreed upon " \
         "relationships (both allies or enemies). If False then State A " \
         "and State B can have different perceptions of their relationship", \
  }
bool symmetric;

 #pragma cyclus var {							\
    "alias": ["pursuit_weights", "factor", "weight"],			\
    "doc": "Weighting for all Pursuit Factors defined in StateInst "	\
           " Total weight should add to 1 (otherwise it will be normalized)", \
    }
  std::map<std::string, double> wts ;

  #pragma cyclus var {      \
    "alias": ["likely_converter", "phase", ["function","name", ["params","val"]]],\
    "doc": "Relational Equation to convert from Pursuit or acquire score to a "\
           " Likelihood of occuring."				       \
           "Beginning with pursuit score between 0 to 10, equation converts to"\
           "a likelihood between 0 to 1. Function option is (Power,A) in the "\
           "form (x over 10) to the A power"			       \
           "The required Phases are Pursuit and Acquire",	         \
    }
  std::map<std::string, std::pair<std::string, std::vector<double> > >
    likely_rescale ;

#pragma cyclus var {							\
    "alias": ["p_conflict_relations", ["states", "primary_state","pair_state"],"relation"], \
    "doc": "Conflict relationships between states at t=0 for Pursuit"\
           "Each state is entered as the Primary state, with it's perceived " \
           "relations to each other state as the map values." \
           "Choices are +1 (allies), 0 (neutral), -1 (enemies)",	 \
    }
  std::map<std::pair<std::string,std::string>, int> p_conflict_map ;


// Defines persistent column names in WeaponProgress table of database
// Must be defined globally so that references to the column name 
// pointers persist
static std::vector<std::string> column_names;

// Defines which of the main factor list are being used based on weights
std::map<std::string, bool> p_present;
std::map<std::string, bool> a_present;

// Tracks the weapons status of each state
std::map<std::string, int> sim_weapon_status;

// Defines conflict scores given weapon status of 2 states and their
// relationship (ally, neut, enemy)
std::map<std::string, int> score_matrix;

  
 
}; //cyclus::Region

}  // namespace mbmore
#endif  // MBMORE_SRC_INTERACT_REGION_H_
