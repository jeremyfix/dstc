Dialog State Tracking Challenge viewer
======================================

This application aims at providing an easy way to visualize the JSON data under the format of [Dialog State Tracking Challenge](http://camdial.org/~mh521/dstc/). It has developped while working in the [ANR MaRDi project](http://mardi.metz.supelec.fr)

Install
-------

To make use of the viewer, you need Qt to be installed. Then enter in the directory and type :

- qmake
- make

You can then run the GUI using :

- ./dstc_Viewer

Or compute statistics over the databases using :

- ./db_statistics filename.flist

If lost, check the help message.

Data
----

You can view :

- ontology files
- dialog and label files (providing a flist file listing the dialogs)
- tracker output files

All these files must fit the DSTC JSON format.

Statistics
----------

db_statistics allows you to compute statistics over the dialogs. It displays the statistics in the console and generate a LaTex file for it as well that you need to compile (calling pdflatex 2 times on it); Below are the computed statistics for DSTC-2 :

- [DSTC-2 training data](./dstc2_train.flist.pdf "DSTC-2 training")
- [DSTC-2 dev data](./dstc2_train.flist.pdf "DSTC-2 dev")
- [DSTC-2 test data](./dstc2_train.flist.pdf "DSTC-2 test")


Screenshots
-----------

![Viewer](./Pics/screenshot.png "The viewers of DSTC files")