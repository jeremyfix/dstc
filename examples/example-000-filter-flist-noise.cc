#include <belief-tracker.h>
#include <sstream>
#include <fstream>
#include <cstring>
#include <boost/filesystem.hpp>


int main(int argc, char * argv[]) {

  if(argc != 5 && argc != 6) {
    std::cerr << "Usage : " << argv[0] << " filename.flist ontology noise verbose(0,1) <sessionid>" << std::endl;
    return -1;
  }

  std::string flist_filename = argv[1];

  boost::filesystem::path pathname(flist_filename);
  std::string flist_filename_basename = pathname.filename().string();

  std::string ontology_filename = argv[2];
  double noise = atof(argv[3]);
  bool verbose = atoi(argv[4]); 
  std::string session_to_process("");
  if(argc == 6)
    session_to_process = argv[5];

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

  std::vector<std::string> dialogs_large_noise;
  std::vector<std::string> dialogs_small_noise;

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

    bool one_turn_with_large_noise = false;
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
      
      if(verbose) {
	std::cout << "Machine act : " << belief_tracker::dialog_acts_to_str(macts) << std::endl;
	std::cout << "SLU hyps : " << std::endl;
	std::cout << belief_tracker::slu_hyps_to_str(slu_hyps) << std::endl;
      }

      if(has_null_larger_than(slu_hyps, noise)) {
	if(verbose)
	  std::cout << "Large null , I stop the dialog ." << std::endl;
	one_turn_with_large_noise = true;
	break;
      }


    } // End of looping over the turns

    if(one_turn_with_large_noise)
      dialogs_large_noise.push_back(dialog_entry); // We save the line of the flist to generate a new list file
    else 
      dialogs_small_noise.push_back(dialog_entry);
  }

  std::cout << std::endl;
  std::cout << "Of the " << number_of_dialogs << " dialogs," << dialogs_large_noise.size() << " have a large noise and " << dialogs_small_noise.size() << " have a small noise" << std::endl;

  // We processed a single session, we don't dump the results
  if(session_to_process == "") {
    std::ofstream outfile;

    std::ostringstream ostr("");
    ostr << "large_noise-" << flist_filename_basename;
    outfile.open(ostr.str().c_str());
    for(auto l: dialogs_large_noise)
      outfile << l << std::endl;
    outfile.close();
    std::cout << "The dialogs with one turn with a large noise have been saved into " << ostr.str() << std::endl;

    ostr.str("");
    ostr << "small_noise-" << flist_filename_basename;
    outfile.open(ostr.str().c_str());
    for(auto l: dialogs_small_noise)
      outfile << l << std::endl;
    outfile.close();
    std::cout << "The dialogs with one turn with a small noise have been saved into " << ostr.str() << std::endl;
  }
  
}
