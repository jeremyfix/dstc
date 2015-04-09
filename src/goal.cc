#include "goal.h"
#include "converter.h"
#include <sstream>
#include <algorithm>






belief_tracker::Goal::Goal(): slots_values() {
}   

belief_tracker::Goal::Goal(const std::initializer_list<Goal::value_type>& il)
{
  for (auto x: il)
    slots_values.push_back(x); 
}

bool belief_tracker::Goal::operator<(const Goal& other) const {

  if(slots_values.size() != other.slots_values.size()) {
    throw std::logic_error("Cannot compare goals with different size of slot/values");
  }

  return std::lexicographical_compare(slots_values.begin(), slots_values.end(),
				      other.slots_values.begin(), other.slots_values.end());
}

std::string belief_tracker::Goal::toStr() const {
  std::ostringstream ostr;
  ostr << "(";
  unsigned int slot_index = 0;
  for(auto& svi: slots_values) {
    std::string slot = Converter::get_slot(slot_index);
    std::string value = Converter::get_value(slot_index, svi);
    ostr << slot << "=" << value;

    if(slot_index != slots_values.size() - 1)
      ostr << ",";

    ++slot_index;
  }
  ostr << ")";
  return ostr.str();
}

std::map<std::string, std::string> belief_tracker::Goal::toStrVec() const {
  std::map<std::string, std::string> res;
  unsigned int slot_index = 0;
  for(auto& svi: slots_values) {
    std::string slot = Converter::get_slot(slot_index);
    std::string value = Converter::get_value(slot_index, svi);
    res[slot] = value;

    ++slot_index;
  }
  return res;
}

std::ostream & belief_tracker::operator<<(std::ostream &os, const belief_tracker::Goal &g) {
  os << "(";
  for(unsigned int i = 0 ; i < g.slots_values.size() ; ++i) {
    os << g.slots_values[i];
    if(i != g.slots_values.size() - 1)
      os << ", ";
  }
  os << ")";
  return os;
}

belief_tracker::Goal belief_tracker::extract_1best_from_tracker_session(const belief_tracker::TrackerSessionTurn& tracker_session, bool use_joint) {
  Goal g;
  unsigned int nb_informable = belief_tracker::Converter::get_informable_slots_size();

  g.slots_values.resize(nb_informable);
  for(unsigned int i = 0 ; i < nb_informable; ++i) 
    g.slots_values[i] = belief_tracker::Converter::get_value(i, belief_tracker::SYMBOL_UNKNOWN);
  
  if(use_joint) {

    // Use the joint labels
    double unknown_score = 1.;
    for(auto& g_v_score: tracker_session.goal_labels_joint_scores) 
      unknown_score -= g_v_score.second;


    double best_score = unknown_score;
    for(auto& g_v_score: tracker_session.goal_labels_joint_scores) {
      if(g_v_score.second > best_score) {
	best_score = g_v_score.second;
	// Reinitialize the goal to unknown everywhere
	for(unsigned int i = 0 ; i < g.slots_values.size(); ++i) 
	  g.slots_values[i] = belief_tracker::Converter::get_value(i, belief_tracker::SYMBOL_UNKNOWN);
	for(auto& kv: g_v_score.first) {
	  g.slots_values[belief_tracker::Converter::get_slot(kv.first)] = belief_tracker::Converter::get_value(kv.first, kv.second);
	}
      }
    }
  }
  else {
    for(auto& g_v_score: tracker_session.goal_labels_scores) { 
      std::string slot = g_v_score.first;
      unsigned int slot_index = belief_tracker::Converter::get_slot(slot);
      double unknown_score = 1.0;
      double best_score = -1.0;
      std::string best_value("");
      for(auto& values_score: g_v_score.second) {
	unknown_score -= values_score.second;
	if(values_score.second > best_score) {
	  best_value = values_score.first;
	  best_score = values_score.second;
	}
      }
      if(unknown_score > best_score) 
	g.slots_values[slot_index] = belief_tracker::Converter::get_value(slot_index, belief_tracker::SYMBOL_UNKNOWN);
      else
	g.slots_values[slot_index] = belief_tracker::Converter::get_value(slot_index, best_value);
    }
  }
  return g;
}

