#include "belief.h"
#include "goal.h"
#include "converter.h"
#include <sstream>
#include <iterator>
#include <set>
#include <math.h>
#include <algorithm>
#include <stdexcept>

void belief_tracker::requested_slots::update(const belief_tracker::SluHyps &user_acts, 
					     const belief_tracker::Uterance& macts,
					     std::map< std::string, double >& requested_slots) {
  // requested_slots is a lazy representation of the requested slots
  // only the non null probabilities are represented

  // We clean up requested_slots by removing all the requestable slots that have been informed by the system
  for(auto& mact_i: macts) {
    if(mact_i.act == std::string("inform")) {
      std::string slot_name = mact_i.act_slots.begin()->first;
      auto iter = requested_slots.find(slot_name);
      if(iter != requested_slots.end())
	requested_slots.erase(iter);
    }
  }


  // We update the probability that a slot has been requested;
  // We first marginalize the scores of the request(slot=s) from the user_acts
  std::map< std::string, double> marg_req_slots;
  for(auto& uact_i: user_acts) {
    double score = uact_i.second;
    std::set<std::string> slots_for_this_utt;
    for(auto& uact_i_j: uact_i.first) {
      if(uact_i_j.act == std::string("request")) {
	// We have a request(slot=s)
	std::string slot_name = uact_i_j.act_slots.begin()->second;
	if(marg_req_slots.find(slot_name) == marg_req_slots.end())
	  marg_req_slots[slot_name] = 0.0;

	// If the slot was not yet considered for this hypothesis 
	// it can be, for the SJTU for example) that request(slot=xx) appears several
	// times in the same utterance
	if(slots_for_this_utt.find(slot_name) == slots_for_this_utt.end()) {
	  marg_req_slots[slot_name] += score;
	  slots_for_this_utt.insert(slot_name);
	}
      }
    }
  }

  // The probability P_t(s) of requesting the slot s can be computed recursively as
  // P_t(s) = P_{t-1}(s) + (1 - P_{t-1}) P(request_t(s))
  for(auto& slot_preq: marg_req_slots) {
    std::string slot_name = slot_preq.first;
    double score = slot_preq.second;
    auto iter = requested_slots.find(slot_name);
    if(iter == requested_slots.end())  // then P_{t-1}(slot_name) = 0
      requested_slots[slot_name] = score;
    else
      iter->second = iter->second + (1.0 - iter->second) * score;
    if(std::isgreater(requested_slots[slot_name], 1.0+std::numeric_limits<double>::epsilon())) {
      std::cout << "Exception for slot " << slot_name << " : " << requested_slots[slot_name] << std::endl;
      for(auto& sp : marg_req_slots) {
	std::cout << sp.first << " " << sp.second << std::endl;
      }
      throw std::exception();

    }
  }
    
}

std::string belief_tracker::requested_slots::toStr(const std::map< std::string, double >& requested_slots) {
  std::ostringstream ostr;
  ostr.str("");
  ostr << "{ ";
  for(auto& rs: requested_slots) 
    ostr << rs.first << "(" << rs.second << ") ";
  ostr << "}";
  return ostr.str();
}

