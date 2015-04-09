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
#include <initializer_list>
#include <map>
#include "JSONObjects.h"
#include "constants.h"

namespace belief_tracker {

  class Goal {

  public:
    typedef unsigned int value_type;
    std::vector<value_type> slots_values;

    Goal();
  
    Goal(const std::initializer_list<value_type>& il);
    bool operator<(const Goal& other) const;

    std::string toStr() const;

    std::map<std::string, std::string> toStrVec() const;

  };

  std::ostream & operator<<(std::ostream &os, const belief_tracker::Goal &g);


  belief_tracker::Goal extract_1best_from_tracker_session(const belief_tracker::TrackerSessionTurn& tracker_session, bool use_joint=true);

}

