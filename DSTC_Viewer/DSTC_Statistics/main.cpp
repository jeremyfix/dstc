#include <iostream>
#include <fstream>
#include <list>
#include <libgen.h> // basename dirname
#include<cstring>

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

  std::string line;
  std::string diag_filename;
  std::string diag_fullpath;
  std::list< std::string > dialogs_fullpath;
  infile >> line;
  while(!infile.eof()){
    infile >> line;
    diag_fullpath = std::string(dirname(strdup(filename.c_str()))) + std::string("/../../data/") + line ;
    std::cout << diag_fullpath << std::endl;
    dialogs_fullpath.push_back(diag_fullpath);
  }
}
