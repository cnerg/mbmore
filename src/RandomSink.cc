// Implements the RandomSink class
#include <algorithm>
#include <sstream>

#include <boost/lexical_cast.hpp>

#include "RandomSink.h"
#include "behavior_functions.h"

namespace mbmore {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RandomSink::RandomSink(cyclus::Context* ctx)
    : cyclus::Facility(ctx),
      social_behav(""), //***
      behav_interval(0), //***
      rng_seed(0), //****
      user_pref(1), //***
      sigma(0), //***
      t_trade(0), //***
      max_inv_size(1e299) {}  // actually only used in header file


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RandomSink::~RandomSink() {}

#pragma cyclus def schema mbmore::RandomSink

#pragma cyclus def annotations mbmore::RandomSink

#pragma cyclus def infiletodb mbmore::RandomSink

#pragma cyclus def snapshot mbmore::RandomSink

#pragma cyclus def snapshotinv mbmore::RandomSink

#pragma cyclus def initinv mbmore::RandomSink

#pragma cyclus def clone mbmore::RandomSink

#pragma cyclus def initfromdb mbmore::RandomSink

#pragma cyclus def initfromcopy mbmore::RandomSink

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string RandomSink::str() {
  using std::string;
  using std::vector;
  std::stringstream ss;
  ss << cyclus::Facility::str();

  string msg = "";
  msg += "accepts commodities ";
  for (vector<string>::iterator commod = in_commods.begin();
       commod != in_commods.end();
       commod++) {
    msg += (commod == in_commods.begin() ? "{" : ", ");
    msg += (*commod);
  }
  msg += "} until its inventory is full at ";
  ss << msg << inventory.capacity() << " kg.";
  return "" + ss.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr>
RandomSink::GetMatlRequests() {
  using cyclus::Material;
  using cyclus::RequestPortfolio;
  using cyclus::Request;
  using cyclus::Composition;

  std::set<RequestPortfolio<Material>::Ptr> ports;

  // If social behavior, amt will be set to zero on non-trading timesteps
  if (amt == 0) {
    return ports;
  }

  // otherwise, respond to all requests
  RequestPortfolio<Material>::Ptr port(new RequestPortfolio<Material>());

  Material::Ptr mat;

  // if no recipe has been specified in either format
  if (recipe_name.empty() and (recipe_names.size() == 0)) {
    mat = cyclus::NewBlankMaterial(amt);
  } else {
    //***    Composition::Ptr rec = this->context()->GetRecipe(recipe_name);
    mat = cyclus::Material::CreateUntracked(amt, curr_recipe); 
  } 

  if (amt > cyclus::eps()) {
    std::vector<std::string>::const_iterator it;
    std::vector<Request<Material>*> mutuals;
    for (it = in_commods.begin(); it != in_commods.end(); ++it) {
      mutuals.push_back(port->AddRequest(mat, this, *it));
    }
    port->AddMutualReqs(mutuals);
    ports.insert(port);
  }  // if amt > eps

  return ports;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::set<cyclus::RequestPortfolio<cyclus::Product>::Ptr>
RandomSink::GetGenRsrcRequests() {
  using cyclus::CapacityConstraint;
  using cyclus::Product;
  using cyclus::RequestPortfolio;
  using cyclus::Request;

  std::set<RequestPortfolio<Product>::Ptr> ports;
  RequestPortfolio<Product>::Ptr
      port(new RequestPortfolio<Product>());

  if (amt > cyclus::eps()) {
    CapacityConstraint<Product> cc(amt);
    port->AddConstraint(cc);

    std::vector<std::string>::const_iterator it;
    for (it = in_commods.begin(); it != in_commods.end(); ++it) {
      std::string quality = "";  // not clear what this should be..
      Product::Ptr rsrc = Product::CreateUntracked(amt, quality);
      port->AddRequest(rsrc, this, *it);
    }

    ports.insert(port);
  }  // if amt > eps

  return ports;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RandomSink::AdjustMatlPrefs(
  cyclus::PrefMap<cyclus::Material>::type& prefs) {

  using cyclus::Bid;
  using cyclus::Material;
  using cyclus::Request;

  cyclus::PrefMap<cyclus::Material>::type::iterator reqit;

  for (reqit = prefs.begin(); reqit != prefs.end(); ++reqit) {
    std::map<Bid<Material>*, double>::iterator mit;
    for (mit = reqit->second.begin(); mit != reqit->second.end(); ++mit) {
      mit->second = user_pref;
    }
  }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RandomSink::AcceptMatlTrades(
    const std::vector< std::pair<cyclus::Trade<cyclus::Material>,
                                 cyclus::Material::Ptr> >& responses) {
  std::vector< std::pair<cyclus::Trade<cyclus::Material>,
                         cyclus::Material::Ptr> >::const_iterator it;
  for (it = responses.begin(); it != responses.end(); ++it) {
    inventory.Push(it->second);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RandomSink::AcceptGenRsrcTrades(
    const std::vector< std::pair<cyclus::Trade<cyclus::Product>,
                                 cyclus::Product::Ptr> >& responses) {
  std::vector< std::pair<cyclus::Trade<cyclus::Product>,
                         cyclus::Product::Ptr> >::const_iterator it;
  for (it = responses.begin(); it != responses.end(); ++it) {
    inventory.Push(it->second);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RandomSink::Tick() {
  using std::string;
  using std::vector;
  LOG(cyclus::LEV_INFO3, "SnkFac") << prototype() << " is ticking {";


  // Determine the correct recipe for the timestep. If only one recipe
  // is given then use that recipe. If multiple recipes are given, choose
  // one randomly
  int n_recipes = recipe_names.size();
  if (n_recipes > 0) {
    int curr_recipe_index = RNG_Integer(0.0, n_recipes, rng_seed);
    curr_recipe = context()->GetRecipe(recipe_names[curr_recipe_index]);
  }
  else {
    curr_recipe = context()->GetRecipe(recipe_name);
  }
  
  // set the amount to be requested on this timestep
  // then determine whether trading will happen on this timestep. If not
  // then change the requested material to zero.
  int cur_time = context()->time();
  
  /// determine the amount to request
  // If sigma=0 then RNG is not queried
  double desired_amt = RNG_NormalDist(avg_qty, sigma, rng_seed);
  amt = std::min(desired_amt, std::max(0.0, inventory.space()));

  if (cur_time < t_trade) {
    std::cout << "Amt is zero because curr time " << cur_time << " <t_trade" << t_trade << std::endl;
    amt = 0;
  }
  if (social_behav == "Every" && behav_interval > 0) {
    if (!EveryXTimestep(cur_time, behav_interval)) // HEU every X time
      {
    std::cout << "Amt is zero because EVERY and interval > 0 " << std::endl;
	amt = 0;
      }
  }
  // Call EveryRandom only if the agent REALLY want it (dummyproofing)
  else if ((social_behav == "Random") && (amt > 0)){
    if (!EveryRandomXTimestep(behav_interval, rng_seed)) // HEU randomly one in X times
      {
	std::cout << "Amt is zero because Random is negatvive " << std::endl;
	amt = 0;
      }
  }
  // If reference, query RNG but force trade as zero quantity.
  else if ((social_behav == "Reference") && (amt > 0)){
    bool res = EveryRandomXTimestep(behav_interval, rng_seed);
    std::cout << "Amt is zero because Reference superficially queries RNG " << std::endl;
    amt = 0;
  }
  
  // inform the simulation about what the sink facility will be requesting
  if (amt > cyclus::eps()) {
    for (vector<string>::iterator commod = in_commods.begin();
         commod != in_commods.end();
         commod++) {
      LOG(cyclus::LEV_INFO4, "SnkFac") << " will request " << amt
                                       << " kg of " << *commod << ".";
    }
  }
  LOG(cyclus::LEV_INFO3, "SnkFac") << "}";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RandomSink::Tock() {
  LOG(cyclus::LEV_INFO3, "SnkFac") << prototype() << " is tocking {";

  // On the tock, the sink facility doesn't really do much.
  // Maybe someday it will record things.
  // For now, lets just print out what we have at each timestep.
  LOG(cyclus::LEV_INFO4, "SnkFac") << "RandomSink " << this->id()
                                   << " is holding " << inventory.quantity()
                                   << " units of material at the close of month "
                                   << context()->time() << ".";
  LOG(cyclus::LEV_INFO3, "SnkFac") << "}";

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
extern "C" cyclus::Agent* ConstructRandomSink(cyclus::Context* ctx) {
  return new RandomSink(ctx);
}

}  // namespace mbmore
