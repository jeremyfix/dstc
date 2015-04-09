#include <fstream>
#include <JSONObjects.h>
#include <iostream>
#include <algorithm> 


void dialog_length(belief_tracker::Dialog& dialog, 
		   std::vector< int >& length_histo) {
  // Length_histo[i] contains the number of dialogs of length i
  unsigned int nb_turns = dialog.turns.size();  

  // Fill in possibly missing slots to put one dialog of size nb_turns
  if(length_histo.size() < nb_turns+1)
    std::fill_n (std::back_inserter(length_histo),nb_turns+1-length_histo.size(),0);

  length_histo[nb_turns] += 1;
}


void dialog_acts_histo(belief_tracker::Dialog& dialog,
		       std::map< std::string, int>& useracts_histo,
		       std::map< std::string, int>& machineacts_histo) {

  // useracts[str] contains the number of times the act  str appears
  //               in the slu hypotheses
  // machineacts_histo[str] is similar for system acts
  
  for(auto& dturni: dialog.turns) {
    // We first browse the machine acts for this turn
    for(auto& macti : dturni.machine_acts) {
      std::string act = macti.act;

      // The act is not yet present in the list, we add it
      if(machineacts_histo.find(act) ==  machineacts_histo.end())
	machineacts_histo[act] = 0;

      ++machineacts_histo[act];
    }


    // We now browse the user acts for this turn
    for(auto& sluhypi : dturni.user_acts) {
      // We just analyze the acts, not the score
      auto acts = sluhypi.first;
      for(auto& uacti : acts) {
	std::string act = uacti.act;
	
	// The act is not yet present in the list, we add it
	if(useracts_histo.find(act) ==  useracts_histo.end())
	  useracts_histo[act] = 0;

	++useracts_histo[act];
      }
    }
  }
}

void slot_value_histo(belief_tracker::Dialog& dialog,
		      std::map<std::string, std::map< std::string, std::map<std::string, int>>>& user_slot_value_histo,
		      std::map<std::string, std::map< std::string, std::map<std::string, int>>>& machine_slot_value_histo) {
  // We browse all the dialog acts
  // And then collect the number of slot/value pairs
  // independently of the associated act

  for(auto& dturni: dialog.turns) {
    // We first browse the machine acts for this turn
    for(auto& macti : dturni.machine_acts) {
      std::string act = macti.act;
      auto& act_slots = macti.act_slots;
      // We will enter a value for machine_slot_value_histo[act] only if the slots are not empty!
      // for example, this function does not care about the affirm act
      // given the ontology of DSTC2, act_slots can be of size 0, 1, 2
      // and higher only for the act canthelp
      if(act_slots.size() != 0) {
	if(machine_slot_value_histo.find(act) == machine_slot_value_histo.end())
	  machine_slot_value_histo[act] = std::map<std::string, std::map<std::string, int>>();
	for(auto& act_slots_i: act_slots) {
	  auto s = act_slots_i.first;
	  auto v = act_slots_i.second;
	  if(machine_slot_value_histo[act].find(s) == machine_slot_value_histo[act].end())
	    machine_slot_value_histo[act][s] = std::map<std::string, int>();
	  if(machine_slot_value_histo[act][s].find(v) == machine_slot_value_histo[act][s].end())
	    machine_slot_value_histo[act][s][v] = 0;
	  machine_slot_value_histo[act][s][v] += 1;
	}
      }
      else {
	//std::cout << "Skipping the machine act : " << act << std::endl;
      }
    }

    // We now browse the user acts
    for(auto& sluhyp_i : dturni.user_acts) {

      for(auto& uacti : sluhyp_i.first) {
	std::string act = uacti.act;
	auto& act_slots = uacti.act_slots;
	// We will enter a value for user_slot_value_histo[act] only if the slots are not empty!
	// for example, this function does not care about the affirm act
	// given the ontology of DSTC2, act_slots can be of size 0, 1, 2
	// and higher only for the act canthelp
	if(act_slots.size() != 0) {
	  if(user_slot_value_histo.find(act) == user_slot_value_histo.end())
	    user_slot_value_histo[act] = std::map<std::string, std::map<std::string, int>>();
	  for(auto& act_slots_i: act_slots) {
	    auto s = act_slots_i.first;
	    auto v = act_slots_i.second;
	    if(user_slot_value_histo[act].find(s) == user_slot_value_histo[act].end())
	      user_slot_value_histo[act][s] = std::map<std::string, int>();
	    if(user_slot_value_histo[act][s].find(v) == user_slot_value_histo[act][s].end())
	      user_slot_value_histo[act][s][v] = 0;
	    user_slot_value_histo[act][s][v] += 1;
	  }
	}
      }
    }


  }

}

void generate_tex_histogram(std::map<std::string, int>& histo_values, 
			    std::ofstream& outfile) {
  
  outfile << "\\begin{tikzpicture}" << std::endl
	  << "   \\begin{axis}[ ybar, width=15cm, height=5cm, enlargelimits=0.15, legend style={at={(0.5,-0.2)}, anchor=north,legend columns=-1}, ylabel={Number of occurences}, symbolic x coords={";
  
  // Fill in the symbolic labels
  for(auto& kv: histo_values) {
    outfile << kv.first << ",";
  }
  outfile << "}, xtick=data, nodes near coords, nodes near coords align={vertical}, x tick label style={rotate=45,anchor=east}, ]" << std::endl
	  << "  \\addplot coordinates {" ;
  for(auto& kv: histo_values) 
    outfile << "(" << kv.first << "," << kv.second << ") ";
  outfile << "};" << std::endl
	  << "   \\end{axis}" << std::endl
	  << "\\end{tikzpicture}" << std::endl;
  
}

void generate_tex_value_histogram(std::vector<int>& histo_values, 
				  std::ofstream& outfile) {
  
  outfile << "\\begin{tikzpicture}" << std::endl
	  << "\\begin{axis}[ ybar, width=15cm, height=5cm, ylabel={Number of dialogs}, xlabel={Number of turns}]" << std::endl
	  << "  \\addplot coordinates {" ;
  for(unsigned int i = 0 ; i < histo_values.size(); ++i)
    outfile << "(" << i << "," << histo_values[i] << ") ";
  outfile << "};" << std::endl
	  << "   \\end{axis}" << std::endl
	  << "\\end{tikzpicture}" << std::endl;
  
}
