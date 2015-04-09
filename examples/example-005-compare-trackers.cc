// In this example, we load two trackers and compare their results on
// the labels in order to discriminate in which conditions one is
// performing better than the other

#include <belief-tracker.h>
#include <fstream>
#include <sstream>
#include <iomanip>

void display_tracker_statistics(std::map<std::string, int>& measures) {
  std::cout << "The tracker did " << measures["tot_nb_mistakes"] << " mistakes " << std::endl;
  std::cout << "At least 1 mistake was done in " << measures["nb_dialogs_at_least_one_mistake"] << " dialogs" << std::endl;
  std::cout << "There were a total number of " << measures["tot_nb_turns"] << " turns" << std::endl;
  std::cout << "At least 1 mistake was done in " << measures["nb_turns_at_least_one_mistake"] << " turns" << " acc = " << 1.0 - (double)measures["nb_turns_at_least_one_mistake"]/measures["tot_nb_turns"] << std::endl;

  std::cout << std::endl;
}



int main(int argc, char * argv[]) {

  if(argc != 8 && argc != 9) {
    std::cerr << "Usage : " << argv[0] << " filename.flist ontology.json tracker1_output.json use_joint tracker2_output.json use_joint verbose(0,1) <session-id>" << std::endl;
    return -1;
  }

  std::string flist_filename = argv[1];
  std::string ontology_filename = argv[2];
  std::string first_tracker_filename = argv[3];
  bool first_tracker_use_joint = atoi(argv[4]);
  std::string second_tracker_filename = argv[5];
  bool second_tracker_use_joint = atoi(argv[6]);
  bool verbose = atoi(argv[7]);

  bool process_a_single_dialog = false;
  std::string session_to_process;
  if(argc == 9) {
    process_a_single_dialog = true;
    session_to_process = std::string(argv[8]);
  }

  std::string outfilename_first("differences_first_tracker_to_labels.txt");
  std::string outfilename_second("differences_second_tracker_to_labels.txt");
  std::string outfilename_ftos("differences_first_to_second_tracker.txt");
  std::string outfilename_stof("differences_second_to_first_tracker.txt");

  std::ofstream outfile_first(outfilename_first.c_str());
  std::ofstream outfile_second(outfilename_second.c_str());
  std::ofstream outfile_ftos(outfilename_ftos.c_str());
  std::ofstream outfile_stof(outfilename_stof.c_str());
  
  
  belief_tracker::Ontology ontology = belief_tracker::parse_ontology_json_file(ontology_filename);
  // Initialize the static fields of the Goal
  belief_tracker::Converter::init(ontology);


  // We parse the flist to access to the dialogs
  std::cout << "Parsing the flist file for getting the dialog and label files" << std::endl;
  auto dialog_label_fullpath = belief_tracker::parse_flist(flist_filename);
  std::cout << "I parsed " << dialog_label_fullpath.size() << " dialogs (and labels if present) " << std::endl;
  
  // We parse the tracker output
  std::cout << "Parsing the first tracker output" << std::endl;
  auto first_tracker_output = belief_tracker::parse_tracker_output_json_file(first_tracker_filename);
  std::cout << "I parsed " << first_tracker_output.sessions_turns.size() << " sessions from the tracker output" << std::endl;

  std::cout << "Parsing the second tracker output" << std::endl;
  auto second_tracker_output = belief_tracker::parse_tracker_output_json_file(second_tracker_filename);
  std::cout << "I parsed " << second_tracker_output.sessions_turns.size() << " sessions from the tracker output" << std::endl;


  unsigned int dialog_index = 0;
  unsigned int number_of_dialogs = dialog_label_fullpath.size();
  auto first_tracker_output_session_iter = first_tracker_output.sessions_turns.begin();
  auto second_tracker_output_session_iter = second_tracker_output.sessions_turns.begin();

  // Some collections to gather statistics
  // For the trackers considered individually
  std::map<std::string, int> first_tracker_measures {
    {"tot_nb_mistakes", 0},
      {"tot_nb_turns", 0},
	{"nb_dialogs_at_least_one_mistake", 0},
	  {"nb_turns_at_least_one_mistake", 0}
  };

  std::map<std::string, int> second_tracker_measures {
    {"tot_nb_mistakes", 0},
      {"tot_nb_turns", 0},
	{"nb_dialogs_at_least_one_mistake", 0},
	  {"nb_turns_at_least_one_mistake", 0},
	    {"nb_first_turns", 0}
  };

  int first_worse_than_second_nb_turns = 0;
  int second_worse_than_first_nb_turns = 0;


  // We now iterate over the dialogs
  for(auto& dlfpi: dialog_label_fullpath) {

    std::string dialog_fullpath = dlfpi[0];
    std::string label_fullpath = dlfpi[1];
    std::string session_id = dlfpi[2];
    
    if(process_a_single_dialog && session_to_process != session_id) {
      ++first_tracker_output_session_iter;
      ++second_tracker_output_session_iter;
      continue;
    }

    if(first_tracker_output_session_iter->first != session_id ||
       second_tracker_output_session_iter->first != session_id) {
      std::cout << "Wrong session number : " << std::endl;
      std::cout << "Dialog session id : " << session_id << std::endl;
      std::cout << "First TrackerOutput(" << first_tracker_output_session_iter->first << ")" << std::endl;
       std::cout << "Second TrackerOutput(" << second_tracker_output_session_iter->first << ")" << std::endl;     
      ::exit(-1);
      ++first_tracker_output_session_iter;
      continue;
    }

    belief_tracker::Dialog dialog;
    belief_tracker::DialogLabels labels;

    unsigned int nb_turns;
    dialog = belief_tracker::parse_dialog_json_file(dialog_fullpath);
    labels = belief_tracker::parse_label_json_file(label_fullpath);
    std::cout << '\r' << "Processing dialog " << dialog_index+1 << "/" << number_of_dialogs << std::flush;
    if(verbose) std::cout << std::endl;

    std::map<std::string, int> first_local_measures {
      {"nb_differences", 0},
	    {"nb_turns_at_least_one_mistake", 0}
    };
    std::map<std::string, int> second_local_measures {
      {"nb_differences", 0},
	    {"nb_turns_at_least_one_mistake", 0}
    };

    //////////////////////////////////////////////
    // We now iterate over the turns of the dialog
    nb_turns = dialog.turns.size();
    std::list<belief_tracker::DialogTurn>::iterator dialog_turn_iter = dialog.turns.begin();
    auto first_tracker_session_turn_iter = first_tracker_output_session_iter->second.begin();
    auto second_tracker_session_turn_iter = second_tracker_output_session_iter->second.begin();

    for(unsigned int turn_index = 0; turn_index < nb_turns ; ++turn_index, ++dialog_turn_iter, ++first_tracker_session_turn_iter, ++second_tracker_session_turn_iter) {

      if(verbose) std::cout << "****** Turn " << turn_index << " ***** " << dialog.session_id << std::endl;
      auto& slu_hyps = dialog_turn_iter->user_acts;
      auto& macts = dialog_turn_iter->machine_acts;

      if(verbose) {
	std::cout << "Machine act : " << macts << std::endl;
	std::cout << "SLU hyps : " << std::endl;
	std::cout << slu_hyps << std::endl;
      }

      // Extract the 1-best hypothesis from the tracker
      auto best_goal_first = belief_tracker::extract_1best_from_tracker_session(*first_tracker_session_turn_iter, first_tracker_use_joint);
      if(verbose) std::cout << "1 best goal from first tracker " << best_goal_first.toStr() << std::endl;
      auto best_goal_second = belief_tracker::extract_1best_from_tracker_session(*second_tracker_session_turn_iter, second_tracker_use_joint);
      if(verbose) std::cout << "1 best goal from second tracker " << best_goal_second.toStr() << std::endl;

      // Extract the labels
      auto labels_turn_i = labels.turns[turn_index].goal_labels;
      if(verbose) std::cout << "Labels : " << belief_tracker::goal_labels_to_str(labels_turn_i) << std::endl;

      int nb_differences_first = belief_tracker::nb_differences_goal_to_label(best_goal_first, labels_turn_i);
      if(verbose) std::cout << "Number of differences 1st tracker : " << nb_differences_first << std::endl;

      int nb_differences_second = belief_tracker::nb_differences_goal_to_label(best_goal_second, labels_turn_i);
      if(verbose) std::cout << "Number of differences 2nd tracker : " << nb_differences_second << std::endl;

      first_local_measures["nb_differences"] += nb_differences_first;
      if(nb_differences_first > 0) {
	first_local_measures["nb_turns_at_least_one_mistake"] += 1;
      }
      second_local_measures["nb_differences"] += nb_differences_second;
      if(nb_differences_second > 0) {
	second_local_measures["nb_turns_at_least_one_mistake"] += 1;
      }

      //************ Update the statistics
      if(nb_differences_first > 0) {
	// We made at least one mistake in this turn
	// Dump the session id, turn , nb mistakes as well as the machine acts of this turn
	outfile_first << session_id << "-t" << turn_index << "\t" << nb_differences_first << "\t" << macts << std::endl;
      } 

      if(nb_differences_second > 0) {
	// We made at least one mistake in this turn
	// Dump the session id, turn , nb mistakes as well as the machine acts of this turn
	outfile_second << session_id << "-t" << turn_index << "\t" << nb_differences_second << "\t" << macts << std::endl;
      } 


      if(nb_differences_first > nb_differences_second) {
	++first_worse_than_second_nb_turns;
	outfile_ftos << session_id << "-t" << turn_index  << "\t +" << nb_differences_first-nb_differences_second << "\t" << macts << "\t";       
	auto best_goal_first_iter = best_goal_first.slots_values.begin();
	auto best_goal_second_iter = best_goal_second.slots_values.begin();
	unsigned int Ninformable = belief_tracker::Converter::get_informable_slots_size();
	for(unsigned int slot_index = 0 ; slot_index < Ninformable ; ++slot_index, ++best_goal_first_iter, ++best_goal_second_iter) {
	  std::string slot_name = belief_tracker::Converter::get_slot(slot_index);
	  std::string label_value_name;
	  auto labels_turn_i_iter = labels_turn_i.find(slot_name);
	  if(labels_turn_i_iter != labels_turn_i.end())
	    label_value_name = labels_turn_i_iter->second;
	  else
	    label_value_name = belief_tracker::SYMBOL_UNKNOWN;
	  std::string value_first = belief_tracker::Converter::get_value(slot_index, *best_goal_first_iter);
	  std::string value_second = belief_tracker::Converter::get_value(slot_index, *best_goal_second_iter);
	  if(value_second == label_value_name && value_first != label_value_name)
	    outfile_ftos << slot_name << "=(" << value_first << "," << value_second << "," << label_value_name << ") ";
	}
	outfile_ftos << std::endl;
      }
      else if(nb_differences_second > nb_differences_first) {
	++second_worse_than_first_nb_turns;
	outfile_stof << session_id << "-t" << turn_index  << "\t +" << nb_differences_second-nb_differences_first << "\t" << macts << "\t";
	auto best_goal_first_iter = best_goal_first.slots_values.begin();
	auto best_goal_second_iter = best_goal_second.slots_values.begin();
	unsigned int Ninformable = belief_tracker::Converter::get_informable_slots_size();
	for(unsigned int slot_index = 0 ; slot_index < Ninformable ; ++slot_index, ++best_goal_first_iter, ++best_goal_second_iter) {
	  std::string slot_name = belief_tracker::Converter::get_slot(slot_index);
	  std::string label_value_name;
	  auto labels_turn_i_iter = labels_turn_i.find(slot_name);
	  if(labels_turn_i_iter != labels_turn_i.end())
	    label_value_name = labels_turn_i_iter->second;
	  else
	    label_value_name = belief_tracker::SYMBOL_UNKNOWN;
	  std::string value_first = belief_tracker::Converter::get_value(slot_index, *best_goal_first_iter);
	  std::string value_second = belief_tracker::Converter::get_value(slot_index, *best_goal_second_iter);
	  if(value_first == label_value_name && value_second != label_value_name)
	    outfile_stof << slot_name << "=(" << value_second << "," << value_first << "," << label_value_name << ") ";
	}
	outfile_stof << std::endl;
      }

      //***********
    } // Next turn;

    first_tracker_measures["tot_nb_mistakes"] += first_local_measures["nb_differences"];
    first_tracker_measures["tot_nb_turns"] += nb_turns;
    first_tracker_measures["nb_dialogs_at_least_one_mistake"] += (first_local_measures["nb_differences"] > 0 ? 1 : 0);
    first_tracker_measures["nb_turns_at_least_one_mistake"] += first_local_measures["nb_turns_at_least_one_mistake"];

    second_tracker_measures["tot_nb_mistakes"] += second_local_measures["nb_differences"];
    second_tracker_measures["tot_nb_turns"] += nb_turns;
    second_tracker_measures["nb_dialogs_at_least_one_mistake"] += (second_local_measures["nb_differences"] > 0 ? 1 : 0);
    second_tracker_measures["nb_turns_at_least_one_mistake"] += second_local_measures["nb_turns_at_least_one_mistake"];


    ++first_tracker_output_session_iter;
    ++second_tracker_output_session_iter;
    ++dialog_index;

    if(verbose) std::cout << std::endl;
  }
  std::cout << std::endl;

  outfile_first.close();
  outfile_second.close();
  outfile_ftos.close();
  outfile_stof.close();
  

  std::cout << "There were a total number of " << number_of_dialogs << " dialogs" << std::endl;
  std::cout << std::string(20,'*') << std::endl;
  std::cout << "First tracker " << std::endl;
  display_tracker_statistics(first_tracker_measures);
  

  std::cout << std::string(20,'*') << std::endl;
  std::cout << "Second tracker " << std::endl;
  display_tracker_statistics(second_tracker_measures);

  std::cout << std::string(20,'*') << std::endl;
  std::cout << "Comparison of the two trackers " << std::endl;
  std::cout << "The first tracker performed worse than the second in " << first_worse_than_second_nb_turns << " turns" << std::endl;
  std::cout << "The second tracker performed worse than the first in " << second_worse_than_first_nb_turns << " turns" << std::endl;
  
  std::cout << std::endl;
  std::cout << "Results saved in " 
	    << outfilename_first << "," 
	    << outfilename_second << "," 
	    << outfilename_ftos << "," 
	    << outfilename_stof << std::endl;
}
