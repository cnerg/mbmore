
#line 1 "/Users/mbmcgarry/git/mbmore/src/mytest.h"
#ifndef CYCLUS_MBMORE_MYTEST_H_
#define CYCLUS_MBMORE_MYTEST_H_

#include <string>

#include "cyclus.h"

namespace mbmore {

/// @class mytest
///
/// This Facility is intended
/// as a skeleton to guide the implementation of new Facility
/// agents.
/// The mytest class inherits from the Facility class and is
/// dynamically loaded by the Agent class when requested.
///
/// @section intro Introduction
/// Place an introduction to the agent here.
///
/// @section agentparams Agent Parameters
/// Place a description of the required input parameters which define the
/// agent implementation.
///
/// @section optionalparams Optional Parameters
/// Place a description of the optional input parameters to define the
/// agent implementation.
///
/// @section detailed Detailed Behavior
/// Place a description of the detailed behavior of the agent. Consider
/// describing the behavior at the tick and tock as well as the behavior
/// upon sending and receiving materials and messages.
class mytest : public cyclus::Facility  {
 public:
  /// Constructor for mytest Class
  /// @param ctx the cyclus context for access to simulation-wide parameters
  explicit mytest(cyclus::Context* ctx);

  /// The Prime Directive
  /// Generates code that handles all input file reading and restart operations
  /// (e.g., reading from the database, instantiating a new object, etc.).
  /// @warning The Prime Directive must have a space before it! (A fix will be
  /// in 2.0 ^TM)

  virtual void InitFrom(mbmore::mytest* m) {
    cyclus::Facility::InitFrom(m);
  };
#line 46 "/Users/mbmcgarry/git/mbmore/src/mytest.h"

  virtual void InitFrom(cyclus::QueryableBackend* b) {
    cyclus::Facility::InitFrom(b);
    cyclus::QueryResult qr = b->Query("Info", NULL);
  };

  virtual void InfileToDb(cyclus::InfileTree* tree, cyclus::DbInit di) {
    cyclus::Facility::InfileToDb(tree, di);
    cyclus::InfileTree* sub = tree->SubTree("config/*");
    int i;
    int n;
    di.NewDatum("Info")
    ->Record();
  };

  virtual cyclus::Agent* Clone() {
    mbmore::mytest* m = new mbmore::mytest(context());
    m->InitFrom(this);
    return m;
  };

  virtual std::string schema() {
    return "<text/>";
  };

  virtual Json::Value annotations() {
    Json::Value root;
    Json::Reader reader;
    bool parsed_ok = reader.parse(
      "{\"name\":\"mbmore::mytest\",\"entity\":\"facility\",\"pare"
      "nts\":[\"cyclus::Facility\"],\"all_parents\":[\"cyclus::"
      "Agent\",\"cyclus::Facility\",\"cyclus::Ider\",\"cyclus::"
      "StateWrangler\",\"cyclus::TimeListener\",\"cyclus::Tra"
      "der\"],\"vars\":{},\"doc\":\"A stub facility is provided"
      " as a skeleton for the design of new facility "
      "agents.\"}", root);
    if (!parsed_ok) {
      throw cyclus::ValueError("failed to parse annotations for mbmore::mytest.");
    }
    return root;
  };

  virtual void InitInv(cyclus::Inventories& inv) {
  };

  virtual cyclus::Inventories SnapshotInv() {
    cyclus::Inventories invs;
    return invs;
  };

  virtual void Snapshot(cyclus::DbInit di) {
    di.NewDatum("Info")
    ->Record();
  };

  #pragma cyclus note {"doc": "A stub facility is provided as a skeleton "                               "for the design of new facility agents."}
#line 49 "/Users/mbmcgarry/git/mbmore/src/mytest.h"

  /// A verbose printer for the mytest
  virtual std::string str();

  /// The handleTick function specific to the mytest.
  /// @param time the time of the tick
  virtual void Tick();

  /// The handleTick function specific to the mytest.
  /// @param time the time of the tock
  virtual void Tock();

  // And away we go!
};

}  // namespace mbmore

#endif  // CYCLUS_MBMORE_MYTEST_H_