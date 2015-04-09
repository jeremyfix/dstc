#include "info.h"
#include "constants.h"
#include "user_acts.h"
#include "tools.h"
#include "converter.h"
#include "JSONObjects.h"
#include "goal.h"
#include <sstream>
#include <list>
#include <functional>
#include <iostream>

std::ostream& operator<<(std::ostream& os, 
			  const belief_tracker::info::BySlot< belief_tracker::info::Values >& info) {
  os << "(";
  unsigned int slot_index = 0;
  for(auto& slot_values: info) {
    os << belief_tracker::Converter::get_slot(slot_index) << ":";
    os << "{";
    unsigned int index = 0;
    unsigned int Nvalues = slot_values.size();
    for(auto& v: slot_values) {
      os << belief_tracker::Converter::get_value(slot_index, v);
      if(index < Nvalues - 1)
	os << ",";
    }
    os << "}";
    ++slot_index;
  }
  os << ")";
  return os;
}

void display_byslot_values(std::ostream &os, const belief_tracker::info::BySlot< belief_tracker::info::Values >& info) {
  os << "(";
  unsigned int slot_index = 0;
  for(auto& slot_values: info) {
    os << belief_tracker::Converter::get_slot(slot_index) << ":";
    os << "{";
    unsigned int index = 0;
    unsigned int Nvalues = slot_values.size();
    for(auto& v: slot_values) {
      os << belief_tracker::Converter::get_value(slot_index, v);
      if(index < Nvalues - 1)
	os << ",";
    }
    os << "}";
    ++slot_index;
  }
  os << ")";
}

std::ostream& operator<<(std::ostream &os, 
			  const belief_tracker::info::TurnInfo& turn_info) {
  unsigned int info_i_index = 0;
  for(auto& info_i: turn_info) {
    
    for(auto& ut_info: info_i) 
      os << ut_info.first << " : " << ut_info.second << " ";
    os << std::endl;
    ++ info_i_index;

  }
  os << std::endl;
  return os;
}

void display_turn_info(std::ostream &os, const belief_tracker::info::TurnInfo& turn_info) {

  os << "<";
  unsigned int info_i_index = 0;
  for(auto& info_i: turn_info) {
    
    for(auto& ut_info: info_i) 
      os << ut_info.first << " : " << ut_info.second;

    if(info_i_index < turn_info.size() - 1)
      os << ",";
    ++ info_i_index;
  }
  os << ">";
  os << std::endl;
}


belief_tracker::Goal belief_tracker::info::transition_function(const belief_tracker::Goal& antecedent, const belief_tracker::info::BySlot<belief_tracker::info::Values>& info) {

  belief_tracker::Goal image;
  image.slots_values.resize(antecedent.slots_values.size());
  unsigned int Ninformable = belief_tracker::Converter::get_informable_slots_size();

  auto antecedent_iter = antecedent.slots_values.begin();
  auto image_iter = image.slots_values.begin();
  auto info_iter = info.begin();

  for(unsigned int slot_index = 0 ; slot_index < Ninformable; ++slot_index, ++info_iter, ++image_iter, ++antecedent_iter) {


    ///////// Checking the content of the information, 
    //        it simplifies the transition function
    bool info_is_unknown = false;
    bool info_has_one_positive = false;
    bool info_has_only_negatives = false;
    // otherwise this is an exception

    for(auto& vi: *info_iter) {
      if(belief_tracker::Converter::is_unknown(slot_index,vi)) {
	// We check if unknown is alone
	if(info_iter->size() != 1)
	  throw std::logic_error("Unexpected informations in transition_function; unknown must be alone");
	info_is_unknown = true;
      }
      else if(belief_tracker::Converter::is_positive(slot_index,vi)) {
	// We check if the positive is alone
	if(info_iter->size() != 1)
	  throw std::logic_error("Unexpected informations in transition_function; a positive must be alone");
	info_has_one_positive = true;
      }
      else if(belief_tracker::Converter::is_negative(slot_index,vi)) {
	info_has_only_negatives = true;
      }
      else {
	std::ostringstream ostr;
	ostr.str("");
	ostr << "Unexpected informations in transition_function; expected a positive, negative or unknown but got " << belief_tracker::Converter::get_slot(slot_index) << " = " << belief_tracker::Converter::get_value(slot_index, vi);
	throw std::logic_error(ostr.str());
      }
    }


    // Updating the goal

    if(info_is_unknown) 
      *image_iter = *antecedent_iter;
    else if(info_has_one_positive && belief_tracker::Converter::is_unknown(slot_index, *antecedent_iter)) {
      // the positive information becomes the new goal
      *image_iter = *(info_iter->begin());
    }
    else if(info_has_only_negatives && belief_tracker::Converter::is_unknown(slot_index, *antecedent_iter)) {
      *image_iter = belief_tracker::Converter::get_unknown(slot_index);
    }
    else {
      // In this case, the goal is necessarily in val*
      if(info_has_one_positive) {
	*image_iter = *(info_iter->begin());
      }
      else if(info_iter->find(belief_tracker::Converter::negate(slot_index,*antecedent_iter)) == info_iter->end()) 
	// The question is whether or not the previous goal belongs to the negatives
	*image_iter = *antecedent_iter;
      else
	*image_iter = belief_tracker::Converter::get_unknown(slot_index);
      
    }    
  }

							    
  return image;  
}




std::pair<belief_tracker::info::Values, belief_tracker::info::Values> belief_tracker::info::split(unsigned int slot_index, const belief_tracker::info::Values& info) {
  belief_tracker::info::Values pos_u, neg_u;

  for(auto& value: info) {
    if(belief_tracker::Converter::is_weak_dontcare(slot_index, value) ||
       belief_tracker::Converter::is_unknown(slot_index, value))
      throw std::runtime_error("belief_tracker::split is not supposed to get unknown or weak dont care in its input !");
    if(belief_tracker::Converter::is_positive(slot_index, value))
      pos_u.insert(value);
    else
      neg_u.insert(value);
  }

  return std::make_pair(std::move(pos_u), std::move(neg_u));
}


