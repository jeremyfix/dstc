
#include "user_acts.h"
#include "converter.h"
#include <string>
#include <stdexcept>
#include <sstream>
#include <set>
#include <cmath>
    
/*
void belief_tracker::renormalize_slu(belief_tracker::SluHyps& slu_hyps) {
  double sum = 0.0;
  for(auto& uact_i: slu_hyps) 
    sum += uact_i.second;
  for(auto& uact_i: slu_hyps) 
    uact_i.second /= sum;
}
*/

bool belief_tracker::has_null_larger_than(const belief_tracker::SluHyps& slu_hyps,
			  double threshold) {
  for(auto& hyp: slu_hyps) {
    double score = hyp.second;
    if(belief_tracker::is_null_uterance(hyp.first) && score >= threshold)
      return true;
  }
  return false;
}

void belief_tracker::renormalize_slu(belief_tracker::SluHyps& slu_hyps) {
  double sum = 0.0;
  for(auto& uact_i: slu_hyps) 
    sum += uact_i.second;

  for(auto& uact_i: slu_hyps) 
    uact_i.second /= sum;
}

bool belief_tracker::is_null_uterance(const belief_tracker::Uterance& uterance) {
  bool has_null = false;
  for(auto& act_i : uterance) {
    has_null = (act_i.act == belief_tracker::NULL_ACT);
    if(has_null) 
      break;
  }
  if(has_null && uterance.size() != 1)
    throw std::logic_error("is_null_uterance : null is expected to be alone in the uterance !");
  return has_null;
}

std::pair<double, double> betafit(const std::list<double>& stats) {
  double Gx = 1.0;
  double G1_x = 1.0;
  auto N = stats.size();
  for(auto x : stats) {
    if(x >= 1.0)
      x = 0.999999;
    Gx *= std::pow(x, 1.0 / double(N));
    G1_x = std::pow(1.0 - x, 1.0 / double(N));
  }
  double coef = 2.0 * (1.0 - Gx - G1_x);
  if(fabs(coef) <= 1e-9)
    return {1.0, 1.0};
  else 
    return { 0.5 + Gx / coef,
	0.5 + G1_x/ coef};
}

void belief_tracker::adjust_noise(belief_tracker::SluHyps& slu_hyps,
				  std::list<double>& stats) {

  double sum_not_null = 0.0;
  bool has_only_null = true;
  bool has_a_null = false;
  
  for(auto& uact_i: slu_hyps) {
    if(is_null_uterance(uact_i.first))
      has_a_null = true;
    else
      has_only_null = false;
  }

  for(auto& uact_i: slu_hyps) {
    if(is_null_uterance(uact_i.first)) {
      if(!has_only_null) uact_i.second = 0.0;
    }
    else {
      sum_not_null += uact_i.second;
    }
  }

  if(has_only_null)
    return;


  double m = 1.0;

  if(!has_only_null || stats.size() != 0)
    stats.push_back(sum_not_null);
  if(stats.size() != 0) {
    double a, b;
    std::tie(a, b) = betafit(stats);
    m = a / (a + b);
  }

  double sum = 0.0;
  for(auto& uact_i: slu_hyps) {
    uact_i.second /= m;
    sum += uact_i.second;
  }

  if(sum > 1.0)
    for(auto& uact_i: slu_hyps) 
      uact_i.second /= sum;  
  else 
    if(has_a_null) {
      for(auto& uact_i: slu_hyps) 
	if((uact_i.first.size() == 1 && is_null_uterance(uact_i.first))) {
	  uact_i.second = 1.0 - sum;      
	  break;
	}
    }
    else {
      belief_tracker::DialogAct act;
      act.act = belief_tracker::NULL_ACT;
      std::pair<belief_tracker::Uterance, double> null_hyp = { {act},1.0-sum};
      slu_hyps.push_back(null_hyp);
    }
      
}

