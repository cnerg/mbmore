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
  double secpermonth = 60*60*24*(365.25/12);


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CascadeEnrich::CascadeEnrich(cyclus::Context* ctx)
    : cyclus::Facility(ctx),
  feed_recipe(""),
  max_centrifuges(),
  design_feed_assay(),
  design_product_assay(),
  design_waste_assay(),
  //  design_feed(0),
  tails_assay(0), //BEGIN LEGACY VARS
  swu_capacity(0),  // REMOVE
  max_enrich(1),
  initial_feed(0),   // CHECK AGAINST DESIGN FEED
  feed_commod(""),
  product_commod(""),
  tails_commod(""),
  order_prefs(true) {}

  
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

  // Calculate ideal machine performance
  double design_delU = CalcDelU(v_a, height, diameter, machine_feed, temp,
				cut, eff, M, dM, x, flow_internal);
  double design_alpha = AlphaBySwu(design_delU, machine_feed, cut, M);

  // Design ideal cascade based on target feed flow and product assay
  std::pair<int, int> n_stages =
    FindNStages(design_alpha, design_feed_assay, design_product_assay,
		design_waste_assay);

  // TODO DELETE THIS, STAGES ARE ALREADY INTS
  // set as internal state variables
  // int truncates but we need # of stages to assure target values,
  // so if the number is 5.1 we need 6. 
  //  n_enrich_stages = int(n_stages.first) + 1;
  //  n_strip_stages = int(n_stages.second) + 1;
  n_enrich_stages = n_stages.first;
  n_strip_stages = n_stages.second;


  std::pair<int,double> cascade_info = DesignCascade(initial_feed,
						     design_alpha,
						     design_delU,
						     cut, max_centrifuges,
						     n_stages);

  max_feed_inventory = cascade_info.second;
  // Number of machines times swu per machine
  //TODO: CONVERT SWU TO PER MONTH INSTEAD OF KG/SEC
  swu_capacity = cascade_info.first * design_delU * secpermonth;

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


 }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CascadeEnrich::Tock() {

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
extern "C" cyclus::Agent* ConstructCascadeEnrich(cyclus::Context* ctx) {
  return new CascadeEnrich(ctx);
}
  
}  // namespace mbmore
