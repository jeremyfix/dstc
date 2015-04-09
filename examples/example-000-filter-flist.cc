#include <belief-tracker.h>
#include <sstream>
#include <fstream>
#include <cstring>
#include <boost/filesystem.hpp>

namespace this_reference = belief_tracker::this_reference::weak_dontcare;

bool is_slot_value_in_dact(std::string slot,
			   std::string value,
			   const belief_tracker::DialogAct& dact) {
  for(auto& sv: dact.act_slots) {
    if(sv.first == slot && sv.second == value)
      return true;
  }
  return false;
}

bool is_slot_value_in_mact(std::string slot,
			   std::string value,
			   const std::list<belief_tracker::DialogAct>& macts) {
  for(auto& mact_i: macts) 
    if(is_slot_value_in_dact(slot, value, mact_i))
      return true;
  return false;
}

bool is_slot_value_in_slu(std::string slot,
			   std::string value,
			  const std::list< std::pair< std::list<belief_tracker::DialogAct>, double > > &user_acts) {
  for(auto& slu_hyp_score_i: user_acts) {
    for(auto& dact: slu_hyp_score_i.first)
      if(is_slot_value_in_dact(slot, value, dact))
	return true;
  }
  return false;
}

std::string goal_label_to_str(const std::map<std::string, std::string>& goal_label) {
  std::string str("(");
  for(unsigned int i = 0 ; i < belief_tracker::Converter::get_informable_slots_size() ; ++i) {
    std::string slot_name = belief_tracker::Converter::get_slot(i);
    str += slot_name + "=";
    auto it = goal_label.find(slot_name);
    if(it != goal_label.end())
      str += it->second;
    else
      str += belief_tracker::SYMBOL_UNKNOWN;
    
    if(i != belief_tracker::Converter::get_informable_slots_size() -1)
      str += ",";
  }

  str += ")";
  return str;
}

// Measure the difference between the goal labels
// e.g. 
// res = compute_delta_goal_label(prev, cur);
// res["pricerange"] = {"?", "cheap"};
std::map<std::string, std::pair<std::string, std::string> > compute_delta_goal_label(const std::map<std::string, std::string>& prev_goal_label,
									     const std::map<std::string, std::string>& cur_goal_label) {
  std::map<std::string, std::pair<std::string, std::string> > delta;
  // We iterate over the slots in the prev_goal_label
  for(auto& kv_prev: prev_goal_label) {
    std::string slot_name = kv_prev.first;
    std::string prev_value = kv_prev.second;
    // Does the current goal contain the slot slot_name ?
    auto it_cur = cur_goal_label.find(slot_name);
    // If the current goal contains the slot slot_name
    if(it_cur != cur_goal_label.end()) {
      std::string cur_value = it_cur->second;
      // If the value is different, we collect it
      if(cur_value != prev_value) {
	delta[slot_name] = std::make_pair(prev_value, cur_value);
      }
      // Otherwise we don't need to collect it
    }
    else {
      // It was present but no more present, we collect it
      delta[slot_name] = std::make_pair(prev_value, belief_tracker::SYMBOL_UNKNOWN);
    }
  }

  // Now, to finish, we iterate over the slots in cur_goal_label 
  // and consider only the slots not present in prev_goal_labels
  for(auto& kv_cur: cur_goal_label) {
    std::string slot_name = kv_cur.first;
    std::string cur_value = kv_cur.second;
    auto it_prev = prev_goal_label.find(slot_name);
    if(it_prev == prev_goal_label.end()) {
      delta[slot_name] = std::make_pair(belief_tracker::SYMBOL_UNKNOWN, cur_value);
    }
  }
  return delta;
}

