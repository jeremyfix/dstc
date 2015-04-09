#include <belief-tracker.h>
#include <fstream>
#include <sstream>
#include <iomanip>

std::set<std::string> collect_machine_acts(const belief_tracker::Uterance& macts) {
  std::set<std::string> res;

  for(auto& macti: macts) {
    if(macti.act == std::string("canthelp")) {
      std::ostringstream ostr("");
      ostr << "canthelp-" << macti.act_slots.size();
      res.insert(std::string("canthelp"));
      res.insert(ostr.str());
    }
    else
      res.insert(macti.act);
  }
  return res;
}


int main(int argc, char * argv[]) {

  if(argc != 6 && argc != 7) {
    std::cerr << "Usage : " << argv[0] << " filename.flist ontology.json tracker_output.json use_joint verbose(0,1) <session-id>" << std::endl;
    return -1;
  }

  std::string flist_filename = argv[1];
  std::string ontology_filename = argv[2];
  std::string tracker_filename = argv[3];
  bool tracker_use_joint = atoi(argv[4]);
  bool verbose = atoi(argv[5]);

  bool process_a_single_dialog = false;
  std::string session_to_process;
  if(argc == 7) {
    process_a_single_dialog = true;
    session_to_process = std::string(argv[6]);
  }

  std::string outfilename("differences_tracker_to_labels.txt");
  std::ofstream outfile(outfilename.c_str());
  
  belief_tracker::Ontology ontology = belief_tracker::parse_ontology_json_file(ontology_filename);
   // Initialize the static fields of the converter
  belief_tracker::Converter::init(ontology);

  // We parse the flist to access to the dialogs
  std::cout << "Parsing the flist file for getting the dialog and label files" << std::endl;
  auto dialog_label_fullpath = belief_tracker::parse_flist(flist_filename);
  std::cout << "I parsed " << dialog_label_fullpath.size() << " dialogs (and labels if present) " << std::endl;
  
  // We parse the tracker output
  std::cout << "Parsing the tracker output" << std::endl;
  auto tracker_output = belief_tracker::parse_tracker_output_json_file(tracker_filename);
  std::cout << "I parsed " << tracker_output.sessions_turns.size() << " sessions from the tracker output" << std::endl;


  unsigned int dialog_index = 0;
  unsigned int number_of_dialogs = dialog_label_fullpath.size();
  auto tracker_output_session_iter = tracker_output.sessions_turns.begin();

  // Some collections to gather statistics
  std::map<std::string, int> measures {
    {"tot_nb_mistakes", 0},
      {"tot_nb_turns", 0},
	{"nb_dialogs_at_least_one_mistake", 0},
	  {"nb_turns_at_least_one_mistake", 0}
  };


  // We now iterate over the dialogs
  for(auto& dlfpi: dialog_label_fullpath) {

    std::string dialog_fullpath = dlfpi[0];
    std::string label_fullpath = dlfpi[1];
    std::string session_id = dlfpi[2];
    
    if(process_a_single_dialog && session_to_process != session_id) {
      ++tracker_output_session_iter;
      continue;
    }

    if(tracker_output_session_iter->first != session_id) {
      std::cout << "Wrong session number : TrackerOutput(" << tracker_output_session_iter->first << ") ; dialog iterator (" << dlfpi[2] << ")" << std::endl;
      ::exit(-1);
      ++tracker_output_session_iter;
      continue;
    }

    belief_tracker::Dialog dialog;
    belief_tracker::DialogLabels labels;

    unsigned int nb_turns;
    dialog = belief_tracker::parse_dialog_json_file(dialog_fullpath);
    labels = belief_tracker::parse_label_json_file(label_fullpath);
    std::cout << '\r' << "Processing dialog " << dialog_index+1 << "/" << number_of_dialogs << std::flush;
    if(verbose) std::cout << std::endl;

    std::map<std::string, int> local_measures {
      {"nb_differences", 0},
	    {"nb_turns_at_least_one_mistake", 0}
    };


    //////////////////////////////////////////////
    // We now iterate over the turns of the dialog
    nb_turns = dialog.turns.size();
    std::list<belief_tracker::DialogTurn>::iterator dialog_turn_iter = dialog.turns.begin();
    auto tracker_session_turn_iter = tracker_output_session_iter->second.begin();

    unsigned int turn_index = 0;
    for(turn_index = 0; turn_index < nb_turns ; ++turn_index, ++dialog_turn_iter, ++tracker_session_turn_iter) {

      if(verbose) std::cout << "****** Turn " << turn_index << " ***** " << dialog.session_id << std::endl;
      auto& slu_hyps = dialog_turn_iter->user_acts;
      auto& macts = dialog_turn_iter->machine_acts;


      if(verbose) std::cout << "Received machine act : " << macts << std::endl;     
      if(verbose) std::cout << "Received hypothesis : " << std::endl << slu_hyps << std::endl;

      /*
      if(has_null_larger_than(slu_hyps, 0.7)) {
	if(verbose)
	  std::cout << "Large null , I stop the dialog ." << std::endl;
	break;
      }
      */

      // Extract the 1-best hypothesis from the tracker
      auto best_goal = belief_tracker::extract_1best_from_tracker_session(*tracker_session_turn_iter, tracker_use_joint);
      if(verbose) std::cout << "1 best goal from tracker " << best_goal.toStr() << std::endl;

      // Extract the labels
      auto labels_turn_i = labels.turns[turn_index].goal_labels;
      if(verbose) std::cout << "Labels : " << belief_tracker::goal_labels_to_str(labels_turn_i) << std::endl;

      int nb_differences = belief_tracker::nb_differences_goal_to_label(best_goal, labels_turn_i);
      if(verbose) std::cout << "Number of differences : " << nb_differences << std::endl;

      local_measures["nb_differences"] += nb_differences;
      if(nb_differences > 0) {
	local_measures["nb_turns_at_least_one_mistake"] += 1;
      }

      //************ Update the statistics
      if(nb_differences > 0) {
	// We made at least one mistake in this turn
	// Dump the session id, turn , nb mistakes as well as the machine acts of this turn
	outfile << dlfpi[2] << "-t" << turn_index << "\t" << nb_differences << std::endl;
      }
      //***********
    } // Next turn;

    measures["tot_nb_mistakes"] += local_measures["nb_differences"];
    measures["tot_nb_turns"] += turn_index;
    measures["nb_dialogs_at_least_one_mistake"] += (local_measures["nb_differences"] > 0 ? 1 : 0);
    measures["nb_turns_at_least_one_mistake"] += local_measures["nb_turns_at_least_one_mistake"];


    ++tracker_output_session_iter;
    ++dialog_index;

    if(verbose) std::cout << std::endl;
  }
  std::cout << std::endl;

  outfile.close();
  std::cout << "Results saved in " << outfilename << std::endl;

  std::cout << std::endl;
  std::cout << "The tracker did " << measures["tot_nb_mistakes"] << " mistakes " << std::endl;
  std::cout << "There were a total number of " << number_of_dialogs << " dialogs" << std::endl;
  std::cout << "At least 1 mistake was done in " << measures["nb_dialogs_at_least_one_mistake"] << " dialogs" << std::endl;
  std::cout << "There were a total number of " << measures["tot_nb_turns"] << " turns" << std::endl;
  std::cout << "At least 1 mistake was done in " << measures["nb_turns_at_least_one_mistake"] << " turns (acc = " << 1.0 - measures["nb_turns_at_least_one_mistake"]/double(measures["tot_nb_turns"]) << " %)" << std::endl;

  std::cout << std::endl;


}
