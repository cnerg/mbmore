// Implements the Region class
#include "InteractRegion.h"
#include "behavior_functions.h"

#include <iostream>
#include <string>


namespace mbmore {

// Globally scoped list of columns for the database
  std::vector<std::string> InteractRegion::column_names;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InteractRegion::InteractRegion(cyclus::Context* ctx)
  : cyclus::Region(ctx) {
    //  kind_ = "InteractRegion";
  cyclus::Warn<cyclus::EXPERIMENTAL_WARNING>("the InteractRegion agent is experimental.");

}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  InteractRegion::~InteractRegion() {}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void InteractRegion::Build(cyclus::Agent* parent) {
    cyclus::Agent::Build(parent);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::map<std::string, double>
  InteractRegion::GetWeights(std::string eqn_type) {
    return wts;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int InteractRegion::GetNStates() {
  int n_states = 0;
  for (std::set<Agent*>::const_iterator inst = children().begin();
       inst != children().end();
       inst++) {
    if ((*inst)->spec() == ":mbmore:StateInst") {
      n_states++; 
    }
  }
  return n_states;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InteractRegion::Tick() {

  // Things to do only at beginning of Simulation
  if (context()->time() == 0){
    
    // Create conflict score map
    BuildScoreMatrix();
    
    // Define main List of column names for the database only once.
    std::string main_factors [] = { "Auth", "Conflict", "Enrich",
				      "Mil_Iso","Mil_Sp","Reactors",
				      "Sci_Net", "U_Reserve"};
    int n_factors = sizeof(main_factors) / sizeof(main_factors[0]);
    
    if (column_names.size() == 0){
      for(int f_it = 0; f_it < n_factors; f_it++) {
	column_names.push_back(main_factors[f_it]);
      }
    }
    
    // Determine which factors are used in the simulation based on the defined
    // weights.
    p_present = DefinedFactors("Pursuit");
    
    // Check weights to make sure they add to one, otherwise normalize
    double tot_weight = 0.0;
    std::map <std::string, double>::iterator wt_it;
    for(wt_it = wts.begin(); wt_it != wts.end(); wt_it++) {
      tot_weight+= wt_it->second;
    }
    if (tot_weight == 0){
      cyclus::Warn<cyclus::VALUE_WARNING>("Weights must be defined!");
    }
    else if (tot_weight != 1.0) {
      for(wt_it = wts.begin(); wt_it != wts.end(); wt_it++) {
	wt_it->second = wt_it->second/tot_weight;
      }
    }
    
    // If conflict is defined, record initial conflict relations in database
    int n_states = GetNStates();
    if ((p_present["Conflict"] == true) && n_states > 1){
      std::string eqn_type = "Pursuit";
      std::map<std::pair<std::string, std::string>,int>::iterator it;
      for (it = p_conflict_map.begin(); it != p_conflict_map.end(); ++it) {
	RecordConflictReln(eqn_type, it->first.first,
			   it->first.second, it->second);
      }
    }
  }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Determines which factors are defined for this sim
std::map<std::string, bool>
  InteractRegion::DefinedFactors(std::string eqn_type) {

  std::map<std::string, bool> present;
  std::map<std::string,double>::iterator factor_it;
  int n_factors = column_names.size();
  for(int i = 0; i < n_factors; i++) {
    factor_it = wts.find(column_names[i]);
    if (factor_it == wts.end()) {   // factor isn't defined in input file
      present[column_names[i]]= false;
    }
    else {
      present[column_names[i]]= true;
    }
  }
  return present;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Returns a map of regularly used factors and bool to indicate whether they are
// defined in this sim.
std::vector<std::string>& InteractRegion::GetMainFactors() {
  return column_names;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Determine the likelihood value for the equation at the current time,
// (where the current value of the equation is normalized to be between 0-1)
double InteractRegion::GetLikely(std::string phase, double eqn_val) {

  double hist_duration = 75; // historical data covers 70 years

  std::pair<std::string, std::vector<double> > likely_pair =
    likely_rescale[phase];
  std::string function = likely_pair.first;
  std::vector<double> constants = likely_pair.second;

  double phase_likely;
  if (phase == "Pursuit"){
    double integ_likely;
    // historical data defines the likelihood integrated over 70yrs
    if ((function == "Power") || (function == "power")){
      integ_likely = CalcYVal(function, constants, eqn_val/10.0);
    }
    else {
      integ_likely = CalcYVal(function, constants, eqn_val);
    }
    phase_likely = ProbPerTime(integ_likely, hist_duration);
  }
  else {
    // for acquire, determine the avg time (N_years) to weapon based on score,
    // then convert to a likelihood per timestep 1/(N_years)
    // TODO: CHANGE HARDCODING TO CHECK FOR ARBITRARY TIMESTEP DURATION
    //       (currently assumes timestep is one year)
    double avg_time = CalcYVal(function, constants, eqn_val);
    phase_likely = 1.0/avg_time;
  }

  if ((phase_likely < 0) || (phase_likely > 1)){
    std::stringstream ss;
    ss << "likelihood of weapon decision isnt between 0-1!"
       << "Something went wrong!";
    cyclus::Warn<cyclus::VALUE_WARNING>(ss.str());
  }
  return phase_likely;
  
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Determine the Conflict or Military Isolation actor for the state at each
// timestep. Begin with the map of relations to all other states, sum these
// values and normalize. Then convert result to a 0-10 scale (0 == alliance,
// 5 == neutral, 10 == conflict)
  
double InteractRegion::GetConflictScore(std::string eqn_type,
					std::string prototype) {
  int gross_score = 0;
  int n_entries = 0;

  std::map<std::pair<std::string, std::string>,int>::iterator map_it;
  for (map_it = p_conflict_map.begin();
       map_it != p_conflict_map.end(); ++map_it){
    std::string curr_state = map_it->first.first;
    if (curr_state == prototype){
      n_entries+=1;
      std::string other_state = map_it->first.second;

      // allies, neutral, or enemies
      int this_relation = map_it->second;
      
      // what is each state's NW status?
      int my_weapon_status = sim_weapon_status[prototype];
      int other_weapon_status = sim_weapon_status[other_state];
      
      std::string relation_string;
      // order smallest number first (0_2 instead of 2_0)
      if (my_weapon_status > other_weapon_status){
	relation_string = BuildRelationString(other_weapon_status,
					      my_weapon_status,
					      this_relation);
      }
      else {
	relation_string = BuildRelationString(my_weapon_status,
					      other_weapon_status,
					      this_relation);
      }
      gross_score += score_matrix[relation_string];
    }
  }
  if (n_entries == 0){
    std::stringstream ss;
    ss << "State " << prototype
       << " is not defined in the p_conflict_relations";
    throw cyclus::ValueError(ss.str());
  }

  // Take all conflict relationships for a single state and average them
  // together to get final conflict score
  // Example: if A-B = 2, A-C = 6, A-D = 10, then total conflict for A = 6
  double avg_score = static_cast<double>(gross_score)/n_entries;
  return avg_score;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Make a string that contains the weapons status of states A and state B, as
// well as their relationship as statusA_statusB_relationship
// String is used as key for map of conflict scores
// Status 0 - Non Weapon State, 2 - Pursuing, 3 - Acquired
std::string InteractRegion::BuildRelationString(int statusA,
						  int statusB,
						  int relation){

    std::string relation_string;
    std::string full_string;
    if (relation == 1){
      relation_string = "ally_";
    }
    else if (relation == 0){
      relation_string = "neut_";
    }
    else {
      relation_string = "enemy_";
    }

    // Any weapons status that is not 0 (never pursued), 2 (pursue), 3 (acquire)
    // is redefined as never pursued
    if ((statusA != 0) && (statusA != 2) && (statusA !=3)){
      statusA = 0;
    }
    if ((statusB != 0) && (statusB != 2) && (statusB !=3)){
      statusB = 0;
    }
    
    full_string.append(relation_string);
    full_string.append(std::to_string(statusA));
    full_string.append("_");
    full_string.append(std::to_string(statusB));

    return full_string;
  }
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Change the Conflict value for a state. If the simulation is symmetric,
// then the change in conflict value is mutual between the two states. Otherwise
// only the state whose change was initiated is affected, such that the two
// states may have different perspectives on their relationship.
void InteractRegion::ChangeConflictReln(std::string eqn_type,
					  std::string this_state,
					  std::string other_state, int new_val){

  p_conflict_map[std::pair<std::string, std::string>
		 (this_state, other_state)] = new_val;
  RecordConflictReln(eqn_type, this_state, other_state, new_val);
  if (symmetric == 1){
    p_conflict_map[std::pair<std::string, std::string>
		   (other_state, this_state)] = new_val;
    RecordConflictReln(eqn_type, other_state, this_state, new_val);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Change the weapon status for a state.
// 0 = not pursuing, 2 = pursuing, 3 = acquired
// (Reserved but not implemented: -1 = gave up weapons program, 1 = exploring)
void InteractRegion::UpdateWeaponStatus(std::string proto,
					int new_weapon_status){

  // add new key value pair. If key already exists then just update value
  std::pair<std::map<std::string, int>::iterator, bool> ret;
  ret = sim_weapon_status.insert(std::pair<std::string, int>
				 (proto, new_weapon_status));
  if (ret.second ==false){
    sim_weapon_status[proto] = new_weapon_status;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Record conflict factors for each pair at start of simulation and
// whenever they are changed 
void InteractRegion::RecordConflictReln(std::string eqn_type,
					std::string this_state,
					std::string other_state, int new_val){
  using cyclus::Context;
  using cyclus::Recorder;
  
  cyclus::Datum *d = context()->NewDatum("InteractRelations");
  d->AddVal("Time", context()->time());
  d->AddVal("PrimaryAgent", this_state);
  d->AddVal("SecondaryAgent", other_state);
  d->AddVal("Conflict", new_val);
  d->Record();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Build Score Matrix
  void InteractRegion::BuildScoreMatrix(){
    score_matrix.insert(std::pair<std::string, int>("ally_0_0", 2));
    score_matrix.insert(std::pair<std::string, int>("neut_0_0", 2));
    score_matrix.insert(std::pair<std::string, int>("enemy_0_0", 6));
    
    score_matrix.insert(std::pair<std::string, int>("ally_0_2", 3));
    score_matrix.insert(std::pair<std::string, int>("neut_0_2", 4));
    score_matrix.insert(std::pair<std::string, int>("enemy_0_2", 8));
    
    score_matrix.insert(std::pair<std::string, int>("ally_0_3", 1));
    score_matrix.insert(std::pair<std::string, int>("neut_0_3", 4));
    score_matrix.insert(std::pair<std::string, int>("enemy_0_3", 6));
    
    score_matrix.insert(std::pair<std::string, int>("ally_2_2", 3));
    score_matrix.insert(std::pair<std::string, int>("neut_2_2", 4));
    score_matrix.insert(std::pair<std::string, int>("enemy_2_2", 9));
    
    score_matrix.insert(std::pair<std::string, int>("ally_2_3", 3));
    score_matrix.insert(std::pair<std::string, int>("neut_2_3", 5));
    score_matrix.insert(std::pair<std::string, int>("enemy_2_3", 10));
    
    score_matrix.insert(std::pair<std::string, int>("ally_3_3", 1));
    score_matrix.insert(std::pair<std::string, int>("neut_3_3", 3));
    score_matrix.insert(std::pair<std::string, int>("enemy_3_3", 5));
  }
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  std::string InteractRegion::str() {
  std::string s = cyclus::Agent::str();

  s += " has insts: ";
  for (std::set<Agent*>::const_iterator inst = children().begin();
       inst != children().end();
       inst++) {
    s += (*inst)->prototype() + ", ";
  }
  return s;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
extern "C" cyclus::Agent* ConstructInteractRegion(cyclus::Context* ctx) {
  return new InteractRegion(ctx);
}
  
}  // namespace mbmore
