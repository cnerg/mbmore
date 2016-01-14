#ifndef MBMORE_SRC_RANDOMSINK_H_
#define MBMORE_SRC_RANDOMSINK_H_

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "cyclus.h"
#include "behavior_functions.h"

namespace mbmore {

class Context;

/// This facility acts as a sink of materials and products with a fixed
/// throughput (per time step) capacity and a lifetime capacity defined by a
/// total inventory size.  The inventory size and throughput capacity both
/// default to infinite. If a recipe is provided, it will request material with
/// that recipe. Requests are made for any number of specified commodities.
class RandomSink : public cyclus::Facility  {
 public:
  RandomSink(cyclus::Context* ctx);

  virtual ~RandomSink();

  #pragma cyclus note { \
    "doc": \
    " A sink facility that accepts materials and products with a fixed\n"\
    " throughput (per time step) capacity and a lifetime capacity defined by\n"\
    " a total inventory size. The inventory size and throughput capacity\n"\
    " both default to infinite. If a recipe is provided, it will request\n"\
    " material with that recipe. Requests are made for any number of\n"\
    " specified commodities.\n" \
    }

  #pragma cyclus decl

  virtual std::string str();

  virtual void Tick();

  virtual void Tock();

  /// @brief RandomSinkFacilities request Materials of their given commodity. Note
  /// that it is assumed the RandomSink operates on a single resource type!
  virtual std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr>
      GetMatlRequests();

  /// @brief RandomSinkFacilities request Products of their given
  /// commodity. Note that it is assumed the RandomSink operates on a single
  /// resource type!
  virtual std::set<cyclus::RequestPortfolio<cyclus::Product>::Ptr>
      GetGenRsrcRequests();

  /// @brief Change preference for requests from this facility as
  /// defined by input (user_prefs state var)
  virtual void AdjustMatlPrefs(cyclus::PrefMap<cyclus::Material>::type& prefs);
 
  /// @brief RandomSinkFacilities place accepted trade Materials in their Inventory
  virtual void AcceptMatlTrades(
      const std::vector< std::pair<cyclus::Trade<cyclus::Material>,
      cyclus::Material::Ptr> >& responses);

  /// @brief RandomSinkFacilities place accepted trade Materials in their Inventory
  virtual void AcceptGenRsrcTrades(
      const std::vector< std::pair<cyclus::Trade<cyclus::Product>,
      cyclus::Product::Ptr> >& responses);

  ///  add a commodity to the set of input commodities
  ///  @param name the commodity name
  inline void AddCommodity(std::string name) { in_commods.push_back(name); }

  /// @return the current inventory storage size
  inline double InventorySize() const { return inventory.quantity(); }

  /// ***
  /// For use only in tests
  
  ///  sets the size of the storage inventory for received material
  ///  @param size the storage size
  inline void SetMaxInventorySize(double size) {
    max_inv_size = size;
    inventory.capacity(size);
  }

  /// @return the maximum inventory storage size
  inline double MaxInventorySize() const { return inventory.capacity(); }

  /// sets the capacity of a material generated at any given time step
  /// @param capacity the reception capacity
  inline void Capacity(double cap) { capacity = cap; }

  /// @return the reception capacity at any given time step
  inline double Capacity() const { return capacity; }

  /// determines the amount to request
  inline double RequestAmt() const {
    return std::min(capacity, std::max(0.0, inventory.space()));
  }
  
  /// End Use in Test only
  /// ***
  
  // Amount of material to be requested. Re-assessed at each timestep
  // in the Tick
  double amt ;

  // Recipe for the current timestep, if multiple recipes are available
  // (re-assessed in the Tick)
  cyclus::Composition::Ptr curr_recipe;
  
  /// @return the input commodities
  inline const std::vector<std::string>&
      input_commodities() const { return in_commods; }

 private:
  /// all facilities must have at least one input commodity
  #pragma cyclus var {"tooltip": "input commodities", \
                      "doc": "commodities that the sink facility accepts", \
                      "uilabel": "List of Input Commodities", \
                      "uitype": ["oneormore", "incommodity"]}
  std::vector<std::string> in_commods;

  // if using a single recipe
  #pragma cyclus var {"default": "", "tooltip": "requested composition",      \
                      "doc": "name of recipe to use for material requests, " \
                             "where the default (empty string) is to accept " \
                             "everything", \
                       "uilabel": "Input Recipe", \
                      "uitype": "recipe"}
  std::string recipe_name;

  // if using multiple recipes
  #pragma cyclus var {"default": [],"uitype": ["oneormore", "recipe"], \
    "uilabel": "Input Recipe List", \
    "doc": "Input recipes to request for different timesteps " \
            "(randomly chosen)", \
  }
  std::vector<std::string> recipe_names;
  
  //***
  #pragma cyclus var {"default": "None", "tooltip": "social behavior",	\
                          "doc": "type of social behavior used in trade " \
                                 "decisions: None, Every, Random, Reference " \
                                 "where behav_interval describes the " \
                                 "time interval for behavior action." \
                                 "Reference queries the RNG to mimic Random, "\
                                 "but returns zero material qty to trade."}
  std::string social_behav;

  #pragma cyclus var {"default": 0, "tooltip": "interval for behavior" ,\
                      "doc": "interval of social behavior: Every or "\
                             "EveryRandom.  If 0 then behavior is not " \
                             "implemented"}
  double behav_interval;

  #pragma cyclus var {"default": 1, "tooltip": "user-defined preference" , \
                      "doc": "change the default preference for requests "\
                             "from this agent"}
  int user_pref;

  #pragma cyclus var {"default": 0, "tooltip": "defines RNG seed",\
                        "doc": "seed on current system time if set to -1," \
                               " otherwise seed on number defined"}
  int rng_seed;

  #pragma cyclus var {"default": 1e299, "tooltip": "sink avg_qty",	\
                          "doc": "mean for the normal distribution that " \
                                 "is sampled to determine the amount of " \
                                 "material actually requested at each " \
                                 "time step"}
  double avg_qty;

  #pragma cyclus var {"default": 0, "tooltip": "standard deviation",	\
                          "doc": "standard deviation (FWHM) of the normal " \
                                 "distribution used to generate requested " \
                                 "amount of material (avg_qty)" }
  double sigma; 

  #pragma cyclus var {"default": 0,					\
                      "tooltip": "time to being allowing trades (starts at 0)",\
                          "doc": "At all timesteps before this value, the "   \
                                 "facility does make material requests. At " \
                                 "times at or beyond this value, requests are "\
                                 "made subject to the other behavioral " \
                                 "features available in this archetype"  }
  double t_trade;   //*** 

  /// max inventory size
  #pragma cyclus var {"default": 1e299, \
                      "tooltip": "sink maximum inventory size", \
                      "uilabel": "Maximum Inventory", \
                      "doc": "total maximum inventory size of sink facility"}
  double max_inv_size;

  /// monthly acceptance capacity
  #pragma cyclus var {"default": 1e299, "tooltip": "sink capacity", \
                      "uilabel": "Maximum Throughput", \
                      "doc": "capacity the sink facility can " \
                             "accept at each time step"}
  double capacity;

  /// this facility holds material in storage.
  #pragma cyclus var {'capacity': 'max_inv_size'}
  cyclus::toolkit::ResBuf<cyclus::Resource> inventory;
};

}  // namespace mbmore

#endif  // MBMORE_SRC_RANDOMSINK_H_
