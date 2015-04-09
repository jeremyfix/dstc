#include <belief-tracker.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <set>
#include <chrono>
#include <boost/filesystem.hpp>

typedef std::chrono::high_resolution_clock clock_type;

namespace info = belief_tracker::info::multi_info;
typedef belief_tracker::joint::max::Belief belief_type;
namespace this_reference = belief_tracker::this_reference::weak_dontcare;

int main(int argc, char * argv[]) {

  if(argc != 4 && argc != 5) {
    std::cerr << "Usage : " << argv[0] << " filename.flist ontology verbose(0,1) <session-id>" << std::endl;
    return -1;
  }

auto clock_begin = clock_type::now();

  std::string flist_filename = argv[1];
  std::string ontology_filename = argv[2];
  bool verbose = atoi(argv[3]);  

  bool process_a_single_dialog = false;
  std::string session_to_process;
  if(argc == 5) {
    process_a_single_dialog = true;
    session_to_process = std::string(argv[4]);
  }

  // We parse the ontology
  belief_tracker::Ontology ontology = belief_tracker::parse_ontology_json_file(ontology_filename);
  // initialize the static fields of the converter
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
  // Iterate over the dialogs
  unsigned int dialog_index = 0;
  unsigned int number_of_dialogs = dialog_label_fullpath.size();
  unsigned int nb_turns;

  for(auto& dlfpi: dialog_label_fullpath) {

    std::string dialog_fullpath = dlfpi[0];
    std::string label_fullpath = dlfpi[1];
    std::string session_id = dlfpi[2];

    if(process_a_single_dialog && session_to_process != session_id)
      continue;

    belief_tracker::Dialog dialog;
    belief_tracker::DialogLabels labels;

    std::list<belief_tracker::DialogTurn>::iterator dialog_turn_iter;
    std::list<belief_tracker::DialogAct> prev_mach_acts;

    dialog = belief_tracker::parse_dialog_json_file(dialog_fullpath);

    std::cout << '\r' << "Processing dialog " << dialog_index+1 << "/" << number_of_dialogs << std::flush;
    if(verbose)
      std::cout << std::endl;


    ////////////////////////////////////
    // Belief initialization
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



    //////////////////////////////////////////////
    // We now iterate over the turns of the dialog
    for(unsigned int turn_index = 0; turn_index < nb_turns ; ++turn_index, ++dialog_turn_iter) {
      if(verbose) std::cout << "****** Turn " << turn_index << " ***** " << dialog.session_id << std::endl;
      auto& slu_hyps = dialog_turn_iter->user_acts;
      auto& macts = dialog_turn_iter->machine_acts;

      if(verbose) std::cout << "Received machine act : " << belief_tracker::dialog_acts_to_str(macts) << std::endl;
      if(verbose) std::cout << "Received hypothesis : " << std::endl << belief_tracker::slu_hyps_to_str(slu_hyps) << std::endl;

      // We rewrite the repeat machine act by copying the previous machine utterance
      belief_tracker::renormalize_slu(slu_hyps);
      belief_tracker::rewrite_repeat_machine_act(macts, prev_mach_acts);
      this_reference::rewrite_slu(slu_hyps, macts);
      if(verbose) std::cout << "New machine act : " << belief_tracker::dialog_acts_to_str(macts) << std::endl;
      if(verbose) std::cout << "Reprocessed hypothesis : " << std::endl << belief_tracker::slu_hyps_to_str(slu_hyps) << std::endl;

      auto turn_info = info::extract_info(slu_hyps, macts, verbose);

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
      
      /****** For debugging, we take the labels  ******/
      if(verbose) {
	std::cout << "Machine act : " << belief_tracker::dialog_acts_to_str(macts) << std::endl;
	std::cout << "SLU hyps : " << std::endl;
	std::cout << belief_tracker::slu_hyps_to_str(slu_hyps) << std::endl;
	std::cout << "Extracted Turn info : " << std::endl;
	std::cout << turn_info << std::endl;
	std::cout << "Belief :" << std::endl;
	std::cout <<  belief << std::endl;
	std::cout << "Best goal : " << best_goal.toStr() << std::endl;
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
    ++dialog_index;

    if(process_a_single_dialog && session_to_process == dialog.session_id) {
      break;
    }

  }
  std::cout << std::endl;

  // Take the clock for computing the walltime
  auto clock_end = clock_type::now();
  double walltime = std::chrono::duration_cast<std::chrono::seconds>(clock_end - clock_begin).count();

  // At the end, we dump the tracker output in the JSON format
  tracker_output.walltime = walltime;
  std::ostringstream ostr;
  ostr.str("");
  ostr << tracker_output.dataset << "-output.json";
  belief_tracker::dump_tracker_output_json_file(tracker_output, ostr.str());
  std::cout << "Output saved to " << ostr.str() << std::endl;
}



