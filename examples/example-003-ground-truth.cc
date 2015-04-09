#include <belief-tracker.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <random>
#include <set>
#include <iterator> 
#include <boost/filesystem.hpp>

// In this example, we make use of the "semantics" in the labels
// in order to simulate a perfect SLU (actually the labeller)
// We extract the informations from this semantics and update the belief

namespace info = belief_tracker::info::single_info;
typedef belief_tracker::joint::sum::Belief belief_type;
namespace this_reference = belief_tracker::this_reference::single;


//namespace info = belief_tracker::info::multi_info;
//typedef belief_tracker::joint::sum::Belief belief_type;
//namespace this_reference = belief_tracker::this_reference::weak_dontcare;


belief_tracker::SluHyps extract_slu_from_semantics(const belief_tracker::Uterance& semantics) {
  return { std::make_pair(semantics, 1.0) };
}

int main(int argc, char * argv[]) {

  if(argc != 4 && argc != 5) {
    std::cerr << "Usage : " << argv[0] << " filename.flist ontology verbose(0,1) <session-id>" << std::endl;
    return -1;
  }

  std::string flist_filename = argv[1];
  std::string ontology_filename = argv[2];
  bool verbose = atoi(argv[3]);  

  bool process_a_single_dialog = false;
  std::string session_to_process;
  if(argc == 5) {
    process_a_single_dialog = true;
    session_to_process = std::string(argv[4]);
  }

  // We define the set of rules to be used for extracting the informations
  // To get 100% accuracy, one should comment out the implconf rule
  std::list<belief_tracker::info::single_info::rules::rule_type> rules {
    belief_tracker::info::single_info::rules::inform,
      belief_tracker::info::single_info::rules::explconf,
      belief_tracker::info::single_info::rules::implconf,
      belief_tracker::info::single_info::rules::negate,
      belief_tracker::info::single_info::rules::deny
      };

  // We parse the ontology
  belief_tracker::Ontology ontology = belief_tracker::parse_ontology_json_file(ontology_filename);

  // Initialize the static fields of the converter
  belief_tracker::Converter::init(ontology);

  // We parse the flist to access to the dialogs
  if(verbose) std::cout << "Parsing the flist file for getting the dialog and label files" << std::endl;
  auto dialog_label_fullpath = belief_tracker::parse_flist(flist_filename);
  if(verbose) std::cout << "I parsed " << dialog_label_fullpath.size() << " dialogs (and labels if present) " << std::endl;
  if(verbose) std::cout << "done " << std::endl;

  // The structure holding the output
  belief_tracker::TrackerOutput tracker_output;

  // Compute and set the dataset name
  boost::filesystem::path pathname(flist_filename);
  tracker_output.dataset = pathname.filename().string();
  int lastindex = tracker_output.dataset.find_last_of("."); 
  tracker_output.dataset = tracker_output.dataset.substr(0, lastindex); 

  //////////////////////////////
  // Structures for holding the measures over the dialogs and turns
  std::map<std::string, int> measures {
    {"tot_nb_mistakes", 0},
      {"tot_nb_turns", 0},
	{"nb_dialogs_at_least_one_mistake", 0},
	  {"nb_turns_at_least_one_mistake", 0}
  };

  // A map containing the dialogs on which we make a mistake
  // it maps a session id to the number of errors we make
  std::map<std::string, int> mistaken_dialogs;

  unsigned int dialog_index = 0;
  unsigned int number_of_dialogs  = dialog_label_fullpath.size();
  unsigned int nb_turns;

  // Iterate over the dialogs
  for(auto& dlfpi: dialog_label_fullpath) {

    std::string dialog_fullpath = dlfpi[0];
    std::string label_fullpath = dlfpi[1];
    std::string session_id = dlfpi[2];
    std::string dialog_entry = dlfpi[3];

    if(process_a_single_dialog && session_to_process != session_id)
      continue;

    belief_tracker::Dialog dialog;
    belief_tracker::DialogLabels labels;

    std::list<belief_tracker::DialogTurn>::iterator dialog_turn_iter;
    std::list<belief_tracker::DialogAct> prev_mach_acts;

    dialog = belief_tracker::parse_dialog_json_file(dialog_fullpath);
    labels = belief_tracker::parse_label_json_file(label_fullpath);

    std::cout << '\r' << "Processing dialog " << (dialog_index +1) << "/" << number_of_dialogs << std::flush;
    if(verbose) std::cout << std::endl;

    //////////////////////////////////////////////
    // We now iterate over the turns of the dialog
    belief_type belief;
    belief.start();
    if(verbose) std::cout << "Initial belief : " << std::endl << belief << std::endl;

    std::map< std::string, double > requested_slots;
    std::map< std::string, double > methods;
    methods["byalternatives"] = 0.0;
    methods["byconstraints"] = 0.0;
    methods["byname"] = 0.0;
    methods["finished"] = 0.0;
    methods["none"] = 1.0;

    /////////////////////
    // Some initializations before looping over the turns
    nb_turns = dialog.turns.size();
    dialog_turn_iter = dialog.turns.begin();
    tracker_output.sessions_turns.push_back(std::make_pair(dialog.session_id, std::vector<belief_tracker::TrackerSessionTurn>()));

    std::map<std::string, int> local_measures {
      {"nb_differences", 0},
    	    {"nb_turns_at_least_one_mistake", 0}
    };


    // //////////////////////////////////////////////
    // // We now iterate over the turns of the dialog
    std::map< std::string, std::string > cur_goal_label;

   for(unsigned int turn_index = 0; turn_index < nb_turns ; ++turn_index, ++dialog_turn_iter) {
     if(verbose) {
       std::cout << "****** Turn " << turn_index << " ***** " << dialog.session_id << std::endl;
       std::cout << std::endl;
       std::cout << "Dialog path : " << dialog_fullpath << std::endl;
       std::cout << "Label path : " << label_fullpath << std::endl;
       std::cout << std::endl;
     }

      //auto& slu_hyps = dialog_turn_iter->user_acts;
      auto slu_hyps = extract_slu_from_semantics(labels.turns[turn_index].semantics);
      auto& macts = dialog_turn_iter->machine_acts;

      if(verbose) std::cout << "Received machine act : " << belief_tracker::dialog_acts_to_str(macts) << std::endl;
      if(verbose) std::cout << "Received hypothesis : " << std::endl << belief_tracker::slu_hyps_to_str(slu_hyps) << std::endl;

      // We rewrite the repeat machine act by copying the previous machine utterance
      belief_tracker::rewrite_repeat_machine_act(macts, prev_mach_acts);
      belief_tracker::renormalize_slu(slu_hyps);
      this_reference::rewrite_slu(slu_hyps, macts);

      if(verbose) std::cout << "New machine act : " << belief_tracker::dialog_acts_to_str(macts) << std::endl;
      if(verbose) std::cout << "Reprocessed hypothesis : " << std::endl << belief_tracker::slu_hyps_to_str(slu_hyps) << std::endl;

      auto turn_info = info::extract_info(slu_hyps, macts, rules, verbose);

      cur_goal_label = labels.turns[turn_index].goal_labels;

     // We now use the scored info to update the probability distribution
      // over the values of each slot
      // This produces the tracker output for this dialog
      // We make use of a lazy representation, i.e. we
      // just represent the slot/value pairs for which the belief is != 0
      belief = belief.update(turn_info, belief_tracker::info::transition_function);
      
      // Update the requested slots
      belief_tracker::requested_slots::update(slu_hyps, macts, requested_slots);
      belief_tracker::methods::update(slu_hyps, macts, methods);

      auto best_goal = belief.extract_best_goal();

      int nb_differences = belief_tracker::nb_differences_goal_to_label(best_goal, cur_goal_label);

      local_measures["nb_differences"] += nb_differences;

      if(nb_differences > 0) 
    	local_measures["nb_turns_at_least_one_mistake"] += 1;


      /****** For debugging, we take the labels  ******/
      if(verbose) {
	std::cout << "Machine act : " << belief_tracker::dialog_acts_to_str(macts) << std::endl;
	std::cout << "SLU hyps : " << std::endl;
	std::cout << belief_tracker::slu_hyps_to_str(slu_hyps) << std::endl;
	std::cout << "Semantics " << std::endl;
	std::cout << belief_tracker::dialog_acts_to_str(labels.turns[turn_index].semantics) << std::endl;
	std::cout << "Extracted Turn info : " << std::endl;
	std::cout << turn_info << std::endl;
	std::cout << "Belief :" << std::endl;
	std::cout << belief << std::endl;
	std::cout << "Best goal : " << best_goal.toStr() << std::endl;
	std::cout << "Labels : " << labels.turns[turn_index].goal_labels_str << std::endl;
	std::cout << "Nb differences 1best to goal : " << nb_differences << std::endl;
      }
      /******* END DEBUG *****/
      if(verbose) std::cout << std::endl;

      belief_tracker::TrackerSessionTurn tracker_session_turn;
      belief.fill_tracker_session(tracker_session_turn);
      tracker_session_turn.requested_slots_scores = requested_slots;
      tracker_session_turn.method_labels_scores = methods;
      tracker_output.sessions_turns[dialog_index].second.push_back(tracker_session_turn);
      
      // Copy the previous machine acts in order to replace 
      // a repeat if any
      prev_mach_acts = macts;

    }

    // Update the statistics 
    measures["tot_nb_mistakes"] += local_measures["nb_differences"];
    measures["tot_nb_turns"] += nb_turns;
    measures["nb_dialogs_at_least_one_mistake"] += (local_measures["nb_differences"] > 0 ? 1 : 0);
    measures["nb_turns_at_least_one_mistake"] += local_measures["nb_turns_at_least_one_mistake"];

    if(local_measures["nb_differences"] > 0)
      mistaken_dialogs[session_id] = local_measures["nb_differences"];



    if(verbose) std::cout << "fraction of turns the best hyp has 1 diff from the labels : " << double(local_measures["nb_turns_at_least_one_mistake"]) / nb_turns << std::endl;
    
    if(process_a_single_dialog && session_to_process == dialog.session_id) {
      break;
    }
    
    ++dialog_index;
  }
  std::cout << std::endl;

  // At the end, we dump the tracker output in the JSON format
  tracker_output.walltime = 10000; // TODO !!!!
  std::ostringstream ostr;
  ostr.str("");
  ostr << tracker_output.dataset << "-output.json";
  belief_tracker::dump_tracker_output_json_file(tracker_output, ostr.str());
  std::cout << "Output saved to " << ostr.str() << std::endl;

  std::cout << measures["nb_dialogs_at_least_one_mistake"] << "/" << dialog_label_fullpath.size() << "(" << double(measures["nb_dialogs_at_least_one_mistake"])/dialog_label_fullpath.size() * 100. << " %)  dialogs have at least one mistake " << std::endl;
  std::cout << "A total number of " << measures["tot_nb_mistakes"] << " mistakes have been done" << std::endl;
  std::cout << "We made at least one mistake in " << measures["nb_turns_at_least_one_mistake"] << " / " << measures["tot_nb_turns"] << " turns, i.e. " << measures["nb_turns_at_least_one_mistake"] * 100. / measures["tot_nb_turns"] << " % (acc 1a = " << 1.0 - (double)measures["nb_turns_at_least_one_mistake"]  / measures["tot_nb_turns"] <<  ") " << std::endl;
  std::cout << "There were a total of " << measures["tot_nb_turns"] << " turns" << std::endl;

  // We exit here if we were processing a single dialog
  if(process_a_single_dialog)
    return 0;

  // At the end, we dump the tracker output in the JSON format
  std::ofstream outfile;
  outfile.open("mistaken_dialogs.data");
  for(auto& kv: mistaken_dialogs)
    outfile << kv.first << '\t' << kv.second << std::endl;
  outfile.close();
  std::cout << "Mistaken dialogs saved in mistaken_dialogs.data" << std::endl;


}
