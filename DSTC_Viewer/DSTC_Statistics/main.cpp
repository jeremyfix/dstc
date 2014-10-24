#include <iostream>
#include <fstream>
#include <list>
#include <libgen.h> // basename dirname
#include <cstring>
#include <functional>
#include "statistics.h"

using namespace std::placeholders;

#define DIALOG_FILENAME "log.json"
#define LABEL_FILENAME "label.json"

int main(int argc, char* argv[]) {
  std::cout << "Program to compute statistics over the DSTC databases" << std::endl;

  if(argc != 2) {
    std::cerr << "Missing flist database file" << std::endl;
    std::cerr << "Usage : " << argv[0] << " filename.flist " << std::endl;
    return -1;
  }
  
  std::string filename = argv[1];
  std::ifstream infile(filename.c_str());
  if(!infile) {
    std::cerr << "Failed to open " << filename << std::endl;
    return -1;
  }

  std::string flist_filename = basename(strdup(filename.c_str()));

  // We parse the flist file and extract the fullpath to each of the dialogs.
  // the directories contain the json files , log.json, label.json ,...
  std::string line;
  std::string diag_filename;
  std::string diag_fullpath;
  std::list< std::string > dialogs_fullpath;
  infile >> line;
  while(!infile.eof()){
    infile >> line;
    diag_fullpath = std::string(dirname(strdup(filename.c_str()))) + std::string("/../../data/") + line ;
    dialogs_fullpath.push_back(diag_fullpath);
  }

  ///////////////////////////////////////////////
  // We now define the functions for 
  // computing the statistics
  std::vector<int> length_histo;
  auto compute_length = std::bind(dialog_length, _1, std::ref(length_histo));

  std::map< std::string, int> useracts_histo;
  std::map< std::string, int> machineacts_histo;
  auto compute_dialog_acts_histo = std::bind(dialog_acts_histo, _1,
					     std::ref(useracts_histo),
					     std::ref(machineacts_histo));


  std::map< std::string, std::map<std::string, std::map<std::string, int>>> user_slot_value_histo;
  std::map< std::string, std::map<std::string, std::map<std::string, int>>> machine_slot_value_histo;
  auto compute_slot_value_histo = std::bind(slot_value_histo, _1,
					    std::ref(user_slot_value_histo),
					    std::ref(machine_slot_value_histo));


  auto statistics_over_dialogs = std::list<std::function<void(Dialog&)>>({compute_length, compute_dialog_acts_histo, compute_slot_value_histo});

  ///////////////////////////////////////////////
  // We now compute our statistics on the dialogs

  unsigned int nb_dialogs = dialogs_fullpath.size();
  unsigned int index_dialog = 1;
  for(auto& dpath: dialogs_fullpath) {
    std::cout << '\r' << index_dialog << " / " << nb_dialogs << std::flush;
    std::string dialog_filename = dpath + "/" + DIALOG_FILENAME;

    Dialog dialog = parse_dialog_json_file(dialog_filename);
    for(auto& fi: statistics_over_dialogs) 
      fi(dialog);

    ++index_dialog;
  }
  std::cout << std::endl << "Done " << std::endl;


  /////////////////////////////////////////////////
  // We now display the statistics

  // Number of turns
  std::cout << "Histogram of the # of turns" << std::endl;
  printf("%-10s%-5s\n", "Nb turns", "Nb dialogs");
  for(unsigned int i = 0 ; i < length_histo.size() ; ++i) 
    printf("%-10d%-5d\n", i, length_histo[i]);
  std::cout << std::string(10, '-') << std::endl;

  // Machine Acts
  std::cout << "Histogram of the machine acts " << std::endl;
  printf("%-30s%-5s\n", "Act", "Nb");
  for(auto& kv: machineacts_histo)
    printf("%-30s%-5d\n", kv.first.c_str(), kv.second);
  std::cout << std::string(10, '-') << std::endl;

  // User Acts
  std::cout << "Histogram of the user acts " << std::endl;
  printf("%-30s%-5s\n", "Act", "Nb");
  for(auto& kv: useracts_histo) 
    printf("%-30s%-5d\n", kv.first.c_str(), kv.second);
  std::cout << std::string(10, '-') << std::endl;


  // slot-value pair for each act
  std::cout << "Histogram of the MACHINE slot/value pair, per act" << std::endl;
  for(auto& kv: machine_slot_value_histo) {
    std::cout << "Act : " << kv.first << std::endl;
    for(auto& kv_sv: kv.second) {
      std::cout << kv_sv.first << "    ";
      for(auto& kv_sv_values: kv_sv.second) 
	std::cout << kv_sv_values.first << "(" << kv_sv_values.second << ") ";
      std::cout << std::endl;
    }
  }
  std::cout << std::string(10, '-') << std::endl;

  std::cout << "Histogram of the USER slot/value pair, per act" << std::endl;
  for(auto& kv: user_slot_value_histo) {
    std::cout << "Act : " << kv.first << std::endl;
    for(auto& kv_sv: kv.second) {
      std::cout << kv_sv.first << "    ";
      for(auto& kv_sv_values: kv_sv.second) 
	std::cout << kv_sv_values.first << "(" << kv_sv_values.second << ") ";
      std::cout << std::endl;
    }
  }
  std::cout << std::string(10, '-') << std::endl;


  // And generate a latex document for it
  std::string tex_filename = flist_filename + ".tex";
  std::ofstream outfile(tex_filename);
  outfile << "\\documentclass{article}" << std::endl
	  << "\\usepackage[utf8]{inputenc}" << std::endl
	  << "\\usepackage[margin=0.5in]{geometry}" << std::endl
	  << "\\usepackage{pgfplots}" << std::endl
	  << "\\pgfplotsset{compat=1.9}" << std::endl
	  << "\\usepackage{verbatim}" << std::endl
	  << std::endl
	  << "\\begin{document}" << std::endl
	  << "\\tableofcontents" << std::endl
	  << "\\begin{itemize}" << std::endl
	  << "\\item Number of dialogs : " << dialogs_fullpath.size() << std::endl
	  << "\\item Filename : \\begin{verbatim} " << flist_filename << "\\end{verbatim}" << std::endl
	  << "\\end{itemize}" << std::endl
	  << "\\pagebreak" << std::endl
	  << "\\section{Overview}" << std::endl
	  << "\\subsection{Number of turns in the dialogs}" << std::endl;
  generate_tex_value_histogram(length_histo, outfile);
  outfile << "\\subsection{Machine acts}" << std::endl
	  << "Number of times we see the different acts within the system acts.\\\\" << std::endl;
  generate_tex_histogram(machineacts_histo, outfile);
  outfile << "\\subsection{User acts}" << std::endl
	  << "Number of times we see the different acts within the SLU hyps of the user acts.\\\\" << std::endl;
  generate_tex_histogram(useracts_histo, outfile);

  outfile << "\\pagebreak" << std::endl
	  << "\\section{Machine acts} " << std::endl;
  for(auto& kv: machine_slot_value_histo) {
    outfile <<"\\subsection{Machine act : " << kv.first << "}" << std::endl;
    for(auto& kv_sv: kv.second) {
      outfile <<"\\subsubsection{Slot :" << kv_sv.first << "}" << std::endl;
      generate_tex_histogram(kv_sv.second, outfile);
    }
  }

  outfile << "\\pagebreak" << std::endl
	  << "\\section{User acts} " << std::endl;
  for(auto& kv: user_slot_value_histo) {
    outfile <<"\\subsection{User act : " << kv.first << "}" << std::endl;
    for(auto& kv_sv: kv.second) {
      outfile <<"\\subsubsection{Slot :" << kv_sv.first << "}" << std::endl;
      generate_tex_histogram(kv_sv.second, outfile);
    }
  }

  
  outfile << "\\end{document}" << std::endl;



  outfile.close();
  std::cout << tex_filename << " tex file generated " << std::endl;
}
