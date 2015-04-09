#include <belief-tracker.h>
#include <cstring>
#include <sstream>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <string>

namespace info = belief_tracker::info::multi_info;
namespace this_reference = belief_tracker::this_reference::weak_dontcare;


std::vector<size_t> find_position_of_char(std::string str, char c) {
  std::vector<size_t> indices;
  size_t cpos=0;
  while(cpos != std::string::npos) {
    cpos = str.find(c, cpos);
    if(cpos != std::string::npos) {
      indices.push_back(cpos);
      cpos += 1; // start at the next symbol
    }
  }
  return indices;
}

bool is_slot_informable(std::string slot,
			const belief_tracker::Ontology& ontology) {
  if(slot == std::string("this"))
    return true;

  if(ontology.informable.find(slot) == ontology.informable.end()) {
    std::cerr << "The slot \"" << slot << "\" is not informable" << std::endl;
    return false;
  }
  else
    return true;
}


bool is_slot_requestable(std::string slot,
			 const belief_tracker::Ontology& ontology) {
  if(slot == std::string("this"))
    return true;

  if(std::find(ontology.requestable.begin(),
	       ontology.requestable.end(),
	       slot) == ontology.requestable.end()) {
    std::cerr << "The slot \"" << slot << "\" is not requestable" << std::endl;
    return false;
  }
  else
    return true;
}

bool check_informable_slot_value(std::string slot,
				 std::string value,
				 const belief_tracker::Ontology& ontology) {
  if(slot == std::string("this"))
    return true;


  bool is_valid = is_slot_informable(slot, ontology);
  if(!is_valid)
    return false;
  else {
    is_valid = std::find(ontology.informable.at(slot).begin(),
			 ontology.informable.at(slot).end(),
			 value) != ontology.informable.at(slot).end() || value == std::string("dontcare");
    if(!is_valid) {
      std::cerr << "I did not find the value \"" << value << "\" for the informable slot \"" << slot << "\"" << std::endl;
      return false;
    }
  }
  return true;
}

bool check_empty_constraint(const std::list< std::pair<std::string, std::string> >& act_slots,
			    const belief_tracker::Ontology& ontology) {
  return true;
}

bool check_canthelp_constraint(const std::list< std::pair<std::string, std::string> >& act_slots,
			       const belief_tracker::Ontology& ontology) {
  // A non empty-list of slot/value pairs (in the informable)
  if(act_slots.size() == 0) {
    std::cerr << "Got an empty slot/value pairs list for a canthelp act" << std::endl;
    return false;
  }
 
  for(auto& sv: act_slots)
    if(!check_informable_slot_value(sv.first, sv.second, ontology))
      return false;
 
  return true;
}

bool check_canthelp_missing_slot_value_constraint(const std::list< std::pair<std::string, std::string> >& act_slots,
						  const belief_tracker::Ontology& ontology) {
  if(act_slots.size() != 2) {
    std::cerr << "Got an " << act_slots.size() << " slot/value pairs, expecting 2 pairs." << std::endl;
    return false;
  }
  auto iter = act_slots.begin();
  return iter->first == std::string("slot") &&
    (++iter)->first == std::string("value");

}

bool check_one_informable_constraint(const std::list< std::pair<std::string, std::string> >& act_slots,
				     const belief_tracker::Ontology& ontology) {
  // One slot/value pair (in the informable)
  if(act_slots.size() != 1) {
    std::cerr << "Got " << act_slots.size() << " slot/value pairs, expecting 1." << std::endl;
    return false;
  }
  auto& sv = *(act_slots.begin());
  if(!check_informable_slot_value(sv.first, sv.second, ontology))
    return false;
  return true;
}

