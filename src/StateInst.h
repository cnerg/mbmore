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

  #pragma cyclus note {"doc": "An institution that owns and operates a" \
                              " manually entered list of facilities in " \
                              "the input file. Assesses likelihood of pursuing"\
                              " nuclear weapons through internal motivational "\
                              "(military spending, nuclear infrastructure etc)"\
                              " as well as external factors such as conflict" \
                              " with neighbors. Must be paired with" \
                              " InteractRegion"}

  /// enter the simulation and register any children present
  virtual void EnterNotify();

  /// register a new child
  virtual void BuildNotify(Agent* m);

  /// unregister a child
  virtual void DecomNotify(Agent* m);

  /// deploy secret child facilities
  //  virtual void DeploySecret(cyclus::Agent* parent);
  virtual void DeploySecret();


  // Do calculation of pursuit equation and convert to a Y/N on whether to
  // start pursuing a weapon at each timestep.
  bool DecidePursuit();

  virtual void Tick();

  virtual void Tock();

  // Adjusts preferences so SecretSink cannot trade until acquired=1
  virtual void AdjustMatlPrefs(cyclus::PrefMap<cyclus::Material>::type& prefs);

  /// write information about a commodity producer to a stream
  /// @param producer the producer
  void WriteProducerInformation(cyclus::toolkit::CommodityProducer*
                                producer);

  bool pursuing = 0;
  bool acquired = 0;

 protected:
  /// register a child
  void Register_(cyclus::Agent* agent);

  /// unregister a child
  void Unregister_(cyclus::Agent* agent);

  // Find the simulation duration
  //  cyclus::SimInfo si_;
  int simdur = context()->sim_info().duration;
  
  #pragma cyclus var { \
    "tooltip": "Declared facility prototypes (at start of sim)",         \
    "uilabel": "Producer Prototype List",                               \
    "uitype": ["oneormore", "prototype"],                                    \
    "doc": "A set of facility prototypes that this institution can build. " \
           "All prototypes in this list must be based on an archetype that "   \
           "implements the cyclus::toolkit::CommodityProducer interface",      \
    }
  std::vector<std::string> declared_protos;

  #pragma cyclus var { \
    "uilabel": "Secret prototypes to deploy", \
    "uitype": ("oneormore", "prototype"), \
    "doc": "Ordered list of secret prototypes to build."   \
           "These are built only if the Pursuit Equation == 1",   \
  }
  std::vector<std::string> secret_protos;

  #pragma cyclus var { \
    "default": 0,\
    "tooltip": "Seed for RNG" ,				    \
    "doc": "seed on current system time if set to -1," \
           " otherwise seed on number defined"}
  int rng_seed;


  #pragma cyclus var { \
    "alias": ["pursuit_factors", "factor", ["function","name", ["params","val"]]], \
    "doc": "Pursuit Factors,  All factors must be between 0 and 10. "  \
           " Each pursuit factor has an associated function (describing time"\
           " dynamics) and constants. For example, if democracy index is" \
           " linearly increasing with a y-intercept of 2 and a slope of 0.5," \
           " then it looks like P_f[\"Dem\"]= (\"linear\", [y-int, slope])" \
           " Available functions are linear (y-int, slope), constant (y-int), "\
           " step (y-int, y_end, t_step). For Random events, use the Step" \
           " function. If t_step is not defined, then the time"	      \
           " is randomly defined at the beginning of the simulation.  The" \
           " required Factors are: Dem (Democracy Index), React (# reactors).",\
  }
  std::map<std::string, std::pair<std::string, std::vector<double> > > P_f ;

  // Defines persistent columne names in WeaponProgress table of database
  static std::vector<std::string> column_names;

   }; // Toolkit::Builder
}  // namespace mbmore

#endif  // MBMORE_SRC_STATE_INST_H_
