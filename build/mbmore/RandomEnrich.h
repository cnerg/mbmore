
#line 1 "/Users/mbmcgarry/git/mbmore/src/RandomEnrich.h"
#ifndef MBMORE_SRC_ENRICHMENT_H_
#define MBMORE_SRC_ENRICHMENT_H_

#include <string>

#include "cyclus.h"

namespace mbmore {

/// @class SWUConverter
///
/// @brief The SWUConverter is a simple Converter class for material to
/// determine the amount of SWU required for their proposed enrichment
class SWUConverter : public cyclus::Converter<cyclus::Material> {
 public:
  SWUConverter(double feed_commod, double tails) : feed_(feed_commod),
    tails_(tails) {}
  virtual ~SWUConverter() {}

  /// @brief provides a conversion for the SWU required
  virtual double convert(
      cyclus::Material::Ptr m,
      cyclus::Arc const * a = NULL,
      cyclus::ExchangeTranslationContext<cyclus::Material>
          const * ctx = NULL) const {
    cyclus::toolkit::Assays assays(feed_, cyclus::toolkit::UraniumAssay(m),
                                   tails_);
    return cyclus::toolkit::SwuRequired(m->quantity(), assays);
  }

  /// @returns true if Converter is a SWUConverter and feed and tails equal
  virtual bool operator==(Converter& other) const {
    SWUConverter* cast = dynamic_cast<SWUConverter*>(&other);
    return cast != NULL &&
    feed_ == cast->feed_ &&
    tails_ == cast->tails_;
  }

 private:
  double feed_, tails_;
};

/// @class NatUConverter
///
/// @brief The NatUConverter is a simple Converter class for material to
/// determine the amount of natural uranium required for their proposed
/// enrichment
class NatUConverter : public cyclus::Converter<cyclus::Material> {
 public:
  NatUConverter(double feed_commod, double tails) : feed_(feed_commod),
    tails_(tails) {}
  virtual ~NatUConverter() {}

  /// @brief provides a conversion for the amount of natural Uranium required
  virtual double convert(
      cyclus::Material::Ptr m,
      cyclus::Arc const * a = NULL,
      cyclus::ExchangeTranslationContext<cyclus::Material>
          const * ctx = NULL) const {
    cyclus::toolkit::Assays assays(feed_, cyclus::toolkit::UraniumAssay(m),
                                   tails_);
    cyclus::toolkit::MatQuery mq(m);
    std::set<cyclus::Nuc> nucs;
    nucs.insert(922350000);
    nucs.insert(922380000);

    double natu_frac = mq.mass_frac(nucs);
    double natu_req = cyclus::toolkit::FeedQty(m->quantity(), assays);
    return natu_req / natu_frac;
  }

  /// @returns true if Converter is a NatUConverter and feed and tails equal
  virtual bool operator==(Converter& other) const {
    NatUConverter* cast = dynamic_cast<NatUConverter*>(&other);
    return cast != NULL &&
    feed_ == cast->feed_ &&
    tails_ == cast->tails_;
  }