void belief_tracker::methods::update(const belief_tracker::SluHyps &user_acts, 
				     const belief_tracker::Uterance& macts,
				    std::map< std::string, double >& methods) {

  // what do we do for :  reqalts() inform(name=toto) inform(food=french) bye()  ???

  // We deal it with a (ARBITRARY!) hierarchy of method parsing
  // If we have a reqalts : all the mass is given the byalternatives
  // otherwise the mass is uniformely split among the candidate methods

  double palternatives = 0.0;
  double pconstraints = 0.0;
  double pname = 0.0;
  double pfinished = 0.0;
  double punknown = 0.0;

  bool has_reqalts;
  bool has_inform_not_name;
  bool has_inform_name;
  bool has_bye;

  for(auto& uact_i: user_acts) {
    double score = uact_i.second;
    has_reqalts = false;
    has_inform_not_name = false;
    has_inform_name = false;
    has_bye = false;

    for(auto& uact_i_j: uact_i.first) {
      has_reqalts = has_reqalts || (uact_i_j.act == std::string("reqalts"));
      has_inform_not_name = has_inform_not_name || ((uact_i_j.act == std::string("inform")) && uact_i_j.act_slots.begin()->first != std::string("name"));      
      has_inform_name = has_inform_name || ((uact_i_j.act == std::string("inform")) && uact_i_j.act_slots.begin()->first == std::string("name"));
      has_bye = has_bye || (uact_i_j.act == std::string("bye"));
    }

    // We now decide how to allocate the mass

    double dpalternatives = 0.0;
    double dpconstraints = 0.0;
    double dpname = 0.0;
    double dpfinished = 0.0;
    double dpunknown = 0.0;

    /*
    if(has_reqalts)
      dpalternatives += score;
    else {
      double n = (has_inform_not_name ? 1.0 : 0.0) + (has_inform_name ? 1.0 : 0.0) + (has_bye ? 1.0 : 0.0);
      
      if(n != 0) {
	dpconstraints += score * (has_inform_not_name ? 1.0 : 0.0)/n;
	dpname += score * (has_inform_name ? 1.0 : 0.0)/n;
	dpfinished += score * (has_bye ? 1.0 : 0.0)/n;
      }
    }
    dpunknown = score - (dpalternatives + dpconstraints + dpname + dpfinished);
    */
    double n = (has_reqalts ? 1.0 : 0.0) + (has_inform_not_name ? 1.0 : 0.0) + (has_inform_name ? 1.0 : 0.0) + (has_bye ? 1.0 : 0.0);
    
    if(n != 0) {
      dpalternatives += score * (has_reqalts ? 1.0 : 0.0)/n;
      dpconstraints += score * (has_inform_not_name ? 1.0 : 0.0)/n;
      dpname += score * (has_inform_name ? 1.0 : 0.0)/n;
      dpfinished += score * (has_bye ? 1.0 : 0.0)/n;
    }
    dpunknown = score - (dpalternatives + dpconstraints + dpname + dpfinished);


    palternatives += dpalternatives;
    pconstraints += dpconstraints;
    pname += dpname;
    pfinished += dpfinished;
    punknown += dpunknown;
  }
  // We can now update the methods
  /*
        "none", 
        "byconstraints", 
        "byname", 
        "finished", 
        "byalternatives"
  */
  methods["byalternatives"] = punknown * methods["byalternatives"] + palternatives;
  methods["byconstraints"] = punknown * methods["byconstraints"] + pconstraints;
  methods["byname"] = punknown * methods["byname"] + pname;
  methods["finished"] = punknown * methods["finished"] + pfinished;
  methods["none"] = punknown * methods["none"];

}

std::string belief_tracker::methods::toStr(const std::map< std::string, double >& methods) {
  std::ostringstream ostr;
  ostr.str("");
  for(auto& ms: methods) 
    ostr << ms.first << "(" << ms.second << ") ";
  return ostr.str();
}


belief_tracker::Belief::Belief() {}
belief_tracker::Belief::~Belief() {}




belief_tracker::joint::Belief::Belief() : belief_tracker::Belief() {}

belief_tracker::joint::Belief::~Belief() {}


belief_tracker::joint::Belief::Belief(const belief_tracker::joint::Belief& other): belief(other.belief) {}
belief_tracker::joint::Belief& belief_tracker::joint::Belief::operator=(const belief_tracker::joint::Belief& other) {
  if(this != &other) 
    belief = other.belief;
  return *this;
}

belief_tracker::Goal belief_tracker::joint::Belief::extract_best_goal() const {

  auto max_it = std::max_element(belief.begin(), belief.end(), [](const belief_type::value_type& key_val1, const belief_type::value_type& key_val2) { return key_val1.second < key_val2.second; });
  return max_it->first;
}

void belief_tracker::joint::Belief::start(void) {

  belief.clear();

  belief_tracker::Goal ginit;

  auto out = std::back_inserter(ginit.slots_values);
  for(unsigned int i = 0 ; i < belief_tracker::Converter::get_informable_slots_size(); ++i)
    *(out++) = belief_tracker::Converter::get_value(i, belief_tracker::SYMBOL_UNKNOWN);

  belief[ginit] = 1.0;
}


