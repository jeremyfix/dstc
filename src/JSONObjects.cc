
#include <fstream>
#include <cassert>
#include <sstream>
#include <exception>
#include <algorithm>
#include <libgen.h> // basename dirname
#include <iterator>
#include <cstring>
#include <boost/filesystem.hpp> 
#include <numeric>
#include "JSONObjects.h"
#include "constants.h"

bool belief_tracker::operator==(const belief_tracker::DialogAct& x, const belief_tracker::DialogAct& y) {
  return (x.act == y.act) && (x.act_slots == y.act_slots);
}

bool belief_tracker::operator==(const Uterance& x, 
				const Uterance& y) {
  // We check if (x c y) and (y c x)
  for(auto& xi: x) 
    if(std::find(y.begin(), y.end(), xi) == y.end())
       return false;
  for(auto& yi: y) 
    if(std::find(x.begin(), x.end(), yi) == x.end())
      return false;
  return true;
}

std::string belief_tracker::dialog_act_to_string(Json::Value& value) {
  std::ostringstream ostr;
  ostr.str("");
  for(unsigned int i = 0 ; i < value.size() ; ++i) {
    ostr << value[i]["act"].asString() << "(";
    Json::Value value_slots = value[i]["slots"];
    for(unsigned int j = 0 ; j < value_slots.size(); ++j) {
      Json::Value value_slot = value_slots[j];
      if(value_slot.size() == 2) {
	if(value_slot[0].asString() == std::string("count")) 
	  ostr << value_slot[0].asString() << " = " << value_slot[1].asInt() << (j == value_slots.size() - 1 ? "" : ",");
	else 
	  ostr << value_slot[0].asString() << " = " << value_slot[1].asString() << (j == value_slots.size() - 1 ? "" : ",");
      }
      else 
	std::cerr << value[i]["act"].asString() << " " << value_slot.size() << std::endl;
    }
    ostr << ")";
  }
  return ostr.str();
}

void belief_tracker::extract_acts(Json::Value& value, std::list<DialogAct>& act_and_slots) {
  if(value.size() == 0) {
    DialogAct dact;
    dact.act=NULL_ACT;
    act_and_slots.push_back(dact);
  }
  else {
    for(unsigned int i = 0 ; i < value.size() ; ++i) {
      DialogAct dact;
      dact.act = value[i]["act"].asString();
      Json::Value value_slots = value[i]["slots"];
      for(unsigned int j = 0 ; j < value_slots.size(); ++j) {
	Json::Value value_slot = value_slots[j];
	if(value_slot.size() == 2) {
	  std::string first = value_slot[0].asString();
	  std::string second;
	  if(first == std::string("count")) 
	    second = std::to_string(value_slot[1].asInt());
	  else 
	    second = value_slot[1].asString();
	  dact.act_slots.push_back(std::make_pair(first, second));
	}
	else 
	  std::cerr << "in extract_act, I thought \"slots\" contained list of PAIRS!" << std::endl;
      }
      act_and_slots.push_back(dact);
    }
  }
}

belief_tracker::Ontology belief_tracker::parse_ontology_json_file(std::string filename) {
  Ontology ontology;

  try {
    std::ifstream infile(filename.c_str());
    Json::Value root;
    Json::Reader reader;
    if ( !reader.parse(infile, root) )
      {
	// report to the user the failure and their locations in the document.
	std::cerr  << "Failed to parse file " << filename << " "
		   << reader.getFormattedErrorMessages();
	return ontology;
      }

    Json::Value value;
    // Requestable
    ontology.requestable.clear();
    value = root["requestable"];
    for(unsigned int i = 0 ; i < value.size(); ++i)
      ontology.requestable.push_back(value[i].asString());

    // Method
    ontology.method.clear();
    value = root["method"];
    for(unsigned int i = 0 ; i < value.size(); ++i)
      ontology.method.push_back(value[i].asString());

    // Informable
    ontology.informable.clear();
    value = root["informable"];

    Json::Value::Members members = value.getMemberNames();
    for(unsigned int i = 0 ; i < members.size() ; ++i) {
      std::string key = members[i];
      ontology.informable[key].clear();

      Json::Value child = value[key];
      for(unsigned int j = 0 ; j < child.size() ; ++j)
	ontology.informable[key].push_back(child[j].asString());
    }

    infile.close();
  }
  catch(std::exception& e) {
    std::cerr << "Received an exception while parsing an ontology file " << e.what() << std::endl;
  }

  return ontology;
}

