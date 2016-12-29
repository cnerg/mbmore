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

 private:
  #pragma cyclus var { \
    "tooltip": "feed recipe",						\
    "doc": "recipe for enrichment facility feed commodity",		\
    "uilabel": "Feed Recipe",                                   \
    "uitype": "recipe" \
  }
  std::string feed_recipe;
  
   friend class CascadeEnrichTest;
  // ---
};
 
}  // namespace mbmore

#endif // MBMORE_SRC_ENRICHMENT_FACILITY_H_
