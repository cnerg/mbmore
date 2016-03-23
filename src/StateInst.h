#ifndef MBMORE_SRC_STATE_INST_H_
#define MBMORE_SRC_STATE_INST_H_

#include <utility>
#include <set>
#include <map>

#include "cyclus.h"

namespace mbmore {

typedef std::map<int, std::vector<std::string> > BuildSched;

// Builds and manages agents (facilities) according to a manually specified
// deployment schedule. Deployed agents are automatically decommissioned at
// the end of their lifetime.  The user specifies a list of prototypes for
// each and corresponding build times, number to build, and (optionally)
// lifetimes.  The same prototype can be specified multiple times with any
// combination of the same or different build times, build number, and
// lifetimes.
class StateInst : public cyclus::Institution {
  #pragma cyclus note { \
    "doc": \
      "Builds and manages agents (facilities) according to a manually" \
      " specified deployment schedule. Deployed agents are automatically" \
      " decommissioned at the end of their lifetime.  The user specifies a" \
      " list of prototypes for" \
      " each and corresponding build times, number to build, and (optionally)" \
      " lifetimes.  The same prototype can be specified multiple times with" \
      " any combination of the same or different build times, build number," \
      " and lifetimes. " \
  }

 private:

  std::string prototype = "Secret_Sink";
  int build_time = 10;

  
 public:
  StateInst(cyclus::Context* ctx);

  virtual ~StateInst();

  #pragma cyclus

  virtual void Build(cyclus::Agent* parent);

  virtual void EnterNotify();

  virtual void Tock();

 protected:
  #pragma cyclus var {"default": 0, "tooltip": "pursuit param" ,	\
    "doc": "input for pursuit eqn"}
  
  double pursuit_param;


  

};

}  // namespace mbmore

#endif  // MBMORE_SRC_STATE_INST_H_
