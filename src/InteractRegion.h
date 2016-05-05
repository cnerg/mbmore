#ifndef MBMORE_SRC_INTERACT_REGION_H_
#define MBMORE_SRC_INTERACT_REGION_H_

#include <set>

#include "cyclus.h"

namespace mbmore {

/// @class Region
///
/// The Region class is the abstract class/interface used by all
/// region agents
///
/// This is all that is known externally about Regions

class InteractRegion
  : public cyclus::Region {
  friend class InteractRegionTests;
 public:
  /// Default constructor for InteractRegion Class
  InteractRegion(cyclus::Context* ctx);

  /// InteractRegions should not be indestructible.
  virtual ~InteractRegion();

  #pragma cyclus

  #pragma cyclus note {"doc": "A region that governs a scenario in which " \
                              "there is growth in demand for a commodity. "}
  
  virtual void Tick() {}

  virtual void Tock() {}


  /*
  // DO NOT call Agent class implementation of this method
  virtual void InfileToDb(InfileTree* qe, DbInit di) {}

  // DO NOT call Agent class implementation of this method
  virtual void InitFrom(QueryableBackend* b) {}

  // DO NOT call Agent class implementation of this method
  virtual void Snapshot(DbInit di) {}

  virtual void InitInv(Inventories& inv) {}

  virtual Inventories SnapshotInv() { return Inventories(); }

  virtual void Decommission();

  virtual void EnterNotify();

  /// register a child
  void Register_(cyclus::Agent* agent);

  */
  

  /// perform actions required when entering the simulation
  virtual void Build(cyclus::Agent* parent);

 

  /// every agent should be able to print a verbose description
  virtual std::string str();

 
  }; //cyclus::Region

}  // namespace mbmore

#endif  // MBMORE_SRC_INTERACT_REGION_H_