belief_tracker::info::Values belief_tracker::info::multi_info::filter(unsigned int slot_index,
							 const belief_tracker::info::Values& values) {
  belief_tracker::info::Values filtered_values;

  // Extract the positives and the negatives from the given informations
  belief_tracker::info::Values pos_u, neg_u;
  std::tie(pos_u, neg_u) = split(slot_index, values);
  

  // If have no positives, we keep all the negatives
  if(pos_u.size() == 0) 
    filtered_values = neg_u;
  else {
    // We keep all the positives
    filtered_values = pos_u;

    // As well as the negatives that are in conflict with the positives
    // ie the not(v) in neg_u, such that  v is in pos_u
    for(auto& nv: neg_u) {
      unsigned int pv = belief_tracker::Converter::positivate(slot_index, nv);
      if(pos_u.find(pv) != pos_u.end()) // if pv is in the positive set, we keep nv
	filtered_values.insert(nv);
    }
  }
  return filtered_values;
}


belief_tracker::info::MultiOf<belief_tracker::info::Values> belief_tracker::info::multi_info::clean(unsigned int slot_index, const belief_tracker::info::Values& values) {

  belief_tracker::info::MultiOf<belief_tracker::info::Values> cleaned_values;
  if(values.size() == 0) 
    cleaned_values.push_back({belief_tracker::Converter::get_unknown(slot_index)});
  else {
    // Extract the positives and the negatives from the given informations
    belief_tracker::info::Values pos_u, neg_u;
    std::tie(pos_u, neg_u) = split(slot_index, values);

    if(neg_u.size() != 0) {
      //std::cout << neg_u.size() << " negatives " << std::endl;
      cleaned_values.push_back(neg_u);
    }
    /*
    if(neg_u.size() > 1) {
      throw std::exception();
      std::cout << "!!!!!!!!!!!!!!!!!! " << neg_u.size() << std::endl;
    }
    */
    //if(pos_u.size() != 0)
    //  std::cout << pos_u.size() << " positives " << std::endl;

    //if(neg_u.size() != 0 && pos_u.size() != 0) 
    //  std::cout << pos_u.size() << " " << neg_u.size() << std::endl;
    //if(pos_u.size() > 1)
    //  std::cout << pos_u.size() << std::endl;
    for(auto pv: pos_u) 
      cleaned_values.push_back({pv});
  }

  return cleaned_values;
}



belief_tracker::info::TurnInfo belief_tracker::info::multi_info::extract_info(const belief_tracker::SluHyps &user_acts, 
																				   const belief_tracker::Uterance& macts, 
																				   bool verbose) {
  
  //if(verbose) std::cout << "Processing the SLU and machine act " << std::endl;
  
  // Collect the machine act informations, i.e. only positive values
  belief_tracker::info::BySlot< belief_tracker::info::Values> mact_expl_info(belief_tracker::Converter::get_informable_slots_size());
  belief_tracker::info::BySlot< belief_tracker::info::Values> mact_impl_info(belief_tracker::Converter::get_informable_slots_size());
  belief_tracker::info::BySlot< belief_tracker::info::Values> mact_canthelp_info(belief_tracker::Converter::get_informable_slots_size());
  for(auto& mact: macts) {
    if(mact.act == std::string("expl-conf"))
      for(auto& kv: mact.act_slots) {
	unsigned int slot_index = belief_tracker::Converter::get_slot(kv.first);
	unsigned int value_index = belief_tracker::Converter::get_value(slot_index, kv.second);
	mact_expl_info[slot_index].insert(value_index);
      }
    else if(mact.act == std::string("impl-conf")) {
      for(auto& kv: mact.act_slots) {
	unsigned int slot_index = belief_tracker::Converter::get_slot(kv.first);
	unsigned int value_index = belief_tracker::Converter::get_value(slot_index, kv.second);
	mact_impl_info[slot_index].insert(value_index);
      }
    }
    else  if(mact.act == std::string("canthelp")) {
      if(mact.act_slots.size() > 1) 
	for(auto& kv: mact.act_slots) {
	  unsigned int slot_index = belief_tracker::Converter::get_slot(kv.first);
	  unsigned int value_index = belief_tracker::Converter::get_value(slot_index, kv.second);
	  mact_canthelp_info[slot_index].insert(value_index);
	}

    }
  }
  
  
  if(verbose) {
    std::cout << "Collected machine act info : ";
    display_byslot_values(std::cout, mact_impl_info);
    display_byslot_values(std::cout, mact_expl_info);
    std::cout << std::endl;
  }
  
  // For each user utterance
  belief_tracker::info::TurnInfo infos;
  for(auto& slu_hyp: user_acts) {
    belief_tracker::info::ScoredUtteranceInfo scored_utterance_info;
    auto utterance_info = extract_info(slu_hyp.first,  mact_expl_info, mact_impl_info, mact_canthelp_info, verbose);
    double score = slu_hyp.second;
    double single_score = score / double(utterance_info.size());
    for(auto& u: utterance_info)
      scored_utterance_info.push_back(std::make_pair(u, single_score));
    infos.push_back(scored_utterance_info);
  }
  //if(verbose) std::cout << "Processing the SLU and machine act DONE" << std::endl << std::endl;
  return infos;
}






