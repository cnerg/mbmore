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
  //  desired_swu(),  // not needed at this time
  design_feed_assay(),
  design_product_assay(),
  design_waste_assay(),
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

  // Determine the steady-state feed flows for this cascade design
  // returns an array pointer
  //  std::vector<double> feed_flows = CalcFeedFlows(n_stages, initial_feed, cut);
  
  
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
  // THESE HAVE BEEN REPLACED BY FindNStages
  /*
  int CascadeEnrich::NEnrichStages(double alpha, double delU, double feed,
				     double feed_assay){

   double stage_feed_assay = feed_assay;
   double stage_feed = feed;
   bool integer_stages = true;
   int n_stage_enr = 0;
   double product_assay, stage_product;
   
   while (integer_stages == true){
     double n_mach = MachinesPerStage(alpha, delU, stage_feed);
     if (((int) n_mach) <= 1){ integer_stages = false; }
     product_assay = ProductAssayByAlpha(alpha, stage_feed_assay);
     stage_product = ProductPerEnrStage(alpha, stage_feed_assay, product_assay,
					stage_feed);
     stage_feed_assay = product_assay;
     stage_feed = stage_product; 
     n_stage_enr += 1; 
     if (n_stage_enr == 15){ integer_stages = false;} // just a catch until I  make sure while loop is working

   }

   return n_stage_enr;

 }
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TODO: FIX THIS FN BY REPLACEING WASTEPERSTRIPSTAGE
  int CascadeEnrich::NStripStages(double alpha, double delU, double enr_feed,
				     double feed_assay){

   bool integer_stages = true;
   int n_stage_strip = 0;
   double waste_assay, stage_waste;

   // the waste from first enrichment stage is the feed for the stripping stages
   double enr_product_assay = ProductAssayByAlpha(alpha, feed_assay);
   double enr_product = ProductPerEnrStage(alpha, feed_assay, enr_product_assay,
					   enr_feed);

   double enr_waste = enr_feed - enr_product;
   double enr_waste_assay = (enr_feed*feed_assay -
			     enr_product*enr_product_assay) / enr_waste;
   
   double stage_feed_assay = enr_waste_assay;
   double stage_feed = enr_waste;
   
   while (integer_stages == true){
     double n_mach = MachinesPerStage(alpha, delU, stage_feed);
     if (((int) n_mach) <= 1){ integer_stages = false; }
     waste_assay = WasteAssayByAlpha(alpha, stage_feed_assay);
     //     stage_waste = WastePerStripStage(alpha, stage_feed_assay, waste_assay,
     //					stage_feed);
     stage_feed_assay = waste_assay;
     stage_feed = stage_waste; 
     n_stage_strip += 1;
     
     if (n_stage_strip == 15){ integer_stages = false;} // just a cath until I  make sure while loop is working

   }

   return n_stage_strip;
   

 }
*/
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
extern "C" cyclus::Agent* ConstructCascadeEnrich(cyclus::Context* ctx) {
  return new CascadeEnrich(ctx);
}
  
}  // namespace mbmore