void belief_tracker::adjust_noise(belief_tracker::SluHyps& slu_hyps,
				  double target) {
  bool has_only_null = (slu_hyps.size() == 1) && (is_null_uterance(slu_hyps.begin()->first));
  if(has_only_null) 
    return;
  
  bool has_a_null = false;
  double pnull = 0.0;
  for(auto& uact_i: slu_hyps) {
    if(is_null_uterance(uact_i.first)) {
      if(has_a_null)
	throw std::logic_error("Scale noise was not expecting to see null() multiple times!");
      else {
	pnull = uact_i.second;
	has_a_null = true;
      }
    }
  }

  //std::cout << "pnull = " << pnull << std::endl;

  if(!has_a_null)
    return;

  // Otherwise
  // the others have a mass of (1 - pnull)
  // which will be scaled to (1 - target)
  for(auto& uact_i: slu_hyps) {
    if(is_null_uterance(uact_i.first))
      uact_i.second = target;
    else 
      uact_i.second *= (1.0 - target) / (1.0 - pnull);
  }

}

void belief_tracker::scale_noise(belief_tracker::SluHyps& slu_hyps, double alpha) {
  // If there is a null, we scale its score by alpha and the others are scaled by beta
  // alpha * p(null)

  bool has_only_null = (slu_hyps.size() == 1) && (is_null_uterance(slu_hyps.begin()->first));
  if(has_only_null) 
    return;
  
  bool has_a_null = false;
  double pnull = 0.0;
  for(auto& uact_i: slu_hyps) {
    if(is_null_uterance(uact_i.first)) {
      if(has_a_null)
	throw std::logic_error("Scale noise was not expecting to see null() multiple times!");
      else {
	pnull = uact_i.second;
	has_a_null = true;
      }
    }
  }

  if(has_a_null)
    adjust_noise(slu_hyps, alpha * pnull);
}

void belief_tracker::saturate_noise(belief_tracker::SluHyps& slu_hyps, double alpha) {
  // If there is a null, we set its score to alpha and the others are scaled by beta
  // alpha 

  bool has_only_null = (slu_hyps.size() == 1) && (is_null_uterance(slu_hyps.begin()->first));
  if(has_only_null) 
    return;
  
  bool has_a_null = false;
  double pnull = 0.0;
  for(auto& uact_i: slu_hyps) {
    if(is_null_uterance(uact_i.first)) {
      if(has_a_null)
	throw std::logic_error("Scale noise was not expecting to see null() multiple times!");
      else {
	pnull = uact_i.second;
	has_a_null = true;
      }
    }
  }

  if(has_a_null && pnull > alpha)
    adjust_noise(slu_hyps, alpha);
}



/*
belief_tracker::Uterance::iterator belief_tracker::find_inform_this_dontcare_in_list(belief_tracker::Uterance& acts) {
  return std::find_if(acts.begin(), acts.end(),
		      [](belief_tracker::DialogAct& acti) -> bool { 
			return acti.act == std::string("inform") &&
			  acti.act_slots.size() == 1 &&
			  acti.act_slots[0].first == "this" &&
			  acti.act_slots[0].second == "dontcare";
		      });
}
*/

/**
   Browse the individual acts and try to find some xx(this=..)
 */

std::string belief_tracker::this_reference::single::solve(const belief_tracker::Uterance& macts, bool verbose) {
  // To solve a "this" reference, we check if there is a request, expl-conf or select or canthelp1  act in the macts

  std::set< std::string > possible_replacements;

  for(auto& mact_i: macts) {
    if(mact_i.act == "request") {
      std::string slot = mact_i.act_slots.begin()->second;
      possible_replacements.insert(slot);
    }
    else if(mact_i.act == std::string("expl-conf") || mact_i.act == std::string("select")) {
      // For DSTC2-test : voip-ef9aa63b85-20130328_190707
      // in the semantics there is a inform(this=dontcare) where this must be infered for a expl-conf 
      // in DSTC3, the inform(this=dontcare) can comes from a select(area=..)select(area=dontcare)
      std::string slot = mact_i.act_slots.begin()->first;
      possible_replacements.insert(slot);
    }
  }

  if(possible_replacements.size() == 0 || possible_replacements.size() > 1)  // We did not find any replacement or too many candidates
    return std::string("this");
  else 
    return *possible_replacements.begin();
  
}

