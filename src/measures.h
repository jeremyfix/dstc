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

#include "belief.h"
#include <map>

namespace belief_tracker {
  int nb_differences_goal_to_label(const belief_tracker::Goal& goal, std::map< std::string, std::string > labels);
  //int nb_differences_beliefbest_to_label(const belief_tracker::Belief& belief, std::map< std::string, std::string > labels);

}
