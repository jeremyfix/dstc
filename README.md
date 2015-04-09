Dialog State Tracking Challenge viewer, statistics and Yarbus baseline
======================================================================

In this repository , you will find various tools in C++:

- a tool for computing statistics on the dialog acts in the Dialog State Tracking challenge datasets (dstc2, dstc3)
- a tool for viewing the data as well as tracker outputs (synchronized to the viewer of the dialog/label files)
- the scripts for running the Yarbus tracker : it has a dedicated page : 

It aims at providing an easy way to play with the JSON data under the format of [Dialog State Tracking Challenge](http://camdial.org/~mh521/dstc/). It has developped while working in the [ANR MaRDi project](http://mardi.metz.supelec.fr)

Install/Usage
-------------

- mkdir build
- cd build
- cmake ..
- make 

As the viewer requires Qt, it has been disabled by default. To allow its compilation, you have to activate it while running cmake : [Yarbus](http://jeremyfix.github.io/dstc "Yarbus page")

- cmake .. -DDSTC_Viewer=ON
- make

The project is not aimed to be installed anywhere, so set up your LD_LIBRARY_FLAGS correctly before running the scripts :
- export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./src


More about the Statistics
-------------------------

To compute the statistics

- cd CLONE_DIR/build
- export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./src
- ./src/statistics  dstc2_test.flist

statistics allows you to compute statistics over the dialogs. It displays the statistics in the console and generate a LaTex file for it as well that you need to compile (calling pdflatex 2 times on it); Below are the computed statistics for DSTC-2 :

- [DSTC-2 training data](./dstc2_train.flist.pdf "DSTC-2 training")
- [DSTC-2 dev data](./dstc2_dev.flist.pdf "DSTC-2 dev")
- [DSTC-2 test data](./dstc2_test.flist.pdf "DSTC-2 test")


More about the GUI
------------------


To run the GUI : 
- cd CLONE_DIR/build
- export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./src
- ./src/viewer 

You can view :

- ontology files
- dialog and label files (providing a flist file listing the dialogs)
- tracker output files

All these files must fit the DSTC JSON format.

![Viewer](./Pics/screenshot.png "The viewers of DSTC files")