belief_tracker::info::UtteranceInfo belief_tracker::info::multi_info::extract_info(const belief_tracker::Uterance& uacts,
										   const belief_tracker::info::BySlot<belief_tracker::info::Values>& mact_expl_conf,
										   const belief_tracker::info::BySlot<belief_tracker::info::Values>& mact_impl_conf, 
										   const belief_tracker::info::BySlot<belief_tracker::info::Values>& mact_canthelp,
										   bool verbose) {
  
  if((uacts.size() == 1) && (uacts.begin()->act == belief_tracker::NULL_ACT)) {
    belief_tracker::info::UtteranceInfo info;
    unsigned int Ninformable = belief_tracker::Converter::get_informable_slots_size();
    belief_tracker::info::BySlot< belief_tracker::info::Values > unknown_info(Ninformable);
    unsigned int slot_index = 0;
    for(auto& info_i: unknown_info)
      info_i.insert(belief_tracker::Converter::get_unknown(slot_index++));
    info.push_back(unknown_info);
    return info;
  }

  //if(verbose) std::cout << "Extracting the info from one utterance " << std::endl;
  unsigned int Ninformable = belief_tracker::Converter::get_informable_slots_size();

  belief_tracker::info::BySlot<belief_tracker::info::Values> user_info(Ninformable);
  belief_tracker::info::BySlot<belief_tracker::info::MultiOf<belief_tracker::info::Values> > cleaned_info(Ninformable);

  for(auto& uact: uacts) {
    if(uact.act == std::string("deny")) {
      for(auto& kv: uact.act_slots) {
	unsigned int slot_index = belief_tracker::Converter::get_slot(kv.first);
	unsigned int value_index = belief_tracker::Converter::get_value(slot_index, std::string(belief_tracker::SYMBOL_NOT) + kv.second);
	user_info[slot_index].insert(value_index);
      }
    }
    if(uact.act == std::string("inform")) {
      for(auto& kv: uact.act_slots) {
	unsigned int slot_index = belief_tracker::Converter::get_slot(kv.first);
	unsigned int value_index = belief_tracker::Converter::get_value(slot_index, kv.second);
	user_info[slot_index].insert(value_index);
      }
    }
  }
  /*
  if(verbose) {
    std::cout << "Collected user info : " << std::endl;
    display_byslot_values(std::cout, user_info);
    std::cout << std::endl;
  }
  */
  //if(verbose) std::cout << "Filtering out the weak dont care" << std::endl;

  // For every slot, we replace or remove the weak_dontcare;
  auto user_info_iter = user_info.begin();
  for(unsigned int slot_index = 0 ; slot_index < Ninformable; ++slot_index, ++user_info_iter) {
    auto weak_dontcare_iter = user_info_iter->find(belief_tracker::Converter::get_weak_dontcare(slot_index));
    if(weak_dontcare_iter != user_info_iter->end()) {
      //if(verbose) std::cout << "I found a weak dont care for slot " << belief_tracker::Converter::get_slot(slot_index) << std::endl;
      // There is a weak dontcare
      // We drop it out
      user_info_iter->erase(weak_dontcare_iter);
      if(user_info_iter->size() == 0)// the weak dontcare was alone, replace it by dontcare
	user_info_iter->insert(belief_tracker::Converter::get_dontcare(slot_index));
    }
  }

  /*
  if(verbose) {
    std::cout << "Resulting user info : " << std::endl;
    display_byslot_values(std::cout, user_info);
    std::cout << std::endl;
  }
  */
  bool has_negate = false;
  for(auto& uact: uacts) 
    if((has_negate = (uact.act == std::string("negate"))))
      break;
  
  bool has_deny = false;
  for(auto& uact: uacts) 
    if((has_deny = (uact.act == std::string("deny"))))
      break;

  bool is_full_empty = true;
  for(auto& ui: user_info)
    if(!(is_full_empty = (ui.size() == 0)))
      break;
  
  bool has_expl_conf = false;
  for(auto& mact_slot: mact_expl_conf) 
    if((has_expl_conf = (mact_slot.size() != 0)))
      break;

  /*
  bool has_canthelp_informed = false;
  auto mact_canthelp_iter = mact_canthelp.begin();
  unsigned int slot_index = 0;
  for(auto& slot_user_info: user_info) {
    belief_tracker::info::Values pos_u, neg_u;
    std::tie(pos_u, neg_u) = split(slot_index, slot_user_info);
    if( (has_canthelp_informed = (pos_u.size() != 0) && (mact_canthelp_iter->size() != 0)))
      break;
    ++slot_index;
    ++mact_canthelp_iter;
  }
  */
  

  //if(verbose) std::cout << "Merging the user info and the machine act info" << std::endl;

  auto mact_expl_conf_iter = mact_expl_conf.begin();
  auto mact_impl_conf_iter = mact_impl_conf.begin();
  //mact_canthelp_iter = mact_canthelp.begin();
  user_info_iter = user_info.begin();
  auto cleaned_info_iter = cleaned_info.begin();
  for(unsigned int slot_index = 0 ; 
      slot_index < Ninformable; 
        ++slot_index, 
	++user_info_iter, 
	++cleaned_info_iter, 
	++mact_expl_conf_iter, 
	++mact_impl_conf_iter
	/*++mact_canthelp_iter*/) {


    // Extract A+ and A-
    belief_tracker::info::Values pos_u, neg_u;
    std::tie(pos_u, neg_u) = split(slot_index, *user_info_iter);


    if(has_negate && is_full_empty) {
      if(has_expl_conf) {
	if(mact_expl_conf_iter->size() != 0) { // Jx(s) != 0
	  for(auto v: *mact_expl_conf_iter) {
	    unsigned int not_v = belief_tracker::Converter::negate(slot_index, v);
	    user_info_iter->insert(not_v);
	  }	    
	}
	else 
	  user_info_iter->insert(mact_impl_conf_iter->begin(),
				 mact_impl_conf_iter->end());
      }
      else {
	for(auto v: *mact_impl_conf_iter) {
	  unsigned int not_v = belief_tracker::Converter::negate(slot_index, v);
	  user_info_iter->insert(not_v);
	}
      }
    }
    else if(has_negate && !is_full_empty && !has_deny ) {
      if(pos_u.size() != 0)
	user_info_iter->insert(pos_u.begin(), pos_u.end());
      else {
	user_info_iter->insert(mact_impl_conf_iter->begin(),
			       mact_impl_conf_iter->end());
	/*
	if(has_canthelp_informed)
	  user_info_iter->insert(mact_canthelp_iter->begin(),
				 mact_canthelp_iter->end());
	*/
      }
    }
    else {      

      if(pos_u.size() == 0) { 
	// There are no positives
	// We add to the user info all the positives from the machine act
	// that are not explicitely negated by the user
	
	for(auto v: *mact_impl_conf_iter) {
	  unsigned int not_v = belief_tracker::Converter::negate(slot_index, v);
	  if(neg_u.find(not_v) == neg_u.end())
	    user_info_iter->insert(v);
	}
	
	
	for(auto v: *mact_expl_conf_iter) {
	  unsigned int not_v = belief_tracker::Converter::negate(slot_index, v);
	  if(neg_u.find(not_v) == neg_u.end())
	    user_info_iter->insert(v);
	}
	/*
	if(has_canthelp_informed) 
	  for(auto v: *mact_canthelp_iter) {
	    unsigned int not_v = belief_tracker::Converter::negate(slot_index, v);
	    if(neg_u.find(not_v) == neg_u.end())
	      user_info_iter->insert(v);
	  }
	*/
      }
    }

    *cleaned_info_iter = clean(slot_index, filter(slot_index, *user_info_iter));
  }

  // We get multiple possibilities for each slot
  // We now expand these multiple possibilities
  //if(verbose) std::cout << "Expanding the informations" << std::endl;
  UtteranceInfo expanded_info;
  auto expanded_info_iter = std::back_inserter(expanded_info);
  belief_tracker::expand<belief_tracker::info::BySlot<belief_tracker::info::Values> >(cleaned_info.begin(), cleaned_info.end(), expanded_info_iter);
  return expanded_info;
}


