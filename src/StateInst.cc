// Implements the StateInst class
#include "StateInst.h"
#include "InteractRegion.h"
#include "behavior_functions.h"
#include <cmath>

namespace mbmore {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StateInst::StateInst(cyclus::Context* ctx)
  : cyclus::Institution(ctx){
    //    kind("State"){
  cyclus::Warn<cyclus::EXPERIMENTAL_WARNING>("the StateInst agent is experimental.");
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StateInst::~StateInst() {}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateInst::BuildNotify(Agent* a) {
  Register_(a);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateInst::DecomNotify(Agent* a) {
  Unregister_(a);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateInst::EnterNotify() {
  cyclus::Institution::EnterNotify();


  //TODO: IS THIS NECESSARY?
  using cyclus::toolkit::CommodityProducer;
  std::vector<std::string>::iterator vit;
  for (vit = declared_protos.begin(); vit != declared_protos.end(); ++vit) {
    Agent* a = context()->CreateAgent<Agent>(*vit);
    Register_(a);

    CommodityProducer* cp_cast = dynamic_cast<CommodityProducer*>(a);
    if (cp_cast != NULL) {
      LOG(cyclus::LEV_INFO3, "mani") << "Registering declared_proto "
                                     << a->prototype() << a->id()
                                     << " with the Builder interface.";
      Builder::Register(cp_cast);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateInst::Register_(Agent* a) {
  using cyclus::toolkit::CommodityProducer;
  using cyclus::toolkit::CommodityProducerManager;

  CommodityProducer* cp_cast = dynamic_cast<CommodityProducer*>(a);
  //  if (cp_cast != NULL) {
    LOG(cyclus::LEV_INFO3, "mani") << "Registering agent "
                                   << a->prototype() << a->id()
                                   << " as a commodity producer.";
    CommodityProducerManager::Register(cp_cast);
    //  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateInst::Unregister_(Agent* a) {
  using cyclus::toolkit::CommodityProducer;
  using cyclus::toolkit::CommodityProducerManager;

  CommodityProducer* cp_cast = dynamic_cast<CommodityProducer*>(a);
  if (cp_cast != NULL)
    CommodityProducerManager::Unregister(cp_cast);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateInst::Tick() {

  // Things to do only at beginning of Simulation
  if (context()->time() == 0){

    // Make sure weapon status definition is allowed
    if ((weapon_status < 0) || (weapon_status > 3) || (weapon_status == 1)){
      throw cyclus::ValueError(
			       "ERROR: Only 0, 2, 3 allowed for Weapon Status");
    }
    
    //Record initial weapon status
    Agent* me = this;
    std::string proto = me->prototype();

    InteractRegion* pseudo_region =
      dynamic_cast<InteractRegion*>(this->parent());
    pseudo_region->UpdateWeaponStatus(proto, weapon_status);

    // If starting status is 'pursuing' or 'acquired', create the secret sink
    // at simulation start
    if ((weapon_status == 2) || (weapon_status == 3)){
      DeploySecret();
    }

    // Calcualte the time points for any random changes to the factors
    std::map<std::string,
	     std::pair<std::string, std::vector<double> > >::iterator eqn_it;
    for(eqn_it = P_f.begin(); eqn_it != P_f.end(); eqn_it++) {
      std::string factor = eqn_it->first;
      std::string function = eqn_it->second.first;
      std::vector<double> constants = eqn_it->second.second;
      
      // If a factor is random, calculate event times
      // RandomStep occurs once and changes the amplitude
      if ((function == "Step" || function == "step")
	  && (constants.size() == 2)){
	double y0 = constants[0];
	double yf = constants[1];
	int t_change = RNG_Integer(0, simdur, rng_seed);
	// add the t_change to the P_f record
	eqn_it->second.second.push_back(t_change);
      }
      // Conflict occurs once and changes to neutral or opposite original value
      // If conflict has a single value and its not +1, 0, -1 then it does not
      // change in the simulation
      if ((factor == "Conflict" || factor == "conflict")
	  && (constants.size() == 1)){
	double yf = constants[0];
	if (std::abs(yf) <= 1){
	  int t_change = RNG_Integer(0, simdur, rng_seed);
	  eqn_it->second.second.push_back(t_change);
	}
      }
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateInst::Tock() {
  // TODO:: How to force SecretEnrich to trade Only with SecretSink??

  InteractRegion* pseudo_region =
    dynamic_cast<InteractRegion*>(this->parent());
  Agent* me = this;
  std::string proto = me->prototype();
  // Pursuit (if detected) and acquire each change the conflict map
  if (weapon_status == 0) {
    std::string eqn_type = "Pursuit";
    bool pursuit_decision = WeaponDecision(eqn_type);
    if (pursuit_decision == 1) {
      LOG(cyclus::LEV_INFO2, "StateInst") << "StateInst " << this->id()
					  << " is deploying a HEUSink at:" 
					  << context()->time() << ".";
      DeploySecret();
      weapon_status = 2;
      pseudo_region->UpdateWeaponStatus(proto, weapon_status);
    }
  }
  // If state is pursuing but hasn't yet acquired
  else if (weapon_status == 2) {
    std::string eqn_type = "Acquire";
    //    std::string eqn_type = "Pu";
    bool acquire_decision = WeaponDecision(eqn_type);
    // State now successfully acquires
    if (acquire_decision == 1) {
      weapon_status = 3;
      pseudo_region->UpdateWeaponStatus(proto, weapon_status);
      LOG(cyclus::LEV_INFO2, "StateInst") << "StateInst " << this->id()
					  << " is producing weapons at: " 
					  << context()->time() << ".";
    }
  }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// State inst disallows any trading from SecretSink or SecretEnrich when
// until acquired = 1.
void StateInst::AdjustMatlPrefs(
  cyclus::PrefMap<cyclus::Material>::type& prefs) {

  using cyclus::Bid;
  using cyclus::Material;
  using cyclus::Request;

  cyclus::PrefMap<cyclus::Material>::type::iterator pmit;
  for (pmit = prefs.begin(); pmit != prefs.end(); ++pmit) {
    std::map<Bid<Material>*, double>::iterator mit;
    Request<Material>* req = pmit->first;
    Agent* you = req->requester()->manager();
    Agent* me = this;
      // If you are my child (then you're secret),
      // and you're a type of Sink, then adjust preferences
      std::string full_name = you->spec();
      std::string archetype = full_name.substr(full_name.rfind(':'));
      if ((you->parent()->id() == me->id()) &&
	  ((archetype == ":Sink") || (archetype == ":RandomSink"))){
	for (mit = pmit->second.begin(); mit != pmit->second.end(); ++mit) {
	  if (weapon_status == 3){
	    mit->second += 1; 
	  }
	  else {
	    mit->second = -1;
	  }
	}
      }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Deploys secret prototypes when called by Tock
void StateInst::DeploySecret() {
  BuildSched::iterator it;

  for (int i = 0; i < secret_protos.size(); i++) {
    std::string s_proto = secret_protos[i];
    context()->SchedBuild(this, s_proto);  //builds on next timestep
    BuildNotify(this);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// At each timestep where pursuit has not yet occurred, calculate whether to
// pursue at this time step.
  bool StateInst::WeaponDecision(std::string eqn_type) {
  using cyclus::Context;
  using cyclus::Agent;
  using cyclus::Recorder;
  cyclus::Datum *d = context()->NewDatum("WeaponProgress");
  d->AddVal("Time", context()->time());
  d->AddVal("AgentId", cyclus::Agent::id());
  d->AddVal("EqnType", eqn_type);

  std::map <std::string, double> P_wt;
  std::map <std::string, double> P_factors;

  // Make a pointer to my parent region so I can access the RegionLevel
  // variables (in a similar way to how the Context provides simulation
  // level information)
  InteractRegion* pseudo_region =
    dynamic_cast<InteractRegion*>(this->parent());
  
  // All defined factors should be recorded with their actual value
  P_wt = pseudo_region->GetWeights("Pursuit");
  // Even if state is already pursuing and working toward acquire, the success
  // rate is determined by the value of the pursuit factors, so score must be
  // calculated
  
  // Any factors not defined for sim should have a value of zero in the table
  std::vector<std::string>& main_factors = pseudo_region->GetmainFactors();
  std::map<std::string, bool> present = pseudo_region->DefinedFactors("Pursuit");

  double pursuit_eqn = 0;

  // Iterate through main list of factors. If not present then record 0
  // in database. If present then calculate current value based on time
  // dynamics
  for(int f = 0; f < main_factors.size(); f++){
    const std::string& factor = main_factors[f];
    bool f_defined  = present[std::string(factor)];
    // for most factors 'relation' defines the function for time dynamics of
    // the factor. But for Conflict, 'relation' is the pair state in the
    // relationship
    std::string relation = P_f[factor].first;
    std::vector<double> constants =  P_f[factor].second;

    // Record zeroes for any columns not defined in input file
    if (!f_defined) {
      d->AddVal(factor.c_str(), 0.0);
    }
    else {
      double factor_curr_y;
      // Determine the State's conflict score for this timestep
      int n_states = pseudo_region->GetNStates();
      if (factor == "Conflict"){
	if (n_states <= 1){
	  factor_curr_y = 0;
	}
	else{
	  Agent* me = this;
	  std::string proto = me->prototype();
	  factor_curr_y =
	    pseudo_region->GetConflictScore("Pursuit", proto);
	  // Then check conflict value to see if it needs to change. If
	  //constants is a single element then it doesn't have a time-based
	  // change. This change is not propogated until the NEXT timestep
	  // This is done last because changing conflict for one state will
	  // also affect another state whose score for this timestep may have
	  // already been calculated.
	  if ((constants.size() > 1) && (constants[1] == context()->time())){
	    int new_val = std::round(constants[0]);
	    // TODO: THIS SHOULD BE eqn_Type not PURSUIT (but doesn't really matteR)
	    pseudo_region->ChangeConflictReln("Pursuit", proto,
				      relation, new_val); 
	  }
	}
      }
      else {
	factor_curr_y = CalcYVal(relation, constants,context()->time());
      }
      pursuit_eqn += (factor_curr_y * P_wt[factor]);
      P_factors[factor] = factor_curr_y;
      d->AddVal(factor.c_str(), factor_curr_y);
    }
  }
  // Convert pursuit eqn result to a Y/N decision
  // GetLikely requires an input value between 0-10, and the function type
  // should be normalized to convert that value to have a max of y=1.0 for x=10
  double likely = pseudo_region->GetLikely(eqn_type, pursuit_eqn);
  bool decision = XLikely(likely, rng_seed);

  d->AddVal("EqnVal", pursuit_eqn);
  d->AddVal("Likelihood", likely);
  d->AddVal("Decision", decision);
  d->Record();
  return decision;  
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateInst::WriteProducerInformation(
  cyclus::toolkit::CommodityProducer* producer) {
  using std::set;
  set<cyclus::toolkit::Commodity,
      cyclus::toolkit::CommodityCompare> commodities =
          producer->ProducedCommodities();
  set<cyclus::toolkit::Commodity, cyclus::toolkit::CommodityCompare>::
      iterator it;

  LOG(cyclus::LEV_DEBUG3, "maninst") << " Clone produces " << commodities.size()
                                     << " commodities.";
  for (it = commodities.begin(); it != commodities.end(); it++) {
    LOG(cyclus::LEV_DEBUG3, "maninst") << " Commodity produced: " << it->name();
    LOG(cyclus::LEV_DEBUG3, "maninst") << "           capacity: " <<
                                       producer->Capacity(*it);
    LOG(cyclus::LEV_DEBUG3, "maninst") << "               cost: " <<
                                       producer->Cost(*it);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
extern "C" cyclus::Agent* ConstructStateInst(cyclus::Context* ctx) {
  return new StateInst(ctx);
}

}  // namespace mbmore
