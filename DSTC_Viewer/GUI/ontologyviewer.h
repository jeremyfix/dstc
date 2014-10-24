/*
  This file is part of dstc-viewer.

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
  along with dstc-viewer.  If not, see <http://www.gnu.org/licenses/>.

  Contact : Jeremy.Fix@supelec.fr
*/

#ifndef ONTOLOGYVIEWER_H
#define ONTOLOGYVIEWER_H

#include <QWidget>
#include <vector>
#include <string>
#include <map>
#include "JSONObjects.h"

namespace Ui {
    class OntologyViewer; // <-- this is the automatically created ui class which we need
}

class OntologyViewer : public QWidget
{
    Q_OBJECT

public:
    OntologyViewer();
    ~OntologyViewer();

    void loadData(std::string filename);

private:

    void updateTreeView();
    void updateInformableTreeView();

    Ui::OntologyViewer *ui;
    Ontology ontology;
};

#endif // ONTOLOGYVIEWER_H
