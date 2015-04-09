#include "tools.h"
#include <iostream>

std::set< std::vector<unsigned int> > belief_tracker::expand_multi_info(std::vector< std::set<unsigned int> > info) {
  std::set< std::vector<unsigned int> > single_infos;

  std::vector<unsigned int> sizes;
  unsigned int nb_hypothesis = 1;
  for(auto& info_i: info) {
    sizes.push_back(info_i.size());
    nb_hypothesis *= info_i.size();
  }

  for(unsigned int i = 0 ; i< nb_hypothesis ; ++i) {
    std::vector<unsigned int> multi_dim_index;
    multi_dim_index.resize(info.size());
    int raw_index = i;
    for(int j = multi_dim_index.size()-1 ; j >= 0  ; --j) {
      int index_j = raw_index % sizes[j];
      multi_dim_index[j] = index_j;
      raw_index /= sizes[j];
    }
    /*
    std::cout << "I transformed " << i << " into ";
    for(auto multi_dim_index_i: multi_dim_index)
      std::cout << multi_dim_index_i << " ";
    std::cout << std::endl;
    */

    // We now produce the info
    std::vector<unsigned int> single_info;
    single_info.resize(info.size());

    for(unsigned int j = 0 ; j < multi_dim_index.size() ; ++j) {
      std::set<unsigned int>::iterator it = info[j].begin();
      std::advance(it, multi_dim_index[j]);
      single_info[j] = *it;
    }
    single_infos.insert(single_info);
  }

  return single_infos;
}

void belief_tracker::test_expand_multi_info() {
  std::vector< std::set<unsigned int> > info;
  info.resize(3);
  info[0] = {{1, 2, 3}};
  info[1] = {{4, 5}};
  info[2] = {{3, 2, 0, 4}};
  auto res = expand_multi_info(info);
  std::cout << "I produced " << res.size() << " info, I was expecting " << info[0].size() * info[1].size() * info[2].size() << std::endl;

  for(auto& res_i: res) {
    for(auto& res_i_j: res_i)
      std::cout << res_i_j << " ";
    std::cout << std::endl;
  }
}