void belief_tracker::joint::Belief::fill_tracker_session(belief_tracker::TrackerSessionTurn& tracker_session_turn) {

  // Compute the probabilities over the single goals 
  // by marginalizing the joint goals

  // For each slot, we collect the values that appear in the belief
  // - Init
  std::map<unsigned int, std::set<unsigned int> > slot_values_in_belief;
  for(unsigned int i = 0 ; i < belief_tracker::Converter::get_informable_slots_size(); ++i)
    slot_values_in_belief[i] = std::set<unsigned int>();

  // - Fill
  for(auto& g_score: belief) {
    const belief_tracker::Goal& goal = g_score.first;
    for(unsigned int i = 0 ; i < goal.slots_values.size() ; ++i)
	slot_values_in_belief[i].insert(goal.slots_values[i]);
  }
  

  // We now set the initial probabilities for all the slots in tracker_output
  for(auto& kv: slot_values_in_belief) {
    unsigned int slot_index = kv.first;
    std::string slot = belief_tracker::Converter::get_slot(slot_index);
    for(unsigned int value_index: kv.second) {
      std::string value = belief_tracker::Converter::get_value(slot_index, value_index);
      tracker_session_turn.goal_labels_scores[slot][value] = 0.0;
    }
  }

  // And compute the marginal
  for(auto& kv: slot_values_in_belief) {
    unsigned int slot_index = kv.first;
    std::string slot = belief_tracker::Converter::get_slot(slot_index);

    for(unsigned int value_index: kv.second) {
      std::string value = belief_tracker::Converter::get_value(slot_index, value_index);
      // We go through the belief and sum the scores for all 
      // the belief where the slot=value appears;
      for(auto& g_score: belief) {
	double score = g_score.second;
	belief_tracker::Goal g = g_score.first;

	if(g.slots_values[slot_index] == value_index)
	  tracker_session_turn.goal_labels_scores[slot][value] += score;
      }
    }
  }

  // Fill in the joint goals
  for(auto& g_score: belief) {
    double score = g_score.second;
    std::map<std::string, std::string> g_to_str_vec =  g_score.first.toStrVec();
    tracker_session_turn.goal_labels_joint_scores.push_back(std::make_pair(g_to_str_vec, score)); 
  }
}


double belief_tracker::joint::Belief::marginalize(std::string slot, std::string value) const {
  unsigned int slot_index = belief_tracker::Converter::get_slot(slot);
  unsigned int value_index = belief_tracker::Converter::get_value(slot, value);
  double score = 0.0;
  for(auto& gs: belief) {
    if(gs.first.slots_values[slot_index] == value_index)
      score += gs.second;
  }
  return score;
}

void belief_tracker::joint::Belief::clean(double thr) {
  bool all_below_thr = true;
  for(auto& b: belief) {
    all_below_thr = (b.second > thr);
    if(!all_below_thr)
      break;
  }

  if(!all_below_thr) {
    auto iter = belief.begin();
    auto iter_end = belief.end();
    double sum = 0.0;
    while(iter != iter_end) {
      if(iter->second <= thr) 
	iter = belief.erase(iter);
      else {
	sum += iter->second;
	++iter;
      }
    }
    
    for(auto& b: belief)
      b.second /= sum;
    
  }
}


belief_tracker::joint::sum::Belief::Belief() : belief_tracker::joint::Belief::Belief(){}
belief_tracker::joint::sum::Belief::~Belief(){}

belief_tracker::joint::max::Belief::Belief() : belief_tracker::joint::Belief::Belief(){}
belief_tracker::joint::max::Belief::~Belief(){}












// belief_tracker::Goal belief_tracker::marginal::extract_best_goal(const belief_tracker::marginal::Belief& b) {

//   belief_tracker::Goal g;
//   g.slots_values.resize(belief_tracker::Converter::get_informable_slots_size());

//   for(unsigned int i = 0 ; i < b.size(); ++i) {
//     // Compute the entropy
//     double H = 0.0;
//     auto& distrib = b[i];
//     auto& distrib_singular = distrib.first;
//     auto& distrib_other = distrib.second;

//     for(auto& kv: distrib_singular) 
//       if(kv.second != 0.0)
// 	H += -kv.second * log(kv.second);
//     if(distrib_other.first != 0.0) {
//       double p = distrib_other.first;
//       int N = distrib_other.second;
//       if(N != 0)
// 	H += -p * log(p/N); // p is the mass uniformely distributed over the N other values
//     }

//     // nb of values in the ontology + dontcare
//     int Nvalues_for_sloti = belief_tracker::Converter::get_value_set_size_from_slot(i) + 1;

//     double p_unknown = H / log(double(Nvalues_for_sloti));
//     double alpha = 1.0 - H / log(double(Nvalues_for_sloti));