void belief_tracker::this_reference::single::rewrite_slu(belief_tracker::SluHyps& slu_hyps, 
						 const belief_tracker::Uterance& macts, 
						 bool verbose) {
  
  // We go through the user acts,
  // in case there is the slot "this", we have to infer from the context how to rewrite it
  // and we rewrite it by inspecting the machine act and taking THE FIRST request(...) 
  // found in the mact if any
  // We don't remove the "this" reference in case we don't find 
  // a replacement

  std::string replacement_for_this = belief_tracker::this_reference::single::solve(macts, verbose);
  bool has_a_replacement = replacement_for_this != std::string("this");
  
  if(!has_a_replacement) {
    // We just delete the xxxx(this=yyyy) acts
    for(auto& slu_hypi: slu_hyps) {
      auto& hyp = slu_hypi.first;
      auto hyp_iter = hyp.begin();
      auto hyp_end = hyp.end();
      while(hyp_iter != hyp_end) {
        if(hyp_iter->act_slots.size() != 0 && hyp_iter->act_slots.begin()->first == std::string("this")) {
	  //std::cout << "A this is deleted" << std::endl;
	  hyp_iter = hyp.erase(hyp_iter);
	}
	else
	  ++hyp_iter;	
      }
      
      // In case we suppressed all the acts, we insert a null act
      if(hyp.size() == 0) {
	belief_tracker::DialogAct dact;
	dact.act = belief_tracker::NULL_ACT;
	hyp.push_back(dact);
      }
    }
  }
  else {
    // We replace the "this" slot by its replacement
    for(auto& slu_hypi: slu_hyps) {
      auto& hyp = slu_hypi.first;
      for(auto& slu_hypi_j : hyp) {
	// We browse the slot/value of the act  slu_hypi_j
	std::string act = slu_hypi_j.act;
	for(auto& sv: slu_hypi_j.act_slots) {
	  std::string slot = sv.first;
	  std::string value = sv.second;
	  if(slot == std::string("this")) {// There is a this to replace 
	    //std::cout << "A this is replaced" << std::endl;
	    sv.first = replacement_for_this;
	  }
	}
      }
    }
  }
  

  // We now need to collapse potential duplicates in the hypotheses
  belief_tracker::collapse_duplicate_hypotheses(slu_hyps);
}

std::set<std::string> belief_tracker::this_reference::weak_dontcare::solve(const belief_tracker::Uterance& macts, bool verbose) {
  std::set< std::string > possible_replacements;
  bool has_a_request = false;
  for(auto& mact_i: macts) {
    if(mact_i.act == "request") {
      for(auto& kv: mact_i.act_slots) {
	std::string slot_name = kv.second;
	if(belief_tracker::Converter::is_slot_informable(slot_name)) {
	  if(!has_a_request)
	    possible_replacements.clear();
	  possible_replacements.insert(slot_name);
	  has_a_request = true;
	}
      }
    }
    else if(mact_i.act == "impl-conf" || mact_i.act == "expl-conf"){
      for(auto& kv: mact_i.act_slots) {
	std::string slot_name = kv.first;
	if(belief_tracker::Converter::is_slot_informable(slot_name))
	  if(!has_a_request) possible_replacements.insert(slot_name);
      }
    }
  }
  return possible_replacements;
}