belief_tracker::Dialog belief_tracker::parse_dialog_json_file(std::string filename) {
  Dialog dialog;
  try {
    std::ifstream infile(filename.c_str());
    Json::Value root;
    Json::Reader reader;
    if ( !reader.parse(infile, root) )
      {
	// report to the user the failure and their locations in the document.
	std::cerr  << "Failed to parse file " << filename << " "
		   << reader.getFormattedErrorMessages();
	throw std::exception();
      }

    dialog.session_id = root["session-id"].asString();
    dialog.session_date = root["session-date"].asString();
    dialog.session_time = root["session-time"].asString();
    dialog.caller_id = root["caller-id"].asString();
    dialog.dialog_manager = root["system-specific"]["dialog-manager"].asInt();
    dialog.acoustic_condition = root["system-specific"]["acoustic-condition"].asInt();


    Json::Value value = root["turns"];
    for(unsigned int i = 0 ; i < value.size() ; ++i) {
      Json::Value turn = value[i];
      Json::Value turn_input = turn["input"];
      Json::Value turn_output = turn["output"];
      Json::Value turn_input_live = turn_input["live"];
      Json::Value asrhyps = turn_input_live["asr-hyps"];
      Json::Value sluhyps = turn_input_live["slu-hyps"];

      DialogTurn dturn;
      // Process the asr-hyps
      for(unsigned int j = 0 ; j < asrhyps.size() ; ++j) {
	setlocale(LC_NUMERIC, "C"); // this allows to correctly parse, for example, 1.2 instead of 1,2 which is rendered as 1
	dturn.asr_hyps.push_back(std::make_pair(asrhyps[j]["asr-hyp"].asString(),
						asrhyps[j]["score"].asDouble()));
      }

      // Process the slu-hyps
      for(unsigned int j = 0 ; j < sluhyps.size() ; ++j) {
	setlocale(LC_NUMERIC, "C"); // this allows to correctly parse, for example, 1.2 instead of 1,2 which is rendered as 1
	Json::Value sluhyp_j = sluhyps[j];
	std::list<DialogAct> user_acts_j;
	extract_acts(sluhyp_j["slu-hyp"], user_acts_j);
	dturn.user_acts.push_back(std::make_pair(user_acts_j, sluhyp_j["score"].asDouble()));
      }

      // Process the output of the system (transcripts and dialog_act)
      dturn.transcript = turn_output["transcript"].asString();
      dturn.dialog_act = dialog_act_to_string(turn_output["dialog-acts"]);
      Json::Value machine_acts = turn_output["dialog-acts"];
      extract_acts(machine_acts, dturn.machine_acts);
	    

      dialog.turns.push_back(dturn);
    }

    infile.close();
  }
  catch(std::exception& e) {
    std::cerr << "Received an exception while parsing a dialog filelist file " << e.what() << std::endl;
  }
  return dialog;
}

