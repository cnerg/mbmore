// Implements the CascadeEnrich class
#include "CascadeEnrich.h"
#include "behavior_functions.h"
#include "enrich_functions.h"
#include "sim_init.h"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <limits>
#include <sstream>
#include <vector>

namespace mbmore {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CascadeEnrich::CascadeEnrich(cyclus::Context* ctx)
    : cyclus::Facility(ctx),
      feed_recipe(""),
      max_centrifuges(),
      design_feed_assay(),
      design_product_assay(),
      design_tails_assay(),
      centrifuge_velocity(485.0),
      temp(320.0),
      height(0.5),
      diameter(0.15),
      machine_feed(15),
      max_enrich(1),
      design_feed_flow(100),
      feed_commod(""),
      product_commod(""),
      tails_commod(""),
      order_prefs(true) {}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CascadeEnrich::~CascadeEnrich() {}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string CascadeEnrich::str() {
  std::stringstream ss;
  ss << cyclus::Facility::str() << " with enrichment facility parameters:"
     << " * Tails assay: " << tails_assay << " * Feed assay: " << FeedAssay()
     << " * Input cyclus::Commodity: " << feed_commod
     << " * Output cyclus::Commodity: " << product_commod
     << " * Tails cyclus::Commodity: " << tails_commod;
  return ss.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CascadeEnrich::EnterNotify() {
  using cyclus::Material;
  cyclus::Facility::EnterNotify();

  // Update Centrifuge paramter from the user input:
  centrifuge.v_a = centrifuge_velocity;
  centrifuge.height = height;
  centrifuge.diameter = diameter;
  centrifuge.feed = machine_feed / 1000 / 1000;
  centrifuge.temp = temp;

  cascade = FindNumberIdealStages(design_feed_assay, design_product_assay,
                                  design_tails_assay, centrifuge, precision);
  cascade =
      DesignCascade(cascade, FlowPerSec(design_feed_flow), max_centrifuges);
  max_feed_flow = FlowPerMon(cascade.feed_flow);
  std::cout << "Delta U " << cascade.stgs_config[0].DU << std::endl;
  std::cout << "max_feed_flow " << max_feed_flow << std::endl;
  std::cout << "n_centrifuges " << FindTotalMachines(cascade)  << std::endl;
  std::cout << "n_strip" << cascade.stripping_stgs  << std::endl;
  std::cout << "n_enrich" << cascade.enrich_stgs << std::endl;
  std::cout << std::endl << std::endl << " cascade geom " << std::endl;
  std::map<int, stg_config>::iterator it;
  for( it = cascade.stgs_config.begin(); it != cascade.stgs_config.end(); it++){
    std::cout << "stg: " <<it->first << " machines: " << it->second.n_machines;
    std::cout << " Prod flow " << FlowPerMon(it->second.flow)*it->second.cut <<std::endl;
  }
  
  if (max_feed_inventory > 0) {
    inventory.capacity(max_feed_inventory);
  }
  if (initial_feed > 0) {
    inventory.Push(Material::Create(this, initial_feed,
                                    context()->GetRecipe(feed_recipe)));
  }
  LOG(cyclus::LEV_DEBUG2, "EnrFac") << "CascadeEnrich "
                                    << " entering the simuluation: ";
  LOG(cyclus::LEV_DEBUG2, "EnrFac") << str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CascadeEnrich::Tick() {}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CascadeEnrich::Tock() {
  using cyclus::toolkit::RecordTimeSeries;

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

    std::sort(bids_vector.begin(), bids_vector.end(), SortBids);

    // Assign preferences to the sorted vector
    double n_bids = bids_vector.size();
    bool u235_mass = 0;

    for (int bidit = 0; bidit < bids_vector.size(); bidit++) {
      int new_pref = bidit + 10;

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
void CascadeEnrich::AcceptMatlTrades(
    const std::vector<std::pair<cyclus::Trade<cyclus::Material>,
                                cyclus::Material::Ptr>>& responses) {
  // see
  // http://stackoverflow.com/questions/5181183/boostshared-ptr-and-inheritance
  std::vector<std::pair<cyclus::Trade<cyclus::Material>,
                        cyclus::Material::Ptr>>::const_iterator it;
  for (it = responses.begin(); it != responses.end(); ++it) {
    AddMat_(it->second);
  }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CascadeEnrich::AddMat_(cyclus::Material::Ptr mat) {
  // Elements and isotopes other than U-235, U-238 are sent directly to tails
  cyclus::CompMap cm = mat->comp()->atom();
  bool extra_u = false;
  bool other_elem = false;
  for (cyclus::CompMap::const_iterator it = cm.begin(); it != cm.end(); ++it) {
    if (pyne::nucname::znum(it->first) == 92) {
      if (pyne::nucname::anum(it->first) != 235 &&
          pyne::nucname::anum(it->first) != 238 && it->second > 0) {
        extra_u = true;
      }
    } else if (it->second > 0) {
      other_elem = true;
    }
  }
  if (extra_u) {
    cyclus::Warn<cyclus::VALUE_WARNING>(
        "More than 2 isotopes of U.  "
        "Istopes other than U-235, U-238 are sent directly to tails.");
  }
  if (other_elem) {
    cyclus::Warn<cyclus::VALUE_WARNING>(
        "Non-uranium elements are "
        "sent directly to tails.");
  }

  LOG(cyclus::LEV_INFO5, "EnrFac") << prototype() << " is initially holding "
                                   << inventory.quantity() << " total.";

  try {
    inventory.Push(mat);
  } catch (cyclus::Error& e) {
    e.msg(Agent::InformErrorMsg(e.msg()));
    throw e;
  }

  LOG(cyclus::LEV_INFO5, "EnrFac")
      << prototype() << " added " << mat->quantity() << " of " << feed_commod
      << " to its inventory, which is holding " << inventory.quantity()
      << " total.";
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::set<cyclus::BidPortfolio<cyclus::Material>::Ptr>
CascadeEnrich::GetMatlBids(
    cyclus::CommodMap<cyclus::Material>::type& out_requests) {
  using cyclus::Bid;
  using cyclus::BidPortfolio;
  using cyclus::CapacityConstraint;
  using cyclus::Converter;
  using cyclus::Material;
  using cyclus::Request;
  using cyclus::toolkit::MatVec;

  std::set<BidPortfolio<Material>::Ptr> ports;

  if ((out_requests.count(tails_commod) > 0) && (tails.quantity() > 0)) {
    BidPortfolio<Material>::Ptr tails_port(new BidPortfolio<Material>());

    std::vector<Request<Material>*>& tails_requests =
        out_requests[tails_commod];
    std::vector<Request<Material>*>::iterator it;
    for (it = tails_requests.begin(); it != tails_requests.end(); ++it) {
      // offer bids for all tails material, keeping discrete quantities
      // to preserve possible variation in composition
      MatVec mats = tails.PopN(tails.count());
      tails.Push(mats);
      for (int k = 0; k < mats.size(); k++) {
        Material::Ptr m = mats[k];
        Request<Material>* req = *it;
        tails_port->AddBid(req, m, this);
      }
    }
    // overbidding (bidding on every offer)
    // add an overall capacity constraint
    CapacityConstraint<Material> tails_constraint(tails.quantity());
    tails_port->AddConstraint(tails_constraint);
    LOG(cyclus::LEV_INFO5, "EnrFac") << prototype()
                                     << " adding tails capacity constraint of "
                                     << tails.capacity();
    ports.insert(tails_port);
  }

  if ((out_requests.count(product_commod) > 0) && (inventory.quantity() > 0)) {
    BidPortfolio<Material>::Ptr commod_port(new BidPortfolio<Material>());

    std::vector<Request<Material>*>& commod_requests =
        out_requests[product_commod];
    std::vector<Request<Material>*>::iterator it;
    for (it = commod_requests.begin(); it != commod_requests.end(); ++it) {
      Request<Material>* req = *it;
      Material::Ptr offer = Offer_(req->target());
      // The offer might not match the required enrichment ! it just produce
      // what it can according to the cascade configuration and the feed asays
      commod_port->AddBid(req, offer, this);
    }

    // overbidding (bidding on every offer)
    // add an overall production capacity constraint

    // correct the actual inventory quantity by the amount of Uranium in it...
    double feed_qty = inventory.quantity();
    Material::Ptr natu_matl = inventory.Pop(feed_qty, cyclus::eps_rsrc());
    inventory.Push(natu_matl);
    cyclus::toolkit::MatQuery mq(natu_matl);
    std::set<cyclus::Nuc> nucs;
    nucs.insert(922350000);
    nucs.insert(922380000);
    double u_frac = mq.mass_frac(nucs);
    double cor_feed_qty = feed_qty * u_frac;
    double production_capacity =
        ProductFlow(std::min(cor_feed_qty, max_feed_flow));
    cyclus::CapacityConstraint<Material> production_contraint(
        production_capacity);
    commod_port->AddConstraint(production_contraint);
    LOG(cyclus::LEV_INFO5, "EnrFac")
        << prototype() << " adding production capacity constraint of "
        << production_capacity;

    ports.insert(commod_port);
  }
  return ports;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CascadeEnrich::GetMatlTrades(
    const std::vector<cyclus::Trade<cyclus::Material>>& trades,
    std::vector<std::pair<cyclus::Trade<cyclus::Material>,
                          cyclus::Material::Ptr>>& responses) {
  using cyclus::Material;
  using cyclus::Trade;

  intra_timestep_feed_ = 0;
  std::vector<Trade<Material>>::const_iterator it;
  for (it = trades.begin(); it != trades.end(); ++it) {
    double qty = it->amt;
    std::string commod_type = it->bid->request()->commodity();
    Material::Ptr response;
    // Figure out whether material is tails or enriched,
    // if tails then make transfer of material
    if (commod_type == tails_commod) {
      LOG(cyclus::LEV_INFO5, "EnrFac")
          << prototype() << " just received an order"
          << " for " << it->amt << " of " << tails_commod;
      double pop_qty = std::min(qty, tails.quantity());
      response = tails.Pop(pop_qty, cyclus::eps_rsrc());
    } else {
      LOG(cyclus::LEV_INFO5, "EnrFac")
          << prototype() << " just received an order"
          << " for " << it->amt << " of " << product_commod;
      response = Enrich_(it->bid->offer(), qty);
    }
    responses.push_back(std::make_pair(*it, response));
  }

  if (cyclus::IsNegative(tails.quantity())) {
    std::stringstream ss;
    ss << "is being asked to provide more than its current inventory.";
    throw cyclus::ValueError(Agent::InformErrorMsg(ss.str()));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::Material::Ptr CascadeEnrich::Enrich_(cyclus::Material::Ptr mat,
                                             double qty) {
  using cyclus::Material;
  using cyclus::ResCast;
  using cyclus::toolkit::Assays;
  using cyclus::toolkit::UraniumAssay;
  using cyclus::toolkit::FeedQty;
  using cyclus::toolkit::TailsQty;
  // get enrichment parameters
  double product_assay = ProductAssay(FeedAssay());
  double max_product_mass = ProductFlow(max_feed_flow);

  double feed_qty = qty / max_product_mass * max_feed_flow;

  double tails_assay = TailsAssay(FeedAssay());
  double tails_mass = TailsFlow(feed_qty);
  // Determine the composition of the natural uranium
  // (ie. U-235+U-238/TotalMass)
  double pop_qty = inventory.quantity();
  Material::Ptr natu_matl = inventory.Pop(pop_qty, cyclus::eps_rsrc());
  inventory.Push(natu_matl);

  cyclus::toolkit::MatQuery mq(natu_matl);
  std::set<cyclus::Nuc> nucs;
  nucs.insert(922350000);
  nucs.insert(922380000);
  double natu_frac = mq.mass_frac(nucs);
  double feed_req = feed_qty / natu_frac;
  // pop amount from inventory and blob it into one material
  Material::Ptr r;
  try {
    // required so popping doesn't take out too much
    if (cyclus::AlmostEq(feed_req, inventory.quantity())) {
      r = cyclus::toolkit::Squash(inventory.PopN(inventory.count()));
    } else {
      r = inventory.Pop(feed_req, cyclus::eps_rsrc());
    }
  } catch (cyclus::Error& e) {
    std::stringstream ss;
    ss << " tried to remove " << feed_req << " from its inventory of size "
       << inventory.quantity();
    throw cyclus::ValueError(Agent::InformErrorMsg(ss.str()));
  }

  // "enrich" it, but pull out the composition and quantity we require from the
  // blob
  cyclus::Composition::Ptr comp = mat->comp();
  Material::Ptr response = r->ExtractComp(qty, comp);
  tails.Push(r);

  RecordEnrichment_(feed_req);

  LOG(cyclus::LEV_INFO5, "EnrFac") << prototype()
                                   << " has performed an enrichment: ";
  LOG(cyclus::LEV_INFO5, "EnrFac") << "   * Feed Qty: " << feed_req;
  LOG(cyclus::LEV_INFO5, "EnrFac") << "   * Feed Assay: " << FeedAssay() * 100;
  LOG(cyclus::LEV_INFO5, "EnrFac") << "   * Product Qty: " << qty;
  LOG(cyclus::LEV_INFO5, "EnrFac") << "   * Product Assay: "
                                   << product_assay * 100;
  LOG(cyclus::LEV_INFO5, "EnrFac") << "   * Tails Qty: " << tails_mass;
  LOG(cyclus::LEV_INFO5, "EnrFac") << "   * Tails Assay: " << tails_assay * 100;

  return response;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CascadeEnrich::RecordEnrichment_(double natural_u) {
  using cyclus::Context;
  using cyclus::Agent;

  LOG(cyclus::LEV_DEBUG1, "EnrFac") << prototype()
                                    << " has enriched a material:";
  LOG(cyclus::LEV_DEBUG1, "EnrFac") << "  * Amount: " << natural_u;

  Context* ctx = Agent::context();
  ctx->NewDatum("Enrichments")
      ->AddVal("ID", id())
      ->AddVal("Time", ctx->time())
      ->AddVal("Natural_Uranium", natural_u)
      ->Record();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::Material::Ptr CascadeEnrich::Request_() {
  double qty = std::max(0.0, inventory.capacity() - inventory.quantity());
  return cyclus::Material::CreateUntracked(qty,
                                           context()->GetRecipe(feed_recipe));
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::Material::Ptr CascadeEnrich::Offer_(cyclus::Material::Ptr mat) {
  double feed_assay = FeedAssay();
  double product_assay = ProductAssay(feed_assay);

  cyclus::CompMap comp;
  comp[922350000] = product_assay;
  comp[922380000] = 1 - product_assay;

  return cyclus::Material::CreateUntracked(
      mat->quantity(), cyclus::Composition::CreateFromMass(comp));
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CascadeEnrich::ValidReq(const cyclus::Material::Ptr mat) {
  cyclus::toolkit::MatQuery q(mat);
  double u235 = q.atom_frac(922350000);
  double u238 = q.atom_frac(922380000);
  return (u238 > 0 && u235 / (u235 + u238) > tails_assay);
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

double CascadeEnrich::ProductAssay(double feed_assay) {
  cascade_config cascade_tmp = Compute_Assay(cascade, feed_assay);
  return cascade_tmp.stgs_config[cascade_tmp.enrich_stgs - 1].product_assay;
}
double CascadeEnrich::TailsAssay(double feed_assay) {
  cascade_config cascade_tmp = Compute_Assay(cascade, feed_assay);
  return cascade_tmp.stgs_config[-cascade_tmp.stripping_stgs].tail_assay;
}

double CascadeEnrich::ProductFlow(double feed_flow) {
  double feed_ratio = feed_flow / max_feed_flow;
  stg_config last_stg = cascade.stgs_config[cascade.enrich_stgs - 1];
  double product_flow = last_stg.flow * last_stg.cut;
  return feed_ratio * FlowPerMon(product_flow);
}

double CascadeEnrich::TailsFlow(double feed_flow) {
  // this assume mass flow conservation
  return feed_flow - ProductFlow(feed_flow);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
extern "C" cyclus::Agent* ConstructCascadeEnrich(cyclus::Context* ctx) {
  return new CascadeEnrich(ctx);
}

}  // namespace mbmore