belief_tracker::info::TurnInfo belief_tracker::info::single_info::extract_info(const belief_tracker::SluHyps &user_acts, 
									       const belief_tracker::Uterance& macts, 
									       bool verbose) {
  belief_tracker::info::TurnInfo info;
  for(auto& slu_hyp: user_acts) {
    double score = slu_hyp.second;
    // extract the hypothesis
    auto utterance_infos = extract_info(slu_hyp.first, macts, verbose);
    //std::cout << "--" << std::endl;
    double single_score = score / utterance_infos.size();


    belief_tracker::info::ScoredUtteranceInfo scored_info;
    for(auto& utt_info: utterance_infos) 
      scored_info.push_back(std::make_pair(utt_info, single_score));
    if(scored_info.size() == 0)
      throw std::logic_error("extract_info : a scored info should not be empty ");
    info.push_back(scored_info);
  }  
  return info;
}

void belief_tracker::info::single_info::rules::inform(const belief_tracker::Uterance& macts,
						      const belief_tracker::Uterance& uacts,
						      belief_tracker::info::BySlot<belief_tracker::info::Values>& pos_mu,
						      belief_tracker::info::BySlot<belief_tracker::info::Values>& neg_mu)
{
  for(auto& uact_i: uacts) 
    if(uact_i.act == std::string("inform"))
      for(auto& sv: uact_i.act_slots) 
	pos_mu[belief_tracker::Converter::get_slot(sv.first)].insert(belief_tracker::Converter::get_value(sv.first, sv.second));
}

void belief_tracker::info::single_info::rules::explconf(const belief_tracker::Uterance& macts,
							const belief_tracker::Uterance& uacts,
							belief_tracker::info::BySlot<belief_tracker::info::Values>& pos_mu,
							belief_tracker::info::BySlot<belief_tracker::info::Values>& neg_mu)
{
  bool has_affirm = false;
  for(auto& uact_i: uacts)  {
    has_affirm = (uact_i.act == std::string("affirm"));
    if(has_affirm)
      break;
  }

  if(has_affirm) {
  for(auto& mact_i: macts) 
    if(mact_i.act == std::string("expl-conf"))
      for(auto& sv: mact_i.act_slots) {
	  std::string slot = sv.first;
	  std::string value = sv.second;
	  pos_mu[belief_tracker::Converter::get_slot(slot)].insert(belief_tracker::Converter::get_value(slot,value));
      }
  }
}

void belief_tracker::info::single_info::rules::implconf(const belief_tracker::Uterance& macts,
							const belief_tracker::Uterance& uacts,
							belief_tracker::info::BySlot<belief_tracker::info::Values>& pos_mu,
							belief_tracker::info::BySlot<belief_tracker::info::Values>& neg_mu)
{
  bool has_negate = false;
  for(auto& uact_i: uacts)  {
    has_negate = (uact_i.act == std::string("negate"));
    if(has_negate)
      break;
  }

  if(!has_negate) {
    for(auto& mact_i: macts) 
      if(mact_i.act == std::string("impl-conf"))
	for(auto& sv: mact_i.act_slots) {
	  std::string slot = sv.first;
	  std::string value = sv.second;
	  pos_mu[belief_tracker::Converter::get_slot(slot)].insert(belief_tracker::Converter::get_value(slot, value));
	}
  }
}

