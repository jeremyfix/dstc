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

#ifndef DSTC_LABELVIEWER_H
#define DSTC_LABELVIEWER_H

#include <QWidget>
#include "JSONObjects.h"

namespace Ui {
class LabelViewer;
}

class LabelViewer : public QWidget
{
    Q_OBJECT
    
public:
    explicit LabelViewer(QWidget *parent = 0);
    ~LabelViewer();

    void loadData(std::string filename);
    
private:
    Ui::LabelViewer *ui;
    belief_tracker::DialogLabels labels;

    void updateView();
};

#endif // LABELVIEWER_H
