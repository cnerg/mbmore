// Implements the StateInst class
#include "StateInst.h"

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
    bool calc_pursuit = 0;
    // calc PE
    if (context()->time() == 4) {calc_pursuit = 1;}
    // if PE == 1 :
    if (calc_pursuit == 1) {
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
    
    //    std::stringstream ss;
    //    ss << s_proto;
    
    //    cyclus::Agent* a = context()->CreateAgent<Agent>(s_proto);
    //    s_proto = ss.str();
    //    context()->AddPrototype(s_proto, a);
    context()->SchedBuild(this, s_proto);  //builds on next timestep
    BuildNotify(this);
  }
  std::cout << "SECRET DEPLOY!!!! at " << context()->time() << std::endl;
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