belief_tracker::DialogLabels belief_tracker::parse_label_json_file(std::string filename) {
  DialogLabels dialog_labels;
  try {
    std::ifstream infile(filename.c_str());
    Json::Value root;
    Json::Reader reader;
    if ( !reader.parse(infile, root) )
      {
	// report to the user the failure and their locations in the document.
	std::cerr  << "Failed to parse file " << filename << " "
		   << reader.getFormattedErrorMessages();
	throw std::exception();
      }

    dialog_labels.session_id = root["session-id"].asString();
    dialog_labels.caller_id = root["caller-id"].asString();

    // Process the turns
    Json::Value value = root["turns"];
    for(unsigned int i = 0 ; i < value.size() ; ++i) {
      Json::Value turn = value[i];
      Json::Value turn_goal_labels = turn["goal-labels"];
      Json::Value turn_requested_slots = turn["requested-slots"];
      Json::Value turn_semantics_json = turn["semantics"]["json"];
      LabelTurn label_turn;

      label_turn.turn_index = turn["turn-index"].asInt();
      // Transcription : a transcription of what the user said
      label_turn.transcription = turn["transcription"].asString();
      //label_turn.semantics_str = turn["semantics"]["cam"].asString();

      // label_turn.semantics : the transcription in Dialog acts of what the user said
      for(unsigned int j = 0 ; j < turn_semantics_json.size() ; ++j) {
	std::string act = turn_semantics_json[j]["act"].asString();
	DialogAct dact;
	dact.act = act;
	Json::Value turn_semantics_json_j_slots = turn_semantics_json[j]["slots"];
	for(unsigned int k = 0 ; k < turn_semantics_json_j_slots.size(); ++k) 
	  dact.act_slots.push_back(std::make_pair(turn_semantics_json_j_slots[k][0].asString(),
					turn_semantics_json_j_slots[k][1].asString()));
	label_turn.semantics.push_back(dact);
      }
      if(label_turn.semantics.size() == 0) {
	DialogAct dact;
	dact.act = NULL_ACT;
	label_turn.semantics.push_back(dact);
      }

      
      label_turn.goal_labels_str = "";
      Json::Value::Members turn_goal_labels_members = turn_goal_labels.getMemberNames();
      for(auto& m: turn_goal_labels_members) {
	label_turn.goal_labels_str += m + " = " + turn_goal_labels[m].asString() + ",";
	//label_turn.goal_labels.push_back(std::make_pair(m, turn_goal_labels[m].asString()));
	label_turn.goal_labels[m] = turn_goal_labels[m].asString();
      }
      if(label_turn.goal_labels_str.size() != 0)
	label_turn.goal_labels_str = label_turn.goal_labels_str.substr(0, label_turn.goal_labels_str.size() - 1);

      // Method label : the true method
      label_turn.method_label = turn["method-label"].asString();

      // Requested slots : the true slots requested by the user, as a list
      label_turn.requested_slots_str = std::string("");
      if(turn_requested_slots.size() != 0) {
	for(unsigned int j = 0 ; j < turn_requested_slots.size() - 1 ; ++j) {
	  std::string tmp =  turn_requested_slots[j].asString();
	  label_turn.requested_slots_str += tmp + ",";
	  label_turn.requested_slots.push_back(tmp);
	}
	std::string tmp = turn_requested_slots[turn_requested_slots.size() - 1].asString();
	label_turn.requested_slots_str += tmp;
	label_turn.requested_slots.push_back(tmp);
      }
	    
      dialog_labels.turns.push_back(label_turn);

    }

    // Process the task-information parts
    Json::Value task_information = root["task-information"];
    Json::Value goal = task_information["goal"];
    Json::Value goal_constraints = goal["constraints"];
    Json::Value feedback = task_information["feedback"];

    dialog_labels.task_text = goal["text"].asString();

    //  A list of slot/value pairs for the restaurant the user must settle on
    dialog_labels.constraints = "";
    for(auto& c: goal_constraints)
      dialog_labels.constraints += c[0].asString() + " = " + c[1].asString() + ",";
    if(dialog_labels.constraints.size() != 0)
      dialog_labels.constraints = dialog_labels.constraints.substr(0, dialog_labels.constraints.size() - 1);

    // The slots the user must request
    dialog_labels.request_slots = "";
    for(auto& c: goal["request-slots"])
      dialog_labels.request_slots += c.asString() + ",";
    if(dialog_labels.request_slots.size() != 0)
      dialog_labels.request_slots = dialog_labels.request_slots.substr(0, dialog_labels.request_slots.size() - 1);

    // Was the dialog successfull ?
    dialog_labels.was_successfull = feedback["success"].asBool();

    // Any comments on the call ?
    dialog_labels.comments = feedback["comments"].asString();

    // A list of question/answer pairs
    dialog_labels.questionnaire = "";
    for(auto & q : feedback["questionnaire"]) {
      dialog_labels.questionnaire += q[0].asString() + std::string(" : ") + q[1].asString() + "\n";
    }
    infile.close();
  }
  catch(std::exception& e) {
    std::cerr << "Received an exception while parsing a label json file " << e.what() << std::endl;
  }

  return dialog_labels;

}

