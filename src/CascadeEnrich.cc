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
     << " with enrichment facility parameters:"
     << " * SWU capacity: " << SwuCapacity()
     << " * Tails assay: " << tails_assay << " * Feed assay: " << FeedAssay()
     << " * Input cyclus::Commodity: " << feed_commod
     << " * Output cyclus::Commodity: " << product_commod
     << " * Tails cyclus::Commodity: " << tails_commod;
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

 current_swu_capacity = SwuCapacity();
 
 }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CascadeEnrich::Tock() {
  using cyclus::toolkit::RecordTimeSeries;

  LOG(cyclus::LEV_INFO4, "EnrFac") << prototype() << " used "
                                   << intra_timestep_swu_ << " SWU";
  RecordTimeSeries<cyclus::toolkit::ENRICH_SWU>(this, intra_timestep_swu_);
  LOG(cyclus::LEV_INFO4, "EnrFac") << prototype() << " used "
                                   << intra_timestep_feed_ << " feed";
  RecordTimeSeries<cyclus::toolkit::ENRICH_FEED>(this, intra_timestep_feed_);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr>
CascadeEnrich::GetMatlRequests() {
  using cyclus::Material;
  using cyclus::RequestPortfolio;
  using cyclus::Request;

  std::set<RequestPortfolio<Material>::Ptr> ports;
  RequestPortfolio<Material>::Ptr port(new RequestPortfolio<Material>());
  Material::Ptr mat = Request_();
  double amt = mat->quantity();

  if (amt > cyclus::eps_rsrc()) {
    port->AddRequest(mat, this, feed_commod);
    ports.insert(port);
  }

  return ports;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Sort offers of input material to have higher preference for more
//  U-235 content
void CascadeEnrich::AdjustMatlPrefs(
    cyclus::PrefMap<cyclus::Material>::type& prefs) {
  using cyclus::Bid;
  using cyclus::Material;
  using cyclus::Request;

  if (order_prefs == false) {
    return;
  }

  cyclus::PrefMap<cyclus::Material>::type::iterator reqit;

  // Loop over all requests
  for (reqit = prefs.begin(); reqit != prefs.end(); ++reqit) {
    std::vector<Bid<Material>*> bids_vector;
    std::map<Bid<Material>*, double>::iterator mit;
    for (mit = reqit->second.begin(); mit != reqit->second.end(); ++mit) {
      Bid<Material>* bid = mit->first;
      bids_vector.push_back(bid);
    }
    //    std::sort(bids_vector.begin(), bids_vector.end(), SortBids);

    // Assign preferences to the sorted vector
    double n_bids = bids_vector.size();
    bool u235_mass = 0;

    for (int bidit = 0; bidit < bids_vector.size(); bidit++) {
      int new_pref = bidit + 1;

      // For any bids with U-235 qty=0, set pref to zero.
      if (!u235_mass) {
        cyclus::Material::Ptr mat = bids_vector[bidit]->offer();
        cyclus::toolkit::MatQuery mq(mat);
        if (mq.mass(922350000) == 0) {
          new_pref = -1;
        } else {
          u235_mass = true;
        }
      }
      (reqit->second)[bids_vector[bidit]] = new_pref;
    }  // each bid
  }    // each Material Request
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::Material::Ptr CascadeEnrich::Request_() {
  double qty = std::max(0.0, inventory.capacity() - inventory.quantity());
  return cyclus::Material::CreateUntracked(qty,
                                           context()->GetRecipe(feed_recipe));
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool SortBids(cyclus::Bid<cyclus::Material>* i,
              cyclus::Bid<cyclus::Material>* j) {
  cyclus::Material::Ptr mat_i = i->offer();
  cyclus::Material::Ptr mat_j = j->offer();

  cyclus::toolkit::MatQuery mq_i(mat_i);
  cyclus::toolkit::MatQuery mq_j(mat_j);

  return ((mq_i.mass(922350000) / mq_i.qty()) <=
          (mq_j.mass(922350000) / mq_j.qty()));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double CascadeEnrich::FeedAssay() {
  using cyclus::Material;

  if (inventory.empty()) {
    return 0;
  }
  double pop_qty = inventory.quantity();
  cyclus::Material::Ptr fission_matl =
      inventory.Pop(pop_qty, cyclus::eps_rsrc());
  inventory.Push(fission_matl);
  return cyclus::toolkit::UraniumAssay(fission_matl);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
extern "C" cyclus::Agent* ConstructCascadeEnrich(cyclus::Context* ctx) {
  return new CascadeEnrich(ctx);
}
  
}  // namespace mbmore
