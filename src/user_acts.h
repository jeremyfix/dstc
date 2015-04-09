#ifndef BELIEF_TRACKER_USER_ACT_H
#define BELIEF_TRACKER_USER_ACT_H

#include <list>
#include <algorithm> 
#include <map>
#include <set>
#include "JSONObjects.h"

namespace belief_tracker {

  // The functions to play with the user acts


  /**
     With some semantic parsers, the SLU hypotheses do not sum to 1.0
     this function ensures this is the case
  */

  bool has_null_larger_than(const belief_tracker::SluHyps& slu_hyps,
			    double threshold);

  void renormalize_slu(belief_tracker::SluHyps& slu_hyps);


  bool is_null_uterance(const belief_tracker::Uterance& uterance);
  void adjust_noise(belief_tracker::SluHyps& slu_hyps,
		    std::list<double>& stats);

  void adjust_noise(belief_tracker::SluHyps& slu_hyps,
		    double target);

  void scale_noise(belief_tracker::SluHyps& slu_hyps,
		    double alpha);
  void saturate_noise(belief_tracker::SluHyps& slu_hyps,
		      double alpha);

  template<typename ACTS_ITERATOR>
  ACTS_ITERATOR find_act_in_list(ACTS_ITERATOR first, ACTS_ITERATOR last, std::string act) {
    return std::find_if(first, last,
			[act](const belief_tracker::DialogAct& acti) -> bool {return acti.act == act;});
  }

  //belief_tracker::Uterance::iterator find_inform_this_dontcare_in_list(belief_tracker::Uterance& acts);


  /**
     Try to replace any "this" in the acts  xxx(this=yyyy)
     The modification of the "this" is done in place in the slu_hyps
  */
  namespace this_reference {

    namespace single {
      std::string solve(const belief_tracker::Uterance& macts, bool verbose=false);
      void rewrite_slu(belief_tracker::SluHyps& slu_hyps, 
		       const std::list<DialogAct>& macts, 
		       bool verbose=false);

    }

    namespace weak_dontcare {
      std::set<std::string> solve(const belief_tracker::Uterance& macts, bool verbose=false);
      void rewrite_slu(belief_tracker::SluHyps& slu_hyps, 
		       const std::list<DialogAct>& macts, 
		       bool verbose=false);
    }
  }


  /**
     Replace the repeat machine act by copying the machine act of the previous turn
  */
  void rewrite_repeat_machine_act(belief_tracker::Uterance& macts, 
				  const belief_tracker::Uterance& prev_macts);


  bool operator<(const belief_tracker::DialogAct& act1, const belief_tracker::DialogAct& act2);


  void collapse_duplicate_hypotheses(SluHyps& slu_hyps);
  
}


#endif
