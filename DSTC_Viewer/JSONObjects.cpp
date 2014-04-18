
#include <JSONObjects.h>
#include <fstream>
#include <cassert>
#include <sstream>
#include <exception>

std::string dialog_act_to_string(Json::Value& value) {
    std::string value_str = "";
    for(unsigned int i = 0 ; i < value.size() ; ++i) {
        value_str += value[i]["act"].asString() + "(";
        Json::Value value_slots = value[i]["slots"];
        for(unsigned int j = 0 ; j < value_slots.size(); ++j) {
            Json::Value value_slot = value_slots[j];
            if(value_slot.size() == 2) {
                value_str += value_slot[0].asString() + " = " + value_slot[1].asString() + (j == value_slots.size() - 1 ? "" : ",");
            }
            else {
                std::cerr << value[i]["act"].asString() << " " << value_slot.size() << std::endl;
            }
        }
        value_str += ")";
    }
    return value_str;
}

Ontology parse_ontology_json_file(std::string filename) {
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

Dialog parse_dialog_json_file(std::string filename) {
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
                dturn.asr_hyps.push_back(asrhyps[j]["asr-hyp"].asString());
                setlocale(LC_NUMERIC, "C"); // this allows to correctly parse, for example, 1.2 instead of 1,2 which is rendered as 1
                dturn.asr_hyps_scores.push_back(asrhyps[j]["score"].asDouble());
            }

            // Process the slu-hyps
            for(unsigned int j = 0 ; j < sluhyps.size() ; ++j) {
                dturn.slu_hyps.push_back(dialog_act_to_string(sluhyps[j]["slu-hyp"]));
                setlocale(LC_NUMERIC, "C"); // this allows to correctly parse, for example, 1.2 instead of 1,2 which is rendered as 1
                dturn.slu_hyps_scores.push_back(sluhyps[j]["score"].asDouble());
            }

            // Process the output of the system (transcripts and dialog_act)
            dturn.transcript = turn_output["transcript"].asString();
            dturn.dialog_act = dialog_act_to_string(turn_output["dialog-acts"]);

            dialog.turns.push_back(dturn);
        }

        infile.close();
    }
    catch(std::exception& e) {
        std::cerr << "Received an exception while parsing a dialog filelist file " << e.what() << std::endl;
    }
    return dialog;
}

DialogLabels parse_label_json_file(std::string filename) {
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

            LabelTurn label_turn;

            // Transcription : a transcription of what the user said
            label_turn.turn_index = turn["turn-index"].asInt();
            label_turn.transcription = turn["transcription"].asString();
            label_turn.semantics = turn["semantics"]["cam"].asString();

            label_turn.goal_labels = "";
            Json::Value::Members turn_goal_labels_members = turn_goal_labels.getMemberNames();
            for(auto& m: turn_goal_labels_members)
                label_turn.goal_labels += m + " = " + turn_goal_labels[m].asString() + ",";
            if(label_turn.goal_labels.size() != 0)
                label_turn.goal_labels = label_turn.goal_labels.substr(0, label_turn.goal_labels.size() - 1);

            // Method label : the true method
            label_turn.method_label = turn["method-label"].asString();

            // Requested slots : the true slots requested by the user, as a list
            label_turn.requested_slots = std::string("");
            if(turn_requested_slots.size() != 0) {
                for(unsigned int j = 0 ; j < turn_requested_slots.size() - 1 ; ++j)
                    label_turn.requested_slots += turn_requested_slots[j].asString() + ",";
                label_turn.requested_slots += turn_requested_slots[turn_requested_slots.size() - 1].asString();
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

TrackerOutput parse_tracker_output_json_file(std::string filename) {
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
        for(auto &s : sessions) {
            std::string session_id = s["session-id"].asString();
            tracker_output.sessions_turns[session_id].clear();
            Json::Value turns = s["turns"];
            int turn_index = 0;
            for(auto & t: turns) {
                TrackerSessionTurn tracker_turn;
                tracker_turn.turn_index = turn_index;

                // Parse the goal-labels
                Json::Value goal_labels = t["goal-labels"];
                Json::Value::Members goal_labels_slots = goal_labels.getMemberNames();
                tracker_turn.goal_labels.clear();
                for(auto & gls: goal_labels_slots) {
                    std::string goalstr = gls + "(";
                    Json::Value distribution = goal_labels[gls];
                    Json::Value::Members distribution_members = distribution.getMemberNames();
                    for(auto & dm: distribution_members) {
                        ostr.str("");
                        ostr << distribution[dm].asFloat();
                        goalstr += dm + ":" + ostr.str() + ",";
                    }
                    if(distribution_members.size() != 0)
                        goalstr = goalstr.substr(0, goalstr.size() - 1);
                    goalstr += ")";
                    tracker_turn.goal_labels.push_back(goalstr);
                }

                // Parse the goal-labels-joints
                Json::Value goal_labels_joint = t["goal-labels-joint"];
                tracker_turn.goal_labels_joint.clear();
                for(auto & glj: goal_labels_joint) {

                    tracker_turn.goal_labels_joint_scores.push_back(glj["score"].asDouble());
                    Json::Value glj_slots = glj["slots"];
                    Json::Value::Members glj_slots_members = glj_slots.getMemberNames();

                    std::string goalstr = "";
                    for(auto & glj_sm : glj_slots_members)
                        goalstr += glj_sm + "=" + glj_slots[glj_sm].asString() + ",";

                    if(goalstr.size() == 0)
                        goalstr = "none";
                    else
                        goalstr = goalstr.substr(0, goalstr.size() - 1);

                    tracker_turn.goal_labels_joint.push_back(goalstr);
                }

                // Parse the method label
                Json::Value method_labels = t["method-label"];
                Json::Value::Members method_labels_members = method_labels.getMemberNames();
                tracker_turn.method_label = "";
                for(auto& mm: method_labels_members) {
                    ostr.str("");
                    ostr << method_labels[mm].asFloat();
                    tracker_turn.method_label += mm + "(" + ostr.str() + "),";
                }
                if(method_labels_members.size() != 0)
                    tracker_turn.method_label = tracker_turn.method_label.substr(0, tracker_turn.method_label.size() - 1);

                // Parse the requested slots
                Json::Value requested_slots = t["requested-slots"];
                Json::Value::Members requested_slots_members = requested_slots.getMemberNames();
                tracker_turn.requested_slots = "";
                for(auto& mm: requested_slots_members) {
                    ostr.str("");
                    ostr << requested_slots[mm].asFloat();
                    tracker_turn.requested_slots += mm + "(" + ostr.str() + "),";
                }
                if(requested_slots_members.size() != 0)
                    tracker_turn.requested_slots = tracker_turn.requested_slots.substr(0, tracker_turn.requested_slots.size() - 1);

                // Add the turn to the tracker_output
                tracker_output.sessions_turns[session_id].push_back(tracker_turn);
                turn_index++;
            }
        }

    }
    catch(std::exception& e) {
        std::cerr << "Received an exception while parsing a tracker output json file " << e.what() << std::endl;
    }

    return tracker_output;
}