bool check_one_requestable_constraint(const std::list< std::pair<std::string, std::string> >& act_slots,
				      const belief_tracker::Ontology& ontology) {
  // One slot/value pair (in the informable)
  if(act_slots.size() != 1) {
    std::cerr << "Got " << act_slots.size() << " slot/value pairs, expecting 1." << std::endl;
    return false;
  }
  auto& sv = *(act_slots.begin());
  return is_slot_requestable(sv.first, ontology);
}
bool check_offer_constraint(const std::list< std::pair<std::string, std::string> >& act_slots,
			    const belief_tracker::Ontology& ontology) {
  // One slot/value pair (in the informable)
  if(act_slots.size() != 1) {
    std::cerr << "Got " << act_slots.size() << " slot/value pairs, expecting 1." << std::endl;
    return false;
  }
  auto& sv = *(act_slots.begin());
  if(sv.first != std::string("name")) {
    std::cerr << "The offer act can only be for the name slot" << std::endl;
    return false;
  }
  return check_informable_slot_value(sv.first, sv.second, ontology);
}
bool check_machine_request_constraint(const std::list< std::pair<std::string, std::string> >& act_slots,
				      const belief_tracker::Ontology& ontology) {
  // One slot/value pair (in the informable)
  if(act_slots.size() != 1) {
    std::cerr << "Got " << act_slots.size() << " slot/value pairs, expecting 1." << std::endl;
    return false;
  }
  auto& sv = *(act_slots.begin());
  if(sv.first != std::string("slot")) {
    std::cerr << "The request act must be on a \"slot\", not \"" << sv.first << "\"" << std::endl;
    return false;
  }
  return is_slot_informable(sv.second, ontology);
}


typedef std::map<std::string, std::function<bool(const std::list< std::pair<std::string, std::string> >&, const belief_tracker::Ontology&)> > constraints_type;


// Definition of the valid machine acts
constraints_type valid_machine_acts({
    {"affirm",&check_empty_constraint}, 
      {"bye", &check_empty_constraint}, 
	{"canthear", &check_empty_constraint},
	  {"confirm-domain", &check_empty_constraint},
	    {"negate", &check_empty_constraint},
	      {"repeat", &check_empty_constraint},
		{"reqmore", &check_empty_constraint},
		  {"welcomemsg", &check_empty_constraint},
		    {"canthelp", &check_canthelp_constraint},
		      {"canthelp.missing_slot_value", &check_canthelp_missing_slot_value_constraint},
			{"expl-conf", &check_one_informable_constraint},
			  {"impl-conf", &check_one_informable_constraint},
			    {"inform", &check_one_requestable_constraint},
			      {"offer", &check_offer_constraint},
				{"request", &check_machine_request_constraint},
				  {"select", &check_one_informable_constraint}
  }
  );

constraints_type valid_user_acts({
    {"ack",&check_empty_constraint}, 
      {"affirm",&check_empty_constraint}, 
	{"bye", &check_empty_constraint}, 
	  {"hello", &check_empty_constraint},
	    {"help", &check_empty_constraint},
	      {"negate", &check_empty_constraint},
		{"null", &check_empty_constraint},
		  {"repeat", &check_empty_constraint},
		    {"reqalts", &check_empty_constraint},
		      {"reqmore", &check_empty_constraint},
			{"restart", &check_empty_constraint},
			  {"silence", &check_empty_constraint},
			    {"thankyou", &check_empty_constraint},
			      {"confirm", &check_one_informable_constraint},
				{"deny", &check_one_informable_constraint},
				  {"inform", &check_one_informable_constraint},
				    {"request", &check_one_requestable_constraint}
  }
  );



bool validate_act(const belief_tracker::DialogAct& act, 
		  const constraints_type& constraints,
		  const belief_tracker::Ontology& ontology) {
  auto iter_constraint = constraints.find(act.act);
  if(iter_constraint == constraints.end()) {
    std::cerr << "The act \"" << act.act << "\" is unrecognized !" << std::endl;
    return false;
  }
  return iter_constraint->second(act.act_slots, ontology);
}


