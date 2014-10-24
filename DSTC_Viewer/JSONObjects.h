/*
  This file is part of dstc-viewer.

  Copyright (C) 2014, Jeremy Fix, Supelec

  Author : Jeremy Fix

  dstc-viewer is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  any later version.

  Foobar is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with dstc-viewer.  If not, see <http://www.gnu.org/licenses/>.

  Contact : Jeremy.Fix@supelec.fr
*/

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

struct DialogAct {
  std::string act;
  std::list< std::pair<std::string, std::string> > act_slots;
};

struct DialogTurn {
  int turn_index;

  // Input
  std::list< std::pair<std::string, double> > asr_hyps;
  std::list< std::pair<std::string, double> > slu_hyps;

  std::list< std::pair< std::list<DialogAct>, double > > user_acts;

  // Output
  std::string transcript;
  std::string dialog_act;
  std::list<DialogAct> machine_acts;
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

  std::string goal_labels_str;
  std::vector< std::pair<std::string, std::string> > goal_labels;

  std::string method_label;

  std::string requested_slots_str;
  std::vector<std::string> requested_slots;
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
void extract_acts(Json::Value& value, std::list<DialogAct>& act_slots);
Ontology parse_ontology_json_file(std::string filename);
Dialog parse_dialog_json_file(std::string filename);
DialogLabels parse_label_json_file(std::string filename);
TrackerOutput parse_tracker_output_json_file(std::string filename);


void display_dialog_acts(std::list<DialogAct>& acts);

#endif // JSONOBJECTS_H