void belief_tracker::info::single_info::rules::negate(const belief_tracker::Uterance& macts,
						      const belief_tracker::Uterance& uacts,
						      belief_tracker::info::BySlot<belief_tracker::info::Values>& pos_mu,
						      belief_tracker::info::BySlot<belief_tracker::info::Values>& neg_mu)
{
  bool has_negate = false;
  for(auto& uact_i: uacts)  {
    has_negate = (uact_i.act == std::string("negate"));
    if(has_negate)
      break;
  }
  
  if(has_negate) {
    for(auto& mact_i: macts) 
      if(mact_i.act == std::string("expl-conf"))
	for(auto& sv: mact_i.act_slots) {
	  std::string slot = sv.first;
	  std::string value = sv.second;
	  auto slot_index = belief_tracker::Converter::get_slot(slot);
	  auto value_index = belief_tracker::Converter::get_value(slot, value);
	  neg_mu[slot_index].insert(belief_tracker::Converter::negate(slot_index, value_index));
	}
  }
}

void belief_tracker::info::single_info::rules::deny(const belief_tracker::Uterance& macts,
							const belief_tracker::Uterance& uacts,
							belief_tracker::info::BySlot<belief_tracker::info::Values>& pos_mu,
							belief_tracker::info::BySlot<belief_tracker::info::Values>& neg_mu)
{
  for(auto& uact_i: uacts) 
    if(uact_i.act == std::string("deny"))
      for(auto& sv: uact_i.act_slots) {
	auto slot = sv.first;
	auto value = sv.second;
	auto slot_index = belief_tracker::Converter::get_slot(slot);
	auto value_index = belief_tracker::Converter::get_value(slot, value);
	auto nvalue_index = belief_tracker::Converter::negate(slot_index, value_index);
	neg_mu[slot_index].insert(nvalue_index);
      }

}

belief_tracker::info::TurnInfo belief_tracker::info::single_info::extract_info(const belief_tracker::SluHyps &user_acts, 
									       const belief_tracker::Uterance& macts,
									       std::list<belief_tracker::info::single_info::rules::rule_type> rules,
									       bool verbose) {

  belief_tracker::info::TurnInfo info;
  unsigned int Ninformable = belief_tracker::Converter::get_informable_slots_size();

  for(auto& slu_hyp: user_acts) {
    double score = slu_hyp.second;

    // extract the pos_mu and neg_mu sets
    belief_tracker::info::BySlot<belief_tracker::info::Values> pos_mu(Ninformable);
    belief_tracker::info::BySlot<belief_tracker::info::Values> neg_mu(Ninformable);

    // by applying the set of rules
    for(auto& f : rules) 
      f(macts, slu_hyp.first, pos_mu, neg_mu);

    // We can now merge the positives and negatives to build up  inf_m,u
    belief_tracker::info::BySlot<belief_tracker::info::MultiOf<belief_tracker::info::Values>> info_mu(Ninformable);
    auto pos_mu_iter = pos_mu.begin();
    auto neg_mu_iter = neg_mu.begin();
    auto info_mu_iter = info_mu.begin();
    for(unsigned int slot_index = 0 ; slot_index < Ninformable; 
	++slot_index, ++pos_mu_iter, ++neg_mu_iter, ++info_mu_iter) {
      if( (pos_mu_iter->size() == 0 && neg_mu_iter->size() == 0) ||
	  belief_tracker::is_null_uterance(slu_hyp.first)) 
	info_mu_iter->push_back({belief_tracker::Converter::get_unknown(slot_index)});
      else if(pos_mu_iter->size() == 0) 
	info_mu_iter->push_back(*neg_mu_iter);
      else {
        // All the positives are inserted as singleton
	for(auto p: *pos_mu_iter)
	  info_mu_iter->push_back({p});
	// And all the negatives that conflict with the positives are also
	// introduced in a set
	belief_tracker::info::Values neg_to_keep;
	for(auto n: *neg_mu_iter) 
	  if(pos_mu_iter->find(belief_tracker::Converter::positivate(slot_index, n)) != pos_mu_iter->end())
	    neg_to_keep.insert(n);
	if(neg_to_keep.size() != 0)
	  info_mu_iter->push_back(neg_to_keep);
	
      }
    }
    // And expand the information given the possibilities for every slot
    // as well as allocating part of the hypothesis score to each possibility
    UtteranceInfo expanded_info;
    auto expanded_info_iter = std::back_inserter(expanded_info);
    belief_tracker::expand<belief_tracker::info::BySlot<belief_tracker::info::Values> >(info_mu.begin(), info_mu.end(), expanded_info_iter); 
    double single_score = score / expanded_info.size();

    
    belief_tracker::info::ScoredUtteranceInfo scored_info;
    for(auto& utt_info: expanded_info) 
      scored_info.push_back(std::make_pair(utt_info, single_score));
    if(scored_info.size() == 0)
      throw std::logic_error("extract_info : a scored info should not be empty ");
    info.push_back(scored_info);
    
  }  
  return info;
}


