// Implements the RandomEnrich class
#include "RandomEnrich.h"
#include "behavior_functions.h"
#include "sim_init.h"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <limits>
#include <sstream>
#include <vector>

namespace mbmore {

bool SortBids(cyclus::Bid<cyclus::Material>* i,
              cyclus::Bid<cyclus::Material>* j) {
  cyclus::Material::Ptr mat_i = i->offer();
  cyclus::Material::Ptr mat_j = j->offer();

  cyclus::toolkit::MatQuery mq_i(mat_i);
  cyclus::toolkit::MatQuery mq_j(mat_j);

  return ((mq_i.mass(922350000) / mq_i.qty()) <=
          (mq_j.mass(922350000) / mq_j.qty()));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RandomEnrich::RandomEnrich(cyclus::Context* ctx)
    : cyclus::Facility(ctx),
      tails_assay(0),
      sigma_tails(0),
      social_behav("None"),
      behav_interval(0),
      heu_ship_qty(0),
      inspect_freq(0),
      n_swipes(10),
      false_pos(0),
      false_neg(0),
      rng_seed(0),
      swu_capacity(0),
      max_enrich(1),
      initial_feed(0),
      feed_commod(""),
      feed_recipe(""),
      product_commod(""),
      tails_commod(""),
      order_prefs(true) {}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RandomEnrich::~RandomEnrich() {}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string RandomEnrich::str() {
  std::stringstream ss;
  ss << cyclus::Facility::str() << " with enrichment facility parameters:"
     << " * SWU capacity: " << SwuCapacity()
     << " * Tails assay: " << curr_tails_assay
     << " * Feed assay: " << FeedAssay()
     << " * Input cyclus::Commodity: " << feed_commod
     << " * Output cyclus::Commodity: " << product_commod
     << " * Tails cyclus::Commodity: " << tails_commod;
  return ss.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RandomEnrich::Build(cyclus::Agent* parent) {
  using cyclus::Material;

  Facility::Build(parent);
  if (initial_feed > 0) {
    inventory.Push(Material::Create(this, initial_feed,
                                    context()->GetRecipe(feed_recipe)));
  }

  LOG(cyclus::LEV_DEBUG2, "EnrFac") << "RandomEnrich "
                                    << " entering the simuluation: ";
  LOG(cyclus::LEV_DEBUG2, "EnrFac") << str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RandomEnrich::Tick() {
  int cur_time = context()->time();

  // set inspection defaults
  if (cur_time == 0) {
    net_heu = 0;
    HEU_present = 0;
  }

  // decide whether trading if trading only sometimes.
  trade_timestep = 0;
  if (social_behav == "Every" && behav_interval > 0) {
    trade_timestep = (EveryXTimestep(cur_time, behav_interval));
  } else if (social_behav == "Random" && behav_interval > 0) {
    trade_timestep = (EveryRandomXTimestep(behav_interval, rng_seed));
  } else if (social_behav == "None") {
    trade_timestep = 1;
  }

  // determine tails assay for the timestep if it is variable
  curr_tails_assay = RNG_NormalDist(tails_assay, sigma_tails, rng_seed);
  if (curr_tails_assay < (tails_assay - sigma_tails)) {
    curr_tails_assay = tails_assay - sigma_tails;
  }
  if (curr_tails_assay > (tails_assay + sigma_tails)) {
    curr_tails_assay = tails_assay + sigma_tails;
  }

  LOG(cyclus::LEV_INFO3, "EnrFac") << prototype() << " is ticking {";
  LOG(cyclus::LEV_INFO3, "EnrFac") << "}";
  current_swu_capacity = SwuCapacity();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RandomEnrich::Tock() {
  using cyclus::toolkit::RecordTimeSeries;
  RecordTimeSeries<cyclus::toolkit::ENRICH_SWU>(this, intra_timestep_swu_);
  RecordTimeSeries<cyclus::toolkit::ENRICH_FEED>(this, intra_timestep_feed_);

  // Add any inspections to the Inspection table
  bool do_inspect = EveryRandomXTimestep(inspect_freq, rng_seed);
  if (do_inspect == true) {
    RecordInspection_();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr>
RandomEnrich::GetMatlRequests() {
  using cyclus::Material;
  using cyclus::RequestPortfolio;
  using cyclus::Request;

  std::set<RequestPortfolio<Material>::Ptr> ports;
  RequestPortfolio<Material>::Ptr port(new RequestPortfolio<Material>());
  Material::Ptr mat = Request_();
  double amt = mat->quantity();

  if (amt > cyclus::eps()) {
    port->AddRequest(mat, this, feed_commod);
    ports.insert(port);
  }

  return ports;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Sort offers of input material to have higher preference for more
//  U-235 content
void RandomEnrich::AdjustMatlPrefs(
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
void RandomEnrich::AcceptMatlTrades(
    const std::vector<std::pair<cyclus::Trade<cyclus::Material>,
                                cyclus::Material::Ptr> >& responses) {
  // see
  // http://stackoverflow.com/questions/5181183/boostshared-ptr-and-inheritance
  std::vector<std::pair<cyclus::Trade<cyclus::Material>,
                        cyclus::Material::Ptr> >::const_iterator it;
  for (it = responses.begin(); it != responses.end(); ++it) {
    AddMat_(it->second);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::set<cyclus::BidPortfolio<cyclus::Material>::Ptr> RandomEnrich::GetMatlBids(
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
    // BidPortfolio<Material>::Ptr commod_port(new BidPortfolio<Material>());
    BidPortfolio<Material>::Ptr commod_port =
        ConsiderMatlRequests(out_requests);

    /*
    std::vector<Request<Material>*>& commod_requests =
      out_requests[product_commod];
    std::vector<Request<Material>*>::iterator it;
    for (it = commod_requests.begin(); it != commod_requests.end(); ++it) {
      Request<Material>* req = *it;
      Material::Ptr mat = req->target();
      double request_enrich = cyclus::toolkit::UraniumAssay(mat) ;

      if (ValidReq(req->target()) && (request_enrich <= max_enrich)) {
        Material::Ptr offer = Offer_(req->target());
        commod_port->AddBid(req, offer, this);
      }
    }
    */
    Converter<Material>::Ptr sc(
        new SWUConverter(FeedAssay(), curr_tails_assay));
    Converter<Material>::Ptr nc(
        new NatUConverter(FeedAssay(), curr_tails_assay));
    CapacityConstraint<Material> swu(swu_capacity, sc);
    CapacityConstraint<Material> natu(inventory.quantity(), nc);
    commod_port->AddConstraint(swu);
    commod_port->AddConstraint(natu);

    LOG(cyclus::LEV_INFO5, "EnrFac")
        << prototype() << " adding a swu constraint of " << swu.capacity();
    LOG(cyclus::LEV_INFO5, "EnrFac")
        << prototype() << " adding a natu constraint of " << natu.capacity();
    ports.insert(commod_port);
  }
  return ports;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RandomEnrich::ValidReq(const cyclus::Material::Ptr mat) {
  cyclus::toolkit::MatQuery q(mat);
  double u235 = q.atom_frac(922350000);
  double u238 = q.atom_frac(922380000);
  return (u238 > 0 && u235 / (u235 + u238) > curr_tails_assay);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RandomEnrich::GetMatlTrades(
    const std::vector<cyclus::Trade<cyclus::Material> >& trades,
    std::vector<std::pair<cyclus::Trade<cyclus::Material>,
                          cyclus::Material::Ptr> >& responses) {
  using cyclus::Material;
  using cyclus::Trade;

  intra_timestep_swu_ = 0;
  intra_timestep_feed_ = 0;

  std::vector<Trade<Material> >::const_iterator it;
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
      response = tails.Pop(qty);
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
  if (cyclus::IsNegative(current_swu_capacity)) {
    throw cyclus::ValueError("EnrFac " + prototype() +
                             " is being asked to provide more than" +
                             " its SWU capacity.");
  }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RandomEnrich::AddMat_(cyclus::Material::Ptr mat) {
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
cyclus::Material::Ptr RandomEnrich::Request_() {
  double qty = std::max(0.0, inventory.capacity() - inventory.quantity());
  return cyclus::Material::CreateUntracked(qty,
                                           context()->GetRecipe(feed_recipe));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::Material::Ptr RandomEnrich::Offer_(cyclus::Material::Ptr mat) {
  cyclus::toolkit::MatQuery q(mat);
  cyclus::CompMap comp;
  comp[922350000] = q.atom_frac(922350000);
  comp[922380000] = q.atom_frac(922380000);
  return cyclus::Material::CreateUntracked(
      mat->quantity(), cyclus::Composition::CreateFromAtom(comp));
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::Material::Ptr RandomEnrich::Enrich_(cyclus::Material::Ptr mat,
                                            double qty) {
  using cyclus::Material;
  using cyclus::ResCast;
  using cyclus::toolkit::Assays;
  using cyclus::toolkit::UraniumAssay;
  using cyclus::toolkit::SwuRequired;
  using cyclus::toolkit::FeedQty;
  using cyclus::toolkit::TailsQty;

  // get enrichment parameters
  double u_assay = UraniumAssay(mat);
  Assays assays(FeedAssay(), u_assay, curr_tails_assay);
  double swu_req = SwuRequired(qty, assays);
  double natu_req = FeedQty(qty, assays);

  // Determine the composition of the natural uranium
  // (ie. U-235+U-238/TotalMass)
  Material::Ptr natu_matl = inventory.Pop(inventory.quantity());
  inventory.Push(natu_matl);

  cyclus::toolkit::MatQuery mq(natu_matl);
  std::set<cyclus::Nuc> nucs;
  nucs.insert(922350000);
  nucs.insert(922380000);
  double natu_frac = mq.mass_frac(nucs);
  double feed_req = natu_req / natu_frac;

  // pop amount from inventory and blob it into one material
  Material::Ptr r;
  try {
    // required so popping doesn't take out too much
    if (cyclus::AlmostEq(feed_req, inventory.quantity())) {
      r = cyclus::toolkit::Squash(inventory.PopN(inventory.count()));
    } else {
      r = inventory.Pop(feed_req);
    }
  } catch (cyclus::Error& e) {
    NatUConverter nc(FeedAssay(), curr_tails_assay);
    std::stringstream ss;
    ss << " tried to remove " << feed_req << " from its inventory of size "
       << inventory.quantity()
       << " and the conversion of the material into natu is "
       << nc.convert(mat);
    throw cyclus::ValueError(Agent::InformErrorMsg(ss.str()));
  }

  // "enrich" it, but pull out the composition and quantity we require from the
  // blob
  cyclus::Composition::Ptr comp = mat->comp();
  Material::Ptr response = r->ExtractComp(qty, comp);
  tails.Push(r);

  current_swu_capacity -= swu_req;

  intra_timestep_swu_ += swu_req;
  intra_timestep_feed_ += feed_req;
  RecordRandomEnrich_(feed_req, swu_req);

  // If enriched to HEU then record total HEU produced
  double heu_definition = 0.2;
  if (u_assay > heu_definition) {
    net_heu += qty;
  }

  LOG(cyclus::LEV_INFO5, "EnrFac") << prototype()
                                   << " has performed an enrichment: ";
  LOG(cyclus::LEV_INFO5, "EnrFac") << "   * Feed Qty: " << feed_req;
  LOG(cyclus::LEV_INFO5, "EnrFac") << "   * Feed Assay: "
                                   << assays.Feed() * 100;
  LOG(cyclus::LEV_INFO5, "EnrFac") << "   * Product Qty: " << qty;
  LOG(cyclus::LEV_INFO5, "EnrFac") << "   * Product Assay: "
                                   << assays.Product() * 100;
  LOG(cyclus::LEV_INFO5, "EnrFac") << "   * Tails Qty: "
                                   << TailsQty(qty, assays);
  LOG(cyclus::LEV_INFO5, "EnrFac") << "   * Tails Assay: "
                                   << assays.Tails() * 100;
  LOG(cyclus::LEV_INFO5, "EnrFac") << "   * SWU: " << swu_req;
  LOG(cyclus::LEV_INFO5, "EnrFac") << "   * Current SWU capacity: "
                                   << current_swu_capacity;

  return response;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RandomEnrich::RecordRandomEnrich_(double natural_u, double swu) {
  using cyclus::Context;
  using cyclus::Agent;

  LOG(cyclus::LEV_DEBUG1, "EnrFac") << prototype()
                                    << " has enriched a material:";
  LOG(cyclus::LEV_DEBUG1, "EnrFac") << "  * Amount: " << natural_u;
  LOG(cyclus::LEV_DEBUG1, "EnrFac") << "  *    SWU: " << swu;

  Context* ctx = Agent::context();
  ctx->NewDatum("RandomEnrichs")
      ->AddVal("ID", id())
      ->AddVal("Time", ctx->time())
      ->AddVal("Natural_Uranium", natural_u)
      ->AddVal("SWU", swu)
      ->Record();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RandomEnrich::RecordInspection_() {
  using cyclus::Context;
  using cyclus::Agent;

  std::string sample_location = "Cascade";

  // TODO: Add multiple samples to an inspection, rules about only Cascade
  // having true positives? Or increased chance of true based on location?
  // TODO: Make HEU definition a State Var (in Tock)

  // If HEU has been made, then we see if a perfect swipe test would find it
  // by calculating HEU_present (ie. maybe heu has been made but swipe is in
  // the wrong spot).  Its presence becomes more likely to detect
  // scaling with time elapsed because we presume there is increasing
  // contamination. Once contamination would be theoretically measured, this
  // contamination remains for the rest of the simulation.
  // (TODO: scale with quantity HEU produced)
  double cur_time = double(context()->time());
  if ((social_behav == "None") && (HEU_present == false)) {
    // HEU is produced continuously (as requested), and removed when some
    // quantity has been
    // produced. Risk of leakage increases with time in discrete steps
    // TODO: This will fail if a facility is producing HEU but never shipping
    // it and inspections are still supposed to occur because it assumes
    // that HEU can only be detected if it has been removed from cascades for
    // shipping.
    std::cout << "Inspect Time: " << cur_time << "  Net HEU produced "
              << net_heu << std::endl;
    if ((net_heu >= heu_ship_qty) && (heu_ship_qty > 0.0)) {
      HEU_present = XLikely(cur_time / (double(simdur) - 1.0), rng_seed);
      std::cout << "HEU Presence? " << HEU_present << std::endl;
      net_heu -= heu_ship_qty;
    }
  } else if ((net_heu > 0.0) && (HEU_present == false)) {
    // HEU is made/shipped at specific intervals defined by behavior fns,
    // so test whether any has been made/shipped since last inspection
    HEU_present = XLikely(cur_time / (double(simdur) - 1.0), rng_seed);
  }

  // Each sample is N swipes, analyzed independently (with a high rate of
  // false readings in practice).
  // Based on whether HEU is 'detected' in the sample, determine whether or not
  // any false positives or negatives change the swipe result.
  int pos_swipes = 0;
  int n_false_pos = 0;
  int n_false_neg = 0;

  for (int swipeit = 0; swipeit < n_swipes; swipeit++) {
    double prob;
    if (HEU_present == true) {
      prob = false_neg;
    } else {
      prob = false_pos;
    }
    bool flip = XLikely(prob, rng_seed);
    //    std::cout << "Flip? " << flip << std::endl;

    // record false positives, false negatives and net 'positive' swipe results
    if (flip) {
      if (!HEU_present) {
        pos_swipes++;
        n_false_pos++;
      } else {
        n_false_neg++;
      }
    } else {
      if (HEU_present) {
        pos_swipes++;
      }
    }

    //    if ((HEU_present && !flip) || (!HEU_present && flip)){
    //      pos_swipes++;
    //    }
  }

  Context* ctx = Agent::context();
  context()
      ->NewDatum("Inspections")
      ->AddVal("AgentID", id())
      ->AddVal("Time", context()->time())
      ->AddVal("SampleLoc", sample_location)
      ->AddVal("FalsePos", double(n_false_pos) / double(n_swipes))
      ->AddVal("FalseNeg", double(n_false_neg) / double(n_swipes))
      ->AddVal("PosSwipeFrac", double(pos_swipes) / double(n_swipes))
      ->Record();

  /*
  LOG(cyclus::LEV_DEBUG1, "EnrFac") << prototype()
                                    << " Has been inspected:";
  LOG(cyclus::LEV_DEBUG1, "EnrFac") << "  * N_Samples: " << natural_u;
  LOG(cyclus::LEV_DEBUG1, "EnrFac") << "  *    Swipe_frac: " << swu;
  */
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Decide whether each individual bid will be responded to based on whether
// the enrichment facility is trading
cyclus::BidPortfolio<cyclus::Material>::Ptr RandomEnrich::ConsiderMatlRequests(
    cyclus::CommodMap<cyclus::Material>::type& out_requests) {
  using cyclus::Bid;
  using cyclus::BidPortfolio;
  using cyclus::Material;
  using cyclus::Request;

  BidPortfolio<Material>::Ptr commod_port(new BidPortfolio<Material>());

  std::vector<Request<Material>*>& commod_requests =
      out_requests[product_commod];
  std::vector<Request<Material>*>::iterator it;
  for (it = commod_requests.begin(); it != commod_requests.end(); ++it) {
    Request<Material>* req = *it;
    Material::Ptr mat = req->target();
    double request_enrich = cyclus::toolkit::UraniumAssay(mat);
    int cur_time = context()->time();
    // if social behavior on and logic says no trade
    if (!trade_timestep) {
      return commod_port;
    }
    if (ValidReq(req->target()) && request_enrich <= max_enrich) {
      Material::Ptr offer = Offer_(req->target());
      commod_port->AddBid(req, offer, this);
    }
  }  // for each out commod

  return commod_port;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double RandomEnrich::FeedAssay() {
  using cyclus::Material;

  if (inventory.empty()) {
    return 0;
  }
  cyclus::Material::Ptr fission_matl = inventory.Pop(inventory.quantity());
  inventory.Push(fission_matl);
  return cyclus::toolkit::UraniumAssay(fission_matl);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
extern "C" cyclus::Agent* ConstructRandomEnrich(cyclus::Context* ctx) {
  return new RandomEnrich(ctx);
}

}  // namespace mbmore
