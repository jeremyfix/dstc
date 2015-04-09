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

#include "JSONObjects.h"

namespace belief_tracker {

  /**
   * Performs the conversion between the slot/values as string and unsigned int representations
   * The values are represented as
   *  [<the positives>, <dontcare>, <the negatives>, <!dontcare>, unknown, weak_dontcare]
   **/
  class Converter {

  private:
    static belief_tracker::Ontology ontology;
    static std::vector<std::string> informable_slots;
    static std::map<std::string, unsigned int> informable_slots_index;
    static std::vector<unsigned int> informable_slots_values_size;


  public:
    static void init(const belief_tracker::Ontology& onto);

    static bool is_slot_informable(std::string slot_name);

    static unsigned int get_informable_slots_size(void);

    static unsigned int get_value_set_size(unsigned int slot_index);
    static unsigned int get_value_set_size(std::string slot);

    static unsigned int get_value(std::string slot, std::string value);
    static unsigned int get_value(unsigned int slot_index, std::string value);
    static std::string get_value(unsigned int slot_index, unsigned int value_index);
    static std::string get_value(std::string slot_name, unsigned int value_index);

    static std::string get_slot(unsigned int slot_index);
    static unsigned int get_slot(std::string slot);


    static bool is_negative(unsigned int slot_index,
			    unsigned int value_index);
    static bool is_positive(unsigned int slot_index,
			    unsigned int value_index);
    static bool is_weak_dontcare(unsigned int slot_index,
				 unsigned int value_index);
    static bool is_unknown(unsigned int slot_index,
			   unsigned int value_index);


    static unsigned int get_unknown(unsigned int slot_index);
    static unsigned int get_dontcare(unsigned int slot_index);
    static unsigned int get_weak_dontcare(unsigned int slot_index);

    static unsigned int negate(unsigned int slot_index,
			       unsigned int value_index);
    static unsigned int positivate(unsigned int slot_index,
				   unsigned int value_index);



  };
}