belief_tracker::info::UtteranceInfo belief_tracker::info::single_info::extract_info(const belief_tracker::Uterance& uacts, 
										    const belief_tracker::Uterance& macts, 
										    bool verbose) {

  if(belief_tracker::is_null_uterance(uacts)) {
    belief_tracker::info::UtteranceInfo info;

    unsigned int Ninformable = belief_tracker::Converter::get_informable_slots_size();

    belief_tracker::info::BySlot< belief_tracker::info::Values > unknown_info(Ninformable);
    auto unknown_info_iter = unknown_info.begin();
    for(unsigned int slot_index = 0 ; slot_index < Ninformable; ++slot_index, ++unknown_info_iter)
      unknown_info_iter->insert(belief_tracker::Converter::get_unknown(slot_index));

    info.push_back(unknown_info);

    return info;
  }

  // info will collect all the possible values for every slot
  unsigned int Ninformable = belief_tracker::Converter::get_informable_slots_size();
  belief_tracker::info::BySlot< belief_tracker::info::Values > info(Ninformable);

  // We collect the slot/values that have been implicitely confirmed
  std::map<std::string, std::string> list_impl_conf;
  for(auto& mact_i: macts) 
    if(mact_i.act == std::string("impl-conf")) 
      for(auto& sv: mact_i.act_slots) 
	list_impl_conf[sv.first] = sv.second;
      
    
  // Does the user has a negate() ?
  bool has_negate = false;
  for(auto& uact: uacts) 
    if((has_negate = (uact.act == std::string("negate"))))
      break;


  // We can now loop over the acts of the user
  for(auto& dact: uacts) {
    if(dact.act == "inform") {
      std::string slot = dact.act_slots.begin()->first;
      if(slot != std::string("this")) { // the reference has been solved 
	std::string value = dact.act_slots.begin()->second;
	int slot_index = belief_tracker::Converter::get_slot(slot);
	unsigned int value_index = belief_tracker::Converter::get_value(slot, value);
	info[slot_index].insert(value_index);
    
	// If the slot is in the impl-conf, we drop it
	auto iter_end = list_impl_conf.end();
	for(auto iter = list_impl_conf.begin(); iter != iter_end ; ++iter) {
	  if(iter->first == slot) {
	    list_impl_conf.erase(iter);
	    break;
	  }
	}  
      }
      else {
	std::cerr << "I still have a this slot in the dialogs ? this should have been removed!" << std::endl;
	throw std::exception();
      }
    } // end inform    
    else if(dact.act == "negate") {
      // We look for a expl-conf in the machine acts
      std::string slot("");
      std::string value("");
      for(auto& mact_i: macts) {
	if(mact_i.act == "expl-conf") {
	  std::string slot = mact_i.act_slots.begin()->first;
	  std::string value = mact_i.act_slots.begin()->second;
	  int slot_index = belief_tracker::Converter::get_slot(slot);
	  unsigned int value_index = belief_tracker::Converter::get_value(slot, belief_tracker::SYMBOL_NOT + value);
	  info[slot_index].insert(value_index);
	}
      }
    } // end negate
    /*
    else if(dact.act == "deny") {
      std::string slot = dact.act_slots.begin()->first;
      std::string value = dact.act_slots.begin()->second;
      int slot_index = belief_tracker::Converter::get_slot(slot);
      unsigned int value_index = belief_tracker::Converter::get_value(slot, belief_tracker::SYMBOL_NOT + value);
      info[slot_index].insert(value_index);

      // If the deny deals with slot/values added to the impl-conf collection
      // we remove them
      auto impl_conf_iter = list_impl_conf.find(slot);
      if(impl_conf_iter != list_impl_conf.end() && impl_conf_iter->second == value) 
	list_impl_conf.erase(impl_conf_iter);
    }
    */
    else if(dact.act == "affirm") {
      // We check if there is expl-conf in the mact
      for(auto& mact_i: macts) {
	if(mact_i.act == "expl-conf") {
	  std::string slot = mact_i.act_slots.begin()->first;
	  std::string value = mact_i.act_slots.begin()->second;
	  int slot_index = belief_tracker::Converter::get_slot(slot);
	  unsigned int value_index = belief_tracker::Converter::get_value(slot, value);
	  info[slot_index].insert(value_index);
	}
      }
    }
  }

  
  if(!has_negate) {
    // As the user did not explicitely negate
    // we add the hypothesis on what has been implicitely confirmed
    for(auto& sv: list_impl_conf) {
      unsigned int slot_index = belief_tracker::Converter::get_slot(sv.first);
      unsigned int value_index = belief_tracker::Converter::get_value(slot_index, sv.second);
      info[slot_index].insert(value_index);
    }
  }
  
  {
    unsigned int slot_index = 0;
    for(auto& info_i: info) {
	int nb_negatives = 0;
	for(auto vi: info_i)
	  nb_negatives += (belief_tracker::Converter::is_negative(slot_index, vi));
	if(nb_negatives > 1)  {
	  display_byslot_values(std::cout, info);
	  std::cout << " " << nb_negatives;
	  std::cout << std::endl;
	}
	slot_index++;
    }
  }

  // Let us clean up the extracted informations
  // for example, if we have multiple negatives and a positive
  // we will keep only the positive

  belief_tracker::info::BySlot< belief_tracker::info::MultiOf<belief_tracker::info::Values> > cleaned_info(Ninformable);
  auto cleaned_info_iter = cleaned_info.begin();
  auto info_iter = info.begin();
  for(unsigned int slot_index = 0 ; slot_index < Ninformable; ++slot_index, ++info_iter, ++cleaned_info_iter)
    *cleaned_info_iter = belief_tracker::info::multi_info::clean(slot_index, belief_tracker::info::multi_info::filter(slot_index, *info_iter));


  // We get multiple possibilities for each slot
  // We now expand these multiple possibilities
  //if(verbose) std::cout << "Expanding the informations" << std::endl;
  UtteranceInfo expanded_info;
  auto expanded_info_iter = std::back_inserter(expanded_info);
  belief_tracker::expand<belief_tracker::info::BySlot<belief_tracker::info::Values> >(cleaned_info.begin(), cleaned_info.end(), expanded_info_iter); 

  return expanded_info;

}


// belief_tracker::MultiInfo belief_tracker::single_info::clean_multi_info(const MultiInfo &info) {
//   MultiInfo filtered_info;
//   filtered_info.resize(info.size());

//   //bool all_slots_set_to_unknown = true;
//   for(unsigned int i = 0 ; i < info.size() ; ++i) {
//     if(info[i].size() == 0)
//       filtered_info[i].insert(belief_tracker::Converter::get_value_index_from_name(i, belief_tracker::SYMBOL_UNKNOWN));
//     else {
//       //all_slots_set_to_unknown = false;
//       // We build up a form with 2 columns indicating whether we want one value and/or not this value
//       // 
//       std::array< std::set<unsigned int>, 2> form;
//       for(auto& info_i: info[i]) {
// 	std::string value = belief_tracker::Converter::get_value_name_from_index(i, info_i);
// 	if(value != belief_tracker::SYMBOL_UNKNOWN) {
// 	  if(value.substr(0,1) == belief_tracker::SYMBOL_NOT)
// 	    form[0].insert(belief_tracker::Converter::get_value_index_from_name(i, value.substr(1))); // put the name of the value ie south instead of !south
// 	  else
// 	    form[1].insert(info_i);
// 	}
//       }


