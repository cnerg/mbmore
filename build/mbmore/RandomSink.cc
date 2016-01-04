#line 1 "/Users/mbmcgarry/git/mbmore/src/RandomSink.cc"
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
std::string RandomSink::schema() {
  return ""
    "<interleave>\n"
    "    <element name=\"in_commods\">\n"
    "        <oneOrMore>\n"
    "            <element name=\"val\">\n"
    "                <data type=\"string\"/>\n"
    "            </element>\n"
    "        </oneOrMore>\n"
    "    </element>\n"
    "    <optional>\n"
    "        <element name=\"recipe_name\">\n"
    "            <data type=\"string\"/>\n"
    "        </element>\n"
    "    </optional>\n"
    "    <optional>\n"
    "        <element name=\"recipe_names\">\n"
    "            <oneOrMore>\n"
    "                <element name=\"val\">\n"
    "                    <data type=\"string\"/>\n"
    "                </element>\n"
    "            </oneOrMore>\n"
    "        </element>\n"
    "    </optional>\n"
    "    <optional>\n"
    "        <element name=\"social_behav\">\n"
    "            <data type=\"string\"/>\n"
    "        </element>\n"
    "    </optional>\n"
    "    <optional>\n"
    "        <element name=\"behav_interval\">\n"
    "            <data type=\"double\"/>\n"
    "        </element>\n"
    "    </optional>\n"
    "    <optional>\n"
    "        <element name=\"user_pref\">\n"
    "            <data type=\"int\"/>\n"
    "        </element>\n"
    "    </optional>\n"
    "    <optional>\n"
    "        <element name=\"rng_seed\">\n"
    "            <data type=\"int\"/>\n"
    "        </element>\n"
    "    </optional>\n"
    "    <optional>\n"
    "        <element name=\"avg_qty\">\n"
    "            <data type=\"double\"/>\n"
    "        </element>\n"
    "    </optional>\n"
    "    <optional>\n"
    "        <element name=\"sigma\">\n"
    "            <data type=\"double\"/>\n"
    "        </element>\n"
    "    </optional>\n"
    "    <optional>\n"
    "        <element name=\"t_trade\">\n"
    "            <data type=\"double\"/>\n"
    "        </element>\n"
    "    </optional>\n"
    "    <optional>\n"
    "        <element name=\"max_inv_size\">\n"
    "            <data type=\"double\"/>\n"
    "        </element>\n"
    "    </optional>\n"
    "    <optional>\n"
    "        <element name=\"capacity\">\n"
    "            <data type=\"double\"/>\n"
    "        </element>\n"
    "    </optional>\n"
    "</interleave>\n";
};
#line 26 "/Users/mbmcgarry/git/mbmore/src/RandomSink.cc"