 private:
  double feed_, tails_;
};

///  The RandomEnrich facility is a simple Agent that enriches natural
///  uranium in a Cyclus simulation. It does not explicitly compute
///  the physical enrichment process, rather it calculates the SWU
///  required to convert an source uranium recipe (ie. natural uranium)
///  into a requested enriched recipe (ie. 4% enriched uranium), given
///  the natural uranium inventory constraint and its SWU capacity
///  constraint.
///
///  The RandomEnrich facility requests an input commodity and associated recipe
///  whose quantity is its remaining inventory capacity.  All facilities
///  trading the same input commodity (even with different recipes) will
///  offer materials for trade.  The RandomEnrich facility accepts any input
///  materials with enrichments less than its tails assay, as long as some
///  U235 is present, and preference increases with U235 content.  If no
///  U235 is present in the offered material, the trade preference is set
///  to -1 and the material is not accepted.  Any material components other
///  other than U235 and U238 are sent directly to the tails buffer.
///
///  The RandomEnrich facility will bid on any request for its output commodity
///  up to the maximum allowed enrichment (if not specified, default is 100%)
///  It bids on either the request quantity, or the maximum quanity allowed
///  by its SWU constraint or natural uranium inventory, whichever is lower.
///  If multiple output commodities with different enrichment levels are
///  requested and the facility does not have the SWU or quantity capacity
///  to meet all requests, the requests are fully, then partially filled
///  in unspecified but repeatable order.
///
///  The RandomEnrich facility also offers its tails as an output commodity with
///  no associated recipe.  Bids for tails are constrained only by total
///  tails inventory.

class RandomEnrich : public cyclus::Facility {
#pragma cyclus note {   	    "niche": "enrichment facility",				    "doc":								  "The RandomEnrich facility is a simple agent that enriches natural "	   "uranium in a Cyclus simulation. It does not explicitly compute "	  "the physical enrichment process, rather it calculates the SWU "	  "required to convert an source uranium recipe (i.e. natural uranium) "   "into a requested enriched recipe (i.e. 4% enriched uranium), given "   "the natural uranium inventory constraint and its SWU capacity "   "constraint."							  "\n\n"								  "The RandomEnrich facility requests an input commodity and associated "   "recipe whose quantity is its remaining inventory capacity.  All "   "facilities trading the same input commodity (even with different "   "recipes) will offer materials for trade.  The RandomEnrich facility "   "accepts any input materials with enrichments less than its tails assay, "  "as long as some U235 is present, and preference increases with U235 "   "content.  If no U235 is present in the offered material, the trade "   "preference is set to -1 and the material is not accepted.  Any material "   "components other than U235 and U238 are sent directly to the tails buffer."  "\n\n"								  "The RandomEnrich facility will bid on any request for its output commodity "  "up to the maximum allowed enrichment (if not specified, default is 100%) "  "It bids on either the request quantity, or the maximum quanity allowed "   "by its SWU constraint or natural uranium inventory, whichever is lower. "   "If multiple output commodities with different enrichment levels are "   "requested and the facility does not have the SWU or quantity capacity "   "to meet all requests, the requests are fully, then partially filled "   "in unspecified but repeatable order."				  "\n\n"								  "Accumulated tails inventory is offered for trading as a specifiable "   "output commodity.", }
#line 149 "/Users/mbmcgarry/git/mbmore/src/RandomEnrich.h"
 public:
  // --- Module Members ---
  ///    Constructor for the RandomEnrich class
  ///    @param ctx the cyclus context for access to simulation-wide parameters
  RandomEnrich(cyclus::Context* ctx);

  ///     Destructor for the RandomEnrich class
  virtual ~RandomEnrich();
  virtual void InitFrom(mbmore::RandomEnrich* m) {
    cyclus::Facility::InitFrom(m);
    feed_commod = m->feed_commod;
    feed_recipe = m->feed_recipe;
    product_commod = m->product_commod;
    tails_commod = m->tails_commod;
    tails_assay = m->tails_assay;
    sigma_tails = m->sigma_tails;
    initial_feed = m->initial_feed;
    max_feed_inventory = m->max_feed_inventory;
    max_enrich = m->max_enrich;
    order_prefs = m->order_prefs;
    social_behav = m->social_behav;
    behav_interval = m->behav_interval;
    rng_seed = m->rng_seed;
    swu_capacity = m->swu_capacity;
    tails.capacity(m->tails.capacity());
    inventory.capacity(m->inventory.capacity());
  };
#line 157 "/Users/mbmcgarry/git/mbmore/src/RandomEnrich.h"

  virtual void InitFrom(cyclus::QueryableBackend* b) {
    cyclus::Facility::InitFrom(b);
    cyclus::QueryResult qr = b->Query("Info", NULL);
    feed_commod = qr.GetVal<std::string>("feed_commod");
    feed_recipe = qr.GetVal<std::string>("feed_recipe");
    product_commod = qr.GetVal<std::string>("product_commod");
    tails_commod = qr.GetVal<std::string>("tails_commod");
    tails_assay = qr.GetVal<double>("tails_assay");
    sigma_tails = qr.GetVal<double>("sigma_tails");
    initial_feed = qr.GetVal<double>("initial_feed");
    max_feed_inventory = qr.GetVal<double>("max_feed_inventory");
    max_enrich = qr.GetVal<double>("max_enrich");
    order_prefs = qr.GetVal<bool>("order_prefs");
    social_behav = qr.GetVal<std::string>("social_behav");
    behav_interval = qr.GetVal<double>("behav_interval");
    rng_seed = qr.GetVal<bool>("rng_seed");
    swu_capacity = qr.GetVal<double>("swu_capacity");
    tails.capacity(1e+300);
    inventory.capacity(max_feed_inventory);
  };

