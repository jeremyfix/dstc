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
  along with belief_tracker.  If not, see <http://www.gnu.org/licenses/>.

  Contact : Jeremy.Fix@supelec.fr
*/

#pragma once

#include <vector>
#include <map>
#include <set>
#include <functional>

#include "JSONObjects.h"
#include "goal.h"
#include "converter.h"

namespace belief_tracker {

  

  namespace info {

    // A set of possibilities
    // e.g. {french, british, !spanish}
    typedef std::set<unsigned int> Values;
    
    template <class T> using BySlot = std::vector<T>;
    template <class T> using Scored = std::pair<T, double>;
    template <class T> using MultiOf = std::list<T>;
    typedef std::list< BySlot< Values > > UtteranceInfo;
    typedef std::list<Scored<BySlot< Values > > > ScoredUtteranceInfo;
    typedef std::list< ScoredUtteranceInfo > TurnInfo;



    // A collection of possibilities for every slot
    // e.g. [{french, british, !spanish}, {centre}, {}, {}]


    //typedef std::map< unsigned int, Infos > SlotsInfos;

    // A collection of scored MultiInfo
    // e.g. < ([{french, british, !spanish}, {centre}, {}, {}], 0.8), ...>
    //typedef std::map< SlotsInfo, double > ScoredSlotsInfos;

    // A collection of possibilities for a slot 
    // e.g.  for area : [ {!centre, !north}, {east} ]
    //typedef std::set< Infos > MultipleInfos;

    // A collection of possibiliites for every slot
    //typedef std::map< unsigned int , MultipleInfos > SlotsMultipleInfos;


    //typedef std::vector< SlotsInfos > ExpandedInfo;

    Goal transition_function(const Goal& g, const BySlot<Values>& info);


    /**
     * Given a collection of values in infos, for the slot slot_index
     * we extract the positive and the negative values
     *  throw an exception in case of weak_dontcare, unknown
     **/
    std::pair<Values, Values> split(unsigned int slot_index, const Values& infos);


    namespace multi_info {

      /**
       * Given a set of possibilities for a slot
       * it keeps the positives as well as the negatives that conflict with the positives
       **/
      Values filter(unsigned int slot_index, const Values& infos);

      /**
       * The process of cleaning a set of values consist in building res as 
       * - if values.size() = 0 :  {?} in res
       * - if neg(values).size() != 0 : neg(values) in res
       * - for every p in pos(values) : {p} in res
       **/ 
      MultiOf<Values> clean(unsigned int slot_index, const Values &values);


      /**
       * Given a set of scored utterances from the user
       * and an utterance of the machine
       * returns a list (one item per SLU hypothese) of set of hypotheses per slot
       **/
      belief_tracker::info::TurnInfo extract_info(const belief_tracker::SluHyps &user_acts, 
							     const belief_tracker::Uterance& macts, 
							     bool verbose = false);

      /**
       * Given one utterance, returns a list of multi-values per slot
       * e.g. < (['!french', '!english'],['cheap'],[?],[?]) ,
       *        (['british'], [?], ['north'], [?]) >
       **/
      UtteranceInfo extract_info(const belief_tracker::Uterance& uacts,
				 const belief_tracker::info::BySlot<belief_tracker::info::Values>& mact_expl_conf,
				 const belief_tracker::info::BySlot<belief_tracker::info::Values>& mact_impl_conf, 
				 const belief_tracker::info::BySlot<belief_tracker::info::Values>& mact_canthelp_conf,
				 bool verbose=false);

    }

    
      namespace single_info {

	namespace rules {
	  typedef std::function<void(const belief_tracker::Uterance&, const belief_tracker::Uterance&, BySlot<Values>&, BySlot<Values>&)> rule_type;

	  void inform(const belief_tracker::Uterance& macts,
		      const belief_tracker::Uterance& uacts,
		      BySlot<Values>& pos_mu,
		      BySlot<Values>& neg_mu);	
	  void explconf(const belief_tracker::Uterance& macts,
			const belief_tracker::Uterance& uacts,
			BySlot<Values>& pos_mu,
			BySlot<Values>& neg_mu);
	  void implconf(const belief_tracker::Uterance& macts,
			const belief_tracker::Uterance& uacts,
			BySlot<Values>& pos_mu,
			BySlot<Values>& neg_mu);			
	  
	  void negate(const belief_tracker::Uterance& macts,
		      const belief_tracker::Uterance& uacts,
		      BySlot<Values>& pos_mu,
		      BySlot<Values>& neg_mu);
	  void deny(const belief_tracker::Uterance& macts,
		    const belief_tracker::Uterance& uacts,
		    BySlot<Values>& pos_mu,
		    BySlot<Values>& neg_mu);

	}


	//MultiInfo clean(const MultiInfo &info);
	
	belief_tracker::info::TurnInfo extract_info(const belief_tracker::SluHyps &user_acts, 
						    const belief_tracker::Uterance& macts,
						    std::list<rules::rule_type> rules,
						    bool verbose = false);


	belief_tracker::info::TurnInfo extract_info(const belief_tracker::SluHyps &user_acts, 
						    const belief_tracker::Uterance& macts, 
						    bool verbose = false);
	
	belief_tracker::info::UtteranceInfo extract_info(const belief_tracker::Uterance& uacts, 
							 const belief_tracker::Uterance& macts, 
							 bool verbose=false);
      }
    
  }
}



std::ostream& operator<<(std::ostream &os, const belief_tracker::info::BySlot< belief_tracker::info::Values >& info);
void display_byslot_values(std::ostream &os, const belief_tracker::info::BySlot< belief_tracker::info::Values >& info);
std::ostream& operator<<(std::ostream &os, const belief_tracker::info::TurnInfo& turn_info);
void display_turn_info(std::ostream &os, const belief_tracker::info::TurnInfo& info);