Json::Value RandomSink::annotations() {
  Json::Value root;
  Json::Reader reader;
  bool parsed_ok = reader.parse(
    "{\"name\":\"mbmore::RandomSink\",\"entity\":\"facility\",\""
    "parents\":[\"cyclus::Facility\"],\"all_parents\":[\"cycl"
    "us::Agent\",\"cyclus::Facility\",\"cyclus::Ider\",\"cycl"
    "us::StateWrangler\",\"cyclus::TimeListener\",\"cyclus:"
    ":Trader\"],\"vars\":{\"in_commods\":{\"uitype\":[\"oneormo"
    "re\",\"incommodity\"],\"index\":0,\"doc\":\"commodities "
    "that the sink facility accepts\",\"tooltip\":[\"input "
    "commodities\",\"\"],\"uilabel\":[\"List of Input Commodi"
    "ties\",\"\"],\"alias\":[\"in_commods\",\"val\"],\"type\":[\"st"
    "d::vector\",\"std::string\"]},\"recipe_name\":{\"uitype\""
    ":\"recipe\",\"index\":1,\"default\":\"\",\"doc\":\"name of "
    "recipe to use for material requests, where the "
    "default (empty string) is to accept "
    "everything\",\"tooltip\":\"requested "
    "composition\",\"uilabel\":\"Input Recipe\",\"alias\":\"rec"
    "ipe_name\",\"type\":\"std::string\"},\"recipe_names\":{\"u"
    "itype\":[\"oneormore\",\"recipe\"],\"index\":2,\"default\":"
    "[],\"doc\":\"Input recipes to request for different "
    "timesteps (randomly chosen)\",\"tooltip\":[\"recipe_na"
    "mes\",\"\"],\"uilabel\":[\"Input Recipe List\",\"\"],\"alias"
    "\":[\"recipe_names\",\"val\"],\"type\":[\"std::vector\",\"st"
    "d::string\"]},\"social_behav\":{\"index\":3,\"default\":\""
    "None\",\"doc\":\"type of social behavior used in trade"
    " decisions: None, Every, Random, Reference where "
    "behav_interval describes the time interval for "
    "behavior action.Reference queries the RNG to mimic"
    " Random, but returns zero material qty to "
    "trade.\",\"tooltip\":\"social behavior\",\"uilabel\":\"soc"
    "ial_behav\",\"alias\":\"social_behav\",\"type\":\"std::str"
    "ing\"},\"behav_interval\":{\"index\":4,\"default\":0,\"doc"
    "\":\"interval of social behavior: Every or "
    "EveryRandom.  If 0 then behavior is not "
    "implemented\",\"tooltip\":\"interval for behavior\",\"ui"
    "label\":\"behav_interval\",\"alias\":\"behav_interval\",\""
    "type\":\"double\"},\"user_pref\":{\"index\":5,\"default\":1"
    ",\"doc\":\"change the default preference for requests"
    " from this agent\",\"tooltip\":\"user-defined preferen"
    "ce\",\"uilabel\":\"user_pref\",\"alias\":\"user_pref\",\"typ"
    "e\":\"int\"},\"rng_seed\":{\"index\":6,\"default\":0,\"doc\":"
    "\"seed on current system time if set to -1, "
    "otherwise seed on number "
    "defined\",\"tooltip\":\"defines RNG seed\",\"uilabel\":\"r"
    "ng_seed\",\"alias\":\"rng_seed\",\"type\":\"int\"},\"avg_qty"
    "\":{\"index\":7,\"default\":1e+299,\"doc\":\"mean for the "
    "normal distribution that is sampled to determine "
    "the amount of material actually requested at each "
    "time step\",\"tooltip\":\"sink avg_qty\",\"uilabel\":\"avg"
    "_qty\",\"alias\":\"avg_qty\",\"type\":\"double\"},\"sigma\":{"
    "\"index\":8,\"default\":0,\"doc\":\"standard deviation "
    "(FWHM) of the normal distribution used to generate"
    " requested amount of material "
    "(avg_qty)\",\"tooltip\":\"standard deviation\",\"uilabel"
    "\":\"sigma\",\"alias\":\"sigma\",\"type\":\"double\"},\"t_trad"
    "e\":{\"index\":9,\"default\":0,\"doc\":\"At all timesteps "
    "before this value, the facility does make material"
    " requests. At times at or beyond this value, "
    "requests are made subject to the other behavioral "
    "features available in this "
    "archetype\",\"tooltip\":\"time to being allowing trade"
    "s\",\"uilabel\":\"t_trade\",\"alias\":\"t_trade\",\"type\":\"d"
    "ouble\"},\"max_inv_size\":{\"index\":10,\"default\":1e+29"
    "9,\"doc\":\"total maximum inventory size of sink "
    "facility\",\"tooltip\":\"sink maximum inventory "
    "size\",\"uilabel\":\"Maximum Inventory\",\"alias\":\"max_i"
    "nv_size\",\"type\":\"double\"},\"capacity\":{\"index\":11,\""
    "default\":1e+299,\"doc\":\"capacity the sink facility "
    "can accept at each time step\",\"tooltip\":\"sink "
    "capacity\",\"uilabel\":\"Maximum Throughput\",\"alias\":\""
    "capacity\",\"type\":\"double\"},\"inventory\":{\"type\":[\"c"
    "yclus::toolkit::ResBuf\",\"cyclus::Resource\"],\"capac"
    "ity\":\"max_inv_size\",\"index\":12}},\"doc\":\" A sink "
    "facility that accepts materials and products with "
    "a fixed\\n throughput (per time step) capacity and "
    "a lifetime capacity defined by\\n a total inventory"
    " size. The inventory size and throughput "
    "capacity\\n both default to infinite. If a recipe "
    "is provided, it will request\\n material with that "
    "recipe. Requests are made for any number of\\n "
    "specified commodities.\\n\"}", root);
  if (!parsed_ok) {
    throw cyclus::ValueError("failed to parse annotations for mbmore::RandomSink.");
  }
  return root;
};
#line 28 "/Users/mbmcgarry/git/mbmore/src/RandomSink.cc"

