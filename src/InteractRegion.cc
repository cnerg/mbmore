// Implements the Region class
#include "InteractRegion.h"
#include "behavior_functions.h"

#include <iostream>
#include <string>


namespace mbmore {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InteractRegion::InteractRegion(cyclus::Context* ctx)
  : cyclus::Region(ctx) {
    //  kind_ = "InteractRegion";
  cyclus::Warn<cyclus::EXPERIMENTAL_WARNING>("the InteractRegion agent is experimental.");

}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  InteractRegion::~InteractRegion() {}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void InteractRegion::Build(cyclus::Agent* parent) {
    cyclus::Agent::Build(parent);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::map<std::string, double>
  InteractRegion::GetWeights(std::string eqn_type) {
    //TODO: use eqn_type ot expand in offering PE or AQ results
    return p_wts;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Returns a map of regularly used factors and bool to indicate whether they are
// defined in this sim.
std::map<std::string, bool>
  InteractRegion::GetFactors(std::string eqn_type) {

  std::map<std::string, bool> present;
  std::map<std::string,double>::iterator factor_it;

  //  std::string master_factors [] = { "Dem", "React"};
  std::string master_factors [] = {				   
    "Enrich", "Auth", "Conflict", "Mil_Sp", "Reactors", "Mil_Iso", 
    "Sci_Net", "U_Reserve"};
  int n_factors = sizeof(master_factors) / sizeof(master_factors[0]);

  for(int i = 0; i < n_factors; i++) {
    factor_it = p_wts.find(master_factors[i]);
    if (factor_it == p_wts.end()) {   // factor isn't defined in input file
      present[master_factors[i]]= false;
    }
    else {
      present[master_factors[i]]= true;
    }
  }
  
  return present;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Determine the likelihood value for the equation at the current time,
// (where the current value of the equation is normalized to be between 0-1)
double InteractRegion::GetLikely(std::string phase, double eqn_val) {

  std::pair<std::string, std::vector<double> > likely_pair = likely[phase];
  std::string function = likely_pair.first;
  std::vector<double> constants = likely_pair.second;
  
  /*
  std::map<std::string, double> curr_likely;
  
  std::map<std::string,
	   std::pair<std::string, std::vector<double> > >::iterator eqn_it;

  for(eqn_it = likely.begin(); eqn_it != likely.end(); eqn_it++) {
    std::string phase = eqn_it->first;
    std::string function = eqn_it->second.first;
    std::vector<double> constants = eqn_it->second.second;
  */

    double phase_likely = CalcYVal(function, constants, eqn_val);
    std::cout << "phase " << phase << " fn " << function << "likely " << phase_likely << std::endl;
    return phase_likely;

    //    curr_likely[phase] = phase_likely;
    //  }
    //  return curr_likely;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string InteractRegion::str() {
  std::string s = cyclus::Agent::str();

  s += " has insts: ";
  for (std::set<Agent*>::const_iterator inst = children().begin();
       inst != children().end();
       inst++) {
    s += (*inst)->prototype() + ", ";
  }
  return s;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
extern "C" cyclus::Agent* ConstructInteractRegion(cyclus::Context* ctx) {
  return new InteractRegion(ctx);
}
  
}  // namespace mbmore
