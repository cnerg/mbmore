#ifndef MBMORE_SRC_CASCADE_ENRICH_H_
#define MBMORE_SRC_CASCADE_ENRICH_H_

#include <string>

#include "cyclus.h"
#include "sim_init.h"

namespace mbmore {

class CascadeEnrich : public cyclus::Facility {
#pragma cyclus note {   	  \
  "niche": "enrichment facility",				  \
  "doc":								\
  "The CascadeEnrich facility based on the Cycamore Enrich facility. " \
 "timesteps (see README for full implementation documentation ",\
}
  /*
Working with cycamore Develop build:  3ada148442de636d
cyclus master commit b54c91516c6

Conceptual Design:
x Build phase: Design cascade
x A: Cascade defined by initial machine parameters and target enrichment:
x 1) Machine SWU, alpha: height, diameter, efficiency (fixed for sim)
                       cut, velocity, max machine feed (may change later)
x 2) Machine Assays: alpha, feed assay
x 3) Stages/Cascade: alpha, feed assay, product assay (per stage) AND waste assay
               OR: # machines for Enrich/Strip (feed, SWU, alpha)
x 4) Matl/Stage: feed, alpha, stage feed and product assay

Tick Phase: Given Cascade Feed, calculate Product, Waste, and SWU
** Machine feed = Cascade Feed/(Machines/Stage0) **
** Machine SWU (1) from machine feed **
** Total SWU = #machines * Machine SWU **
** Iterate through Matl/Stage (4) to determine total Product, Waste **

Tick Phase: At Critical Timestep, change parameters:
A) Change (1) max SWU of machine: 
      - velocity, temperature, cut,  feed assay

B) Change cascade feed assay
      - new max enrichment(Assay_from_NStages)

   */

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

  //  virtual std::string version() { return CYCAMORE_VERSION; }

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


  
 public:
  // --- Module Members ---
  ///    Constructor for the CascadeEnrich class
  ///    @param ctx the cyclus context for access to simulation-wide parameters
  CascadeEnrich(cyclus::Context* ctx);

  ///     Destructor for the CascadeEnrich class
  virtual ~CascadeEnrich();

  #pragma cyclus

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

  /// @brief The Enrichment request Materials of its given
  /// commodity.
    virtual std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr>
      GetMatlRequests();

  /// @brief The Enrichment adjusts preferences for offers of
  /// natural uranium it has received to maximize U-235 content
  /// Any offers that have zero U-235 content are not accepted
  virtual void AdjustMatlPrefs(cyclus::PrefMap<cyclus::Material>::type& prefs);

  /// @brief The Enrichment place accepted trade Materials in their
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

  
   inline void SetMaxInventorySize(double size) {
    max_feed_inventory = size;
    inventory.capacity(size);
  }
 
  inline void SwuCapacity(double capacity) {
    swu_capacity = capacity;
    current_swu_capacity = swu_capacity;
  }

  inline double SwuCapacity() const { return swu_capacity; }

  
  ///  @brief Determines if a particular material is a valid request to respond
  ///  to.  Valid requests must contain U235 and U238 and must have a relative
  ///  U235-to-U238 ratio less than this facility's tails_assay().
  ///  @return true if the above description is met by the material
  bool ValidReq(const cyclus::Material::Ptr mat);


  /*
  // TODO: DELETE, These are not used 
  int NEnrichStages(double alpha, double delU, double feed,
		       double feed_assay);

  int NStripStages(double alpha, double delU, double feed,
		       double feed_assay);

  */
 private:
    ///  @brief calculates the feed assay based on the unenriched inventory
  double FeedAssay();

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

  ///  @brief records and enrichment with the cyclus::Recorder
  void RecordEnrichment_(double natural_u, double swu);

  
  // These state variables are constrained by the design input params at
  // the start of the simulation:
  // Set by max feed for an individual machine
  double design_delU;
  double design_alpha;
  // Set by design assays (feed, product, waste)
  int n_enrich_stages;
  int n_strip_stages;
  // Set by maximum allowable centrifuges
  double max_feed_inventory;
  double swu_capacity;


  
  // TODO: Turn into optional state variables
  const double v_a = 485; // m/s
  const double height = 0.5; // meters
  const double diameter = 0.15; // meters
  const double machine_feed = 15*60*60/((1e3)*60*60*1000.0) ; // g/hr to kg/sec
  const double temp = 320.0 ; //Kelvin

  // TODO: Turn into Required state variables
  const double cascade_feed = 739/(30.4*24*60*60) ; // g/hr to kg/sec
  //  const double design_feed_assay = 0.0071;
  //  const double design_product_assay = 0.0071;

  // Not physical constants but fixed assumptions for a cascade separating
  // out U235 from U238 in UF6 gas
  const double M = 0.352; // kg/mol UF6
  const double dM = 0.003; // kg/mol U238 - U235
  const double x = 1000;  // Pressure ratio (Glaser)

  const double flow_internal = 2.0 ;  // can vary from 2-4
  const double eff = 1.0;  // typical efficiencies <0.6
  const double cut = 0.5;  // target for ideal cascade

  
 private:
  #pragma cyclus var { \
    "tooltip": "feed recipe",						\
    "doc": "recipe for enrichment facility feed commodity",		\
    "uilabel": "Feed Recipe",                                   \
    "uitype": "recipe" \
  }
  std::string feed_recipe;

