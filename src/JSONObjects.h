/*
  This file is part of belief-tracker.

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

#pragma once

#include <vector>
#include <string>
#include <map>
#include <array>
#include <list>
#include <json/json.h>
#include <iomanip>
#include <iostream>
#include "constants.h"

#define BELIEF_TRACKER_TABULAR_JSON 4

namespace belief_tracker {

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

  typedef std::list<DialogAct>                      Uterance;
  typedef std::pair< Uterance, double >             SluHyp;
  typedef std::list<SluHyp>                         SluHyps;

  bool operator==(const DialogAct& x, const DialogAct& y);
  bool operator==(const Uterance& x, const Uterance& y);


  struct DialogTurn {
    int turn_index;

    // Input
    std::list< std::pair<std::string, double> > asr_hyps;
    SluHyps user_acts;

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
    int dialog_manager;
    int acoustic_condition;

    std::list<DialogTurn> turns;
  };

  //////////////
  // The struct for reprensenting the labels

  struct LabelTurn {
    int turn_index;
    std::string transcription;
    //std::string semantics_str;
    Uterance semantics;

    std::string goal_labels_str;
    std::map< std::string, std::string > goal_labels;

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
    std::map< std::string, double> method_labels_scores;
    std::map< std::string, double> requested_slots_scores;

    std::map< std::string, std::map<std::string, double> > goal_labels_scores; 
    std::vector< std::pair< std::map<std::string, std::string>, double> > goal_labels_joint_scores;
  };

  struct TrackerOutput {
    std::string dataset;
    float walltime;
    // Do not use a map, otherwise the dialogs got reordered!!
    // and the order is important for the scoring function
    std::vector< std::pair<std::string, std::vector<TrackerSessionTurn> > > sessions_turns;
  };


  std::string dialog_act_to_string(Json::Value& value);
  void extract_acts(Json::Value& value, std::list<DialogAct>& act_slots);
  Ontology parse_ontology_json_file(std::string filename);
  Dialog parse_dialog_json_file(std::string filename);
  DialogLabels parse_label_json_file(std::string filename);
  TrackerOutput parse_tracker_output_json_file(std::string filename);
  void dump_tracker_output_json_file(const TrackerOutput& tracker_output, std::string filename);

  // For the DSTC-3, the files can be quite large
  // especially when using the SJTU SLU
  // in this case, we serialize into the files the sessions as they
  // are computed
  namespace serial {
    void begin_dump(std::ofstream& outfile, std::string dataset);
    void dump_session(std::ofstream& outfile,  std::string session_id, const std::vector<TrackerSessionTurn>& session, bool last);
    void end_dump(std::ofstream& outfile, double walltime);
  }

  std::vector< std::array<std::string, 4> > parse_flist(std::string flist_filename_fullpath, std::string relative_path_to_files="../../data", std::string logfilename="log.json", std::string labelfilename="label.json");

  //std::string slu_hyps_to_str(const std::list< std::pair<std::string, double> >& );
  std::string slu_hyps_to_str(const SluHyps& );
  std::string dialog_act_to_str(const DialogAct& act);
  std::string dialog_acts_to_str(const Uterance& acts);
  void display_dialog_acts(const Uterance& acts);
  std::string goal_labels_to_str(const std::map< std::string, std::string >& goal_labels);
}

std::ostream & operator<<(std::ostream &os, const belief_tracker::DialogAct& act);
std::ostream & operator<<(std::ostream &os, const belief_tracker::Uterance& acts);
std::ostream & operator<<(std::ostream &os, const belief_tracker::SluHyp& slu_hyp);
std::ostream & operator<<(std::ostream &os, const belief_tracker::SluHyps& slu_hyps);
