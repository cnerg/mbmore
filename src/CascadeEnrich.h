#ifndef MBMORE_SRC_CASCADE_ENRICH_H_
#define MBMORE_SRC_CASCADE_ENRICH_H_

#include <string>

#include "cyclus.h"
#include "sim_init.h"

namespace mbmore {

class CascadeEnrich : public cyclus::Facility {
#pragma cyclus note {   	  \
  "niche": "enrichment facility",				  \
  "doc":								\
  "The CascadeEnrich facility based on the Cycamore Enrich facility. " \
 "timesteps (see README for full implementation documentation ",\
}
  
 public:
  // --- Module Members ---
  ///    Constructor for the CascadeEnrich class
  ///    @param ctx the cyclus context for access to simulation-wide parameters
  CascadeEnrich(cyclus::Context* ctx);

  ///     Destructor for the CascadeEnrich class
  virtual ~CascadeEnrich();

  #pragma cyclus

  ///     Print information about this agent
  virtual std::string str();
  // ---

  // --- Facility Members ---
  /// perform module-specific tasks when entering the simulation
  virtual void Build(cyclus::Agent* parent);
  // ---

  // --- Agent Members ---
  ///  Each facility is prompted to do its beginning-of-time-step
  ///  stuff at the tick of the timer.

  ///  @param time is the time to perform the tick
  virtual void Tick();

  ///  Each facility is prompted to its end-of-time-step
  ///  stuff on the tock of the timer.

  ///  @param time is the time to perform the tock
  virtual void Tock();

  /*
  // TODO: Turn into optional state variables
  const double v_a = 485; // m/s
  const double height = 0.5; // meters
  const double diameter = 0.15; // meters
  const double feed = 15*60*60/((1e3)*60*60*1000.0) ; // g/hr to kg/sec
  const double temp = 320.0 ; //Kelvin

  // Not physical constants but fixed assumptions for a cascade separating
  // out U235 from U238 in UF6 gas
  const double M = 0.352; // kg/mol UF6
  const double dM = 0.003; // kg/mol U238 - U235
  const double x = 1000;  // Pressure ratio (Glaser)

  const double flow_internal = 2.0 ;  // can vary from 2-4
  const double eff = 1.0;  // typical efficiencies <0.6
  const double cut = 0.5;  // target for ideal cascade
  */  


 private:
  #pragma cyclus var { \
    "tooltip": "feed recipe",						\
    "doc": "recipe for enrichment facility feed commodity",		\
    "uilabel": "Feed Recipe",                                   \
    "uitype": "recipe" \
  }
  std::string feed_recipe;

  #pragma cyclus var {							\
    "default": 0, "tooltip": "initial uranium reserves (kg)",		\
    "uilabel": "Initial Feed Inventory",				\
    "doc": "amount of natural uranium stored at the enrichment "	\
    "facility at the beginning of the simulation (kg)"			\
  }
  double initial_feed;

  #pragma cyclus var {							\
    "default": 1e299, "tooltip": "max inventory of feed material (kg)", \
    "uilabel": "Maximum Feed Inventory",                                \
    "doc": "maximum total inventory of natural uranium in "		\
           "the enrichment facility (kg)"     \
  }
  double max_feed_inventory;

  
  #pragma cyclus var { 'capacity': 'max_feed_inventory' }
  cyclus::toolkit::ResBuf<cyclus::Material> inventory;  // natural u
  
  friend class CascadeEnrichTest;
  // ---
};
 
}  // namespace mbmore

#endif // MBMORE_SRC_CASCADE_ENRICH_H_
