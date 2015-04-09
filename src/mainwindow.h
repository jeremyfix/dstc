/*
  This file is part of dstc-viewer.

  Copyright (C) 2014, Jeremy Fix, Supelec

  Author : Jeremy Fix

  dstc-viewer is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  any later version.

  dstc-viewer is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with dstc-viewer.  If not, see <http://www.gnu.org/licenses/>.

  Contact : Jeremy.Fix@supelec.fr
*/

#ifndef DSTC_MAINWINDOW_H
#define DSTC_MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QHBoxLayout>
#include "ontologyviewer.h"
#include "labelviewer.h"
#include "trackeroutputviewer.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    void closeEvent(QCloseEvent *event);
    ~MainWindow();
    
private:
    Ui::MainWindow *ui;

    OntologyViewer ontology_viewer;
    LabelViewer label_viewer;
    TrackerOutputViewer tracker_output_viewer;

    std::map<std::string, std::string> dialog_filelist;
    belief_tracker::Dialog current_dialog;
    belief_tracker::DialogLabels current_labels;

public slots:
    void openOntology();
    void openDialogs();
    void openTrackerOutput();

    void changeDialog(QString);

    void next_dialog(void);
    void prev_dialog(void);

    void sync_tracker_output_to_session(bool);

    void dialog_prefix_textChanged(QString text);

    void showTips(void);
};

#endif // MAINWINDOW_H