//     // We look for the value maximizing the probability for this slot
//     int pmax_value_index = belief_tracker::Converter::get_value_index_from_name(i, belief_tracker::SYMBOL_UNKNOWN);
//     double pmax = p_unknown;
//     for(auto& kv: distrib_singular) {
//       double p = kv.second * alpha;
//       if(p > pmax) {
// 	pmax = p;
// 	pmax_value_index = kv.first;
//       }
//     }
//     g.slots_values[i] = pmax_value_index;    
//   }


//   return g;
// }


// belief_tracker::marginal::Belief belief_tracker::marginal::init() {
//   belief_tracker::marginal::Belief belief;
//   for(unsigned int i = 0 ; i < belief_tracker::Converter::get_informable_slots_size(); ++i) 
//     belief.push_back(std::make_pair(std::map<Goal::value_type, double>(), 
// 				    std::make_pair(1.0, belief_tracker::Converter::get_value_set_size_from_slot(i)+1)));
  
//   return belief;
// }


// belief_tracker::marginal::Belief belief_tracker::marginal::update(const belief_tracker::ScoredInfo& turn_info, 
// 								  const belief_tracker::marginal::Belief& belief_t,
// 								  std::function<belief_tracker::Goal(const belief_tracker::Goal&, const belief_tracker::Info&)>) {

//   belief_tracker::marginal::Belief belief_tp1;
//   belief_tp1.resize(belief_t.size());
  

//   for(unsigned int i = 0 ; i < belief_tp1.size(); ++i) {
//     // Whether the info for the slot i   is positive or negative,
//     // we need to singularize that value, i.e. excluding it from the mass of the "others"

//     // nb values in ontology + 1
//     int N_values_for_sloti = belief_tracker::Converter::get_value_set_size_from_slot(i) + 1;

//     auto& distrib_t = belief_t.at(i);
//     auto& distrib_t_singular = distrib_t.first;
//     auto& distrib_t_other = distrib_t.second;

//     auto& distrib_tp1 = belief_tp1.at(i);
//     auto& distrib_tp1_singular = distrib_tp1.first;
//     auto& distrib_tp1_other = distrib_tp1.second;


//     // We first begin by defining the entries in the map for the slot i
//     // by taking all the values mentioned in the previous belief
//     for(auto& kv_t: distrib_t_singular)
//       distrib_tp1_singular[kv_t.first] = 0.0;
//     // and the values mentioned in the informations, without the unknown
//     for(auto& kv_info: turn_info) {
//       auto info_i = kv_info.first[i];
//       std::string value_name = belief_tracker::Converter::get_value_name_from_index(i, info_i);
//       if(value_name != belief_tracker::SYMBOL_UNKNOWN) {
// 	if(value_name.substr(0,1) == belief_tracker::SYMBOL_NOT)
// 	  value_name = value_name.substr(1);
// 	distrib_tp1_singular[belief_tracker::Converter::get_value_index_from_name(i, value_name)] = 0.0;
//       }
//     }

//     // We now compute the new probabilities as :
//     // P(v)_{t+1} = P(v)_t (Pi(?) + Pi(v) + 1/(N-1)sum_w!=v Pi(!w)) + (1 - P(v)_t)(Pi(v) + 1/(N-1)sum_w!=v Pi(!w))
//     //            = Pi(v) + 1/(N-1)sum_w!=v Pi(!w)  + P(v)_t Pi(?)
//     // e.g.
//     // so we need to get : P(v)_t,   P(info=v)  , sum_{w != v} P(info=!w), Pi(?)


//     //double sum_singular = 0.0;
//     int N_singular = 0.0;

//     for(auto& kv: distrib_tp1_singular) {

//       double pv_t = 0.0;
//       double pinfo_v = 0.0;
//       double pinfo_nv = 0.0;
//       double punknown = 0.0;

//       unsigned int value_index = kv.first;
//       std::string value_name = belief_tracker::Converter::get_value_name_from_index(i, value_index);

//       // Let us compute : P(info=v)  , sum_{w != v} P(info=!w), Pi(?)
//       for(auto& kv_info: turn_info) {
// 	auto& info = kv_info.first;
// 	double score = kv_info.second;
// 	std::string info_name = belief_tracker::Converter::get_value_name_from_index(i, info.at(i));
// 	if(info_name == belief_tracker::SYMBOL_UNKNOWN)
// 	  punknown += score;
// 	else if(value_name == info_name)
// 	  pinfo_v += score;
// 	else if(info_name.substr(0,1) == belief_tracker::SYMBOL_NOT && info_name.substr(1) != value_name)
// 	  pinfo_nv += score;
//       }
//       pinfo_nv /= double(N_values_for_sloti - 1.0);

