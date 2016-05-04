// Implements the StateInst class
#include "StateInst.h"
#include "behavior_functions.h"

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
  /*
  std::set<cyclus::Agent*>::iterator sit;
  for (sit = cyclus::Agent::children().begin();
       sit != cyclus::Agent::children().end();
       ++sit) {
    Agent* a = *sit;
    Register_(a);
  }

  */

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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateInst::Tock() {
  // Has a secret sink already been deployed?
  // TODO:: How to force SecretEnrich to trade Only with SecretSink??

  if (pursuing == 1) {
    // TODO: calc AE
    bool calc_acquire = 0;
    if (calc_acquire == 1) {
      // Makes Sink bid for HEU in AdjustMatlPrefs
      std::cout << "t = " <<context()->time() << ",  Acquired" << std::endl;
    }
  }
  else {
    std::cout << "Calculating pursuit" << std::endl;
    bool pursuit_decision = DecidePursuit();
    if (pursuit_decision == 1) {
      LOG(cyclus::LEV_INFO2, "StateInst") << "StateInst " << this->id()
					  << " is deploying a HEUSink at:" 
					  << context()->time() << ".";
      DeploySecret();
      pursuing = 1;
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

  std::cout << "Adjusting Matl Prefs" << std::endl;

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
      std::cout << "Archetype" << archetype << std::endl;
      if ((you->parent() == me) &&
	  ((archetype == "Sink") || (archetype == "RandomSink"))){
	std::cout << "Testing acquisition" << std::endl;
	for (mit = pmit->second.begin(); mit != pmit->second.end(); ++mit) {
	  if (acquired == 1){
	    mit->second += 1; 
	  }
	  else {
	    mit->second = 0;
	  }
	}
      }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Deploys secret prototypes when called by Tock
void StateInst::DeploySecret() {
  //void StateInst::DeploySecret(cyclus::Agent* parent) {
  //  cyclus::Institution::Build(parent);
  BuildSched::iterator it;

  for (int i = 0; i < secret_protos.size(); i++) {
    std::string s_proto = secret_protos[i];
    context()->SchedBuild(this, s_proto);  //builds on next timestep
    BuildNotify(this);
  }
  std::cout << "SECRET DEPLOY!!!! at " << context()->time() << std::endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// At each timestep where pursuit has not yet occurred, calculate whether to
// pursue at this time step.
bool StateInst::DecidePursuit() {

  // TODO: Weights will be moved to the Region and be uniform across all StateInst
  // TODO: Add in a check that total weighting equals One.
  std::map <std::string, double> P_wt;
  P_wt["Dem"] = 0.5;
  P_wt["React"] = 0.5;

  std::map<std::string, double> y_current;
  std::map<std::string,
	   std::pair<std::string, std::vector<double> > >::iterator eqn_it;

  for(eqn_it = P_f.begin(); eqn_it != P_f.end(); eqn_it++) {
    std::string factor = eqn_it->first;
    std::string function = eqn_it->second.first;
    std::vector<double> constants = eqn_it->second.second;

    std::cout << "factor " << factor << " fn " << function << std::endl;

    y_current[factor] = CalcYVal(function, constants,context()->time());
  }
  
  std::cout << "Dem weight  " << P_wt["Dem"] << "  yint  " << y_current["Dem"] << std::endl;
  
  double pursuit_eqn = P_wt["Dem"]*y_current["Dem"]; //+ P_wt["Reactors"] * y_int; 
  std::cout << "Pursuit Eqn equals : " << pursuit_eqn << std:: endl;

  // Convert pursuit eqn value to a binary value using whatever equation
  
  bool decision = 0;
  if (context()->time() == 4) {
    decision = 1;
  }
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
