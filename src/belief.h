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

#include <map>
#include <functional>
#include "goal.h"
#include "JSONObjects.h"
#include "info.h"

namespace belief_tracker {


  namespace requested_slots {
    void update(const belief_tracker::SluHyps &user_acts, 
		const belief_tracker::Uterance& macts,
		std::map< std::string, double >& requested_slots);
    std::string toStr(const std::map< std::string, double >& requested_slots);
  }

  namespace methods {
    void update(const belief_tracker::SluHyps &user_acts, 
		const belief_tracker::Uterance& macts,
		std::map< std::string, double >& methods);
    std::string toStr(const std::map< std::string, double >& methods);
  }

  class Belief {

  public:
    Belief();
    virtual ~Belief(void);
    virtual Goal extract_best_goal() const = 0;
    virtual void start() = 0;
    virtual void fill_tracker_session(belief_tracker::TrackerSessionTurn& tracker_session_turn) = 0;
  };

  namespace joint {

      class Belief : public belief_tracker::Belief{
      public:
	// A belief is a probability distribution over joint goals
	typedef std::map<belief_tracker::Goal, double> belief_type;
	belief_type belief;

      public:
	Belief();
	Belief(const Belief& other);
	Belief& operator=(const Belief& other);

	
	virtual ~Belief();
	virtual Goal extract_best_goal() const;
	virtual void start();
	virtual void fill_tracker_session(belief_tracker::TrackerSessionTurn& tracker_session_turn);
	virtual double marginalize(std::string slot, std::string value) const;
	//friend std::ostream& operator<<(std::ostream &os, const Belief &b);
	void clean(double thr=1e-3);
      };

    


    namespace sum {

      class Belief : public belief_tracker::joint::Belief{
      public:
	Belief();
	virtual ~Belief();
	template<typename transition_function_type>
	  Belief update(const belief_tracker::info::TurnInfo& turn_info,
			const transition_function_type& transition_function) {

	  
	  if(turn_info.size() == 0)
	    throw std::logic_error("belief update : I received an empty info, that was not expected ! ");
	  
	  belief_tracker::joint::sum::Belief new_belief;
	  for(auto& bi: belief) {
	    for(auto& utterance_info : turn_info) {
	      for(auto& scored_info: utterance_info) {
		double score = scored_info.second;
		belief_tracker::Goal image = transition_function(bi.first, scored_info.first);

		auto ins = new_belief.belief.insert(std::make_pair(image, 0.0));
		ins.first->second += bi.second * score;
	      }
	    }
	  }
	  
	  return new_belief;
	}
      };

    }

    namespace max {

      class Belief : public belief_tracker::joint::Belief{
      public:
	Belief();
	virtual ~Belief();

	template<typename transition_function_type>
	Belief update(const belief_tracker::info::TurnInfo& turn_info, 
		      const transition_function_type& transition_function) {

	  if(turn_info.size() == 0)
	    throw std::logic_error("belief update : I received an empty info, that was not expected ! ");


	  belief_tracker::joint::max::Belief new_belief;

	  for(auto& bi: belief) {
	    for(auto& utterance_info : turn_info) {
	      
	      // We collect the images and sum the scores because
	      // if several informations from the same utterance lead to the same
	      // image, their scores must be sumed
	      std::map<Goal, double> images;
	      for(auto& scored_info: utterance_info) {
		double score = scored_info.second;
		belief_tracker::Goal image = transition_function(bi.first, scored_info.first);

		auto ins = images.insert(std::make_pair(image, 0.0));
		ins.first->second += bi.second * score;
	      }

	      // We now update the belief with the max
	      for(auto& image_score: images) {
		auto ins = new_belief.belief.insert(std::make_pair(image_score.first, 0.0));
		ins.first->second = std::max(ins.first->second, image_score.second);
	      }
	    }
	  }
	  return new_belief;
	}
      };
    }
  } // namespace joint

  /*
  namespace marginal {
    // a collection of lazy probability distributions
    // for e.g. Belief[0] for the slot area
    // Belief[0] : { {'south' : 0.1, 'dontcare': 0.5}, {0.4, 3} }
    // Any is always the last element
    typedef std::vector< std::pair< std::map<Goal::value_type, double>, std::pair<double, int> >> Belief;

    Goal extract_best_goal(const belief_tracker::marginal::Belief& b);

    Belief init(const belief_tracker::Ontology& ontology);

    Belief update(const belief_tracker::ScoredInfo& turn_info, 
		  const belief_tracker::marginal::Belief& belief_t,
		  std::function<belief_tracker::Goal(const belief_tracker::Goal&, const belief_tracker::Info&)>);


    void fill_tracker_session(belief_tracker::TrackerSessionTurn& tracker_session_turn, 
			      const belief_tracker::marginal::Belief& belief,
			      const belief_tracker::Ontology& ontology);

  }
  */

}
std::ostream & operator<<(std::ostream &os, const belief_tracker::joint::Belief &b);
//std::ostream & operator<<(std::ostream &os, const belief_tracker::marginal::Belief &b);