int main(int argc, char * argv[]) {

  if(argc != 4 && argc != 5) {
    std::cerr << "Usage : " << argv[0] << " filename.flist ontology verbose(0,1) <sessionid>" << std::endl;
    return -1;
  }

  std::string flist_filename = argv[1];

  boost::filesystem::path pathname(flist_filename);
  std::string flist_filename_basename = pathname.filename().string();

  std::string ontology_filename = argv[2];
  bool verbose = atoi(argv[3]); 
  std::string session_to_process("");
  if(argc == 5)
    session_to_process = argv[4];

  // We parse the ontology
  belief_tracker::Ontology ontology = belief_tracker::parse_ontology_json_file(ontology_filename);

  // Initialize the static fields of the converter
  belief_tracker::Converter::init(ontology);

  // We parse the flist to access to the dialogs
  if(verbose) std::cout << "Parsing the flist file for getting the dialog and label files" << std::endl;
  auto dialog_label_fullpath = belief_tracker::parse_flist(flist_filename);
  if(verbose) std::cout << "I parsed " << dialog_label_fullpath.size() << " dialogs (and labels if present) " << std::endl;
  if(verbose) std::cout << "done " << std::endl;

  unsigned int dialog_counter = 0;
  unsigned int number_of_dialogs  = dialog_label_fullpath.size();
  unsigned int nb_turns;

  std::vector<std::string> dialogs_to_keep;
  std::vector<std::string> dialogs_to_discard; // Contains session_id-turn_index

  // Iterate over the dialogs
  for(auto& dlfpi: dialog_label_fullpath) {

    std::string dialog_fullpath = dlfpi[0];
    std::string label_fullpath = dlfpi[1];
    std::string session_id = dlfpi[2];
    std::string dialog_entry = dlfpi[3];

    if(session_to_process != "" &&
       session_to_process != session_id)
      continue;

    belief_tracker::Dialog dialog;
    belief_tracker::DialogLabels labels;

    std::list<belief_tracker::DialogTurn>::iterator dialog_turn_iter;
    std::list<belief_tracker::DialogAct> prev_mach_acts;

    dialog = belief_tracker::parse_dialog_json_file(dialog_fullpath);
    labels = belief_tracker::parse_label_json_file(label_fullpath);

    std::cout << '\r' << "Processing dialog " << dialog_counter++ << "/" << number_of_dialogs << std::flush;
    if(verbose)  std::cout << std::endl;

    /////////////////////
    // Some initializations before looping over the turns
    nb_turns = dialog.turns.size();
    dialog_turn_iter = dialog.turns.begin();
    std::map< std::string, std::string > prev_goal_label;
    std::map< std::string, std::string > cur_goal_label;

    //////////////////////////////////////////////
    // We now iterate over the turns of the dialog

    bool keep_the_dialog = true;
    unsigned int turn_index = 0;
   for(turn_index = 0; turn_index < nb_turns ; ++turn_index, ++dialog_turn_iter) {
      if(verbose) {
	std::cout << "****** Turn " << turn_index << " ***** " << dialog.session_id << std::endl;
	std::cout << std::endl;
	std::cout << "Dialog path : " << dialog_fullpath << std::endl;
	std::cout << "Label path : " << label_fullpath << std::endl;
	std::cout << std::endl;
      }
      auto& slu_hyps = dialog_turn_iter->user_acts;
      auto& macts = dialog_turn_iter->machine_acts;

      if(verbose) std::cout << "Received machine act : " << belief_tracker::dialog_acts_to_str(macts) << std::endl;

      // We rewrite the repeat machine act by copying the previous machine utterance
      belief_tracker::rewrite_repeat_machine_act(macts, prev_mach_acts);
      this_reference::rewrite_slu(slu_hyps, macts);

      cur_goal_label = labels.turns[turn_index].goal_labels;
      // Compute the delta over the goal labels
      auto delta_goal_label = compute_delta_goal_label(prev_goal_label, cur_goal_label);


      if(verbose) {
	std::cout << "Machine act : " << belief_tracker::dialog_acts_to_str(macts) << std::endl;
	std::cout << "SLU hyps : " << std::endl;
	std::cout << belief_tracker::slu_hyps_to_str(slu_hyps) << std::endl;
	std::cout << "Labels    : " << goal_label_to_str(cur_goal_label) << std::endl;
	std::cout << "Delta labels: ";
	for(auto& kv: delta_goal_label) 
	  std::cout << kv.first << "=" << kv.second.first << "->" << kv.second.second << ",";
	std::cout << std::endl;
      }

      // We now check if the transition between the goal labels
      // has the slot/value in the machine act or SLU
      for(auto& kv: delta_goal_label) {
	std::string slot_name = kv.first;
	std::string value_in_cur_goal = kv.second.second;
	bool is_slot_value_in_mact_or_slu = is_slot_value_in_mact(slot_name, value_in_cur_goal, macts) || is_slot_value_in_slu(slot_name, value_in_cur_goal, slu_hyps);
	if(!is_slot_value_in_mact_or_slu) {
	  if(verbose)
	    std::cout << "How do you want me to predict the transition : \"" << slot_name << "= " << kv.second.first << " -> " << kv.second.second << "\" ?" << std::endl;
	  keep_the_dialog=false;
	}
	// No need to check the other transitions,
	// we can break and stop the check of the other differences
	if(!keep_the_dialog)
	  break;
      }
      
      if(verbose) {
	if(keep_the_dialog) std::cout << "Okay, I keep the dialog" << std::endl;
	else std::cout << "The dialog is discarded ! " << std::endl;
	std::cout << std::endl;
      }

      // No need to check the other turns if !keep_dialog
      if(!keep_the_dialog) 
	  break;

      // Copy the previous machine acts in order to replace 
      // a repeat if any
      prev_mach_acts = macts;
      // Copy the previous goal label to measure the delta in the goal label
      prev_goal_label = cur_goal_label;

   } // End of looping over the turns

   if(keep_the_dialog)
     dialogs_to_keep.push_back(dialog_entry); // We save the line of the flist to generate a new list file
   else {
     std::ostringstream ostr("");
     ostr << session_id << "-t" << turn_index;
     dialogs_to_discard.push_back(ostr.str());
   }
  }

  std::cout << std::endl;
  std::cout << "Of the " << number_of_dialogs << " dialogs, I'm keeping " << dialogs_to_keep.size() << " dialogs" << std::endl;

  // We processed a single session, we don't dump the results
  if(session_to_process == "") {
    std::ostringstream ostr("");
    ostr << "filtered-" << flist_filename_basename;
    std::ofstream outfile(ostr.str().c_str());
    for(auto l: dialogs_to_keep)
      outfile << l << std::endl;
    outfile.close();
    std::cout << "The dialogs to keep have been saved into " << ostr.str() << std::endl;

    ostr.str("");
    ostr << "discarded-" << flist_filename_basename << ".data";
    outfile.open(ostr.str().c_str());
    for(auto l: dialogs_to_discard) 
      outfile << l << std::endl;
    outfile.close();
    std::cout << "The list of the discarded dialogs can be found in " << ostr.str() << std::endl;
  }
  
}