void belief_tracker::this_reference::weak_dontcare::rewrite_slu(belief_tracker::SluHyps& slu_hyps, 
								const belief_tracker::Uterance& macts, 
								bool verbose) {
  
  // We go through the user acts,
  // in case there is the slot "this", we have to infer from the context how to rewrite it
  // and we rewrite it by inspecting the machine act and taking THE FIRST request(...) 
  // found in the mact if any
  // We don't remove the "this" reference in case we don't find 
  // a replacement

  std::set<std::string> replacement_for_this = belief_tracker::this_reference::weak_dontcare::solve(macts, verbose);

  for(auto& slu_hyp: slu_hyps) {

    // We go through all the acts in the current user utterance
    auto act_iter = slu_hyp.first.begin();
    auto act_iter_end = slu_hyp.first.end();

    while(act_iter != act_iter_end) {
      std::string act_name = act_iter->act;
      auto sv_iter = act_iter->act_slots.begin();
      auto sv_iter_end = act_iter->act_slots.end();
      bool has_a_this = false;
      // We remove all the (this=yyyy) slot/value pairs
      while(sv_iter != sv_iter_end) {
	if(sv_iter->first == std::string("this") && sv_iter->second == std::string(belief_tracker::SYMBOL_DONTCARE)) {
	  has_a_this = true;
	  sv_iter = act_iter->act_slots.erase(sv_iter);
	}
	else
	  ++sv_iter;
      }

      // We make a copy of the act that will be used for the replacements
      auto act_cpy = *act_iter;

      // If it had a this
      if(has_a_this) {
	if(verbose) std::cout << "I had a this in the utterance" << std::endl;
	// if we removed all the slot/value pairs for the act
	// We remove the act
	
	if(act_iter->act_slots.size() == 0) 
	  act_iter = slu_hyp.first.erase(act_iter);
	
	// We now need to insert copies of the act_cpy for each of the replacements with a weak dont care
	for(auto& slot: replacement_for_this) {
	  auto act = act_cpy;
	  act.act_slots.push_back(std::make_pair(slot, std::string(belief_tracker::SYMBOL_WEAKDONTCARE)));
	  slu_hyp.first.insert(act_iter, act);
	  ++act_iter;
	}
      }
      else
	++act_iter;
    }

    // In case the hypotheses is empty (e.g. we had only inform(this=dontcare) without being able to solve the "this" reference,
    // we put an empty act in the utterance
    if(slu_hyp.first.size() == 0) {
      belief_tracker::DialogAct empty_act;
      empty_act.act = belief_tracker::NULL_ACT;
      slu_hyp.first.push_back(empty_act);
    }
  }

  // We now need to collapse potential duplicates in the hypotheses
  belief_tracker::collapse_duplicate_hypotheses(slu_hyps);
}

void belief_tracker::rewrite_repeat_machine_act(belief_tracker::Uterance& macts, 
						const belief_tracker::Uterance& prev_macts) {

  // We check if there is a repeat() machine act
  auto iter = find_act_in_list(macts.begin(), macts.end(), "repeat");
  if(iter != macts.end()) {
    // There is indeed a repeat !
    if(macts.size() != 1)
      throw std::logic_error("I was expecting a single machine act when replacing the repeat act !");
    macts.clear();
    std::copy(prev_macts.begin(), prev_macts.end(), std::back_inserter(macts));   
  }  
}

void belief_tracker::collapse_duplicate_hypotheses(belief_tracker::SluHyps& slu_hyps) {
  auto iter = slu_hyps.begin();
  unsigned int index = 0;
  auto iter_end = slu_hyps.end();
  while(iter != iter_end) {
    // We look if (*iter) = slu_hyps[0:index[ in 
    // which case we give the mass to the first occurence
    // and drop the second one
    bool has_found_duplicate = false;
    auto iter_duplicate = slu_hyps.begin();
    for(unsigned int i = 0 ; i < index; ++i, ++iter_duplicate) {
      if(iter_duplicate->first == iter->first) { // we have duplicates if the utterances are the same
	has_found_duplicate = true;
	break;
      }
    }


    if(has_found_duplicate) { // iter and iter_duplicate are duplicate hypotheses
      iter_duplicate->second += iter->second; // we give to iter_duplicate the mass of iter
      iter = slu_hyps.erase(iter); // we remove iter
      // and there is no need to increment index and iter
    }
    else {
      ++index;
      ++iter;
    }


  }
}