void RandomSink::InfileToDb(cyclus::InfileTree* tree, cyclus::DbInit di) {
  cyclus::Facility::InfileToDb(tree, di);
  cyclus::InfileTree* sub = tree->SubTree("config/*");
  int i;
  int n;
  {
    cyclus::InfileTree* bub = sub->SubTree("in_commods", 0);
    cyclus::InfileTree* sub = bub;
    int n1 = sub->NMatches("val");
    std::vector< std::string > in_commods_val;
    in_commods_val.resize(n1);
    for (int i1 = 0; i1 < n1; ++i1) {
      std::string elem;
      {
        std::string elem_in = cyclus::Query<std::string>(sub, "val", i1);
        elem = elem_in;
      }
      in_commods_val[i1] = elem;
    }
    in_commods = in_commods_val;
  }
  if (sub->NMatches("recipe_name") > 0) {
    {
      std::string recipe_name_val = cyclus::Query<std::string>(sub, "recipe_name");
      recipe_name = recipe_name_val;
    }
  } else {
    std::string recipe_name_tmp("");
    recipe_name = recipe_name_tmp;
  }
  if (sub->NMatches("recipe_names") > 0) {
    {
      cyclus::InfileTree* bub = sub->SubTree("recipe_names", 0);
      cyclus::InfileTree* sub = bub;
      int n1 = sub->NMatches("val");
      std::vector< std::string > recipe_names_val;
      recipe_names_val.resize(n1);
      for (int i1 = 0; i1 < n1; ++i1) {
        std::string elem;
        {
          std::string elem_in = cyclus::Query<std::string>(sub, "val", i1);
          elem = elem_in;
        }
        recipe_names_val[i1] = elem;
      }
      recipe_names = recipe_names_val;
    }
  } else {
    std::vector< std::string > recipe_names_tmp;
    recipe_names_tmp.resize(0);
    {
    }
    recipe_names = recipe_names_tmp;
  }
  if (sub->NMatches("social_behav") > 0) {
    {
      std::string social_behav_val = cyclus::Query<std::string>(sub, "social_behav");
      social_behav = social_behav_val;
    }
  } else {
    std::string social_behav_tmp("None");
    social_behav = social_behav_tmp;
  }
  if (sub->NMatches("behav_interval") > 0) {
    {
      double behav_interval_val = cyclus::Query<double>(sub, "behav_interval");
      behav_interval = behav_interval_val;
    }
  } else {
    double behav_interval_tmp = 0;
    behav_interval = behav_interval_tmp;
  }
  if (sub->NMatches("user_pref") > 0) {
    {
      int user_pref_val = cyclus::Query<int>(sub, "user_pref");
      user_pref = user_pref_val;
    }
  } else {
    int user_pref_tmp = 1;
    user_pref = user_pref_tmp;
  }
  if (sub->NMatches("rng_seed") > 0) {
    {
      int rng_seed_val = cyclus::Query<int>(sub, "rng_seed");
      rng_seed = rng_seed_val;
    }
  } else {
    int rng_seed_tmp = 0;
    rng_seed = rng_seed_tmp;
  }
  if (sub->NMatches("avg_qty") > 0) {
    {
      double avg_qty_val = cyclus::Query<double>(sub, "avg_qty");
      avg_qty = avg_qty_val;
    }
  } else {
    double avg_qty_tmp = 1e+299;
    avg_qty = avg_qty_tmp;
  }
  if (sub->NMatches("sigma") > 0) {
    {
      double sigma_val = cyclus::Query<double>(sub, "sigma");
      sigma = sigma_val;
    }
  } else {
    double sigma_tmp = 0;
    sigma = sigma_tmp;
  }
  if (sub->NMatches("t_trade") > 0) {
    {
      double t_trade_val = cyclus::Query<double>(sub, "t_trade");
      t_trade = t_trade_val;
    }
  } else {
    double t_trade_tmp = 0;
    t_trade = t_trade_tmp;
  }
  if (sub->NMatches("max_inv_size") > 0) {
    {
      double max_inv_size_val = cyclus::Query<double>(sub, "max_inv_size");
      max_inv_size = max_inv_size_val;
    }
  } else {
    double max_inv_size_tmp = 1e+299;
    max_inv_size = max_inv_size_tmp;
  }
  if (sub->NMatches("capacity") > 0) {
    {
      double capacity_val = cyclus::Query<double>(sub, "capacity");
      capacity = capacity_val;
    }
  } else {
    double capacity_tmp = 1e+299;
    capacity = capacity_tmp;
  }
  di.NewDatum("Info")
  ->AddVal("in_commods", in_commods)
  ->AddVal("recipe_name", recipe_name)
  ->AddVal("recipe_names", recipe_names)
  ->AddVal("social_behav", social_behav)
  ->AddVal("behav_interval", behav_interval)
  ->AddVal("user_pref", user_pref)
  ->AddVal("rng_seed", rng_seed)
  ->AddVal("avg_qty", avg_qty)
  ->AddVal("sigma", sigma)
  ->AddVal("t_trade", t_trade)
  ->AddVal("max_inv_size", max_inv_size)
  ->AddVal("capacity", capacity)
  ->Record();
};
#line 30 "/Users/mbmcgarry/git/mbmore/src/RandomSink.cc"