//       // We now need P(area=west)_t
//       auto iter = distrib_t_singular.find(belief_tracker::Converter::get_value_index_from_name(i, value_name));
//       if(iter != distrib_t_singular.end()) 
// 	pv_t = iter->second;
//       else
// 	pv_t = distrib_t_other.first / double(distrib_t_other.second);

//       //std::cout << "Slot " << i << ", " << value_name << " " << pv_t << " " << pinfo_nv << " " << pinfo_v << std::endl;

//       // We now update the probability
//       // P(v)_{t+1} = Pi(v) + 1/(N-1)sum_w!=v Pi(!w)  + P(v)_t Pi(?)

      
//       kv.second = pinfo_v + pinfo_nv + pv_t * punknown ;

//       //sum_singular += kv.second;
//       ++N_singular;
//     }

//     double sum_p_not_in_info = 0.0;
//     double p_unknown = 0.0;
//     for(auto& kv_info: turn_info) {
//       auto& info = kv_info.first;
//       double score = kv_info.second;
//       std::string info_name = belief_tracker::Converter::get_value_name_from_index(i, info.at(i));
//       if(info_name == belief_tracker::SYMBOL_UNKNOWN)
// 	p_unknown += score;
//       else if(info_name.substr(0,1) == belief_tracker::SYMBOL_NOT)
// 	sum_p_not_in_info += score;
//     }

//     //std::cout << "Singular : " << N_singular << " with a mass " << sum_singular << std::endl;
//     int Nother = N_values_for_sloti - N_singular;
//     if(Nother == 0) {
//       distrib_tp1_other.first = 0.0;
//       distrib_tp1_other.second = 0.0;
//     }
//     else {
//       distrib_tp1_other.first = double(Nother) * distrib_t_other.first/double(distrib_t_other.second) * p_unknown + sum_p_not_in_info / double(belief_tracker::Converter::get_value_set_size_from_slot(i) - 1.0);
//       distrib_tp1_other.second = Nother;
//     }

//   }


//   return belief_tp1;
// }


// void belief_tracker::marginal::fill_tracker_session(belief_tracker::TrackerSessionTurn& tracker_session_turn, 
// 						    const belief_tracker::marginal::Belief& belief,
// 						    const belief_tracker::Ontology& ontology) {

// }



// std::ostream & operator<<(std::ostream &os, const belief_tracker::marginal::Belief &b) {

//   for(unsigned int i = 0 ; i < b.size(); ++i) {
//     // Compute the entropy
//     double H = 0.0;
//     auto& distrib = b[i];
//     auto& distrib_singular = distrib.first;
//     auto& distrib_other = distrib.second;

//     for(auto& kv: distrib_singular) 
//       if(kv.second != 0.0)
// 	H += -kv.second * log(kv.second);
//     if(distrib_other.first != 0.0) {
//       double p = distrib_other.first;
//       int N = distrib_other.second;
//       if(N != 0)
// 	H += -p * log(p/N); // p is distributed uniformely over the N other values
//     }
//     // nb values in ontology + dontcare
//     int N_values_for_sloti = belief_tracker::Converter::get_value_set_size_from_slot(i) + 1;
//     double p_unknown = H / log(double(N_values_for_sloti));
//     double alpha = 1.0 - H / log(double(N_values_for_sloti));


    

//     os << belief_tracker::Converter::get_slot_name_from_index(i) << ": {";
//     for(auto& kv: distrib_singular) {
//       os << belief_tracker::Converter::get_value_name_from_index(i, kv.first)
// 	 << ": " << kv.second << "(" << alpha * kv.second <<"),";
//     }
//     double p_other = distrib_other.first;
//     int N_other = distrib_other.second;
//     os << "others(" << N_other <<"):" << p_other << "(" << p_other * alpha << ")";
//     os << ",(unknown:" << p_unknown << ")";
//     os << "}";
//     os << " H = " << H ;

//     if(i != b.size() - 1)
//       os << ",";

//     os << std::endl;

//   }

//   os << std::endl;
//   return os;
// }


std::ostream & operator<<(std::ostream &os, const belief_tracker::joint::Belief &b) {
  for(auto& g_score: b.belief)
    os << g_score.first.toStr() << "(" << g_score.second << ")\n";
  return os;
}