belief_tracker::TrackerOutput belief_tracker::parse_tracker_output_json_file(std::string filename) {
  TrackerOutput tracker_output;

  try {
    std::ifstream infile(filename.c_str());
    Json::Value root;
    Json::Reader reader;

    if ( !reader.parse(infile, root) )
      {
	// report to the user the failure and their locations in the document.
	std::cerr  << "Failed to parse file " << filename << " "
		   << reader.getFormattedErrorMessages();
	throw std::exception();
      }

    tracker_output.dataset = root["dataset"].asString();
    tracker_output.walltime = root["wall-time"].asFloat();

    std::ostringstream ostr;

    Json::Value sessions = root["sessions"];
    tracker_output.sessions_turns.clear();
    unsigned int session_index = 0;
    for(auto &s : sessions) {
      std::string session_id = s["session-id"].asString();
      tracker_output.sessions_turns.push_back(std::make_pair(session_id, std::vector<TrackerSessionTurn>()));
      Json::Value turns = s["turns"];
      int turn_index = 0;
      for(auto & t: turns) {
	TrackerSessionTurn tracker_turn;
	tracker_turn.turn_index = turn_index;

	// Parse the goal-labels
	Json::Value goal_labels = t["goal-labels"];
	Json::Value::Members goal_labels_slots = goal_labels.getMemberNames();
	//tracker_turn.goal_labels_str.clear();
	for(auto & gls: goal_labels_slots) {

	  tracker_turn.goal_labels_scores[gls] = std::map<std::string, double>();
	  Json::Value distribution = goal_labels[gls];
	  Json::Value::Members distribution_members = distribution.getMemberNames();
	  for(auto & dm: distribution_members) {
	    double score = distribution[dm].asFloat();
	    tracker_turn.goal_labels_scores[gls][dm] = score;
	  }
	}

	// Parse the goal-labels-joints
	Json::Value goal_labels_joint = t["goal-labels-joint"];
	for(auto & glj: goal_labels_joint) {

	  double score = glj["score"].asDouble();
	  Json::Value glj_slots = glj["slots"];
	  Json::Value::Members glj_slots_members = glj_slots.getMemberNames();

	  std::map<std::string, std::string> map_joint_goals;
	  for(auto & glj_sm : glj_slots_members) {
	    map_joint_goals[glj_sm] = glj_slots[glj_sm].asString();
	  }
	  tracker_turn.goal_labels_joint_scores.push_back(std::make_pair(map_joint_goals, score));

	}

	// Parse the method label
	Json::Value method_labels = t["method-label"];
	Json::Value::Members method_labels_members = method_labels.getMemberNames();
	for(auto& mm: method_labels_members) {
	  double method_label_score = method_labels[mm].asFloat();
	  tracker_turn.method_labels_scores[mm] = method_label_score;
	}

	// Parse the requested slots
	Json::Value requested_slots = t["requested-slots"];
	Json::Value::Members requested_slots_members = requested_slots.getMemberNames();
	for(auto& mm: requested_slots_members) {
	  double requested_slots_score = requested_slots[mm].asFloat();
	  tracker_turn.requested_slots_scores[mm] = requested_slots_score;
	}

	// Add the turn to the tracker_output
	tracker_output.sessions_turns[session_index].second.push_back(tracker_turn);
	turn_index++;
      }
      ++session_index;
    }

  }
  catch(std::exception& e) {
    std::cerr << "Received an exception while parsing a tracker output json file " << e.what() << std::endl;
  }

  return tracker_output;
}


