#include "converter.h"
#include <algorithm>
#include <stdexcept>
#include <sstream>

belief_tracker::Ontology belief_tracker::Converter::ontology;
std::vector<std::string> belief_tracker::Converter::informable_slots;
std::map<std::string, unsigned int> belief_tracker::Converter::informable_slots_index;
std::vector<unsigned int> belief_tracker::Converter::informable_slots_values_size;


void belief_tracker::Converter::init(const belief_tracker::Ontology& onto) {
  ontology = onto;

  unsigned int slot_index = 0;
  for(auto& kv: ontology.informable) {
    std::string slot_name = kv.first;
    auto& slot_values = kv.second;
    informable_slots.push_back(slot_name);
    informable_slots_index[slot_name] = slot_index++;
    informable_slots_values_size.push_back(slot_values.size());
  }
}


bool belief_tracker::Converter::is_slot_informable(std::string slot_name) {
  return std::find(informable_slots.begin(), informable_slots.end(), slot_name) != informable_slots.end();
}

unsigned int belief_tracker::Converter::get_informable_slots_size(void) {
  return informable_slots.size();
}


unsigned int belief_tracker::Converter::get_value_set_size(unsigned int slot_index) {
  if(slot_index >= informable_slots_values_size.size()) {
    std::cerr << "Slot index " << slot_index << " out of bound, I have " << informable_slots_values_size.size() << " possible slots" << std::endl;
    throw std::exception();
  }
  return informable_slots_values_size[slot_index];
}

unsigned int belief_tracker::Converter::get_value_set_size(std::string slot) {
  auto iter = std::find(informable_slots.begin(), informable_slots.end(), slot);
  if(iter == informable_slots.end()) {
    std::cerr << "Cannot find slot named " << slot << " in the collection of slots" << std::endl;
    throw std::exception();
  }
  return get_value_set_size(std::distance(informable_slots.begin(), iter));
}

unsigned int belief_tracker::Converter::get_value(std::string slot, std::string value) {
  unsigned int N = get_value_set_size(slot);

  if(value == belief_tracker::SYMBOL_WEAKDONTCARE) {
    return 2*N + 3;
  }
  else if(value == belief_tracker::SYMBOL_UNKNOWN) {
    return 2*N + 2;
  }
  else if(value == belief_tracker::SYMBOL_NOT + belief_tracker::SYMBOL_DONTCARE) {
    return 2*N+1;
  }
  else if(value == belief_tracker::SYMBOL_DONTCARE) {
    return N;
  }
  else {    
    bool is_not = false;
    std::string not_str = std::string(belief_tracker::SYMBOL_NOT);
    auto mismatch_iters = std::mismatch(not_str.begin(), not_str.end(), value.begin());
    if(mismatch_iters.first == not_str.end()) {
      value = std::string(mismatch_iters.second, value.end());
      is_not = true;
    }

    auto& values_vec = ontology.informable[slot];
    auto value_vec_iter = std::find(values_vec.begin(), values_vec.end(), value);
    if(value_vec_iter == values_vec.end()) {
      std::cerr << "I did not find the value " << value << " in the values for slot " << slot << " in the ontology" << std::endl;
      throw std::exception();
    }
    unsigned int value_index = std::distance(values_vec.begin(), value_vec_iter);

    if(is_not) 
      return value_index + N + 1;
    else
      return value_index;    
  }
}

unsigned int belief_tracker::Converter::get_value(unsigned int slot_index, std::string value) {
  return get_value(get_slot(slot_index), value);
}

std::string belief_tracker::Converter::get_value(unsigned int slot_index, unsigned int value_index){
  unsigned int N = get_value_set_size(slot_index);
  std::string slot = get_slot(slot_index);

  if(value_index < N) 
    return ontology.informable[slot][value_index];
  else if(value_index == N)
    return SYMBOL_DONTCARE;
  else if(value_index < 2*N + 1) 
    return belief_tracker::SYMBOL_NOT + ontology.informable[slot][value_index - (N+1)];
  else if(value_index == 2*N + 1)
    return belief_tracker::SYMBOL_NOT + belief_tracker::SYMBOL_DONTCARE;
  else if(value_index == 2*N + 2)
    return belief_tracker::SYMBOL_UNKNOWN;
  else if(value_index == 2*N + 3)
    return belief_tracker::SYMBOL_WEAKDONTCARE;
  else {
    std::cerr << "Unrecognized value " << value_index << " for slot " << slot_index << std::endl;
    throw std::exception();
  }

}

std::string belief_tracker::Converter::get_value(std::string slot_name, unsigned int value_index) {
  return get_value(get_slot(slot_name), value_index);
}

std::string belief_tracker::Converter::get_slot(unsigned int slot_index){
  if(slot_index >= informable_slots.size()) {
    std::cerr << "Slot index " << slot_index << " out of bound, I have " << informable_slots.size() << " possible slots" << std::endl;
    throw std::exception();
  }
 
  return informable_slots[slot_index];
}

unsigned int belief_tracker::Converter::get_slot(std::string slot) {
  auto iter = std::find(informable_slots.begin(), informable_slots.end(), slot);
  if(iter == informable_slots.end()) {
    std::cerr << "Cannot find slot named " << slot << " in the collection of slots of the goals" << std::endl;
    throw std::exception();
  }
  return std::distance(informable_slots.begin(), iter);
}




unsigned int belief_tracker::Converter::negate(unsigned int slot_index,
					       unsigned int value_index) {
  unsigned int N = get_value_set_size(slot_index);
  if(value_index >= 0 && value_index < N + 1) {
    return value_index + N + 1;
  }
  else {
    std::ostringstream ostr;
    ostr.str("");
    ostr << "Invalid value index for negating! you asked me to negate " << get_value(slot_index, value_index) << " for the slot " << get_slot(slot_index) << std::endl;
    throw std::runtime_error(ostr.str());
  }
}

unsigned int belief_tracker::Converter::positivate(unsigned int slot_index,
						   unsigned int value_index) {
  unsigned int N = get_value_set_size(slot_index);
  if(value_index >= N+1 && value_index < 2*N+2) {
    return value_index - (N + 1);
  }
  else 
    throw std::runtime_error("Invalid value index for negating!");
}

bool belief_tracker::Converter::is_negative(unsigned int slot_index,
						  unsigned int value_index) {
  unsigned int N = get_value_set_size(slot_index);
  return value_index >= N + 1 &&
    value_index < 2 * N + 2;
}

bool belief_tracker::Converter::is_positive(unsigned int slot_index,
						  unsigned int value_index) {
  unsigned int N = get_value_set_size(slot_index);
  return value_index < N + 1;
}

bool belief_tracker::Converter::is_weak_dontcare(unsigned int slot_index,
						 unsigned int value_index) {
    unsigned int N = get_value_set_size(slot_index);
    return value_index == 2*N + 3;
}

bool belief_tracker::Converter::is_unknown(unsigned int slot_index,
					   unsigned int value_index) {
    unsigned int N = get_value_set_size(slot_index);
    return value_index == 2*N + 2;
}


unsigned int belief_tracker::Converter::get_unknown(unsigned int slot_index){
  unsigned int N = get_value_set_size(slot_index);
  return 2 * N + 2;
}

unsigned int belief_tracker::Converter::get_dontcare(unsigned int slot_index) {

  unsigned int N = get_value_set_size(slot_index);
  return N ;
}

unsigned int belief_tracker::Converter::get_weak_dontcare(unsigned int slot_index) {

  unsigned int N = get_value_set_size(slot_index);
  return 2 * N + 3 ;
}



