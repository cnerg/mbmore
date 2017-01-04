// Implements the CascadeEnrich class
#include "CascadeEnrich.h"
#include "behavior_functions.h"
#include "enrich_functions.h"
#include "sim_init.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <vector>
#include <boost/lexical_cast.hpp>


namespace mbmore {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CascadeEnrich::CascadeEnrich(cyclus::Context* ctx)
    : cyclus::Facility(ctx),
  feed_recipe(""),
  initial_feed(0){}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CascadeEnrich::~CascadeEnrich() {}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string CascadeEnrich::str() {
  std::stringstream ss;
  ss << cyclus::Facility::str()
     << " with enrichment facility parameters:";
  return ss.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CascadeEnrich::Build(cyclus::Agent* parent) {
  using cyclus::Material;
  
  Facility::Build(parent);
  if (initial_feed > 0) {
    inventory.Push(
      Material::Create(
        this, initial_feed, context()->GetRecipe(feed_recipe)));
  }

  LOG(cyclus::LEV_DEBUG2, "EnrFac") << "CascadeEnrich "
				    << " entering the simuluation: ";
  LOG(cyclus::LEV_DEBUG2, "EnrFac") << str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CascadeEnrich::Tick() {
  /*
  double delU = CalcDelU(v_a, height, diameter, feed, temp, cut, eff,
			 M, dM, x, flow_internal);

  std::cout << "DelU is " << delU << std::endl;
  */
 }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CascadeEnrich::Tock() {

}






// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
extern "C" cyclus::Agent* ConstructCascadeEnrich(cyclus::Context* ctx) {
  return new CascadeEnrich(ctx);
}
  
}  // namespace mbmore
