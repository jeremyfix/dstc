#ifndef JSONOBJECTS_H
#define JSONOBJECTS_H

#include <vector>
#include <string>
#include <map>
#include <list>
#include <json/json.h>

struct Ontology {
    std::vector<std::string> requestable;
    std::map<std::string, std::vector<std::string> > informable;
    std::vector<std::string> method;
};

//////////////
// The structs for representing one dialog

struct DialogTurn {
    int turn_index;

    // Input
    std::list<std::string> asr_hyps;
    std::list<double> asr_hyps_scores;
    std::list<std::string> slu_hyps;
    std::list<double> slu_hyps_scores;

    // Output
    std::string transcript;
    std::string dialog_act;
};

struct Dialog {
    std::string session_id;
    std::string session_date;
    std::string session_time;
    std::string caller_id;
    std::string system_specific;

    std::list<DialogTurn> turns;
};

//////////////
// The struct for reprensenting the labels

struct LabelTurn {
    int turn_index;
    std::string transcription;
    std::string semantics;
    std::string goal_labels;
    std::string method_label;
    std::string requested_slots;
};

struct DialogLabels {
    // general information
    std::string session_id;
    std::string caller_id;

    // turns
    std::vector<LabelTurn> turns;

    // task-information
    std::string task_text;
    std::string constraints;
    std::string request_slots;
    bool was_successfull;
    std::string comments;
    std::string questionnaire;
};

//////////////
// The struct for reprensenting the tracker output

struct TrackerSessionTurn {
    int turn_index;
    std::vector< std::string > goal_labels;
    std::vector< std::string > goal_labels_joint;
    std::vector< double> goal_labels_joint_scores;
    std::string method_label;
    std::string requested_slots;
};

struct TrackerOutput {
    std::string dataset;
    float walltime;
    std::map< std::string, std::vector<TrackerSessionTurn> > sessions_turns;
};


std::string dialog_act_to_string(Json::Value& value);
Ontology parse_ontology_json_file(std::string filename);
Dialog parse_dialog_json_file(std::string filename);
DialogLabels parse_label_json_file(std::string filename);
TrackerOutput parse_tracker_output_json_file(std::string filename);


#endif // JSONOBJECTS_H