void belief_tracker::dump_tracker_output_json_file(const belief_tracker::TrackerOutput& tracker_output, 
						   std::string filename) {

  Json::Value root(Json::objectValue);

  root["wall-time"] = Json::Value(tracker_output.walltime);
  root["dataset"] = Json::Value(tracker_output.dataset);

  Json::Value sessions(Json::arrayValue);
  for(auto& kv: tracker_output.sessions_turns) {
    Json::Value cur_session;
    cur_session["session-id"] = Json::Value(kv.first);

    Json::Value turns(Json::arrayValue);
    // Iterate over the turns
    unsigned int turn_index = 0;
    for(auto& turn_i: kv.second) {
      Json::Value cur_turn;

      // Dump the probabilities over the values for each slot
      Json::Value goal_labels(Json::objectValue);
      for(auto& gls_i: turn_i.goal_labels_scores) {
	const std::string& slot_name = gls_i.first;
	Json::Value distrib_values;
	double sum = std::accumulate(gls_i.second.begin(),
				     gls_i.second.end(),
				     0.0,
				     [](double acc, const std::pair<std::string, double>& elem) -> double { return acc + elem.second;});

	bool is_full_unknown = true;
	for(auto& v_score: gls_i.second)
	  if(v_score.first != belief_tracker::SYMBOL_UNKNOWN) {
	    is_full_unknown = false;
	    distrib_values[v_score.first] = v_score.second/sum;
	  }
	if(!is_full_unknown) goal_labels[slot_name] = distrib_values;
	
      }
      cur_turn["goal-labels"] = goal_labels;

      // Dump the joint goals
      Json::Value goal_labels_joints(Json::arrayValue);


      auto & jg = turn_i.goal_labels_joint_scores;
      double sum = std::accumulate(jg.begin(),
				   jg.end(),
				   0.0,
				   [](double acc, const std::pair< std::map<std::string, std::string>, double>& elem) { return acc + elem.second; });
				   

      for(auto& gj_i : turn_i.goal_labels_joint_scores) {
	const std::map<std::string, std::string>& form = gj_i.first;
	double score =  gj_i.second / sum;

	Json::Value cur_joint_label;
	cur_joint_label["score"] = Json::Value(score);


	bool is_full_unknown = true;
	Json::Value slots_assignements;
	for(auto& sv: form) 
	  if(sv.second != belief_tracker::SYMBOL_UNKNOWN) {
	    slots_assignements[sv.first] = Json::Value(sv.second);
	    is_full_unknown = false;
	  }
	cur_joint_label["slots"] = slots_assignements;

	if(!is_full_unknown)
	  goal_labels_joints.append(cur_joint_label);

      }     
      cur_turn["goal-labels-joint"] = goal_labels_joints;

      for(auto& m_score: turn_i.method_labels_scores)
	cur_turn["method-label"][m_score.first] = Json::Value(m_score.second);
     
      // Dump the requested slots
      Json::Value requested_slots(Json::objectValue);
      for(auto& req_score: turn_i.requested_slots_scores)
	requested_slots[req_score.first] = req_score.second;
      cur_turn["requested-slots"] = requested_slots;

      turns.append(cur_turn);

      ++turn_index;
    }

    cur_session["turns"] = turns;
    sessions.append(cur_session);
  }

  root["sessions"] = sessions;
  
  Json::StyledWriter writer;
  std::string outputConfig = writer.write( root );
  std::ofstream outfile(filename);
  outfile << outputConfig;
  outfile.close();

}

void belief_tracker::serial::begin_dump(std::ofstream& outfile, std::string dataset) {
  outfile << "{" << std::endl
	  << std::string(BELIEF_TRACKER_TABULAR_JSON, ' ') << "\"dataset\": \"" << dataset << "\"," << std::endl
	  << std::string(BELIEF_TRACKER_TABULAR_JSON, ' ') << "\"sessions\": [" << std::endl;
}

