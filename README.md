Dialog State Tracking Challenge viewer
======================================

This application aims at providing an easy way to visualize the JSON data under the format of [Dialog State Tracking Challenge](http://camdial.org/~mh521/dstc/).

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

Screenshots
-----------

![Viewer](./Pics/screenshot.png "The viewers of DSTC files")