//       if(form[1].size() == 0) {// there is no positive, we keep all the negatives
// 	if(form[1].size() > 1)
// 	  throw std::exception();
// 	for(auto& form_i : form[0])
// 	  filtered_info[i].insert(belief_tracker::Converter::get_value_index_from_name(i, belief_tracker::SYMBOL_NOT + belief_tracker::Converter::get_value_name_from_index(i, form_i)));
//       }
//       else {// we have at least one positive
// 	for(auto& pos: form[1]) {
// 	  // if the positive is also negative, we introduce the two conflicting hypothesis
// 	  if(form[0].find(pos) != form[0].end()) {
// 	    filtered_info[i].insert(pos);
// 	    filtered_info[i].insert(belief_tracker::Converter::get_value_index_from_name(i, belief_tracker::SYMBOL_NOT + belief_tracker::Converter::get_value_name_from_index(i, pos)));
// 	  }
// 	  else { // otherwise we  keep only the positive
// 	    filtered_info[i].insert(pos);
// 	  }
// 	}	
//       }
//     }
//   }


//   return filtered_info;
// }


// belief_tracker::ScoredInfo belief_tracker::single_info::extract_info_from_slu(const belief_tracker::SluHyps &user_acts, 
// 								 const belief_tracker::Uterance& macts, 
// 								 bool verbose) {

//   belief_tracker::ScoredInfo scored_info;

//   for(auto& slu_hyp: user_acts) {
//     double score = slu_hyp.second;
//     // extract the hypothesis
//     auto extracted_infos = belief_tracker::single_info::extract_info_from_dialog_act_collection(slu_hyp.first, macts, verbose);

//     // and insert them into the scored_info map
//     for(auto& sinfos_i: extracted_infos) {
//       if(scored_info.find(sinfos_i.first) == scored_info.end())
// 	scored_info[sinfos_i.first] = 0.0;
//       scored_info[sinfos_i.first] += score* sinfos_i.second;
//     }
//   }

//   return scored_info;
// }



// belief_tracker::ScoredInfo belief_tracker::single_info::extract_info_from_dialog_act_collection(const belief_tracker::Uterance& uacts, const belief_tracker::Uterance& macts, bool verbose) {

//   // In case we have the null uterance, the info is unknown
//   if(uacts.size() == 1 && uacts.begin()->act == NULL_ACT) {
//     belief_tracker::ScoredInfo scored_info;
//     belief_tracker::Info unknown(belief_tracker::Converter::get_informable_slots_size());
//     for(unsigned int i = 0 ; i< unknown.size() ; ++i)
//       unknown[i] = belief_tracker::Converter::get_value_index_from_name(i, belief_tracker::SYMBOL_UNKNOWN);
//     scored_info[unknown] = 1.0;
//     return scored_info;
//   }


//   belief_tracker::ScoredInfo scored_info;

//   // We will collect all the possible values for each slot
//   // we make use of std::set  to collect unique values
//   MultiInfo info;
//   info.resize(belief_tracker::Converter::get_informable_slots_size());

//   std::vector<std::pair<std::string, std::string> > canthelp_slots_values;
//   bool has_canthelp_exception = false;
//   for(auto& mact_i: macts) {
//     if(mact_i.act == std::string("canthelp.exception"))
//       has_canthelp_exception = true;
//     if(has_canthelp_exception)
//       break;
//   }

//   if(!has_canthelp_exception) {
//     for(auto& mact_i: macts) {
//       if(mact_i.act == std::string("canthelp"))
// 	std::copy(mact_i.act_slots.begin(),
// 		  mact_i.act_slots.end(),
// 		  std::back_inserter(canthelp_slots_values));
//     }
//   }

//   // Used to modulate the influence of the expl-conf / affirm pattern
//   // in case of affirm()inform(), we don't take the affirm to confirm the explconf
//   /*
//     bool has_an_inform = false;
//     for(auto& mact_i: uacts) {
//     has_an_inform = mact_i.act == std::string("inform");
//     if(has_an_inform)
//     break;
//     }
//   */

//   // We collect the slot/values that have been implicitely confirmed
//   std::map<std::string, std::string> list_impl_conf;
//   for(auto& mact_i: macts) {
//     if(mact_i.act == std::string("impl-conf")) {
//       for(auto& sv: mact_i.act_slots) {
// 	list_impl_conf[sv.first] = sv.second;
//       }
//     }
//   }

//   bool has_negate = false;

