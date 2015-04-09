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
#include <set>
#include <iostream>

namespace belief_tracker {

  
  template<typename ValueType, typename SubCollectionIter, typename OutputIterator>
  void expand(const SubCollectionIter& begin,
	      const SubCollectionIter& end,
	      OutputIterator& out) {
    if(begin == end) {
      *(out++) = ValueType();
      return;
    }

    //typedef typename std::iterator_traits<OutputIterator>::value_type content_type;
    std::vector<ValueType> res;
    auto res_iter = std::back_inserter(res);  
    auto it = begin;
    std::advance(it, 1);
    expand<ValueType>(it, end, res_iter);
  
    for(auto& vi: (*begin)) {
      for(auto& resi: res) {
	ValueType content;
	auto content_iter = std::back_inserter(content);
	*(content_iter++) = vi;
	std::copy(resi.begin(), resi.end(), content_iter);
	*(out++) = content;
      }
    } 
  }

  inline void test_expand(void) {
    std::vector< std::set<unsigned int> > collection = {{1,2,3}, {5,6} , {12}, {1,29,28}};
    std::vector< std::vector< unsigned int> > res;
    auto res_iter = std::back_inserter(res);
    expand<std::vector< unsigned int> >(collection.begin(), collection.end(), res_iter);
    
    std::cout << "res size :" << res.size() << std::endl;
    
    for(auto& resi: res) {
      for(auto& v: resi) 
	std::cout << v << " ";
      std::cout << std::endl;
    }    
  }

	      
 
  std::set< std::vector<unsigned int> > expand_multi_info(std::vector< std::set<unsigned int> > info);



  void test_expand_multi_info();
}