void belief_tracker::serial::dump_session(std::ofstream& outfile, std::string session_id, const std::vector<TrackerSessionTurn>& session, bool last) {

  Json::Value cur_session(Json::objectValue);
  cur_session["session-id"] = session_id;

  Json::Value turns(Json::arrayValue);
  // Iterate over the turns
  unsigned int turn_index = 0;
  for(auto& turn_i: session) {
    Json::Value cur_turn;

    // Dump the probabilities over the values for each slot
    Json::Value goal_labels(Json::objectValue);
    for(auto& gls_i: turn_i.goal_labels_scores) {
      const std::string& slot_name = gls_i.first;
      Json::Value distrib_values;
      double sum = std::accumulate(gls_i.second.begin(),
				   gls_i.second.end(),
				   0.0,
				   [](double acc, const std::pair<std::string, double>& elem) -> double { return acc + elem.second;});

      bool is_full_unknown = true;
      for(auto& v_score: gls_i.second)
	if(v_score.first != belief_tracker::SYMBOL_UNKNOWN) {
	  is_full_unknown = false;
	  distrib_values[v_score.first] = v_score.second/sum;
	}
      if(!is_full_unknown) goal_labels[slot_name] = distrib_values;
	
    }
    cur_turn["goal-labels"] = goal_labels;

    // Dump the joint goals
    Json::Value goal_labels_joints(Json::arrayValue);


    auto & jg = turn_i.goal_labels_joint_scores;
    double sum = std::accumulate(jg.begin(),
				 jg.end(),
				 0.0,
				 [](double acc, const std::pair< std::map<std::string, std::string>, double>& elem) { return acc + elem.second; });
				   

    for(auto& gj_i : turn_i.goal_labels_joint_scores) {
      const std::map<std::string, std::string>& form = gj_i.first;
      double score =  gj_i.second / sum;

      Json::Value cur_joint_label;
      cur_joint_label["score"] = Json::Value(score);


      bool is_full_unknown = true;
      Json::Value slots_assignements;
      for(auto& sv: form) 
	if(sv.second != belief_tracker::SYMBOL_UNKNOWN) {
	  slots_assignements[sv.first] = Json::Value(sv.second);
	  is_full_unknown = false;
	}
      cur_joint_label["slots"] = slots_assignements;

      if(!is_full_unknown)
	goal_labels_joints.append(cur_joint_label);

    }     
    cur_turn["goal-labels-joint"] = goal_labels_joints;

    for(auto& m_score: turn_i.method_labels_scores)
      cur_turn["method-label"][m_score.first] = Json::Value(m_score.second);
     
    // Dump the requested slots
    Json::Value requested_slots(Json::objectValue);
    for(auto& req_score: turn_i.requested_slots_scores)
      requested_slots[req_score.first] = req_score.second;
    cur_turn["requested-slots"] = requested_slots;

    turns.append(cur_turn);

    ++turn_index;
  }

  cur_session["turns"] = turns;

  Json::StyledWriter writer;
  std::string outputConfig = writer.write( cur_session );
  outfile << outputConfig;
  if(!last)
    outfile << "," << std::endl;
}

void belief_tracker::serial::end_dump(std::ofstream& outfile, double walltime) {
  outfile << std::string(BELIEF_TRACKER_TABULAR_JSON, ' ') << "]," << std::endl
	  << std::string(BELIEF_TRACKER_TABULAR_JSON, ' ') << "\"wall-time\": " << walltime  << std::endl
	  << "}" << std::endl;
}