  virtual void InfileToDb(cyclus::InfileTree* tree, cyclus::DbInit di) {
    cyclus::Facility::InfileToDb(tree, di);
    cyclus::InfileTree* sub = tree->SubTree("config/*");
    int i;
    int n;
    {
      std::string feed_commod_val = cyclus::Query<std::string>(sub, "feed_commod");
      feed_commod = feed_commod_val;
    }
    {
      std::string feed_recipe_val = cyclus::Query<std::string>(sub, "feed_recipe");
      feed_recipe = feed_recipe_val;
    }
    {
      std::string product_commod_val = cyclus::Query<std::string>(sub, "product_commod");
      product_commod = product_commod_val;
    }
    {
      std::string tails_commod_val = cyclus::Query<std::string>(sub, "tails_commod");
      tails_commod = tails_commod_val;
    }
    if (sub->NMatches("tails_assay") > 0) {
      {
        double tails_assay_val = cyclus::Query<double>(sub, "tails_assay");
        tails_assay = tails_assay_val;
      }
    } else {
      double tails_assay_tmp = 0.003;
      tails_assay = tails_assay_tmp;
    }
    if (sub->NMatches("sigma_tails") > 0) {
      {
        double sigma_tails_val = cyclus::Query<double>(sub, "sigma_tails");
        sigma_tails = sigma_tails_val;
      }
    } else {
      double sigma_tails_tmp = 0;
      sigma_tails = sigma_tails_tmp;
    }
    if (sub->NMatches("initial_feed") > 0) {
      {
        double initial_feed_val = cyclus::Query<double>(sub, "initial_feed");
        initial_feed = initial_feed_val;
      }
    } else {
      double initial_feed_tmp = 0;
      initial_feed = initial_feed_tmp;
    }
    if (sub->NMatches("max_feed_inventory") > 0) {
      {
        double max_feed_inventory_val = cyclus::Query<double>(sub, "max_feed_inventory");
        max_feed_inventory = max_feed_inventory_val;
      }
    } else {
      double max_feed_inventory_tmp = 1e+299;
      max_feed_inventory = max_feed_inventory_tmp;
    }
    if (sub->NMatches("max_enrich") > 0) {
      {
        double max_enrich_val = cyclus::Query<double>(sub, "max_enrich");
        max_enrich = max_enrich_val;
      }
    } else {
      double max_enrich_tmp = 1.0;
      max_enrich = max_enrich_tmp;
    }
    if (sub->NMatches("order_prefs") > 0) {
      {
        bool order_prefs_val = cyclus::Query<bool>(sub, "order_prefs");
        order_prefs = order_prefs_val;
      }
    } else {
      bool order_prefs_tmp = true;
      order_prefs = order_prefs_tmp;
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
    if (sub->NMatches("rng_seed") > 0) {
      {
        bool rng_seed_val = cyclus::Query<bool>(sub, "rng_seed");
        rng_seed = rng_seed_val;
      }
    } else {
      bool rng_seed_tmp = false;
      rng_seed = rng_seed_tmp;
    }
    if (sub->NMatches("swu_capacity") > 0) {
      {
        double swu_capacity_val = cyclus::Query<double>(sub, "swu_capacity");
        swu_capacity = swu_capacity_val;
      }
    } else {
      double swu_capacity_tmp = 1e+299;
      swu_capacity = swu_capacity_tmp;
    }
    di.NewDatum("Info")
    ->AddVal("feed_commod", feed_commod)
    ->AddVal("feed_recipe", feed_recipe)
    ->AddVal("product_commod", product_commod)
    ->AddVal("tails_commod", tails_commod)
    ->AddVal("tails_assay", tails_assay)
    ->AddVal("sigma_tails", sigma_tails)
    ->AddVal("initial_feed", initial_feed)
    ->AddVal("max_feed_inventory", max_feed_inventory)
    ->AddVal("max_enrich", max_enrich)
    ->AddVal("order_prefs", order_prefs)
    ->AddVal("social_behav", social_behav)
    ->AddVal("behav_interval", behav_interval)
    ->AddVal("rng_seed", rng_seed)
    ->AddVal("swu_capacity", swu_capacity)
    ->Record();
  };

  virtual cyclus::Agent* Clone() {
    mbmore::RandomEnrich* m = new mbmore::RandomEnrich(context());
    m->InitFrom(this);
    return m;
  };

  virtual std::string schema() {
    return ""
      "<interleave>\n"
      "    <element name=\"feed_commod\">\n"
      "        <data type=\"string\"/>\n"
      "    </element>\n"
      "    <element name=\"feed_recipe\">\n"
      "        <data type=\"string\"/>\n"
      "    </element>\n"
      "    <element name=\"product_commod\">\n"
      "        <data type=\"string\"/>\n"
      "    </element>\n"
      "    <element name=\"tails_commod\">\n"
      "        <data type=\"string\"/>\n"
      "    </element>\n"
      "    <optional>\n"
      "        <element name=\"tails_assay\">\n"
      "            <data type=\"double\"/>\n"
      "        </element>\n"
      "    </optional>\n"
      "    <optional>\n"
      "        <element name=\"sigma_tails\">\n"
      "            <data type=\"double\"/>\n"
      "        </element>\n"
      "    </optional>\n"
      "    <optional>\n"
      "        <element name=\"initial_feed\">\n"
      "            <data type=\"double\"/>\n"
      "        </element>\n"
      "    </optional>\n"
      "    <optional>\n"
      "        <element name=\"max_feed_inventory\">\n"
      "            <data type=\"double\"/>\n"
      "        </element>\n"
      "    </optional>\n"
      "    <optional>\n"
      "        <element name=\"max_enrich\">\n"
      "            <data type=\"double\">\n"
      "                <param name=\"minInclusive\">0</param>\n"
      "                <param name=\"maxInclusive\">1</param>\n"
      "            </data>\n"
      "        </element>\n"
      "    </optional>\n"
      "    <optional>\n"
      "        <element name=\"order_prefs\">\n"
      "            <data type=\"boolean\"/>\n"
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
      "        <element name=\"rng_seed\">\n"
      "            <data type=\"boolean\"/>\n"
      "        </element>\n"
      "    </optional>\n"
      "    <optional>\n"
      "        <element name=\"swu_capacity\">\n"
      "            <data type=\"double\"/>\n"
      "        </element>\n"
      "    </optional>\n"
      "</interleave>\n";
  };

  virtual Json::Value annotations() {
    Json::Value root;
    Json::Reader reader;
    bool parsed_ok = reader.parse(
      "{\"name\":\"mbmore::RandomEnrich\",\"entity\":\"facility\""
      ",\"parents\":[\"cyclus::Facility\"],\"all_parents\":[\"cy"
      "clus::Agent\",\"cyclus::Facility\",\"cyclus::Ider\",\"cy"
      "clus::StateWrangler\",\"cyclus::TimeListener\",\"cyclu"
      "s::Trader\"],\"vars\":{\"feed_commod\":{\"uitype\":\"incom"
      "modity\",\"index\":0,\"doc\":\"feed commodity that the "
      "enrichment facility accepts\",\"tooltip\":\"feed "
      "commodity\",\"uilabel\":\"Feed Commodity\",\"alias\":\"fee"
      "d_commod\",\"type\":\"std::string\"},\"feed_recipe\":{\"ui"
      "type\":\"recipe\",\"index\":1,\"doc\":\"recipe for "
      "enrichment facility feed "
      "commodity\",\"tooltip\":\"feed recipe\",\"uilabel\":\"Feed"
      " Recipe\",\"alias\":\"feed_recipe\",\"type\":\"std::string"
      "\"},\"product_commod\":{\"uitype\":\"outcommodity\",\"inde"
      "x\":2,\"doc\":\"product commodity that the enrichment "
      "facility generates\",\"tooltip\":\"product "
      "commodity\",\"uilabel\":\"Product Commodity\",\"alias\":\""
      "product_commod\",\"type\":\"std::string\"},\"tails_commo"
      "d\":{\"uitype\":\"outcommodity\",\"index\":3,\"doc\":\"tails"
      " commodity supplied by enrichment "
      "facility\",\"tooltip\":\"tails "
      "commodity\",\"uilabel\":\"Tails Commodity\",\"alias\":\"ta"
      "ils_commod\",\"type\":\"std::string\"},\"tails_assay\":{\""
      "index\":4,\"default\":0.003,\"doc\":\"tails assay from "
      "the enrichment process\",\"tooltip\":\"tails "
      "assay\",\"uilabel\":\"Tails Assay\",\"alias\":\"tails_assa"
      "y\",\"type\":\"double\"},\"sigma_tails\":{\"index\":5,\"defa"
      "ult\":0,\"doc\":\"standard deviation (FWHM) of the "
      "normal distribution used to generate tails assay "
      "(if 0 then no distribution is calculated and assay"
      " is constant in time.\",\"tooltip\":\"standard "
      "deviation of tails\",\"uilabel\":\"sigma_tails\",\"alias"
      "\":\"sigma_tails\",\"type\":\"double\"},\"initial_feed\":{\""
      "index\":6,\"default\":0,\"doc\":\"amount of natural "
      "uranium stored at the enrichment facility at the "
      "beginning of the simulation "
      "(kg)\",\"tooltip\":\"initial uranium reserves "
      "(kg)\",\"uilabel\":\"Initial Feed Inventory\",\"alias\":\""
      "initial_feed\",\"type\":\"double\"},\"max_feed_inventory"
      "\":{\"index\":7,\"default\":1e+299,\"doc\":\"maximum total"
      " inventory of natural uranium in the enrichment "
      "facility (kg)\",\"tooltip\":\"max inventory of feed "
      "material (kg)\",\"uilabel\":\"Maximum Feed Inventory\","
      "\"alias\":\"max_feed_inventory\",\"type\":\"double\"},\"max"
      "_enrich\":{\"index\":8,\"default\":1.0,\"doc\":\"maximum "
      "allowed weight fraction of U235 in "
      "product\",\"tooltip\":\"maximum allowed enrichment "
      "fraction\",\"uilabel\":\"Maximum Allowed RandomEnrich\""
      ",\"alias\":\"max_enrich\",\"type\":\"double\",\"schema\":\"<o"
      "ptional>          <element name=\\\"max_enrich\\\">"
      "              <data type=\\\"double\\\">"
      "                  <param "
      "name=\\\"minInclusive\\\">0</param>                  "
      "<param name=\\\"maxInclusive\\\">1</param>"
      "              </data>          </element>      </o"
      "ptional>\"},\"order_prefs\":{\"index\":9,\"default\":1,\"d"
      "oc\":\"turn on preference ordering for input "
      "material so that EF chooses higher U235 content "
      "first\",\"tooltip\":\"Rank Material Requests by U235 "
      "Content\",\"uilabel\":\"Prefer feed with higher U235 c"
      "ontent\",\"alias\":\"order_prefs\",\"userlevel\":10,\"type"
      "\":\"bool\"},\"social_behav\":{\"index\":10,\"default\":\"No"
      "ne\",\"doc\":\"type of social behavior used in trade "
      "decisions: None, Every, Random where "
      "behav_interval describes the time interval for "
      "behavior action\",\"tooltip\":\"social behavior\",\"uila"
      "bel\":\"social_behav\",\"alias\":\"social_behav\",\"type\":"
      "\"std::string\"},\"behav_interval\":{\"index\":11,\"defau"
      "lt\":0,\"doc\":\"interval of social behavior: Every or"
      " EveryRandom.  If 0 then behavior is not "
      "implemented\",\"tooltip\":\"interval for behavior\",\"ui"
      "label\":\"behav_interval\",\"alias\":\"behav_interval\",\""
      "type\":\"double\"},\"rng_seed\":{\"index\":12,\"default\":0"
      ",\"doc\":\"seed on current system time if set to -1, "
      "otherwise seed on number defined\",\"tooltip\":\"Seed "
      "for RNG\",\"uilabel\":\"rng_seed\",\"alias\":\"rng_seed\",\""
      "type\":\"bool\"},\"swu_capacity\":{\"index\":13,\"default\""
      ":1e+299,\"doc\":\"separative work unit (SWU) capacity"
      " of enrichment facility (kgSWU/timestep) "
      "\",\"tooltip\":\"SWU capacity "
      "(kgSWU/month)\",\"uilabel\":\"SWU Capacity\",\"alias\":\"s"
      "wu_capacity\",\"type\":\"double\"},\"inventory\":{\"type\":"
      "[\"cyclus::toolkit::ResBuf\",\"cyclus::Material\"],\"ca"
      "pacity\":\"max_feed_inventory\",\"index\":14},\"tails\":{"
      "\"index\":15,\"type\":[\"cyclus::toolkit::ResBuf\",\"cycl"
      "us::Material\"]}},\"doc\":\"The RandomEnrich facility "
      "is a simple agent that enriches natural uranium in"
      " a Cyclus simulation. It does not explicitly "
      "compute the physical enrichment process, rather it"
      " calculates the SWU required to convert an source "
      "uranium recipe (i.e. natural uranium) into a "
      "requested enriched recipe (i.e. 4% enriched "
      "uranium), given the natural uranium inventory "
      "constraint and its SWU capacity constraint.\\n\\nThe"
      " RandomEnrich facility requests an input commodity"
      " and associated recipe whose quantity is its "
      "remaining inventory capacity.  All facilities "
      "trading the same input commodity (even with "
      "different recipes) will offer materials for trade."
      "  The RandomEnrich facility accepts any input "
      "materials with enrichments less than its tails "
      "assay, as long as some U235 is present, and "
      "preference increases with U235 content.  If no "
      "U235 is present in the offered material, the trade"
      " preference is set to -1 and the material is not "
      "accepted.  Any material components other than U235"
      " and U238 are sent directly to the tails "
      "buffer.\\n\\nThe RandomEnrich facility will bid on "
      "any request for its output commodity up to the "
      "maximum allowed enrichment (if not specified, "
      "default is 100%) It bids on either the request "
      "quantity, or the maximum quanity allowed by its "
      "SWU constraint or natural uranium inventory, "
      "whichever is lower. If multiple output commodities"
      " with different enrichment levels are requested "
      "and the facility does not have the SWU or quantity"
      " capacity to meet all requests, the requests are "
      "fully, then partially filled in unspecified but "
      "repeatable order.\\n\\nAccumulated tails inventory "
      "is offered for trading as a specifiable output "
      "commodity.\",\"niche\":\"enrichment facility\"}", root);
    if (!parsed_ok) {
      throw cyclus::ValueError("failed to parse annotations for mbmore::RandomEnrich.");
    }
    return root;
  };

  virtual void InitInv(cyclus::Inventories& inv) {
    tails.Push(inv["tails"]);
        inventory.Push(inv["inventory"]);
    
  };

  virtual cyclus::Inventories SnapshotInv() {
    cyclus::Inventories invs;
    invs["tails"] = tails.PopNRes(tails.count());
    tails.Push(invs["tails"]);
    invs["inventory"] = inventory.PopNRes(inventory.count());
    inventory.Push(invs["inventory"]);
    return invs;
  };

  virtual void Snapshot(cyclus::DbInit di) {
    di.NewDatum("Info")
    ->AddVal("feed_commod", feed_commod)
    ->AddVal("feed_recipe", feed_recipe)
    ->AddVal("product_commod", product_commod)
    ->AddVal("tails_commod", tails_commod)
    ->AddVal("tails_assay", tails_assay)
    ->AddVal("sigma_tails", sigma_tails)
    ->AddVal("initial_feed", initial_feed)
    ->AddVal("max_feed_inventory", max_feed_inventory)
    ->AddVal("max_enrich", max_enrich)
    ->AddVal("order_prefs", order_prefs)
    ->AddVal("social_behav", social_behav)
    ->AddVal("behav_interval", behav_interval)
    ->AddVal("rng_seed", rng_seed)
    ->AddVal("swu_capacity", swu_capacity)
    ->Record();
  };
#line 159 "/Users/mbmcgarry/git/mbmore/src/RandomEnrich.h"

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

  /// @brief The RandomEnrich request Materials of its given
  /// commodity.
  virtual std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr>
      GetMatlRequests();

  /// @brief The RandomEnrich adjusts preferences for offers of
  /// natural uranium it has received to maximize U-235 content
  /// Any offers that have zero U-235 content are not accepted
  virtual void AdjustMatlPrefs(cyclus::PrefMap<cyclus::Material>::type& prefs);
 
  /// @brief The RandomEnrich place accepted trade Materials in their
  /// Inventory
  virtual void AcceptMatlTrades(
      const std::vector< std::pair<cyclus::Trade<cyclus::Material>,
      cyclus::Material::Ptr> >& responses);

  /// @brief Responds to each request for this facility's commodity.  If a given
  /// request is more than this facility's inventory or SWU capacity, it will
  /// offer its minimum of its capacities.
  virtual std::set<cyclus::BidPortfolio<cyclus::Material>::Ptr>
    GetMatlBids(cyclus::CommodMap<cyclus::Material>::type&
    commod_requests);

  /// @brief respond to each trade with a material enriched to the appropriate
  /// level given this facility's inventory
  ///
  /// @param trades all trades in which this trader is the supplier
  /// @param responses a container to populate with responses to each trade
  virtual void GetMatlTrades(
    const std::vector< cyclus::Trade<cyclus::Material> >& trades,
    std::vector<std::pair<cyclus::Trade<cyclus::Material>,
    cyclus::Material::Ptr> >& responses);
  // ---

  ///  @brief Determines if a particular material is a valid request to respond
  ///  to.  Valid requests must contain U235 and U238 and must have a relative
  ///  U235-to-U238 ratio less than this facility's tails_assay().
  ///  @return true if the above description is met by the material
  bool ValidReq(const cyclus::Material::Ptr mat);

  /// Determines whether EF is offering bids on a timestep
  bool trade_timestep;

  ///  @brief Determines if a particular request will be responded to
  ///  based on user specification such as maximum allowed enrichment
  ///  or other behavior parameters.
  virtual cyclus::BidPortfolio<cyclus::Material>::Ptr
    ConsiderMatlRequests(cyclus::CommodMap<cyclus::Material>::type&
		     commod_requests);

  inline void SetMaxInventorySize(double size) {
    max_feed_inventory = size;
    inventory.capacity(size);
  }
 
  inline void SwuCapacity(double capacity) {
    swu_capacity = capacity;
    current_swu_capacity = swu_capacity;
  }

  inline double SwuCapacity() const { return swu_capacity; }

  inline const cyclus::toolkit::ResBuf<cyclus::Material>& Tails() const {
    return tails;
  } 
  // Tails assay at each timestep. Re-assessed at each Tick if sigma_tails > 0
  double curr_tails_assay ;
 


 private:
  ///   @brief adds a material into the natural uranium inventory
  ///   @throws if the material is not the same composition as the feed_recipe
  void AddMat_(cyclus::Material::Ptr mat);

  ///   @brief generates a request for this facility given its current state.
  ///   Quantity of the material will be equal to remaining inventory size.
  cyclus::Material::Ptr Request_();

  ///  @brief Generates a material offer for a given request. The response
  ///  composition will be comprised only of U235 and U238 at their relative
  ///  ratio in the requested material. The response quantity will be the
  ///  same as the requested commodity.
  ///
  ///  @param req the requested material being responded to
  cyclus::Material::Ptr Offer_(cyclus::Material::Ptr req);

  cyclus::Material::Ptr Enrich_(cyclus::Material::Ptr mat, double qty);

  ///  @brief calculates the feed assay based on the unenriched inventory
  double FeedAssay();

  ///  @brief records and enrichment with the cyclus::Recorder
  void RecordRandomEnrich_(double natural_u, double swu);
  
  #pragma cyclus var {     "tooltip": "feed commodity",					    "doc": "feed commodity that the enrichment facility accepts",	    "uilabel": "Feed Commodity",                                        "uitype": "incommodity"   }
#line 283 "/Users/mbmcgarry/git/mbmore/src/RandomEnrich.h"
  std::string feed_commod;
  
  #pragma cyclus var {     "tooltip": "feed recipe",						    "doc": "recipe for enrichment facility feed commodity",		    "uilabel": "Feed Recipe",                                       "uitype": "recipe"   }
#line 291 "/Users/mbmcgarry/git/mbmore/src/RandomEnrich.h"
  std::string feed_recipe;
  
  #pragma cyclus var {     "tooltip": "product commodity",					    "doc": "product commodity that the enrichment facility generates",	     "uilabel": "Product Commodity",                                         "uitype": "outcommodity"   }
#line 299 "/Users/mbmcgarry/git/mbmore/src/RandomEnrich.h"
  std::string product_commod;
  
  #pragma cyclus var {							    "tooltip": "tails commodity",					    "doc": "tails commodity supplied by enrichment facility",		    "uilabel": "Tails Commodity",                                       "uitype": "outcommodity"   }
#line 307 "/Users/mbmcgarry/git/mbmore/src/RandomEnrich.h"
  std::string tails_commod;

  #pragma cyclus var {							    "default": 0.003, "tooltip": "tails assay",				    "uilabel": "Tails Assay",                                   "doc": "tails assay from the enrichment process",         }
#line 314 "/Users/mbmcgarry/git/mbmore/src/RandomEnrich.h"
  double tails_assay;

  #pragma cyclus var {"default": 0, "tooltip": "standard deviation of tails",                          "doc": "standard deviation (FWHM) of the normal "                                  "distribution used to generate tails "                                  "assay (if 0 then no distribution is "                                  "calculated and assay is constant in time."   }
#line 322 "/Users/mbmcgarry/git/mbmore/src/RandomEnrich.h"
  double sigma_tails;  

  #pragma cyclus var {							    "default": 0, "tooltip": "initial uranium reserves (kg)",		    "uilabel": "Initial Feed Inventory",				    "doc": "amount of natural uranium stored at the enrichment "	    "facility at the beginning of the simulation (kg)"			  }
#line 330 "/Users/mbmcgarry/git/mbmore/src/RandomEnrich.h"
  double initial_feed;

  #pragma cyclus var {							    "default": 1e299, "tooltip": "max inventory of feed material (kg)",     "uilabel": "Maximum Feed Inventory",                                    "doc": "maximum total inventory of natural uranium in "		           "the enrichment facility (kg)"       }
#line 338 "/Users/mbmcgarry/git/mbmore/src/RandomEnrich.h"
  double max_feed_inventory;
 
  #pragma cyclus var {     "default": 1.0,						    "tooltip": "maximum allowed enrichment fraction",		    "doc": "maximum allowed weight fraction of U235 in product",    "uilabel": "Maximum Allowed RandomEnrich",     "schema": '<optional>'				     	           '          <element name="max_enrich">'			           '              <data type="double">'			           '                  <param name="minInclusive">0</param>'           '                  <param name="maxInclusive">1</param>'           '              </data>'					           '          </element>'					           '      </optional>'					     }
#line 354 "/Users/mbmcgarry/git/mbmore/src/RandomEnrich.h"
  double max_enrich;

  #pragma cyclus var {     "default": 1,		           "userlevel": 10,							    "tooltip": "Rank Material Requests by U235 Content",		    "uilabel": "Prefer feed with higher U235 content",     "doc": "turn on preference ordering for input material "		           "so that EF chooses higher U235 content first"   }
#line 364 "/Users/mbmcgarry/git/mbmore/src/RandomEnrich.h"
  bool order_prefs;
  double initial_reserves;
  //***
  #pragma cyclus var {"default": "None", "tooltip": "social behavior" ,	                          "doc": "type of social behavior used in trade "                                  "decisions: None, Every, Random "                                  "where behav_interval describes the "                                  "time interval for behavior action"}
#line 372 "/Users/mbmcgarry/git/mbmore/src/RandomEnrich.h"
  std::string social_behav;

  #pragma cyclus var {"default": 0, "tooltip": "interval for behavior" ,                      "doc": "interval of social behavior: Every or "                             "EveryRandom.  If 0 then behavior is not "                              "implemented"}
#line 378 "/Users/mbmcgarry/git/mbmore/src/RandomEnrich.h"
  double behav_interval;

  #pragma cyclus var {"default": 0, "tooltip": "Seed for RNG" ,		                          "doc": "seed on current system time if set to -1,"                                  " otherwise seed on number defined"}
#line 383 "/Users/mbmcgarry/git/mbmore/src/RandomEnrich.h"
  bool rng_seed;
  //***
  
  #pragma cyclus var {						           "default": 1e299,						           "tooltip": "SWU capacity (kgSWU/month)",			           "uilabel": "SWU Capacity",                                             "doc": "separative work unit (SWU) capacity of enrichment "		           "facility (kgSWU/timestep) "                                       }
#line 393 "/Users/mbmcgarry/git/mbmore/src/RandomEnrich.h"
  double swu_capacity;

  double current_swu_capacity;

  #pragma cyclus var { 'capacity': 'max_feed_inventory' }
  cyclus::toolkit::ResBuf<cyclus::Material> inventory;  // natural u
  #pragma cyclus var {}
  cyclus::toolkit::ResBuf<cyclus::Material> tails;  // depleted u

  // used to total intra-timestep swu and natu usage for meeting requests -
  // these help enable time series generation.
  double intra_timestep_swu_;
  double intra_timestep_feed_;
  
  friend class RandomEnrichTest;
  // ---
};
 
}  // namespace mbmore

#endif // MBMORE_SRC_ENRICHMENT_FACILITY_H_