//   for(auto& dact: uacts) {
//     if(dact.act == "inform") {
//       std::string slot = dact.act_slots.begin()->first;
//       if(slot != std::string("this")) { // the reference has been solved 
// 	std::string value = dact.act_slots.begin()->second;
// 	int slot_index = belief_tracker::Converter::get_slot_index_from_name(slot);
// 	unsigned int value_index = belief_tracker::Converter::get_value_index_from_name(slot, value);
// 	info[slot_index].insert(value_index);

      
// 	// If the slot is in the impl-conf, we drop it
// 	auto iter_end = list_impl_conf.end();
// 	for(auto iter = list_impl_conf.begin(); iter != iter_end ; ++iter) {
// 	  if(iter->first == slot) {
// 	    list_impl_conf.erase(iter);
// 	    break;
// 	  }
// 	}  
//       }
//       else {
// 	std::cerr << "I still have a this slot in the dialogs ? this should have been removed!" << std::endl;
// 	throw std::exception();
//       }
//     } // end inform
//     /*
//       else if(dact.act == "confirm") {
//       std::string slot = dact.act_slots[0].first;
//       if(slot != std::string("this")) { // the reference has been solved 
//       std::string value = dact.act_slots[0].second;
//       int slot_index = belief_tracker::Goal::get_slot_index_from_name(slot);
//       unsigned int value_index = belief_tracker::Goal::get_value_index_from_name(slot, value);
//       info[slot_index].insert(value_index);
//       }
//       }
//     */
//     else if(dact.act == "negate") {
//       has_negate = true;
//       // We look for a expl-conf in the machine acts
//       std::string slot("");
//       std::string value("");
//       for(auto& mact_i: macts) {
// 	if(mact_i.act == "expl-conf") {
// 	  std::string slot = mact_i.act_slots.begin()->first;
// 	  std::string value = mact_i.act_slots.begin()->second;
// 	  int slot_index = belief_tracker::Converter::get_slot_index_from_name(slot);
// 	  unsigned int value_index = belief_tracker::Converter::get_value_index_from_name(slot, belief_tracker::SYMBOL_NOT + value);
// 	  info[slot_index].insert(value_index);
// 	}
//       }
//     } // end negate
//     else if(dact.act == "deny") {
//       std::string slot = dact.act_slots.begin()->first;
//       std::string value = dact.act_slots.begin()->second;
//       int slot_index = belief_tracker::Converter::get_slot_index_from_name(slot);
//       unsigned int value_index = belief_tracker::Converter::get_value_index_from_name(slot, belief_tracker::SYMBOL_NOT + value);
//       info[slot_index].insert(value_index);

//       // If the deny deals with slot/values added to the impl-conf collection
//       // we remove them
//       auto impl_conf_iter = list_impl_conf.find(slot);
//       if(impl_conf_iter != list_impl_conf.end() && impl_conf_iter->second == value) 
// 	list_impl_conf.erase(impl_conf_iter);
//     }
//     else if(dact.act == "reqalts") {
//       if(canthelp_slots_values.size() == 1) {
// 	// There is a single canthelp, the reqlats triggers a slot=!value
// 	std::string slot = canthelp_slots_values.begin()->first;
// 	std::string value = canthelp_slots_values.begin()->second;
// 	int slot_index = belief_tracker::Converter::get_slot_index_from_name(slot);
// 	unsigned int value_index = belief_tracker::Converter::get_value_index_from_name(slot, belief_tracker::SYMBOL_NOT + value);
// 	info[slot_index].insert(value_index);
//       }
//     }
//     else if(dact.act == "affirm") {
//       // We check if there is no inform in the utterance
//       // if there is no inform in the utterance
//       //if(verbose) std::cout << "I have an affirm" << std::endl;

//       if(canthelp_slots_values.size() == 1) {
// 	// There is a single canthelp, the reqlats triggers a slot=!value
// 	std::string slot = canthelp_slots_values.begin()->first;
// 	std::string value = canthelp_slots_values.begin()->second;
// 	int slot_index = belief_tracker::Converter::get_slot_index_from_name(slot);
// 	unsigned int value_index = belief_tracker::Converter::get_value_index_from_name(slot, belief_tracker::SYMBOL_NOT + value);
// 	info[slot_index].insert(value_index);
//       }

//       // We check if there is expl-conf in the mact
//       for(auto& mact_i: macts) {
// 	if(mact_i.act == "expl-conf") {
// 	  std::string slot = mact_i.act_slots.begin()->first;
// 	  std::string value = mact_i.act_slots.begin()->second;
// 	  int slot_index = belief_tracker::Converter::get_slot_index_from_name(slot);
// 	  unsigned int value_index = belief_tracker::Converter::get_value_index_from_name(slot, value);

// 	  // Is there an inform(slot=..) in the user act ?
// 	  /*
// 	    bool has_an_inform_slot = false;
// 	    for(auto& mact_i: uacts) {
// 	    has_an_inform_slot = mact_i.act == std::string("inform") && mact_i.act_slots[0].first == slot;
// 	    if(has_an_inform_slot)
// 	    break;
// 	    }

// 	    if(!has_an_inform_slot) // we consider that the user acknowledges the expl-conf
// 	  */
// 	  info[slot_index].insert(value_index);
// 	}
// 	/*
// 	  else if(mact_i.act == "canthelp" && mact_i.act_slots.size() == 1) {
// 	  std::string slot = mact_i.act_slots[0].first;
// 	  std::string value = mact_i.act_slots[0].second;
// 	  int slot_index = belief_tracker::Goal::get_slot_index_from_name(slot);
// 	  unsigned int value_index = belief_tracker::Goal::get_value_index_from_name(slot, belief_tracker::SYMBOL_NOT + value);
// 	  info[slot_index].insert(value_index);
// 	  }
// 	*/
//       }
//     }
//   }



//   if(!has_negate) {
//     // As the user did not explicitely negate
//     // we add the hypothesis on what has been implicitely confirmed
//     for(auto& sv: list_impl_conf) {
//       unsigned int slot_index = belief_tracker::Converter::get_slot_index_from_name(sv.first);
//       unsigned int value_index = belief_tracker::Converter::get_value_index_from_name(slot_index, sv.second);
//       info[slot_index].insert(value_index);
//     }
//   }

//   auto filtered_info = clean_multi_info(info);

//   // If we extracted nothing, extracted_infos will only contain the (?, ?, ?, ..) unknown info
//   auto extracted_infos = belief_tracker::expand_multi_info(filtered_info);
//   double single_score = 1.0 / extracted_infos.size();
//   for(auto& info_i : extracted_infos)
//     scored_info[info_i] = single_score;

//   return scored_info;
// }
