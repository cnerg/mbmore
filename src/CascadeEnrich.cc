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
  double secpermon = 60*60*24*(365.25/12);


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CascadeEnrich::CascadeEnrich(cyclus::Context* ctx)
    : cyclus::Facility(ctx),
  feed_recipe(""),
  max_centrifuges(),
  design_feed_assay(),
  design_product_assay(),
  design_waste_assay(),
  design_feed(0){}
  
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
  std::pair<double, double> n_stages =
    FindNStages(design_alpha, design_feed_assay, design_product_assay,
		design_waste_assay);

  // set as internal state variables
  // int truncates but we need # of stages to assure target values,
  // so if the number is 5.1 we need 6. 
  n_enrich_stages = int(n_stages.first) + 1;
  n_strip_stages = int(n_stages.second) + 1;


  std::pair<int,double> cascade_info = DesignCascade(design_feed,
						     design_alpha,
						     design_delU,
						     cut, n_stages);

  max_feed_inventory = cascade_info.second;
  // Number of machines times swu per machine
  //TOD CONVERT SWU TO PER MONTH INSTEAD OF KG/SEC
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
  std::pair<int,double> CascadeEnrich::DesignCascade(double design_feed,
						     double design_alpha,
						     double design_delU,
						     double cut,
						     double n_stages){

  // Determine the ideal steady-state feed flows for this cascade design given
  // the maximum potential design feed rate
  std::vector<double> feed_flows = CalcFeedFlows(n_stages, design_feed, cut);

  std::vector<std::pair<int, double>> stage_info =
    CalcStageFeatures(design_feed, design_alpha, design_delU, cut,
		      n_stages, feed_flows);
  
  // Do design parameters require more centrifuges than what is available?
  int machines_needed = FindCascadeMachines(stage_info);

  if (machines_needed > max_centrifuges){
    double curr_feed = design_feed;
    double step_size = 0.95;
    int max_tries = 1000;
    bool optimum_number = false;
    int ntries = 0;
    double optimal_feed;
    
    while ((optimum_number == false) && (ntries < max_tries)){
      ntries += 1;
      double last_feed = curr_feed;
      curr_feed *=step;
      feed_flows = CalcFeedFlows(n_stages, curr_feed, cut);
      stage_info = CalcStageFeatures(curr_feed, design_alpha, design_delU,
				     cut, n_stages, flow_flows);
      machines_needed = FindCascadeMachines(stage_info);
      std::pair<int, double> last_stage = stage_info.back();
      
      // If cannot converge on a cascade with allowable number of centrifuges
      if (ntries >= max_tries){
	throw cyclus::ValueError(
				 "Could not design a cascade using the max allowed machines");
      }
      // If the last stage of the cascade has zero centrifuges then there are
      // not enough to achieve the target enrichment
      else if (last_stage.first < 1){
	throw cyclus::ValueError(
				 "Not enough available centrifuges to achieve target enrichment level");
      }
      // If optimal design is finally found
      else if (machines_needed <= max_centrifuges){
	optimal_feed = last_feed;
	optimal_number = true;
      }
    }
    feed_flows = CalcFeedFlows(n_stages, optimal_feed, cut);
    stage_info = CalcStageFeatures(optimal_feed, design_alpha, design_delU, cut,
				   n_stages, feed_flows);
    machines_needed = FindCascadeMachines(stage_info);
    
  }
  std::pair<int, double> cascade_info = std::make_pair(machines_needed,
						       optimal_feed);
  return cascade_info;
  }


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
extern "C" cyclus::Agent* ConstructCascadeEnrich(cyclus::Context* ctx) {
  return new CascadeEnrich(ctx);
}
  
}  // namespace mbmore