std::list<belief_tracker::DialogAct> parse_str_to_dialog_acts(std::string str,
							      const constraints_type& constraints,
							      const belief_tracker::Ontology& ontology) {
  std::list<belief_tracker::DialogAct> acts;

  std::istringstream ss(str);
  std::string token;


  
    while(std::getline(ss, token, ')')) {
      // token must be :
      // act(
      // act(s=v
      // act(s1=v1,s2=v2

      auto opening_commas_index = find_position_of_char(token, '(');

      if(opening_commas_index.size() != 1) {
	std::cerr << "While parsing " << token << ", I was expecting a single opening comma in an individual act" << std::endl;
	continue;
      }

      auto opening_comma_index = opening_commas_index[0];

      // We can already define the act type
      belief_tracker::DialogAct dact;
      dact.act = token.substr(0, opening_comma_index);

      bool skip = false;

      // We now deal with optional slot/value pairs
      if(opening_comma_index < token.size() - 1) {
	// We have at least one slot/value pair, separated by commas
	// e.g. 
	// pricerange=cheap,food=french
	// area=centre
	std::string slot_value_str = token.substr(opening_comma_index+1);
	std::istringstream ss_slot_value(slot_value_str);
	std::string token_slot_value;
  
	while(std::getline(ss_slot_value, token_slot_value, ',')) {
	  // We must have token_slot_value ==  s=v
	  auto sv_sep_index = find_position_of_char(token_slot_value, '=');
	  if(sv_sep_index.size() != 1) {
	    std::cerr << "While parsing the arguments of " << token << ", I was expecting an '=' sign separating the slot and value but did not found one" << std::endl;
	    skip = true;
	    break;
	  }

	  auto equal_sign_pos = sv_sep_index[0];
	  std::string slot_name = token_slot_value.substr(0, equal_sign_pos);
	  std::string value_name = token_slot_value.substr(equal_sign_pos+1);
	  dact.act_slots.push_back(std::make_pair(slot_name, 
						  value_name));
	}

      }

      if(!skip && validate_act(dact, constraints, ontology)) acts.push_back(dact);

    }
  return acts;
}

belief_tracker::SluHyps parse_str_to_slu(std::string str,
					 const constraints_type& constraints,
					 const belief_tracker::Ontology& ontology) {

  belief_tracker::SluHyps hyps;

  std::istringstream ss(str);
  std::string token;

  // We suppose that the string is :
  // request(slot=phone)inform(food=french):0.5;negate():0.3;null():0.2


  while(std::getline(ss, token, ';')) {

    std::istringstream ss_hyp(token);
    std::string utterance;
    std::string score_str;
    std::getline(ss_hyp, utterance,':');
    std::getline(ss_hyp, score_str,':');
    if(score_str.size() == 0) {
      std::cout << "You forgot the score .... " << std::endl;
      std::cout << "I remind you the input should be like : request(slot=phone)inform(food=french):0.5" << std::endl;
      break;
    }

    double score = std::stod(score_str);

    std::istringstream ss_utterance(utterance);
    std::string token_utterance;

    hyps.push_back(std::make_pair(parse_str_to_dialog_acts(utterance, constraints, ontology), score));
  }
  return hyps;
}

int main(int argc, char * argv[]) {

  if(argc != 2) {
    std::cerr << "Usage : " << argv[0] << " ontology" << std::endl;
    return -1;
  }

  std::string ontology_filename = argv[1];

  // We parse the ontology
  belief_tracker::Ontology ontology = belief_tracker::parse_ontology_json_file(ontology_filename);
  // Initialize the static fields of the converter
  belief_tracker::Converter::init(ontology);

  std::cout << "e.g request(slot=food)inform(pricerange=cheap)canthelp(pricerange=cheap,food=french)" << std::endl;
  std::cout << " impl-conf(food=french) " << std::endl;
  std::cout << " negate()inform(food=english):1" << std::endl;
  
  while(true) {
    std::cout << std::string(10, '*') << std::endl;

    std::string uact_str;
    std::string mact_str;
    
    std::cout << "Machine act : " << std::endl;
    std::getline(std::cin, mact_str);

    auto macts = parse_str_to_dialog_acts(mact_str, valid_machine_acts, ontology);
    std::cout << "   -> Recognized : " << belief_tracker::dialog_acts_to_str(macts) << std::endl;
    std::cout << std::endl;

    std::cout << "User act : " << std::endl;
    std::getline(std::cin, uact_str);
    auto slu_hyps = parse_str_to_slu(uact_str, valid_user_acts, ontology);
    std::cout << "   -> Recognized : " << belief_tracker::slu_hyps_to_str(slu_hyps) << std::endl;
    std::cout << std::endl;


    // Rewrite
    this_reference::rewrite_slu(slu_hyps, macts, false);
    std::cout << "   -> Post processed hypotheses : " << slu_hyps << std::endl;
    std::cout << std::endl;
    auto turn_info = info::extract_info(slu_hyps, macts, true);
    std::cout << "I extracted the following informations : " << std::endl;
    std::cout << turn_info;
  }

}