#pragma cyclus var {							\
  "default": 0, "tooltip": "initial uranium reserves (kg)",		\
    "uilabel": "Initial Feed Inventory",				\
    "doc": "amount of natural uranium stored at the enrichment "	\
    "facility at the beginning of the simulation (kg)"			\
  }
  double initial_feed;


  /* Redundant, only need initial feed
 #pragma cyclus var {							\
    "default": 1e299, "tooltip": "Desired feed capacity (kg)", \
    "uilabel": "Design Maximum Feed Capacity",                          \
    "doc": "maximum total capacity to process natural uranium per timestep "\
           "in the enrichment facility (kg)"     \
  }
  double design_feed;
  */

#pragma cyclus var {							\
  "default": 0, "tooltip": "number of centrifuges available ",\
  "uilabel": "Number of Centrifuges",					\
    "doc": "number of centrifuges available to make the cascade" \
  }
  int max_centrifuges;


  // TODO: USE FEED RECIPE TO DETERMINE FEED ASSAY!!!
#pragma cyclus var {							\
    "default": 0.0071, "tooltip": "initial uranium reserves (kg)",		\
    "uilabel": "Initial feed assay",				\
    "doc": "desired fraction of U235 in feed material (should be consistent "\
           "with feed recipe" \
  }
  double design_feed_assay;

  #pragma cyclus var {							\
    "default": 0.035, "tooltip": "Initial target product assay",	\
    "uilabel": "Target product assay",				\
    "doc": "desired fraction of U235 in product"\
  }
  double design_product_assay;

  #pragma cyclus var {							\
    "default": 0.003, "tooltip": "Initial target waste assay",	\
    "uilabel": "Target waste assay",				\
    "doc": "desired fraction of U235 in waste"\
  }
  double design_waste_assay;

  //TODO: REMOVE THIS BECAUSE IT IS OVER_WRITTEN BASED ON SWU CONSTRAINT
  // THAT COME OUT OF CASCADE DESIGN
  /*
#pragma cyclus var {							\
    "default": 1e299, "tooltip": "max inventory of feed material (kg)", \
    "uilabel": "Maximum Feed Inventory",                                \
    "doc": "maximum total inventory of natural uranium in "		\
           "the enrichment facility (kg)"     \
  }
  double max_feed_inventory;

#pragma cyclus var {						       \
    "default": 1e299,						       \
    "tooltip": "SWU capacity (kgSWU/month)",			       \
    "uilabel": "SWU Capacity",                                         \
    "uitype": "range",                                                  \
    "range": [0.0, 1e299],                                               \
    "doc": "separative work unit (SWU) capacity of enrichment "		\
           "facility (kgSWU/timestep) "                                     \
  }
  double swu_capacity;

  */
  
  // FROM LEGACY EF, MAY NEED TO BE REMOVED LATER
   #pragma cyclus var {							\
    "default": 0.003, "tooltip": "tails assay",				\
    "uilabel": "Tails Assay",                               \
    "doc": "tails assay from the enrichment process",       \
  }
  double tails_assay;

#pragma cyclus var {		       \
    "default": 1,		       \
    "userlevel": 10,							\
    "tooltip": "Rank Material Requests by U235 Content",		\
    "uilabel": "Prefer feed with higher U235 content", \
    "doc": "turn on preference ordering for input material "		\
           "so that EF chooses higher U235 content first" \
  }
  bool order_prefs;


#pragma cyclus var {						\
    "default": 1.0,						\
    "tooltip": "maximum allowed enrichment fraction",		\
    "doc": "maximum allowed weight fraction of U235 in product", \
    "uilabel": "Maximum Allowed Enrichment", \
    "schema": '<optional>'				     	   \
        '          <element name="max_enrich">'			   \
        '              <data type="double">'			   \
        '                  <param name="minInclusive">0</param>'   \
        '                  <param name="maxInclusive">1</param>'   \
        '              </data>'					   \
        '          </element>'					   \
        '      </optional>'					   \
  }
  double max_enrich;

#pragma cyclus var {							\
    "tooltip": "feed commodity",					\
    "doc": "feed commodity that the enrichment facility accepts",	\
    "uilabel": "Feed Commodity",                                    \
    "uitype": "incommodity" \
  }
  std::string feed_commod;

#pragma cyclus var {							\
    "tooltip": "product commodity",					\
    "doc": "product commodity that the enrichment facility generates",	 \
    "uilabel": "Product Commodity",                                     \
    "uitype": "outcommodity" \
  }
  std::string product_commod;
  
  #pragma cyclus var {							\
    "tooltip": "tails commodity",					\
    "doc": "tails commodity supplied by enrichment facility",		\
    "uilabel": "Tails Commodity",                                   \
    "uitype": "outcommodity" \
  }
  std::string tails_commod;

  double current_swu_capacity;
  
 #pragma cyclus var {}
  cyclus::toolkit::ResBuf<cyclus::Material> tails;  // depleted u

  // used to total intra-timestep swu and natu usage for meeting requests -
  // these help enable time series generation.
  double intra_timestep_swu_;
  double intra_timestep_feed_;
  
  // END LEGACY
  
  #pragma cyclus var { 'capacity': 'max_feed_inventory' }
  cyclus::toolkit::ResBuf<cyclus::Material> inventory;  // natural u
  
  friend class CascadeEnrichTest;
  // ---
};
 
}  // namespace mbmore

#endif // MBMORE_SRC_CASCADE_ENRICH_H_