void RandomSink::Snapshot(cyclus::DbInit di) {
  di.NewDatum("Info")
  ->AddVal("in_commods", in_commods)
  ->AddVal("recipe_name", recipe_name)
  ->AddVal("recipe_names", recipe_names)
  ->AddVal("social_behav", social_behav)
  ->AddVal("behav_interval", behav_interval)
  ->AddVal("user_pref", user_pref)
  ->AddVal("rng_seed", rng_seed)
  ->AddVal("avg_qty", avg_qty)
  ->AddVal("sigma", sigma)
  ->AddVal("t_trade", t_trade)
  ->AddVal("max_inv_size", max_inv_size)
  ->AddVal("capacity", capacity)
  ->Record();
};
#line 32 "/Users/mbmcgarry/git/mbmore/src/RandomSink.cc"

cyclus::Inventories RandomSink::SnapshotInv() {
  cyclus::Inventories invs;
  invs["inventory"] = inventory.PopNRes(inventory.count());
  inventory.Push(invs["inventory"]);
  return invs;
};
#line 34 "/Users/mbmcgarry/git/mbmore/src/RandomSink.cc"

void RandomSink::InitInv(cyclus::Inventories& inv) {
  inventory.Push(inv["inventory"]);
  
};
#line 36 "/Users/mbmcgarry/git/mbmore/src/RandomSink.cc"

cyclus::Agent* RandomSink::Clone() {
  mbmore::RandomSink* m = new mbmore::RandomSink(context());
  m->InitFrom(this);
  return m;
};
#line 38 "/Users/mbmcgarry/git/mbmore/src/RandomSink.cc"

void RandomSink::InitFrom(cyclus::QueryableBackend* b) {
  cyclus::Facility::InitFrom(b);
  cyclus::QueryResult qr = b->Query("Info", NULL);
  in_commods = qr.GetVal<std::vector< std::string > >("in_commods");
  recipe_name = qr.GetVal<std::string>("recipe_name");
  recipe_names = qr.GetVal<std::vector< std::string > >("recipe_names");
  social_behav = qr.GetVal<std::string>("social_behav");
  behav_interval = qr.GetVal<double>("behav_interval");
  user_pref = qr.GetVal<int>("user_pref");
  rng_seed = qr.GetVal<int>("rng_seed");
  avg_qty = qr.GetVal<double>("avg_qty");
  sigma = qr.GetVal<double>("sigma");
  t_trade = qr.GetVal<double>("t_trade");
  max_inv_size = qr.GetVal<double>("max_inv_size");
  capacity = qr.GetVal<double>("capacity");
  inventory.capacity(max_inv_size);
};
#line 40 "/Users/mbmcgarry/git/mbmore/src/RandomSink.cc"

void RandomSink::InitFrom(mbmore::RandomSink* m) {
  cyclus::Facility::InitFrom(m);
  in_commods = m->in_commods;
  recipe_name = m->recipe_name;
  recipe_names = m->recipe_names;
  social_behav = m->social_behav;
  behav_interval = m->behav_interval;
  user_pref = m->user_pref;
  rng_seed = m->rng_seed;
  avg_qty = m->avg_qty;
  sigma = m->sigma;
  t_trade = m->t_trade;
  max_inv_size = m->max_inv_size;
  capacity = m->capacity;
  inventory.capacity(m->inventory.capacity());
};
#line 44 "/Users/mbmcgarry/git/mbmore/src/RandomSink.cc"

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

  std::cout << "sink is holding" << inventory.quantity() << " at "
	    << context()->time() << std::endl;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
extern "C" cyclus::Agent* ConstructRandomSink(cyclus::Context* ctx) {
  return new RandomSink(ctx);
}

}  // namespace mbmore