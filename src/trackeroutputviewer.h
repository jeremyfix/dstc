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



#ifndef DSTC_TRACKEROUTPUTVIEWER_H
#define DSTC_TRACKEROUTPUTVIEWER_H

#include <QWidget>
#include <QString>
#include "JSONObjects.h"

namespace Ui {
  class TrackerOutputViewer;
}

class TrackerOutputViewer : public QWidget
{
  Q_OBJECT
    
 public:
  explicit TrackerOutputViewer(QWidget *parent = 0);
  ~TrackerOutputViewer();

  void loadData(std::string filename);
    
 signals:
  void sig_sync_to_dialog(bool);

  public slots:
    void new_dialog(QString);
    void sync_to_dialog(bool);

    // Updates the tree view given a session-id
    void updateTreeView(QString);

    // Slots to go the previous or next tracker output
    // when the viewer is not sync. to the mainwindow session-id
    void prev_output();
    void next_output();

 private:
    Ui::TrackerOutputViewer *ui;
    belief_tracker::TrackerOutput tracker_output;

    // Method to update the walltime, dataset and combo box elements
    void updateView(void);

};

#endif // TRACKEROUTPUTVIEWER_H
