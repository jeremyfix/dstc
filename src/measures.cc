#include "measures.h"
#include "converter.h"

int belief_tracker::nb_differences_goal_to_label(const belief_tracker::Goal& goal, std::map< std::string, std::string > labels) {
  int d = 0;

  for(unsigned int i = 0 ; i < goal.slots_values.size() ; ++i) {
    std::string slot = Converter::get_slot(i);
    std::string value = Converter::get_value(i, goal.slots_values[i]);
    auto iter_on_labels = labels.find(slot);
    if(iter_on_labels != labels.end()) {
      int di = (iter_on_labels->second != value);
      d += di;
    }
    else {
      d += value != belief_tracker::SYMBOL_UNKNOWN;
    }
  }
  return d;
}

/*
int belief_tracker::nb_differences_beliefbest_to_label(const belief_tracker::Belief& belief, std::map< std::string, std::string > labels) {
  // Extract the best goal from the belief
  belief_tracker::Goal best_goal = belief_tracker::extract_best_goal_from_belief(belief);
  return nb_differences_goal_to_label(best_goal, labels);
}
*/