std::vector< std::array<std::string, 4> > belief_tracker::parse_flist(std::string flist_filename_fullpath, std::string relative_path_to_files, std::string logfilename, std::string labelfilename) {
  std::string line;
  std::string diag_fullpath;
  std::string label_fullpath;
  std::vector< std::array<std::string, 4> > dialog_label_fullpath;

  boost::filesystem::path pathname(flist_filename_fullpath);
  std::string dirname_flist = pathname.parent_path().string();


  std::ifstream infile(flist_filename_fullpath.c_str());
  if(!infile) {
    std::cerr << "Failed to open " << flist_filename_fullpath << std::endl;
    throw std::exception();
  }
  infile >> line;
  while(!infile.eof()) {
    std::string line_cpy = line;
    diag_fullpath = dirname_flist + std::string("/") + relative_path_to_files + std::string("/") + line + std::string("/") + logfilename;
    label_fullpath = dirname_flist + std::string("/") + relative_path_to_files + std::string("/") + line + std::string("/") + labelfilename;

    std::replace(line_cpy.begin(), line_cpy.end(), '/', ' ');
    std::istringstream iss(line_cpy);
    std::vector<std::string> tokens({std::istream_iterator<std::string>{iss},  std::istream_iterator<std::string>{}});

    dialog_label_fullpath.push_back({{diag_fullpath, label_fullpath, tokens[tokens.size() - 1], line}});
    infile >> line;
  }
  infile.close();

  return dialog_label_fullpath;

}
/*
std::string belief_tracker::slu_hyps_to_str(const std::list< std::pair<std::string, double> >& hyps) {
  std::ostringstream ostr;
  ostr.str("");
  for(auto& hyp_i : hyps) 
    ostr << hyp_i.first << " (" << hyp_i.second << ")" << std::endl;
  return ostr.str();
  }
*/

std::string belief_tracker::slu_hyps_to_str(const belief_tracker::SluHyps& hyps) {
  std::ostringstream ostr;
  ostr.str("");
  for(auto& hyp_i : hyps) 
    ostr << dialog_acts_to_str(hyp_i.first) << " (" << hyp_i.second << ")" << std::endl;
  return ostr.str();
}

std::string belief_tracker::dialog_act_to_str(const belief_tracker::DialogAct& act) {
  std::string str = act.act + "(";
  unsigned int count = 0;
  for(auto& act_slot_i: act.act_slots) {
    str += act_slot_i.first + "=" + act_slot_i.second ;
    if((count++) != act.act_slots.size()-1)
      str += ",";
  }
  str += ")";
  return str;
}

std::string belief_tracker::dialog_acts_to_str(const belief_tracker::Uterance& acts) {
  std::string str("");
  for(auto& acti : acts) 
    str += dialog_act_to_str(acti);
  return str;
}

void belief_tracker::display_dialog_acts(const belief_tracker::Uterance& acts) {
  std::cout << dialog_acts_to_str(acts) << std::endl;
}

std::string belief_tracker::goal_labels_to_str(const std::map<std::string, std::string>& goal_labels) {
  std::string str("");
  unsigned int count = 0;
  for(auto& kv: goal_labels) {
    str += kv.first + "=" + kv.second;
    if((count++) != goal_labels.size()-1)
      str += ",";
  }

  return str;
}

std::ostream & operator<<(std::ostream &os, const belief_tracker::DialogAct& act) {
  os << act.act << "(";
  unsigned int count = 0;
  for(auto& act_slot_i: act.act_slots) {
    os << act_slot_i.first << "=" << act_slot_i.second ;
    if((count++) != act.act_slots.size()-1)
      os << ",";
  }

  os << ")";
  return os;
}
std::ostream & operator<<(std::ostream &os, const belief_tracker::Uterance& acts) {
 for(auto& acti : acts) 
   os << dialog_act_to_str(acti);
 return os;
}


std::ostream & operator<<(std::ostream &os, const belief_tracker::SluHyp& slu_hyp) {
  os << slu_hyp.first << "(" << slu_hyp.second << ")";
  return os;
}

std::ostream & operator<<(std::ostream &os, const belief_tracker::SluHyps& slu_hyps) {
  for(auto& hyp: slu_hyps)
    os << hyp << std::endl;
  
  return os;

}


