// Implements the StateInst class
#include "StateInst.h"

namespace mbmore {

StateInst::StateInst(cyclus::Context* ctx) : cyclus::Institution(ctx) {}

StateInst::~StateInst() {}
  /*
    change build time to be based on logic of a PE outcome variable (from Tick?)
   */

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateInst::Build(cyclus::Agent* parent) {
  cyclus::Institution::Build(parent);

  std::string proto = prototype;
  
  cyclus::Agent* a = context()->CreateAgent<Agent>(proto);
  a->lifetime(-1);
  
  context()->AddPrototype(proto, a);

  context()->SchedBuild(this, proto, build_time);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateInst::EnterNotify() {
  cyclus::Institution::EnterNotify();
  std::stringstream ss;
  ss << "prototype '" << prototype << "is deployed at " << context()->time();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateInst::Tock() {
  LOG(cyclus::LEV_INFO3, "StateInst") << prototype << " is tocking {";

  //  if (deploy == 1) {
  LOG(cyclus::LEV_INFO4, "StateInst") << "StateInst " << this->id()
				      << " is deploying a HEUSink at:" 
				      << context()->time() << ".";
  //  }
  LOG(cyclus::LEV_INFO3, "StateInst") << "}";

    

    
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
extern "C" cyclus::Agent* ConstructStateInst(cyclus::Context* ctx) {
  return new StateInst(ctx);
}

}  // namespace mbmore
