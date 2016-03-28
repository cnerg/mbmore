#ifndef MBMORE_SRC_STATE_INST_H_
#define MBMORE_SRC_STATE_INST_H_

#include "cyclus.h"

namespace mbmore {

typedef std::map<int, std::vector<std::string> > BuildSched;
  
/// @class StateInst
/// @section introduction Introduction
/// @section detailedBehavior Detailed Behavior
/// @warning The StateInst is experimental
class StateInst
    : public cyclus::Institution,
      public cyclus::toolkit::CommodityProducerManager,
      public cyclus::toolkit::Builder {
 public:
  /// Default constructor
  StateInst(cyclus::Context* ctx);

  /// Default destructor
  virtual ~StateInst();

  #pragma cyclus

  #pragma cyclus note {"doc": "An institution that owns and operates a " \
                              "manually entered list of facilities in " \
                              "the input file"}

  /// enter the simulation and register any children present
  virtual void EnterNotify();

  /// register a new child
  virtual void BuildNotify(Agent* m);

  /// unregister a child
  virtual void DecomNotify(Agent* m);

  /// deploy secret child facilities
  //  virtual void DeploySecret(cyclus::Agent* parent);
  virtual void DeploySecret();

  virtual void Tock();

  
  /// write information about a commodity producer to a stream
  /// @param producer the producer
  void WriteProducerInformation(cyclus::toolkit::CommodityProducer*
                                producer);

 private:
  /// register a child
  void Register_(cyclus::Agent* agent);

  /// unregister a child
  void Unregister_(cyclus::Agent* agent);

  #pragma cyclus var { \
    "tooltip": "Declared facility prototypes (at start of sim)",         \
    "uilabel": "Producer Prototype List",                               \
    "uitype": ["oneormore", "prototype"],                                    \
    "doc": "A set of facility prototypes that this institution can build. " \
    "All prototypes in this list must be based on an archetype that "   \
    "implements the cyclus::toolkit::CommodityProducer interface",      \
    }
  std::vector<std::string> declared_protos;

 protected:
  #pragma cyclus var { \
    "uilabel": "Secret prototypes to deploy", \
    "uitype": ("oneormore", "prototype"), \
    "doc": "Ordered list of secret prototypes to build."   \
           "These are built only if the Pursuit Equation == 1",   \
  }
  std::vector<std::string> secret_protos;


  }; // Toolkit::Builder
}  // namespace mbmore

#endif  // MBMORE_SRC_STATE_INST_H_
