#include <belief-tracker.h>
#include <fstream>
#include <sstream>
#include <iomanip>

int main(int argc, char * argv[]) {

  if(argc != 2) {
    std::cerr << "Usage : " << argv[0] << " tracker_output.json" << std::endl;
    return -1;
  }

  std::string tracker_filename = argv[1];

  std::ostringstream outfilename;
  outfilename.str("");
  outfilename << tracker_filename << "-sizes.data";
  std::ofstream outfile(outfilename.str().c_str());
  std::cout << "The output will be saved in " << outfilename.str() << std::endl;

  std::cout << "Parsing the tracker output" << std::endl;
  auto tracker_output = belief_tracker::parse_tracker_output_json_file(tracker_filename);
  std::cout << "Done" << std::endl;

  unsigned int session_index = 1;
  unsigned int nb_sessions = tracker_output.sessions_turns.size();
  for(auto& session_turns: tracker_output.sessions_turns) {
    std::cout << "\r " << session_index << " / " << nb_sessions << std::flush;
    for(auto& turn: session_turns.second) {
      double sum_scores = 0.0;
      for(auto& kv: turn.goal_labels_joint_scores)
	sum_scores += kv.second;

      int nb_joint_goals = turn.goal_labels_joint_scores.size();
      if(sum_scores < 1.0)
	nb_joint_goals += 1; // there is some mass on the unknown hyp

      outfile << nb_joint_goals << std::endl;
      if(nb_joint_goals == 0)
	std::cout << session_turns.first << std::endl;
    }
    ++session_index;
  }
  std::cout << std::endl;

  outfile.close();

  return 0;
}
